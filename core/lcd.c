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
#include "mem.h"
#include "emu.h"
#include "capture/gif.h"

/* Global LCD state */
lcd_cntrl_state_t lcd;

/* Draw the current screen into a 16bpp upside-down bitmap. */
void lcd_drawframe(uint16_t *buffer, uint32_t *bitfields) {
    uint32_t mode = lcd.control >> 1 & 7;
    uint32_t bpp;
    uint32_t *in;
    int row;

    if (mode <= 5) {
        bpp = 1 << mode;
    }
    else {
        bpp = 16;
    }

    if (mode == 7) {
        // 444 format
        bitfields[0] = 0x000F;
        bitfields[1] = 0x00F0;
        bitfields[2] = 0x0F00;
    } else {
        // 565 format
        bitfields[0] = 0x001F;
        bitfields[1] = 0x07E0;
        bitfields[2] = 0xF800;
    }
    if (lcd.control & (1 << 8)) {
        // BGR format (R high, B low)
        uint32_t tmp = bitfields[0];
        bitfields[0] = bitfields[2];
        bitfields[2] = tmp;
    }

    in = (uint32_t *)(intptr_t)phys_mem_ptr(lcd.upcurr, (320 * 240) / 8 * bpp);
    if (!in || !lcd.upcurr) {
        memset(buffer, 0, 320 * 240 * 2);
        return;
    }
    for (row = 0; row < 240; ++row) {
        uint16_t *out = buffer + (row * 320);
        uint32_t words = (320 / 32) * bpp;
        if (bpp < 16) {
            uint32_t mask = (1 << bpp) - 1;
            uint32_t bi = (lcd.control & (1 << 9)) ? 0 : 24;
            if (!(lcd.control & (1 << 10))) {
                bi ^= (8 - bpp);
            }
            do {
                uint32_t word = *in++;
                int bitpos = 32;
                do {
                    uint16_t color = lcd.palette[word >> ((bitpos -= bpp) ^ bi) & mask];
                    *out++ = color + (color & 0xFFE0) + (color >> 10 & 0x20);
                } while (bitpos != 0);
            } while (--words != 0);
        } else if (mode == 4) {
            uint32_t i, bi = lcd.control >> 9 & 1;
            for (i = 0; i < 320; i++) {
                uint16_t color = ((uint16_t *)in)[i ^ bi];
                uint8_t r = color & 0x1F,
                        g = (color >> 5) & 0x1F,
                        b = (color >> 10) & 0x1F;

                out[i] = (r << 11) | (g << 6) | b | (color >> 10 & 0x20);
            }
            in += 160;
        } else if (mode == 5) {
            // 32bpp mode: Convert 888 to 565
            do {
                uint32_t word = *in++;
                *out++ = (word >> 8 & 0xF800) | (word >> 5 & 0x7E0) | (word >> 3 & 0x1F);
            } while (--words != 0);
        } else {
            if (!(lcd.control & (1 << 9))) {
                memcpy(out, in, 640);
                in += 160;
            } else {
                uint32_t *outw = (uint32_t *)out;
                do {
                    uint32_t word = *in++;
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
    htime =   (lcd.timing[0] >> 24 & 0x0FF) + 1  // Back porch
            + (lcd.timing[0] >> 16 & 0x0FF) + 1  // Front porch
            + (lcd.timing[0] >>  8 & 0x0FF) + 1  // Sync pulse
            + (lcd.timing[2] >> 16 & 0x3FF) + 1; // Active
    vtime =   (lcd.timing[1] >> 24 & 0x0FF)      // Back porch
            + (lcd.timing[1] >> 16 & 0x0FF)      // Front porch
            + (lcd.timing[1] >> 10 & 0x03F) + 1  // Sync pulse
            + (lcd.timing[1]       & 0x3FF) + 1; // Active
    event_repeat(index, pcd * htime * vtime);
    /* for now, assuming vcomp occurs at same time UPBASE is loaded */
    lcd.upcurr = lcd.upbase;
    lcd.ris |= 0xC;
    intrpt_trigger(INT_LCD, lcd.ris & lcd.mis ? INTERRUPT_SET : INTERRUPT_CLEAR);

    gif_new_frame();
}

void lcd_reset(void) {
    /* Palette is unchanged on a reset */
    memset(&lcd, 0, (char *)&lcd.palette - (char *)&lcd);
    sched.items[SCHED_LCD].clock = CLOCK_12M;
    sched.items[SCHED_LCD].second = -1;
    sched.items[SCHED_LCD].proc = lcd_event;
    gui_console_printf("LCD reset.\n");
}

uint8_t lcd_read(const uint16_t pio) {
    uint16_t offset = pio & 0xFFF;
    uint8_t bit_offset = (offset & 0x3) << 0x3;

    if (offset < 0x200) {
        if(offset < 0x010) { return read8(lcd.timing[offset >> 2], bit_offset); }
        if(offset < 0x014 && offset >= 0x010) { return read8(lcd.upbase, bit_offset); }
        if(offset < 0x018 && offset >= 0x014) { return read8(lcd.lpbase, bit_offset); }
        if(offset < 0x01C && offset >= 0x018) { return read8(lcd.control, bit_offset); }
        if(offset < 0x020 && offset >= 0x01C) { return read8(lcd.imsc, bit_offset); }
        if(offset < 0x024 && offset >= 0x020) { return read8(lcd.mis, bit_offset); }
        if(offset < 0x028 && offset >= 0x024) { return read8(lcd.mis & lcd.ris, bit_offset); }
    } else if (offset < 0x400) {
        return *(uint32_t *)((uint8_t *)lcd.palette + offset - 0x200);
    } else if (offset >= 0xFE0) {
        static const uint8_t id[1][8] = {
            { 0x11, 0x11, 0x14, 0x00, 0x0D, 0xF0, 0x05, 0xB1 }
        };
        return read8(id[0][(offset - 0xFE0) >> 2], bit_offset);
    }

    /* Return 0 if bad read */
    return 0;
}

void lcd_write(const uint16_t pio, const uint8_t value) {
    uint32_t offset = pio & 0xFFC;
    if (offset < 0x200) {
        uint8_t byte_offset = pio & 0x3;
        uint8_t bit_offset = byte_offset << 0x3;
        if (offset < 0x010) {
            write8(lcd.timing[offset >> 2], bit_offset, value);
        } else if (offset == 0x010) {
            write8(lcd.upbase, bit_offset, value);
            if (lcd.upbase & 0x7) {
                gui_console_printf("Warning: LCD upper panel base not 8-byte aligned!\n");
            }
            lcd.upbase &= ~0x7;
        } else if (offset == 0x014) {
            write8(lcd.lpbase, bit_offset, value);
            if (lcd.lpbase & 0x7) {
                gui_console_printf("Warning: LCD lower panel base not 8-byte aligned!\n");
            }
            lcd.lpbase &= ~0x7;
        } else if (offset == 0x018) {
            if (((value << bit_offset) ^ lcd.control) & 1) {
                if (value & 1) { event_set(SCHED_LCD, 0); }
                else { event_clear(SCHED_LCD); }
            }
            write8(lcd.control, bit_offset, value);
        } else if (offset == 0x01C) {
            write8(lcd.imsc, bit_offset, value);
            lcd.imsc &= 0x1E;
            intrpt_trigger(INT_LCD, lcd.ris & lcd.imsc ? INTERRUPT_SET : INTERRUPT_CLEAR);
        } else if (offset == 0x028) {
            lcd.ris &= ~(value << bit_offset);
            intrpt_trigger(INT_LCD, lcd.ris & lcd.imsc ? INTERRUPT_SET : INTERRUPT_CLEAR);
        }
    } else if (offset < 0x400) {
        write8(lcd.palette[pio >> 1 & 0xFF], (pio & 1) << 3, value);
    }
}

static const eZ80portrange_t device = {
    .read_in    = lcd_read,
    .write_out  = lcd_write
};

eZ80portrange_t init_lcd(void) {
    return device;
}
