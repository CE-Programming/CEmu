#include "flash.h"
#include "control.h"
#include "emu.h"
#include "mem.h"
#include "os/os.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Global flash state */
flash_state_t flash;

static void flash_set_map(void) {
    /* Determine how many bytes of flash are mapped */
    if (flash.ports[0x00] == 0 || flash.ports[0x01] > 0x3F) {
        flash.mappedBytes = 0;
    } else {
        uint8_t map = flash.ports[0x02];
        flash.mappedBytes = 0x10000 << (map < 8 ? map : 0);
    }

    /* Set the effective access bounds, overriding for low wait states */
    flash.waitStates = flash.ports[0x05] + 6;
    if (unlikely(flash.waitStates == 6)) {
        flash.mask = 0;
    } else {
        flash.mask = flash.mappedBytes;
    }
}

static void flash_set_mask(void) {
    uint32_t value = flash.maskReg[0]
                   | flash.maskReg[1] << 8
                   | flash.maskReg[2] << 16
                   | flash.maskReg[3] << 24;
    flash.mask = ~value & (SIZE_FLASH - 1);
}

void flash_flush_cache(void) {
    /* Flush only if the cache has been used since the last flush */
    if (flash.lastCacheLine != FLASH_CACHE_INVALID_LINE) {
        flash.lastCacheLine = FLASH_CACHE_INVALID_LINE;
        for (unsigned int set = 0; set < FLASH_CACHE_SETS; set++) {
            flash.cacheTags[set].mru = flash.cacheTags[set].lru = FLASH_CACHE_INVALID_TAG;
        }
    }
}

uint32_t flash_touch_cache(uint32_t addr) {
    uint32_t line = addr >> FLASH_CACHE_LINE_BITS;
    if (likely(line == flash.lastCacheLine)) {
        return 2;
    }
    else {
        flash.lastCacheLine = line;
        flash_cache_set_t* set = &flash.cacheTags[line & (FLASH_CACHE_SETS - 1)];
        uint16_t tag = (uint16_t)(line >> FLASH_CACHE_SET_BITS);
        if (likely(set->mru == tag)) {
            return 3;
        }
        else if (likely(set->lru == tag)) {
            /* Swap to track most-recently-used */
            set->lru = set->mru;
            set->mru = tag;
            return 3;
        }
        else {
            /* Handle a cache miss by replacing least-recently-used */
            set->lru = tag;
            /* Supposedly this takes from 195-201 cycles, but typically seems to be 196-197 */
            return 197;
        }
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
    flash_flush_cache();
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
    if (asic.serFlash) {
        if (pio & 0x800) {
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
                    case 0x2C: case 0x2D: case 0x2E: case 0x2F:
                        value = flash.maskReg[index - 0x2C];
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
            value = flash.ports[index];
        }
    } else {
        uint8_t index = pio;
        value = flash.ports[index];
    }
    return value;
}

/* Write to the 0x1000 range of ports */
static void flash_write(const uint16_t pio, const uint8_t byte, bool poke) {
    (void)poke;
    if (asic.serFlash) {
        if (!flash_unlocked()) {
            return;
        }

        if (pio & 0x800) {
            uint16_t index = pio & 0x7FF;
            if (index < 0x10) {
                flash.command[index] = byte;
                if (index == 0xF) {
                    flash_execute_command();
                }
            }
            else {
                switch (index) {
                    case 0x24:
                        flash.commandStatus[0] &= ~(byte & 1);
                        break;
                    case 0x2C: case 0x2D: case 0x2E: case 0x2F:
                        flash.maskReg[index - 0x2C] = byte;
                        flash_set_mask();
                        break;
                    case 0x100:
                        if ((flash.command[0xC] & 1 << 1) && flash.commandLength) {
                            flash_write_command(byte);
                            flash_command_byte_transferred();
                        }
                        break;
                }
            }
            return;
        } else {
            uint8_t index = pio & 0x7F;
            switch (index) {
                case 0x10:
                    flash.ports[index] = byte & 1;
                    break;
                default:
                    flash.ports[index] = byte;
                    break;
            }
        }
    } else {
        uint8_t index = pio;
        switch (index) {
            case 0x00:
                flash.ports[index] = byte & 1;
                flash_set_map();
                break;
            case 0x01:
                flash.ports[index] = byte;
                flash_set_map();
                break;
            case 0x02:
                flash.ports[index] = byte & 0xF;
                flash_set_map();
                break;
            case 0x05:
                flash.ports[index] = byte;
                flash_set_map();
                break;
            case 0x08:
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
    flash.uniqueID = UINT64_C(0xFFFFFFFFDE680000) | (rand() & 0xFFFF);
    gui_console_printf("[CEmu] Initialized Flash...\n");
    return device;
}

void flash_reset(void) {
    memset((uint8_t*)&flash + sizeof(flash.uniqueID), 0, sizeof(flash) - sizeof(flash.uniqueID));
    flash.commandStatus[1] = 0x28;
    flash.commandStatus[2] = 0x03;
    flash.commandStatus[3] = 0x60;
    if (asic.serFlash) {
        flash.maskReg[0] = 0x00;
        flash.maskReg[1] = 0x00;
        flash.maskReg[2] = 0xC0;
        flash.maskReg[3] = 0xFF;
        flash_set_mask();
        flash_flush_cache();
    } else {
        flash.ports[0x00] = 0x01;
        flash.ports[0x02] = 0x06;
        flash.ports[0x05] = 0x04;
        flash.ports[0x07] = 0xFF;
        flash_set_map();
    }
    gui_console_printf("[CEmu] Flash reset.\n");
}

bool flash_save(FILE *image) {
    return fwrite(&flash, sizeof(flash), 1, image) == 1;
}

bool flash_restore(FILE *image) {
    return fread(&flash, sizeof(flash), 1, image) == 1;
}
