#include <string.h>
#include <assert.h>

#include "mem.h"
#include "emu.h"
#include "cpu.h"
#include "flash.h"
#include "control.h"
#include "debug/debug.h"

#define mmio_mapped(address, select) ((address) < (((select) = (address) >> 6 & 0x4000) ? 0xFB0000 : 0xE40000))
#define mmio_port(address, select) (0x1000 + (select) + ((address) >> 4 & 0xf000) + ((address) & 0xfff))

/* Global MEMORY state */
mem_state_t mem;

void mem_init(void) {
    unsigned int i;

    /* Allocate FLASH memory */
    mem.flash.block = (uint8_t*)malloc(flash_size);
    memset(mem.flash.block, 0xFF, flash_size);
    mem.flash.size = flash_size;

    for (i = 0; i < 8; i++) {
        mem.flash.sector_8k[i].ptr = mem.flash.block + (i*flash_sector_size_8K);
        mem.flash.sector_8k[i].locked = true;
    }

    for (i = 0; i < 64; i++) {
        mem.flash.sector[i].ptr = mem.flash.block + (i*flash_sector_size_64K);
        mem.flash.sector[i].locked = false;
    }

    /* Sector 2 is locked */
    mem.flash.sector[1].locked = true;
    mem.flash.locked = true;

    /* Allocate RAM */
    mem.ram.block = (uint8_t*)calloc(ram_size, sizeof(uint8_t));

    mem.flash.write_index = 0;
    mem.flash.command = NO_COMMAND;
    gui_console_printf("[CEmu] Initialized Memory...\n");
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
    gui_console_printf("[CEmu] Freed Memory.\n");
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

uint8_t *phys_mem_ptr(uint32_t address, int32_t size) {
    void *block;
    uint32_t block_size, end_addr;

    if (address < 0xD00000) {
        address = flash_address(address, &block_size);
        block = mem.flash.block;
    } else if (address < 0xE00000) {
        address -= 0xD00000;
        block = mem.ram.block;
        block_size = ram_size;
    } else {
        address -= 0xE30800;
        // TODO: Handle some other MMIO things
        block = lcd.crsrImage;
        block_size = sizeof(lcd.crsrImage);
    }
    if (size < 0) {
        address += size;
        size = -size;
    }
    end_addr = address + size;
    if (address <= end_addr && address <= block_size && end_addr <= block_size && block) {
        return (uint8_t *)block + address;
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
    gui_console_printf("[CEmu] Erased Flash chip.\n");
}

static void flash_erase_sector(uint32_t address, uint8_t byte) {
    uint8_t selected;
    (void)byte;

    mem.flash.command = FLASH_SECTOR_ERASE;
    selected = address/flash_sector_size_64K;

    if (!mem.flash.sector[selected].locked) {
        memset(mem.flash.sector[selected].ptr, 0xFF, flash_sector_size_64K);
    }
}

static void flash_verify_sector_protection(uint32_t address, uint8_t byte) {
    (void)address;
    (void)byte;

    mem.flash.command = FLASH_READ_SECTOR_PROTECTION;
}

static void flash_cfi_read(uint32_t address, uint8_t byte) {
    (void)address;
    (void)byte;

    mem.flash.command = FLASH_READ_CFI;
}

static void flash_enter_deep_power_down(uint32_t address, uint8_t byte) {
    (void)address;
    (void)byte;

    mem.flash.command = FLASH_DEEP_POWER_DOWN;
}

static void flash_enter_IPB(uint32_t address, uint8_t byte) {
    (void)address;
    (void)byte;

    mem.flash.command = FLASH_IPB_MODE;
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
        .length = 1,
        .pattern = {
            { .address = 0xAA, .address_mask = 0xFFF, .value = 0x98, .value_mask = 0xFF },
        },
        .handler = flash_cfi_read
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
        .length = 3,
        .pattern = {
            { .address = 0xAAA, .address_mask = 0xFFF, .value = 0xAA, .value_mask = 0xFF },
            { .address = 0x555, .address_mask = 0xFFF, .value = 0x55, .value_mask = 0xFF },
            { .address = 0x000, .address_mask = 0x000, .value = 0xB9, .value_mask = 0xFF },
        },
        .handler = flash_enter_deep_power_down
    },
    {
        .length = 3,
        .pattern = {
            { .address = 0xAAA, .address_mask = 0xFFF, .value = 0xAA, .value_mask = 0xFF },
            { .address = 0x555, .address_mask = 0xFFF, .value = 0x55, .value_mask = 0xFF },
            { .address = 0xAAA, .address_mask = 0xFFF, .value = 0xC0, .value_mask = 0xFF },
        },
        .handler = flash_enter_IPB
    },
    {
        .length = 0
    }

};

static uint8_t flash_read_handler(uint32_t address) {
    uint8_t value = 0;
    uint8_t selected;

    address = flash_address(address, NULL);
    if (flash.mapped) {
        switch(mem.flash.command) {
            case NO_COMMAND:
                value = mem.flash.block[address];
                break;
            case FLASH_SECTOR_ERASE:
                value = 0x80;
                mem.flash.read_index++;
                if (mem.flash.read_index == 3) {
                    /* Simulate erase delay */
                    gui_emu_sleep(1.5e4);
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
                    selected = address/flash_sector_size_8K;
                    value = mem.flash.sector_8k[selected].locked ? 1 : 0;
                } else {
                    selected = address/flash_sector_size_64K;
                    value = mem.flash.sector[selected].locked ? 1 : 0;
                }
                break;
            case FLASH_READ_CFI:
                if (address >= 0x20 && address <= 0x2A) {
                    static const uint8_t id[7] = { 0x51, 0x52, 0x59, 0x02, 0x00, 0x40, 0x00 };
                    value = id[(address - 0x20)/2];
                } else if (address >= 0x36 && address <= 0x50) {
                    static const uint8_t id[] = { 0x27, 0x36, 0x00, 0x00, 0x03, 0x04, 0x08,
                                                  0x0E, 0x03, 0x05, 0x03, 0x03, 0x16, 0x02,
                                                  0x00, 0x05, 0x00, 0x01, 0x08, 0x00, 0x00,
                                                  0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50,
                                                  0x52, 0x49, 0x31, 0x33, 0x0C, 0x02, 0x01,
                                                  0x00, 0x08, 0x00, 0x00, 0x02, 0x95, 0xA5,
                                                  0x02, 0x01 };
                    value = id[(address - 0x36)/2];
                }
                break;
            case FLASH_DEEP_POWER_DOWN:
                break;
            case FLASH_IPB_MODE:
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
        if ((mem.flash.command != FLASH_DEEP_POWER_DOWN && byte == 0xF0) ||
            (mem.flash.command == FLASH_DEEP_POWER_DOWN && byte == 0xAB)) {
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

uint8_t mem_read_cpu(uint32_t address, bool fetch) {
    static const uint8_t mmio_readcycles[0x20] = {2,2,4,3,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,2};
    uint8_t value = 0;
    uint32_t ramAddress, select;

    address &= 0xFFFFFF;
#ifdef DEBUG_SUPPORT
    if (debugger.data.block[address] & DBG_READ_WATCHPOINT) {
        open_debugger(HIT_READ_BREAKPOINT, address);
    }
#endif
    // reads from protected memory return 0
    if (!(!fetch && address >= control.protectedStart && address <= control.protectedEnd && unprivileged_code())) {
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
                cpu.cycles += mmio_readcycles[(address >> 16) & 0x1F];
                if (mmio_mapped(address, select)) {
                    value = port_read_byte(mmio_port(address, select));
                }
                break;
        }
    }
    return value;
}

void mem_write_cpu(uint32_t address, uint8_t value) {
    static const uint8_t mmio_writecycles[0x20] = {2,2,4,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,2};
    uint32_t ramAddress, select;
    address &= 0xFFFFFF;

#ifdef DEBUG_SUPPORT
    if ((debugger.data.block[address] &= ~(DBG_INST_START_MARKER | DBG_INST_MARKER)) & DBG_WRITE_WATCHPOINT) {
        open_debugger(HIT_WRITE_BREAKPOINT, address);
    }
#endif

    if (address == control.stackLimit) {
        control.protectionStatus |= 1;
        cpu_nmi();
        gui_console_printf("[CEmu] NMI reset caused by writing to the stack limit at address %#06x. Hint: Probably a stack overflow (aka too much recursion).\n", address);
    } // writes to stack limit succeed

    if (address >= control.protectedStart && address <= control.protectedEnd && unprivileged_code()) {
        control.protectionStatus |= 2;
        cpu_nmi();
        gui_console_printf("[CEmu] NMI reset caused by writing to protected memory (%#06x through %#06x) at address %#06x from unprivileged code.\n", control.protectedStart, control.protectedEnd, address);
    } else { // writes to protected memory are ignored
        switch((address >> 20) & 0xF) {
            /* FLASH */
            case 0x0: case 0x1: case 0x2: case 0x3:
            case 0x4: case 0x5: case 0x6: case 0x7:
                if (unprivileged_code()) {
                    control.protectionStatus |= 2;
                    cpu_nmi();
                    gui_console_printf("[CEmu] NMI reset cause by write to flash at address %#06x from unprivileged code. Hint: Possibly a null pointer dereference.\n", address);
                } else if (!mem.flash.locked) {
                    flash_write_handler(address, value);
                } // privileged writes with flash locked are probably ignored
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
                    mem.ram.block[ramAddress] = value;
                }
                break;

                /* MMIO <-> Advanced Perphrial Bus */
            case 0xE: case 0xF:
                cpu.cycles += mmio_writecycles[(address >> 16) & 0x1F];
#ifdef DEBUG_SUPPORT
                if (address >= DBG_PORT_RANGE) {
                    open_debugger(address, value);
                    break;
                } else if ((address >= DBGOUT_PORT_RANGE && address < DBGOUT_PORT_RANGE+SIZEOF_DBG_BUFFER-1)) {
                    if (value != 0) {
                        debugger.buffer[debugger.currentBuffPos] = (char)value;
                        debugger.currentBuffPos = (debugger.currentBuffPos + 1) % (SIZEOF_DBG_BUFFER);
                    }
                    break;
                } else if ((address >= DBGERR_PORT_RANGE && address < DBGERR_PORT_RANGE+SIZEOF_DBG_BUFFER-1)) {
                    if (value != 0) {
                        debugger.errBuffer[debugger.currentErrBuffPos] = (char)value;
                        debugger.currentErrBuffPos = (debugger.currentErrBuffPos + 1) % (SIZEOF_DBG_BUFFER);
                    }
                    break;
                }
#endif
                if (mmio_mapped(address, select)) {
                    port_write_byte(mmio_port(address, select), value);
                }
                break;
        }
    }
}

uint8_t mem_peek_byte(uint32_t address) {
    uint8_t value = 0;
    uint32_t select;
    address &= 0xFFFFFF;
    if (address < 0xE00000) {
        uint8_t *ptr;
        if ((ptr = phys_mem_ptr(address, 1))) {
            value = *ptr;
        }
    } else if (mmio_mapped(address, select)) {
        value = port_peek_byte(mmio_port(address, select));
    }
    return value;
}
uint16_t mem_peek_short(uint32_t address) {
    return mem_peek_byte(address)
         | mem_peek_byte(address + 1) << 8;
}
uint32_t mem_peek_long(uint32_t address) {
    return mem_peek_byte(address)
         | mem_peek_byte(address + 1) << 8
         | mem_peek_byte(address + 2) << 16;
}
uint32_t mem_peek_word(uint32_t address, bool mode) {
    if (mode) {
        return mem_peek_long(address);
    } else {
        return (uint32_t)mem_peek_short((address & 0xFFFF) | (cpu.registers.MBASE << 16));
    }
}

void mem_poke_byte(uint32_t address, uint8_t value) {
    uint32_t select;
    address &= 0xFFFFFF;
    if (address < 0xE00000) {
        uint8_t *ptr;
        if ((ptr = phys_mem_ptr(address, 1))) {
            *ptr = value;
        }
    } else if (mmio_mapped(address, select)) {
        port_poke_byte(mmio_port(address, select), value);
    }
}

bool mem_save(emu_image *s) {
    assert(mem.flash.block);
    assert(mem.ram.block);

    memcpy(s->mem_flash, mem.flash.block, flash_size);
    memcpy(s->mem_ram, mem.ram.block, ram_size);

    s->mem = mem;
    s->mem.flash.block = NULL;
    s->mem.ram.block = NULL;
    return true;
}

bool mem_restore(const emu_image *s) {
    unsigned int i;
    uint8_t *tmp_flash_ptr = mem.flash.block;
    uint8_t *tmp_ram_ptr = mem.ram.block;

    mem = s->mem;

    mem.flash.block = tmp_flash_ptr;
    mem.ram.block = tmp_ram_ptr;

    memcpy(mem.flash.block, s->mem_flash, flash_size);
    memcpy(mem.ram.block, s->mem_ram, ram_size);

    for (i = 0; i < 8; i++) {
        mem.flash.sector[i].ptr = mem.flash.block + (i*flash_sector_size_8K);
    }
    for (i = 0; i < 64; i++) {
        mem.flash.sector[i].ptr = mem.flash.block + (i*flash_sector_size_64K);
    }
    return true;
}
