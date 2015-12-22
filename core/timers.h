#ifndef TIMERS_H
#define TIMERS_H

#include <core/cpu.h>

#ifdef __cplusplus
extern "C" {
#endif

struct timer_state {
    uint32_t counter;
    uint32_t load;
    uint32_t match1;
    uint32_t match2;
};
typedef struct timer_state timer_state_t;

/* Standard GPT state */
struct general_timers_state {
    timer_state_t timer[3];
    uint32_t control;
    uint32_t interrupt_state;
    uint32_t interrupt_mask;    /* Probably unused? */
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
