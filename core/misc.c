#include "misc.h"
#include "control.h"
#include "cpu.h"
#include "schedule.h"
#include "emu.h"
#include "defines.h"
#include "interrupt.h"
#include "debug/debug.h"

#include <string.h>
#include <stdio.h>

watchdog_state_t watchdog;
protected_state_t protect;
cxxx_state_t cxxx; /* Global CXXX state */
exxx_state_t exxx; /* Global EXXX state */
fxxx_state_t fxxx; /* Global FXXX state */

static void watchdog_event(enum sched_item_id id) {

    if (watchdog.control & 1) {
        watchdog.status = 1;
        if (watchdog.control & 2) {
            cpu_crash("[CEmu] Reset triggered by watchdog timer.\n");
        }
        if (watchdog.control & 4) {
            gui_console_printf("[CEmu] Watchdog NMI triggered.\n");
            cpu_nmi();
        }
        sched_repeat(id, watchdog.load);
    }
}

/* Watchdog read routine */
static uint8_t watchdog_read(const uint16_t pio, bool peek) {
    uint8_t index = pio;
    uint8_t bit_offset = (index & 3) << 3;
    uint8_t value = 0;
    (void)peek;

    switch (index) {
        case 0x000: case 0x001: case 0x002: case 0x003:
            if (watchdog.control & 1) {
                value = read8(sched_ticks_remaining(SCHED_WATCHDOG), bit_offset);
            } else {
                value = watchdog.count;
            }
            break;
        case 0x004: case 0x005: case 0x006: case 0x007:
            value = read8(watchdog.load, bit_offset);
            break;
        case 0x00C:
            value = read8(watchdog.control, bit_offset);
            break;
        case 0x010:
            value = read8(watchdog.status, bit_offset);
            break;
        case 0x018:
            value = read8(watchdog.length, bit_offset);
            break;
        case 0x01C: case 0x01D: case 0x01E: case 0x01F:
            value = read8(watchdog.revision, bit_offset);
            break;
        default:
            break;
    }

    /* Return 0 if unimplemented */
    return value;
}

/* Watchdog write routine */
static void watchdog_write(const uint16_t pio, const uint8_t byte, bool poke) {
    uint32_t old;
    uint8_t index = pio;
    uint8_t bit_offset = (index & 3) << 3;

    (void)poke;

    switch (index) {
        case 0x004: case 0x005: case 0x006: case 0x007:
            write8(watchdog.load, bit_offset, byte);
            break;
        case 0x008: case 0x009:
            write8(watchdog.restart, bit_offset, byte);
            if (watchdog.restart == 0x5AB9) {
                sched_set(SCHED_WATCHDOG, watchdog.load);
                watchdog.count = watchdog.load;
                watchdog.restart = 0;
            }
            break;
        case 0x00C:
            old = watchdog.control;
            write8(watchdog.control, bit_offset, byte);                    
            if (watchdog.control & 16) {
                sched.items[SCHED_WATCHDOG].clock = CLOCK_32K;
            } else {
                sched.items[SCHED_WATCHDOG].clock = CLOCK_CPU;
            }
            if ((watchdog.control ^ old) & 1) {
                if (watchdog.control & 1) {
                    sched_set(SCHED_WATCHDOG, watchdog.load);
                } else {
                    watchdog.count = sched_ticks_remaining(SCHED_WATCHDOG);
                    sched_clear(SCHED_WATCHDOG);
                }
            }
            break;
        case 0x014:
            if (byte & 1) {
                watchdog.status = 0;
            }
            break;
        default:
            break;
    }
}

static const eZ80portrange_t pwatchdog = {
    .read  = watchdog_read,
    .write = watchdog_write
};

void watchdog_reset() {
    /* Initialize device to default state */
    memset(&watchdog, 0, sizeof watchdog);

    sched.items[SCHED_WATCHDOG].callback.event = watchdog_event;
    sched.items[SCHED_WATCHDOG].clock = CLOCK_CPU;
    sched_clear(SCHED_WATCHDOG);
    watchdog.revision = 0x00010602;
    watchdog.load = 0x03EF1480;   /* (66MHz) */
    watchdog.count = 0x03EF1480;

    gui_console_printf("[CEmu] Watchdog timer reset.\n");
}

eZ80portrange_t init_watchdog(void) {
    gui_console_printf("[CEmu] Initialized Watchdog Timer...\n");
    return pwatchdog;
}

bool watchdog_save(FILE *image) {
    return fwrite(&watchdog, sizeof(watchdog), 1, image) == 1;
}

