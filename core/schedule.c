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

#include <string.h>

#include "emu.h"
#include "schedule.h"

sched_state_t sched;

static uint32_t muldiv(uint32_t a, uint32_t b, uint32_t c) {
#if defined(__i386__) || defined(__x86_64__)
    asm ("mull %k1\n\tdivl %k2" : "+a" (a) : "g" (b), "g" (c) : "cc", "edx");
    return a;
#else
    uint64_t d = a;
    d *= b;
    return d / c;
#endif
}

void sched_reset(void) {
    const uint32_t def_rates[] = { 0, 0, 27000000, 12000000, 32768 };
    memcpy(sched.clock_rates, def_rates, sizeof(def_rates));
    memset(sched.items, 0, sizeof sched.items);
    sched.next_index = 0;
}

void event_repeat(int index, uint64_t ticks) {
    struct sched_item *item = &sched.items[index];

    ticks += item->tick;
    item->second = ticks / sched.clock_rates[item->clock];
    item->tick = ticks % sched.clock_rates[item->clock];

    item->cputick = muldiv(item->tick, sched.clock_rates[CLOCK_CPU], sched.clock_rates[item->clock]);
}

void sched_update_next_event(uint32_t cputick) {
    int i;
    sched.next_cputick = sched.clock_rates[CLOCK_CPU];
    sched.next_index = -1;

    for(i = 0; i < SCHED_NUM_ITEMS; i++) {
        struct sched_item *item = &sched.items[i];
        if (item->proc != NULL && item->second == 0 && item->cputick < sched.next_cputick) {
            sched.next_cputick = item->cputick;
            sched.next_index = i;
        }
    }
    /* printf("Next event: (%8d,%d)\n", next_cputick, next_index); */
    cycle_count_delta = cputick - sched.next_cputick;
}

uint32_t sched_process_pending_events(void) {
    uint32_t cputick = sched.next_cputick + cycle_count_delta;

    while (cputick >= sched.next_cputick) {
        if (sched.next_index < 0) {
            /* printf("[%8d] New second\n", cputick); */
            int i;
            for (i = 0; i < SCHED_NUM_ITEMS; i++) {
                if (sched.items[i].second >= 0) {
                    sched.items[i].second--;
                }
            }
            cputick -= sched.clock_rates[CLOCK_CPU];
        } else {
            /* printf("[%8d/%8d] Event %d\n", cputick, sched.next_cputick, sched.next_index); */
            sched.items[sched.next_index].second = -1;
            sched.items[sched.next_index].proc(sched.next_index);
        }
        sched_update_next_event(cputick);
    }
    return cputick;
}

void event_clear(int index) {
    uint32_t cputick = sched_process_pending_events();

    sched.items[index].second = -1;

    sched_update_next_event(cputick);
}

void event_set(int index, uint64_t ticks) {
    uint32_t cputick = sched_process_pending_events();

    struct sched_item *item = &sched.items[index];
    item->tick = muldiv(cputick, sched.clock_rates[item->clock], sched.clock_rates[CLOCK_CPU]);
    event_repeat(index, ticks);

    sched_update_next_event(cputick);
}

uint32_t event_ticks_remaining(int index) {
    uint32_t cputick = sched_process_pending_events();

    struct sched_item *item = &sched.items[index];
    return item->second * sched.clock_rates[item->clock]
        + item->tick - muldiv(cputick, sched.clock_rates[item->clock], sched.clock_rates[CLOCK_CPU]);
}

void sched_set_clocks(int count, uint32_t *new_rates) {
    uint32_t cputick = sched_process_pending_events();

    uint32_t remaining[SCHED_NUM_ITEMS];
    int i;
    for (i = 0; i < SCHED_NUM_ITEMS; i++) {
        struct sched_item *item = &sched.items[i];
        if (item->second >= 0) {
            remaining[i] = event_ticks_remaining(i);
        }
    }

    cputick = muldiv(cputick, new_rates[CLOCK_CPU], sched.clock_rates[CLOCK_CPU]);
    memcpy(sched.clock_rates, new_rates, sizeof(uint32_t) * count);

    for (i = 0; i < SCHED_NUM_ITEMS; i++) {
        struct sched_item *item = &sched.items[i];
        if (item->second >= 0) {
            item->tick = muldiv(cputick, sched.clock_rates[item->clock], sched.clock_rates[CLOCK_CPU]);
            event_repeat(i, remaining[i]);
        }
    }

    sched_update_next_event(cputick);
}
