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

#include <assert.h>
#include <string.h>
#include <stdio.h>

sched_state_t sched;

static uint32_t muldiv_floor(uint32_t a, uint32_t b, uint32_t c) {
    return (uint64_t)a * b / c;
}
static uint32_t muldiv_ceil(uint32_t a, uint32_t b, uint32_t c) {
    return ((uint64_t)a * b + c - 1) / c;
}

uint32_t sched_event_next_cycle(void) {
    enum sched_item_id next = sched.event.next;
    assert(next >= SCHED_SECOND && next <= SCHED_LAST_EVENT);
    assert(!sched.items[next].second || next == SCHED_SECOND);
    return sched.event.cycle ? sched.event.cycle : sched.items[next].cycle;
}

static bool sched_before(enum sched_item_id a_id, enum sched_item_id b_id) {
    struct sched_item *a = &sched.items[a_id], *b = &sched.items[b_id];
    return a->second <= b->second && a->cycle < b->cycle;
}

static void sched_update(enum sched_item_id id) {
    struct sched_item *item = &sched.items[id];
    if (id == SCHED_SECOND) {
        sched.event.next = id;
        cpu_restore_next();
        for (id = SCHED_FIRST_EVENT; id <= SCHED_LAST_EVENT; id++) {
            sched_update(id);
        }
    } else if (id >= SCHED_FIRST_EVENT && id <= SCHED_LAST_EVENT) {
        if (item->callback.event && !item->second &&
            item->cycle < sched_event_next_cycle()) {
            sched.event.next = id;
            cpu_restore_next();
        }
    } else if (id == SCHED_PREV_MA) {
        sched.dma.next = id;
        for (id = SCHED_FIRST_DMA; id <= SCHED_LAST_DMA; id++) {
            sched_update(id);
        }
    } else if (id >= SCHED_FIRST_DMA && id <= SCHED_LAST_DMA) {
        if (item->callback.dma && item->second >= 0 &&
            (sched.dma.next == SCHED_PREV_MA || sched_before(id, sched.dma.next))) {
            sched.dma.next = id;
        }
    }
}

static void sched_schedule(enum sched_item_id id, int32_t seconds, uint64_t ticks) {
    struct sched_item *item = &sched.items[id];
    item->second = seconds + ticks / sched.clockRates[item->clock];
    item->tick = ticks % sched.clockRates[item->clock];
    item->cycle = muldiv_floor(item->tick, sched.clockRates[CLOCK_CPU], sched.clockRates[item->clock]);
    if (id == sched.event.next) {
        sched_update(SCHED_SECOND);
    } else if (id == sched.dma.next) {
        sched_update(SCHED_PREV_MA);
    } else {
        sched_update(id);
    }
}

void sched_repeat_relative(enum sched_item_id id, enum sched_item_id base, uint32_t offset, uint64_t ticks) {
    struct sched_item *item = &sched.items[base];
    sched_schedule(id, item->second ^ item->second >> 31, muldiv_ceil(item->tick + offset, sched.clockRates[sched.items[id].clock], sched.clockRates[item->clock]) + ticks);
}

void sched_repeat(enum sched_item_id id, uint64_t ticks) {
    sched_repeat_relative(id, id, 0, ticks);
}

void sched_set(enum sched_item_id id, uint64_t ticks) {
    sched_schedule(id, 0, muldiv_ceil(cpu.cycles, sched.clockRates[sched.items[id].clock], sched.clockRates[CLOCK_CPU]) + ticks);
}

void sched_clear(enum sched_item_id id) {
    struct sched_item *item = &sched.items[id];
    if (item->second >= 0) {
        item->second = ~item->second;
        if (id == sched.event.next) {
            sched_update(SCHED_SECOND);
        } else if (id == sched.dma.next) {
            sched_update(SCHED_PREV_MA);
        }
    }
}

uint64_t sched_cycle(enum sched_item_id id) {
    struct sched_item *item = &sched.items[id];
    assert(item->second >= 0);
    return (uint64_t)item->second * sched.clockRates[CLOCK_CPU] + item->cycle;
}

uint64_t sched_cycles_remaining(enum sched_item_id id) {
    return sched_cycle(id) - cpu.cycles;
}

uint64_t sched_tick(enum sched_item_id id) {
    struct sched_item *item = &sched.items[id];
    assert(item->second >= 0);
    return (uint64_t)item->second * sched.clockRates[item->clock] + item->tick;
}

