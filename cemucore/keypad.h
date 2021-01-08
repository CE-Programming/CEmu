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

#ifndef KEYPAD_H
#define KEYPAD_H

#include "compiler.h"

#include <stdint.h>

typedef struct keypad
{
    uint8_t keys[8];
    CEMUCORE_MAYBE_ATOMIC(uint16_t) events[8];
    CEMUCORE_MAYBE_ATOMIC(uint32_t) gpio_enable;
} keypad_t;

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
}
#endif

#endif
