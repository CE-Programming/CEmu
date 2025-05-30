﻿#include "lcd.h"
#include "cpu.h"
#include "emu.h"
#include "mem.h"
#include "bus.h"
#include "asic.h"
#include "defines.h"
#include "control.h"
#include "schedule.h"
#include "interrupt.h"
#include "panel.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* Global LCD state */
lcd_state_t lcd;

#define c1555(w) (((w) & ~0x8000) + ((w) & 0xFFE07FE0) + ((w) >> 10 & 0x00200020))
#define c888(w)  (((w) >> 8 & 0xF800) | ((w) >> 5 & 0x7E0) | ((w) >> 3 & 0x1F))
#define c444(w)  (((w) << 4 & 0xF000F000) | ((w) << 3 & 0x07800780) | ((w) << 1 & 0x001E001E))

static inline uint32_t rotr32(uint32_t val, uint_fast8_t amount) {
    return (val >> amount) | (val << (-amount & 31));
}

static inline uint32_t lcd_next_word(uint32_t **dat) {
    uint32_t word = from_le32(**dat);
    (*dat)++;
    return word;
}

static inline uint32_t lcd_bgr565swap(uint32_t bgr565, uint32_t mask) {
    uint32_t diff = (bgr565 ^ (bgr565 >> 11)) & mask;
    return bgr565 ^ diff ^ (diff << 11);
}

static inline uint32_t lcd_bgr888swap(uint32_t bgr888, uint32_t mask) {
    uint32_t diff = (bgr888 ^ (bgr888 >> 16)) & mask;
    return bgr888 ^ diff ^ (diff << 16);
}

static inline uint32_t lcd_argb8888out(uint32_t argb8888) {
    return argb8888 | (argb8888 >> 5 & 0x040004) | (argb8888 >> 6 & 0x030303);
}

static inline uint32_t lcd_rgb565out(uint32_t rgb565) {
    return lcd_argb8888out(UINT32_C(0xFF000000) | (rgb565 << 8 & 0xF80000) | (rgb565 << 5 & 0xFC00) | (rgb565 << 3 & 0xF8));
}

static inline uint32_t lcd_rgb888out(uint32_t rgb888) {
    return lcd_argb8888out(UINT32_C(0xFF000000) | (rgb888 & 0xF8FCF8));
}

static inline void lcd_intrpt_check(void) {
    intrpt_set(INT_LCD, lcd.ris & lcd.imsc);
}

static inline void lcd_crsr_update(void) {
    if (lcd.compare != LCD_FRONT_PORCH) {
        return;
    }
    lcd_crsr_state_t *crsrRegs = &lcd.crsrRegs[lcd.crsrConfig >> 1 & 1];
    uint32_t crsrSize = ((lcd.crsrConfig & 1) + 1) * 32;
    uint32_t crsrClipX = crsrRegs->crsrClip & 0xFF;
    uint32_t crsrClipY = crsrRegs->crsrClip >> 8;
    uint32_t crsrRow = crsrRegs->crsrXY >> 16;
    uint32_t crsrRowUnclipped = crsrRow - crsrClipY;
    /* This value is treated as a signed 12-bit value for the cursor interrupt check */
    crsrRowUnclipped = (int32_t)(crsrRowUnclipped << 20) >> 20;
    uint32_t crsrRowOffset = lcd.curRow - crsrRowUnclipped;
    if (likely((int32_t)crsrRowOffset >= (int32_t)crsrSize)) {
        if (!likely(lcd.crsrIntrptd)) {
            lcd.crsrIntrptd = true;
            lcd.ris |= 1 << 0;
            lcd_intrpt_check();
        }
        lcd.curCrsrWidth = 0;
    } else if (!unlikely(lcd.crsrControl) || likely(lcd.curRow < crsrRow) || crsrClipX >= crsrSize) {
        lcd.curCrsrWidth = 0;
    } else {
        lcd.curCrsrOffset = ((crsrRegs->crsrImage * crsrSize + crsrRowOffset) * crsrSize + crsrClipX) & 0xFFF;
        lcd.curCrsrCol = crsrRegs->crsrXY & 0xFFFF;
        lcd.curCrsrWidth = crsrSize - crsrClipX;
    }
}

static inline uint8_t lcd_crsr_pixel(uint32_t crsrOffset) {
    return lcd.crsrImageBytes[crsrOffset >> 2] >> ((~crsrOffset & 3) * 2) & 3;
}

