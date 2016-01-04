#include <string.h>

#include "mem.h"
#include "emu.h"
#include "cpu.h"

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
    mem.flash.size = flash_size;

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
    mem.flash.locked = true;

    mem.ram.block = (uint8_t*)calloc(ram_size, sizeof(uint8_t));      /* Allocate RAM */

    mem.debug.block = (uint8_t*)calloc(0xFFFFFF, sizeof(uint8_t));    /* Allocate Debug memory */
    mem.debug.ports = (uint8_t*)calloc(0xFFFF, sizeof(uint8_t));      /* Allocate Debug Port Monitor */

    mem.flash.mapped = false;
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
    if (mem.debug.block) {
        free(mem.debug.block);
        mem.debug.block = NULL;
    }
    if (mem.debug.ports) {
        free(mem.debug.ports);
        mem.debug.ports = NULL;
    }
    gui_console_printf("Freed memory...\n");
}

void mem_reset(void) {
    memset(mem.ram.block, 0, ram_size);
    gui_console_printf("RAM reset.\n");
}

uint8_t* phys_mem_ptr(uint32_t addr, uint32_t size) {
    if (addr < 0xD00000) {
        if (mem.flash.block != NULL) {
            return mem.flash.block+addr;
        } else {
            return NULL;
        }
    }
    if (mem.ram.block != NULL) {
        addr -= 0xD00000;
        return mem.ram.block+addr;
    } else {
       return NULL;
    }
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
    uint8_t value = 0;
    uint32_t addr = address & 0xFFFFFF;

    switch((addr >> 20) & 0xF) {
        // FLASH
        case 0x0: case 0x1: case 0x2: case 0x3:
            cpu.cycles += 5;
            value = flash_read_handler(address);
            break;

        // MAYBE FLASH
        case 0x4: case 0x5: case 0x6: case 0x7:
            addr -= 0x400000;
            if (mem.flash.mapped == true) {
                cpu.cycles += 5;
                value = flash_read_handler(address);
            }
            break;

        // UNMAPPED
        case 0x8: case 0x9: case 0xA: case 0xB: case 0xC:
            cpu.cycles += 257;
            break;

        // RAM
        case 0xD:
            addr -= 0xD00000;
            if (addr < 0x65800) {
                cpu.cycles += 3;
                value = mem.ram.block[addr];
                break;
            }
        // UNMAPPED
            addr -= 0x65800;
            if (addr < 0x1A800) {
                cpu.cycles += 3;
                break;
            }
        // MIRRORED
            value = memory_read_byte(addr - 0x80000);
            break;

        case 0xE: case 0xF:
            cpu.cycles += 2;
            value = port_read_byte(mmio_range(addr)<<12 | addr_range(addr));          // read byte from mmio
            break;

        default:
            cpu.cycles += 1;
            break;
    }

    if (mem.debug.block[addr] & DBG_READ_BREAKPOINT) {
        debugger(HIT_READ_BREAKPOINT, addr);
    }
    if ((mem.debug.block[addr] & DBG_EXEC_BREAKPOINT) && (cpu.registers.PC == ((addr+1)&0xFFFFFF))) {
        debugger(HIT_EXEC_BREAKPOINT, addr);
    }

    return value;
}

void memory_write_byte(const uint32_t address, const uint8_t byte) {
    uint32_t addr = address & 0xFFFFFF;

    switch((addr >> 20) & 0xF) {
        // FLASH
        case 0x0: case 0x1: case 0x2: case 0x3:
            if (mem.flash.locked == false) {
                flash_write_handler(addr, byte);
            }
            cpu.cycles += 5;
            break;

        // MAYBE FLASH
        case 0x4: case 0x5: case 0x6: case 0x7:
            addr -= 0x400000;
            if (mem.flash.locked == false) {
                if (mem.flash.mapped == true) {
                    cpu.cycles += 5;
                    flash_write_handler(addr, byte);
                    break;
                }
            }
            cpu.cycles += 257;
            break;

        // UNMAPPED
        case 0x8: case 0x9: case 0xA: case 0xB: case 0xC:
            cpu.cycles += 5;
            break;

        // RAM
        case 0xD:
            addr -= 0xD00000;
            if (addr < 0x65800) {
                cpu.cycles += 2;
                mem.ram.block[addr] = byte;
                break;
            }
            // UNMAPPED
            addr -=  0x65800;
            if (addr < 0x1A800) {
                cpu.cycles += 1;
                break;
            }
            // MIRRORED
            memory_write_byte(addr - 0x80000, byte);
            break;

        // MMIO <-> Advanced Perphrial Bus
        case 0xE: case 0xF:
            cpu.cycles += 2;
            port_write_byte(mmio_range(addr)<<12 | addr_range(addr), byte);         // write byte to the mmio port
            break;

        default:
            cpu.cycles += 1;
            break;
    }

    if (mem.debug.block[addr] & DBG_WRITE_BREAKPOINT) {
        debugger(HIT_WRITE_BREAKPOINT, addr);
    }

    return;
}
