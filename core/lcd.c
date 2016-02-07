/* Copyright (C) 2015  Fabian Vogt
 * Modified for the CE calculator by CEmu developers
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
*/

#include <string.h>

#include "lcd.h"
#include "schedule.h"
#include "interrupt.h"
#include "cpu.h"
#include "emu.h"

/* Global LCD state */
lcd_cntrl_state_t lcd;

void (*lcd_event_gui_callback)(void) = NULL;

#define dataswap(a, b) do { (a) ^= (b); (b) ^= (a); (a) ^= (b); } while(0)

/* Draw the current screen into a 16bpp upside-down bitmap. */
void lcd_drawframe(uint16_t *buffer, uint32_t *bitfields) {
    uint32_t mode = lcd.control >> 1 & 7;
    uint32_t bpp;
    uint32_t words,word;
    uint32_t i,bi,color,mask;
    uint32_t *outw;
    uint32_t *in;
    uint16_t *out;
    uint8_t r,
            g,
            b;

    int row;

    if (mode <= 5) {
        bpp = 1 << mode;
    }
    else {
        bpp = 16;
    }

    if (mode == 7) {
        /* 444 format */
        bitfields[0] = 0x000F;
        bitfields[1] = 0x00F0;
        bitfields[2] = 0x0F00;
    } else {
        /* 565 format */
        bitfields[0] = 0x001F;
        bitfields[1] = 0x07E0;
        bitfields[2] = 0xF800;
    }
    if (lcd.control & (1 << 8)) {
        /* BGR format (R high, B low) */
        dataswap(bitfields[0], bitfields[2]);
    }

    in = (uint32_t *)(intptr_t)phys_mem_ptr(lcd.upcurr, (320 * 240) / 8 * bpp);
    if (!in || !lcd.upcurr) {
        memset(buffer, 0, 320 * 240 * 2);
        return;
    }

    if (bpp < 16) {
        for (row = 0; row < 240; ++row) {
            out = buffer + (row * 320);
            words = (320 / 32) * bpp;
            mask = (1 << bpp) - 1;
            bi = (lcd.control & (1 << 9)) ? 0 : 24;
            if (!(lcd.control & (1 << 10))) {
                bi ^= (8 - bpp);
            }
            do {
                int bitpos = 32;
                word = *in++;
                do {
                    color = lcd.palette[word >> ((bitpos -= bpp) ^ bi) & mask];
                    *out++ = color + (color & 0xFFE0) + (color >> 10 & 0x20);
                } while (bitpos != 0);
            } while (--words != 0);
        }
    } else if (mode == 4) {
        for (row = 0; row < 240; ++row) {
            out = buffer + (row * 320);
            words = (320 / 32) * bpp;
            bi = lcd.control >> 9 & 1;
            for (i = 0; i < 320; i++) {
                color = ((uint16_t *)in)[i ^ bi];
                r = color & 0x1F;
                g = (color >> 5) & 0x1F;
                b = (color >> 10) & 0x1F;

                out[i] = (r << 11) | (g << 6) | b | (color >> 10 & 0x20);
            }
            in += 160;
       }
    } else if (mode == 5) {
        for (row = 0; row < 240; ++row) {
            out = buffer + (row * 320);
            words = (320 / 32) * bpp;
            /* 32bpp mode: Convert 888 to 565 */
            do {
                word = *in++;
                *out++ = (word >> 8 & 0xF800) | (word >> 5 & 0x7E0) | (word >> 3 & 0x1F);
            } while (--words != 0);
        }
    } else {
        for (row = 0; row < 240; ++row) {
            out = buffer + (row * 320);
            words = (320 / 32) * bpp;
            if (!(lcd.control & (1 << 9))) {
                memcpy(out, in, 640);
                in += 160;
            } else {
                outw = (uint32_t *)out;
                do {
                    word = *in++;
                    *outw++ = word << 16 | word >> 16;
                } while (--words != 0);
            }
        }
    }
}

