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

#ifndef CEMUCORE_DEBUG_H
#define CEMUCORE_DEBUG_H

#include "cemucore.h"

#include <stdint.h>
#include <stdlib.h>

typedef union debug_watch debug_watch_t;

typedef union debug_mem_range debug_mem_range_t;

typedef union debug_port_range debug_port_range_t;

typedef struct debug
{
    debug_watch_t *watches;
    int32_t fwatches;
    uint32_t nwatches;

    debug_mem_range_t *rmem;
    debug_port_range_t *rport;
    uint32_t nmem, nport;
    uint32_t mem[0x100 * 2 * 3 + 1];
    uint16_t port[0x100 * 1 * 2 + 1];
} debug_t;

void debug_init(debug_t *debug);
void debug_destroy(debug_t *debug);
int32_t debug_watch_create(debug_t *debug);
void debug_watch_destroy(debug_t *debug, int32_t id);
void debug_watch_copy(debug_t *debug, int32_t dstId, int32_t srcId);
int32_t debug_watch_get_addr(debug_t *debug, int32_t id);
void debug_watch_set_addr(debug_t *debug, int32_t id, int32_t addr);
int32_t debug_watch_get_size(debug_t *debug, int32_t id);
void debug_watch_set_size(debug_t *debug, int32_t id, int32_t size);
cemucore_watch_flags_t debug_watch_get_flags(debug_t *debug, int32_t id);
void debug_watch_set_flags(debug_t *debug, int32_t id, cemucore_watch_flags_t flags);
bool debug_has_mem_z80_read_watch(debug_t *debug, int32_t addr);
bool debug_has_mem_z80_write_watch(debug_t *debug, int32_t addr);
bool debug_has_mem_z80_execute_watch(debug_t *debug, int32_t addr);
bool debug_has_mem_adl_read_watch(debug_t *debug, int32_t addr);
bool debug_has_mem_adl_write_watch(debug_t *debug, int32_t addr);
bool debug_has_mem_adl_execute_watch(debug_t *debug, int32_t addr);
bool debug_has_port_read_watch(debug_t *debug, int32_t addr);
bool debug_has_port_write_watch(debug_t *debug, int32_t addr);
cemucore_watch_flags_t debug_get_watch_flags(debug_t *debug, int32_t addr, cemucore_watch_flags_t flags);

#endif
