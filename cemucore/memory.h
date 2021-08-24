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

#ifndef CEMUCORE_MEMORY_H
#define CEMUCORE_MEMORY_H

#include <stdbool.h>
#include <stdint.h>

typedef struct memory
{
    uint8_t *flash, *ram;
#ifndef CEMUCORE_NODEBUG
    uint8_t *debug;
#endif
} memory_t;

bool memory_init(memory_t *memory);
void memory_destroy(memory_t *memory);

#endif
