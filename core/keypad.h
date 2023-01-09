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

typedef struct keypad_state {
    union {
        struct {
            uint16_t mode : 2, rowWait : 14, scanWait;
        };
        uint32_t control;
    };
    union {
        struct {
            uint8_t rows, cols;
        };
        uint32_t size;
    };
    uint8_t  row;
    uint8_t  status;
    uint8_t  enable;
    uint16_t data[16];
    uint16_t keyMap[16];
    uint16_t delay[16];
    uint32_t gpioStatus;
    uint32_t gpioEnable;
} keypad_state_t;

extern keypad_state_t keypad;

eZ80portrange_t init_keypad(void);
void keypad_intrpt_check(void);
void keypad_reset(void);
bool keypad_restore(FILE *image);
bool keypad_save(FILE *image);

/* api functions */
void emu_keypad_event(unsigned int row, unsigned int col, bool press);

#ifdef __cplusplus
}
#endif

#endif