static void lcd_drawcrsr(void *output, uint32_t rowWidth, lcd_crsr_state_t *crsrRegs, bool crsrIsLarge, uint32_t size) {
    uint32_t crsrSize = (crsrIsLarge + 1) * 32;
    uint32_t crsrClipX = crsrRegs->crsrClip & 0xFF;
    uint32_t crsrClipY = crsrRegs->crsrClip >> 8;
    if ((crsrClipX | crsrClipY) >= crsrSize) {
        return;
    }
    uint32_t crsrCol = crsrRegs->crsrXY & 0xFFFF;
    if (crsrCol >= rowWidth) {
        return;
    }
    uint32_t crsrWidth = crsrSize - crsrClipX;
    if (crsrWidth > rowWidth - crsrCol) {
        crsrWidth = rowWidth - crsrCol;
    }
    uint32_t crsrHeight = crsrSize - crsrClipY;
    uint32_t crsrRow = crsrRegs->crsrXY >> 16;
    uint32_t crsrRowOffset = ((crsrRegs->crsrImage * crsrSize + crsrClipY) * crsrSize + crsrClipX) & 0xFFF;
    uint32_t *out = output;
    uint32_t outRowOffset = crsrRow * rowWidth + crsrCol;
    uint32_t crsrPalette[4] = {
        lcd_rgb888out(lcd_bgr888swap(lcd.crsrPalette0, 0xFF)),
        lcd_rgb888out(lcd_bgr888swap(lcd.crsrPalette1, 0xFF)),
        0x000000,
        0xFFFFFF
    };
    do {
        if (outRowOffset >= size) {
            return;
        }
        uint32_t i = crsrWidth;
        if (i > size - outRowOffset) {
            i = size - outRowOffset;
        }
        uint32_t crsrOffset = crsrRowOffset;
        uint32_t *outPtr = out + outRowOffset;
        do {
            uint8_t pixel = lcd_crsr_pixel(crsrOffset);
            *outPtr = (pixel & 2 ? *outPtr : 0) ^ crsrPalette[pixel];
            crsrOffset++;
            outPtr++;
        } while (--i);
        crsrRowOffset += crsrSize;
        outRowOffset += rowWidth;
    } while (--crsrHeight);
}

void emu_set_lcd_callback(bool (*callback)(void*), void *data) {
    lcd.gui_callback = callback;
    lcd.gui_callback_data = data;
}

void emu_set_lcd_dma(int enable) {
    lcd.useDma = enable;
}

void emu_set_lcd_gamma(int enable) {
    panel.accurateGamma = enable;
    panel.gammaDirty = true;
}

void emu_lcd_drawframe(void *output) {
    if (lcd.useDma) {
        memcpy(output, panel.display, sizeof(panel.display));
    } else if (lcd.control & 1 << 11) {
        emu_lcd_drawmem(output, lcd.data, lcd.data_end, lcd.control, LCD_SIZE);
        if (unlikely(lcd.crsrControl)) {
            lcd_drawcrsr(output, lcd.CPL, &lcd.crsrRegs[1], lcd.crsrConfig & 1, LCD_SIZE);
        }
    }
}

