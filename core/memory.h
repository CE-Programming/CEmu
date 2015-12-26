/* Declarations for memory.c */

#ifndef MEMORY_H
#define MEMORY_H

#include <core/apb.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NO_COMMAND          0
#define FLASH_SECTOR_ERASE  1
#define FLASH_CHIP_ERASE    2

typedef struct {
    uint32_t address;
    uint32_t address_mask;
    uint8_t value;
    uint8_t value_mask;
} flash_write_t;

typedef struct {
    bool locked;
    uint8_t *ptr;
} flash_sector_state_t;

typedef struct {
    bool locked;
    uint8_t write_index;
    uint8_t read_index;
    flash_sector_state_t sector[64];
    uint8_t *block;     /* Flash mem */

    /* Internal */
    bool mapped;
    uint8_t command;
    flash_write_t writes[6];
} flash_chip_t;

typedef struct {
    uint8_t *block;       /* RAM mem */
} ram_chip_t;

struct mem_state {
    flash_chip_t flash;
    ram_chip_t ram;
};

/* Type definitions */
typedef struct mem_state mem_state_t;

/* Global MEMORY state */
extern mem_state_t mem;

/* Available Functions */
uint8_t *phys_mem_ptr(uint32_t address, uint32_t size);

typedef struct apb_map_entry apb_map_entry;

void mem_init(void);
void mem_free(void);

uint8_t memory_read_byte(uint32_t address);
void memory_write_byte(uint32_t address, uint8_t value);

#ifdef __cplusplus
}
#endif

#endif
