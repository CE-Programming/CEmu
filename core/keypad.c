#include "core/keypad.h"
#include "core/emu.h"
#include "core/schedule.h"
#include "core/interrupt.h"
#include <stdio.h>
#include <stdlib.h>

/* Global KEYPAD state */
keypad_state_t keypad;

void keypad_intrpt_check() {
    uint8_t status = (keypad.status & keypad.enable) | (keypad.gpio_status & keypad.gpio_enable);

    intrpt_set(INT_KEYPAD, status);
}

void keypad_on_pressed(void) {
    gui_console_printf("[ON] key pressed.\n");
    intrpt_set(INT_ON, true);
}

static uint8_t keypad_read(const uint16_t pio)
{
    uint16_t index = (pio >> 2) & 0x7F;
    uint8_t bit_offset = (pio & 3) << 3;

    switch(upperNibble8(pio & 0x7F)) {
        case 0x1:
        case 0x2:        /* keypad data range */
            return ((uint8_t *)keypad.data)[(pio-0x10) & 0xF];
        default:
            switch(index) {
                case 0x00:
                    return read8(keypad.cntrl,bit_offset);
                case 0x01:
                    return read8(keypad.size,bit_offset);
                case 0x02:
                    return read8(keypad.status,bit_offset);
                case 0x03:
                    return read8(keypad.enable,bit_offset);
                case 0x10:
                    return read8(keypad.gpio_status,bit_offset);
                case 0x11:
                    return read8(keypad.gpio_enable,bit_offset);
                default:
                    break;
            }
    }

    /* return 0 if unimplemented or not in range */
    return 0;
}

/* Scan next row of keypad, if scanning is enabled */
static void keypad_scan_event(int index) {
    uint16_t row;

    if (keypad.current_row >= sizeof(keypad.data) / sizeof(keypad.data[0])) {
        return; /* too many keypad rows */
    }

    row = ~keypad.key_map[keypad.current_row];
    row &= ~(0x80000 >> keypad.current_row);      /* Emulate weird diagonal glitch */
    row |= 0xFFFF << (keypad.size >> 8 & 0xFF);   /* Unused columns read as 1 */
    row = ~row;

    if (keypad.data[keypad.current_row] != row) {
        keypad.data[keypad.current_row] = row;
        keypad.status |= 6;
    }

    keypad.current_row++;
    if (keypad.current_row < (keypad.size & 0xFF)) {
        event_repeat(index, keypad.cntrl >> 2 & 0x3FFF);
    } else {
        keypad.current_row = 0;
        keypad.enable |= 1;
        if (keypad.cntrl & 1) {
            event_repeat(index, (keypad.cntrl >> 16) + (keypad.cntrl >> 2 & 0x3FFF));
        } else {
            /* If in single scan mode, go to idle mode */
            keypad.cntrl &= ~3;
        }
    }
    keypad_intrpt_check();
}

static void keypad_write(const uint16_t pio, const uint8_t byte)
{
    uint16_t index = (pio >> 2) & 0x7F;
    uint8_t bit_offset = (pio & 3) << 3;

    switch (index) {
        case 0x00:
            write8(keypad.cntrl,bit_offset,byte);
            if ((keypad.cntrl & 3)) {
                event_set(SCHED_KEYPAD, ((keypad.cntrl >> 16) + (keypad.cntrl >> 2 & 0x3FFF)));
            } else {
                event_clear(SCHED_KEYPAD);
            }
            return;
        case 0x01:
            write8(keypad.size,bit_offset,byte);
            return;
        case 0x02:
            write8(keypad.status,bit_offset,keypad.status & ~(byte & 7));
            keypad_intrpt_check();
            return;
        case 0x03:
            write8(keypad.enable,bit_offset,byte & 7);
            return;
        case 0x10:
            write8(keypad.gpio_status,bit_offset,byte);
            keypad_intrpt_check();
            return;
        case 0x11:
            write8(keypad.gpio_enable,bit_offset,keypad.gpio_enable & ~byte);
            keypad_intrpt_check();
            return;
        default:
            return;  /* Escape write sequence if unimplemented */
    }
}

void keypad_reset() {
    unsigned int i;

    keypad.current_row = 0;
    for(i=0; i<sizeof(keypad.data) / sizeof(keypad.data[0]); i++) {
        keypad.data[i] = 0;
    }

    sched.items[SCHED_KEYPAD].clock = CLOCK_APB;
    sched.items[SCHED_KEYPAD].second = -1;
    sched.items[SCHED_KEYPAD].proc = keypad_scan_event;

    gui_console_printf("Keypad reset.\n");
}

static const eZ80portrange_t device = {
    .read_in    = keypad_read,
    .write_out  = keypad_write
};

eZ80portrange_t init_keypad(void) {
    gui_console_printf("Initialized keypad...\n");
    return device;
}
