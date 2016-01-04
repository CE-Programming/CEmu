#include "usb.h"
#include "emu.h"
#include "schedule.h"
#include "interrupt.h"

/* Global GPT state */
usb_state_t usb;

static uint8_t usb_read(const uint16_t pio) {
    (void)pio;
    return 0xFF;
}

static void usb_write(const uint16_t pio, const uint8_t byte) {
    (void)pio;
    (void)byte;
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
