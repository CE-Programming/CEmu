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

#include <stdlib.h>

bool scheduler_init(scheduler_t *scheduler)
{
    (void)scheduler;
    return true;
}

void scheduler_destroy(scheduler_t *scheduler)
{
    free(scheduler->clocks);
    free(scheduler->events);
    free(scheduler->heap);
}
