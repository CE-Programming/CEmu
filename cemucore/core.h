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

#ifndef CEMUCORE_CORE_H
#define CEMUCORE_CORE_H

#include "cemucore.h"

#include "cpu.h"
#include "keypad.h"
#include "memory.h"
#include "scheduler.h"
#include "usb/usb.h"

#ifndef CEMUCORE_NOTHREADS
# include "sync.h"

# include <pthread.h>
# include <stdatomic.h>
#endif

#include <stdbool.h>

struct cemucore
{
#ifndef CEMUCORE_NOTHREADS
    pthread_t thread;
    sync_t sync;
#endif
    cemucore_sig_handler_t sig_handler;
    void *sig_handler_data;
    cemucore_dev_t dev;
    scheduler_t scheduler;
    cpu_t cpu;
    memory_t memory;
    keypad_t keypad;
    usb_t usb;
};

void core_sig(cemucore_t *core, cemucore_sig_t sig, bool locked);

#endif