bool watchdog_restore(FILE *image) {
    return fread(&watchdog, sizeof(watchdog), 1, image) == 1;
}

/* ============================================= */

/* TODO: Is the (0x9XXX) range complete enough? */

/* Read from the 0x9XXX range of ports */
static uint8_t protected_read(const uint16_t pio, bool peek) {
    uint8_t value = 0;
    if (peek || protected_ports_unlocked()) {
        switch (pio) {
            case 0xB00:
                value = protect.led;
                break;
            default:
                value = protect.ports[pio & 0xFF];
                break;
        }
    }
    return value;
}

/* Write to the 0x9XXX range of ports */
static void protected_write(const uint16_t pio, const uint8_t byte, bool poke) {
    if (poke || protected_ports_unlocked()) {
        switch (pio) {
            case 0xB00:
                protect.led = byte;
                break;
            default:
                protect.ports[pio & 0xFF] = byte;
                break;
        }
    }
}

static const eZ80portrange_t p9xxx = {
    .read  = protected_read,
    .write = protected_write
};

eZ80portrange_t init_protected(void) {
    gui_console_printf("[CEmu] Initialized Protected Ports...\n");
    return p9xxx;
}

bool protect_save(FILE *image) {
    return fwrite(&protect, sizeof(protect), 1, image) == 1;
}

bool protect_restore(FILE *image) {
    return fread(&protect, sizeof(protect), 1, image) == 1;
}

/* ============================================= */

/* Read from the 0xCXXX range of ports */
static uint8_t cxxx_read(const uint16_t pio, bool peek) {
    (void)peek;
    return cxxx.ports[pio];
}

/* Write to the 0xCXXX range of ports */
static void cxxx_write(const uint16_t pio, const uint8_t byte, bool poke) {
    (void)poke;
    cxxx.ports[pio] = byte;
}

static const eZ80portrange_t pcxxx = {
    .read  = cxxx_read,
    .write = cxxx_write
};

eZ80portrange_t init_cxxx(void) {
    memset(&cxxx, 0, sizeof(cxxx));
    return pcxxx;
}

bool cxxx_save(FILE *image) {
    return fwrite(&cxxx, sizeof(cxxx), 1, image) == 1;
}

bool cxxx_restore(FILE *image) {
    return fread(&cxxx, sizeof(cxxx), 1, image) == 1;
}

/* ============================================= */

/* Read from the 0xEXXX range of ports */
static uint8_t exxx_read(const uint16_t pio, bool peek) {
    uint8_t index = pio & 0x7F;
    uint8_t read_byte;
    (void)peek;

    switch (index) {
        case 0x14:
            read_byte = 32 | exxx.ports[index];
            break;
        default:
            read_byte = exxx.ports[index];
            break;
    }
    return read_byte;
}

/* Write to the 0xEXXX range of ports */
static void exxx_write(const uint16_t pio, const uint8_t byte, bool poke) {
    (void)poke;
    exxx.ports[pio & 0x7F] = byte;
}

static const eZ80portrange_t pexxx = {
    .read  = exxx_read,
    .write = exxx_write
};

eZ80portrange_t init_exxx(void) {
    memset(&exxx, 0, sizeof(exxx));
    return pexxx;
}

bool exxx_save(FILE *image) {
    return fwrite(&exxx, sizeof(exxx), 1, image) == 1;
}

bool exxx_restore(FILE *image) {
    return fread(&exxx, sizeof(exxx), 1, image) == 1;
}

/* ============================================= */

/* Write to the 0xFXXX range of ports */
static void fxxx_write(const uint16_t pio, const uint8_t value, bool poke) {
    (void)poke;
    /* 0xFFE appears to dump the contents of flash. Probably not a good thing to print to a console :) */
    if (pio != 0xFFF) {
        return;
    }

#ifdef DEBUG_SUPPORT
    if (value != 0) {
        debug.buffer[debug.bufPos] = (char)value;
        debug.bufPos = (debug.bufPos + 1) % (SIZEOF_DBG_BUFFER);
    }
#else
    (void)value;
#endif
}

/* Read from the 0xFXXX range of ports */
static uint8_t fxxx_read(const uint16_t pio, bool peek) {
    (void)pio; /* Uncomment me when needed */
    (void)peek;
    return 0;
}

static const eZ80portrange_t pfxxx = {
    .read  = fxxx_read,
    .write = fxxx_write
};

eZ80portrange_t init_fxxx(void) {
    return pfxxx;
}
