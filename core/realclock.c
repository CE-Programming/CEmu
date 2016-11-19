#include <string.h>

#include "realclock.h"
#include "emu.h"
#include "schedule.h"
#include "interrupt.h"

/* Global GPT state */
rtc_state_t rtc;

static void rtc_event(int index) {
    /* Update exactly once a second */
    event_repeat(index, 32768);

    if (rtc.control & 64) { /* (Bit 6) -- Load time */
        rtc.readSec = rtc.writeSec;
        rtc.readMin = rtc.writeMin;
        rtc.readHour = rtc.writeHour;
        rtc.readDay = rtc.writeDay;
        rtc.control &= ~64;
        rtc.interrupt |= 32;  /* Load operation complete */
        intrpt_set(INT_RTC, true);
    }

    rtc.readSec++;
    if (rtc.control & 2) {
        rtc.interrupt |= 1;
        intrpt_set(INT_RTC, true);
    }
    if (rtc.readSec > 59) {
        rtc.readSec = 0;
        if (rtc.control & 4) {
            rtc.interrupt |= 2;
            intrpt_set(INT_RTC, true);
        }
        rtc.readMin++;
        if (rtc.readMin > 59) {
            rtc.readMin = 0;
            if (rtc.control & 8) {
                rtc.interrupt |= 4;
                intrpt_set(INT_RTC, true);
            }
            rtc.readHour++;
            if (rtc.readHour > 23) {
                rtc.readHour = 0;
                if (rtc.control & 16) {
                    rtc.interrupt |= 8;
                    intrpt_set(INT_RTC, true);
                }
                rtc.readDay++;
            }
        }
    }

    if ((rtc.control & 32) && (rtc.readSec == rtc.alarmSec) &&
        (rtc.readMin == rtc.alarmMin) && (rtc.readHour == rtc.alarmHour)) {
            rtc.interrupt |= 16;
            intrpt_set(INT_RTC, true);
    }
}

static void hold_read(void) {
    rtc.holdSec = rtc.readSec;
    rtc.holdMin = rtc.readMin;
    rtc.holdHour = rtc.readHour;
    rtc.holdDay = rtc.readDay;
}

static uint8_t rtc_read(const uint16_t pio, bool peek) {
    uint8_t index = pio & 0xFF;
    uint8_t bit_offset = (index & 3) << 3;

    uint8_t value = 0;
    (void)peek;

    switch (index) {
        case 0x00:
            value = (rtc.control & 128) ? rtc.readSec : rtc.holdSec;
            break;
        case 0x04:
            value = (rtc.control & 128) ? rtc.readMin : rtc.holdMin;
            break;
        case 0x08:
            value = (rtc.control & 128) ? rtc.readHour : rtc.holdHour;
            break;
        case 0x0C: case 0x0D:
            value = (rtc.control & 128) ? rtc.readDay : rtc.holdDay;
            break;
        case 0x10:
            value = rtc.alarmSec;
            break;
        case 0x14:
            value = rtc.alarmMin;
            break;
        case 0x18:
            value = rtc.alarmHour;
            break;
        case 0x20:
            value = rtc.control;
            break;
        case 0x24:
            value = rtc.writeSec;
            break;
        case 0x28:
            value = rtc.writeMin;
            break;
        case 0x2C:
            value = rtc.writeHour;
            break;
        case 0x30: case 0x31:
            value = read8(rtc.writeDay, bit_offset);
            break;
        case 0x34:
            value = rtc.interrupt;
            break;
        case 0x3C: case 0x3D: case 0x3E: case 0x3F:
            value = read8(rtc.revision, bit_offset);
            break;
        case 0x44: case 0x45: case 0x46: case 0x47:
            value = read8(rtc.readSec | (rtc.readMin<<6) | (rtc.readHour<<12) | (rtc.readDay<<17), bit_offset);
            break;
        default:
            break;
    }

    return value;
}

static void rtc_write(const uint16_t pio, const uint8_t byte, bool peek) {
    uint16_t index = pio & 0xFF;
    uint8_t bit_offset = (index & 3) << 3;
    (void)peek;

    switch (index) {
        case 0x10:
            rtc.alarmSec = byte;
            break;
        case 0x14:
            rtc.alarmMin = byte;
            break;
        case 0x18:
            rtc.alarmHour = byte;
            break;
        case 0x20:
            rtc.control = byte;
            if (rtc.control & 1) {
                event_set(SCHED_RTC, 0);
            } else {
                event_clear(SCHED_RTC);
            }
            if (!(rtc.control & 128)) {
                hold_read();
            }
            break;
        case 0x24:
            rtc.writeSec = byte;
            break;
        case 0x28:
            rtc.writeMin = byte;
            break;
        case 0x2C:
            rtc.writeHour = byte;
            break;
        case 0x30: case 0x31:
            write8(rtc.writeDay, bit_offset, byte);
            break;
        case 0x34:
            rtc.interrupt &= ~byte;
            intrpt_set(INT_RTC, rtc.interrupt & rtc.control & 15);
            break;
        default:
            break;
    }
}

void rtc_reset() {
    memset(&rtc, 0, sizeof rtc);
    rtc.revision = 0x00010500;

    sched.items[SCHED_RTC].clock = CLOCK_32K;
    sched.items[SCHED_RTC].second = -1;
    sched.items[SCHED_RTC].proc = rtc_event;

    gui_console_printf("[CEmu] RTC reset.\n");
}

static const eZ80portrange_t device = {
    .read_in    = rtc_read,
    .write_out  = rtc_write
};

eZ80portrange_t init_rtc(void) {
    gui_console_printf("[CEmu] Initialized RTC...\n");
    return device;
}

bool rtc_save(emu_image *s) {
    s->rtc = rtc;
    return true;
}

bool rtc_restore(const emu_image *s) {
    rtc = s->rtc;
    return true;
}
