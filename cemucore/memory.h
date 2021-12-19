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

#ifndef CEMUCORE_MEMORY_H
#define CEMUCORE_MEMORY_H

#include <stdbool.h>
#include <stdint.h>

#define MEMORY_DEFAULT_FLASH_SIZE  0x400000
#define MEMORY_MAX_FLASH_SIZE     0x1000000
#define MEMORY_RAM_SIZE             0x65800

typedef struct memory
{
    int32_t flash_size;
    uint8_t *flash, *ram;
#ifndef CEMUCORE_NODEBUG
    uint8_t *debug;
#endif
} memory_t;

bool memory_init(memory_t *memory);
void memory_destroy(memory_t *memory);

#endif
