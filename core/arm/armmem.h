#ifndef ARMMEM_H
#define ARMMEM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct arm_mem_state {
    uint32_t *flash, *ram;
} arm_mem_state_t;

extern arm_mem_state_t arm_mem;

void arm_mem_init(void);
uint8_t arm_mem_load_byte(uint32_t addr);
uint16_t arm_mem_load_half(uint32_t addr);
uint32_t arm_mem_load_word(uint32_t addr);
void arm_mem_store_byte(uint8_t val, uint32_t addr);
void arm_mem_store_half(uint16_t val, uint32_t addr);
void arm_mem_store_word(uint32_t val, uint32_t addr);

#ifdef __cplusplus
}
#endif

#endif
