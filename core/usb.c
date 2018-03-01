#include "usb.h"
#include "emu.h"
#include "schedule.h"
#include "interrupt.h"

#include <stdio.h>

/* Global GPT state */
usb_state_t usb;

static uint8_t usb_read(const uint16_t pio, bool peek) {
    (void)pio;
    (void)peek;
    return 0;
}

static void usb_write(const uint16_t pio, const uint8_t byte, bool poke) {
    (void)pio;
    (void)byte;
    (void)poke;
}

void usb_reset(void) {
    gui_console_printf("[CEmu] USB Reset.\n");
}

static const eZ80portrange_t device = {
    .read  = usb_read,
    .write = usb_write
};

eZ80portrange_t init_usb(void) {
    gui_console_printf("[CEmu] Initialized Universal Serial Bus...\n");
    return device;
}

bool usb_save(FILE *image) {
    return fwrite(&usb, sizeof(usb), 1, image) == 1;
}

bool usb_restore(FILE *image) {
    return fread(&usb, sizeof(usb), 1, image) == 1;
}
