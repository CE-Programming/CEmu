#ifndef COPROC_H
#define COPROC_H

#include "arm/arm.h"

typedef struct coproc_state {
    arm_t *arm;
} coproc_state_t;

#ifdef __cplusplus
extern "C" {
#endif

extern coproc_state_t state;

void coproc_init(void);
void coproc_reset(void);
bool coproc_load(const char *path);

#ifdef __cplusplus
}
#endif

#endif
