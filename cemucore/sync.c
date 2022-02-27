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

#include "sync.h"

#include "compiler.h"

#include <errno.h>
#include <stdlib.h>

enum { NSEC_PER_SEC = 1000000000 };
void timespec_add(struct timespec *res, const struct timespec *lhs, const struct timespec *rhs)
{
    uint32_t nsec = lhs->tv_nsec + rhs->tv_nsec, norm;
    bool overflow = cemucore_add_overflow(nsec, (uint32_t)-NSEC_PER_SEC, &norm);
    res->tv_nsec = cemucore_unpredictable(overflow) ? norm : nsec;
    res->tv_sec = lhs->tv_sec + rhs->tv_sec + overflow;
}
void timespec_sub(struct timespec *res, const struct timespec *lhs, const struct timespec *rhs)
{
    uint32_t nsec = lhs->tv_nsec - rhs->tv_nsec, norm;
    bool overflow = cemucore_add_overflow(nsec, (uint32_t)+NSEC_PER_SEC, &norm);
    res->tv_nsec = cemucore_unpredictable(overflow) ? norm : nsec;
    res->tv_sec = lhs->tv_sec - rhs->tv_sec - overflow;
}

typedef enum sync_state
{
    SYNC_STATE_STOPPING = UINT32_C(1) << 0,
    SYNC_STATE_RUNNING  = UINT32_C(1) << 1,
    SYNC_STATE_SYNCED   = UINT32_C(1) << 2,
    SYNC_STATE_COUNTER  = UINT32_C(1) << 3,
} sync_state_t;

bool sync_init(sync_t *sync)
{
    pthread_condattr_t condattr;
    if (cemucore_unlikely(pthread_condattr_init(&condattr)))
    {
        return false;
    }

    int error = 0;
#ifndef __APPLE__
    sync->clock = CLOCK_MONOTONIC;
    error = pthread_condattr_setclock(&condattr, sync->clock);
    if (cemucore_unlikely(error == EINVAL))
    {
#endif
        sync->clock = CLOCK_REALTIME;
#ifndef __APPLE__
        error = pthread_condattr_setclock(&condattr, sync->clock);
    }
#endif

    if (cemucore_unlikely(error || pthread_mutex_init(&sync->mutex, NULL)))
    {
        cemucore_unused(pthread_condattr_destroy(&condattr));
        return false;
    }
    for (int i = 0; i != SYNC_COND_COUNT; ++i)
    {
        if (cemucore_unlikely(pthread_cond_init(&sync->cond[i], &condattr)))
        {
            while (i)
            {
                cemucore_unused(pthread_cond_destroy(&sync->cond[--i]));
            }
            cemucore_unused(pthread_mutex_destroy(&sync->mutex));
            cemucore_unused(pthread_condattr_destroy(&condattr));
            return false;
        }
    }
    if (cemucore_unlikely(pthread_condattr_destroy(&condattr)))
    {
        sync_destroy(sync);
        return false;
    }
    atomic_init(&sync->state, SYNC_STATE_RUNNING);
    return true;
}

void sync_destroy(sync_t *sync)
{
    for (int i = SYNC_COND_COUNT; i; )
    {
        if (cemucore_unlikely(pthread_cond_destroy(&sync->cond[--i])))
        {
            abort();
        }
    }
    if (cemucore_unlikely(pthread_mutex_destroy(&sync->mutex)))
    {
        abort();
    }
}

void sync_lock(sync_t *sync)
{
    if (cemucore_unlikely(pthread_mutex_lock(&sync->mutex)))
    {
        abort();
    }
}

void sync_unlock(sync_t *sync)
{
    if (cemucore_unlikely(pthread_mutex_unlock(&sync->mutex)))
    {
        abort();
    }
}

static bool sync_leave_mask(sync_t *sync, uint32_t mask)
{
    bool stopping = atomic_fetch_and_explicit(&sync->state, ~mask,
                                              memory_order_relaxed) & SYNC_STATE_STOPPING;
    sync_unlock(sync);
    if (cemucore_unlikely(pthread_cond_broadcast(&sync->cond[SYNC_COND_WAIT_RUN])))
    {
        abort();
    }
    return !stopping;
}

bool sync_loop(sync_t *sync)
{
    if (cemucore_likely(!(atomic_load_explicit(&sync->state, memory_order_relaxed) / SYNC_STATE_COUNTER)))
    {
        return true;
    }
    sync_lock(sync);
    uint32_t state = atomic_fetch_or_explicit(&sync->state, SYNC_STATE_SYNCED,
                                              memory_order_relaxed);
    while (cemucore_unlikely(state / SYNC_STATE_COUNTER && !(state & SYNC_STATE_STOPPING)))
    {
        if (cemucore_unlikely(pthread_cond_signal(&sync->cond[SYNC_COND_WAIT_SYNCED]) ||
                              pthread_cond_wait(&sync->cond[SYNC_COND_SYNCED], &sync->mutex)))
        {
            abort();
        }
        state = atomic_load_explicit(&sync->state, memory_order_relaxed);
    }
    return sync_leave_mask(sync, SYNC_STATE_SYNCED | SYNC_STATE_STOPPING);
}

