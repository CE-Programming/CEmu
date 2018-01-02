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
    asm ("mull %k1\n\tdivl %k2" : "+a" (a) : "rm" (b), "rm" (c) : "cc", "edx");
    return a;
#else
    return (uint64_t)a * b / c;
#endif
}

static void sched_update(enum sched_item_id id) {
    struct sched_item *item = &sched.items[id];
    if (id >= SCHED_FIRST_EVENT && id <= SCHED_LAST_EVENT) {
        if (item->callback.event && !item->second &&
            item->cputick < sched.items[sched.event.next].cputick) {
            sched.event.next = id;
            cpu_restore_next();
        }
    } else if (id >= SCHED_FIRST_DMA && id <= SCHED_LAST_DMA) {
        if (item->callback.dma && !item->second &&
            item->cputick < sched.items[sched.dma.next].cputick) {
            sched.dma.next = id;
        }
    }
}

static void sched_update_all(void) {
    enum sched_item_id id;
    sched.event.next = sched.dma.next = SCHED_SECOND;
    cpu_restore_next();
    for (id = 0; id < SCHED_NUM_ITEMS; id++) {
        sched_update(id);
    }
}

static void sched_schedule(enum sched_item_id id, int seconds, uint64_t ticks) {
    struct sched_item *item = &sched.items[id];
    item->second = seconds + ticks / sched.clockRates[item->clock];
    item->tick = ticks % sched.clockRates[item->clock];
    item->cputick = muldiv(item->tick, sched.clockRates[CLOCK_CPU], sched.clockRates[item->clock]);
    if (id == sched.event.next) {
        sched_update_all();
    } else {
        sched_update(id);
    }
}

void sched_repeat_relative(enum sched_item_id id, enum sched_item_id base_id, uint64_t ticks) {
    struct sched_item *item = &sched.items[id], *base = &sched.items[base_id];
    sched_schedule(id, base->second, muldiv(base->tick, sched.clockRates[item->clock], sched.clockRates[base->clock]) + ticks);
}

void sched_repeat(enum sched_item_id id, uint64_t ticks) {
    sched_schedule(id, 0, sched.items[id].tick + ticks);
}

void sched_set(enum sched_item_id id, uint64_t ticks) {
    sched_schedule(id, 0, cpu.cycles + ticks);
}

void sched_clear(enum sched_item_id id) {
    sched.items[id].second = -1;
    if (id == sched.event.next) {
        sched_update_all();
    }
}

static void sched_second(enum sched_item_id id) {
    struct sched_item *item;
    for (id = 0; id < SCHED_NUM_ITEMS; id++) {
        item = &sched.items[id];
        if (item->second >= 0) {
            item->second--;
        }
    }
    cpu.cycles -= sched.clockRates[CLOCK_CPU];
    cpu.baseCycles += sched.clockRates[CLOCK_CPU];
    sched.items[SCHED_SECOND].second = 0; // Don't use sched_repeat!
    sched_update_all();
}

void sched_set_clocks(enum clock_id count, uint32_t *new_rates) {
    enum sched_item_id id;
    struct sched_item *item;
    uint64_t ticks;

    cpu.baseCycles += cpu.cycles;
    cpu.cycles = muldiv(cpu.cycles, new_rates[CLOCK_CPU], sched.clockRates[CLOCK_CPU]);
    cpu.baseCycles -= cpu.cycles;
    for (id = 0; id < SCHED_NUM_ITEMS; id++) {
        item = &sched.items[id];
        if (item->second >= 0) {
            if (item->clock < count) {
                ticks = (uint64_t)item->second * sched.clockRates[item->clock] + item->tick;
                item->second = ticks / new_rates[item->clock];
                item->tick = ticks % new_rates[item->clock];
                item->cputick = muldiv(item->tick, new_rates[CLOCK_CPU], new_rates[item->clock]);
            } else {
                item->cputick = muldiv(item->tick, new_rates[CLOCK_CPU], sched.clockRates[item->clock]);
            }
            if (id == sched.event.next) {
                cpu_restore_next();
            }
        }
    }
    memcpy(sched.clockRates, new_rates, sizeof(uint32_t) * count);
}

void sched_process_pending_events(void) {
    while (cpu.cycles >= sched.items[sched.event.next].cputick) {
        enum sched_item_id id = sched.event.next;
        sched_clear(id);
        sched.items[id].callback.event(id);
    }
}

void sched_reset(void) {
    const uint32_t def_rates[CLOCK_NUM_ITEMS] = { 48000000, 48000000, 24000000, 12000000, 6000000, 32768, 1 };

    memcpy(sched.clockRates, def_rates, sizeof(def_rates));
    memset(sched.items, 0, sizeof sched.items);

    sched.event.next = sched.dma.next = SCHED_SECOND;
    cpu_restore_next();
    sched.items[SCHED_SECOND].callback.event = sched_second;
    sched.items[SCHED_SECOND].clock = CLOCK_1;
    sched.items[SCHED_SECOND].second = 0;
    sched.items[SCHED_SECOND].tick = 1;
    sched.items[SCHED_SECOND].cputick = sched.clockRates[CLOCK_CPU];
    sched.items[SCHED_THROTTLE].clock = CLOCK_6M;
    sched.items[SCHED_THROTTLE].callback.event = throttle_interval_event;
    sched_set(SCHED_THROTTLE, 0);
}

uint64_t event_next_cycle(enum sched_item_id id) {
    struct sched_item *item = &sched.items[id];
    return (uint64_t)item->second * sched.clockRates[CLOCK_CPU] + item->cputick - sched.items[sched.event.next].cputick + cpu.baseCycles;
}

uint64_t event_ticks_remaining(enum sched_item_id id) {
    struct sched_item *item = &sched.items[id];
    return (uint64_t)item->second * sched.clockRates[item->clock] + item->tick
        - muldiv(cpu.cycles, sched.clockRates[item->clock], sched.clockRates[CLOCK_CPU]);
}

bool sched_save(FILE *image) {
    return fwrite(&sched, sizeof(sched), 1, image) == 1;
}

bool sched_restore(FILE *image) {
    bool ret;
    enum sched_item_id id;
    union sched_callback callbacks[SCHED_NUM_ITEMS];

    for (id = 0; id < SCHED_NUM_ITEMS; id++) {
        callbacks[id] = sched.items[id].callback;
    }

    ret = fread(&sched, sizeof(sched), 1, image) == 1;

    for (id = 0; id < SCHED_NUM_ITEMS; id++) {
        sched.items[id].callback = callbacks[id];
    }

    return ret;
}

void dma_delay(uint8_t duration) {
    (void)duration;
    cpu.cycles += duration;
}
