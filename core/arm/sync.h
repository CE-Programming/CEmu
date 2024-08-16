#ifndef SYNC_H
#define SYNC_H

#include <stdatomic.h>
#include <stdbool.h>
#include "threading.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sync {
    mtx_t mtx;
    cnd_t cnd[3];
    atomic_uint cnt;
    bool run, slp, rdy;
} sync_t;

bool sync_init(sync_t *sync);
void sync_destroy(sync_t *sync);

/* Thread-safe, any thread */
void sync_lock(sync_t *sync);
void sync_unlock(sync_t *sync);

/* Target thread or while synced only */
bool sync_check(sync_t *sync);
bool sync_loop(sync_t *sync);
void sync_sleep(sync_t *sync);
void sync_wake(sync_t *sync);

/* Thread-safe, not target thread */
void sync_enter(sync_t *sync);
void sync_run(sync_t *sync);
void sync_leave(sync_t *sync);
void sync_run_leave(sync_t *sync);

#ifdef __cplusplus
}
#endif

#endif