/* Draw the lcd onto an RGBA8888 buffer. Alpha is always 255. */
void emu_lcd_drawmem(void *output, void *data, void *data_end, uint32_t lcd_control, int size) {
    bool bebo;
    uint_fast8_t mode;
    uint32_t word, color, bgr;
    uint32_t *out;
    uint32_t *out_end;
    uint32_t *dat;
    uint32_t *dat_end;

    bgr = lcd_control & (1 << 8) ? 0 : 0x001F001F;
    bebo = lcd_control & (1 << 9);
    mode = lcd_control >> 1 & 7;
    out = output;
    out_end = out + size;
    dat = data;
    dat_end = data_end;

    if (!out) { return; }
    if (!dat) { goto draw_black; }

    if (mode < 4) {
        const uint16_t *palette = lcd.palettes[bgr & 1];
        uint_fast8_t bpp = 1u << mode;
        uint32_t mask = (1 << bpp) - 1;
        uint_fast8_t bi = bebo ^ (CEMU_BYTE_ORDER == CEMU_BIG_ENDIAN) ? 0 : 24;
        bool bepo = lcd_control & (1 << 10);
        if (!bepo) { bi ^= 8 - bpp; }
        do {
            uint_fast8_t bitpos = 32;
            word = *dat++;
            do {
                color = palette[word >> ((bitpos -= bpp) ^ bi) & mask];
                *out++ = lcd_rgb565out(color);
            } while (bitpos && out != out_end);
        } while (dat < dat_end);

    } else if (mode == 4) {
        uint_fast8_t bi = bebo ? 16 : 0;
        do {
            word = rotr32(lcd_next_word(&dat), bi);
            word = lcd_bgr565swap(c1555(word), bgr);
            *out++ = lcd_rgb565out(word);
            if (out == out_end) break;
            *out++ = lcd_rgb565out(word >> 16);
        } while (dat < dat_end);

    } else if (mode == 5) {
        bgr = bgr ? 0xFF : 0;
        do {
            word = lcd_next_word(&dat);
            word = lcd_bgr888swap(word, bgr);
            *out++ = lcd_rgb888out(word);
        } while (dat < dat_end);

    } else if (mode == 6) {
        uint_fast8_t bi = bebo ? 16 : 0;
        do {
            word = rotr32(lcd_next_word(&dat), bi);
            word = lcd_bgr565swap(word, bgr);
            *out++ = lcd_rgb565out(word);
            if (out == out_end) break;
            *out++ = lcd_rgb565out(word >> 16);
        } while (dat < dat_end);

    } else { /* mode == 7 */
        uint_fast8_t bi = bebo ? 16 : 0;
        do {
            word = rotr32(lcd_next_word(&dat), bi);
            word = lcd_bgr565swap(c444(word), bgr);
            *out++ = lcd_rgb565out(word);
            if (out == out_end) break;
            *out++ = lcd_rgb565out(word >> 16);
        } while (dat < dat_end);
    }

draw_black:
    while (out < out_end) { *out++ = 0xFF000000 | (unsigned int)(bus_rand() << (bus_rand() & 0x18)); }
}

bool lcd_gui_event(void) {
    if (lcd.gui_callback) {
        return lcd.gui_callback(lcd.gui_callback_data);
    }
    return true;
}

void lcd_free(void) {
    lcd.gui_callback = NULL;
    lcd.gui_callback_data = NULL;
}

static uint32_t lcd_process_pixel(uint32_t ticks, uint16_t bgr565) {
    uint32_t v;
    ticks += lcd.PCD * 2;
    if (likely(lcd.curRow < lcd.LPP)) {
        if (!likely(lcd.curCol)) {
            if (!likely(lcd.curRow)) {
                panel_clock_porch(lcd.HSW + lcd.HBP + lcd.CPL + lcd.HFP);
                for (v = (lcd.VSW - 1) + lcd.VBP; v; v--) {
                    if (!panel_hsync()) {
                        break;
                    }
                    panel_clock_porch(lcd.HSW + lcd.HBP + lcd.CPL + lcd.HFP);
                }
            }
            panel_hsync();
            panel_clock_porch(lcd.HSW + lcd.HBP);
            lcd_crsr_update();
        }

        if (unlikely(sched_active(SCHED_PANEL))) {
            panel_scan_until(sched_ticks_remaining_relative(SCHED_PANEL, SCHED_LCD_DMA, ticks));
        }
        assert(lcd.curCol < lcd.CPL);
        uint32_t crsrColOffset = lcd.curCol - lcd.curCrsrCol;
        if (unlikely(crsrColOffset < lcd.curCrsrWidth)) {
            uint8_t crsrPixel = lcd_crsr_pixel(lcd.curCrsrOffset + crsrColOffset);
            bgr565 = (crsrPixel & 2 ? bgr565 : 0) ^ lcd.crsrPalette[crsrPixel];
        }
        panel.clock_pixel(bgr565);

        if (unlikely(++lcd.curCol >= lcd.CPL)) {
            panel_clock_porch(lcd.HFP);
            lcd.curCol = 0;
            lcd.curRow++;
            if (unlikely(lcd.curRow >= lcd.LPP)) {
                for (v = lcd.VFP; v; v--) {
                    if (!panel_hsync()) {
                        break;
                    }
                    panel_clock_porch(lcd.HSW + lcd.HBP + lcd.CPL + lcd.HFP);
                }
            }
            ticks += (lcd.HFP + lcd.HSW + lcd.HBP) * lcd.PCD * 2;
        }
    }
    return ticks;
}

