#include <string.h>

#include "mem.h"
#include "emu.h"
#include "cpu.h"
#include "flash.h"
#include "control.h"

/* Global MEMORY state */
mem_state_t mem;

/* Standard equates */
static const uint32_t ram_size = 0x65800;
static const uint32_t flash_size = 0x400000;
static const uint32_t flash_sector_size_8K = 0x2000;
static const uint32_t flash_sector_size_64K = 0x10000;

void mem_init(void) {
    unsigned int i;

    mem.flash.block = (uint8_t*)malloc(flash_size);               /* allocate Flash memory */
    memset(mem.flash.block, 0xFF, flash_size);
    mem.flash.size = flash_size;

    for (i = 0; i < 8; i++) {
        mem.flash.sector[i].ptr = mem.flash.block + (i*flash_sector_size_8K);
        mem.flash.sector[i].locked = true;
    }

    for (i = 8; i < 8+63; i++) {
        mem.flash.sector[i].ptr = mem.flash.block + (i*flash_sector_size_64K);
        mem.flash.sector[i].locked = false;
    }

    /* Sector 9 is locked */
    mem.flash.sector[8].locked = true;
    mem.flash.locked = true;

    mem.ram.block = (uint8_t*)calloc(ram_size, sizeof(uint8_t));      /* Allocate RAM */

    mem.flash.write_index = 0;
    mem.flash.command = NO_COMMAND;
    gui_console_printf("Initialized memory...\n");
}

void mem_free(void) {
    if (mem.ram.block) {
        free(mem.ram.block);
    }
    if (mem.flash.block) {
        free(mem.flash.block);
    }
    gui_console_printf("Freed memory...\n");
}

void mem_reset(void) {
    memset(mem.ram.block, 0, ram_size);
    gui_console_printf("RAM reset.\n");
}

static uint32_t flash_address(uint32_t address, uint32_t *size) {
    uint32_t mask = flash.mask;
    if (size) {
        *size = mask + 1;
    }
    if (address > mask || !flash.mapped)  {
        address &= mask;
        if (!size) {
            cpu.cycles += 258;
        }
    } else if (!size) {
        cpu.cycles += 6 + flash.addedWaitStates;
    }
    return address;
}

uint8_t *phys_mem_ptr(uint32_t address, uint32_t size) {
    uint8_t **block;
    uint32_t block_size, end_addr;
    if (address < 0xD00000) {
        address = flash_address(address, &block_size);
        block = &mem.flash.block;
    } else {
        address -= 0xD00000;
        block = &mem.ram.block;
        block_size = ram_size;
    }
    end_addr = address + size;
    if (address <= end_addr && address <= block_size && end_addr <= block_size && *block) {
        return *block + address;
    }
    return NULL;
}

static void flash_reset_write_index(uint32_t addr, uint8_t byte) {
    (void)addr;
    (void)byte;
    mem.flash.write_index = 0;
}

static void flash_write(uint32_t address, uint8_t byte) {
    mem.flash.block[address] &= byte;
}

static void flash_erase(uint32_t address, uint8_t byte) {
    (void)address;
    (void)byte;

    mem.flash.command = FLASH_CHIP_ERASE;

    memset(mem.flash.block, 0xFF, flash_size);
    gui_console_printf("Erased entire Flash chip.\n");
}

static void flash_erase_sector(uint32_t address, uint8_t byte) {
    uint8_t sector;

    (void)byte;

    mem.flash.command = FLASH_SECTOR_ERASE;

    /* Reset sector */
    sector = address/flash_sector_size_64K;
    if(mem.flash.sector[sector].locked == false) {
        memset(mem.flash.sector[sector].ptr, 0xFF, flash_sector_size_64K);
    }
}

static void flash_verify_sector_protection(uint32_t address, uint8_t byte) {
    (void)address;
    (void)byte;

    mem.flash.command = FLASH_READ_SECTOR_PROTECTION;
}

