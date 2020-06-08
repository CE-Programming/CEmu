#ifndef ARMSTATE_H
#define ARMSTATE_H

#include "armcpu.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct arm_state {
    arm_cpu_t cpu;
    uint32_t *flash, *ram;
} arm_state_t;

#ifdef __cplusplus
extern "C" {
#endif

bool arm_state_init(arm_state_t *state);
void arm_state_destroy(arm_state_t *state);
void arm_state_reset(arm_state_t *state);
void arm_state_load(arm_state_t *state, FILE *file);

#ifdef __cplusplus
}
#endif

#endif
