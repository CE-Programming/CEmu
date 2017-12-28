#ifndef MEM_H
#define MEM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

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
    uint32_t addr;
    uint32_t addr_mask;
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
    uint8_t write_index;
    uint8_t read_index;
    flash_sector_state_t sector_8k[8];
    flash_sector_state_t sector[64];
    uint8_t *block;       /* Flash mem */

    /* Internal */
    uint8_t command;
    flash_write_t writes[6];
} flash_chip_t;

typedef struct {
    uint8_t *block;       /* RAM mem */
} ram_chip_t;

typedef struct mem_state {
    flash_chip_t flash;
    ram_chip_t ram;
    uint8_t fetch_index : 4, fetch_buffer[1 << 4];
} mem_state_t;

/* Global MEMORY state */
extern mem_state_t mem;

/* Standard definitions */
#define SIZE_RAM              0x65800
#define SIZE_FLASH            0x400000
#define SIZE_FLASH_SECTOR_8K  0x2000
#define SIZE_FLASH_SECTOR_64K 0x10000

/* Available Functions */
void mem_init(void);
void mem_free(void);
void mem_reset(void);

uint8_t *phys_mem_ptr(uint32_t addr, int32_t size);
uint8_t *virt_mem_cpy(uint8_t *buf, uint32_t addr, int32_t size);
uint8_t *virt_mem_dup(uint32_t addr, int32_t size);
uint8_t mem_peek_byte(uint32_t addr);
uint16_t mem_peek_short(uint32_t addr);
uint32_t mem_peek_long(uint32_t addr);
uint32_t mem_peek_word(uint32_t addr, bool mode);
void mem_poke_byte(uint32_t addr, uint8_t value);

/* Mateo, do not use! Use the ones above. */
uint8_t mem_read_cpu(uint32_t address, bool fetch);
void mem_write_cpu(uint32_t address, uint8_t value);

/* Save/Restore */
bool mem_restore(FILE *image);
bool mem_save(FILE *image);

#ifdef __cplusplus
}
#endif

#endif