static void lcd_event(int index) {
    int pcd = 1;
    int htime, vtime;

    if (!(lcd.timing[2] & (1 << 26))) {
        pcd = (lcd.timing[2] >> 27 << 5) + (lcd.timing[2] & 0x1F) + 2;
    }

    htime =   (lcd.timing[0] >> 24 & 0x0FF) + 1  /* Back porch    */
            + (lcd.timing[0] >> 16 & 0x0FF) + 1  /* Front porch   */
            + (lcd.timing[0] >>  8 & 0x0FF) + 1  /* Sync pulse    */
            + (lcd.timing[2] >> 16 & 0x3FF) + 1; /* Active        */
    vtime =   (lcd.timing[1] >> 24 & 0x0FF)      /* Back porch    */
            + (lcd.timing[1] >> 16 & 0x0FF)      /* Front porch   */
            + (lcd.timing[1] >> 10 & 0x03F) + 1  /* Sync pulse    */
            + (lcd.timing[1]       & 0x3FF) + 1; /* Active        */
    event_repeat(index, pcd * htime * vtime);

    /* For now, assuming vcomp occurs at same time UPBASE is loaded */
    lcd.upcurr = lcd.upbase;
    lcd.ris |= 0xC;
    intrpt_set(INT_LCD, lcd.ris & lcd.mis);

    if (lcd_event_gui_callback) {
        lcd_event_gui_callback();
    }
}

void lcd_reset(void) {
    /* Palette is unchanged on a reset */
    memset(&lcd, 0, (char *)&lcd.palette - (char *)&lcd);
    sched.items[SCHED_LCD].clock = CLOCK_12M;
    sched.items[SCHED_LCD].second = -1;
    sched.items[SCHED_LCD].proc = lcd_event;
    gui_console_printf("[CEmu] LCD reset.\n");
}

uint8_t lcd_read(const uint16_t pio) {
    uint16_t index = pio & 0xFFF;
    uint8_t bit_offset = (index & 3) << 3;

    if (index < 0x200) {
        if(index < 0x010) { return read8(lcd.timing[index >> 2], bit_offset); }
        if(index < 0x014 && index >= 0x010) { return read8(lcd.upbase, bit_offset); }
        if(index < 0x018 && index >= 0x014) { return read8(lcd.lpbase, bit_offset); }
        if(index < 0x01C && index >= 0x018) { return read8(lcd.control, bit_offset); }
        if(index < 0x020 && index >= 0x01C) { return read8(lcd.imsc, bit_offset); }
        if(index < 0x024 && index >= 0x020) { return read8(lcd.ris, bit_offset); }
        if(index < 0x028 && index >= 0x024) { return read8(lcd.mis & lcd.ris, bit_offset); }
    } else if (index < 0x400) {
        return *((uint8_t *)lcd.palette + index - 0x200);
    } else if (index >= 0xFE0) {
        static const uint8_t id[1][8] = {
            { 0x11, 0x11, 0x14, 0x00, 0x0D, 0xF0, 0x05, 0xB1 }
        };
        return read8(id[0][(index - 0xFE0) >> 2], bit_offset);
    }

    /* Return 0 if bad read */
    return 0;
}

void lcd_write(const uint16_t pio, const uint8_t value) {
    uint32_t index = pio & 0xFFC;

    if (index < 0x200) {
        uint8_t byte_offset = pio & 3;
        uint8_t bit_offset = byte_offset << 3;
        if (index < 0x010) {
            write8(lcd.timing[index >> 2], bit_offset, value);
        } else if (index == 0x010) {
            write8(lcd.upbase, bit_offset, value);
            if (lcd.upbase & 7) {
                gui_console_printf("Warning: LCD upper panel base not 8-byte aligned!\n");
            }
            lcd.upbase &= ~7U;
        } else if (index == 0x014) {
            write8(lcd.lpbase, bit_offset, value);
            if (lcd.lpbase & 7) {
                gui_console_printf("Warning: LCD lower panel base not 8-byte aligned!\n");
            }
            lcd.lpbase &= ~7U;
        } else if (index == 0x018) {
            if(byte_offset == 0) {
                if (value & 1) { event_set(SCHED_LCD, 0); }
                else { event_clear(SCHED_LCD); }
            }
            write8(lcd.control, bit_offset, value);
        } else if (index == 0x01C) {
            write8(lcd.imsc, bit_offset, value);
            lcd.imsc &= 0x1E;
            intrpt_set(INT_LCD, lcd.ris & lcd.imsc);
        } else if (index == 0x028) {
            lcd.ris &= ~(value << bit_offset);
            intrpt_set(INT_LCD, lcd.ris & lcd.imsc);
        }
    } else if (index < 0x400) {
        write8(lcd.palette[pio >> 1 & 0xFF], (pio & 1) << 3, value);
    }
}

static const eZ80portrange_t device = {
    .read_in    = lcd_read,
    .write_out  = lcd_write
};

eZ80portrange_t init_lcd(void) {
    gui_console_printf("[CEmu] Initialized LCD...\n");
    return device;
}
