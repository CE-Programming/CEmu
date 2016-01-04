#ifndef MEM_H
#define MEM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "apb.h"
#include "debug/debug.h"

enum flash_commands {
    NO_COMMAND,
    FLASH_SECTOR_ERASE,
    FLASH_CHIP_ERASE,
    FLASH_READ_SECTOR_PROTECTION
};

typedef struct {
    uint32_t address;
    uint32_t address_mask;
    uint8_t value;
    uint8_t value_mask;
} flash_write_t;

/* The first 8 sectors are 8K in length */
/* The other 63 are 64K in length, uniform, for a total of 64 uniform sectors */
typedef struct {
    bool locked;
    uint8_t *ptr;
} flash_sector_state_t;

typedef struct {
    bool locked;
    uint8_t write_index;
    uint8_t read_index;
    flash_sector_state_t sector[8+63];
    uint8_t *block;     /* Flash mem */
    uint32_t size;

    /* Internal */
    bool mapped;
    uint8_t command;
    flash_write_t writes[6];
} flash_chip_t;

typedef struct {
    uint8_t *block;       /* RAM mem */
} ram_chip_t;

typedef struct mem_state {
    flash_chip_t flash;
    ram_chip_t ram;

    /* Debugging */
    debug_state_t debug;
} mem_state_t;

/* Global MEMORY state */
extern mem_state_t mem;

/* Available Functions */
void mem_init(void);
void mem_free(void);
void mem_reset(void);

uint8_t memory_read_byte(uint32_t address);
void memory_write_byte(uint32_t address, uint8_t value);

uint8_t *phys_mem_ptr(uint32_t address, uint32_t size);

#ifdef __cplusplus
}
#endif

#endif
