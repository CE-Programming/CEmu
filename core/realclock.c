#include <time.h>
#include <string.h>

#include "realclock.h"
#include "emu.h"
#include "schedule.h"
#include "interrupt.h"

/* Global GPT state */
rtc_state_t rtc;

static void rtc_event(int index) {
    time_t currsec;
    /* Update 3 or so times a second just so we don't miss a step */
    event_repeat(index, 27000000 / 3);

    currsec = time(NULL);

    /* TODO -- these events need to trigger interrupts */
    if (currsec > rtc.prevsec) {
        if (rtc.control & 1) { rtc.interrupt |= 1; }
        rtc.read_sec++;
        rtc.prevsec = currsec;
        if (rtc.read_sec > 59) {
            rtc.read_sec = 0;
            if (rtc.control & 2) {
                rtc.interrupt |= 2;
                intrpt_set(INT_RTC, true);
            }
            rtc.read_min++;
            if (rtc.read_min > 59) {
                rtc.read_min = 0;
                if (rtc.control & 4) {
                    rtc.interrupt |= 4;
                    intrpt_set(INT_RTC, true);
                }
                rtc.read_hour++;
                if (rtc.read_hour > 23) {
                    rtc.read_hour = 0;
                    if (rtc.control & 8) {
                        rtc.interrupt |= 8;
                        intrpt_set(INT_RTC, true);
                    }
                    rtc.read_day++;
                }
            }
        }
    }

    if ((rtc.control & 16) && (rtc.read_sec == rtc.alarm_sec) &&
        (rtc.read_min == rtc.alarm_min) && (rtc.read_hour == rtc.alarm_hour)) {
            rtc.interrupt |= 16;
            intrpt_set(INT_RTC, true);
    }
}

static void hold_read(void) {
    rtc.hold_sec = rtc.read_sec;
    rtc.hold_min = rtc.read_min;
    rtc.hold_hour = rtc.read_hour;
    rtc.hold_day = rtc.read_day;
}

static uint8_t rtc_read(const uint16_t pio) {
    uint16_t index = pio & 0xFF;
    uint8_t bit_offset = (index & 3) << 3;

    uint8_t value = 0;

    switch (index) {
        case 0x00:
            value = (rtc.control & 128) ? rtc.read_sec : rtc.hold_sec;
            break;
        case 0x04:
            value = (rtc.control & 128) ? rtc.read_min : rtc.hold_min;
            break;
        case 0x08:
            value = (rtc.control & 128) ? rtc.read_hour : rtc.hold_hour;
            break;
        case 0x0C: case 0x0D:
            value = (rtc.control & 128) ? rtc.read_day : rtc.hold_day;
            break;
        case 0x10:
            value = rtc.alarm_sec;
            break;
        case 0x14:
            value = rtc.alarm_min;
            break;
        case 0x18:
            value = rtc.alarm_hour;
            break;
        case 0x20:
            value = rtc.control;
            break;
        case 0x24:
            value = rtc.write_sec;
            break;
        case 0x28:
            value = rtc.write_min;
            break;
        case 0x2C:
            value = rtc.write_hour;
            break;
        case 0x30: case 0x31:
            value = read8(rtc.write_day, bit_offset);
            break;
        case 0x34:
            value = rtc.interrupt;
            break;
        case 0x3C: case 0x3D: case 0x3E: case 0x3F:
            value = read8(rtc.revision, bit_offset);
            break;
        case 0x44: case 0x45: case 0x46: case 0x47:
            value = read8(rtc.read_sec | (rtc.read_min<<6) | (rtc.read_hour<<12) | (rtc.read_day<<17), bit_offset);
            break;
        default:
            break;
    }

    return value;
}

static void rtc_write(const uint16_t pio, const uint8_t byte) {
    uint16_t index = pio & 0xFF;
    uint8_t bit_offset = (index & 3) << 3;

    switch (index) {
        case 0x10:
            rtc.alarm_sec = byte;
            break;
        case 0x14:
            rtc.alarm_min = byte;
            break;
        case 0x18:
            rtc.alarm_hour = byte;
            break;
        case 0x20:
            rtc.control = byte;
            if (rtc.control & 1) {
                event_set(SCHED_RTC, 0);
            } else {
                event_clear(SCHED_RTC);
            }
            if (rtc.control & 64) { /* (Bit 6) -- Load time */
                rtc.read_sec = rtc.write_sec;
                rtc.read_min = rtc.write_min;
                rtc.read_hour = rtc.write_hour;
                rtc.read_day = rtc.write_day;
                rtc.control &= ~64;
                rtc.interrupt |= 32;  /* Load operation complete */
                intrpt_set(INT_RTC, (rtc.interrupt & rtc.control & 15) ? true : false);
            }
            if (!(rtc.control & 128)) {
                hold_read();
            }
            break;
        case 0x24:
            rtc.write_sec = byte;
            break;
        case 0x28:
            rtc.write_min = byte;
            break;
        case 0x2C:
            rtc.write_hour = byte;
            break;
        case 0x30: case 0x31:
            write8(rtc.write_day, bit_offset, byte);
            break;
        case 0x34:
            rtc.interrupt &= ~byte;
            intrpt_set(INT_RTC, (rtc.interrupt & rtc.control & 15) ? true : false);
            break;
        default:
            break;
    }
}

void rtc_reset() {
    memset(&rtc, 0, sizeof rtc);
    rtc.revision = 0x00010500;
    rtc.prevsec = time(NULL);

    sched.items[SCHED_RTC].clock = CLOCK_12M;
    sched.items[SCHED_RTC].second = -1;
    sched.items[SCHED_RTC].proc = rtc_event;

    gui_console_printf("RTC Reset.\n");
}

static const eZ80portrange_t device = {
    .read_in    = rtc_read,
    .write_out  = rtc_write
};

eZ80portrange_t init_rtc(void) {
    gui_console_printf("Initialized real time clock...\n");
    return device;
}
