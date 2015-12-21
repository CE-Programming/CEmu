#include "core/keypad.h"
#include "core/emu.h"
#include "core/schedule.h"
#include "core/interrupt.h"
#include <stdio.h>
#include <stdlib.h>

/* Global KEYPAD state */
keypad_state_t keypad;

void keypad_intrpt_check() {
    intrpt_set(INT_KEYPAD, (keypad.interrupt_ack & keypad.interrupt_mask) | (keypad.gpio_interrupt_ack & keypad.gpio_interrupt_mask));
}

void keypad_on_pressed() {
    intrpt_set(INT_PWR, true);
}

static uint8_t keypad_read(const uint16_t pio)
{
    uint16_t index = pio & 0x7F;
    uint8_t bit_offset = (index&3)<<3;

    switch(upperNibble8(index)) {
        case 0x1:
        case 0x2:        /* keypad data range */
            return ((uint8_t *)keypad.data)[index & 0xF];
        default:
            switch(index) {
                case 0x00: case 0x01: case 0x02: case 0x03:     /* keypad contoller range */
                    return read8(keypad.cntrl,bit_offset);
                case 0x04: case 0x05:                           /* keypad size range */
                    return read8(keypad.size,bit_offset);
                case 0x08: case 0x09: case 0x0A: case 0x0B:     /* keypad interrupt acknowledge range */
                    return read8(keypad.interrupt_ack,bit_offset);
                case 0x0C: case 0x0D: case 0x0E: case 0x0F:     /* keypad interrupt mask range */
                    return read8(keypad.interrupt_mask,bit_offset);
                case 0x40: case 0x41: case 0x42: case 0x43:     /* GPIO interrupt acknowlegde range */
                    return read8(keypad.gpio_interrupt_ack,bit_offset);
                case 0x44: case 0x45: case 0x46: case 0x47:     /* GPIO interrupt mask range */
                    return read8(keypad.gpio_interrupt_mask,bit_offset);
                default:
                    return 0; /* return 0 if unimplemented or not in range */
            }
    }
}

/* Scan next row of keypad, if scanning is enabled */
static void keypad_scan_event(int index) {
    if (keypad.current_row >= 16) {
        return; /* too many keypad rows */
    }

    uint16_t row = ~keypad.key_map[keypad.current_row];
    row &= ~(0x80000 >> keypad.current_row); /* Emulate weird diagonal glitch */
    row |= -1 << (keypad.size >> 8 & 0xFF);  /* Unused columns read as 1 */
    row = ~row;

    if (keypad.data[keypad.current_row] != row) {
        keypad.data[keypad.current_row] = row;
        keypad.interrupt_ack |= 2;
    }

    keypad.current_row++;
    if (keypad.current_row < (keypad.size & 0xFF)) {
        event_repeat(index, keypad.cntrl >> 2 & 0x3FFF);
    } else {
        keypad.current_row = 0;
        keypad.interrupt_mask |= 1;
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
    uint16_t index = (int)pio & 0x7F;
    uint8_t bit_offset = (index&3)<<3;

    switch ( index & 0x7F ) {
        case 0x00: case 0x01: case 0x02: case 0x03:
        write8(keypad.cntrl,bit_offset,byte);
        if (keypad.cntrl & 2) {
            event_set(SCHED_KEYPAD, ((keypad.cntrl >> 16) + (keypad.cntrl >> 2 & 0x3FFF))/1000);
        } else {
            event_clear(SCHED_KEYPAD);
        }
        return;
        case 0x04: case 0x05:
            write8(keypad.size,bit_offset,byte);
            return;
        case 0x08: case 0x09: case 0x0A: case 0x0B:
            write8(keypad.interrupt_ack,bit_offset,byte);
            return;
        case 0x0C: case 0x0D: case 0x0E: case 0x0F:
            write8(keypad.interrupt_mask,bit_offset,byte);
            return;
        case 0x40: case 0x41: case 0x42: case 0x43:
            write8(keypad.gpio_interrupt_ack,bit_offset,byte);
            return;
        case 0x44: case 0x45: case 0x46: case 0x47:
            write8(keypad.gpio_interrupt_mask,bit_offset,byte);
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
}

static const eZ80portrange_t device = {
    .read_in    = keypad_read,
    .write_out  = keypad_write
};

eZ80portrange_t init_keypad(void) {
    gui_console_printf("Initialized keypad...\n");
    return device;
}
