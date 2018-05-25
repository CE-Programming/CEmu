#ifndef TIMERS_H
#define TIMERS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "port.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct timer_state {
    uint32_t counter, reset, match[2];
} timer_state_t;

typedef struct general_timers_state {
    timer_state_t timer[3];
    uint32_t control, status, mask, revision;
    uint32_t delayStatus, delayIntrpt;
    bool osTimerState;
} general_timers_state_t;

extern general_timers_state_t gpt;

eZ80portrange_t init_gpt(void);
void gpt_reset(void);
bool gpt_restore(FILE *image);
bool gpt_save(FILE *image);

#ifdef __cplusplus
}
#endif

#endif
