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

/* Standard GPT state */
typedef struct general_timers_state {
    timer_state_t timer[3];
    uint32_t control, status, mask, revision;
    uint8_t raw_status[3], padding[1];
    bool ost_state;
} general_timers_state_t;

/* Global GPT state */
extern general_timers_state_t gpt;

/* Available Functions */
eZ80portrange_t init_gpt(void);
void gpt_reset(void);

/* Save/Restore */
bool gpt_restore(FILE *image);
bool gpt_save(FILE *image);

#ifdef __cplusplus
}
#endif

#endif
