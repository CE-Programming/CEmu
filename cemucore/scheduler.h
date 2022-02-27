/*
 * Copyright (c) 2015-2021 CE Programming.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CEMUCORE_SCHEDULER_H
#define CEMUCORE_SCHEDULER_H

#include <stdbool.h>
#include <stdint.h>

typedef struct { uint8_t handle; } scheduler_clock_t;
typedef struct { uint8_t handle; } scheduler_slot_t;
typedef struct { uint8_t handle; } scheduler_event_t;

typedef struct scheduler
{
    uint64_t slots_available, slots_enabled;
    struct scheduler_clock
    {
        char *name;
        uint64_t frequency;
    } *clocks;
    struct scheduler_slot
    {
        char *name;
    } *slots;
    struct scheduler_event
    {
        char *name;
        uint64_t slots;
        scheduler_clock_t clock;
    } *events;
    scheduler_event_t *heap;
    uint8_t nclocks, nslots, nevents;
} scheduler_t;

void scheduler_init(scheduler_t *scheduler);
void scheduler_destroy(scheduler_t *scheduler);
void scheduler_change_sources(scheduler_t *scheduler, uint64_t sources);
scheduler_clock_t scheduler_register_clock(scheduler_t *scheduler, const char *name, uint64_t frequency);
scheduler_clock_t scheduler_get_clock(scheduler_t *scheduler, const char *name);
scheduler_slot_t scheduler_register_slot(scheduler_t *scheduler, const char *name, uint8_t index);
void scheduler_change_clock_frequency(scheduler_t *scheduler, scheduler_clock_t clock, uint64_t frequency);
scheduler_event_t scheduler_register_event(scheduler_t *scheduler, const char *name, scheduler_clock_t clock);

#endif
