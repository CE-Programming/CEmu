#include "timers.h"
#include "control.h"
#include "emu.h"
#include "schedule.h"
#include "interrupt.h"

#include <string.h>
#include <stdio.h>

/* Global GPT state */
general_timers_state_t gpt;

static void ost_event(enum sched_event event) {
    static const int ost_ticks[4] = { 37, 77, 109, 157 };
    intrpt_pulse(INT_OSTMR);
    event_repeat(event, ost_ticks[control.ports[0] & 3]);
}

static void gpt_restore_state(enum sched_event event) {
    int index = event - SCHED_TIMER1;
    timer_state_t *timer = &gpt.timer[index];
    uint32_t invert = (gpt.control >> (9 + index) & 1) ? ~0 : 0;
    if (gpt.control >> (index * 3) & 1 && sched.items[SCHED_TIMER1 + index].second >= 0) {
        timer->counter += (event_ticks_remaining(event) + invert) ^ invert;
    }
}

static uint64_t gpt_next_event(enum sched_event event) {
    int index = event - SCHED_TIMER1;
    struct sched_item *item = &sched.items[event];
    timer_state_t *timer = &gpt.timer[index];
    if (gpt.control >> (index * 3) & 1) {
        int32_t invert, event;
        uint32_t status = 0;
        uint64_t next;
        invert = (gpt.control >> (9 + index) & 1) ? ~0 : 0;
        if (!timer->counter) {
            timer->counter = timer->reset;
            if (gpt.control >> (index * 3) & 4) {
                status = 1 << 2;
            }
            if (!invert) {
                if (!timer->match[1]) {
                    status |= 1 << 1;
                }
                if (!timer->match[0]) {
                    status |= 1 << 0;
                }
            }
        }
        next = (uint64_t)(timer->counter ^ invert) - invert;
        for (event = 1; event >= 0; event--) {
            uint32_t temp = (timer->counter - timer->match[event] + invert) ^ invert;
            if (!temp) {
                status |= 1 << event;
            } else if (temp < next) {
                next = temp;
            }
        }
        gpt.status |= (status & ~gpt.raw_status[index]) << (index * 3);
        intrpt_set(INT_TIMER1 << index, status);
        gpt.raw_status[index] = next ? 0 : status;
        intrpt_set(INT_TIMER1 << index, gpt.raw_status[index]);
        timer->counter -= ((uint32_t)next + invert) ^ invert;
        item->clock = (gpt.control >> index*3 & 2) ? CLOCK_32K : CLOCK_CPU;
        return next;
    }
    gpt.raw_status[index] = 0;
    intrpt_set(INT_TIMER1 << index, 0);
    return 0;
}

static void gpt_refresh(enum sched_event event) {
    uint64_t next_event = gpt_next_event(event);
    if (next_event) {
        event_set(event, next_event);
    } else {
        event_clear(event);
    }
}

static void gpt_event(enum sched_event event) {
    uint64_t next_event = gpt_next_event(event);
    if (next_event) {
        event_repeat(event, next_event);
    } else {
        event_clear(event);
    }
}

static void gpt_some(int which, void update(enum sched_event event)) {
    int index = which < 3 ? which : 0;
    do {
        update(SCHED_TIMER1 + index);
    } while (++index < which);
}

static uint8_t gpt_read(uint16_t address, bool peek) {
    uint8_t value = 0;
    (void)peek;

    gpt_some(address >> 4 & 0b11, gpt_restore_state);
    if (address < 0x40) {
        value = ((uint8_t *)&gpt)[address];
    }
    gpt_some(address >> 4 & 0b11, gpt_refresh);
    return value;
}

static void gpt_write(uint16_t address, uint8_t value, bool poke) {
    int timer;
    (void)poke;

    if (address >= 0x34 && address < 0x38) {
        ((uint8_t *)&gpt)[address] &= ~value;
    } else if (address < 0x3C) {
        timer = address >> 4 & 0b11;
        gpt_some(timer, gpt_restore_state);
        ((uint8_t *)&gpt)[address] = value;
        gpt_some(timer, gpt_refresh);
    }
}

void gpt_reset() {
    enum sched_event event;
    memset(&gpt, 0, sizeof(gpt));
    gpt.revision = 0x00010801;
    for (event = SCHED_TIMER1; event <= SCHED_TIMER3; event++) {
        sched.items[event].proc = gpt_event;
        gpt_refresh(event);
    }
    sched.items[SCHED_OSTIMER].proc = ost_event;
    sched.items[SCHED_OSTIMER].clock = CLOCK_32K;
    event_set(SCHED_OSTIMER, 0);
    gui_console_printf("[CEmu] GPT reset.\n");
}

static const eZ80portrange_t device = {
    .read  = gpt_read,
    .write = gpt_write
};

eZ80portrange_t init_gpt(void) {
    gui_console_printf("[CEmu] Initialized General Purpose Timers...\n");
    return device;
}

bool gpt_save(FILE *image) {
    return fwrite(&gpt, sizeof(gpt), 1, image) == 1;
}

bool gpt_restore(FILE *image) {
    return fread(&gpt, sizeof(gpt), 1, image) == 1;
}
