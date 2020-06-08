#include "coproc.h"

coproc_state_t coproc;

void coproc_init(void) {
}

void coproc_reset(void) {
    arm_destroy(coproc.arm);
    coproc.arm = arm_create();
}

bool coproc_load(const char *path) {
    return arm_load(coproc.arm, path);
}
