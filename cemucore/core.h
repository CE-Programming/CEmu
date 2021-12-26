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

#include "compiler.h"
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
#include <stddef.h>

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

void *core_alloc_location(size_t type_size, size_t new_count, const char *file, int line) cemucore_alloc;
#define core_alloc_location(new_memory, new_count, file, line) ((new_memory) = core_alloc_location(sizeof(*(new_memory)), new_count, file, line))
#define core_alloc(new_memory, new_count) core_alloc_location(new_memory, new_count, __FILE__, __LINE__)

void *core_realloc_location(size_t type_size, void *old_memory, size_t old_count, size_t new_count, const char *file, int line);
#define core_realloc_location(old_memory, old_count, new_count, file, line) ((old_memory) = core_realloc_location(sizeof(*(old_memory)), old_memory, old_count, new_count, file, line))
#define core_realloc(old_memory, old_count, new_count) core_realloc_location(old_memory, old_count, new_count, __FILE__, __LINE__)

void *core_free_location(size_t type_size, void *old_memory, size_t old_count, const char *file, int line);
#define core_free_location(old_memory, count, file, line) ((old_memory) = core_free_location(sizeof(*(old_memory)), old_memory, count, file, line))
#define core_free(old_memory, count) core_free_location(old_memory, count, __FILE__, __LINE__)

void core_fatal_location(const char *format, ...) cemucore_noreturn;
#define core_fatal_location(file, line, format, ...) core_fatal_location("%s:%d fatal: " format "\n", file, line, ## __VA_ARGS__)
#define core_fatal(format, ...) core_fatal_location(__FILE__, __LINE__, format, ## __VA_ARGS__)

#endif
