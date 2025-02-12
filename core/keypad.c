#include "keypad.h"
#include "atomics.h"
#include "defines.h"
#include "emu.h"
#include "schedule.h"
#include "interrupt.h"
#include "control.h"
#include "asic.h"
#include "cpu.h"

#include <string.h>
#include <stdio.h>

#define ON_KEY_PRESSED 1
#define ON_KEY_EDGE 2

/* Global KEYPAD state */
keypad_state_t keypad;

typedef struct keypad_atomics {
    _Atomic(uint16_t) keyMap[KEYPAD_ACTUAL_ROWS];
    _Atomic(uint8_t) onKey;
    _Atomic(bool) ghosting;
} keypad_atomics_t;

static keypad_atomics_t keypad_atomics;

void emu_set_keypad_ghosting(int enable) {
    keypad_atomics.ghosting = enable;
}

static inline void keypad_intrpt_check(void) {
    intrpt_set(INT_KEYPAD, keypad.status & keypad.enable);
}

static inline uint8_t keypad_row_limit(void) {
    return keypad.rows >= KEYPAD_MAX_ROWS ? KEYPAD_MAX_ROWS : keypad.rows;
}

static inline uint8_t keypad_actual_row_limit(void) {
    return keypad.rows >= KEYPAD_ACTUAL_ROWS ? KEYPAD_ACTUAL_ROWS : keypad.rows;
}

static inline uint16_t keypad_data_mask(void) {
    uint8_t colLimit = keypad.cols >= KEYPAD_ACTUAL_COLS ? KEYPAD_ACTUAL_COLS : keypad.cols;
    return (1 << colLimit) - 1;
}

static inline uint8_t keypad_query_keymap(uint8_t row) {
    uint32_t data = atomic16_fetch_and_explicit(&keypad_atomics.keyMap[row], (1 << KEYPAD_ACTUAL_COLS) - 1, memory_order_relaxed);
    return (data | data >> KEYPAD_ACTUAL_COLS) & ((1 << KEYPAD_ACTUAL_COLS) - 1);
}

static inline uint8_t keypad_peek_keymap(uint8_t row) {
    uint32_t data = atomic_load_explicit(&keypad_atomics.keyMap[row], memory_order_relaxed);
    return data & ((1 << KEYPAD_ACTUAL_COLS) - 1);
}

static uint64_t matrix_transpose(uint64_t matrix) {
    /* See Hacker's Delight 2nd edition section 7-3 */
    uint64_t temp;

    /* Transpose 2x2 submatrices */
    temp = (matrix ^ (matrix >> 7)) & UINT64_C(0x00AA00AA00AA00AA);
    matrix ^= temp ^ (temp << 7);
    /* Transpose 4x4 submatrices */
    temp = (matrix ^ (matrix >> 14)) & UINT64_C(0x0000CCCC0000CCCC);
    matrix ^= temp ^ (temp << 14);
    /* Transpose 8x8 matrix */
    temp = (matrix ^ (matrix >> 28)) & UINT64_C(0x00000000F0F0F0F0);
    matrix ^= temp ^ (temp << 28);

    return matrix;
}

static uint64_t matrix_mul(uint64_t a, uint64_t b) {
    uint64_t result = 0;
    do {
        result |= (a & UINT64_C(0x0101010101010101)) * (uint8_t)b;
        a >>= 1;
        b >>= 8;
    } while (b);
    return result;
}

