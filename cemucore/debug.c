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

#include "debug.h"

#include "core.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

union debug_watch
{
    bool active : 1;
    struct {
        int32_t : 1, next;
    };
    struct {
        uint32_t : 1, addr : 24, size : 24, flags : 8;
    };
};
static_assert(sizeof(debug_watch_t) == sizeof(uint64_t), "watch_t bigger than expected");

static debug_watch_t *debug_watch(debug_t *debug, int32_t id)
{
    return (uint32_t)id < debug->nwatches ? &debug->watches[id] : NULL;
}

static bool debug_watch_active(debug_watch_t *watch)
{
    return watch && watch->active;
}

static void debug_watch_update(debug_t *debug)
{
    cemucore_unused(debug);
}

void debug_init(debug_t *debug)
{
    debug->fwatches = -1;
}

void debug_destroy(debug_t *debug)
{
    core_free(debug->watches, debug->nwatches);
}

int32_t debug_watch_create(debug_t *debug)
{
    uint32_t id = debug->fwatches;
    debug_watch_t *watch = debug_watch(debug, id);
    if (watch)
    {
        debug->fwatches = watch->next;
    }
    else
    {
        id = debug->nwatches;
        core_realloc(debug->watches, debug->nwatches, id + 1);
        watch = debug_watch(debug, id);
    }
    watch->active = true;
    watch->size = 0;
    watch->flags = CEMUCORE_DEBUG_WATCH_MEMORY | CEMUCORE_DEBUG_WATCH_ANY;
    watch->addr = 0;
    return id;
}

void debug_watch_destroy(debug_t *debug, int32_t id)
{
    debug_watch_t *watch = debug_watch(debug, id);
    if (!watch)
    {
        return;
    }
    watch->active = false;
    watch->next = debug->fwatches;
    debug->fwatches = id;
}

void debug_watch_copy(debug_t *debug, int32_t dstId, int32_t srcId)
{
    debug_watch_t *dst = debug_watch(debug, dstId);
    if (!debug_watch_active(dst))
    {
        return;
    }
    if (srcId == -1)
    {
        debug_watch_destroy(debug, dstId);
        return;
    }
    debug_watch_t *src = debug_watch(debug, srcId);
    if (!debug_watch_active(src))
    {
        return;
    }
    *dst = *src;
}

int32_t debug_watch_get_addr(debug_t *debug, int32_t id)
{
    debug_watch_t *watch = debug_watch(debug, id);
    return debug_watch_active(watch) ? watch->addr : -1;
}

void debug_watch_set_addr(debug_t *debug, int32_t id, int32_t addr)
{
    debug_watch_t *watch = debug_watch(debug, id);
    if (!debug_watch_active(watch))
    {
        return;
    }
    uint32_t old = watch->addr;
    watch->addr = addr;
    if (watch->flags & CEMUCORE_DEBUG_WATCH_ENABLE && watch->addr != old)
    {
        debug_watch_update(debug);
    }
}

int32_t debug_watch_get_size(debug_t *debug, int32_t id)
{
    debug_watch_t *watch = debug_watch(debug, id);
    return debug_watch_active(watch) ? watch->size + 1 : -1;
}

void debug_watch_set_size(debug_t *debug, int32_t id, int32_t size)
{
    debug_watch_t *watch = debug_watch(debug, id);
    if (!debug_watch_active(watch))
    {
        return;
    }
    uint32_t old = watch->size;
    watch->size = size ? size - 1 : 0;
    if (watch->flags & CEMUCORE_DEBUG_WATCH_ENABLE && watch->size != old)
    {
        debug_watch_update(debug);
    }
}

cemucore_debug_flags_t debug_watch_get_flags(debug_t *debug, int32_t id)
{
    debug_watch_t *watch = debug_watch(debug, id);
    return debug_watch_active(watch) ? watch->flags : -1;
}

void debug_watch_set_flags(debug_t *debug, int32_t id, cemucore_debug_flags_t flags)
{
    debug_watch_t *watch = debug_watch(debug, id);
    if (!debug_watch_active(watch))
    {
        return;
    }
    cemucore_debug_flags_t old = watch->flags;
    watch->flags = flags;
    if (watch->flags & CEMUCORE_DEBUG_WATCH_ENABLE && watch->size != old)
    {
        debug_watch_update(debug);
    }
}

bool debug_has_watch(debug_t *debug, int32_t addr, cemucore_debug_flags_t flags)
{
    // TODO: do the thing
    bool port = (flags & CEMUCORE_DEBUG_WATCH_AREA) == CEMUCORE_DEBUG_WATCH_PORT;
    cemucore_debug_flags_t mode = flags & CEMUCORE_DEBUG_WATCH_MODE;
    cemucore_debug_flags_t type = flags & CEMUCORE_DEBUG_WATCH_TYPE;
    for (uint32_t id = 0; id != debug->nwatches; ++id)
    {
        debug_watch_t *watch = debug_watch(debug, id);
        if (debug_watch_active(watch) &&
            watch->flags & CEMUCORE_DEBUG_WATCH_ENABLE &&
            ((watch->flags & CEMUCORE_DEBUG_WATCH_AREA) == CEMUCORE_DEBUG_WATCH_PORT) == port &&
            (watch->flags & mode) == mode && (watch->flags & type) == type &&
            watch->addr <= addr && watch->addr + watch->size >= addr)
        {
            return true;
        }
    }
    return false;
}
