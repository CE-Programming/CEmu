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
    ti_device_t device;
    bool preI;
    bool revM;
} asic_state_t;

extern asic_state_t asic;

void asic_init(void);
void asic_free(void);
void asic_reset(void);
bool asic_restore(FILE *image);
bool asic_save(FILE *image);
void set_cpu_clock(uint32_t new_rate);
void set_device_type(ti_device_t device);
ti_device_t get_device_type(void);

#ifdef __cplusplus
}
#endif

#endif
