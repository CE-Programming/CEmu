/* Copyright (C) 2015  Fabian Vogt
 * Modified for the CE calculator by CEmu developers
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
*/

#include "schedule.h"
#include "cpu.h"
#include "emu.h"
#include "debug/debug.h"

#include <string.h>
#include <stdio.h>

sched_state_t sched;

static uint32_t muldiv(uint32_t a, uint32_t b, uint32_t c) {
#if defined(__i386__) || defined(__x86_64__)
    asm ("mull %k1\n\tdivl %k2" : "+a" (a) : "g" (b), "g" (c) : "cc", "edx");
    return a;
#else
    return (uint64_t)a * b / c;
#endif
}

static void sched_set_next(uint32_t next) {
    sched.next = next;
    cpu_restore_next();
}

void sched_reset(void) {
    enum sched_event event;
    const uint32_t def_rates[CLOCK_NUM_ITEMS] = { 48000000, 78000000, 27000000, 12000000, 24000000, 32768 };

    memcpy(sched.clockRates, def_rates, sizeof(def_rates));
    memset(sched.items, 0, sizeof sched.items);

    for (event = 0; event < SCHED_NUM_EVENTS; event++) {
        sched.items[event].second = -1;
    }

    sched.event = SCHED_NUM_EVENTS;
    sched_set_next(sched.clockRates[CLOCK_CPU]);
    sched.items[SCHED_THROTTLE].clock = CLOCK_27M;
    sched.items[SCHED_THROTTLE].proc = throttle_interval_event;
    event_set(SCHED_THROTTLE, 0);
}

static void sched_update_event(enum sched_event event) {
    struct sched_item *item = &sched.items[event];
    if (item->proc && !item->second && item->cputick < sched.next) {
        sched.event = event;
        sched_set_next(item->cputick);
    }
}

static void sched_update_events(void) {
    enum sched_event event;
    sched.event = SCHED_NUM_EVENTS;
    sched_set_next(sched.clockRates[CLOCK_CPU]);
    for (event = 0; event < SCHED_NUM_EVENTS; event++) {
        sched_update_event(event);
    }
}

void event_repeat(enum sched_event event, uint64_t ticks) {
    struct sched_item *item = &sched.items[event];

    ticks += item->tick;
    item->second = ticks / sched.clockRates[item->clock];
    item->tick = ticks % sched.clockRates[item->clock];
    item->cputick = muldiv(item->tick, sched.clockRates[CLOCK_CPU], sched.clockRates[item->clock]);

    if (event == sched.event) {
        sched_update_events();
    } else {
        sched_update_event(event);
    }
}

void sched_process_pending_events(void) {
    enum sched_event event;
    while (cpu.cycles >= sched.next) {
        event = sched.event;
        if (event == SCHED_NUM_EVENTS) {
            for (event = 0; event < SCHED_NUM_EVENTS; event++) {
                if (sched.items[event].second >= 0) {
                    sched.items[event].second--;
                }
            }
            cpu.cycles -= sched.clockRates[CLOCK_CPU];
            cpu.baseCycles += sched.clockRates[CLOCK_CPU];
            sched_update_events();
        } else {
            event_clear(event);
            sched.items[event].proc(event);
        }
    }
}

void event_clear(enum sched_event event) {
    sched.items[event].second = -1;
    if (event == sched.event) {
        sched_update_events();
    }
}

void event_set(enum sched_event event, uint64_t ticks) {
    struct sched_item *item = &sched.items[event];
    item->tick = muldiv(cpu.cycles, sched.clockRates[item->clock], sched.clockRates[CLOCK_CPU]);
    event_repeat(event, ticks);
}

uint64_t event_next_cycle(enum sched_event event) {
    struct sched_item *item = &sched.items[event];
    return (uint64_t)item->second * sched.clockRates[CLOCK_CPU] + item->cputick - sched.next + cpu.baseCycles;
}

uint64_t event_ticks_remaining(enum sched_event event) {
    struct sched_item *item = &sched.items[event];
    return (uint64_t)item->second * sched.clockRates[item->clock] + item->tick
        - muldiv(cpu.cycles, sched.clockRates[item->clock], sched.clockRates[CLOCK_CPU]);
}

void sched_set_clocks(enum clock_id count, uint32_t *new_rates) {
    enum sched_event event;

    cpu.baseCycles += cpu.cycles;
    cpu.cycles = muldiv(cpu.cycles, new_rates[CLOCK_CPU], sched.clockRates[CLOCK_CPU]);
    cpu.baseCycles -= cpu.cycles;
    for (event = 0; event < SCHED_NUM_EVENTS; event++) {
        struct sched_item *item = &sched.items[event];
        if (item->second >= 0) {
            if (item->clock < count) {
                uint64_t ticks = (uint64_t)item->second * sched.clockRates[item->clock] + item->tick;
                item->second = ticks / new_rates[item->clock];
                item->tick = ticks % new_rates[item->clock];
            }
            item->cputick = muldiv(item->tick, new_rates[CLOCK_CPU], sched.clockRates[item->clock]);
            if (event == sched.event) {
                sched_set_next(item->cputick);
            }
        }
    }
    if (sched.event == SCHED_NUM_EVENTS) {
        sched_set_next(new_rates[CLOCK_CPU]);
    }
    cpu_restore_next();
    memcpy(sched.clockRates, new_rates, sizeof(uint32_t) * count);
}

bool sched_save(FILE *image) {
    return fwrite(&sched, sizeof(sched), 1, image) == 1;
}

bool sched_restore(FILE *image) {
    bool ret;
    enum sched_event event;
    void (*procs[SCHED_NUM_EVENTS])(enum sched_event event);

    for (event = 0; event < SCHED_NUM_EVENTS; event++) {
        procs[event] = sched.items[event].proc;
    }

    ret = fread(&sched, sizeof(sched), 1, image) == 1;

    for (event = 0; event < SCHED_NUM_EVENTS; event++) {
        sched.items[event].proc = procs[event];
    }

    return ret;
}
