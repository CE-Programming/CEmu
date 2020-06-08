#include "armstate.h"

#include "armcpu.h"
#include "armmem.h"

bool arm_state_init(arm_state_t *state) {
    return arm_mem_init(state);
}

void arm_state_destroy(arm_state_t *state) {
    arm_mem_destroy(state);
}

void arm_state_reset(arm_state_t *state) {
    arm_mem_reset(state);
    arm_cpu_reset(state);
}

void arm_state_load(arm_state_t *state, FILE *file) {
    arm_mem_load_rom(state, file);
}
