#include <string.h>

#include "flash.h"
#include "emu.h"
#include "os/os.h"

/* Global flash state */
flash_state_t flash;

/* Read from the 0x1000 range of ports */
static uint8_t flash_read(const uint16_t pio) {
    uint8_t addr = pio & 0xFF;
    uint8_t read_byte;

    switch (addr) {
        case 0x02:
            read_byte = flash.map;
            break;
        case 0x05:
            read_byte = flash.added_wait_states;
            break;
        default:
            read_byte = flash.ports[addr];
            break;
    }
    return read_byte;
}

/* Write to the 0x1000 range of ports */
static void flash_write(const uint16_t pio, const uint8_t byte)
{
    uint8_t addr = pio & 0xFF;

    switch (addr) {
        case 0x00:
            flash.ports[addr] = byte & 1;
            break;
        case 0x02:
            flash.map = byte;
            break;
        case 0x05:
            flash.added_wait_states = byte;
            break;
        default:
            flash.ports[addr] = byte;
            break;
    }
}

static const eZ80portrange_t device = {
    .read_in    = flash_read,
    .write_out  = flash_write
};


eZ80portrange_t init_flash(void) {
    memset(flash.ports, 0, sizeof flash.ports);

    flash.ports[0x00] = 0x01; /* From WikiTI */
    flash.ports[0x07] = 0xFF; /* From WikiTI */
    flash.map = 0x06;         /* From WikiTI */

    gui_console_printf("Initialized flash device...\n");
    return device;
}
