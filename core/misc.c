#include <string.h>

#include "misc.h"
#include "schedule.h"
#include "emu.h"
#include "defines.h"
#include "interrupt.h"

watchdog_state_t watchdog;
protected_state_t protect;
cxxx_state_t cxxx; // Global CXXX state
dxxx_state_t dxxx; // Global DXXX state
exxx_state_t exxx; // Global EXXX state
fxxx_state_t fxxx; // Global FXXX state

static void watchdog_event(int index) {
    (void)index;

    if (watchdog.control & 1) {
        if (--watchdog.count == 0) {
            watchdog.status = 1;
            watchdog.count = watchdog.load;
        }
    }

    if ((watchdog.count == 0) && ((watchdog.control & 2) || (watchdog.control & 1))) {
        cpu_events |= EVENT_RESET;
        gui_console_printf("Watchdog reset triggered...");
    } else {
        //intrpt_trigger(INT_WATCHDOG, INTERRUPT_SET);  // TODO
        event_repeat(SCHED_WATCHDOG, watchdog.load);
    }
}

/* Watchdog read routine */
static uint8_t watchdog_read(const uint16_t pio) {
    uint8_t index = pio & 0xFFF;
    uint8_t bit_offset = (index&3)<<3;

    static const uint32_t revision = 0x00010602;

    switch (index) {
        case 0x000: case 0x001: case 0x002: case 0x003:
            return read8(watchdog.count,bit_offset);
        case 0x004: case 0x005: case 0x006: case 0x007:
            return read8(watchdog.load,bit_offset);
        case 0x00C:
            return read8(watchdog.control,bit_offset);
        case 0x010:
            return read8(watchdog.status,bit_offset);
        case 0x018:
            return read8(watchdog.intrpt_length,bit_offset);    // TODO: Find out what this does
        case 0x01C: case 0x01D: case 0x01E: case 0x01F:
            return read8(revision, bit_offset);
        default:
            break;
    }

    /* Return 0 if unimplemented */
    return 0;

}

/* Watchdog write routine */
static void watchdog_write(const uint16_t pio, const uint8_t byte) {
    uint8_t index = pio & 0xFFF;
    uint8_t bit_offset = (index&3)<<3;

    switch (index) {
        case 0x004: case 0x005: case 0x006: case 0x007:
            write8(watchdog.load,bit_offset,byte);
            return;
        case 0x008: case 0x009:
            write8(watchdog.restart,bit_offset,byte);
            if(watchdog.restart == 0x5AB9) {
                event_set(SCHED_WATCHDOG, watchdog.load);
                watchdog.count = watchdog.load;
            }
            return;
        case 0x00C:
            write8(watchdog.control,bit_offset,byte);
            if(watchdog.control & 1) {
                event_set(SCHED_WATCHDOG, watchdog.load);
            } else {
                event_clear(SCHED_WATCHDOG);
            }
            if(watchdog.control & 16) {
                sched.items[SCHED_WATCHDOG].clock = CLOCK_32K;
            } else {
                sched.items[SCHED_WATCHDOG].clock = CLOCK_CPU;
            }
            return;
        case 0x014:
            if(byte & 1) {
                watchdog.status = 0;
            }
            return;
        default:
            return;
    }
}

static const eZ80portrange_t pwatchdog = {
    .read_in    = watchdog_read,
    .write_out  = watchdog_write
};

void watchdog_reset() {
    /* Initialize device to default state */
    memset(&watchdog, 0, sizeof watchdog);

    sched.items[SCHED_WATCHDOG].clock = CLOCK_CPU;
    sched.items[SCHED_WATCHDOG].second = -1;
    sched.items[SCHED_WATCHDOG].proc = watchdog_event;
    watchdog.load = 0x03EF1480;   /* (66MHz) */
    watchdog.count = 0x03EF1480;

    gui_console_printf("Watchdog Timer Reset.\n");
}

eZ80portrange_t init_watchdog(void) {
    gui_console_printf("Initialized watchdog timer...\n");
    return pwatchdog;
}

/* ============================================= */

/* TODO: Is the (0x9XXX) range complete enough? */

/* Read from the 0x9XXX range of ports */
static uint8_t protected_read(const uint16_t pio) {

    uint8_t value = 0;

    switch (pio) {
        case 0xB00:
            value = protect.led_state;
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
            protect.led_state = byte;
            break;
        default:
            protect.unknown_ports[pio & 0xFF] = byte;
            break;
    }

    return;
}

static const eZ80portrange_t p9xxx = {
    .read_in    = protected_read,
    .write_out  = protected_write
};

eZ80portrange_t init_protected(void) {
    gui_console_printf("Initialized protected port range...\n");
    return p9xxx;
}

/* ============================================= */

/* Read from the 0xCXXX range of ports */
static uint8_t cxxx_read(const uint16_t pio) {
    return cxxx.ports[pio & 0xFF];
}

/* Write to the 0xCXXX range of ports */
static void cxxx_write(const uint16_t pio, const uint8_t byte) {
    cxxx.ports[pio & 0xFF] = byte;
}

static const eZ80portrange_t pcxxx = {
    .read_in    = cxxx_read,
    .write_out  = cxxx_write
};

eZ80portrange_t init_cxxx(void) {
    unsigned int i;
    /* Initialize device to default state */
    for(i = 0; i<sizeof(cxxx.ports) / sizeof(cxxx.ports[0]); i++) {
        cxxx.ports[i] = 0;
    }

    return pcxxx;
}

/* ============================================= */

/* TODO: Implement DXXX range -- USB related? */

/* Read from the 0xDXXX range of ports */
static uint8_t dxxx_read(const uint16_t pio) {
    return 0;
}

/* Write to the 0xDXXX range of ports */
static void dxxx_write(const uint16_t pio, const uint8_t byte) {
    return;
}

static const eZ80portrange_t pdxxx = {
    .read_in    = dxxx_read,
    .write_out  = dxxx_write
};

eZ80portrange_t init_dxxx(void) {
    return pdxxx;
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

/* ============================================= */

/* Write to the 0xFXXX range of ports -- This is unmapped, so no real point */
static void fxxx_write(const uint16_t pio, const uint8_t value) {
}

/* Read from the 0xFXXX range of ports */
static uint8_t fxxx_read(const uint16_t pio) {
    return 0;
}

static const eZ80portrange_t pfxxx = {
    .read_in    = fxxx_read,
    .write_out  = fxxx_write
};

eZ80portrange_t init_fxxx(void) {
    return pfxxx;
}
