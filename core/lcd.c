#include <string.h>
#include <stdio.h>

#include "lcd.h"
#include "cpu.h"
#include "emu.h"
#include "mem.h"
#include "asic.h"
#include "defines.h"
#include "control.h"
#include "schedule.h"
#include "interrupt.h"

/* Global LCD state */
lcd_state_t lcd;

/* Set this callback function pointer from the GUI. Called in lcd_event() */
void *lcd_event_callback_data = NULL;
void (*lcd_event_callback)(void*) = NULL;

static bool _rgb;

#define c1555(w) ((w) + ((w) & 0xFFE0) + ((w) >> 10 & 0x20))
#define c565(w)  (((w) >> 8 & 0xF800) | ((w) >> 5 & 0x7E0) | ((w) >> 3 & 0x1F))
#define c12(w)   (((w) << 4 & 0xF000) | ((w) << 3 & 0x780) | ((w) << 1 & 0x1E))

#ifdef LCD_RGBA8888
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
void lcd_drawframe(lcd_state_t *buffer) {
    uint_fast8_t mode = buffer->control >> 1 & 7;
    _rgb = buffer->control & (1 << 8);
    bool bebo = buffer->control & (1 << 9);
    uint32_t word, color;
    uint32_t *ofs = buffer->ofs;
    uint32_t *ofs_end = buffer->ofs_end;
    uint32_t *out = buffer->frame;
    uint32_t *out_end = out + buffer->size;

    if (!buffer->size) { return; }
    if (!ofs) { goto draw_black; }

    if (mode < 4) {
        uint_fast8_t bpp = 1 << mode;
        uint32_t mask = (1 << bpp) - 1;
        uint_fast8_t bi = bebo ? 0 : 24;
        bool bepo = buffer->control & (1 << 10);
        if (!bepo) { bi ^= 8 - bpp; }
        do {
            uint_fast8_t bitpos = 32;
            word = *ofs++;
            do {
                color = lcd.palette[word >> ((bitpos -= bpp) ^ bi) & mask];
                *out++ = lcd_bgr16out(c1555(color));
            } while (bitpos && out != out_end);
        } while (ofs < ofs_end);

    } else if (mode == 4) {
        do {
            word = *ofs++;
            if (bebo) { word = word << 16 | word >> 16; }
            *out++ = lcd_bgr16out(c1555(word));
            if (out == out_end) break;
            word >>= 16;
            *out++ = lcd_bgr16out(c1555(word));
        } while (ofs < ofs_end);

    } else if (mode == 5) {
        do {
            word = *ofs++;
            *out++ = lcd_bgr16out(c565(word));
        } while (ofs < ofs_end);

    } else if (mode == 6) {
        do {
            word = *ofs++;
            if (bebo) { word = word << 16 | word >> 16; }
            *out++ = lcd_bgr16out(word);
            if (out == out_end) break;
            word >>= 16;
            *out++ = lcd_bgr16out(word);
        } while (ofs < ofs_end);

    } else { /* mode == 7 */
        do {
            word = *ofs++;
            if (bebo) { word = word << 16 | word >> 16; }
            *out++ = lcd_bgr16out(c12(word));
            if (out == out_end) break;
            word >>= 16;
            *out++ = lcd_bgr16out(c12(word));
        } while (ofs < ofs_end);
    }

draw_black:
    while (out < out_end) { *out++ = 0xFF000000; }
}
#else
static void lcd_bgr16out(uint32_t bgr16, uint8_t **out) {
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
        *(*out)++ = r;
        *(*out)++ = g;
        *(*out)++ = b;
    } else {
        *(*out)++ = b;
        *(*out)++ = g;
        *(*out)++ = r;
    }
}

