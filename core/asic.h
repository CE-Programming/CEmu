#ifndef ASIC_H
#define ASIC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef enum {
    TI84PCE = 0,
    TI83PCE = 1
} ti_device_t;

typedef struct asic_state {
    ti_device_t deviceType;
} asic_state_t;

/* External Global ASIC state */
extern asic_state_t asic;

/* Available Functions */
void asic_init(void);
void asic_free(void);
void asic_reset(void);

void set_device_type(ti_device_t device);
ti_device_t get_device_type(void);

uint32_t set_cpu_clock_rate(uint32_t new_rate);

/* Save/Restore */
bool asic_restore(FILE *image);
bool asic_save(FILE *image);

#ifdef __cplusplus
}
#endif

#endif
