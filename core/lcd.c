#include <string.h>
#include <stdio.h>

#include "cpu.h"
#include "emu.h"
#include "mem.h"
#include "lcd.h"
#include "asic.h"
#include "control.h"
#include "schedule.h"
#include "interrupt.h"

/* Global LCD state */
lcd_cntrl_t lcd;

void (*lcd_event_gui_callback)(void) = NULL;

static bool _rgb;

/* This is an intensive function. Need speedz. */
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

#define c1555(w) ((w) + ((w) & 0xFFE0) + ((w) >> 10 & 0x20))
#define c565(w)  (((w) >> 8 & 0xF800) | ((w) >> 5 & 0x7E0) | ((w) >> 3 & 0x1F))
#define c12(w)   (((w) << 4 & 0xF000) | ((w) << 3 & 0x780) | ((w) << 1 & 0x1E))

/* Draw the lcd onto an RGB888 buffer. */
void lcd_drawframe(uint32_t *out, lcd_state_t *buffer) {
    uint_fast8_t mode = buffer->control >> 1 & 7;
    _rgb = buffer->control & (1 << 8);
    bool bebo = buffer->control & (1 << 9);
    uint32_t word, color;
    uint32_t *ofs = buffer->ofs;
    uint32_t *ofs_end = buffer->ofs_end;
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
                color = lcd.cntrl.palette[word >> ((bitpos -= bpp) ^ bi) & mask];
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

static void lcd_event(int index) {
    int pcd = 1;
    int htime, vtime;

    if (!(lcd.cntrl.timing[2] & (1 << 26))) {
        pcd = (lcd.cntrl.timing[2] >> 27 << 5) + (lcd.cntrl.timing[2] & 0x1F) + 2;
    }

    htime =   (lcd.cntrl.timing[0] >> 24 & 0x0FF) + 1  /* Back porch    */
            + (lcd.cntrl.timing[0] >> 16 & 0x0FF) + 1  /* Front porch   */
            + (lcd.cntrl.timing[0] >>  8 & 0x0FF) + 1  /* Sync pulse    */
            + (lcd.cntrl.timing[2] >> 16 & 0x3FF) + 1; /* Active        */
    vtime =   (lcd.cntrl.timing[1] >> 24 & 0x0FF)      /* Back porch    */
            + (lcd.cntrl.timing[1] >> 16 & 0x0FF)      /* Front porch   */
            + (lcd.cntrl.timing[1] >> 10 & 0x03F) + 1  /* Sync pulse    */
            + (lcd.cntrl.timing[1]       & 0x3FF) + 1; /* Active        */
    event_repeat(index, pcd * htime * vtime);

    lcd_drawframe(lcd.cntrl.frame, &lcd.cntrl);

    /* For now, assuming vsync occurs at same time UPBASE is loaded */
    lcd.cntrl.upcurr = lcd.cntrl.upbase;
    lcd.cntrl.ris |= 0xC;
    intrpt_set(INT_LCD, lcd.cntrl.ris & lcd.cntrl.imsc);
}

void lcd_reset(void) {
    memset(&lcd, 0, sizeof(lcd_state_t));
    sched.items[SCHED_LCD].proc = lcd_event;
    sched.items[SCHED_LCD].clock = CLOCK_24M;
    sched.items[SCHED_LCD].second = -1;
    lcd.cntrl.width = LCD_WIDTH;
    lcd.cntrl.height = LCD_HEIGHT;
    lcd.cntrl.mask = true;
    lcd_setptrs(&lcd.cntrl);
    gui_console_printf("[CEmu] LCD reset.\n");
}

static uint8_t lcd_read(const uint16_t pio, bool peek) {
    uint16_t index = pio;
    uint8_t bit_offset = (index & 3) << 3;

    (void)peek;

    if (index < 0x200) {
        if (index < 0x010) { return read8(lcd.cntrl.timing[index >> 2], bit_offset); }
        if (index < 0x014 && index >= 0x010) { return read8(lcd.cntrl.upbase, bit_offset); }
        if (index < 0x018 && index >= 0x014) { return read8(lcd.cntrl.lpbase, bit_offset); }
        if (index < 0x01C && index >= 0x018) { return read8(lcd.cntrl.control, bit_offset); }
        if (index < 0x020 && index >= 0x01C) { return read8(lcd.cntrl.imsc, bit_offset); }
        if (index < 0x024 && index >= 0x020) { return read8(lcd.cntrl.ris, bit_offset); }
        if (index < 0x028 && index >= 0x024) { return read8(lcd.cntrl.imsc & lcd.cntrl.ris, bit_offset); }
    } else if (index < 0x400) {
        return *((uint8_t *)lcd.cntrl.palette + index - 0x200);
    } else if (index < 0xC30) {
        if (index < 0xC00 && index >= 0x800) { return read8(lcd.cntrl.crsrImage[((pio-0x800) & 0x3FF) >> 2], bit_offset); }
        if (index == 0xC00) { return read8(lcd.cntrl.crsrControl, bit_offset); }
        if (index == 0xC04) { return read8(lcd.cntrl.crsrConfig, bit_offset); }
        if (index < 0xC0C && index >= 0xC08) { return read8(lcd.cntrl.crsrPalette0, bit_offset); }
        if (index < 0xC10 && index >= 0xC0C) { return read8(lcd.cntrl.crsrPalette1, bit_offset); }
        if (index < 0xC14 && index >= 0xC10) { return read8(lcd.cntrl.crsrXY, bit_offset); }
        if (index < 0xC16 && index >= 0xC14) { return read8(lcd.cntrl.crsrClip, bit_offset); }
        if (index == 0xC20) { return read8(lcd.cntrl.crsrImsc, bit_offset); }
        if (index == 0xC28) { return read8(lcd.cntrl.crsrRis, bit_offset); }
        if (index == 0xC2C) { return read8(lcd.cntrl.crsrRis & lcd.cntrl.crsrImsc, bit_offset); }
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
    lcd.cntrl.ofs = NULL;
}

void lcd_enable(void) {
    lcd_setptrs(&lcd.cntrl);
}

void lcd_setptrs(lcd_state_t *x) {
    uint8_t mode = x->control >> 1 & 7;
    uint8_t *ofs_start, *ofs_end, *mem_end;
    uint32_t dma_length;
    uint32_t addr = x->upbase;

    x->ofs = NULL;
    x->ofs_end = NULL;
    x->size = x->width * x->height;

    if (!x->size) { return; }

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
        mem_end = (uint8_t *)lcd.cntrl.palette + sizeof lcd.cntrl.palette;
        ofs_start = (uint8_t *)lcd.cntrl.palette + addr - 0xE30200;
    } else if (addr < 0xE30C00){
        mem_end = (uint8_t *)lcd.cntrl.crsrImage + sizeof lcd.cntrl.crsrImage;
        ofs_start = (uint8_t *)lcd.cntrl.crsrImage + addr - 0xE30800;
    } else {
        return;
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

    if (ofs_start >= mem_end) { return; }
    ofs_end = ofs_start + dma_length;
    if (ofs_end > mem_end) { ofs_end = mem_end; }

    x->ofs     = (uint32_t *)ofs_start;
    x->ofs_end = (uint32_t *)ofs_end;
}

static void lcd_write(const uint16_t pio, const uint8_t value, bool poke) {
    uint16_t index = pio & 0xFFC;

    uint8_t byte_offset = pio & 3;
    uint8_t bit_offset = byte_offset << 3;

    (void)poke;

    if (index < 0x200) {
        if (index < 0x010) {
            write8(lcd.cntrl.timing[index >> 2], bit_offset, value);
            lcd.cntrl.PPL =  ((lcd.cntrl.timing[0] >>  2 &  0x3F) + 1) << 4;
            lcd.cntrl.HSW =   (lcd.cntrl.timing[0] >>  8 &  0xFF) + 1;
            lcd.cntrl.HFP =   (lcd.cntrl.timing[0] >> 16 &  0xFF) + 1;
            lcd.cntrl.HBP =   (lcd.cntrl.timing[0] >> 24 &  0xFF) + 1;
            lcd.cntrl.LPP =   (lcd.cntrl.timing[1] >>  0 & 0x3FF) + 1;
            lcd.cntrl.VSW =   (lcd.cntrl.timing[1] >> 10 &  0x3F) + 1;
            lcd.cntrl.VFP =    lcd.cntrl.timing[1] >> 16;
            lcd.cntrl.VBP =    lcd.cntrl.timing[1] >> 24;
            lcd.cntrl.PCD =  ((lcd.cntrl.timing[2] >>  0 &  0x1F) |
                              (lcd.cntrl.timing[2] >> 27 &  0x1F) << 5) + 2;
            lcd.cntrl.CLKSEL = lcd.cntrl.timing[2] >>  5 &     1;
            lcd.cntrl.ACB =   (lcd.cntrl.timing[2] >>  6 &  0x1F) + 1;
            lcd.cntrl.IVS =    lcd.cntrl.timing[2] >> 11 &     1;
            lcd.cntrl.IHS =    lcd.cntrl.timing[2] >> 12 &     1;
            lcd.cntrl.IPC =    lcd.cntrl.timing[2] >> 13 &     1;
            lcd.cntrl.IOE =    lcd.cntrl.timing[2] >> 14 &     1;
            lcd.cntrl.CPL =   (lcd.cntrl.timing[2] >> 16 & 0x3FF) + 1;
            lcd.cntrl.PCD =    lcd.cntrl.timing[2] >> 26 &     1;
            lcd.cntrl.LED =   (lcd.cntrl.timing[3] >>  0 &  0x7F) + 1;
            lcd.cntrl.LEE =   (lcd.cntrl.timing[3] >> 16 &     1;
        } else if (index < 0x014 && index >= 0x010) {
            write8(lcd.cntrl.upbase, bit_offset, value);
            if (lcd.cntrl.upbase & 7) {
                gui_console_printf("[CEmu] Warning: Aligning LCD panel\n");
            }
            lcd.cntrl.upbase &= ~7U;
            lcd_setptrs(&lcd.cntrl);
        } else if (index < 0x018 && index >= 0x014) {
            write8(lcd.cntrl.lpbase, bit_offset, value);
            lcd.cntrl.lpbase &= ~7U;
        } else if (index == 0x018) {
            if (byte_offset == 0) {
                if (value & 1) { event_set(SCHED_LCD, 0); }
                else { event_clear(SCHED_LCD); }
            }
            write8(lcd.cntrl.control, bit_offset, value);
            /* Simple power down of lcd -- Needs to be correctly emulated in future */
            if (!(lcd.cntrl.control & 0x800)) { lcd_reset(); }
        } else if (index == 0x01C) {
            write8(lcd.cntrl.imsc, bit_offset, value);
            lcd.cntrl.imsc &= 0x1E;
            intrpt_set(INT_LCD, lcd.cntrl.ris & lcd.cntrl.imsc);
        } else if (index == 0x028) {
            lcd.cntrl.ris &= ~(value << bit_offset);
            intrpt_set(INT_LCD, lcd.cntrl.ris & lcd.cntrl.imsc);
        }
        lcd_setptrs(&lcd.cntrl);
    } else if (index < 0x400) {
        write8(lcd.cntrl.palette[pio >> 1 & 0xFF], (pio & 1) << 3, value);
    } else if (index < 0xC30) {
        if (index < 0xC00 && index >= 0x800) {
            write8(lcd.cntrl.crsrImage[((pio-0x800) & 0x3FF) >> 2], bit_offset, value);
        }
        if (index == 0xC00) {
            write8(lcd.cntrl.crsrControl, bit_offset, value);
        }
        if (index == 0xC04) {
            write8(lcd.cntrl.crsrConfig, bit_offset, value);
            lcd.cntrl.crsrConfig &= 0xF;
        }
        if (index < 0xC0B && index >= 0xC08) {
            write8(lcd.cntrl.crsrPalette0, bit_offset, value);
        }
        if (index < 0xC0F && index >= 0xC0C) {
            write8(lcd.cntrl.crsrPalette1, bit_offset, value);
        }
        if (index < 0xC14 && index >= 0xC10) {
            write8(lcd.cntrl.crsrXY, bit_offset, value);
            lcd.cntrl.crsrXY &= (0xFFF | (0xFFF << 16));
        }
        if (index < 0xC16 && index >= 0xC14) {
            write8(lcd.cntrl.crsrClip, bit_offset, value);
            lcd.cntrl.crsrClip &= (0x3F | (0x3F << 8));
        }
        if (index == 0xC20) {
            write8(lcd.cntrl.crsrImsc, bit_offset, value);
            lcd.cntrl.crsrImsc &= 0xF;
        }
        if (index == 0xC24) {
            lcd.cntrl.crsrRis &= ~(value << bit_offset);
            lcd.cntrl.crsrRis &= 0xF;
        }
    }
}

static const eZ80portrange_t device = {
    .read  = lcd_read,
    .write = lcd_write
};

eZ80portrange_t init_lcd(void) {
    lcd.spi = &spi;
    gui_console_printf("[CEmu] Initialized LCD...\n");
    return device;
}

bool lcd_save(emu_image *s) {
    s->lcd = lcd;
    s->lcd.cntrl.ofs_end = s->lcd.cntrl.ofs = NULL;
    s->lcd.spi = NULL;
    return true;
}

bool lcd_restore(const emu_image *s) {
    lcd = s->lcd;
    lcd.spi = &spi;
    lcd_setptrs(&lcd.cntrl);
    return true;
}
