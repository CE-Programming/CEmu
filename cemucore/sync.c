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

#include "sync.h"
#include "compiler.h"

#include <stdlib.h>
#include <stdio.h>

#define STATE_STOPPING (UINT32_C(1) << 0)
#define STATE_SLEEPING (UINT32_C(1) << 1)
#define STATE_SYNCED   (UINT32_C(1) << 2)
#define STATE_COUNTER  (UINT32_C(1) << 3)

#define COND_SYNCED    0
#define COND_WAIT_SYNC 1
#define COND_WAIT_RUN  2

bool sync_init(sync_t *sync)
{
    if (unlikely(pthread_mutex_init(&sync->mutex, NULL)))
    {
        return false;
    }
    for (int i = 0; i != SYNC_COND_COUNT; ++i)
    {
        if (unlikely(pthread_cond_init(&sync->cond[i], NULL)))
        {
            while (i)
            {
                pthread_cond_destroy(&sync->cond[--i]);
            }
            pthread_mutex_destroy(&sync->mutex);
            return false;
        }
    }
    atomic_init(&sync->state, 0);
    return true;
}

void sync_destroy(sync_t *sync)
{
    for (int i = SYNC_COND_COUNT; i; )
    {
        if (unlikely(pthread_cond_destroy(&sync->cond[--i])))
        {
            abort();
        }
    }
    if (unlikely(pthread_mutex_destroy(&sync->mutex)))
    {
        abort();
    }
}

void sync_lock(sync_t *sync)
{
    if (unlikely(pthread_mutex_lock(&sync->mutex)))
    {
        abort();
    }
}

void sync_unlock(sync_t *sync)
{
    if (unlikely(pthread_mutex_unlock(&sync->mutex)))
    {
        abort();
    }
}

bool sync_check(sync_t *sync)
{
    return unlikely(atomic_load_explicit(&sync->state, memory_order_relaxed) / STATE_COUNTER);
}

bool sync_loop(sync_t *sync)
{
    if (!sync_check(sync))
    {
        return true;
    }
    sync_lock(sync);
    (void)atomic_fetch_or_explicit(&sync->state, STATE_SYNCED, memory_order_relaxed);
    do
    {
        if (unlikely(pthread_cond_signal(&sync->cond[COND_WAIT_SYNC]) ||
                     pthread_cond_wait(&sync->cond[COND_SYNCED], &sync->mutex)))
        {
            abort();
        }
    } while (sync_check(sync));
    bool stopping = atomic_fetch_and_explicit(&sync->state, ~STATE_SYNCED, memory_order_relaxed) & STATE_STOPPING;
    sync_unlock(sync);
    if (unlikely(pthread_cond_broadcast(&sync->cond[COND_WAIT_RUN])))
    {
        abort();
    }
    return likely(!stopping);
}

static void sync_reenter(sync_t *sync)
{
    bool synced = atomic_fetch_add_explicit(&sync->state, STATE_COUNTER, memory_order_relaxed) & STATE_SYNCED;
    while (unlikely(!synced))
    {
        if (unlikely(pthread_cond_wait(&sync->cond[COND_WAIT_SYNC], &sync->mutex)))
        {
            abort();
        }
        synced = atomic_load_explicit(&sync->state, memory_order_relaxed) & STATE_SYNCED;
    }
}

static void sync_maybe_leave(sync_t *sync, bool unlock)
{
    bool more = atomic_fetch_sub_explicit(&sync->state, STATE_COUNTER, memory_order_relaxed) >= STATE_COUNTER << 1;
    if (unlock)
    {
        sync_unlock(sync);
    }
    if (unlikely(pthread_cond_signal(&sync->cond[more ? COND_WAIT_SYNC : COND_SYNCED])))
    {
        abort();
    }
}

static void sync_wait_run(sync_t *sync)
{
    sync_maybe_leave(sync, false);
    if (unlikely(pthread_cond_wait(&sync->cond[COND_WAIT_RUN], &sync->mutex)))
    {
        abort();
    }
}

bool sync_sleep(sync_t *sync)
{
    if (unlikely(atomic_fetch_or_explicit(&sync->state, STATE_SLEEPING,
                                          memory_order_relaxed) & STATE_SLEEPING))
    {
        return false;
    }
    (void)atomic_fetch_add_explicit(&sync->state, STATE_COUNTER, memory_order_relaxed);
    return true;
}

void sync_wake(sync_t *sync)
{
    if (likely(atomic_fetch_and_explicit(&sync->state, ~STATE_SLEEPING,
                                         memory_order_relaxed) & STATE_SLEEPING))
    {
        sync_maybe_leave(sync, false);
    }
}

void sync_enter(sync_t *sync)
{
    sync_lock(sync);
    sync_reenter(sync);
}

void sync_run(sync_t *sync)
{
    sync_wait_run(sync);
    sync_reenter(sync);
}

void sync_leave(sync_t *sync)
{
    sync_maybe_leave(sync, true);
}

void sync_run_leave(sync_t *sync)
{
    sync_wait_run(sync);
    sync_unlock(sync);
}

void sync_stop(sync_t *sync)
{
    sync_enter(sync);
    if (atomic_fetch_or_explicit(&sync->state, STATE_STOPPING, memory_order_relaxed) & STATE_STOPPING)
    {
        sync_leave(sync);
    }
    else
    {
        sync_wake(sync);
        sync_run_leave(sync);
        sync_destroy(sync);
    }
}
