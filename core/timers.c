#include <string.h>

#include "timers.h"
#include "control.h"
#include "emu.h"
#include "schedule.h"
#include "interrupt.h"

/* Global GPT state */
general_timers_state_t gpt;

static const int ost_ticks[4] = { 74, 154, 218, 314 };
static void ost_event(int index) {
    intrpt_trigger(INT_OSTMR, INTERRUPT_PULSE);
    event_repeat(index, ost_ticks[control.ports[0] & 3]);
}

static void gpt_restore(int index) {
    timer_state_t *timer = &gpt.timer[index -= SCHED_TIMER1];
    uint32_t invert = gpt.control >> (9 + index) & 1 ? ~0 : 0;
    if (gpt.control >> index * 3 & 1) {
        timer->counter += (event_ticks_remaining(SCHED_TIMER1 + index) + invert) ^ invert;
    }
}

static uint64_t gpt_next_event(int index) {
    struct sched_item *item = &sched.items[index];
    timer_state_t *timer = &gpt.timer[index -= SCHED_TIMER1];
    uint32_t invert, event, temp = 0, status = 0, next = ~0;
    if (gpt.control >> index * 3 & 1) {
        invert = gpt.control >> (9 + index) & 1 ? ~0 : 0;
        if (!timer->counter) {
            timer->counter = timer->reset;
            status = 1 << 2;
        }
        for (event = 2; event < 3; event--) {
            if (event < 2) {
                temp = timer->match[event];
            }
            temp = (timer->counter - temp + invert) ^ invert;
            if (!temp) {
                status |= 1 << event;
            }
            if (--temp < next) {
                next = temp;
            }
        }
        if (status) {
            intrpt_trigger(INT_TIMER1 + index, INTERRUPT_PULSE);
        }
        gpt.status |= status << index * 3;
        timer->counter -= (next - ~invert) ^ invert;
        item->clock = gpt.control >> index * 3 & 2 ? CLOCK_32K : CLOCK_CPU;
        return (uint64_t)next + 1;
    }
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

static void gpt_update(int index) {
    gpt_restore(index);
    gpt_refresh(index);
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

static uint8_t gpt_read(uint16_t address) {
    uint8_t value = 0;
    gpt_some(address >> 4 & 0b11, gpt_update);
    if (address < 0x40) {
        value = ((uint8_t *)&gpt)[address];
    }
    return value;
}

static void gpt_write(uint16_t address, uint8_t value) {
    int timer;
    if (address >= 0x34 && address < 0x38) {
        ((uint8_t *)&gpt)[address] &= ~value;
        for (timer = 0; timer < 3; timer++) {
            if (!(gpt.status >> timer * 3 & 0b111)) {
                intrpt_trigger(INT_TIMER1 + timer, INTERRUPT_CLEAR);
            }
        }
    } else if (address < 0x3C) {
        timer = address >> 4 & 0b11;
        gpt_some(timer, gpt_restore);
        ((uint8_t *)&gpt)[address] = value;
        gpt_some(timer, gpt_refresh);
    }
}

void gpt_reset() {
    int timer;
    memset(&gpt, 0, sizeof gpt - sizeof gpt.revision);
    for (timer = SCHED_TIMER1; timer <= SCHED_TIMER3; timer++) {
        gpt_refresh(timer);
        sched.items[timer].proc = gpt_event;
    }
    sched.items[SCHED_OSTIMER].clock = CLOCK_32K;
    sched.items[SCHED_OSTIMER].proc = ost_event;
    event_set(SCHED_OSTIMER, ost_ticks[control.ports[0] & 3]);
    gui_console_printf("GPT reset.\n");
}

static const eZ80portrange_t device = {
    .read_in    = gpt_read,
    .write_out  = gpt_write
};

eZ80portrange_t init_gpt(void) {
    gpt.revision = 0x00010801;
    gui_console_printf("Initialized general purpose timers...\n");
    return device;
}
