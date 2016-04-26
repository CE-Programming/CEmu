#ifndef TIMERS_H
#define TIMERS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "port.h"

typedef struct timer_state {
    uint32_t counter, reset, match[2];
} timer_state_t;

/* Standard GPT state */
PACK(typedef struct general_timers_state {
    timer_state_t timer[3];
    uint32_t control, status, mask, revision;
    uint8_t raw_status[3], padding[1];
}) general_timers_state_t;

/* Global GPT state */
extern general_timers_state_t gpt;

/* Available Functions */
eZ80portrange_t init_gpt(void);
void gpt_reset(void);

/* Save/Restore */
typedef struct emu_image emu_image;
bool gpt_restore(const emu_image*);
bool gpt_save(emu_image*);

#ifdef __cplusplus
}
#endif

#endif
