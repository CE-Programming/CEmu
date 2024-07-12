#include "misc.h"
#include "control.h"
#include "cpu.h"
#include "schedule.h"
#include "emu.h"
#include "defines.h"
#include "interrupt.h"
#include "debug/debug.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>

watchdog_state_t watchdog;
protected_state_t protect;
cxxx_state_t cxxx; /* Global CXXX state */
fxxx_state_t fxxx; /* Global FXXX state */

static inline uint32_t watchdog_counter(void) {
    if (watchdog.mode != WATCHDOG_COUNTER || !sched_active(SCHED_WATCHDOG)) {
        return watchdog.count;
    }
    return sched_ticks_remaining(SCHED_WATCHDOG);
}

static inline uint8_t watchdog_pulse_counter(void) {
    if (watchdog.mode != WATCHDOG_PULSE || !sched_active(SCHED_WATCHDOG)) {
        return watchdog.pulseCount;
    }
    return sched_ticks_remaining(SCHED_WATCHDOG) - 1;
}

static void watchdog_pulse(uint8_t signals) {
    if (signals & 2) {
        cpu_crash("[CEmu] Reset triggered by watchdog timer.\n");
    }
    if (signals & 4) {
        gui_console_printf("[CEmu] Watchdog NMI triggered.\n");
        cpu_nmi();
    }
}

static inline enum clock_id watchdog_clock(void) {
    return watchdog.control & 0x10 ? CLOCK_32K : CLOCK_CPU;
}

static void watchdog_repeat_counter(enum sched_item_id id) {
    assert(watchdog.count != 0);
    if (watchdog.control & 1) {
        if (watchdog_clock() == CLOCK_32K) {
            sched_repeat(id, 0); /* re-activate to event cycle */
            sched_set_item_clock(id, CLOCK_32K);
        }
        sched_repeat(id, watchdog.count);
    }
    watchdog.mode = WATCHDOG_COUNTER;
}

static void watchdog_repeat_expired(enum sched_item_id id) {
    assert(watchdog.count == 0);
    sched_repeat(id, 1);
    watchdog.mode = WATCHDOG_EXPIRED;
}

