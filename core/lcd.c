#include "lcd.h"
#include "cpu.h"
#include "emu.h"
#include "mem.h"
#include "asic.h"
#include "defines.h"
#include "control.h"
#include "schedule.h"
#include "interrupt.h"

#include <string.h>
#include <stdlib.h>

/* Global LCD state */
lcd_state_t lcd;

/* Set this callback function pointer from the GUI. Called in lcd_event() */
void (*lcd_gui_callback)(void*) = NULL;
void *lcd_gui_callback_data = NULL;

static bool _rgb;

#define c1555(w) ((w) + ((w) & 0xFFE0) + ((w) >> 10 & 0x20))
#define c565(w)  (((w) >> 8 & 0xF800) | ((w) >> 5 & 0x7E0) | ((w) >> 3 & 0x1F))
#define c12(w)   (((w) << 4 & 0xF000) | ((w) << 3 & 0x780) | ((w) << 1 & 0x1E))

static uint32_t lcd_bgr16out(uint32_t bgr16) {
    uint_fast8_t r, g, b;

    r = (bgr16 >> 10) & 0x3E;
    g = bgr16 >> 5 & 0x3F;
    b = (bgr16 << 1) & 0x3E;

    r |= r >> 5;
    r = (r << 2) | (r >> 4);

    g = (g << 2) | (g >> 4);

    b |= b >> 5;
    b = (b << 2) | (b >> 4);

    if (_rgb) {
        return r | (g << 8) | (b << 16) | (255 << 24);
    } else {
        return b | (g << 8) | (r << 16) | (255 << 24);
    }
}

/* Draw the lcd onto an RGBA8888 buffer. Alpha is always 255. */
void lcd_drawframe(void *output, void *data, void *data_end, uint32_t control, uint32_t size) {
    _rgb = control & (1 << 8);
    bool bebo = control & (1 << 9);
    uint_fast8_t mode = control >> 1 & 7;
    uint32_t word, color;
    uint32_t *out = output;
    uint32_t *out_end = out + size;
    uint32_t *dat = data;
    uint32_t *dat_end = data_end;

    if (!out) { return; }
    if (!dat) { goto draw_black; }

    if (mode < 4) {
        uint_fast8_t bpp = 1 << mode;
        uint32_t mask = (1 << bpp) - 1;
        uint_fast8_t bi = bebo ? 0 : 24;
        bool bepo = control & (1 << 10);
        if (!bepo) { bi ^= 8 - bpp; }
        do {
            uint_fast8_t bitpos = 32;
            word = *dat++;
            do {
                color = lcd.palette[word >> ((bitpos -= bpp) ^ bi) & mask];
                *out++ = lcd_bgr16out(c1555(color));
            } while (bitpos && out != out_end);
        } while (dat < dat_end);

    } else if (mode == 4) {
        do {
            word = *dat++;
            if (bebo) { word = word << 16 | word >> 16; }
            *out++ = lcd_bgr16out(c1555(word));
            if (out == out_end) break;
            word >>= 16;
            *out++ = lcd_bgr16out(c1555(word));
        } while (dat < dat_end);

    } else if (mode == 5) {
        do {
            word = *dat++;
            *out++ = lcd_bgr16out(c565(word));
        } while (dat < dat_end);

    } else if (mode == 6) {
        do {
            word = *dat++;
            if (bebo) { word = word << 16 | word >> 16; }
            *out++ = lcd_bgr16out(word);
            if (out == out_end) break;
            word >>= 16;
            *out++ = lcd_bgr16out(word);
        } while (dat < dat_end);

    } else { /* mode == 7 */
        do {
            word = *dat++;
            if (bebo) { word = word << 16 | word >> 16; }
            *out++ = lcd_bgr16out(c12(word));
            if (out == out_end) break;
            word >>= 16;
            *out++ = lcd_bgr16out(c12(word));
        } while (dat < dat_end);
    }

draw_black:
    while (out < out_end) { *out++ = 0xFF000000 | rand(); }
}

void lcd_gui_event(void) {
    if (lcd_gui_callback) {
        lcd_gui_callback(lcd_gui_callback_data);
    }
}

