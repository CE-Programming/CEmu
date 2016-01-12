#ifndef TIMERS_H
#define TIMERS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "apb.h"

typedef struct timer_state {
    uint32_t counter, reset, match[2];
} timer_state_t;

/* Standard GPT state */
typedef struct general_timers_state {
    timer_state_t timer[3];
    uint32_t control, status, mask, revision;
} general_timers_state_t;

/* Global GPT state */
extern general_timers_state_t gpt;

/* Available Functions */
eZ80portrange_t init_gpt(void);
void gpt_reset(void);

#ifdef __cplusplus
}
#endif

#endif
