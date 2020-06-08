#ifndef ARMMEM_H
#define ARMMEM_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct arm_state arm_state_t;

#ifdef __cplusplus
extern "C" {
#endif

bool arm_mem_init(arm_state_t *state);
void arm_mem_destroy(arm_state_t *state);
void arm_mem_reset(arm_state_t *state);
bool arm_mem_load_rom(arm_state_t *state, FILE *file);
uint8_t arm_mem_load_byte(arm_state_t *state, uint32_t addr);
uint16_t arm_mem_load_half(arm_state_t *state, uint32_t addr);
uint32_t arm_mem_load_word(arm_state_t *state, uint32_t addr);
void arm_mem_store_byte(arm_state_t *state, uint8_t val, uint32_t addr);
void arm_mem_store_half(arm_state_t *state, uint16_t val, uint32_t addr);
void arm_mem_store_word(arm_state_t *state, uint32_t val, uint32_t addr);

#ifdef __cplusplus
}
#endif

#endif
