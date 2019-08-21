#include "armmem.h"

#include <assert.h>
#include <stdlib.h>

arm_mem_state_t arm_mem;

void arm_mem_init(void) {
    arm_mem.flash = calloc(0x10000, sizeof(uint32_t));
    arm_mem.ram = calloc(0x2000, sizeof(uint32_t));
}

uint8_t arm_mem_load_byte(uint32_t addr) {
    return arm_mem_load_word(addr & ~UINT32_C(3)) >> ((addr & UINT32_C(3)) << UINT32_C(3));
}

uint16_t arm_mem_load_half(uint32_t addr) {
    return arm_mem_load_word(addr & ~UINT32_C(1)) >> ((addr & UINT32_C(1)) << UINT32_C(4));
}

uint32_t arm_mem_load_word(uint32_t addr) {
    assert(!(addr & UINT32_C(3)));
    if (addr - UINT32_C(0) < UINT32_C(0x40000)) {
        return arm_mem.flash[(addr - UINT32_C(0)) >> 2];
    }
    if (addr - UINT32_C(0x20000000) < UINT32_C(0x8000)) {
        return arm_mem.ram[(addr - UINT32_C(0x20000000)) >> 2];
    }
    return 0;
}

static void arm_mem_store(uint32_t val, uint32_t mask, uint32_t addr) {
    assert(!(addr & UINT32_C(3)) && !(val & ~mask));
    if (addr - UINT32_C(0x20000000) < UINT32_C(0x8000)) {
        uint32_t *ptr = &arm_mem.ram[(addr - UINT32_C(0x20000000)) >> UINT32_C(2)];
        *ptr = (*ptr & ~mask) | val;
    }
}

void arm_mem_store_byte(uint8_t val, uint32_t addr) {
    uint32_t shift = (addr & UINT32_C(3)) << UINT32_C(3);
    arm_mem_store(val << shift, UINT32_C(0xFF) << shift, addr & ~UINT32_C(3));
}

void arm_mem_store_half(uint16_t val, uint32_t addr) {
    uint32_t shift = (addr & UINT32_C(1)) << UINT32_C(4);
    arm_mem_store(val << shift, UINT32_C(0xFFFF) << shift, addr & ~UINT32_C(1));
}

void arm_mem_store_word(uint32_t val, uint32_t addr) {
    arm_mem_store(val, UINT32_C(0xFFFFFFFF), addr);
}
