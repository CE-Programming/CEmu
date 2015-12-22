#include "core/realclock.h"
#include "core/emu.h"
#include "core/schedule.h"
#include "core/interrupt.h"

/* Global GPT state */
rtc_state_t rtc;

static uint8_t rtc_read(const uint16_t pio)
{
    //uint16_t index = pio & 0xFF;
    //uint8_t bit_offset = (index&3)<<3;

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
    gui_console_printf("Initialized general purpose timers...\n");
    return device;
}
