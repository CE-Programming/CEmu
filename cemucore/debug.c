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
        int32_t : 1, addr : 25;
        uint32_t flags : 8, size : 24;
    };
};
static_assert(sizeof(debug_watch_t) == sizeof(uint64_t), "struct debug_watch larger than expected");

union debug_mem_range
{
    uint32_t all;
    struct {
        uint16_t end, begin;
    };
};
static_assert(sizeof(debug_mem_range_t) == sizeof(uint32_t), "struct debug_mem_range larger than expected");

union debug_port_range
{
    uint16_t all;
    struct {
        uint8_t end, begin;
    };
};
static_assert(sizeof(debug_port_range_t) == sizeof(uint16_t), "struct debug_port_range larger than expected");

static debug_watch_t *debug_watch(debug_t *debug, int32_t id)
{
    return (uint32_t)id < debug->nwatches ? &debug->watches[id] : NULL;
}

static bool debug_watch_active(debug_watch_t *watch)
{
    return cemucore_likely(watch && watch->active);
}

static bool debug_watch_enabled(debug_watch_t *watch)
{
    return debug_watch_active(watch) && watch->flags & CEMUCORE_WATCH_ENABLE;
}

static int debug_mem_range_sort(const void *vlhs, const void *vrhs)
{
    const debug_mem_range_t *lhs = vlhs, *rhs = vrhs;
    return lhs->all < rhs->all ? -1 : lhs->all > rhs->all ? 1 : 0;
}

static int debug_mem_range_search(const void *vkey, const void *vrange)
{
    const uint16_t *pkey = vkey, key = *pkey;
    const debug_mem_range_t *range = vrange;
    return key < range->begin ? -1 : key > range->end ? 1 : 0;
}

static bool debug_has_mem_watch(debug_t *debug, uint32_t index, uint16_t key)
{
    uint32_t begin = debug->mem[index], end = debug->mem[index + 1];
    return bsearch(&key, &debug->rmem[begin], end - begin,
                   sizeof(*debug->rmem), &debug_mem_range_search);
}

static int debug_port_range_sort(const void *vlhs, const void *vrhs)
{
    const debug_port_range_t *lhs = vlhs, *rhs = vrhs;
    return lhs->all < rhs->all ? -1 : lhs->all > rhs->all ? 1 : 0;
}

static int debug_port_range_search(const void *vkey, const void *vrange)
{
    const uint8_t *pkey = vkey, key = *pkey;
    const debug_port_range_t *range = vrange;
    return key < range->begin ? -1 : key > range->end ? 1 : 0;
}

static bool debug_has_port_watch(debug_t *debug, uint32_t index, uint8_t key)
{
    uint32_t begin = debug->port[index], end = debug->port[index + 1];
    if (cemucore_unlikely(begin > end))
    {
        end += UINT32_C(1) << 16;
    }
    return bsearch(&key, &debug->rport[begin], end - begin,
                   sizeof(*debug->rport), &debug_port_range_search);
}

