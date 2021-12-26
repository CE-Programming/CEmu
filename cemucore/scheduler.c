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

bool scheduler_init(scheduler_t *scheduler)
{
    (void)scheduler;
    return true;
}

void scheduler_destroy(scheduler_t *scheduler)
{
    core_free(scheduler->heap, scheduler->nevents);
    core_free(scheduler->events, scheduler->nevents);
    scheduler->nevents = 0;
    core_free(scheduler->slots, scheduler->nslots);
    scheduler->nslots = 0;
    core_free(scheduler->clocks, scheduler->nclocks);
    scheduler->nclocks = 0;
}

scheduler_event_t scheduler_register_event(scheduler_t *scheduler, const char *name, scheduler_clock_t clock)
{
    if (scheduler->nevents == 0xFF)
    {
        core_fatal("too many events");
    }
    scheduler_event_t event = {.handle = scheduler->nevents};
    scheduler->nevents += 1;
    core_realloc(scheduler->events, event.handle, scheduler->nevents);
    scheduler->events[event.handle].name = name;
    scheduler->events[event.handle].clock = clock;
    return event;
}
