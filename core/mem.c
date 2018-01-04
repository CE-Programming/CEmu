#include "mem.h"
#include "emu.h"
#include "cpu.h"
#include "flash.h"
#include "control.h"
#include "lcd.h"
#include "debug/debug.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define mmio_mapped(addr, select) ((addr) < (((select) = (addr) >> 6 & 0x4000) ? 0xFB0000 : 0xE40000))
#define mmio_port(addr, select) (0x1000 + (select) + ((addr) >> 4 & 0xF000) + ((addr) & 0xFFF))

/* Global MEMORY state */
mem_state_t mem;

void mem_init(void) {
    unsigned int i;

    /* Allocate FLASH memory */
    mem.flash.block = (uint8_t*)malloc(SIZE_FLASH);
    memset(mem.flash.block, 0xFF, SIZE_FLASH);

    for (i = 0; i < 8; i++) {
        mem.flash.sector_8k[i].ptr = mem.flash.block + i * SIZE_FLASH_SECTOR_8K;
        mem.flash.sector_8k[i].locked = true;
    }

    for (i = 0; i < 64; i++) {
        mem.flash.sector[i].ptr = mem.flash.block + i * SIZE_FLASH_SECTOR_64K;
        mem.flash.sector[i].locked = false;
    }

    /* Sector 2 is locked */
    mem.flash.sector[1].locked = true;

    /* Allocate RAM */
    mem.ram.block = (uint8_t*)calloc(SIZE_RAM, 1);

    mem.flash.write_index = 0;
    mem.flash.command = NO_COMMAND;
    gui_console_printf("[CEmu] Initialized Memory...\n");
}

void mem_free(void) {
    free(mem.ram.block);
    mem.ram.block = NULL;
    free(mem.flash.block);
    mem.flash.block = NULL;
    gui_console_printf("[CEmu] Freed Memory.\n");
}

void mem_reset(void) {
    memset(mem.ram.block, 0, SIZE_RAM);
    gui_console_printf("[CEmu] Memory reset.\n");
}

static uint32_t flash_block(uint32_t *addr, uint32_t *size) {
    uint32_t mask = flash.mask;
    if (size) {
        *size = mask + 1;
    }
    if (*addr <= mask && flash.mapped) {
        return flash.waitStates;
    }
    *addr &= mask;
    return 258;
}

static void fix_size(uint32_t *addr, int32_t *size) {
    if (*size < 0) {
        *addr += *size;
        *size = -*size;
    }
}

static uint32_t addr_block(uint32_t *addr, int32_t size, void **block, uint32_t *block_size) {
    if (*addr < 0xD00000) {
        flash_block(addr, block_size);
        *block = mem.flash.block;
        *block_size = SIZE_FLASH;
    } else if (*addr < 0xE00000) {
        *addr -= 0xD00000;
        *block = mem.ram.block;
        *block_size = SIZE_RAM;
    } else if (*addr < 0xE30800) {
        *addr -= 0xE30200;
        *block = lcd.palette;
        *block_size = sizeof lcd.palette;
    } else {
        *addr -= 0xE30800;
        *block = lcd.crsrImage;
        *block_size = sizeof lcd.crsrImage;
    }
    return *addr + size;
}

uint8_t *phys_mem_ptr(uint32_t addr, int32_t size) {
    void *block;
    uint32_t block_size, end_addr;
    fix_size(&addr, &size);
    end_addr = addr_block(&addr, size, &block, &block_size);
    if (addr <= end_addr && addr <= block_size && end_addr <= block_size && block) {
        return (uint8_t *)block + addr;
    }
    return NULL;
}

uint8_t *virt_mem_cpy(uint8_t *dest, uint32_t addr, int32_t size) {
    uint8_t *save_dest;
    void *block;
    uint32_t temp_addr, block_size, end_addr;
    fix_size(&addr, &size);
    if (!dest) {
        dest = malloc(size);
    }
    save_dest = dest;
    while (size) {
        temp_addr = addr;
        end_addr = addr_block(&temp_addr, size, &block, &block_size);
        if (temp_addr <= end_addr && temp_addr < block_size && block) {
            uint32_t temp_size = (end_addr <= block_size ? end_addr : block_size) - temp_addr;
            memcpy(dest, (uint8_t *)block + temp_addr, temp_size);
            dest += temp_size;
            addr += temp_size;
            size -= temp_size;
        } else {
            *dest++ = mem_peek_byte(addr++);
            size--;
        }
    }
    return save_dest;
}

