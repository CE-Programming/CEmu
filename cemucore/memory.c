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

#include "memory.h"

#include "core.h"

#include <string.h>

void memory_init(memory_t *memory)
{
    memory->flash_size = MEMORY_DEFAULT_FLASH_SIZE;
    core_alloc(memory->flash, memory->flash_size);
    memset(memory->flash, 0xFF, memory->flash_size);
    core_alloc(memory->ram, MEMORY_RAM_SIZE);
#ifndef CEMUCORE_NODEBUG
    core_alloc(memory->debug, MEMORY_SPACE_SIZE);
#endif
}

void memory_destroy(memory_t *memory)
{
#ifndef CEMUCORE_NODEBUG
    core_free(memory->debug, MEMORY_SPACE_SIZE);
#endif
    core_free(memory->ram, MEMORY_RAM_SIZE);
    core_free(memory->flash, memory->flash_size);
}