static uint8_t keypad_handle_ghosting(uint8_t data, uint8_t queryMask) {
    /* build a bit matrix with the initial data as the first row */
    uint64_t ghostMatrix = data;
    uint8_t peekRows = 0;
    uint8_t ghostData = 0;
    uint8_t shift = 0;
    for (uint8_t row = 0; row < KEYPAD_ACTUAL_ROWS; row++) {
        if (!(queryMask & (1 << row))) {
            uint8_t peekData = keypad_peek_keymap(row);
            /* only rows with at least two bits set can contribute to ghosting */
            if (peekData & (peekData - 1)) {
                ghostData |= peekData;
                ghostMatrix |= (uint64_t)peekData << (++peekRows * 8);
            }
        }
    }
    if ((data & ghostData) == 0) {
        /* if no other rows have keys pressed in the same columns, ghosting is impossible */
        return data;
    } else if (--peekRows == 0) {
        /* if exactly one other row has keys pressed in the same columns, combine directly */
        return data | ghostData;
    }
    /* now determine the reachable columns when treating the keypresses as a biadjacency matrix */
    /* first get a row adjacency matrix by multiplying the biadjacency matrix by its transpose */
    uint64_t adjacency = matrix_mul(ghostMatrix, matrix_transpose(ghostMatrix));
    /* repeatedly double the number of steps to get reachability to all peeked rows */
    do {
        adjacency = matrix_mul(adjacency, adjacency);
    } while (peekRows >>= 1);
    /* get mask of reachability from the first row (which is also the first column in the adjacency matrix) */
    adjacency &= UINT64_C(0x0101010101010101);
    adjacency = (adjacency << 8) - adjacency;
    /* combine columns from all reachable rows */
    ghostMatrix &= adjacency;
    uint32_t ghostMatrix32 = ghostMatrix | (ghostMatrix >> 32);
    uint16_t ghostMatrix16 = ghostMatrix32 | (ghostMatrix32 >> 16);
    uint8_t ghostMatrix8 = ghostMatrix16 | (ghostMatrix16 >> 8);
    return ghostMatrix8 & (1 << KEYPAD_ACTUAL_COLS) - 1;
}

void keypad_any_check(void) {
    uint8_t row;
    if (keypad.mode != 1) {
        return;
    }
    uint16_t any = 0, mask = keypad.mask;
    uint8_t rowLimit = keypad_actual_row_limit();
    uint8_t queryMask = mask & ((1 << rowLimit) - 1);
    for (row = 0; row < rowLimit; row++) {
        if (queryMask & (1 << row)) {
            any |= keypad_query_keymap(row);
        }
    }
    uint16_t dataMask = keypad_data_mask();
    /* if not all rows were queried, ghosting is possible */
    if (unlikely(queryMask != (1 << KEYPAD_ACTUAL_ROWS) - 1) &&
        atomic_load_explicit(&keypad_atomics.ghosting, memory_order_relaxed)) {
        /* if at least one key was pressed and not every data bit is filled, ghosting is possible */
        if (any != 0 && (any & dataMask) != dataMask) {
            any = keypad_handle_ghosting(any, queryMask);
        }
    }
    any &= dataMask;
    rowLimit = keypad_row_limit();
    for (row = 0; row < rowLimit; row++) {
        if (mask & (1 << row)) {
            keypad.data[row] = any;
        }
    }
    if (any & mask) {
        keypad.status |= 4;
        keypad_intrpt_check();
    }
}

void keypad_on_check(void) {
    uint8_t onState = atomic8_fetch_and_explicit(&keypad_atomics.onKey, ON_KEY_PRESSED, memory_order_relaxed);
    intrpt_set(INT_ON, onState);
    if (onState == ON_KEY_EDGE) {
        intrpt_set(INT_ON, false);
    }
    if ((onState & ON_KEY_EDGE) && control.off) {
        control.readBatteryStatus = ~1;
        control.off = false;
        intrpt_pulse(INT_WAKE);
    }
}

void EMSCRIPTEN_KEEPALIVE emu_keypad_event(unsigned int row, unsigned int col, bool press) {
    if (row == 2 && col == 0) {
        if (press) {
            atomic8_fetch_or_explicit(&keypad_atomics.onKey, ON_KEY_EDGE | ON_KEY_PRESSED, memory_order_relaxed);
        } else {
            atomic8_fetch_and_explicit(&keypad_atomics.onKey, ~ON_KEY_PRESSED, memory_order_relaxed);
        }
        cpu_set_signal(CPU_SIGNAL_ON_KEY);
    } else if (row < KEYPAD_ACTUAL_ROWS && col < KEYPAD_ACTUAL_COLS) {
        if (press) {
            atomic16_fetch_or_explicit(&keypad_atomics.keyMap[row], (1 | 1 << KEYPAD_ACTUAL_COLS) << col, memory_order_relaxed);
        } else {
            atomic16_fetch_and_explicit(&keypad_atomics.keyMap[row], ~(1 << col), memory_order_relaxed);
        }
        cpu_set_signal(CPU_SIGNAL_ANY_KEY);
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
            value = read8(keypad.gpioEnable, bit_offset);
            break;
        case 0x11: /* GPIO status is always 0 */
        default:
            break;
    }

    /* return 0x00 if unimplemented or not in range */
    return value;
}

