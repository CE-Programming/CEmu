#ifndef TIMERS_H
#define TIMERS_H

#include <core/cpu.h>

#ifdef __cplusplus
extern "C" {
#endif

struct timer_state {
    uint32_t count;
    uint32_t reset_value;
    uint32_t match_1;
    uint32_t match_2;
};
typedef struct timer_state timer_state_t;

/* Standard GPT state */
struct general_timers_state {
    timer_state_t timer[3];
    uint32_t control      : 12;
    uint32_t intrpt       : 12;
    uint32_t intrpt_mask  : 12; /* Probably unused? */
};

/* Type definitions */
typedef struct general_timers_state general_timers_state_t;

/* Global GPT state */
extern general_timers_state_t gpt;

/* Available Functions */
eZ80portrange_t init_gpt(void);
void gpt_reset(void);

#ifdef __cplusplus
}
#endif

#endif