static void watchdog_event(enum sched_item_id id) {
    switch (watchdog.mode) {
        case WATCHDOG_COUNTER: /* May be CLOCK_CPU or CLOCK_32K */
            watchdog.count = 0;
            if (watchdog_clock() != CLOCK_CPU) {
                sched_repeat(id, 0); /* re-activate to event cycle */
                sched_set_item_clock(id, CLOCK_CPU);
            }
            watchdog_repeat_expired(id);
            break;

        case WATCHDOG_PULSE: /* Always CLOCK_CPU */
            watchdog.pulseCount = watchdog.pulseLoad;
            watchdog.blockPulseReload = false;
            if (unlikely(watchdog.pendingReload)) {
                watchdog.pendingReload = false;
                sched_repeat(id, 1);
                watchdog.mode = WATCHDOG_RELOAD;
            } else if (likely(watchdog.count != 0)) {
                watchdog_repeat_counter(id);
            } else {
                watchdog_repeat_expired(id);
            }
            break;

        case WATCHDOG_RELOAD: /* Always CLOCK_CPU */
            watchdog.count = watchdog.load;
            if (likely(watchdog.count != 0)) {
                watchdog_repeat_counter(id);
                break;
            }
            fallthrough;
        case WATCHDOG_EXPIRED: /* Always CLOCK_CPU */
            watchdog.count = watchdog.load;
            if (watchdog.control & 1) {
                watchdog_pulse(watchdog.control);
                if (!watchdog.blockStatus) {
                    watchdog.status = 1;
                }
            }
            watchdog.blockStatus = false;
            if (watchdog.pulseCount == 0) {
                watchdog.blockPulseReload = true;
                sched_repeat(id, 1);
            } else if (watchdog.control & 1) {
                sched_repeat(id, watchdog.pulseCount + 1);
            }
            watchdog.mode = WATCHDOG_PULSE;
            break;

        default:
            unreachable();
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
            value = read8(watchdog_counter(), bit_offset);
            break;
        case 0x004: case 0x005: case 0x006: case 0x007:
            value = read8(watchdog.load, bit_offset);
            break;
        case 0x00C:
            value = watchdog.control;
            break;
        case 0x010:
            value = watchdog.status;
            break;
        case 0x018:
            value = watchdog_pulse_counter();
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
            if (watchdog.mode == WATCHDOG_PULSE && watchdog.count == 0) {
                watchdog.count = watchdog.load;
            }
            break;
        case 0x008:
            if (byte == 0xB9) {
                if (watchdog.mode == WATCHDOG_COUNTER && sched_active(SCHED_WATCHDOG)) {
                    watchdog.count = sched_ticks_remaining(SCHED_WATCHDOG);
                    sched_set_item_clock(SCHED_WATCHDOG, CLOCK_CPU);
                    sched_set(SCHED_WATCHDOG, 2);
                    watchdog.mode = WATCHDOG_RELOAD;
                } else if (watchdog.mode == WATCHDOG_PULSE && sched_active(SCHED_WATCHDOG) && sched_ticks_remaining(SCHED_WATCHDOG) == 1) {
                    watchdog.pendingReload = true;
                } else {
                    watchdog.count = watchdog.load;
                    if (watchdog.mode == WATCHDOG_COUNTER && unlikely(watchdog.count == 0)) {
                        assert(!(watchdog.control & 1));
                        sched_set_item_clock(SCHED_WATCHDOG, CLOCK_CPU);
                        if (unlikely(watchdog.pulseCount == 0)) {
                            watchdog.blockPulseReload = true;
                            sched_set(SCHED_WATCHDOG, 1);
                        }
                        watchdog.mode = WATCHDOG_PULSE;
                    }
                }
            }
            break;
        case 0x00C:
            old = watchdog.control;
            watchdog.control = byte;
            if (watchdog.mode == WATCHDOG_COUNTER) {
                sched_set_item_clock(SCHED_WATCHDOG, watchdog_clock());
            } else if (watchdog.mode == WATCHDOG_PULSE) {
                if (byte & old & 1) {
                    watchdog_pulse(byte & ~old);
                }
            }
            if ((byte ^ old) & 1) {
                if (byte & 1) {
                    if (!sched_active(SCHED_WATCHDOG)) {
                        if (watchdog.mode == WATCHDOG_COUNTER) {
                            assert(watchdog.count != 0);
                            sched_set(SCHED_WATCHDOG, watchdog.count);
                        } else if (watchdog.mode == WATCHDOG_PULSE) {
                            watchdog_pulse(byte);
                            sched_set(SCHED_WATCHDOG, watchdog.pulseCount + 1);
                        }
                    }
                } else {
                    assert(sched_active(SCHED_WATCHDOG));
                    if (watchdog.mode == WATCHDOG_COUNTER) {
                        watchdog.count = sched_ticks_remaining(SCHED_WATCHDOG);
                        sched_clear(SCHED_WATCHDOG);
                    } else if (watchdog.mode == WATCHDOG_PULSE) {
                        watchdog.pulseCount = sched_ticks_remaining(SCHED_WATCHDOG) - 1;
                        if (watchdog.pulseCount != 0) {
                            sched_clear(SCHED_WATCHDOG);
                        }
                    }
                }
            }
            break;
        case 0x014: case 0x015: case 0x016: case 0x017:
            watchdog.status = 0;
            if (unlikely(watchdog.mode == WATCHDOG_EXPIRED)) {
                assert(sched_ticks_remaining(SCHED_WATCHDOG) == 1);
                watchdog.blockStatus = true;
            }
            break;
        case 0x018:
            watchdog.pulseLoad = byte;
            fallthrough;
        case 0x019: case 0x01A: case 0x01B:
            if (!unlikely(watchdog.blockPulseReload)) {
                watchdog.pulseCount = watchdog.pulseLoad;
                if (watchdog.mode == WATCHDOG_PULSE && (watchdog.pulseCount == 0 || sched_active(SCHED_WATCHDOG))) {
                    sched_set(SCHED_WATCHDOG, watchdog.pulseCount + 2);
                }
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

    sched_init_event(SCHED_WATCHDOG, CLOCK_CPU, watchdog_event);
    watchdog.revision = 0x00010602;
    watchdog.count = watchdog.load = 0x03EF1480;   /* (66MHz) */
    watchdog.pulseCount = watchdog.pulseLoad = 0xFF;

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
