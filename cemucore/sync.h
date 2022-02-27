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

#ifndef SYNC_H
#define SYNC_H

#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

typedef enum sync_cond
{
    SYNC_COND_SYNCED,
    SYNC_COND_WAIT_SYNCED,
    SYNC_COND_WAIT_RUN,
    SYNC_COND_COUNT,
} sync_cond_t;

typedef struct sync
{
    clockid_t clock;
    pthread_mutex_t mutex;
    pthread_cond_t cond[SYNC_COND_COUNT];
    _Atomic uint32_t state;
} sync_t;

#ifdef __cplusplus
extern "C"
{
#endif

bool sync_init(sync_t *sync);
void sync_destroy(sync_t *sync);

/* Thread-safe, any thread */
void sync_lock(sync_t *sync);
void sync_unlock(sync_t *sync);
bool sync_sleep(sync_t *sync);
bool sync_wake(sync_t *sync);

/* Target thread or while synced only */
bool sync_loop(sync_t *sync);
bool sync_delay(sync_t *sync, const struct timespec *delay);

/* Thread-safe, not target thread */
void sync_enter(sync_t *sync);
void sync_run(sync_t *sync);
void sync_leave(sync_t *sync);
void sync_run_leave(sync_t *sync);
void sync_stop(sync_t *sync);

#ifdef __cplusplus
}
#endif

#endif