static inline void lcd_fill_bytes(uint8_t bytes) {
    assert((bytes % sizeof(uint32_t)) == 0);
    mem_dma_read(&lcd.fifo[lcd.pos / sizeof(uint32_t)], lcd.upcurr, bytes);
    lcd.pos += bytes;
    lcd.upcurr += bytes;
}

static uint32_t lcd_words(uint8_t words) {
    assert(words > 0);
    uint32_t ticks = 0;
    if (likely(lcd.control >> 11 & 1)) {
        uint32_t bgr = lcd.BGR ? 0x001F001F : 0;
        uint32_t *dat = &lcd.fifo[lcd.pos / sizeof(uint32_t)];
        switch (lcd.LCDBPP) {
            case 4: {
                uint_fast8_t bi = lcd.BEBO ? 16 : 0;
                do {
                    uint32_t word = rotr32(lcd_next_word(&dat), bi);
                    word = lcd_bgr565swap(c1555(word), bgr);
                    ticks = lcd_process_pixel(ticks, word);
                    ticks = lcd_process_pixel(ticks, word >> 16);
                } while (--words);
                break;
            }

            case 5:
                do {
                    uint32_t word = lcd_next_word(&dat);
                    word = lcd_bgr565swap(c888(word), bgr);
                    ticks = lcd_process_pixel(ticks, word);
                } while (--words);
                break;

            case 6: {
                uint_fast8_t bi = lcd.BEBO ? 16 : 0;
                do {
                    uint32_t word = rotr32(lcd_next_word(&dat), bi);
                    word = lcd_bgr565swap(word, bgr);
                    ticks = lcd_process_pixel(ticks, word);
                    ticks = lcd_process_pixel(ticks, word >> 16);
                } while (--words);
                break;
            }

            case 7: {
                uint_fast8_t bi = lcd.BEBO ? 16 : 0;
                do {
                    uint32_t word = rotr32(lcd_next_word(&dat), bi);
                    word = lcd_bgr565swap(c444(word), bgr);
                    ticks = lcd_process_pixel(ticks, word);
                    ticks = lcd_process_pixel(ticks, word >> 16);
                } while (--words);
                break;
            }

            default: {
                const uint16_t *palette = lcd.palettes[bgr & 1];
                uint_fast8_t bpp = 1 << lcd.LCDBPP;
                uint_fast8_t mask = (1 << bpp) - 1;
                uint_fast8_t bi = (lcd.BEBO ^ (CEMU_BYTE_ORDER == CEMU_BIG_ENDIAN) ? 0 : 24) ^ (lcd.BEPO ? 0 : 8 - bpp);
                do {
                    uint_fast8_t bitpos = 32;
                    uint32_t word = *dat++;
                    do {
                        uint16_t pixel = palette[word >> ((bitpos -= bpp) ^ bi) & mask];
                        ticks = lcd_process_pixel(ticks, pixel);
                    } while (bitpos);
                } while (--words);
                break;
            }
        }
    } else {
        uint32_t pixels = words << (5 - lcd.BPP);
        do {
            ticks = lcd_process_pixel(ticks, 0);
        } while (--pixels);
    }
    return ticks;
}

