#include "core/timers.h"
#include "core/emu.h"
#include "core/schedule.h"
#include "core/interrupt.h"
#include <stdio.h>
#include <stdlib.h>

/* Global GPT state */
general_timers_state_t gpt;

static uint8_t gpt_read(const uint16_t pio)
{
    uint16_t index = pio & 0xFF;
    uint8_t bit_offset = (index&3)<<3;

    gui_console_printf("Attempted read of timers: %04X\n", pio);

    return 0;
}

static void gpt_write(const uint16_t pio, const uint8_t byte)
{
    uint16_t index = (int)pio & 0xFF;
    uint8_t bit_offset = (index&3)<<3;

    uint8_t timer = (index & 0xF) & 3;

    gui_console_printf("Attempted write of timers: %04X\n", pio);
}

void gpt_reset() {

}

static const eZ80portrange_t device = {
    .read_in    = gpt_read,
    .write_out  = gpt_write
};

eZ80portrange_t init_gpt(void) {
    gui_console_printf("Initialized general purpose timers...\n");
    return device;
}
