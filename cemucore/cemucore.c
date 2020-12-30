/*
 * Copyright (c) 2015-2020 CE Programming.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "cemucore.h"

keypad_t keypad;
debug_t debug;
lcd_t lcd;

ti_device_t get_device_type(void) {
    return TI84PCE;
}
uint8_t mem_peek_byte(uint32_t addr) {
    (void)addr;
    return 0;
}
void emu_set_lcd_ptrs(uint32_t **dat, uint32_t **dat_end, int width, int height, uint32_t addr, uint32_t control, bool mask) {
    (void)dat;
    (void)dat_end;
    (void)width;
    (void)height;
    (void)addr;
    (void)control;
    (void)mask;
}
void emu_lcd_drawmem(void *output, void *data, void *data_end, uint32_t control, int size, int spi) {
    (void)output;
    (void)data;
    (void)data_end;
    (void)control;
    (void)size;
    (void)spi;
}
void emu_keypad_event(unsigned int row, unsigned int col, bool press) {
    (void)row;
    (void)col;
    (void)press;
}
void keypad_intrpt_check(void) {}
FILE *fopen_utf8(const char *filename, const char *mode) {
    return fopen(filename, mode);
}
