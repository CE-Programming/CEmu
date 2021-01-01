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

#ifndef CEMUCORE_H
#define CEMUCORE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef void (*cemucore_signal_t)(void *);

typedef enum cemucore_create_flags
{
#ifndef CEMUCORE_NOTHREADS
    CEMUCORE_CREATE_THREADED = 1,
#endif
} cemucore_create_flags_t;

typedef struct cemucore cemucore_t;

cemucore_t *cemucore_create(cemucore_create_flags_t, cemucore_signal_t, void *);
void cemucore_destroy(cemucore_t *);

/* !!! DEPRECATED API !!! */
typedef enum {
    TI84PCE = 0,
    TI83PCE = 1
} ti_device_t;
ti_device_t get_device_type(void);
uint8_t mem_peek_byte(uint32_t addr);
void emu_set_lcd_ptrs(uint32_t **dat, uint32_t **dat_end, int width, int height, uint32_t addr, uint32_t control, bool mask);
#define DBG_MASK_READ  1
#define DBG_MASK_WRITE 2
#define DBG_MASK_EXEC  4
typedef struct {
    uint32_t gpioEnable;
} keypad_t;
extern keypad_t keypad;
typedef struct {
    uint8_t addr[0x10000];
} debug_t;
extern debug_t debug;
typedef struct {
    uint32_t control, upbase;
} lcd_t;
extern lcd_t lcd;
void emu_lcd_drawmem(void *output, void *data, void *data_end, uint32_t control, int size, int spi);
#define LCD_WIDTH 320
#define LCD_HEIGHT 240
void emu_keypad_event(unsigned int row, unsigned int col, bool press);
void keypad_intrpt_check(void);
#include <stdio.h>
FILE *fopen_utf8(const char *filename, const char *mode);

#ifdef __cplusplus
}
#endif

#endif