static void lcd_event(enum sched_item_id id) {
    uint32_t duration;
    sched_process_pending_dma(0);
    enum lcd_comp compare = lcd.control >> 12 & 3;
    switch (lcd.compare) {
        case LCD_FRONT_PORCH:
            lcd.ris |= !lcd.crsrIntrptd << 0;
            if (lcd.VFP) {
                if (compare == LCD_FRONT_PORCH) {
                    lcd.ris |= 1 << 3;
                }
                duration = lcd.VFP * (lcd.HSW + lcd.HBP + lcd.CPL + lcd.HFP) * lcd.PCD;
                lcd.compare = LCD_SYNC;
                break;
            }
            fallthrough;
        default:
            fallthrough;
        case LCD_SYNC:
            if (compare == LCD_SYNC) {
                lcd.ris |= 1 << 3;
            }
            lcd.PPL =  ((lcd.timing[0] >>  2 &  0x3F) + 1) << 4;
            lcd.HSW =   (lcd.timing[0] >>  8 &  0xFF) + 1;
            lcd.HFP =   (lcd.timing[0] >> 16 &  0xFF) + 1;
            lcd.HBP =   (lcd.timing[0] >> 24 &  0xFF) + 1;
            lcd.LPP =   (lcd.timing[1] >>  0 & 0x3FF) + 1;
            lcd.VSW =   (lcd.timing[1] >> 10 &  0x3F) + 1;
            lcd.VFP =    lcd.timing[1] >> 16 &  0xFF;
            lcd.VBP =    lcd.timing[1] >> 24 &  0xFF;
            lcd.PCD =  ((lcd.timing[2] >>  0 &  0x1F) |
                        (lcd.timing[2] >> 27 &  0x1F) << 5) + 2;
            lcd.CLKSEL = lcd.timing[2] >>  5 &     1;
            lcd.ACB =   (lcd.timing[2] >>  6 &  0x1F) + 1;
            lcd.IVS =    lcd.timing[2] >> 11 &     1;
            lcd.IHS =    lcd.timing[2] >> 12 &     1;
            lcd.IPC =    lcd.timing[2] >> 13 &     1;
            lcd.IOE =    lcd.timing[2] >> 14 &     1;
            lcd.CPL =   (lcd.timing[2] >> 16 & 0x3FF) + 1;
            if (lcd.timing[2] >> 26 & 1) {
                lcd.PCD = 1;
            }
            lcd.LED =   (lcd.timing[3] >>  0 &  0x7F) + 1;
            lcd.LEE =    lcd.timing[3] >> 16 &     1;
            lcd.LCDBPP = lcd.control   >>  1 &     7;
            lcd.BGR =    lcd.control   >>  8 &     1;
            lcd.BEBO =   lcd.control   >>  9 &     1;
            lcd.BEPO =   lcd.control   >> 10 &     1;
            lcd.WTRMRK = lcd.control   >> 16 &     1;
            lcd.BPP = lcd.LCDBPP <= 5 ? lcd.LCDBPP : 4;
            lcd.PPF = 1 << (8 + lcd.WTRMRK - lcd.BPP);
            duration = ((lcd.VSW - 1) * (lcd.HSW + lcd.HBP + lcd.CPL + lcd.HFP) +
                        lcd.HSW) * lcd.PCD + 1;
            lcd.prefill = true;
            lcd.crsrIntrptd = false;
            lcd.crsrRegs[1] = lcd.crsrRegs[0];
            if (lcd.useDma) {
                lcd.pos = 0;
                lcd.curRow = lcd.curCol = 0;
                panel_vsync();
                sched_repeat_relative(SCHED_LCD_DMA, SCHED_LCD, duration, 0);
            } else {
                (void)lcd_gui_event();
                panel.skipFrame = true;
            }
            lcd.compare = LCD_LNBU;
            break;
        case LCD_LNBU:
            lcd.ris |= 1 << 2;
            duration = (lcd.HBP + lcd.CPL + lcd.HFP) * lcd.PCD - 1;
            lcd.compare = LCD_BACK_PORCH;
            break;
        case LCD_BACK_PORCH:
            if (lcd.VBP) {
                if (compare == LCD_BACK_PORCH) {
                    lcd.ris |= 1 << 3;
                }
                duration = lcd.VBP * (lcd.HSW + lcd.HBP + lcd.CPL + lcd.HFP) * lcd.PCD;
                lcd.compare = LCD_ACTIVE_VIDEO;
                break;
            }
            fallthrough;
        case LCD_ACTIVE_VIDEO:
            if (compare == LCD_ACTIVE_VIDEO) {
                lcd.ris |= 1 << 3;
            }
            duration = lcd.LPP * (lcd.HSW + lcd.HBP + lcd.CPL + lcd.HFP) * lcd.PCD;
            if (!lcd.prefill) {
                sched_repeat_relative(SCHED_LCD_DMA, SCHED_LCD, (lcd.HSW + lcd.HBP) * lcd.PCD, 0);
            }
            lcd.compare = LCD_FRONT_PORCH;
            break;
    }
    lcd_intrpt_check();
    sched_repeat(id, duration);
}