uint8_t *virt_mem_dup(uint32_t addr, int32_t size) {
    return virt_mem_cpy(NULL, addr, size);
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
    unsigned int i;
    (void)addr;
    (void)byte;

    mem.flash.command = FLASH_CHIP_ERASE;

    for (i = 0; i < 8; i++) {
        if (!mem.flash.sector_8k[i].locked) {
            memset(mem.flash.sector_8k[i].ptr, 0xFF, SIZE_FLASH_SECTOR_8K);
        }
    }

    for (i = 0; i < 64; i++) {
        if (!mem.flash.sector[i].locked) {
            memset(mem.flash.sector[i].ptr, 0xFF, SIZE_FLASH_SECTOR_64K);
        }
    }

    gui_console_printf("[CEmu] Erased Unlocked Sectors.\n");
}

static void flash_erase_sector(uint32_t addr, uint8_t byte) {
    uint8_t selected;
    (void)byte;

    mem.flash.command = FLASH_SECTOR_ERASE;
    selected = addr/SIZE_FLASH_SECTOR_64K;

    if (!mem.flash.sector[selected].locked) {
        memset(mem.flash.sector[selected].ptr, 0xFF, SIZE_FLASH_SECTOR_64K);
    }
}

static void flash_verify_sector_protection(uint32_t addr, uint8_t byte) {
    (void)addr;
    (void)byte;

    mem.flash.command = FLASH_READ_SECTOR_PROTECTION;
}

static void flash_cfi_read(uint32_t addr, uint8_t byte) {
    (void)addr;
    (void)byte;

    mem.flash.command = FLASH_READ_CFI;
}

static void flash_enter_deep_power_down(uint32_t addr, uint8_t byte) {
    (void)addr;
    (void)byte;

    mem.flash.command = FLASH_DEEP_POWER_DOWN;
}

static void flash_enter_IPB(uint32_t addr, uint8_t byte) {
    (void)addr;
    (void)byte;

    mem.flash.command = FLASH_IPB_MODE;
}

typedef const struct flash_write_pattern {
    const int length;
    const flash_write_t pattern[6];
    void (*const handler)(uint32_t addr, uint8_t value);
} flash_write_pattern_t;

typedef struct flash_status_pattern {
    uint8_t length;
    uint8_t pattern[10];
} flash_status_pattern_t;

static flash_write_pattern_t patterns[] = {
    {
        .length = 4,
        .pattern = {
            { .addr = 0xAAA, .addr_mask = 0xFFF, .value = 0xAA, .value_mask = 0xFF },
            { .addr = 0x555, .addr_mask = 0xFFF, .value = 0x55, .value_mask = 0xFF },
            { .addr = 0xAAA, .addr_mask = 0xFFF, .value = 0xA0, .value_mask = 0xFF },
            { .addr = 0x000, .addr_mask = 0x000, .value = 0x00, .value_mask = 0x00 },
        },
        .handler = flash_write
    },
    {
        .length = 6,
        .pattern = {
            { .addr = 0xAAA, .addr_mask = 0xFFF, .value = 0xAA, .value_mask = 0xFF },
            { .addr = 0x555, .addr_mask = 0xFFF, .value = 0x55, .value_mask = 0xFF },
            { .addr = 0xAAA, .addr_mask = 0xFFF, .value = 0x80, .value_mask = 0xFF },
            { .addr = 0xAAA, .addr_mask = 0xFFF, .value = 0xAA, .value_mask = 0xFF },
            { .addr = 0x555, .addr_mask = 0xFFF, .value = 0x55, .value_mask = 0xFF },
            { .addr = 0x000, .addr_mask = 0x000, .value = 0x30, .value_mask = 0xFF },
        },
        .handler = flash_erase_sector
    },
    {
        .length = 6,
        .pattern = {
            { .addr = 0xAAA, .addr_mask = 0xFFF, .value = 0xAA, .value_mask = 0xFF },
            { .addr = 0x555, .addr_mask = 0xFFF, .value = 0x55, .value_mask = 0xFF },
            { .addr = 0xAAA, .addr_mask = 0xFFF, .value = 0x80, .value_mask = 0xFF },
            { .addr = 0xAAA, .addr_mask = 0xFFF, .value = 0xAA, .value_mask = 0xFF },
            { .addr = 0x555, .addr_mask = 0xFFF, .value = 0x55, .value_mask = 0xFF },
            { .addr = 0xAAA, .addr_mask = 0xFFF, .value = 0x10, .value_mask = 0xFF },
        },
        .handler = flash_erase
    },
    {
        .length = 1,
        .pattern = {
            { .addr = 0xAA, .addr_mask = 0xFFF, .value = 0x98, .value_mask = 0xFF },
        },
        .handler = flash_cfi_read
    },
    {
        .length = 3,
        .pattern = {
            { .addr = 0xAAA, .addr_mask = 0xFFF, .value = 0xAA, .value_mask = 0xFF },
            { .addr = 0x555, .addr_mask = 0xFFF, .value = 0x55, .value_mask = 0xFF },
            { .addr = 0xAAA, .addr_mask = 0xFFF, .value = 0x90, .value_mask = 0xFF },
        },
        .handler = flash_verify_sector_protection
    },
    {
        .length = 3,
        .pattern = {
            { .addr = 0xAAA, .addr_mask = 0xFFF, .value = 0xAA, .value_mask = 0xFF },
            { .addr = 0x555, .addr_mask = 0xFFF, .value = 0x55, .value_mask = 0xFF },
            { .addr = 0x000, .addr_mask = 0x000, .value = 0xB9, .value_mask = 0xFF },
        },
        .handler = flash_enter_deep_power_down
    },
    {
        .length = 3,
        .pattern = {
            { .addr = 0xAAA, .addr_mask = 0xFFF, .value = 0xAA, .value_mask = 0xFF },
            { .addr = 0x555, .addr_mask = 0xFFF, .value = 0x55, .value_mask = 0xFF },
            { .addr = 0xAAA, .addr_mask = 0xFFF, .value = 0xC0, .value_mask = 0xFF },
        },
        .handler = flash_enter_IPB
    },
    {
        .length = 0
    }

};