/* Draw the lcd onto an RGB888 buffer. */
void lcd_drawframe(lcd_state_t *buffer) {
    uint_fast8_t mode = buffer->control >> 1 & 7;
    _rgb = buffer->control & (1 << 8);
    bool bebo = buffer->control & (1 << 9);
    uint32_t word, color;
    uint32_t *ofs = buffer->ofs;
    uint32_t *ofs_end = buffer->ofs_end;
    uint32_t *out = buffer->frame;
    uint32_t *out_end = out + buffer->size - ((uintptr_t)(out + buffer->size) >> 2);

    if (!buffer->size) { return; }
    if (!ofs) { goto draw_black; }

    if (mode < 4) {
        uint_fast8_t bpp = 1 << mode;
        uint32_t mask = (1 << bpp) - 1;
        uint_fast8_t bi = bebo ? 0 : 24;
        bool bepo = buffer->control & (1 << 10);
        if (!bepo) { bi ^= 8 - bpp; }
        do {
            uint_fast8_t bitpos = 32;
            word = *ofs++;
            do {
                color = lcd.palette[word >> ((bitpos -= bpp) ^ bi) & mask];
                lcd_bgr16out(c1555(color), (uint8_t**)&out);
            } while (bitpos && out != out_end);
        } while (ofs < ofs_end);

    } else if (mode == 4) {
        do {
            word = *ofs++;
            if (bebo) { word = word << 16 | word >> 16; }
            lcd_bgr16out(c1555(word), (uint8_t**)&out);
            if (out == out_end) break;
            word >>= 16;
            lcd_bgr16out(c1555(word), (uint8_t**)&out);
        } while (ofs < ofs_end);

    } else if (mode == 5) {
        do {
            word = *ofs++;
            lcd_bgr16out(c565(word), (uint8_t**)&out);
        } while (ofs < ofs_end);

    } else if (mode == 6) {
        do {
            word = *ofs++;
            if (bebo) { word = word << 16 | word >> 16; }
            lcd_bgr16out(word, (uint8_t**)&out);
            if (out == out_end) break;
            word >>= 16;
            lcd_bgr16out(word, (uint8_t**)&out);
        } while (ofs < ofs_end);

    } else { /* mode == 7 */
        do {
            word = *ofs++;
            if (bebo) { word = word << 16 | word >> 16; }
            lcd_bgr16out(c12(word), (uint8_t**)&out);
            if (out == out_end) break;
            word >>= 16;
            lcd_bgr16out(c12(word), (uint8_t**)&out);
        } while (ofs < ofs_end);
    }

draw_black:
    while (out < out_end) { *out++ = 0xFF000000; }
}
#endif

