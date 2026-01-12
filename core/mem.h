#ifndef MEM_H
#define MEM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define SIZE_RAM              0x65800
#define SIZE_FLASH_MIN        0x400000
#define SIZE_FLASH_MAX_PAR    0x800000
#define SIZE_FLASH_MAX        0x2000000
#define SIZE_FLASH_SECTOR_8K  0x2000
#define SIZE_FLASH_SECTOR_64K 0x10000
#define NUM_SECTORS_MAX (SIZE_FLASH_MAX_PAR / SIZE_FLASH_SECTOR_64K)
#define NUM_8K_SECTORS 8

enum flash_commands {
    FLASH_NO_COMMAND,
    FLASH_SECTOR_ERASE,
    FLASH_CHIP_ERASE,
    FLASH_READ_SECTOR_PROTECTION,
    FLASH_DEEP_POWER_DOWN,
    FLASH_READ_CFI,
    FLASH_IPB_MODE,
    FLASH_DPB_MODE,
    FLASH_WAIT_PB_EXIT,
};

typedef struct {
    uint32_t addr;
    uint32_t addrMask;
    uint8_t value;
    uint8_t valueMask;
} flash_write_t;

typedef struct {
    uint8_t dpb : 1;
    uint8_t ipb : 1;
} flash_sector_state_t;

typedef struct {
    uint8_t write;
    uint8_t read;
    flash_sector_state_t sector8k[NUM_8K_SECTORS];
    flash_sector_state_t sector[NUM_SECTORS_MAX];
    uint8_t *block;
    uint32_t size;

    /* internal */
    uint8_t command;
    flash_write_t writes[6];
} flash_chip_t;

typedef struct {
    uint8_t *block;
} ram_chip_t;

typedef struct mem_state {
    flash_chip_t flash;
    ram_chip_t ram;
    uint8_t fetch : 4, buffer[1 << 4];
} mem_state_t;

extern mem_state_t mem;

void mem_init(void);
void mem_free(void);
void mem_reset(void);
bool mem_restore(FILE *image);
bool mem_save(FILE *image);

void *phys_mem_ptr(uint32_t addr, int32_t size);
void *virt_mem_cpy(void *buf, uint32_t addr, int32_t size);
void *virt_mem_dup(uint32_t addr, int32_t size);
void *mem_dma_read(void *buf, uint32_t addr, int32_t size);
void mem_dma_write(const void *buf, uint32_t addr, int32_t size);
uint8_t mem_peek_byte(uint32_t addr);
uint16_t mem_peek_short(uint32_t addr);
uint32_t mem_peek_long(uint32_t addr);
uint32_t mem_peek_word(uint32_t addr, bool mode);
void mem_poke_byte(uint32_t addr, uint8_t value);
void mem_poke_short(uint32_t addr, uint16_t value);
void mem_poke_long(uint32_t addr, uint32_t value);
void mem_poke_word(uint32_t addr, uint32_t value, bool mode);
uint8_t mem_read_unmapped_ram(bool update);
uint8_t mem_read_unmapped_flash(bool update);
uint8_t mem_read_unmapped_other(bool update);

/* Don NOT use (Mateo)! Use the above ones. */
uint8_t mem_read_cpu(uint32_t address, bool fetch);
void mem_write_cpu(uint32_t address, uint8_t value);

#ifdef __cplusplus
}
#endif

#endif