bool sync_delay(sync_t *sync, const struct timespec *delay)
{
    struct timespec abstime;
    if (cemucore_unlikely(clock_gettime(sync->clock, &abstime)))
    {
        abort();
    }
    timespec_add(&abstime, &abstime, delay);
    sync_lock(sync);
    bool timedout = false,
        stopping = atomic_fetch_or_explicit(&sync->state, SYNC_STATE_SYNCED,
                                            memory_order_relaxed) & SYNC_STATE_STOPPING;
    while (cemucore_unlikely(!timedout && !stopping))
    {
        int status;
        if (cemucore_unlikely(pthread_cond_signal(&sync->cond[SYNC_COND_WAIT_SYNCED]) ||
                              (((timedout = status = pthread_cond_timedwait(&sync->cond[SYNC_COND_SYNCED],
                                                                            &sync->mutex, &abstime))) &&
                               status != ETIMEDOUT)))
        {
            abort();
        }
        stopping = atomic_load_explicit(&sync->state, memory_order_relaxed) & SYNC_STATE_STOPPING;
    }
    return sync_leave_mask(sync, SYNC_STATE_SYNCED);
}

static uint32_t sync_inc(sync_t *sync)
{
    uint32_t state;
    if (cemucore_unlikely(cemucore_add_overflow(atomic_fetch_add_explicit(&sync->state, SYNC_STATE_COUNTER,
                                                                          memory_order_relaxed),
                                                (uint32_t)SYNC_STATE_COUNTER, &state)))
    {
        abort();
    }
    return state;
}

static uint32_t sync_dec(sync_t *sync, bool unlock)
{
    uint32_t state;
    if (cemucore_unlikely(cemucore_sub_overflow(atomic_fetch_sub_explicit(&sync->state, SYNC_STATE_COUNTER,
                                                                          memory_order_relaxed),
                                                (uint32_t)SYNC_STATE_COUNTER, &state)))
    {
        abort();
    }
    if (cemucore_likely(unlock))
    {
        sync_unlock(sync);
    }
    if (cemucore_unlikely(pthread_cond_signal(&sync->cond[state / SYNC_STATE_COUNTER ? SYNC_COND_WAIT_SYNCED
                                                                                     : SYNC_COND_SYNCED])))
    {
        abort();
    }
    return state;
}

static uint32_t sync_wait_synced(sync_t *sync)
{
    uint32_t state = sync_inc(sync);
    while (cemucore_unlikely(!(state & SYNC_STATE_SYNCED)))
    {
        if (cemucore_unlikely(pthread_cond_wait(&sync->cond[SYNC_COND_WAIT_SYNCED], &sync->mutex)))
        {
            abort();
        }
        state = atomic_load_explicit(&sync->state, memory_order_relaxed);
    }
    return state;
}

static uint32_t sync_wait_run(sync_t *sync)
{
    uint32_t state = sync_dec(sync, false);
    while (cemucore_unlikely(state / SYNC_STATE_COUNTER || state & SYNC_STATE_SYNCED))
    {
        if (cemucore_unlikely(pthread_cond_wait(&sync->cond[SYNC_COND_WAIT_RUN], &sync->mutex)))
        {
            abort();
        }
        state = atomic_load_explicit(&sync->state, memory_order_relaxed);
    }
    return state;
}

static uint32_t sync_sleep_mask(sync_t *sync, uint32_t mask)
{
    uint32_t state = atomic_fetch_and_explicit(&sync->state, ~mask, memory_order_relaxed);
    if (cemucore_likely(state & SYNC_STATE_RUNNING))
    {
        cemucore_unused(sync_inc(sync));
    }
    return state;
}

static uint32_t sync_wake_mask(sync_t *sync, uint32_t mask)
{
    uint32_t state = atomic_fetch_or_explicit(&sync->state, mask, memory_order_relaxed);
    if (cemucore_likely(!(state & SYNC_STATE_RUNNING)))
    {
        cemucore_unused(sync_dec(sync, false));
    }
    return state;
}

bool sync_sleep(sync_t *sync)
{
    return sync_sleep_mask(sync, SYNC_STATE_RUNNING) & SYNC_STATE_RUNNING;
}

bool sync_wake(sync_t *sync)
{
    return !(sync_wake_mask(sync, SYNC_STATE_RUNNING) & SYNC_STATE_RUNNING);
}

void sync_enter(sync_t *sync)
{
    sync_lock(sync);
    cemucore_unused(sync_wait_synced(sync));
}

void sync_run(sync_t *sync)
{
    cemucore_unused(sync_wait_run(sync));
    cemucore_unused(sync_wait_synced(sync));
}

void sync_leave(sync_t *sync)
{
    cemucore_unused(sync_dec(sync, true));
}

void sync_run_leave(sync_t *sync)
{
    cemucore_unused(sync_wait_run(sync));
    sync_unlock(sync);
}

void sync_stop(sync_t *sync)
{
    sync_enter(sync);
    sync_wake_mask(sync, SYNC_STATE_RUNNING | SYNC_STATE_STOPPING);
    do
    {
        if (cemucore_unlikely(pthread_cond_signal(&sync->cond[SYNC_COND_SYNCED]) ||
                              pthread_cond_wait(&sync->cond[SYNC_COND_WAIT_RUN], &sync->mutex)))
        {
            abort();
        }
    }
    while (atomic_load_explicit(&sync->state, memory_order_relaxed) & SYNC_STATE_STOPPING);
    sync_leave(sync);
}