static void debug_range_update(debug_t *debug)
{
    uint32_t rindex = 0, index = 0;
    for (int32_t rmin = INT32_C(0x0000); rmin <= INT32_C(0xFFFFFF); rmin += INT32_C(0x010000))
    {
        int32_t rmax = rmin + INT32_C(0x00FFFF);
        for (cemucore_watch_flags_t mode = CEMUCORE_WATCH_MODE_Z80;
             mode & CEMUCORE_WATCH_MODE_ANY; mode <<= 1)
        {
            for (cemucore_watch_flags_t type = CEMUCORE_WATCH_TYPE_READ;
                 type & CEMUCORE_WATCH_TYPE_ALL; type <<= 1)
            {
                uint32_t rbegin = rindex;
                for (uint32_t id = 0; id != debug->nwatches; ++id)
                {
                    debug_watch_t *watch = debug_watch(debug, id);
                    if (cemucore_likely(!debug_watch_enabled(watch) ||
                                        (watch->flags & mode) != mode ||
                                        (watch->flags & type) != type))
                    {
                        continue;
                    }
                    int32_t rbegin = watch->addr, rend = rbegin + watch->size;
                    if (cemucore_likely(rbegin > rmax || rend < rmin))
                    {
                        continue;
                    }
                    if (cemucore_unlikely(rbegin < rmin))
                    {
                        rbegin = rmin;
                    }
                    if (cemucore_unlikely(rend > rmax))
                    {
                        rend = rmax;
                    }
                    if (cemucore_unlikely(rindex == debug->nmem))
                    {
                        core_realloc(debug->rmem, debug->nmem, debug->nmem * 2 + 1);
                    }
                    debug->rmem[rindex].begin = rbegin;
                    debug->rmem[rindex++].end = rend;
                }
                uint32_t rend = rindex;
                if (rbegin + 1 < rend)
                {
                    qsort(&debug->rmem[rbegin], rend - rbegin,
                          sizeof(*debug->rmem), &debug_mem_range_sort);
                    rindex = rbegin + 1;
                    for (uint32_t rsource = rindex; rsource < rend; ++rsource)
                    {
                        if (debug->rmem[rindex - 1].end + 1 < debug->rmem[rsource].begin)
                        {
                            debug->rmem[rindex++] = debug->rmem[rsource];
                        }
                        else if (debug->rmem[rindex - 1].end < debug->rmem[rsource].end)
                        {
                            debug->rmem[rindex - 1].end = debug->rmem[rsource].end;
                        }
                    }
                }
                debug->mem[++index] = rindex;
            }
        }
    }
    rindex = index = 0;
    for (int32_t rmin = INT32_C(0x0000); rmin <= INT32_C(0xFFFF); rmin += INT32_C(0x0100))
    {
        int32_t rmax = rmin + INT32_C(0x00FF);
        for (cemucore_watch_flags_t mode = CEMUCORE_WATCH_MODE_PORT;
             mode & CEMUCORE_WATCH_MODE_PORT; mode <<= 1)
        {
            for (cemucore_watch_flags_t type = CEMUCORE_WATCH_TYPE_READ;
                 type & CEMUCORE_WATCH_TYPE_READ_WRITE; type <<= 1)
            {
                uint32_t rbegin = rindex;
                for (uint32_t id = 0; id != debug->nwatches; ++id)
                {
                    debug_watch_t *watch = debug_watch(debug, id);
                    if (!debug_watch_enabled(watch) ||
                        (watch->flags & mode) != mode || (watch->flags & type) != type)
                    {
                        continue;
                    }
                    int32_t rbegin = watch->addr, rend = rbegin + watch->size;
                    if (cemucore_likely(rbegin > rmax || rend < rmin))
                    {
                        continue;
                    }
                    if (cemucore_unlikely(rbegin < rmin))
                    {
                        rbegin = rmin;
                    }
                    if (cemucore_unlikely(rend > rmax))
                    {
                        rend = rmax;
                    }
                    if (cemucore_unlikely(rindex == debug->nport))
                    {
                        core_realloc(debug->rport, debug->nport, debug->nport * 2 + 1);
                    }
                    debug->rport[rindex].begin = rbegin;
                    debug->rport[rindex++].end = rend;
                }
                uint32_t rend = rindex;
                if (rbegin + 1 < rend)
                {
                    qsort(&debug->rport[rbegin], rend - rbegin,
                          sizeof(*debug->rport), &debug_port_range_sort);
                    rindex = rbegin + 1;
                    for (uint32_t rsource = rindex; rsource < rend; ++rsource)
                    {
                        if (debug->rport[rindex - 1].end + 1 <= debug->rport[rsource].begin)
                        {
                            debug->rport[rindex++] = debug->rport[rsource];
                        }
                        else if (debug->rport[rindex - 1].end < debug->rport[rsource].end)
                        {
                            debug->rport[rindex - 1].end = debug->rport[rsource].end;
                        }
                    }
                }
                debug->port[++index] = rindex;
            }
        }
    }
}


void debug_init(debug_t *debug)
{
    debug->fwatches = -1;
}

void debug_destroy(debug_t *debug)
{
    core_free(debug->rmem, debug->nmem);
    core_free(debug->rport, debug->nport);
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
    watch->flags = CEMUCORE_WATCH_AREA_MEM | CEMUCORE_WATCH_MODE_ANY;
    watch->addr = 0;
    return id;
}

void debug_watch_destroy(debug_t *debug, int32_t id)
{
    debug_watch_t *watch = debug_watch(debug, id);
    if (!debug_watch_active(watch))
    {
        return;
    }
    watch->active = false;
    if (watch->flags & CEMUCORE_WATCH_ENABLE)
    {
        debug_range_update(debug);
    }
    watch->next = debug->fwatches;
    debug->fwatches = id;
}

void debug_watch_copy(debug_t *debug, int32_t dstId, int32_t srcId)
{
    debug_watch_t *dst = debug_watch(debug, dstId), *src = debug_watch(debug, srcId);
    if (debug_watch_active(dst) && debug_watch_active(src))
    {
        *dst = *src;
    }
}

int32_t debug_watch_get_addr(debug_t *debug, int32_t id)
{
    debug_watch_t *watch = debug_watch(debug, id);
    return debug_watch_active(watch) ? watch->addr : -1;
}

void debug_watch_set_addr(debug_t *debug, int32_t id, int32_t addr)
{
    static const int32_t min = -(INT32_C(1) << 24), max = (INT32_C(1) << 24) - 1;
    debug_watch_t *watch = debug_watch(debug, id);
    if (!debug_watch_active(watch))
    {
        return;
    }
    int32_t old = watch->addr;
    if (cemucore_unlikely(addr < min))
    {
        addr = min;
    }
    if (cemucore_unlikely(addr > max))
    {
        addr = max;
    }
    watch->addr = addr;
    if (watch->flags & CEMUCORE_WATCH_ENABLE && watch->addr != old)
    {
        debug_range_update(debug);
    }
}

int32_t debug_watch_get_size(debug_t *debug, int32_t id)
{
    debug_watch_t *watch = debug_watch(debug, id);
    return debug_watch_active(watch) ? watch->size + 1 : -1;
}

