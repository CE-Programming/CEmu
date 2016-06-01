#include <string.h>

#include "cpu.h"
#include "misc.h"
#include "schedule.h"
#include "emu.h"
#include "defines.h"
#include "interrupt.h"
#include "debug/debug.h"

watchdog_state_t watchdog;
protected_state_t protect;
cxxx_state_t cxxx; /* Global CXXX state */
dxxx_state_t dxxx; /* Global DXXX state */
exxx_state_t exxx; /* Global EXXX state */
fxxx_state_t fxxx; /* Global FXXX state */

static void watchdog_event(int index) {

    if (watchdog.control & 1) {
        watchdog.status = 1;
        if (watchdog.control & 2) {
            gui_console_printf("[CEmu] Watchdog reset triggered.\n");
            cpuEvents |= EVENT_RESET;
        }
        if (watchdog.control & 4) {
            cpu_nmi();
            gui_console_printf("[CEmu] Watchdog NMI triggered.\n");
        }
        event_repeat(index, watchdog.load);
    }
}

/* Watchdog read routine */
static uint8_t watchdog_read(const uint16_t pio) {
    uint8_t index = pio;
    uint8_t bit_offset = (index & 3) << 3;
    uint8_t value = 0;

    switch (index) {
        case 0x000: case 0x001: case 0x002: case 0x003:
            if (watchdog.control & 1) {
                value = read8(event_ticks_remaining(SCHED_WATCHDOG), bit_offset);
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
            value = read8(watchdog.intrptLength, bit_offset);
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
static void watchdog_write(const uint16_t pio, const uint8_t byte) {
    uint8_t index = pio;
    uint8_t bit_offset = (index & 3) << 3;

    switch (index) {
        case 0x004: case 0x005: case 0x006: case 0x007:
            write8(watchdog.load, bit_offset, byte);
            break;
        case 0x008: case 0x009:
            write8(watchdog.restart, bit_offset, byte);
            if(watchdog.restart == 0x5AB9) {
                event_set(SCHED_WATCHDOG, watchdog.load);
                watchdog.count = watchdog.load;
                watchdog.restart = 0;
            }
            break;
        case 0x00C:
            write8(watchdog.control, bit_offset, byte);                    
            if(watchdog.control & 1) {
                event_set(SCHED_WATCHDOG, watchdog.load);
            } else {
                watchdog.count = event_ticks_remaining(SCHED_WATCHDOG);
                event_clear(SCHED_WATCHDOG);
            }
            if(watchdog.control & 16) {
                sched.items[SCHED_WATCHDOG].clock = CLOCK_32K;
            } else {
                sched.items[SCHED_WATCHDOG].clock = CLOCK_CPU;
            }
            break;
        case 0x014:
            if(byte & 1) {
                watchdog.status = 0;
            }
            break;
        default:
            break;
    }
}

static const eZ80portrange_t pwatchdog = {
    .read_in    = watchdog_read,
    .write_out  = watchdog_write
};

void watchdog_reset() {
    /* Initialize device to default state */
    memset(&watchdog, 0, sizeof watchdog);

    sched.items[SCHED_WATCHDOG].clock = CLOCK_APB;
    sched.items[SCHED_WATCHDOG].second = -1;
    sched.items[SCHED_WATCHDOG].proc = watchdog_event;
    watchdog.revision = 0x00010602;
    watchdog.load = 0x03EF1480;   /* (66MHz) */
    watchdog.count = 0x03EF1480;

    gui_console_printf("[CEmu] Watchdog timer reset.\n");
}

eZ80portrange_t init_watchdog(void) {
    gui_console_printf("[CEmu] Initialized Watchdog Timer...\n");
    return pwatchdog;
}

bool watchdog_save(emu_image *s) {
    s->watchdog = watchdog;
    return true;
}

bool watchdog_restore(const emu_image *s) {
    watchdog = s->watchdog;
    return true;
}

/* ============================================= */

/* TODO: Is the (0x9XXX) range complete enough? */

/* Read from the 0x9XXX range of ports */
static uint8_t protected_read(const uint16_t pio) {

    uint8_t value = 0;

    switch (pio) {
        case 0xB00:
            value = protect.ledState;
            break;
        default:
            value = protect.unknown_ports[pio & 0xFF];
            break;
    }
    return value;
}

/* Write to the 0x9XXX range of ports */
static void protected_write(const uint16_t pio, const uint8_t byte) {

    switch (pio) {
        case 0xB00:
            protect.ledState = byte;
            break;
        default:
            protect.unknown_ports[pio] = byte;
            break;
    }

    return;
}

static const eZ80portrange_t p9xxx = {
    .read_in    = protected_read,
    .write_out  = protected_write
};

eZ80portrange_t init_protected(void) {
    gui_console_printf("[CEmu] Initialized Protected Ports...\n");
    return p9xxx;
}

bool protect_save(emu_image *s) {
    s->protect = protect;
    return true;
}

bool protect_restore(const emu_image *s) {
    protect = s->protect;
    return true;
}

/* ============================================= */

/* Read from the 0xCXXX range of ports */
static uint8_t cxxx_read(const uint16_t pio) {
    return cxxx.ports[pio];
}

/* Write to the 0xCXXX range of ports */
static void cxxx_write(const uint16_t pio, const uint8_t byte) {
    cxxx.ports[pio] = byte;
}

static const eZ80portrange_t pcxxx = {
    .read_in    = cxxx_read,
    .write_out  = cxxx_write
};

eZ80portrange_t init_cxxx(void) {
    memset(&cxxx, 0, sizeof cxxx);
    return pcxxx;
}

bool cxxx_save(emu_image *s) {
    s->cxxx = cxxx;
    return true;
}

bool cxxx_restore(const emu_image *s) {
    cxxx = s->cxxx;
    return true;
}

/* ============================================= */

/* TODO: Implement DXXX range -- USB related? */

/* Read from the 0xDXXX range of ports */
static uint8_t dxxx_read(const uint16_t pio) {
    (void)pio; /* Uncomment me when needed */
    return 0;
}

/* Write to the 0xDXXX range of ports */
static void dxxx_write(const uint16_t pio, const uint8_t byte) {
    (void)pio;  /* Uncomment me when needed */
    (void)byte; /* Uncomment me when needed */
    return;
}

static const eZ80portrange_t pdxxx = {
    .read_in    = dxxx_read,
    .write_out  = dxxx_write
};

eZ80portrange_t init_dxxx(void) {
    return pdxxx;
}

bool dxxx_save(emu_image *s) {
    s->dxxx = dxxx;
    return true;
}

bool dxxx_restore(const emu_image *s) {
    dxxx = s->dxxx;
    return true;
}

/* ============================================= */

/* Read from the 0xEXXX range of ports */
static uint8_t exxx_read(const uint16_t pio) {
    uint8_t index = pio & 0x7F;
    uint8_t read_byte;

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
static void exxx_write(const uint16_t pio, const uint8_t byte) {
    exxx.ports[pio & 0x7F] = byte;
}

static const eZ80portrange_t pexxx = {
    .read_in    = exxx_read,
    .write_out  = exxx_write
};

eZ80portrange_t init_exxx(void) {
    memset(&exxx, 0, sizeof exxx);
    return pexxx;
}

bool exxx_save(emu_image *s) {
    s->exxx = exxx;
    return true;
}

bool exxx_restore(const emu_image *s) {
    exxx = s->exxx;
    return true;
}

/* ============================================= */

/* Write to the 0xFXXX range of ports */
static void fxxx_write(const uint16_t pio, const uint8_t value) {
    /* 0xFFE appears to dump the contents of flash. Probably not a good thing to print to a console :) */
    if (pio != 0xFFF) {
        return;
    }

#ifdef DEBUG_SUPPORT
    if (value != 0) {
        debugger.buffer[debugger.currentBuffPos] = (char)value;
        debugger.currentBuffPos = (debugger.currentBuffPos + 1) % (SIZEOF_DBG_BUFFER);
    }
#else
    (void)value; /* Uncomment me when needed */
#endif
}

/* Read from the 0xFXXX range of ports */
static uint8_t fxxx_read(const uint16_t pio) {
    (void)pio; /* Uncomment me when needed */
    return 0;
}

static const eZ80portrange_t pfxxx = {
    .read_in    = fxxx_read,
    .write_out  = fxxx_write
};

eZ80portrange_t init_fxxx(void) {
    return pfxxx;
}
