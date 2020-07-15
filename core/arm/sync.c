#include "sync.h"

#include "../defines.h"

#include <stdlib.h>

bool sync_init(sync_t *sync) {
    if (likely(mtx_init(&sync->mtx, mtx_plain) == thrd_success)) {
        if (likely(cnd_init(&sync->cnd[false]) == thrd_success)) {
            if (likely(cnd_init(&sync->cnd[true]) == thrd_success)) {
                if (likely(cnd_init(&sync->cnd[2]) == thrd_success)) {
                    atomic_init(&sync->cnt, 0u);
                    sync->run = true;
                    sync->slp = false;
                    sync->rdy = false;
                    return true;
                    cnd_destroy(&sync->cnd[2]);
                }
                cnd_destroy(&sync->cnd[true]);
            }
            cnd_destroy(&sync->cnd[false]);
        }
        mtx_destroy(&sync->mtx);
    }
    return false;
}

void sync_destroy(sync_t *sync) {
    cnd_destroy(&sync->cnd[2]);
    cnd_destroy(&sync->cnd[true]);
    cnd_destroy(&sync->cnd[false]);
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

bool sync_loop(sync_t *sync) {
    if (likely(!sync_check(sync))) {
        return true;
    }
    sync_lock(sync);
    sync->rdy = true;
    do {
        if (unlikely(cnd_signal(&sync->cnd[false]) != thrd_success ||
                     cnd_wait(&sync->cnd[true], &sync->mtx) != thrd_success)) {
            abort();
        }
    } while (unlikely(atomic_load_explicit(&sync->cnt, memory_order_relaxed)));
    bool run = sync->run;
    sync->rdy = false;
    sync_unlock(sync);
    if (likely(run)) {
        if (unlikely(cnd_broadcast(&sync->cnd[2]) != thrd_success)) {
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

static void sync_reenter(sync_t *sync) {
    (void)atomic_fetch_add_explicit(&sync->cnt, 1, memory_order_relaxed);
    while (unlikely(!sync->rdy)) {
        if (unlikely(cnd_wait(&sync->cnd[false], &sync->mtx) != thrd_success)) {
            abort();
        }
    }
}

static void sync_maybe_leave(sync_t *sync, bool unlock) {
    bool done = atomic_fetch_sub_explicit(&sync->cnt, 1, memory_order_relaxed) == 1;
    if (unlock) {
        sync_unlock(sync);
    }
    if (unlikely(cnd_signal(&sync->cnd[done]) != thrd_success)) {
        abort();
    }
}

static void sync_wait_run(sync_t *sync) {
    sync_maybe_leave(sync, false);
    if (unlikely(cnd_wait(&sync->cnd[2], &sync->mtx) != thrd_success)) {
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