static uint32_t lcd_process_pixel(uint8_t red, uint8_t green, uint8_t blue) {
    uint32_t v, h, ticks = 1;
    if (likely(lcd.curRow < lcd.LPP)) {
        if (!likely(lcd.curCol)) {
            if (!likely(lcd.curRow)) {
                for (v = lcd.VBP; v; v--) {
                    for (h = lcd.HBP + lcd.CPL + lcd.HFP; h && spi_refresh_pixel(); h--) {
                    }
                    if (!spi_hsync()) {
                        break;
                    }
                }
            }
            for (h = lcd.HBP; h && spi_refresh_pixel(); h--) {
            }
        }
        spi_refresh_pixel();
        if (likely(lcd.curCol < lcd.PPL)) {
            if (!likely(lcd.control & 1 << 11)) {
                red = green = blue = 0;
            } else if (likely(lcd.BGR)) {
                uint8_t temp = red;
                red = blue;
                blue = temp;
            }
            spi_update_pixel(red, green, blue);
        }
        if (unlikely(++lcd.curCol >= lcd.CPL)) {
            for (h = lcd.HFP; h && spi_refresh_pixel(); h--) {
            }
            spi_hsync();
            if (unlikely(++lcd.curRow >= lcd.LPP)) {
                for (v = lcd.VFP; v; v--) {
                    for (h = lcd.HBP + lcd.CPL + lcd.HFP; h && spi_refresh_pixel(); h--) {
                    }
                    if (!spi_hsync()) {
                        break;
                    }
                }
            }
            lcd.curCol = 0;
            ticks += lcd.HFP + lcd.HSW + lcd.HBP;
        }
    }
    return ticks * lcd.PCD * 2;
}

static uint32_t lcd_process_half(uint16_t pixel) {
    switch (lcd.LCDBPP) {
        default: // 1555
            return lcd_process_pixel(pixel & 0x1F, (pixel >> 4 & 0x3E) | (pixel >> 15 & 1), pixel >> 10 & 0x1F);
        case 6: // 565
            return lcd_process_pixel(pixel & 0x1F, pixel >> 5 & 0x3F, pixel >> 11 & 0x1F);
        case 7: // 444
            return lcd_process_pixel(pixel << 1 & 0x1E, pixel >> 2 & 0x3C, pixel >> 7 & 0x1E);
    }
}

static uint32_t lcd_process_index(uint8_t index) {
    return lcd_process_half(lcd.palette[index]);
}

static void lcd_fill_bytes(uint8_t bytes) {
    mem_dma_cpy(&lcd.fifo[lcd.pos], lcd.upcurr, bytes);
    lcd.pos += bytes;
    lcd.upcurr += bytes;
}

static uint32_t lcd_drain_word(uint8_t *pos) {
    uint32_t word = 0;
    if (unlikely(lcd.BEBO)) {
        word |= lcd.fifo[(*pos)++] << 24;
        word |= lcd.fifo[(*pos)++] << 16;
        word |= lcd.fifo[(*pos)++] <<  8;
        word |= lcd.fifo[(*pos)++] <<  0;
    } else {
        word |= lcd.fifo[(*pos)++] <<  0;
        word |= lcd.fifo[(*pos)++] <<  8;
        word |= lcd.fifo[(*pos)++] << 16;
        word |= lcd.fifo[(*pos)++] << 24;
    }
    return word;
}

static uint32_t lcd_words(uint8_t words) {
    uint32_t ticks = 0;
    uint8_t pos = lcd.pos, bit, bpp = 1 << lcd.LCDBPP;
    while (words--) {
        uint32_t word = lcd_drain_word(&pos);
        if (unlikely(lcd.LCDBPP == 5)) {
            ticks += lcd_process_pixel(word >> 3 & 0x1F, word >> 10 & 0x3F, word >> 19 & 0x1F);
        } else if (unlikely(lcd.LCDBPP >= 4)) {
            ticks += lcd_process_half(word);
            ticks += lcd_process_half(word >> 16);
        } else {
            for (bit = 0; bit < 32; bit += bpp) {
                ticks += lcd_process_index(word >> (bit ^ (unlikely(lcd.BEPO) ? 8 - bpp : 0)) &
                                           ((1 << bpp) - 1));
            }
        }
    }
    return ticks;
}

