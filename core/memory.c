#include "core/memory.h"
#include "core/cpu.h"
#include "core/emu.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// Global MEMORY state
mem_state_t mem;

static const uint32_t ram_size = 0x65800;
static const uint32_t flash_size = 0x400000;
static const uint32_t flash_sector_size_8K = 0x2000;
static const uint32_t flash_sector_size_64K = 0x10000;
static const uint32_t flash_sectors_8K = 8;
static const uint32_t flash_sectors_64K = 63;

void mem_init(void) {
    unsigned int i;

    mem.flash.block = (uint8_t*)malloc(flash_size);               /* allocate Flash memory */
    memset(mem.flash.block, 0xFF, flash_size);

    for (i = 0; i < flash_sectors_8K; i++) {
        mem.flash.sector[i].ptr = mem.flash.block + (i*flash_sector_size_8K);
        mem.flash.sector[i].locked = true;
    }

    for (i = flash_sectors_8K; i < flash_sectors_64K+flash_sectors_8K; i++) {
        mem.flash.sector[i].ptr = mem.flash.block + (i*flash_sector_size_64K);
        mem.flash.sector[i].locked = false;
    }

    /* Sector 9 is locked */
    mem.flash.sector[9].locked = true;

    mem.ram.block = (uint8_t*)calloc(ram_size, sizeof(uint8_t));  /* allocate RAM */

    mem.flash.mapped = false;
    mem.flash.locked = true;
    mem.flash.write_index = 0;
    mem.flash.command = NO_COMMAND;
    gui_console_printf("Initialized memory...\n");
}

void mem_free(void) {
    if (mem.ram.block) {
        free(mem.ram.block);
        mem.ram.block = NULL;
    }
    if (mem.flash.block) {
        free(mem.flash.block);
        mem.flash.block = NULL;
    }
    gui_console_printf("Freed memory...\n");
}

uint8_t* phys_mem_ptr(uint32_t addr, uint32_t size) {
    if (addr < 0xD00000) {
        return mem.flash.block+addr;
    }
    addr -= 0xD00000;
    return mem.ram.block+addr;
}

static void flash_reset_write_index(uint32_t addr, uint8_t byte) {
    (void)addr;
    (void)byte;
    mem.flash.write_index = 0;
}

static void flash_write(uint32_t addr, uint8_t byte) {
    mem.flash.block[addr] &= byte;
}

static void flash_erase(uint32_t addr, uint8_t byte) {
    (void)addr;
    (void)byte;

    mem.flash.command = FLASH_CHIP_ERASE;

    memset(mem.flash.block, 0xFF, flash_size);
    gui_console_printf("Erased entire Flash chip.\n");
}

static void flash_erase_sector(uint32_t addr, uint8_t byte) {
    uint8_t sector;

    (void)byte;

    mem.flash.command = FLASH_SECTOR_ERASE;

    /* Reset sector */
    sector = addr / flash_sector_size_64K;
    if(mem.flash.sector[sector].locked == false) {
        memset(mem.flash.sector[sector].ptr, 0xFF, flash_sector_size_64K);
    }
}

static void flash_verify_sector_protection(uint32_t addr, uint8_t byte) {
    (void)byte;
    (void)addr;

    mem.flash.command = FLASH_READ_SECTOR_PROTECTION;
}

typedef struct flash_write_pattern {
    int length;
    const flash_write_t pattern[6];
    void (*handler)(uint32_t address, uint8_t value);
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
    uint8_t sector = 0;

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
            value = 0x80;
            mem.flash.command = NO_COMMAND;
            break;
        case FLASH_READ_SECTOR_PROTECTION:
            sector = (address / ((address < 0x10000) ? flash_sector_size_8K : flash_sector_size_64K)) + ((address < 0x10000) ? 0 : flash_sectors_8K);
            gui_console_printf("%d\n",sector);
            value = (uint8_t)mem.flash.sector[sector].locked;
            break;
    }

    /* Returning 0x00 is enough to emulate for the OS. Set the msb bit for user routines? */
    return value;
}

static void flash_write_handler(uint32_t address, uint8_t byte) {
    int i;
    int partial_match = 0;
    flash_write_t *w;
    struct flash_write_pattern *pattern;

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

/* returns wait cycles */
uint8_t memory_read_byte(const uint32_t address)
{
    uint32_t addr;
    addr = address & 0xFFFFFF;

    switch(upperNibble24(addr)) {
        // FLASH
        case 0x0: case 0x1: case 0x2: case 0x3:
            cpu.cycles += 5;
            return flash_read_handler(address);

        // MAYBE FLASH
        case 0x4: case 0x5: case 0x6: case 0x7:
            addr -= 0x400000;
            if (mem.flash.mapped == true) {
                cpu.cycles += 5;
                return flash_read_handler(address);
            }

        // UNMAPPED
        case 0x8: case 0x9: case 0xA: case 0xB: case 0xC:
            cpu.cycles += 257;
            return 0;

        // RAM
        case 0xD:
            addr -= 0xD00000;
            if (addr < 0x65800) {
                cpu.cycles += 3;
                return mem.ram.block[addr];
            }
        // UNMAPPED
            addr -= 0x65800;
            if (addr < 0x1A800) {
                cpu.cycles += 3;
                return 0;
            }
        // MIRRORED
            return memory_read_byte(addr - 0x80000);

        case 0xE: case 0xF:
            cpu.cycles += 2;
            return mmio_read_byte(addr);          // read byte from mmio

        default:
            cpu.cycles += 1;
            break;
    }
    return 0;
}

void memory_write_byte(const uint32_t address, const uint8_t byte) {
    uint32_t addr;
    addr = address & 0xFFFFFF;

    switch(upperNibble24(addr)) {
        // FLASH
        case 0x0: case 0x1: case 0x2: case 0x3:
            if (mem.flash.locked == false) {
                flash_write_handler(addr, byte);
            }
            cpu.cycles += 5;
            return;

        // MAYBE FLASH
        case 0x4: case 0x5: case 0x6: case 0x7:
            addr -= 0x400000;
            if (mem.flash.locked == false) {
                if (mem.flash.mapped == true) {
                    cpu.cycles += 5;
                    flash_write_handler(addr, byte);
                    return;
                }
            }
            cpu.cycles += 257;
            return;

        // UNMAPPED
        case 0x8: case 0x9: case 0xA: case 0xB: case 0xC:
            cpu.cycles += 5;
            return;

        // RAM
        case 0xD:
            addr -= 0xD00000;
            if (addr <= 0x657FF) {
                cpu.cycles += 2;
                mem.ram.block[addr] = byte;
                return;
            }
            // UNMAPPED
            addr -=  0x65800;
            if (addr <= 0x1A7FF) {
                cpu.cycles += 1;
                return;
            }
            // MIRRORED
            memory_write_byte(addr - 0x80000, byte);
            return;

        // MMIO <-> Advanced Perphrial Bus
        case 0xE: case 0xF:
            cpu.cycles += 2;
            mmio_write_byte(addr, byte);         // write byte to the mmio port
            return;

        default:
            cpu.cycles += 1;
            break;
    }
    return;
}
