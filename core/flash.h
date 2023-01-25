#ifndef FLASH_H
#define FLASH_H

#ifdef __cplusplus
extern "C" {
#endif

#include "port.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#define FLASH_CACHE_LINE_BITS 5
#define FLASH_CACHE_LINE_SIZE (1 << FLASH_CACHE_LINE_BITS)
#define FLASH_CACHE_SET_BITS 7
#define FLASH_CACHE_SETS (1 << FLASH_CACHE_SET_BITS)

#define FLASH_CACHE_INVALID_LINE 0xFFFFFFFF
#define FLASH_CACHE_INVALID_TAG 0xFFFF

typedef struct flash_cache_set {
    uint16_t mru;
    uint16_t lru;
} flash_cache_set_t;

typedef struct flash_state {
    uint64_t uniqueID;
    uint32_t waitStates;
    uint32_t mappedBytes;
    uint32_t mask;
    uint32_t lastCacheLine;
    uint32_t commandAddress;
    uint32_t commandLength;
    uint8_t command[0x10];
    uint8_t commandStatus[4];
    uint8_t maskReg[4];
    flash_cache_set_t cacheTags[FLASH_CACHE_SETS];
    uint8_t ports[0x100];
} flash_state_t;

extern flash_state_t flash;

void flash_flush_cache(void);
uint32_t flash_touch_cache(uint32_t addr);

eZ80portrange_t init_flash(void);
void flash_reset(void);
bool flash_restore(FILE *image);
bool flash_save(FILE *image);

#ifdef __cplusplus
}
#endif

#endif
