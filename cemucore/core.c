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

#include "core.h"

#include <stdio.h>
#include <stdlib.h>

#ifndef CEMUCORE_NOTHREADS
static bool check_signals(cemucore_t *core)
{
#if ATOMIC_INT_LOCK_FREE != 2
    return !atomic_flag_test_and_set_explicit(&core->no_pending, memory_order_relaxed);
#else
    return !atomic_load_explicit(&core->pending, memory_order_relaxed);
#endif
}

static void *thread_start(void *data)
{
    cemucore_t *core = data;
    fprintf(stderr, "Hello from the other side\n");
    if (core->signal)
    {
        core->signal(core->signal_data);
    }
    return NULL;
}
#endif

cemucore_t *cemucore_create(cemucore_create_flags_t create_flags,
                            cemucore_signal_t signal, void *signal_data)
{
    cemucore_t *core = malloc(sizeof(cemucore_t));
    if (!core)
    {
        return NULL;
    }

    core->signal = signal;
    core->signal_data = signal_data;

#ifndef CEMUCORE_NOTHREADS
    if (create_flags & CEMUCORE_CREATE_THREADED)
    {
        if (pthread_create(&core->thread, NULL, &thread_start, core))
        {
            free(core);
            return NULL;
        }
    }
    else
    {
        core->thread = pthread_self();
    }
#endif

    return core;
}

void cemucore_destroy(cemucore_t *core)
{
    if (!core)
    {
        return;
    }

#ifndef CEMUCORE_NOTHREADS
    pthread_join(core->thread, NULL);
#endif

    free(core);
}

keypad_t keypad;
debug_t debug;
lcd_t lcd;

ti_device_t get_device_type(void) {
    return TI84PCE;
}
uint8_t mem_peek_byte(uint32_t addr) {
    (void)addr;
    return 0;
}
void emu_set_lcd_ptrs(uint32_t **dat, uint32_t **dat_end, int width, int height, uint32_t addr, uint32_t control, bool mask) {
    (void)dat;
    (void)dat_end;
    (void)width;
    (void)height;
    (void)addr;
    (void)control;
    (void)mask;
}
void emu_lcd_drawmem(void *output, void *data, void *data_end, uint32_t control, int size, int spi) {
    (void)output;
    (void)data;
    (void)data_end;
    (void)control;
    (void)size;
    (void)spi;
}
void emu_keypad_event(unsigned int row, unsigned int col, bool press) {
    (void)row;
    (void)col;
    (void)press;
}
void keypad_intrpt_check(void) {}
FILE *fopen_utf8(const char *filename, const char *mode) {
    return fopen(filename, mode);
}
