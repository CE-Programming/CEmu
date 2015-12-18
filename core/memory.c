#include "core/memory.h"
#include "core/cpu.h"
#include "core/emu.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Global MEMORY state
mem_state_t mem;

const uint32_t ram_size = 0x65800;
const uint32_t ram_base = 0xD00000;
const uint32_t flash_size = 0x400000;

void mem_init(void) {
    mem.flash=(uint8_t*)malloc(flash_size);     // allocate Flash memory
    memset(mem.flash, 0xFF, flash_size);

    mem.ram=(uint8_t*)calloc(ram_size, sizeof(uint8_t)); // allocate RAM

    mem.flash_mapped = 0;
    mem.flash_unlocked = 0;
    gui_console_printf("Initialized memory...\n");
}

void mem_free(void) {
    free(mem.ram);
    free(mem.flash);
    gui_console_printf("Freed memory...\n");
}

uint8_t* phys_mem_ptr(uint32_t addr, uint32_t size) {
    if(addr<0xD00000) {
        return mem.flash+addr;
    }
    addr -= 0xD00000;
    return mem.ram+addr;
}

// returns wait cycles
uint8_t memory_read_byte(uint32_t address)
{
    uint32_t addr; // XXX should be uint32_t
    address &= 0xFFFFFF;
    addr = address;

    switch(upperNibble24(addr)) {
        // FLASH
        case 0x0: case 0x1: case 0x2: case 0x3:
            cpu.cycles += 5;
            return mem.flash[addr];

        // MAYBE FLASH
        case 0x4: case 0x5: case 0x6: case 0x7:
            addr -= 0x400000;
            if (mem.flash_mapped) {
                cpu.cycles += 5;
                return mem.flash[addr];
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
                return mem.ram[addr];
            }
        // UNMAPPED
            addr -= 0x65800;
            if(addr < 0x1A800) {
                cpu.cycles += 3;
                return 0;
            }
        // MIRRORED
            return memory_read_byte(address - 0x80000);

        case 0xE: case 0xF:
            cpu.cycles += 2;
            return mmio_read_byte(addr);          // read byte from mmio

        default:
            cpu.cycles += 1;
            break;
    }
    return 0;
}

void memory_write_byte(uint32_t address, const uint8_t byte) {
    uint32_t addr; // XXX should be uint32_t
    address &= 0xFFFFFF;
    addr = address;

    switch(upperNibble24(addr)) {
        // FLASH
        case 0x0: case 0x1: case 0x2: case 0x3:
            if(mem.flash_unlocked) {
                mem.flash[addr] = byte;
            }
            cpu.cycles += 5;
            return;

        // MAYBE FLASH
        case 0x4: case 0x5: case 0x6: case 0x7:
            addr -= 0x400000;
            if(mem.flash_unlocked) {
                if(mem.flash_mapped) {
                    cpu.cycles += 5;
                    mem.flash[addr] = byte;
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
            if(addr <= 0x657FF) {
                cpu.cycles += 2;
                mem.ram[addr] = byte;
                return;
            }
            // UNMAPPED
            addr -=  0x65800;
            if(addr <= 0x1A7FF) {
                cpu.cycles += 1;
                return;
            }
            // MIRRORED
            memory_write_byte(address - 0x80000, byte);
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
