#ifndef FLASH_H
#define FLASH_H

#ifdef __cplusplus
extern "C" {
#endif

#include "port.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct flash_state {
    uint8_t ports[0x100];
    uint8_t command[0x10];
    uint64_t uniqueID;
    uint8_t commandStatus[4];
    uint32_t commandAddress;
    uint32_t commandLength;
    uint32_t waitStates;
    uint32_t mask;
    uint8_t mapped : 1;
    uint8_t map    : 4;
} flash_state_t;

extern flash_state_t flash;

eZ80portrange_t init_flash(void);
bool flash_restore(FILE *image);
bool flash_save(FILE *image);

#ifdef __cplusplus
}
#endif

#endif