/* Scan next row of keypad, if scanning is enabled */
static void keypad_scan_event(enum sched_item_id id) {
    uint8_t row = keypad.row++;
    if (row < keypad_row_limit()) {
        /* scan each data row */
        uint16_t data = 0;
        if (row < KEYPAD_ACTUAL_ROWS) {
            data = keypad_query_keymap(row);
            uint16_t dataMask = keypad_data_mask();
            /* if at least one key was pressed and not every data bit is filled, ghosting is possible */
            if (atomic_load_explicit(&keypad_atomics.ghosting, memory_order_relaxed) &&
                data != 0 && (data & dataMask) != dataMask) {
                data = keypad_handle_ghosting(data, 1 << row);
            }
            data &= dataMask;
        }

        /* if mode 3 or 2, generate data change interrupt */
        if (keypad.data[row] != data) {
            keypad.status |= 2;
            keypad.data[row] = data;
        }
    }
    if (keypad.row < keypad.rows) { /* scan the next row */
        sched_repeat(id, keypad.rowWait);
    } else { /* finished scanning the keypad */
        keypad.status |= 1;
        if (keypad.mode & 1) { /* are we in mode 1 or 3 */
            keypad.row = 0;
            sched_repeat(id, 2 + keypad.scanWait + keypad.rowWait);
        } else {
            /* If in single scan mode, go to idle mode */
            keypad.mode = 0;
        }
    }
    keypad_intrpt_check();
}

static void keypad_write(const uint16_t pio, const uint8_t byte, bool poke) {
    uint16_t index = (pio >> 2) & 0x7F;
    uint8_t bit_offset = (pio & 3) << 3;

    switch (index) {
        case 0x00:
            write8(keypad.control, bit_offset, byte);
            if (keypad.mode & 2) {
                keypad.row = 0;
                sched_set(SCHED_KEYPAD, keypad.rowWait);
            } else {
                sched_clear(SCHED_KEYPAD);
                keypad_any_check();
            }
            break;
        case 0x01:
            write8(keypad.size, bit_offset, byte);
            keypad_any_check();
            break;
        case 0x02:
            write8(keypad.status, bit_offset, keypad.status >> bit_offset & ~byte);
            keypad_any_check();
            keypad_intrpt_check();
            break;
        case 0x03:
            write8(keypad.enable, bit_offset, byte & 7);
            keypad_intrpt_check();
            break;
        case 0x04: case 0x05: case 0x06: case 0x07:
        case 0x08: case 0x09: case 0x0A: case 0x0B:
            if (poke) {
                write8(keypad.data[(pio - 0x10) >> 1 & (KEYPAD_MAX_ROWS - 1)], pio << 3 & 8, byte);
                if ((pio & 0x21) == 0) {
                    write8(keypad_atomics.keyMap[pio >> 1 & (KEYPAD_ACTUAL_ROWS - 1)], 0, byte);
                }
            }
            break;
        case 0x10:
            write8(keypad.gpioEnable, bit_offset, byte);
            break;
        case 0x11:  /* GPIO status is always 0, no 1 bits to reset */
        default:
            break;  /* Escape write sequence if unimplemented */
    }
}

static void keypad_init_events(void) {
    sched_init_event(SCHED_KEYPAD, CLOCK_6M, keypad_scan_event);
}

void keypad_reset(void) {
    keypad.row = 0;
    keypad.mask = 0xFFFF;

    keypad_init_events();

    gui_console_printf("[CEmu] Keypad reset.\n");
}

static const eZ80portrange_t device = {
    .read  = keypad_read,
    .write = keypad_write
};

eZ80portrange_t init_keypad(void) {
    keypad.row = 0;

    memset(keypad.data, 0, sizeof(keypad.data));

    gui_console_printf("[CEmu] Initialized Keypad...\n");
    return device;
}

bool keypad_save(FILE *image) {
    return fwrite(&keypad, sizeof(keypad), 1, image) == 1;
}

bool keypad_restore(FILE *image) {
    keypad_init_events();
    return fread(&keypad, sizeof(keypad), 1, image) == 1;
}
