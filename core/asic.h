#ifndef ASIC_H
#define ASIC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/* Note that this does not map a cert field */
typedef enum {
    TI84PCE = 0,
    TI83PCE = 1,
    TI82AEP = 2
} emu_device_t;

typedef enum {
    ASIC_REV_AUTO = 0, /* Used only with set_asic_revision() */
    ASIC_REV_A = 1,
    ASIC_REV_I = 2,
    ASIC_REV_M = 3
} asic_rev_t;

typedef struct asic_state {
    emu_device_t device;
    /* Only updated on reset */
    asic_rev_t revision;
    bool python;
    /* Populated based on revision */
    bool im2;
    bool serFlash;
    bool hasWorkingSPIReads;
} asic_state_t;

extern asic_state_t asic;

void asic_init(void);
void asic_free(void);
void asic_reset(void);
bool asic_restore(FILE *image);
bool asic_save(FILE *image);
void set_cpu_clock(uint32_t new_rate);
void set_device_type(emu_device_t device);
emu_device_t get_device_type(void);
asic_rev_t get_asic_revision(void);
bool get_asic_python(void);

#ifdef __cplusplus
}
#endif

#endif