static uint8_t flash_read_handler(uint32_t addr) {
    uint8_t value = 0;
    uint8_t selected;

    cpu.cycles += flash_block(&addr, NULL);
    if (flash.mapped) {
        switch(mem.flash.command) {
            case NO_COMMAND:
                value = mem.flash.block[addr];
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
                if (addr < 0x10000) {
                    selected = addr/SIZE_FLASH_SECTOR_8K;
                    value = mem.flash.sector_8k[selected].locked ? 1 : 0;
                } else {
                    selected = addr/SIZE_FLASH_SECTOR_64K;
                    value = mem.flash.sector[selected].locked ? 1 : 0;
                }
                break;
            case FLASH_READ_CFI:
                if (addr >= 0x20 && addr <= 0x2A) {
                    static const uint8_t id[7] = { 0x51, 0x52, 0x59, 0x02, 0x00, 0x40, 0x00 };
                    value = id[(addr - 0x20)/2];
                } else if (addr >= 0x36 && addr <= 0x50) {
                    static const uint8_t id[] = {
                        0x27, 0x36, 0x00, 0x00, 0x03, 0x04, 0x08, 0x0E,
                        0x03, 0x05, 0x03, 0x03, 0x16, 0x02, 0x00, 0x05,
                        0x00, 0x01, 0x08, 0x00, 0x00, 0x3F, 0x00, 0x00,
                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                        0x00, 0x00, 0x50, 0x52, 0x49, 0x31, 0x33, 0x0C,
                        0x02, 0x01, 0x00, 0x08, 0x00, 0x00, 0x02, 0x95,
                        0xA5, 0x02, 0x01 };
                    value = id[(addr - 0x36)/2];
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

static void flash_write_handler(uint32_t addr, uint8_t byte) {
    int i;
    int partial_match = 0;
    flash_write_t *w;
    flash_write_pattern_t *pattern;

    cpu.cycles += flash_block(&addr, NULL);
    if (!flash.mapped) {
        return;
    }

    /* See if we can reset to default */
    if (mem.flash.command != NO_COMMAND) {
        if ((mem.flash.command != FLASH_DEEP_POWER_DOWN && byte == 0xF0) ||
            (mem.flash.command == FLASH_DEEP_POWER_DOWN && byte == 0xAB)) {
            mem.flash.command = NO_COMMAND;
            flash_reset_write_index(addr, byte);
            return;
        }
    }

    w = &mem.flash.writes[mem.flash.write_index++];
    w->addr = addr;
    w->value = byte;

    for (pattern = patterns; pattern->length; pattern++) {
        for (i = 0; (i < mem.flash.write_index) && (i < pattern->length) &&
             (mem.flash.writes[i].addr & pattern->pattern[i].addr_mask) == pattern->pattern[i].addr &&
             (mem.flash.writes[i].value & pattern->pattern[i].value_mask) == pattern->pattern[i].value; i++) {
        }
        if (i == pattern->length) {
            pattern->handler(addr, byte);
            partial_match = 0;
            break;
        } else if (i == mem.flash.write_index) {
            partial_match = 1;
        }
    }
    if (!partial_match) {
        flash_reset_write_index(addr, byte);
    }
}

static bool detect_flash_unlock_sequence(uint8_t current) {
    static const uint8_t flash_unlock_sequence[] = { 0xF3, 0x18, 0x00, 0xF3, 0xF3, 0xED, 0x7E, 0xED, 0x56, 0xED, 0x39, 0x28, 0xED, 0x38, 0x28, 0xCB, 0x57 };
    uint8_t i;
    if (current != flash_unlock_sequence[sizeof(flash_unlock_sequence) - 1] || !mmio_unlocked() || unprivileged_code()) {
        return false;
    }
    for (i = 1; i != sizeof(flash_unlock_sequence); i++) {
        if (mem.fetch_buffer[(mem.fetch_index + i) & (sizeof(mem.fetch_buffer) - 1)] != flash_unlock_sequence[i - 1]) {
            return false;
        }
    }
    return true;
}

uint8_t mem_read_cpu(uint32_t addr, bool fetch) {
    uint8_t value = 0;
    uint32_t ramAddr, select;

    addr &= 0xFFFFFF;
#ifdef DEBUG_SUPPORT
    if (!fetch && debugger.data.block[addr] & DBG_READ_WATCHPOINT) {
        open_debugger(HIT_READ_WATCHPOINT, addr);
    }
#endif
    switch((addr >> 20) & 0xF) {
            /* FLASH */
        case 0x0: case 0x1: case 0x2: case 0x3:
        case 0x4: case 0x5: case 0x6: case 0x7:
            value = flash_read_handler(addr);
            if (fetch && detect_flash_unlock_sequence(value)) {
                control.flashUnlocked |= 1 << 3;
            }
            break;

            /* UNMAPPED */
        case 0x8: case 0x9: case 0xA: case 0xB: case 0xC:
            cpu.cycles += 258;
            break;

            /* RAM */
        case 0xD:
            sched_process_pending_dma(4);
            ramAddr = addr & 0x7FFFF;
            if (ramAddr < 0x65800) {
                value = mem.ram.block[ramAddr];
            }
            break;

            /* MMIO <-> Advanced Perphrial Bus */
        case 0xE: case 0xF:
            if (mmio_mapped(addr, select)) {
                value = port_read_byte(mmio_port(addr, select));
            } else {
                if (addr >= 0xFB0000 && addr < 0xFF0000) {
                    cpu.cycles += 3;
                } else {
                    cpu.cycles += 2;
                }
            }
            break;
        }
    }
    if (fetch) {
        mem.fetch_buffer[++mem.fetch_index] = value;
        if (unprivileged_code()) {
            control.flashUnlocked &= ~(1 << 3);
        }
    } else if (addr >= control.protectedStart && addr <= control.protectedEnd && unprivileged_code()) {
        value = 0; /* reads from protected memory return 0 */
    }
    return value;
}

void mem_write_cpu(uint32_t addr, uint8_t value) {
    uint32_t ramAddr, select;
    addr &= 0xFFFFFF;

#ifdef DEBUG_SUPPORT
    if ((debugger.data.block[addr] &= ~(DBG_INST_START_MARKER | DBG_INST_MARKER)) & DBG_WRITE_WATCHPOINT) {
        open_debugger(HIT_WRITE_WATCHPOINT, addr);
    }
#endif

    if (addr == control.stackLimit) {
        control.protectionStatus |= 1;
        gui_console_printf("[CEmu] NMI reset caused by writing to the stack limit at address %#06x. Hint: Probably a stack overflow (aka too much recursion).\n", addr);
        cpu_nmi();
    } // writes to stack limit succeed

    if (addr >= control.protectedStart && addr <= control.protectedEnd && unprivileged_code()) {
        control.protectionStatus |= 2;
        gui_console_printf("[CEmu] NMI reset caused by writing to protected memory (%#06x through %#06x) at address %#06x from unprivileged code.\n", control.protectedStart, control.protectedEnd, addr);
        cpu_nmi();
    } else { // writes to protected memory are ignored
        switch((addr >> 20) & 0xF) {
                /* FLASH */
            case 0x0: case 0x1: case 0x2: case 0x3:
            case 0x4: case 0x5: case 0x6: case 0x7:
                if (unprivileged_code()) {
                    control.protectionStatus |= 2;
                    gui_console_printf("[CEmu] NMI reset cause by write to flash at address %#06x from unprivileged code. Hint: Possibly a null pointer dereference.\n", addr);
                    cpu_nmi();
                } else if (flash_unlocked()) {
                    flash_write_handler(addr, value);
                } // privileged writes with flash locked are probably ignored
                break;

                /* UNMAPPED */
            case 0x8: case 0x9: case 0xA: case 0xB: case 0xC:
                cpu.cycles += 258;
                break;

                /* RAM */
            case 0xD:
                sched_process_pending_dma(2);
                ramAddr = addr & 0x7FFFF;
                if (ramAddr < 0x65800) {
                    mem.ram.block[ramAddr] = value;
                }
                break;

                /* MMIO <-> Advanced Perphrial Bus */
            case 0xE: case 0xF:
#ifdef DEBUG_SUPPORT
                if (emuCommands) {
                    if (addr >= DBG_PORT_RANGE) {
                        open_debugger(addr, value);
                        break;
                    } else if ((addr >= DBGOUT_PORT_RANGE && addr < DBGOUT_PORT_RANGE+SIZEOF_DBG_BUFFER-1)) {
                        if (value) {
                            debugger.buffer[debugger.bufferPos] = (char)value;
                            debugger.bufferPos = (debugger.bufferPos + 1) % SIZEOF_DBG_BUFFER;
                        }
                        break;
                    } else if ((addr >= DBGERR_PORT_RANGE && addr < DBGERR_PORT_RANGE+SIZEOF_DBG_BUFFER-1)) {
                        if (value) {
                            debugger.bufferErr[debugger.bufferErrPos] = (char)value;
                            debugger.bufferErrPos = (debugger.bufferErrPos + 1) % SIZEOF_DBG_BUFFER;
                        }
                        break;
                    }
                }
#endif
                if (mmio_mapped(addr, select)) {
                    port_write_byte(mmio_port(addr, select), value);
                } else {
                    if (addr >= 0xFB0000 && addr < 0xFF0000) {
                        cpu.cycles += 3;
                    } else {
                        cpu.cycles += 2;
                    }
                }
                break;
        }
    }
}

uint8_t mem_peek_byte(uint32_t addr) {
    uint8_t value = 0;
    uint32_t select;
    addr &= 0xFFFFFF;
    if (addr < 0xE00000) {
        uint8_t *ptr;
        if ((ptr = phys_mem_ptr(addr, 1))) {
            value = *ptr;
        }
    } else if (mmio_mapped(addr, select)) {
        value = port_peek_byte(mmio_port(addr, select));
    }
    return value;
}
uint16_t mem_peek_short(uint32_t addr) {
    return mem_peek_byte(addr)
    | mem_peek_byte(addr + 1) << 8;
}
uint32_t mem_peek_long(uint32_t addr) {
    return mem_peek_byte(addr)
    | mem_peek_byte(addr + 1) << 8
    | mem_peek_byte(addr + 2) << 16;
}
uint32_t mem_peek_word(uint32_t addr, bool mode) {
    if (mode) {
        return mem_peek_long(addr);
    } else {
        return (uint32_t)mem_peek_short((addr & 0xFFFF) | (cpu.registers.MBASE << 16));
    }
}

void mem_poke_byte(uint32_t addr, uint8_t value) {
    uint32_t select;
    addr &= 0xFFFFFF;
    if (addr < 0xE00000) {
        uint8_t *ptr;
        if ((ptr = phys_mem_ptr(addr, 1))) {
            *ptr = value;
        }
    } else if (mmio_mapped(addr, select)) {
        port_poke_byte(mmio_port(addr, select), value);
    }
}

bool mem_save(FILE *image) {
    assert(mem.flash.block);
    assert(mem.ram.block);

    return fwrite(&mem, sizeof(mem), 1, image) == 1 &&
           fwrite(mem.flash.block, SIZE_FLASH, 1, image) == 1 &&
           fwrite(mem.ram.block, SIZE_RAM, 1, image) == 1;
}

bool mem_restore(FILE *image) {
    bool ret = false;
    unsigned int i;
    uint8_t *tmp_flash_ptr;
    uint8_t *tmp_ram_ptr;

    assert(mem.flash.block);
    assert(mem.ram.block);

    tmp_flash_ptr = mem.flash.block;
    tmp_ram_ptr = mem.ram.block;

    ret |= fread(&mem, sizeof(mem), 1, image) == 1;

    mem.flash.block = tmp_flash_ptr;
    mem.ram.block = tmp_ram_ptr;

    ret |= fread(mem.flash.block, SIZE_FLASH, 1, image) == 1 &&
           fread(mem.ram.block, SIZE_RAM, 1, image) == 1;

    for (i = 0; i < 8; i++) {
        mem.flash.sector[i].ptr = &mem.flash.block[i*SIZE_FLASH_SECTOR_8K];
    }
    for (i = 0; i < 64; i++) {
        mem.flash.sector[i].ptr = &mem.flash.block[i*SIZE_FLASH_SECTOR_64K];
    }

    return ret;
}

