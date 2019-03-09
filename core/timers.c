#include "timers.h"
#include "control.h"
#include "cpu.h"
#include "emu.h"
#include "schedule.h"
#include "interrupt.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Global GPT state */
general_timers_state_t gpt;

static void ost_event(enum sched_item_id id) {
    static const int ost_ticks[4] = { 73, 153, 217, 313 };
    intrpt_set(INT_OSTIMER, gpt.osTimerState);
    sched_repeat(id, gpt.osTimerState ? 1 : ost_ticks[control.ports[0] & 3]);
    gpt.osTimerState = !gpt.osTimerState;
}

static uint32_t gpt_peek_counter(int index) {
    enum sched_item_id id = SCHED_TIMER1 + index;
    timer_state_t *timer = &gpt.timer[index];
    uint32_t invert = (gpt.control >> (9 + index) & 1) ? ~0 : 0;
    if (!(gpt.control >> (index * 3) & 1) || !sched_active(id)) {
        return timer->counter;
    }
    if (gpt.reset & 1 << index) {
        gpt.reset &= ~(1 << index);
        return invert;
    }
    return timer->counter + ((sched_ticks_remaining(id) + invert) ^ invert);
}

static void gpt_restore_state(enum sched_item_id id) {
    int index = id - SCHED_TIMER1;
    gpt.timer[index].counter = gpt_peek_counter(index);
}

static uint64_t gpt_next_event(enum sched_item_id id) {
    int index = id - SCHED_TIMER1;
    struct sched_item *item = &sched.items[id];
    timer_state_t *timer = &gpt.timer[index];
    uint64_t delay;
    gpt.reset &= ~(1 << index);
    if (gpt.control >> (3*index) & 1) {
        int event;
        uint32_t counter = timer->counter, invert, status = 0, next;
        invert = (gpt.control >> (9 + index) & 1) ? ~0 : 0;
        for (event = 0; event <= 1; event++) {
            if (counter == timer->match[event]) {
                status |= 1 << event;
            }
        }
        if (counter == invert) {
            gpt.reset |= 1 << index;
            if (gpt.control >> (3*index) & 1 << 2) {
                status |= 1 << 2;
            }
            counter = timer->reset;
        } else {
            counter -= invert | 1;
        }
        if (status) {
            if (!sched_active(SCHED_TIMER_DELAY)) {
                sched_repeat_relative(SCHED_TIMER_DELAY, id, 0, 2);
            }
            delay = sched_cycle(SCHED_TIMER_DELAY) - sched_cycle(id);
            assert(delay <= 2);
            gpt.delayStatus |= status << (((2 - delay)*3 + index)*3);
            gpt.delayIntrpt |= 1 << (3*(4 - delay) + index);
        }
        item->clock = (gpt.control >> index*3 & 2) ? CLOCK_32K : CLOCK_CPU;
        if (gpt.reset & 1 << index) {
            next = 0;
            timer->counter = counter;
        } else {
            next = counter ^ invert;
            timer->counter = invert;
        }
        for (event = 1; event >= 0; event--) {
            uint32_t temp = (counter ^ invert) - (timer->match[event] ^ invert);
            if (temp < next) {
                next = temp;
                timer->counter = timer->match[event];
            }
        }
        return (uint64_t)next + 1;
    }
    return 0;
}

static void gpt_refresh(enum sched_item_id id) {
    uint64_t next_event;
    sched.items[id].clock = CLOCK_CPU;
    sched_set(id, 0); /* dummy activate to current cycle */
    next_event = gpt_next_event(id);
    if (next_event) {
        sched_set(id, next_event);
    } else {
        sched_clear(id);
    }
}

static void gpt_event(enum sched_item_id id) {
    uint64_t next_event;
    sched_repeat(id, 0); /* re-activate to event cycle */
    next_event = gpt_next_event(id);
    if (next_event) {
        sched_repeat(id, next_event);
    } else {
        sched_clear(id);
    }
}

static void gpt_delay(enum sched_item_id id) {
    int index;
    gpt.status |= gpt.delayStatus & ((1 << 9) - 1);
    for (index = 0; index < 3; index++) {
        intrpt_set(INT_TIMER1 << index, gpt.delayIntrpt & 1 << index);
    }
    gpt.delayStatus >>= 9;
    if (gpt.delayStatus || gpt.delayIntrpt) {
        sched_repeat(id, 1);
    }
    gpt.delayIntrpt >>= 3;
}

static void gpt_some(int which, void update(enum sched_item_id id)) {
    int index = which < 3 ? which : 0;
    do {
        update(SCHED_TIMER1 + index);
    } while (++index < which);
}

static uint8_t gpt_read(uint16_t address, bool peek) {
    uint8_t value = 0;
    (void)peek;

    if (address < 0x30 && (address & 0xC) == 0) {
        value = read8(gpt_peek_counter(address >> 4 & 3), (address & 3) << 3);
    } else if (address < 0x40) {
        value = ((uint8_t *)&gpt)[address];
    }
    return value;
}

static void gpt_write(uint16_t address, uint8_t value, bool poke) {
    int timer;
    bool counter_delay;
    (void)poke;

    if (address >= 0x34 && address < 0x38) {
        ((uint8_t *)&gpt)[address] &= ~value;
    } else if (address < 0x3C) {
        counter_delay = address < 0x30 && (address & 0xC) == 0;
        cpu.cycles += counter_delay;
        timer = address >> 4 & 3;
        gpt_some(timer, gpt_restore_state);
        ((uint8_t *)&gpt)[address] = value;
        gpt_some(timer, gpt_refresh);
        cpu.cycles -= counter_delay;
    }
}

void gpt_reset() {
    enum sched_item_id id;
    memset(&gpt, 0, sizeof(gpt));
    gpt.revision = 0x00010801;
    sched.items[SCHED_TIMER_DELAY].callback.event = gpt_delay;
    sched.items[SCHED_TIMER_DELAY].clock = CLOCK_CPU;
    for (id = SCHED_TIMER1; id <= SCHED_TIMER3; id++) {
        sched.items[id].callback.event = gpt_event;
        gpt_refresh(id);
    }
    sched.items[SCHED_OSTIMER].callback.event = ost_event;
    sched.items[SCHED_OSTIMER].clock = CLOCK_32K;
    sched_set(SCHED_OSTIMER, 0);
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