static void lcd_event(enum sched_item_id id) {
    uint32_t duration;
    sched_process_pending_dma(0);
    if ((lcd.control >> 12 & 3) == lcd.compare) {
        lcd.ris |= 1 << 3;
    }
    switch (lcd.compare) {
        case LCD_FRONT_PORCH:
            if (lcd.VFP) {
                duration = lcd.VFP * (lcd.HSW + lcd.HBP + lcd.CPL + lcd.HFP) * lcd.PCD;
                lcd.compare = LCD_SYNC;
                break;
            }
        default:
        case LCD_SYNC:
            lcd_gui_event();
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
            if (lcd.spi) {
                lcd.pos = 0;
                lcd.curRow = lcd.curCol = 0;
                spi_vsync();
                sched_repeat_relative(SCHED_LCD_DMA, SCHED_LCD, duration, 0);
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
                duration = lcd.VBP * (lcd.HSW + lcd.HBP + lcd.CPL + lcd.HFP) * lcd.PCD;
                lcd.compare = LCD_ACTIVE_VIDEO;
                break;
            }
        case LCD_ACTIVE_VIDEO:
            duration = lcd.LPP * (lcd.HSW + lcd.HBP + lcd.CPL + lcd.HFP) * lcd.PCD;
            if (!lcd.prefill) {
                sched_repeat_relative(SCHED_LCD_DMA, SCHED_LCD, lcd.HSW + lcd.HBP, 0);
            }
            lcd.compare = LCD_FRONT_PORCH;
            break;
    }
    intrpt_set(INT_LCD, lcd.ris & lcd.imsc);
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
            sched_repeat_relative(SCHED_LCD_DMA, SCHED_LCD, lcd.HSW + lcd.HBP, 0);
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

void lcd_reset(void) {
    bool spi = lcd.spi;
    memset(&lcd, 0, sizeof(lcd_state_t));
    lcd.spi = spi;
    lcd_update();

    sched.items[SCHED_LCD].callback.event = lcd_event;
    sched.items[SCHED_LCD].clock = CLOCK_24M;
    sched_clear(SCHED_LCD);
    sched.items[SCHED_LCD_DMA].callback.dma = lcd_dma;
    sched.items[SCHED_LCD_DMA].clock = CLOCK_48M;
    sched_clear(SCHED_LCD_DMA);
    gui_console_printf("[CEmu] LCD reset.\n");
}

static uint8_t lcd_read(const uint16_t pio, bool peek) {
    uint16_t index = pio;
    uint8_t bit_offset = (index & 3) << 3;

    if (!peek) {
        sched_process_pending_dma(0);
    }

    if (index < 0x200) {
        if (index < 0x010) { return read8(lcd.timing[index >> 2], bit_offset); }
        if (index < 0x014 && index >= 0x010) { return read8(lcd.upbase, bit_offset); }
        if (index < 0x018 && index >= 0x014) { return read8(lcd.lpbase, bit_offset); }
        if (index < 0x01C && index >= 0x018) { return read8(lcd.control, bit_offset); }
        if (index < 0x020 && index >= 0x01C) { return read8(lcd.imsc, bit_offset); }
        if (index < 0x024 && index >= 0x020) { return read8(lcd.ris, bit_offset); }
        if (index < 0x028 && index >= 0x024) { return read8(lcd.imsc & lcd.ris, bit_offset); }
        if (index < 0x030 && index >= 0x02C) {
            if (!peek) {
                sched_process_pending_dma(0);
            }
            return read8(lcd.upcurr, bit_offset);
        }
        if (index < 0x034 && index >= 0x030) { return read8(lcd.lpcurr, bit_offset); }
    } else if (index < 0x400) {
        return *((uint8_t *)lcd.palette + index - 0x200);
    } else if (index < 0xC30) {
        if (index < 0xC00 && index >= 0x800) { return read8(lcd.crsrImage[((pio-0x800) & 0x3FF) >> 2], bit_offset); }
        if (index == 0xC00) { return read8(lcd.crsrControl, bit_offset); }
        if (index == 0xC04) { return read8(lcd.crsrConfig, bit_offset); }
        if (index < 0xC0C && index >= 0xC08) { return read8(lcd.crsrPalette0, bit_offset); }
        if (index < 0xC10 && index >= 0xC0C) { return read8(lcd.crsrPalette1, bit_offset); }
        if (index < 0xC14 && index >= 0xC10) { return read8(lcd.crsrXY, bit_offset); }
        if (index < 0xC16 && index >= 0xC14) { return read8(lcd.crsrClip, bit_offset); }
        if (index == 0xC20) { return read8(lcd.crsrImsc, bit_offset); }
        if (index == 0xC28) { return read8(lcd.crsrRis, bit_offset); }
        if (index == 0xC2C) { return read8(lcd.crsrRis & lcd.crsrImsc, bit_offset); }
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
    lcd_setptrs(&lcd.data, &lcd.data_end, LCD_WIDTH, LCD_HEIGHT, lcd.upbase, lcd.control, true);
}

void lcd_setptrs(uint32_t **dat, uint32_t **dat_end, uint32_t width, uint32_t height, uint32_t addr, uint32_t control, bool mask) {
    uint8_t mode = control >> 1 & 7;
    uint8_t *data_start, *data_end, *mem_end;
    uint32_t length = 0;
    uint32_t size;

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
        data_start = mem.ram.block + addr - 0xD00000;
    } else if (addr < 0xE30400 && addr >= 0xE30200) {
        mem_end = (uint8_t *)lcd.palette + sizeof lcd.palette;
        data_start = (uint8_t *)lcd.palette + addr - 0xE30200;
    } else if (addr < 0xE30C00 && addr >= 0xE30800) {
        mem_end = (uint8_t *)lcd.crsrImage + sizeof lcd.crsrImage;
        data_start = (uint8_t *)lcd.crsrImage + addr - 0xE30800;
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

    *dat     = (uint32_t *)data_start;
    *dat_end = (uint32_t *)data_end;
}

static void lcd_write(const uint16_t pio, const uint8_t value, bool poke) {
    uint16_t index = pio & 0xFFC;

    uint8_t byte_offset = pio & 3;
    uint8_t bit_offset = byte_offset << 3;

    uint32_t old;

    (void)poke;

    if (index < 0x200) {
        if (index < 0x010) {
            write8(lcd.timing[index >> 2], bit_offset, value);
        } else if (index < 0x014 && index >= 0x010) {
            write8(lcd.upbase, bit_offset, value);
            if (lcd.upbase & 7) {
                gui_console_printf("[CEmu] Warning: Aligning LCD panel\n");
            }
            lcd.upbase &= ~7U;
            lcd_update();
        } else if (index < 0x018 && index >= 0x014) {
            write8(lcd.lpbase, bit_offset, value);
            lcd.lpbase &= ~7U;
        } else if (index == 0x018) {
            old = lcd.control;
            write8(lcd.control, bit_offset, value);
            if ((lcd.control ^ old) & 1 << 0) { // lcdEn changed
                if (lcd.control & 1 << 0) {
                    lcd.compare = LCD_SYNC;
                    sched_set(SCHED_LCD, 0);
                } else {
                    sched_clear(SCHED_LCD);
                }
            }
        } else if (index == 0x01C) {
            write8(lcd.imsc, bit_offset, value);
            lcd.imsc &= 0x1E;
            intrpt_set(INT_LCD, lcd.ris & lcd.imsc);
        } else if (index == 0x028) {
            lcd.ris &= ~(value << bit_offset);
            intrpt_set(INT_LCD, lcd.ris & lcd.imsc);
        }
        lcd_update();
    } else if (index < 0x400) {
        write8(lcd.palette[pio >> 1 & 0xFF], (pio & 1) << 3, value);
    } else if (index < 0xC30) {
        if (index < 0xC00 && index >= 0x800) {
            write8(lcd.crsrImage[((pio-0x800) & 0x3FF) >> 2], bit_offset, value);
        }
        if (index == 0xC00) {
            write8(lcd.crsrControl, bit_offset, value);
        }
        if (index == 0xC04) {
            write8(lcd.crsrConfig, bit_offset, value);
            lcd.crsrConfig &= 0xF;
        }
        if (index < 0xC0B && index >= 0xC08) {
            write8(lcd.crsrPalette0, bit_offset, value);
        }
        if (index < 0xC0F && index >= 0xC0C) {
            write8(lcd.crsrPalette1, bit_offset, value);
        }
        if (index < 0xC14 && index >= 0xC10) {
            write8(lcd.crsrXY, bit_offset, value);
            lcd.crsrXY &= (0xFFF | (0xFFF << 16));
        }
        if (index < 0xC16 && index >= 0xC14) {
            write8(lcd.crsrClip, bit_offset, value);
            lcd.crsrClip &= (0x3F | (0x3F << 8));
        }
        if (index == 0xC20) {
            write8(lcd.crsrImsc, bit_offset, value);
            lcd.crsrImsc &= 0xF;
        }
        if (index == 0xC24) {
            lcd.crsrRis &= ~(value << bit_offset);
            lcd.crsrRis &= 0xF;
        }
    }
}

static const eZ80portrange_t device = {
    .read  = lcd_read,
    .write = lcd_write
};

eZ80portrange_t init_lcd(void) {
    gui_console_printf("[CEmu] Initialized LCD...\n");
    return device;
}

bool lcd_save(FILE *image) {
    return fwrite(&lcd, sizeof(lcd), 1, image) == 1;
}

bool lcd_restore(FILE *image) {
    bool ret = fread(&lcd, sizeof(lcd), 1, image) == 1;
    lcd_update();
    return ret;
}
