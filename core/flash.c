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

static void flash_set_wait_states(uint8_t value) {
    if (asic.revM) {
        flash.waitStates = 2;
    } else {
        flash.waitStates = value;
    }
}

static void flash_execute_command(void) {
    flash.commandAddress = flash.command[0] | flash.command[1] << 8 | flash.command[2] << 16;
    flash.commandLength = flash.command[8] | flash.command[9] << 8 | flash.command[10] << 16;
    switch (flash.command[0xF]) {
        case 0x05: // Read Status Register-1
        case 0x35: // Read Status Register-2
        case 0x15: // Read Status Register-3
            flash.commandStatus[0] |= 1 << 1;
            break;
    }
}
static uint8_t flash_read_command(void) {
    switch (flash.command[0xF]) {
        case 0x05: // Read Status Register-1
            return flash.commandStatus[1];
        case 0x35: // Read Status Register-2
            return flash.commandStatus[2];
        case 0x15: // Read Status Register-3
            return flash.commandStatus[3];
        default:
            return 0;
    }
}
static void flash_write_command(uint8_t byte) {
    switch (flash.command[0xF]) {
    }
}
static void flash_command_byte_transferred(void) {
    if (!--flash.commandLength) {
        flash.commandStatus[0] &= ~(1 << 1);
        flash.commandStatus[0] |= 1 << 0;
    }
}

/* Read from the 0x1000 range of ports */
static uint8_t flash_read(const uint16_t pio, bool peek) {
    uint8_t value;
    (void)peek;

    if (asic.revM && pio & 0x800) {
        uint16_t index = pio & 0x7FF;
        if (index < 0x10) {
            value = flash.command[index];
        } else {
            switch (index) {
                case 0x18:
                    value = flash.commandStatus[0] & 1 << 1;
                    break;
                case 0x24:
                    value = flash.commandStatus[0] & 1 << 0;
                    break;
                case 0x30: case 0x31: case 0x32: case 0x33:
                    value = 0;
                    break;
                case 0x100:
                    if (!(flash.command[0xC] & 1 << 1) && flash.commandLength) {
                        value = flash_read_command();
                        flash_command_byte_transferred();
                    } else {
                        value = 0;
                    }
                    break;
                default:
                    value = 0;
                    break;
            }
        }
    } else {
        uint8_t index = pio;
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
    }
    return value;
}

/* Write to the 0x1000 range of ports */
static void flash_write(const uint16_t pio, const uint8_t byte, bool poke) {
    (void)poke;

    if (asic.revM && pio & 0x800) {
        uint16_t index = pio & 0x7FF;
        if (index < 0x10) {
            flash.command[index] = byte;
            if (index == 0xF) {
                flash_execute_command();
            }
        } else {
            switch (index) {
                case 0x24:
                    flash.commandStatus[0] &= ~(byte & 1);
                    break;
                case 0x100:
                    if ((flash.command[0xC] & 1 << 1) && flash.commandLength) {
                        flash_write_command(byte);
                        flash_command_byte_transferred();
                    }
                    break;
            }
        }
    } else {
        uint8_t index = pio;
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
                flash_set_wait_states(byte + 6);
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
}

static const eZ80portrange_t device = {
    .read  = flash_read,
    .write = flash_write
};

eZ80portrange_t init_flash(void) {
    memset(flash.ports, 0, sizeof flash.ports);

    flash.ports[0x00] = 0x01;
    flash.ports[0x07] = 0xFF;
    flash.commandStatus[1] = 0x28;
    flash.commandStatus[2] = 0x03;
    flash.commandStatus[3] = 0x60;
    flash_set_wait_states(10);
    flash.mapped = 1;
    flash_set_map(6);

    gui_console_printf("[CEmu] Initialized Flash...\n");
    return device;
}

bool flash_save(FILE *image) {
    return fwrite(&flash, sizeof(flash), 1, image) == 1;
}

bool flash_restore(FILE *image) {
    return fread(&flash, sizeof(flash), 1, image) == 1;
}