static void lcd_event(enum sched_item_id id) {
    uint32_t duration;
    if ((lcd.control >> 12 & 3) == lcd.compare) {
        lcd.ris |= 1 << 3;
    }
    switch (lcd.compare) {
        default:
        case LCD_SYNC:
            if (lcd_event_callback) {
                lcd_event_callback(lcd_event_callback_data);
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
            duration = ((lcd.VSW - 1) * (lcd.HSW + lcd.HBP + lcd.CPL + lcd.HFP) +
                        lcd.HSW) * lcd.PCD + 1;
            lcd.compare = LCD_LNBU;
            break;
        case LCD_LNBU:
            lcd.upcurr = lcd.upbase;
            lcd.ris |= 1 << 2;
            duration = (lcd.HBP + lcd.CPL + lcd.HFP) * lcd.PCD - 1;
            lcd.compare = LCD_BACK_PORCH;
            break;
        case LCD_BACK_PORCH:
            duration = lcd.VBP * (lcd.HSW + lcd.HBP + lcd.CPL + lcd.HFP) * lcd.PCD;
            lcd.compare = LCD_ACTIVE_VIDEO;
            break;
        case LCD_ACTIVE_VIDEO:
            duration = lcd.LPP * (lcd.HSW + lcd.HBP + lcd.CPL + lcd.HFP) * lcd.PCD;
            lcd.compare = LCD_FRONT_PORCH;
            break;
        case LCD_FRONT_PORCH:
            duration = lcd.VFP * (lcd.HSW + lcd.HBP + lcd.CPL + lcd.HFP) * lcd.PCD;
            lcd.compare = LCD_SYNC;
            break;
    }
    sched_repeat(id, duration);
    intrpt_set(INT_LCD, lcd.ris & lcd.imsc);
}

static uint32_t lcd_dma(enum sched_item_id id) {
    (void)id;
    return 0;
}

void lcd_reset(void) {
    memset(&lcd, 0, sizeof(lcd_state_t));
    sched.items[SCHED_LCD].callback.event = lcd_event;
    sched.items[SCHED_LCD].clock = CLOCK_24M;
    sched_clear(SCHED_LCD);
    sched.items[SCHED_LCD_DMA].callback.dma = lcd_dma;
    sched.items[SCHED_LCD_DMA].clock = CLOCK_48M;
    sched_clear(SCHED_LCD_DMA);
    lcd.width = LCD_WIDTH;
    lcd.height = LCD_HEIGHT;
    lcd.mask = true;
    lcd_setptrs(&lcd);
    gui_console_printf("[CEmu] LCD reset.\n");
}

static uint8_t lcd_read(const uint16_t pio, bool peek) {
    uint16_t index = pio;
    uint8_t bit_offset = (index & 3) << 3;

    (void)peek;

    if (index < 0x200) {
        if (index < 0x010) { return read8(lcd.timing[index >> 2], bit_offset); }
        if (index < 0x014 && index >= 0x010) { return read8(lcd.upbase, bit_offset); }
        if (index < 0x018 && index >= 0x014) { return read8(lcd.lpbase, bit_offset); }
        if (index < 0x01C && index >= 0x018) { return read8(lcd.control, bit_offset); }
        if (index < 0x020 && index >= 0x01C) { return read8(lcd.imsc, bit_offset); }
        if (index < 0x024 && index >= 0x020) { return read8(lcd.ris, bit_offset); }
        if (index < 0x028 && index >= 0x024) { return read8(lcd.imsc & lcd.ris, bit_offset); }
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
    lcd.ofs = NULL;
}

void lcd_enable(void) {
    lcd_setptrs(&lcd);
}

lcd_state_t *lcd_setptrs(lcd_state_t *x) {
    uint8_t mode = x->control >> 1 & 7;
    uint8_t *ofs_start, *ofs_end, *mem_end;
    uint32_t dma_length = 0;
    uint32_t addr = x->upbase;

    x->ofs = NULL;
    x->ofs_end = NULL;
    x->size = x->width * x->height;

    if (!x->size) { return x; }

    /* Mask if true lcd */
    if (x->mask) {
        addr &= 0x7FFFF;
        addr |= 0xD00000;
    }

    if (addr < 0xD00000) {
        mem_end = mem.flash.block + SIZE_FLASH;
        ofs_start = mem.flash.block + addr;
    } else if (addr < 0xE00000){
        mem_end = mem.ram.block + SIZE_RAM;
        ofs_start = mem.ram.block + addr - 0xD00000;
    } else if (addr < 0xE30800) {
        mem_end = (uint8_t *)lcd.palette + sizeof lcd.palette;
        ofs_start = (uint8_t *)lcd.palette + addr - 0xE30200;
    } else if (addr < 0xE30C00){
        mem_end = (uint8_t *)lcd.crsrImage + sizeof lcd.crsrImage;
        ofs_start = (uint8_t *)lcd.crsrImage + addr - 0xE30800;
    } else {
        return x;
    }

    switch (mode) {
        case 0: dma_length = x->size >> 3; break;
        case 1: dma_length = x->size >> 2; break;
        case 2: dma_length = x->size >> 1; break;
        case 3: dma_length = x->size >> 0; break;
        case 4: dma_length = x->size << 1; break;
        case 5: dma_length = x->size << 2; break;
        case 6: dma_length = x->size << 1; break;
        case 7: dma_length = (x->size >> 1) + x->size; break;
    }

    if (ofs_start >= mem_end) { return x; }
    ofs_end = ofs_start + dma_length;
    if (ofs_end > mem_end) { ofs_end = mem_end; }

    x->ofs     = (uint32_t *)ofs_start;
    x->ofs_end = (uint32_t *)ofs_end;

    return x;
}

static void lcd_write(const uint16_t pio, const uint8_t value, bool poke) {
    uint16_t index = pio & 0xFFC;

    uint8_t byte_offset = pio & 3;
    uint8_t bit_offset = byte_offset << 3;

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
            lcd_setptrs(&lcd);
        } else if (index < 0x018 && index >= 0x014) {
            write8(lcd.lpbase, bit_offset, value);
            lcd.lpbase &= ~7U;
        } else if (index == 0x018) {
            if (byte_offset == 0) {
                if (value & 1) { sched_set(SCHED_LCD, 0); }
                else { sched_clear(SCHED_LCD); }
            }
            write8(lcd.control, bit_offset, value);
            /* Simple power down of lcd -- Needs to be correctly emulated in future */
            if (!(lcd.control & 0x800)) { lcd_reset(); }
        } else if (index == 0x01C) {
            write8(lcd.imsc, bit_offset, value);
            lcd.imsc &= 0x1E;
            intrpt_set(INT_LCD, lcd.ris & lcd.imsc);
        } else if (index == 0x028) {
            lcd.ris &= ~(value << bit_offset);
            intrpt_set(INT_LCD, lcd.ris & lcd.imsc);
        }
        lcd_setptrs(&lcd);
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
    lcd_setptrs(&lcd);
    return ret;
}