typedef const struct flash_write_pattern {
    const int length;
    const flash_write_t pattern[6];
    void (*const handler)(uint32_t address, uint8_t value);
} flash_write_pattern_t;

typedef struct flash_status_pattern {
    uint8_t length;
    uint8_t pattern[10];
} flash_status_pattern_t;

static flash_write_pattern_t patterns[] = {
    {
        .length = 4,
        .pattern = {
            { .address = 0xAAA, .address_mask = 0xFFF, .value = 0xAA, .value_mask = 0xFF },
            { .address = 0x555, .address_mask = 0xFFF, .value = 0x55, .value_mask = 0xFF },
            { .address = 0xAAA, .address_mask = 0xFFF, .value = 0xA0, .value_mask = 0xFF },
            { .address = 0x000, .address_mask = 0x000, .value = 0x00, .value_mask = 0x00 },
        },
        .handler = flash_write
    },
    {
        .length = 6,
        .pattern = {
            { .address = 0xAAA, .address_mask = 0xFFF, .value = 0xAA, .value_mask = 0xFF },
            { .address = 0x555, .address_mask = 0xFFF, .value = 0x55, .value_mask = 0xFF },
            { .address = 0xAAA, .address_mask = 0xFFF, .value = 0x80, .value_mask = 0xFF },
            { .address = 0xAAA, .address_mask = 0xFFF, .value = 0xAA, .value_mask = 0xFF },
            { .address = 0x555, .address_mask = 0xFFF, .value = 0x55, .value_mask = 0xFF },
            { .address = 0x000, .address_mask = 0x000, .value = 0x30, .value_mask = 0xFF },
        },
        .handler = flash_erase_sector
    },
    {
        .length = 6,
        .pattern = {
            { .address = 0xAAA, .address_mask = 0xFFF, .value = 0xAA, .value_mask = 0xFF },
            { .address = 0x555, .address_mask = 0xFFF, .value = 0x55, .value_mask = 0xFF },
            { .address = 0xAAA, .address_mask = 0xFFF, .value = 0x80, .value_mask = 0xFF },
            { .address = 0xAAA, .address_mask = 0xFFF, .value = 0xAA, .value_mask = 0xFF },
            { .address = 0x555, .address_mask = 0xFFF, .value = 0x55, .value_mask = 0xFF },
            { .address = 0xAAA, .address_mask = 0xFFF, .value = 0x10, .value_mask = 0xFF },
        },
        .handler = flash_erase
    },
    {
        .length = 3,
        .pattern = {
            { .address = 0xAAA, .address_mask = 0xFFF, .value = 0xAA, .value_mask = 0xFF },
            { .address = 0x555, .address_mask = 0xFFF, .value = 0x55, .value_mask = 0xFF },
            { .address = 0xAAA, .address_mask = 0xFFF, .value = 0x90, .value_mask = 0xFF },
        },
        .handler = flash_verify_sector_protection
    },
    {
        .length = 0
    }

    /* TODO: More flash patterns */

};

static uint8_t flash_read_handler(uint32_t address) {
    uint8_t value = 0;
    uint8_t sector;

    address = flash_address(address, NULL);
    if (flash.mapped) {
        switch(mem.flash.command) {
            case NO_COMMAND:
                value = mem.flash.block[address];
                break;
            case FLASH_SECTOR_ERASE:
                value = 0x80;
                mem.flash.read_index++;
                if(mem.flash.read_index == 3) {
                    mem.flash.read_index = 0;
                    mem.flash.command = NO_COMMAND;
                }
                break;
            case FLASH_CHIP_ERASE:
                value = 0xFF;
                mem.flash.command = NO_COMMAND;
                break;
            case FLASH_READ_SECTOR_PROTECTION:
                if (address < 0x10000) {
                    sector = address/flash_sector_size_8K;
                } else {
                    sector = (address/flash_sector_size_64K)+7;
                }
                value = (uint8_t)mem.flash.sector[sector].locked;
                break;
        }
    }

    /* Returning 0x00 is enough to emulate for the OS. Set the msb bit for user routines? */
    return value;
}

