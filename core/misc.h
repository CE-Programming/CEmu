#ifndef MISC_H
#define MISC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "port.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct watchdog_state {
    uint32_t count;
    uint32_t load;
    uint16_t restart;
    uint32_t control;
    uint32_t status;
    uint32_t length;
    uint32_t revision;
} watchdog_state_t;

typedef struct protected_state {
    bool locked;
    uint8_t led;
    uint8_t ports[0x100];
} protected_state_t;

typedef struct cxxx_state {
    uint8_t ports[0x100];
} cxxx_state_t;
typedef struct exxx_state {
    uint8_t ports[0x80];
} exxx_state_t;
typedef struct fxxx_state {
    uint8_t dummy;
} fxxx_state_t;

extern watchdog_state_t watchdog;
extern protected_state_t protect;
extern cxxx_state_t cxxx;
extern exxx_state_t exxx;
extern fxxx_state_t fxxx;

eZ80portrange_t init_watchdog(void);
eZ80portrange_t init_protected(void);
eZ80portrange_t init_cxxx(void);
eZ80portrange_t init_exxx(void);
eZ80portrange_t init_fxxx(void);
void watchdog_reset(void);
bool watchdog_restore(FILE *image);
bool watchdog_save(FILE *image);
bool protect_restore(FILE *image);
bool protect_save(FILE *image);
bool cxxx_restore(FILE *image);
bool cxxx_save(FILE *image);
bool exxx_restore(FILE *image);
bool exxx_save(FILE *image);

#ifdef __cplusplus
}
#endif

#endif
