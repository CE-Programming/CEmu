#include "core/timers.h"
#include "core/controlport.h"
#include "core/emu.h"
#include "core/schedule.h"
#include "core/interrupt.h"
#include <string.h>

/* Global GPT state */
general_timers_state_t gpt;

static const int ost_ticks[4] = { 74, 154, 218, 314 };
static void ost_event(int index) {
    intrpt_trigger(INT_OSTMR, INTERRUPT_PULSE);
    event_repeat(index, ost_ticks[control.ports[0] & 3]);
}

static void gpt_update(void) {
    uint8_t timer;
    for (timer = 0; timer < 3; timer++) {
        if (gpt.control >> timer * 3 & 1) {
            sched.items[SCHED_TIMER1 + timer].clock = gpt.control >> timer * 3 & 2 ? CLOCK_32K : CLOCK_CPU;
            event_set(SCHED_TIMER1 + timer, 1);
        } else {
            sched.items[SCHED_TIMER1 + timer].second = -1;
        }
    }
}

static void gpt_event(int index) {
    int event;
    timer_state_t *timer;
    bool matched = false;
    index -= SCHED_TIMER1;
    if (gpt.control >> index * 3 & 1) {
        timer = &gpt.timer[index];
        if (gpt.control >> (9 + index) & 1) {
            timer->counter++;
        } else {
            timer->counter--;
        }
        for (event = 0; event < 2; event++) {
            if (timer->counter == timer->match[event]) {
                gpt.status |= 1 << (index * 3 + event);
                matched = true;
            }
        }
        if (!timer->counter) {
            gpt.status |= 1 << (index * 3 + 2);
            timer->counter = timer->reset;
            matched = true;
        }
        if (matched) {
            intrpt_trigger(INT_TIMER1 + index, INTERRUPT_SET);
        }
        event_repeat(SCHED_TIMER1 + index, 1);
    }
}

static uint8_t gpt_read(uint16_t address) {
    if (address < 0x40) {
        return ((uint8_t *)&gpt)[address];
    }
    return 0;
}

static void gpt_write(uint16_t address, uint8_t value) {
    uint8_t timer;
    if (address >= 0x34 && address < 0x38) {
        ((uint8_t *)&gpt)[address] &= ~value;
        for (timer = 0; timer < 3; timer++) {
            if (!(gpt.status >> timer * 3 & 0b111)) {
                intrpt_trigger(INT_TIMER1 + timer, INTERRUPT_CLEAR);
            }
        }
    } else if (address < 0x3C) {
        ((uint8_t *)&gpt)[address] = value;
        if (address >= 0x30) {
            gpt_update();
        }
    }
}

void gpt_reset() {
    uint8_t timer;
    memset(&gpt, 0, sizeof gpt - sizeof gpt.revision);
    gpt_update();
    for (timer = 0; timer < 3; timer++) {
        sched.items[SCHED_TIMER1 + timer].proc = gpt_event;
    }
    sched.items[SCHED_OSTIMER].clock = CLOCK_32K;
    sched.items[SCHED_OSTIMER].proc = ost_event;
    event_set(SCHED_OSTIMER, ost_ticks[control.ports[0] & 3]);
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
