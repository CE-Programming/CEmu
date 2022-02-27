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

#include "scheduler.h"

#include "core.h"

void scheduler_init(scheduler_t *scheduler)
{
    cemucore_unused(scheduler);
}

void scheduler_destroy(scheduler_t *scheduler)
{
    core_free(scheduler->heap, scheduler->nevents);
    for (uint8_t i = 0; i != scheduler->nevents; ++i)
    {
        core_free_string(scheduler->events[i].name);
    }
    core_free(scheduler->events, scheduler->nevents);
    scheduler->nevents = 0;
    for (uint8_t i = 0; i != scheduler->nslots; ++i)
    {
        core_free_string(scheduler->slots[i].name);
    }
    core_free(scheduler->slots, scheduler->nslots);
    scheduler->nslots = 0;
    for (uint8_t i = 0; i != scheduler->nclocks; ++i)
    {
        core_free_string(scheduler->clocks[i].name);
    }
    core_free(scheduler->clocks, scheduler->nclocks);
    scheduler->nclocks = 0;
}

scheduler_event_t scheduler_register_event(scheduler_t *scheduler, const char *name, scheduler_clock_t clock)
{
    if (cemucore_unlikely(scheduler->nevents == 0xFF))
    {
        core_fatal("too many events");
    }
    scheduler_event_t event = {.handle = scheduler->nevents};
    scheduler->nevents += 1;
    core_realloc(scheduler->events, event.handle, scheduler->nevents);
    scheduler->events[event.handle].name = core_duplicate_string(name);
    scheduler->events[event.handle].clock = clock;
    return event;
}

scheduler_slot_t scheduler_register_slot(scheduler_t *scheduler, const char *name, uint8_t index)
{
    if (cemucore_unlikely(index >= 64))
    {
        core_fatal("invalid slot index");
    }
    scheduler_slot_t slot = {.handle = index};
    if (scheduler->nslots <= index)
    {
        core_realloc(scheduler->slots, scheduler->nslots, index + 1);
        scheduler->nslots = index + 1;
    }
    scheduler->slots[index].name = core_duplicate_string(name);
    return slot;
}
