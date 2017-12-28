#include "flash.h"
#include "emu.h"
#include "os/os.h"

#include <string.h>
#include <stdio.h>

/* Global flash state */
flash_state_t flash;

static void flash_set_map(uint8_t map) {
    flash.map = map & 0x0F;
    if (map & 8) {
        flash.mask = 0xFFFF;
    } else {
        flash.mask = ((0x10000 << (map & 7)) - 1) & 0x3FFFFF;
    }
}

/* Read from the 0x1000 range of ports */
static uint8_t flash_read(const uint16_t pio, bool peek) {
    uint8_t index = (uint8_t)pio;
    uint8_t value;
    (void)peek;

    switch (index) {
        case 0x00:
            value = flash.mapped;
            break;
        case 0x02:
            value = flash.map;
            break;
        case 0x05:
            value = flash.waitStates - 6;
            break;
        default:
            value = flash.ports[index];
            break;
    }
    return value;
}

/* Write to the 0x1000 range of ports */
static void flash_write(const uint16_t pio, const uint8_t byte, bool poke) {
    uint8_t index = (uint8_t)pio;
    (void)poke;

    switch (index) {
        case 0x00:
            flash.mapped = byte;
            break;
        case 0x01:
            flash.ports[index] = byte;
            if (byte > 0x3F) {
                flash.mapped = 0;
            }
            break;
        case 0x02:
            flash_set_map(byte);
            break;
        case 0x05:
            flash.waitStates = byte + 6;
            break;
        case 0x08:
            flash.ports[index] = byte & 1;
            break;
        case 0x10:
            flash.ports[index] = byte & 1;
            break;
        default:
            flash.ports[index] = byte;
            break;
    }
}

static const eZ80portrange_t device = {
    .read  = flash_read,
    .write = flash_write
};

eZ80portrange_t init_flash(void) {
    memset(flash.ports, 0, sizeof flash.ports);

    flash.ports[0x00] = 0x01;
    flash.ports[0x07] = 0xFF;
    flash.waitStates = 10;
    flash.mapped = 1;
    flash_set_map(6);

    gui_console_printf("[CEmu] Initialized Flash Chip...\n");
    return device;
}

bool flash_save(FILE *image) {
    return fwrite(&flash, sizeof(flash), 1, image) == 1;
}

bool flash_restore(FILE *image) {
    return fread(&flash, sizeof(flash), 1, image) == 1;
}
