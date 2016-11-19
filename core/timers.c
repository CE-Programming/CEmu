#include <string.h>

#include "timers.h"
#include "control.h"
#include "emu.h"
#include "schedule.h"
#include "interrupt.h"

/* Global GPT state */
general_timers_state_t gpt;

static const int ost_ticks[4] = { 37, 77, 109, 157 };
static void ost_event(int index) {
    intrpt_pulse(INT_OSTMR);
    event_repeat(index, ost_ticks[control.ports[0] & 3]);
}

static void gpt_restore_state(int index) {
    timer_state_t *timer = &gpt.timer[index -= SCHED_TIMER1];
    uint32_t invert = (gpt.control >> (9 + index) & 1) ? ~0 : 0;
    if (gpt.control >> index * 3 & 1 && sched.items[SCHED_TIMER1 + index].second >= 0) {
        timer->counter += (event_ticks_remaining(SCHED_TIMER1 + index) + invert) ^ invert;
    }
}

static uint64_t gpt_next_event(int index) {
    struct sched_item *item = &sched.items[index];
    timer_state_t *timer = &gpt.timer[index -= SCHED_TIMER1];
    if (gpt.control >> index * 3 & 1) {
        int32_t invert, event;
        uint32_t status = 0;
        uint64_t next;
        invert = (gpt.control >> (9 + index) & 1) ? ~0 : 0;
        if (!timer->counter) {
            timer->counter = timer->reset;
            if (gpt.control >> index * 3 & 4) {
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
        gpt.status |= (status & ~gpt.raw_status[index]) << index * 3;
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

static void gpt_refresh(int index) {
    uint64_t next_event = gpt_next_event(index);
    if (next_event) {
        event_set(index, next_event);
    } else {
        event_clear(index);
    }
}

static void gpt_event(int index) {
    uint64_t next_event = gpt_next_event(index);
    if (next_event) {
        event_repeat(index, next_event);
    } else {
        event_clear(index);
    }
}

static void gpt_some(int which, void update(int index)) {
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

static void gpt_write(uint16_t address, uint8_t value, bool peek) {
    int timer;
    (void)peek;

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
    int timer;
    memset(&gpt, 0, sizeof(gpt));
    gpt.revision = 0x00010801;
    for(timer = SCHED_TIMER1; timer <= SCHED_TIMER3; timer++) {
        gpt_refresh(timer);
        sched.items[timer].proc = gpt_event;
    }
    sched.items[SCHED_OSTIMER].clock = CLOCK_32K;
    sched.items[SCHED_OSTIMER].proc = ost_event;
    event_set(SCHED_OSTIMER, ost_ticks[control.ports[0] & 3]);
    gui_console_printf("[CEmu] GPT reset.\n");
}

static const eZ80portrange_t device = {
    .read_in    = gpt_read,
    .write_out  = gpt_write
};

eZ80portrange_t init_gpt(void) {
    gui_console_printf("[CEmu] Initialized GP timers...\n");
    return device;
}

bool gpt_save(emu_image *s) {
    s->gpt = gpt;
    return true;
}

bool gpt_restore(const emu_image *s) {
    gpt = s->gpt;
    return true;
}