uint64_t sched_ticks_remaining(enum sched_item_id id) {
    return sched_tick(id) - muldiv_floor(cpu.cycles, sched.clockRates[sched.items[id].clock], sched.clockRates[CLOCK_CPU]);
}

void sched_process_pending_events(void) {
    while (sched_event_next_cycle() <= cpu.cycles) {
        enum sched_item_id id = sched.event.next;
        sched_clear(id);
        sched.items[id].callback.event(id);
    }
}

void sched_process_pending_dma(uint8_t duration) {
    if (sched.event.cycle) {
        cpu.cycles += duration;
        return;
    }
    while (true) {
        enum sched_item_id id = sched.dma.next;
        if (id == SCHED_PREV_MA) {
            break;
        }
        if (sched_before(SCHED_PREV_MA, id)) {
            if (sched_cycle(id) > cpu.cycles) {
                break;
            }
            sched_repeat_relative(SCHED_PREV_MA, id, 0, 0);
        } else if (sched_cycle(SCHED_PREV_MA) > cpu.cycles) {
            break;
        }
        sched_clear(id);
        sched_repeat(SCHED_PREV_MA, sched.items[id].callback.dma(id));
    }
    if (duration) {
        uint32_t prev_cycle = sched_cycle(SCHED_PREV_MA);
        if (prev_cycle > cpu.cycles) {
            cpu.dmaCycles += prev_cycle - cpu.cycles;
            cpu.cycles = prev_cycle;
        }
        cpu.cycles += duration;
    }
    sched_set(SCHED_PREV_MA, 0);
}

static void sched_second(enum sched_item_id id) {
    struct sched_item *item;
    sched_process_pending_dma(0);
    for (id = 0; id < SCHED_NUM_ITEMS; id++) {
        item = &sched.items[id];
        if (item->second >= 0) {
            item->second--;
        }
    }
    cpu.seconds++;
    cpu.cycles -= sched.clockRates[CLOCK_CPU];
    cpu.baseCycles += sched.clockRates[CLOCK_CPU];
    sched.items[SCHED_SECOND].second = 0; // Don't use sched_repeat!
    sched_update(SCHED_SECOND);
}

void sched_set_clocks(enum clock_id count, uint32_t *new_rates) {
    enum sched_item_id id;
    struct sched_item *item;
    uint64_t ticks;

    if (sched.event.cycle) {
        return;
    }
    cpu.baseCycles += cpu.cycles;
    cpu.cycles = muldiv_floor(cpu.cycles, new_rates[CLOCK_CPU], sched.clockRates[CLOCK_CPU]);
    cpu.baseCycles -= cpu.cycles;
    for (id = 0; id < SCHED_NUM_ITEMS; id++) {
        item = &sched.items[id];
        if (item->second >= 0) {
            if (item->clock < count) {
                ticks = (uint64_t)item->second * sched.clockRates[item->clock] + item->tick;
                item->second = ticks / new_rates[item->clock];
                item->tick = ticks % new_rates[item->clock];
                item->cycle = muldiv_floor(item->tick, new_rates[CLOCK_CPU], new_rates[item->clock]);
            } else {
                item->cycle = muldiv_floor(item->tick, new_rates[CLOCK_CPU], sched.clockRates[item->clock]);
            }
        }
    }
    sched_update(SCHED_SECOND);
    memcpy(sched.clockRates, new_rates, sizeof(uint32_t) * count);
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
    sched.items[SCHED_SECOND].cycle = sched.clockRates[CLOCK_CPU];

    sched.items[SCHED_THROTTLE].clock = CLOCK_6M;
    sched.items[SCHED_THROTTLE].callback.event = emu_throttle_event;
    sched_set(SCHED_THROTTLE, 0);

    sched.items[SCHED_PREV_MA].clock = CLOCK_48M;
    sched_set(SCHED_PREV_MA, 0);
}

uint64_t sched_total_cycles(void) {
    return cpu.baseCycles + cpu.cycles;
}

uint64_t sched_total_time(enum clock_id clock) {
    return (uint64_t)cpu.seconds * sched.clockRates[clock] + muldiv_floor(cpu.cycles, sched.clockRates[clock], sched.clockRates[CLOCK_CPU]);
}

uint64_t event_next_cycle(enum sched_item_id id) {
    struct sched_item *item = &sched.items[id];
    return (uint64_t)item->second * sched.clockRates[CLOCK_CPU] + item->cycle - sched_event_next_cycle() + cpu.baseCycles;
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
