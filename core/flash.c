#include "flash.h"
#include "emu.h"
#include "mem.h"
#include "os/os.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

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

static void flash_finish_command(void) {
    flash.commandStatus[0] |= 1 << 0;
    switch (flash.command[0xF]) {
        // Reset instructions
        case 0x04: // Write Disable
        case 0x01: // Write Status Register-1
        case 0x31: // Write Status Register-2
        case 0x11: // Write Status Register-3
        case 0x02: // Page Program
        case 0x32: // Quad Input Page Program
        case 0x20: // Sector Erase
        case 0x52: // 32KB Block Erase
        case 0xD8: // 64KB Block Erase
        case 0xC7: case 0x60: // Chip Erase
        case 0x44: // Erase Security Registers
        case 0x42: // Program Security Registers
        case 0x99: // Reset
            flash.commandStatus[1] &= ~(1 << 1);
            break;
    }
}

static void flash_erase(uint32_t size) {
    assert(!(size & (size - 1)));
    if (flash.commandStatus[1] & 1 << 1) {
        memset(&mem.flash.block[flash.commandAddress & (SIZE_FLASH - 1) & -size], 0xFF, size);
    }
    flash_finish_command();
}

static void flash_execute_command(void) {
    flash.commandAddress = flash.command[0] | flash.command[1] << 8 | flash.command[2] << 16;
    flash.commandLength = flash.command[8] | flash.command[9] << 8 | flash.command[10] << 16;
    flash.commandStatus[0] &= ~7;
    switch (flash.command[0xF]) {
        case 0x06: // Write Enable
            flash.commandStatus[1] |= 1 << 1;
            // fallthrough
        case 0x04: // Write Disable
            flash_finish_command();
            break;
        case 0x94: // Read Manufacturer / Device ID Quad I/O
        case 0x4B: // Read Unique ID Number
        case 0x9F: // Read JEDEC ID
            flash.commandAddress = 0;
            // fallthrough
        case 0x05: // Read Status Register-1
        case 0x35: // Read Status Register-2
        case 0x15: // Read Status Register-3
            flash.commandStatus[0] |= 1 << 2;
            break;
        case 0x32: // Quad Input Page Program
            flash.commandStatus[0] |= 1 << 1;
            break;
        case 0x20: // Sector Erase
            flash_erase(4 << 10);
            break;
        case 0x52: // 32KB Block Erase
            flash_erase(32 << 10);
            break;
        case 0xD8: // 64KB Block Erase
            flash_erase(64 << 10);
            break;
        case 0xC7: case 0x60: // Chip Erase
            flash_erase(SIZE_FLASH);
            break;
    }
    if (!flash.commandLength && flash.commandStatus[0] & 3 << 1) {
        flash_finish_command();
    }
}
static uint8_t flash_read_command(bool peek) {
    switch (flash.command[0xF]) {
        case 0x05: // Read Status Register-1
            return flash.commandStatus[1];
        case 0x35: // Read Status Register-2
            return flash.commandStatus[2];
        case 0x15: // Read Status Register-3
            return flash.commandStatus[3];
        case 0xAB: // Release Power-down / Device ID
            flash.commandAddress = 1;
            // fallthrough
        case 0x90: // Read Manufacturer / Device ID
        case 0x92: // Read Manufacturer / Device ID Dual I/O
        case 0x94: // Read Manufacturer / Device ID Quad I/O
            return 0xEF15 >> (~flash.commandAddress++ & 1) * 8;
        case 0x4B: // Read Unique ID Number
            return flash.uniqueID >> (~flash.commandAddress++ & 7) * 8;
        case 0x9F: // Read JEDEC ID
            if (flash.commandAddress >= 3) {
                flash.commandAddress = 0;
            }
            return 0xEF4016 >> (2 - flash.commandAddress++) * 8;
    }
    return 0;
}
static void flash_write_command(uint8_t byte) {
    switch (flash.command[0xF]) {
        case 0x32: // Quad Input Page Program
            mem.flash.block[flash.commandAddress & (SIZE_FLASH - 1)] &= byte;
            flash.commandAddress = (flash.commandAddress & ~0xFF) | ((flash.commandAddress + 1) & 0xFF);
            break;
    }
}
static void flash_command_byte_transferred(void) {
    if (!--flash.commandLength) {
        flash.commandStatus[0] &= ~(3 << 1);
        flash_finish_command();
    }
}

/* Read from the 0x1000 range of ports */
static uint8_t flash_read(const uint16_t pio, bool peek) {
    uint8_t value;
    if (asic.revM && pio & 0x800) {
        uint16_t index = pio & 0x7FF;
        if (index < 0x10) {
            value = flash.command[index];
        } else {
            switch (index) {
                case 0x18:
                    value = flash.commandStatus[0] >> 1 & 3;
                    break;
                case 0x24:
                    value = flash.commandStatus[0] >> 0 & 1;
                    break;
                case 0x30:
                    value = 0x04;
                    break;
                case 0x31:
                    value = 0xB4;
                    break;
                case 0x32:
                    value = 0x0E;
                    break;
                case 0x33:
                    value = 0x1F;
                    break;
                case 0x100:
                    if (!(flash.command[0xC] & 1 << 1) && flash.commandLength) {
                        value = flash_read_command(peek);
                        if (!peek) {
                            flash_command_byte_transferred();
                        }
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
        uint8_t index = pio & 0x7F;
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
    flash.uniqueID = UINT64_C(0xFFFFFFFFDE680000) | (rand() & 0xFFFF);
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
