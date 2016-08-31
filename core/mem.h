#ifndef MEM_H
#define MEM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#include "defines.h"

enum flash_commands {
    NO_COMMAND,
    FLASH_SECTOR_ERASE,
    FLASH_CHIP_ERASE,
    FLASH_READ_SECTOR_PROTECTION,
    FLASH_DEEP_POWER_DOWN,
    FLASH_READ_CFI,
    FLASH_IPB_MODE
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
    uint8_t command;
    flash_write_t writes[6];
} flash_chip_t;

typedef struct {
    uint8_t *block;       /* RAM mem */
} ram_chip_t;

PACK(typedef struct mem_state {
    flash_chip_t flash;
    ram_chip_t ram;
}) mem_state_t;

/* Global MEMORY state */
extern mem_state_t mem;

/* Standard definitions */
#define ram_size   0x65800
#define flash_size 0x400000

static const uint32_t flash_sector_size_8K = 0x2000;
static const uint32_t flash_sector_size_64K = 0x10000;

/* Available Functions */
void mem_init(void);
void mem_free(void);
void mem_reset(void);

uint8_t *phys_mem_ptr(uint32_t address, int32_t size);
uint8_t mem_peek_byte(uint32_t address);
uint16_t mem_peek_short(uint32_t address);
uint32_t mem_peek_long(uint32_t address);
uint32_t mem_peek_word(uint32_t address, bool mode);
void mem_poke_byte(uint32_t address, uint8_t value);

/* Mateo, do not use! Use the ones above. */
uint8_t mem_read_cpu(uint32_t address, bool fetch);
void mem_write_cpu(uint32_t address, uint8_t value);

/* Save/Restore */
typedef struct emu_image emu_image;
bool mem_restore(const emu_image*);
bool mem_save(emu_image*);

#ifdef __cplusplus
}
#endif

#endif
