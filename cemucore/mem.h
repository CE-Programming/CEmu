/*
 * Copyright (c) 2015-2020 CE Programming.
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

#ifndef CEMUCORE_MEM_H
#define CEMUCORE_MEM_H

#include <stdbool.h>
#include <stdint.h>

typedef struct mem
{
    uint8_t *flash, *ram;
#ifndef CEMUCORE_NODEBUG
    uint8_t *dbg;
#endif
} mem_t;

bool mem_init(mem_t *mem);
void mem_destroy(mem_t *mem);

#endif