static uint32_t lcd_dma(enum sched_item_id id) {
    uint32_t ticks;
    if (unlikely(lcd.prefill)) {
        if (!lcd.pos) {
            lcd.upcurr = lcd.upbase;
        }
        lcd_fill_bytes(64);
        if ((lcd.prefill = lcd.pos)) {
            sched_repeat(id, lcd.pos == 128 ? 22 : 19);
        } else if (lcd.compare == LCD_FRONT_PORCH) {
            sched_repeat_relative(SCHED_LCD_DMA, SCHED_LCD, (lcd.HSW + lcd.HBP) * lcd.PCD, 0);
        }
        return lcd.pos & 64 ? 18 : 19;
    }
    ticks = lcd_words(lcd.WTRMRK ? 16 : 8);
    lcd_fill_bytes(lcd.WTRMRK ? 64 : 32);
    if (lcd.curRow < lcd.LPP) {
        sched_repeat(id, ticks);
    }
    return lcd.WTRMRK ? 19 : 11;
}

static void lcd_init_events(void) {
    sched_init_event(SCHED_LCD, CLOCK_24M, lcd_event);
    sched_init_dma(SCHED_LCD_DMA, CLOCK_48M, lcd_dma);
}

void lcd_reset(void) {
    memset(&lcd, 0, offsetof(lcd_state_t, useDma));
    lcd.crsrPalette[3] = 0xFFFF;
    lcd_update();
    lcd_init_events();
    gui_console_printf("[CEmu] LCD reset.\n");
}

static uint8_t lcd_read(const uint16_t pio, bool peek) {
    uint16_t index = pio;
    uint8_t bit_offset = (index & 3) << 3;

    if (index < 0x200) {
        if (index < 0x010) { return read8(lcd.timing[index >> 2], bit_offset); }
        if (index < 0x014 && index >= 0x010) { return read8(lcd.upbase, bit_offset); }
        if (index < 0x018 && index >= 0x014) { return read8(lcd.lpbase, bit_offset); }
        if (index < 0x01C && index >= 0x018) { return read8(lcd.control, bit_offset); }
        if (index == 0x01C) { return lcd.imsc & ~1; }
        if (index == 0x020) { return lcd.ris & ~1; }
        if (index == 0x024) { return lcd.imsc & lcd.ris & ~1; }
        if (index < 0x030 && index >= 0x02C) {
            if (!peek) {
                sched_process_pending_dma(0);
            }
            return read8(lcd.upcurr, bit_offset);
        }
        if (index < 0x034 && index >= 0x030) { return read8(lcd.lpcurr, bit_offset); }
    } else if (index < 0x400) {
        if (!peek) {
            cpu.cycles++;
        }
        return lcd.paletteBytes[index - 0x200];
    } else if (index < 0xC00) {
        if (index >= 0x800) { return lcd.crsrImageBytes[index - 0x800]; }
    } else if (index < 0xE00) {
        if (!peek) {
            sched_rewind_cpu(1); /* safely handle underflow */
        }
        if (index == 0xC00) { return lcd.crsrControl | (lcd.crsrRegs[0].crsrImage << 4); }
        if (index == 0xC04) { return lcd.crsrConfig; }
        if (index < 0xC0C && index >= 0xC08) { return read8(lcd.crsrPalette0, bit_offset); }
        if (index < 0xC10 && index >= 0xC0C) { return read8(lcd.crsrPalette1, bit_offset); }
        if (index < 0xC14 && index >= 0xC10) { return read8(lcd.crsrRegs[0].crsrXY, bit_offset); }
        if (index < 0xC16 && index >= 0xC14) { return read8(lcd.crsrRegs[0].crsrClip, bit_offset); }
        if (index == 0xC20) { return lcd.imsc & 1; }
        if (index == 0xC28) { return lcd.ris & 1; }
        if (index == 0xC2C) { return lcd.ris & lcd.imsc & 1; }
    } else if (index >= 0xFE0) {
        static const uint8_t id[1][8] = {
            { 0x11, 0x11, 0x14, 0x00, 0x0D, 0xF0, 0x05, 0xB1 }
        };
        return read8(id[0][(index - 0xFE0) >> 2], bit_offset);
    }

    /* Return 0 if bad read */
    return 0;
}

void lcd_disable(void) {
    lcd.data = NULL;
}

void lcd_update(void) {
    emu_set_lcd_ptrs(&lcd.data, &lcd.data_end, LCD_WIDTH, LCD_HEIGHT, lcd.upbase, lcd.control, true);
}