void debug_watch_set_size(debug_t *debug, int32_t id, int32_t size)
{
    static const int32_t min = INT32_C(1), max = INT32_C(1) << 24;
    debug_watch_t *watch = debug_watch(debug, id);
    if (!debug_watch_active(watch))
    {
        return;
    }
    uint32_t old = watch->size;
    if (cemucore_unlikely(size < min))
    {
        size = min;
    }
    if (cemucore_unlikely(size > max))
    {
        size = max;
    }
    watch->size = size - 1;
    if (watch->flags & CEMUCORE_WATCH_ENABLE && watch->size != old)
    {
        debug_range_update(debug);
    }
}

cemucore_watch_flags_t debug_watch_get_flags(debug_t *debug, int32_t id)
{
    debug_watch_t *watch = debug_watch(debug, id);
    return debug_watch_active(watch) ? watch->flags : -1;
}

void debug_watch_set_flags(debug_t *debug, int32_t id, cemucore_watch_flags_t flags)
{
    debug_watch_t *watch = debug_watch(debug, id);
    if (!debug_watch_active(watch))
    {
        return;
    }
    cemucore_watch_flags_t old = watch->flags;
    watch->flags = flags;
    if ((old | flags) & CEMUCORE_WATCH_ENABLE && watch->size != old)
    {
        debug_range_update(debug);
    }
}

bool debug_has_mem_z80_read_watch(debug_t *debug, int32_t addr)
{
    return debug_has_mem_watch(debug, ((addr >> 15 & 0xFF << 1) | 0) * 3 + 0, addr);
}

bool debug_has_mem_z80_write_watch(debug_t *debug, int32_t addr)
{
    return debug_has_mem_watch(debug, ((addr >> 15 & 0xFF << 1) | 0) * 3 + 1, addr);
}

bool debug_has_mem_z80_execute_watch(debug_t *debug, int32_t addr)
{
    return debug_has_mem_watch(debug, ((addr >> 15 & 0xFF << 1) | 0) * 3 + 2, addr);
}

bool debug_has_mem_adl_read_watch(debug_t *debug, int32_t addr)
{
    return debug_has_mem_watch(debug, ((addr >> 15 & 0xFF << 1) | 1) * 3 + 0, addr);
}

bool debug_has_mem_adl_write_watch(debug_t *debug, int32_t addr)
{
    return debug_has_mem_watch(debug, ((addr >> 15 & 0xFF << 1) | 1) * 3 + 1, addr);
}

bool debug_has_mem_adl_execute_watch(debug_t *debug, int32_t addr)
{
    return debug_has_mem_watch(debug, ((addr >> 15 & 0xFF << 1) | 1) * 3 + 2, addr);
}

bool debug_has_port_read_watch(debug_t *debug, int32_t addr)
{
    return debug_has_port_watch(debug, (addr >> 7 & 0xFF << 1) | 0, addr);
}

bool debug_has_port_write_watch(debug_t *debug, int32_t addr)
{
    return debug_has_port_watch(debug, (addr >> 7 & 0xFF << 1) | 1, addr);
}

cemucore_watch_flags_t debug_get_watch_flags(debug_t *debug, int32_t addr, cemucore_watch_flags_t flags)
{
    if ((flags & CEMUCORE_WATCH_AREA_MASK) == CEMUCORE_WATCH_AREA_PORT)
    {
        if (flags & CEMUCORE_WATCH_TYPE_READ && !debug_has_port_read_watch(debug, addr))
        {
            flags &= ~CEMUCORE_WATCH_TYPE_READ;
        }
        if (flags & CEMUCORE_WATCH_TYPE_WRITE && !debug_has_port_write_watch(debug, addr))
        {
            flags &= ~CEMUCORE_WATCH_TYPE_WRITE;
        }
        return flags & ~CEMUCORE_WATCH_TYPE_EXECUTE;
    }
    if (flags & CEMUCORE_WATCH_TYPE_READ &&
        !(flags & CEMUCORE_WATCH_MODE_Z80 && debug_has_mem_z80_read_watch(debug, addr)) &&
        !(flags & CEMUCORE_WATCH_MODE_ADL && debug_has_mem_adl_read_watch(debug, addr)))
    {
        flags &= ~CEMUCORE_WATCH_TYPE_READ;
    }
    if (flags & CEMUCORE_WATCH_TYPE_WRITE &&
        !(flags & CEMUCORE_WATCH_MODE_Z80 && debug_has_mem_z80_write_watch(debug, addr)) &&
        !(flags & CEMUCORE_WATCH_MODE_ADL && debug_has_mem_adl_write_watch(debug, addr)))
    {
        flags &= ~CEMUCORE_WATCH_TYPE_WRITE;
    }
    if (flags & CEMUCORE_WATCH_TYPE_EXECUTE &&
        !(flags & CEMUCORE_WATCH_MODE_Z80 && debug_has_mem_z80_execute_watch(debug, addr)) &&
        !(flags & CEMUCORE_WATCH_MODE_ADL && debug_has_mem_adl_execute_watch(debug, addr)))
    {
        flags &= ~CEMUCORE_WATCH_TYPE_EXECUTE;
    }
    return flags;
}
