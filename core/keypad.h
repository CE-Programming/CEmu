#ifndef KEYPAD_H
#define KEYPAD_H

#include "defines.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "port.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#define KEYPAD_MAX_COLS 16
#define KEYPAD_MAX_ROWS 16
#define KEYPAD_ACTUAL_COLS 8
#define KEYPAD_ACTUAL_ROWS 8

typedef struct keypad_state {
    union {
        struct {
#if CEMU_BITFIELD_ORDER == CEMU_LITTLE_ENDIAN
            uint32_t mode : 2, rowWait : 14, scanWait : 16;
#else
            uint32_t scanWait : 16, rowWait : 14, mode : 2;
#endif
        };
        uint32_t control;
    };
    union {
        struct {
#if CEMU_BITFIELD_ORDER == CEMU_LITTLE_ENDIAN
            uint32_t rows : 8, cols : 8, mask : 16;
#else
            uint32_t mask : 16, cols : 8, rows : 8;
#endif
        };
        uint32_t size;
    };
    uint8_t  row;
    uint8_t  status;
    uint8_t  enable;
    uint16_t data[KEYPAD_MAX_ROWS];
    uint32_t gpioEnable;
} keypad_state_t;

extern keypad_state_t keypad;

eZ80portrange_t init_keypad(void);
void keypad_any_check(void);
void keypad_on_check(void);
void keypad_reset(void);
bool keypad_restore(FILE *image);
bool keypad_save(FILE *image);

/* api functions */
void emu_keypad_event(unsigned int row, unsigned int col, bool press);
void emu_set_keypad_ghosting(int enable);

#ifdef __cplusplus
}
#endif

#endif
