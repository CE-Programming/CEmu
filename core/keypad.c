#include "keypad.h"
#include "defines.h"
#include "emu.h"
#include "schedule.h"
#include "interrupt.h"
#include "control.h"
#include "asic.h"
#include "cpu.h"

#include <string.h>
#include <stdio.h>

/* Global KEYPAD state */
keypad_state_t keypad;

void keypad_intrpt_check() {
    intrpt_set(INT_KEYPAD, (keypad.status & keypad.enable) | (keypad.gpio_status & keypad.gpio_enable));
}

void EMSCRIPTEN_KEEPALIVE keypad_key_event(unsigned int row, unsigned int col, bool press) {
    if (row == 2 && col == 0) {
        intrpt_set(INT_ON, press);
        if (press && control.off) {
            control.readBatteryStatus = ~1;
            control.off = false;
            intrpt_pulse(INT_WAKE);
        }
    } else {
        if (press) {
            keypad.keyMap[row] |= 1 << col;
            keypad.delay[row] |= 1 << col;
            if (keypad.mode == 1) {
                keypad.status |= 4;
                keypad_intrpt_check();
            }
        } else {
            keypad.keyMap[row] &= ~(1 << col);
            keypad_intrpt_check();
        }
    }
}

static uint8_t keypad_read(const uint16_t pio, bool peek) {
    uint16_t index = (pio >> 2) & 0x7F;
    uint8_t bit_offset = (pio & 3) << 3;
    uint8_t value = 0;
    (void)peek;

    switch(index) {
        case 0x00:
            value = read8(keypad.control, bit_offset);
            break;
        case 0x01:
            value = read8(keypad.size, bit_offset);
            break;
        case 0x02:
            value = read8(keypad.status & keypad.enable, bit_offset);
            break;
        case 0x03:
            value = read8(keypad.enable, bit_offset);
            break;
        case 0x04: case 0x05: case 0x06: case 0x07:
        case 0x08: case 0x09: case 0x0A: case 0x0B:
            value = read8(keypad.data[(pio - 0x10) >> 1 & 15], pio << 3 & 8);
            break;
        case 0x10:
            value = read8(keypad.gpio_enable, bit_offset);
            break;
        case 0x11:
            value = read8(keypad.gpio_status, bit_offset);
            break;
        default:
            break;
    }

    /* return 0x00 if unimplemented or not in range */
    return value;
}

/* Scan next row of keypad, if scanning is enabled */
static void keypad_scan_event(enum sched_item_id id) {
    uint8_t row = keypad.current_row++;
    if (row < keypad.rows && row < sizeof(keypad.data) / sizeof(keypad.data[0])) {
        /* scan each data row */
        uint16_t data = keypad.keyMap[row] | keypad.delay[row];
        keypad.delay[row] = 0;
        data &= (1 << keypad.cols) - 1;

        /* if mode 3 or 2, generate data change interrupt */
        if (keypad.data[row] != data) {
            keypad.status |= 2;
            keypad.data[row] = data;
        }
    }
    if (keypad.current_row < keypad.rows) { /* scan the next row */
        sched_repeat(id, keypad.row_wait);
    } else { /* finished scanning the keypad */
        keypad.status |= 1;
        if (keypad.mode & 1) { /* are we in mode 1 or 3 */
            keypad.current_row = 0;
            sched_repeat(id, 2 + keypad.scan_wait + keypad.row_wait);
        } else {
            /* If in single scan mode, go to idle mode */
            keypad.mode = 0;
        }
    }
    keypad_intrpt_check();
}

static bool keypad_any_key_pressed(void) {
    int row;
    for (row = 0; row < keypad.rows; row++) {
        if ((keypad.keyMap[row] | keypad.delay[row]) & ((1 << keypad.cols) - 1)) {
            return true;
        }
    }
    return false;
}

static void keypad_write(const uint16_t pio, const uint8_t byte, bool poke) {
    uint16_t index = (pio >> 2) & 0x7F;
    uint8_t bit_offset = (pio & 3) << 3;

    switch (index) {
        case 0x00:
            write8(keypad.control,bit_offset,byte);
            if (keypad.mode & 2) {
                keypad.current_row = 0;
                sched_set(SCHED_KEYPAD, keypad.row_wait);
            } else {
                sched_clear(SCHED_KEYPAD);
                if (keypad.mode == 1 && keypad_any_key_pressed()) {
                    keypad.status |= 4;
                    keypad_intrpt_check();
                }
            }
            break;
        case 0x01:
            write8(keypad.size, bit_offset, byte);
            break;
        case 0x02:
            write8(keypad.status, bit_offset, keypad.status >> bit_offset & ~byte);
            if (keypad.mode == 1 && keypad_any_key_pressed()) {
                keypad.status |= 4;
            }
            keypad_intrpt_check();
            break;
        case 0x03:
            write8(keypad.enable, bit_offset, byte & 7);
            keypad_intrpt_check();
            break;
        case 0x04: case 0x05: case 0x06: case 0x07:
        case 0x08: case 0x09: case 0x0A: case 0x0B:
            if (poke) {
                write8(keypad.data[(pio - 0x10) >> 1 & 15], pio << 3 & 8, byte);
                write8(keypad.keyMap[pio >> 1 & 15], pio << 3 & 8, byte);
            }
            break;
        case 0x10:
            write8(keypad.gpio_enable, bit_offset, byte);
            keypad_intrpt_check();
            break;
        case 0x11:
            write8(keypad.gpio_status, bit_offset, keypad.gpio_status >> bit_offset & ~byte);
            keypad_intrpt_check();
            break;
        default:
            break;  /* Escape write sequence if unimplemented */
    }
}

void keypad_reset() {
    keypad.current_row = 0;
    memset(keypad.data, 0, sizeof(keypad.data));
    memset(keypad.keyMap, 0, sizeof(keypad.keyMap));
    memset(keypad.delay, 0, sizeof(keypad.delay));

    sched.items[SCHED_KEYPAD].callback.event = keypad_scan_event;
    sched.items[SCHED_KEYPAD].clock = CLOCK_6M;
    sched_clear(SCHED_KEYPAD);

    gui_console_printf("[CEmu] Keypad reset.\n");
}

static const eZ80portrange_t device = {
    .read  = keypad_read,
    .write = keypad_write
};

eZ80portrange_t init_keypad(void) {
    gui_console_printf("[CEmu] Initialized Keypad...\n");
    return device;
}

bool keypad_save(FILE *image) {
    return fwrite(&keypad, sizeof(keypad), 1, image) == 1;
}

bool keypad_restore(FILE *image) {
    return fread(&keypad, sizeof(keypad), 1, image) == 1;
}
