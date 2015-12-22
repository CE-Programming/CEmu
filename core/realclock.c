#include <time.h>

#include "core/realclock.h"
#include "core/emu.h"
#include "core/schedule.h"
#include "core/interrupt.h"

/* Global GPT state */
rtc_state_t rtc;

static uint8_t rtc_read(const uint16_t pio)
{
    uint16_t index = pio & 0xFF;
    uint8_t bit_offset = (index&3)<<3;

    time(&rtc.rawtime);
    rtc.timeinfo = localtime(&rtc.rawtime);

    switch (index) {
        case 0x00: case 0x01: case 0x02: case 0x03:
            return read8(rtc.timeinfo->tm_sec, bit_offset);
        case 0x04: case 0x05: case 0x06: case 0x07:
            return read8(rtc.timeinfo->tm_min, bit_offset);
        case 0x08: case 0x09: case 0x0A: case 0x0B:
            return read8(rtc.timeinfo->tm_hour, bit_offset);
        case 0x0C: case 0x0D: case 0x0E: case 0x0F:
            return read8(rtc.timeinfo->tm_yday, bit_offset);
    }

    return 0;
}

static void rtc_write(const uint16_t pio, const uint8_t byte)
{
    //uint16_t index = (int)pio & 0xFF;
    //uint8_t bit_offset = (index&3)<<3;

}

void rtc_reset() {

}

static const eZ80portrange_t device = {
    .read_in    = rtc_read,
    .write_out  = rtc_write
};

eZ80portrange_t init_rtc(void) {
    gui_console_printf("Initialized real time clock...\n");
    return device;
}
