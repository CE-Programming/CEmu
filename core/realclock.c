#include <time.h>
#include <string.h>

#include "core/realclock.h"
#include "core/emu.h"
#include "core/schedule.h"
#include "core/interrupt.h"

/* Global GPT state */
rtc_state_t rtc;

static void rtc_event(int index) {
    /* Update 3 or so times a second just so we don't miss a step */
    event_repeat(index, 27000000 / 4);

    time_t currsec = time(NULL);
    if (!(rtc.control & 1)) {
        return;
    }

    if (currsec > rtc.prevsec) {
        if (rtc.control & 1) { rtc.interrupt |= 1; }
        rtc.read_sec++;
        rtc.prevsec = currsec;
        if (rtc.read_sec > 59) {
            rtc.read_sec = 0;
            if (rtc.control & 2) { rtc.interrupt |= 2; }
            rtc.read_min++;
            if (rtc.read_min > 59) {
                rtc.read_min = 0;
                if (rtc.control & 4) { rtc.interrupt |= 4; }
                rtc.read_hour++;
                if (rtc.read_hour > 23) {
                    rtc.read_hour = 0;
                    if (rtc.control & 8) { rtc.interrupt |= 8; }
                    rtc.read_day++;
                }
            }
        }
    }

    if ((rtc.control & 16) && (rtc.read_sec == rtc.alarm_sec)) {
            rtc.interrupt |= 16;
    }
}

static void hold_read(void) {
    rtc.hold_sec = rtc.read_sec;
    rtc.hold_min = rtc.read_min;
    rtc.hold_hour = rtc.read_hour;
    rtc.hold_day = rtc.read_day;
}

static uint8_t rtc_read(const uint16_t pio)
{
    static const uint32_t revision = 0x00010500;
    uint16_t index = pio & 0xFF;
    uint8_t bit_offset = (index&3)<<3;

    switch (index) {
        case 0x00:
            if(!(rtc.control & 128)) { return rtc.hold_sec; }
            return rtc.read_sec;
        case 0x04:
            if(!(rtc.control & 128)) { return rtc.hold_min; }
            return rtc.read_min;
        case 0x08:
            if(!(rtc.control & 128)) { return rtc.hold_hour; }
            return rtc.read_hour;
        case 0x0C: case 0x0D:
            if(!(rtc.control & 128)) { return rtc.hold_day; }
            return read8(rtc.read_day, bit_offset);
        case 0x10:
            return rtc.alarm_sec;
        case 0x14:
            return rtc.alarm_min;
        case 0x18:
            return rtc.alarm_hour;
        case 0x20:
            return rtc.control;
        case 0x24:
            return rtc.write_sec;
        case 0x28:
            return rtc.write_min;
        case 0x2C:
            return rtc.write_hour;
        case 0x30: case 0x31:
            return read8(rtc.write_day, bit_offset);
        case 0x34:
            return rtc.interrupt;
        case 0x3C: case 0x3D: case 0x3E: case 0x3F:
            return read8(revision, bit_offset);
        case 0x44: case 0x45: case 0x46: case 0x47:
            return read8(rtc.read_sec | (rtc.read_min<<6) | (rtc.read_hour<<12) | (rtc.read_day<<17), bit_offset);
    }

    return 0;
}

static void rtc_write(const uint16_t pio, const uint8_t byte)
{
    uint16_t index = (int)pio & 0xFF;
    uint8_t bit_offset = (index&3)<<3;

    switch (index) {
        case 0x10:
            rtc.alarm_sec = byte; return;
        case 0x14:
            rtc.alarm_min = byte; return;
        case 0x18:
            rtc.alarm_hour = byte; return;
        case 0x20:
            rtc.control = byte;
            if (rtc.control & 64) { /* (Bit 6) -- Load time */
                rtc.read_sec = rtc.write_sec;
                rtc.read_min = rtc.write_min;
                rtc.read_hour = rtc.write_hour;
                rtc.read_day = rtc.write_day;
                rtc.control &= ~(64);
                rtc.interrupt |= 32;  /* Load operation complete */
            }
            if (!(rtc.control & 128)) {
                hold_read();
            }
        case 0x24:
            rtc.write_sec = byte; return;
        case 0x28:
            rtc.write_min = byte; return;
        case 0x2C:
            rtc.write_hour = byte; return;
        case 0x30: case 0x31:
            write8(rtc.write_day,bit_offset,byte); return;
        case 0x34:
            rtc.interrupt &= ~(byte); return;
    }
}

void rtc_reset() {
    memset(&rtc,0,sizeof(rtc));
    rtc.prevsec = time(NULL);

    sched.items[SCHED_RTC].clock = CLOCK_12M;
    sched.items[SCHED_RTC].proc = rtc_event;
}

static const eZ80portrange_t device = {
    .read_in    = rtc_read,
    .write_out  = rtc_write
};

eZ80portrange_t init_rtc(void) {
    gui_console_printf("Initialized real time clock...\n");
    return device;
}
