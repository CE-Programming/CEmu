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

    static const uint32_t revision = 0x00010801;

    uint8_t curr = ((index>>4)&0x3);

    if (index > 0x2F) {
        switch (index) {
            case 0x30: case 0x31: case 0x32: case 0x33:
                return read8(gpt.control, bit_offset);
            case 0x34: case 0x35: case 0x36: case 0x37:
                return read8(gpt.interrupt_state, bit_offset);
            case 0x38: case 0x39: case 0x3A: case 0x3B:
                return read8(gpt.interrupt_mask, bit_offset);
        }
    } else {
        switch (index & 0xF) {
            case 0x00: case 0x01: case 0x02: case 0x03:
                return read8(gpt.timer[curr].counter, bit_offset);
            case 0x04: case 0x05: case 0x06: case 0x07:
                return read8(gpt.timer[curr].load, bit_offset);
            case 0x08: case 0x09: case 0x0A: case 0x0B:
                return read8(gpt.timer[curr].match1, bit_offset);
            case 0x0C: case 0x0D: case 0x0E: case 0x0F:
                return read8(gpt.timer[curr].match2, bit_offset);
        }
    }

    return 0;
}

static void gpt_write(const uint16_t pio, const uint8_t byte)
{
    uint16_t index = (int)pio & 0xFF;
    uint8_t bit_offset = (index&3)<<3;

    uint8_t curr = ((index>>4)&0x3);

    if (index > 0x2F) {
        switch (index) {
            case 0x30: case 0x31: case 0x32: case 0x33:
                write8(gpt.control, bit_offset, byte);
                return;
            case 0x34: case 0x35: case 0x36: case 0x37:
                write8(gpt.interrupt_state, bit_offset, byte);
                return;
            case 0x38: case 0x39: case 0x3A: case 0x3B:
                write8(gpt.interrupt_mask, bit_offset, byte);
                return;
        }
    } else {
        switch (index & 0xF) {
            case 0x00: case 0x01: case 0x02: case 0x03:
                write8(gpt.timer[curr].counter, bit_offset, byte);
                return;
            case 0x04: case 0x05: case 0x06: case 0x07:
                write8(gpt.timer[curr].load, bit_offset, byte);
                return;
            case 0x08: case 0x09: case 0x0A: case 0x0B:
                write8(gpt.timer[curr].match1, bit_offset, byte);
                return;
            case 0x0C: case 0x0D: case 0x0E: case 0x0F:
                write8(gpt.timer[curr].match2, bit_offset, byte);
                return;
        }
    }
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
