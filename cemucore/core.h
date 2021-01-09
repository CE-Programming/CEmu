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

#ifndef CEMUCORE_CORE_H
#define CEMUCORE_CORE_H

#include "cemucore.h"

#include "cpu.h"
#include "keypad.h"
#include "mem.h"

#ifndef CEMUCORE_NOTHREADS
# include "sync.h"

# include <pthread.h>
# include <stdatomic.h>
#endif

struct cemucore
{
#ifndef CEMUCORE_NOTHREADS
    pthread_t thread;
    sync_t sync;
#endif
    cemucore_signal_handler_t signal_handler;
    void *signal_handler_data;
    cpu_t cpu;
    mem_t mem;
    keypad_t keypad;
};

#endif