void emu_set_lcd_ptrs(uint32_t **dat, uint32_t **dat_end, int width, int height, uint32_t addr, uint32_t lcd_control, bool mask) {
    uint8_t mode = lcd_control >> 1 & 7;
    uint8_t *data_start, *data_end, *mem_end;
    int length = 0;
    int size;

    *dat = NULL;
    *dat_end = NULL;
    size = width * height;

    if (!size) { return; }

    /* Mask if true lcd */
    if (mask) {
        addr &= 0x7FFFF;
        addr |= 0xD00000;
    }

    if (addr < 0xD00000) {
        mem_end = mem.flash.block + SIZE_FLASH;
        data_start = mem.flash.block + addr;
    } else if (addr < 0xE00000) {
        mem_end = mem.ram.block + SIZE_RAM;
        data_start = mem.ram.block + (addr - 0xD00000);
    } else if (addr < 0xE30400 && addr >= 0xE30200) {
        mem_end = lcd.paletteBytes + sizeof lcd.paletteBytes;
        data_start = lcd.paletteBytes + (addr - 0xE30200);
    } else if (addr < 0xE30C00 && addr >= 0xE30800) {
        mem_end = lcd.crsrImageBytes + sizeof lcd.crsrImageBytes;
        data_start = lcd.crsrImageBytes + (addr - 0xE30800);
    } else {
        return;
    }

    switch (mode) {
        case 0: length = size >> 3; break;
        case 1: length = size >> 2; break;
        case 2: length = size >> 1; break;
        case 3: length = size >> 0; break;
        case 4: length = size << 1; break;
        case 5: length = size << 2; break;
        case 6: length = size << 1; break;
        case 7: length = (size >> 1) + size; break;
    }

    if (data_start >= mem_end) { return; }
    data_end = data_start + length;
    if (data_end > mem_end) { data_end = mem_end; }

    *dat     = (uint32_t*)data_start;
    *dat_end = (uint32_t*)data_end;
}

static void lcd_write_ctrl_delay() {
    switch (control.cpuSpeed) {
        case 0:
            cpu.cycles += (10 - 2);
            break;
        case 1:
            cpu.cycles += (12 - 2);
            break;
        case 2:
            cpu.cycles += asic.serFlash ? (14 - 2) : (16 - 2);
            break;
        case 3:
            if (asic.serFlash) {
                cpu.cycles += (23 - 2);
            } else {
                cpu.cycles += (21 - 2);
                /* Align CPU to LCD clock */
                cpu.cycles |= 1;
            }
            break;
    }
}

static void lcd_write_crsr_delay() {
    switch (control.cpuSpeed) {
        case 0:
            cpu.cycles += (9 - 2);
            break;
        case 1:
            cpu.cycles += asic.serFlash ? (9 - 2) : (11 - 2);
            break;
        case 2:
            cpu.cycles += asic.serFlash ? (11 - 2) : (13 - 2);
            break;
        case 3:
            if (asic.serFlash) {
                cpu.cycles += (14 - 2);
            } else {
                cpu.cycles += (16 - 2);
                /* Align CPU to LCD clock */
                cpu.cycles |= 1;
            }
            break;
    }
}

