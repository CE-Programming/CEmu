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
                delay = 2;
            } else {
                delay = sched_ticks_remaining_relative(SCHED_TIMER_DELAY, id, 0);
            }
            assert(delay <= 2);
            gpt.delayStatus |= status << (((2 - delay)*3 + index)*3);
            gpt.delayIntrpt |= 1 << (3*(4 - delay) + index);
        }
        sched_set_item_clock(id, (gpt.control >> index*3 & 2) ? CLOCK_32K : CLOCK_CPU);
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
    sched_set_item_clock(id, CLOCK_CPU);
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
    uint8_t index = address >> 2;
    uint8_t bit_offset = (address & 3) << 3;
    (void)peek;

    if (index < 0xC && (index & 3) == 0) {
        value = read8(gpt_peek_counter(index >> 2), bit_offset);
    } else if (address < 0x40) {
        value = read8(gpt.regs[index], bit_offset);
    }
    return value;
}

static void gpt_write(uint16_t address, uint8_t value, bool poke) {
    int timer;
    bool counter_delay;
    uint8_t index = address >> 2;
    uint8_t bit_offset = (address & 3) << 3;
    (void)poke;

    if (index == 0xD) {
        uint32_t mask = ~((uint32_t)value << bit_offset & 0x1FF);
        gpt.status &= mask;
        if (sched_active(SCHED_TIMER_DELAY) && sched_ticks_remaining(SCHED_TIMER_DELAY) == 1) {
            gpt.delayStatus &= mask;
        }
    } else if (index < 0xF) {
        counter_delay = index < 0xC && (index & 3) == 0;
        cpu.cycles += counter_delay;
        timer = index >> 2;
        gpt_some(timer, gpt_restore_state);
        write8(gpt.regs[index], bit_offset, value);
        gpt_some(timer, gpt_refresh);
        cpu.cycles -= counter_delay;
    }
}

static void gpt_init_events(void) {
    enum sched_item_id id;
    sched_init_event(SCHED_TIMER_DELAY, CLOCK_CPU, gpt_delay);
    for (id = SCHED_TIMER1; id <= SCHED_TIMER3; id++) {
        sched_init_event(id, CLOCK_CPU, gpt_event);
    }
    sched_init_event(SCHED_OSTIMER, CLOCK_32K, ost_event);
}

void gpt_reset() {
    enum sched_item_id id;
    memset(&gpt, 0, sizeof(gpt));
    gpt.revision = 0x00010801;

    gpt_init_events();
    for (id = SCHED_TIMER1; id <= SCHED_TIMER3; id++) {
        gpt_refresh(id);
    }
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
    gpt_init_events();
    return fread(&gpt, sizeof(gpt), 1, image) == 1;
}
