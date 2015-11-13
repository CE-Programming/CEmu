/* Declarations for memory.c */

#ifndef MEMORY_H
#define MEMORY_H

#include <core/apb.h>

#ifdef __cplusplus
extern "C" {
#endif

struct mem_state {
  uint8_t *flash;     // flash mem
  uint8_t *ram;       // ram mem
  uint8_t *vram;      // vram mem
  int flash_mapped;
  int flash_unlocked;
  int wait_state;
};

// Type definitions
typedef struct mem_state mem_state_t;

// Global MEMORY state
extern mem_state_t mem;

// Available Functions
uint8_t *phys_mem_ptr(uint32_t addr, uint32_t size);

int mem_wait_states();
int reset_mem_wait_states();

typedef struct apb_map_entry apb_map_entry;

void mem_init();
void mem_free();

uint8_t memory_read_byte(const uint32_t);
void memory_write_byte(const uint32_t, const uint8_t);

void apb_set_map(int, eZ80portrange_t*);

uint8_t mmio_read_byte(const uint32_t);
void mmio_write_byte(const uint32_t, const uint8_t);
#ifdef __cplusplus
}
#endif

#endif