static void lcd_write(const uint16_t pio, const uint8_t value, bool poke) {
    uint16_t index = pio & 0xFFC;

    uint8_t byte_offset = pio & 3;
    uint8_t bit_offset = byte_offset << 3;

    uint32_t old;

    if (index < 0x200) {
        if (index < 0x010) {
            write8(lcd.timing[index >> 2], bit_offset, value);
            if (!poke) {
                lcd_write_ctrl_delay();
            }
        } else if (index == 0x010) {
            write8(lcd.upbase, bit_offset, value);
            if (lcd.upbase & 7) {
                gui_console_err_printf("[CEmu] Warning: Aligning LCD panel\n");
            }
            lcd.upbase &= ~7U;
            lcd_update();
        } else if (index == 0x014) {
            write8(lcd.lpbase, bit_offset, value);
            lcd.lpbase &= ~7U;
        } else if (index == 0x018) {
            if (!poke) {
                lcd_write_ctrl_delay();
                sched_process_pending_dma(0);
            }
            old = lcd.control;
            write8(lcd.control, bit_offset, value);
            if ((lcd.control ^ old) & 1 << 0) { /* lcdEn changed */
                if (lcd.control & 1 << 0) {
                    lcd.compare = LCD_SYNC;
                    sched_set(SCHED_LCD, 0);
                } else {
                    sched_clear(SCHED_LCD);
                }
            }
        } else if (index == 0x01C) {
            if (likely(bit_offset == 0)) {
                lcd.imsc &= ~0x1E;
                lcd.imsc |= value & 0x1E;
                lcd_intrpt_check();
            }
        } else if (index == 0x028) {
            if (likely(bit_offset == 0)) {
                lcd.ris &= ~(value & 0x1E);
                lcd_intrpt_check();
            }
        }
        lcd_update();
    } else if (index < 0x400) {
        if (!poke) {
            cpu.cycles += (4 - 2);
            sched_process_pending_dma(0);
        }
        uint16_t paletteIndex = pio - 0x200;
        if (lcd.paletteBytes[paletteIndex] != value) {
            lcd.paletteBytes[paletteIndex] = value;
            /* Convert to RGB565 in native endianness */
            paletteIndex >>= 1;
            uint16_t color = lcd.paletteBytes[paletteIndex * 2] | (lcd.paletteBytes[paletteIndex * 2 + 1] << 8);
            uint16_t bgr565 = color + (color & 0xFFE0) + (color >> 10 & 0x0020);
            lcd.palettes[0][paletteIndex] = bgr565;
            lcd.palettes[1][paletteIndex] = lcd_bgr565swap(bgr565, 0x1F);
        }
    } else if (index < 0xC00) {
        if (index >= 0x800) {
            if (!poke && unlikely(lcd.crsrControl)) {
                sched_process_pending_dma(0);
            }
            lcd.crsrImageBytes[pio - 0x800] = value;
        }
    } else if (index < 0xE00) {
        if (!poke) {
            lcd_write_crsr_delay();
            if (index < 0xC18) {
                sched_process_pending_dma(0);
            }
        }
        if (index == 0xC00) {
            if (likely(bit_offset == 0)) {
                lcd.crsrControl = value & 1;
                lcd.crsrRegs[0].crsrImage = value >> 4 & 3;
                lcd_crsr_update();
            }
        } else if (index == 0xC04) {
            if (likely(bit_offset == 0)) {
                lcd.crsrConfig = value & 3;
                lcd_crsr_update();
            }
        } else if (index == 0xC08) {
            if (likely(bit_offset < 24)) {
                write8(lcd.crsrPalette0, bit_offset, value);
                lcd.crsrPalette[0] = c888(lcd.crsrPalette0);
            }
        } else if (index == 0xC0C) {
            if (likely(bit_offset < 24)) {
                write8(lcd.crsrPalette1, bit_offset, value);
                lcd.crsrPalette[1] = c888(lcd.crsrPalette1);
            }
        } else if (index == 0xC10) {
            write8(lcd.crsrRegs[0].crsrXY, bit_offset, value);
            lcd.crsrRegs[0].crsrXY &= (0xFFF | (0xFFF << 16));
            lcd_crsr_update();
        } else if (index == 0xC14) {
            write8(lcd.crsrRegs[0].crsrClip, bit_offset, value);
            lcd.crsrRegs[0].crsrClip &= (0x3F | (0x3F << 8));
            lcd_crsr_update();
        } else if (index == 0xC20) {
            if (likely(bit_offset == 0)) {
                lcd.imsc &= ~1;
                lcd.imsc |= value & 1;
                lcd_intrpt_check();
            }
        } else if (index == 0xC24) {
            if (likely(bit_offset == 0)) {
                lcd.ris &= ~(value & 1);
                lcd_intrpt_check();
            }
        }
    }
}

static const eZ80portrange_t device = {
    .read  = lcd_read,
    .write = lcd_write
};

eZ80portrange_t init_lcd(void) {
    memset(&lcd, 0, offsetof(lcd_state_t, useDma));
    gui_console_printf("[CEmu] Initialized LCD...\n");
    return device;
}

bool lcd_save(FILE *image) {
    return fwrite(&lcd, offsetof(lcd_state_t, data), 1, image) == 1;
}

bool lcd_restore(FILE *image) {
    lcd_init_events();
    bool ret = fread(&lcd, offsetof(lcd_state_t, data), 1, image) == 1;
    lcd_update();
    return ret;
}