static void flash_write_handler(uint32_t address, uint8_t byte) {
    int i;
    int partial_match = 0;
    flash_write_t *w;
    flash_write_pattern_t *pattern;

    address = flash_address(address, NULL);
    if (!flash.mapped) {
        return;
    }

    /* See if we can reset to default */
    if (mem.flash.command != NO_COMMAND) {
        if (byte == 0xF0) {
            mem.flash.command = NO_COMMAND;
            flash_reset_write_index(address, byte);
            return;
        }
    }

    w = &mem.flash.writes[mem.flash.write_index++];
    w->address = address;
    w->value = byte;

    for (pattern = patterns; pattern->length; pattern++) {
        for (i = 0; (i < mem.flash.write_index) && (i < pattern->length) &&
            (mem.flash.writes[i].address & pattern->pattern[i].address_mask) == pattern->pattern[i].address &&
            (mem.flash.writes[i].value & pattern->pattern[i].value_mask) == pattern->pattern[i].value; i++) {
        }
        if (i == pattern->length) {
            pattern->handler(address, byte);
            partial_match = 0;
            break;
        } else if (i == mem.flash.write_index) {
            partial_match = 1;
        }
    }
    if (!partial_match) {
        flash_reset_write_index(address, byte);
    }
}

uint8_t mem_read_byte(uint32_t address) {
    uint8_t value = 0;
    uint32_t ramAddress;

    address &= 0xFFFFFF;
#ifdef DEBUG_SUPPORT
    if (debugger.data.block[address] & DBG_READ_BREAKPOINT) {
        open_debugger(HIT_READ_BREAKPOINT, address);
    }
#endif
    switch((address >> 20) & 0xF) {
        /* FLASH */
        case 0x0: case 0x1: case 0x2: case 0x3:
        case 0x4: case 0x5: case 0x6: case 0x7:
            value = flash_read_handler(address);
            break;

        /* UNMAPPED */
        case 0x8: case 0x9: case 0xA: case 0xB: case 0xC:
            cpu.cycles += 258;
            break;

        /* RAM */
        case 0xD:
            cpu.cycles += 4;
            ramAddress = address & 0x7FFFF;
            if (ramAddress < 0x65800) {
                value = mem.ram.block[ramAddress];
            }
            break;

        /* MMIO <-> Advanced Perphrial Bus */
        case 0xE: case 0xF:
            cpu.cycles += 2;
            value = port_read_byte(mmio_range(address)<<12 | addr_range(address));
            break;
    }
    return value;
}

void mem_write_byte(uint32_t address, uint8_t byte) {
    uint32_t ramAddress;
    address &= 0xFFFFFF;

    switch((address >> 20) & 0xF) {
        /* FLASH */
        case 0x0: case 0x1: case 0x2: case 0x3:
        case 0x4: case 0x5: case 0x6: case 0x7:
            if (mem.flash.locked && cpu.registers.PC >= control.privileged) {
                cpu_nmi();
            } else {
                flash_write_handler(address, byte);
            }
            break;

        /* UNMAPPED */
        case 0x8: case 0x9: case 0xA: case 0xB: case 0xC:
            cpu.cycles += 258;
            break;

        /* RAM */
        case 0xD:
            cpu.cycles += 2;
            ramAddress = address & 0x7FFFF;
            if (ramAddress < 0x65800) {
                mem.ram.block[ramAddress] = byte;
            }
            break;

        /* MMIO <-> Advanced Perphrial Bus */
        case 0xE: case 0xF:
            cpu.cycles += 2;
            port_write_byte(mmio_range(address)<<12 | addr_range(address), byte);
            break;
    }
#ifdef DEBUG_SUPPORT
    if (debugger.data.block[address] & DBG_WRITE_BREAKPOINT) {
        open_debugger(HIT_WRITE_BREAKPOINT, address);
    }
#endif
}
