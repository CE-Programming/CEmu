#ifndef SCHEDULE_H
#define SCHEDULE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

enum clock_id { CLOCK_CPU, CLOCK_APB, CLOCK_27M, CLOCK_12M, CLOCK_24M, CLOCK_32K,
                CLOCK_NUM_ITEMS };

enum sched_event {
    SCHED_THROTTLE,
    SCHED_KEYPAD,
    SCHED_LCD,
    SCHED_RTC,
    SCHED_OSTIMER,
    SCHED_TIMER1,
    SCHED_TIMER2,
    SCHED_TIMER3,
    SCHED_WATCHDOG,
    SCHED_NUM_EVENTS
};

struct sched_item {
    enum clock_id clock;
    int second; /* -1 = disabled */
    uint32_t tick;
    uint32_t cputick;
    void (*proc)(enum sched_event event);
};

typedef struct sched_state {
    struct sched_item items[SCHED_NUM_EVENTS];
    uint32_t clockRates[CLOCK_NUM_ITEMS];
    enum sched_event event;
    uint32_t next;
} sched_state_t;

/* Global SCHED state */
extern sched_state_t sched;

/* Available Functions */
void sched_reset(void);
void sched_process_pending_events(void);
void event_clear(enum sched_event event);
void event_set(enum sched_event event, uint64_t ticks);
void event_repeat(enum sched_event event, uint64_t ticks);
void sched_set_clocks(enum clock_id count, uint32_t *new_rates);
uint64_t event_next_cycle(enum sched_event event);
uint64_t event_ticks_remaining(enum sched_event event);

/* Save/Restore */
bool sched_restore(FILE *image);
bool sched_save(FILE *image);

#ifdef __cplusplus
}
#endif

#endif
