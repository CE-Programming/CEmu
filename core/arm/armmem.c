#include "armmem.h"

#include "armstate.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define ARM_FLASH_WORDS 0x10000
#define ARM_RAM_WORDS   0x2000

bool arm_mem_init(arm_state_t *state) {
    state->flash = calloc(ARM_FLASH_WORDS, sizeof(uint32_t));
    if (state->flash) {
        state->ram = calloc(ARM_RAM_WORDS, sizeof(uint32_t));
        if (state->ram) {
            return true;
            free(state->ram);
        }
        free(state->flash);
    }
    return false;
}

void arm_mem_destroy(arm_state_t *state) {
    free(state->flash);
    free(state->ram);
}

void arm_mem_reset(arm_state_t *state) {
    memset(state->ram, 0, ARM_RAM_WORDS * sizeof(uint32_t));
}

bool arm_mem_load_rom(arm_state_t *state, FILE *file) {
    size_t read = fread(state->flash, 1, ARM_FLASH_WORDS * sizeof(uint32_t), file);
    if (!read) {
        return false;
    }
    memset(state->flash + read, ~0, ARM_FLASH_WORDS * sizeof(uint32_t) - read);
    return true;
}

uint8_t arm_mem_load_byte(arm_state_t *state, uint32_t addr) {
    return arm_mem_load_word(state, addr & ~UINT32_C(3)) >> ((addr & UINT32_C(3)) << UINT32_C(3));
}

uint16_t arm_mem_load_half(arm_state_t *state, uint32_t addr) {
    return arm_mem_load_word(state, addr & ~UINT32_C(2)) >> ((addr & UINT32_C(2)) << UINT32_C(3));
}

uint32_t arm_mem_load_word(arm_state_t *state, uint32_t addr) {
    assert(!(addr & UINT32_C(3)));
    if (addr - UINT32_C(0) < UINT32_C(0x40000)) {
        return state->flash[(addr - UINT32_C(0)) >> 2];
    }
    if (addr - UINT32_C(0x20000000) < UINT32_C(0x8000)) {
        return state->ram[(addr - UINT32_C(0x20000000)) >> 2];
    }
    return 0;
}

static void arm_mem_store(arm_state_t *state, uint32_t val, uint32_t mask, uint32_t addr) {
    assert(!(addr & UINT32_C(3)) && !(val & ~mask));
    if (addr - UINT32_C(0x20000000) < UINT32_C(0x8000)) {
        uint32_t *ptr = &state->ram[(addr - UINT32_C(0x20000000)) >> UINT32_C(2)];
        *ptr = (*ptr & ~mask) | val;
    }
}

void arm_mem_store_byte(arm_state_t *state, uint8_t val, uint32_t addr) {
    uint32_t shift = (addr & UINT32_C(3)) << UINT32_C(3);
    arm_mem_store(state, val << shift, UINT32_C(0xFF) << shift, addr & ~UINT32_C(3));
}

void arm_mem_store_half(arm_state_t *state, uint16_t val, uint32_t addr) {
    uint32_t shift = (addr & UINT32_C(2)) << UINT32_C(3);
    arm_mem_store(state, val << shift, UINT32_C(0xFFFF) << shift, addr & ~UINT32_C(2));
}

void arm_mem_store_word(arm_state_t *state, uint32_t val, uint32_t addr) {
    arm_mem_store(state, val, UINT32_C(0xFFFFFFFF), addr);
}
