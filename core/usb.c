#include "usb.h"
#include "emu.h"
#include "schedule.h"
#include "interrupt.h"

/* Global GPT state */
usb_state_t usb;

static uint8_t usb_read(const uint16_t pio)
{
    //uint16_t index = pio & 0xFFF;
    //uint8_t bit_offset = (index&3)<<3;

    return 0xFF;
}

static void usb_write(const uint16_t pio, const uint8_t byte)
{
    //uint16_t index = (int)pio & 0xFFF;
    //uint8_t bit_offset = (index&3)<<3;
}

void usb_reset(void) {

}

static const eZ80portrange_t device = {
    .read_in    = usb_read,
    .write_out  = usb_write
};

eZ80portrange_t init_usb(void) {
    gui_console_printf("Initialized USB...\n");
    return device;
}
