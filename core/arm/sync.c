#include "sync.h"

#include "../defines.h"

#include <stdlib.h>

enum {
    SYNC_CND_READY,
    SYNC_CND_RUN,
    SYNC_CND_QUANTUM,
    SYNC_CND_IDLE,
    SYNC_CND_COUNT
};

bool sync_init(sync_t *sync) {
    if (likely(mtx_init(&sync->mtx, mtx_plain) == thrd_success)) {
        unsigned int initialized = 0;
        while (initialized != SYNC_CND_COUNT &&
               likely(cnd_init(&sync->cnd[initialized]) == thrd_success)) {
            ++initialized;
        }
        if (initialized == SYNC_CND_COUNT) {
            atomic_init(&sync->cnt, 0u);
            sync->run = true;
            sync->slp = false;
            sync->thr = false;
            sync->rdy = false;
            return true;
        }
        while (initialized) {
            cnd_destroy(&sync->cnd[--initialized]);
        }
        mtx_destroy(&sync->mtx);
    }
    return false;
}

void sync_destroy(sync_t *sync) {
    for (unsigned int i = SYNC_CND_COUNT; i; ) {
        cnd_destroy(&sync->cnd[--i]);
    }
    mtx_destroy(&sync->mtx);
}

void sync_lock(sync_t *sync) {
    if (unlikely(mtx_lock(&sync->mtx) != thrd_success)) {
        abort();
    }
}

void sync_unlock(sync_t *sync) {
    if (unlikely(mtx_unlock(&sync->mtx) != thrd_success)) {
        abort();
    }
}

bool sync_check(sync_t *sync) {
    return unlikely(atomic_load_explicit(&sync->cnt, memory_order_relaxed));
}

bool sync_loop(sync_t *sync, bool throttle) {
    if (likely(!sync_check(sync) && !throttle)) {
        return true;
    }
    sync_lock(sync);
    if (throttle && !sync->thr) {
        (void)atomic_fetch_add_explicit(&sync->cnt, 1, memory_order_relaxed);
        sync->thr = true;
    }
    sync->rdy = true;
    if (sync->slp || sync->thr) {
        if (unlikely(cnd_broadcast(&sync->cnd[SYNC_CND_IDLE]) != thrd_success)) {
            abort();
        }
    }
    do {
        if (unlikely(cnd_signal(&sync->cnd[SYNC_CND_READY]) != thrd_success ||
                     cnd_wait(&sync->cnd[SYNC_CND_RUN], &sync->mtx) != thrd_success)) {
            abort();
        }
    } while (unlikely(atomic_load_explicit(&sync->cnt, memory_order_relaxed)));
    bool run = sync->run;
    sync->rdy = false;
    sync_unlock(sync);
    if (likely(run)) {
        if (unlikely(cnd_broadcast(&sync->cnd[SYNC_CND_QUANTUM]) != thrd_success)) {
            abort();
        }
        return true;
    }
    sync_destroy(sync);
    return false;
}

void sync_sleep(sync_t *sync) {
    if (likely(!sync->slp)) {
        (void)atomic_fetch_add_explicit(&sync->cnt, 1, memory_order_relaxed);
        sync->slp = true;
    }
}

void sync_wake(sync_t *sync) {
    if (likely(sync->slp)) {
        (void)atomic_fetch_sub_explicit(&sync->cnt, 1, memory_order_relaxed);
        sync->slp = false;
    }
}

void sync_throttle(sync_t *sync) {
    if (!sync->thr) {
        (void)atomic_fetch_add_explicit(&sync->cnt, 1, memory_order_relaxed);
        sync->thr = true;
    }
}

void sync_throttle_wake(sync_t *sync) {
    if (sync->thr) {
        (void)atomic_fetch_sub_explicit(&sync->cnt, 1, memory_order_relaxed);
        sync->thr = false;
    }
}

bool sync_idle(sync_t *sync) {
    return sync->slp || sync->thr;
}

static void sync_reenter(sync_t *sync) {
    (void)atomic_fetch_add_explicit(&sync->cnt, 1, memory_order_relaxed);
    while (unlikely(!sync->rdy)) {
        if (unlikely(cnd_wait(&sync->cnd[SYNC_CND_READY], &sync->mtx) != thrd_success)) {
            abort();
        }
    }
}

static void sync_maybe_leave(sync_t *sync, bool unlock) {
    bool done = atomic_fetch_sub_explicit(&sync->cnt, 1, memory_order_relaxed) == 1;
    if (unlock) {
        sync_unlock(sync);
    }
    if (unlikely(cnd_signal(&sync->cnd[done ? SYNC_CND_RUN : SYNC_CND_READY]) != thrd_success)) {
        abort();
    }
}

static void sync_wait_run(sync_t *sync) {
    sync_maybe_leave(sync, false);
    if (unlikely(cnd_wait(&sync->cnd[SYNC_CND_QUANTUM], &sync->mtx) != thrd_success)) {
        abort();
    }
}

void sync_enter(sync_t *sync) {
    sync_lock(sync);
    sync_reenter(sync);
}

void sync_run(sync_t *sync) {
    sync_wait_run(sync);
    sync_reenter(sync);
}

void sync_leave(sync_t *sync) {
    sync_maybe_leave(sync, true);
}

void sync_run_leave(sync_t *sync) {
    sync_wait_run(sync);
    sync_unlock(sync);
}

void sync_wait_idle(sync_t *sync) {
    sync_lock(sync);
    while (!sync_idle(sync)) {
        if (unlikely(cnd_wait(&sync->cnd[SYNC_CND_IDLE], &sync->mtx) != thrd_success)) {
            abort();
        }
    }
    sync_unlock(sync);
}
