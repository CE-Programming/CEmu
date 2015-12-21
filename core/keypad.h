/* Declarations for keypad.c */

#ifndef KEYPAD_H
#define KEYPAD_H

#include <core/cpu.h>

#ifdef __cplusplus
extern "C" {
#endif

// Standard KEYPAD state
struct keypad_state {
    uint32_t cntrl;
    uint32_t size;
    uint8_t  current_row;
    uint8_t  interrupt_ack;
    uint8_t  interrupt_mask;
    uint16_t data[16];
    uint16_t key_map[16];
    uint32_t gpio_interrupt_ack;
    uint32_t gpio_interrupt_mask;
    uint8_t data_write_enabled;
};

/* Type definitions */
typedef struct keypad_state keypad_state_t;

/* Global KEYPAD state */
extern keypad_state_t keypad;

/* Available Functions */
eZ80portrange_t init_keypad(void);
void keypad_intrpt_check(void);
void keypad_reset(void);
void keypad_on_pressed(void);

#ifdef __cplusplus
}
#endif

#endif
