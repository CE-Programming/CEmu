#include "sync.h"

#include "../defines.h"

#include <stdlib.h>

bool sync_init(sync_t *sync) {
    if (likely(mtx_init(&sync->mtx, mtx_plain) == thrd_success)) {
        if (likely(cnd_init(&sync->cnd[0]) == thrd_success)) {
            if (likely(cnd_init(&sync->cnd[1]) == thrd_success)) {
                atomic_init(&sync->cnt, 0u);
                sync->run = true;
                sync->rdy = false;
                return true;
                cnd_destroy(&sync->cnd[1]);
            }
            cnd_destroy(&sync->cnd[0]);
        }
        mtx_destroy(&sync->mtx);
    }
    return false;
}

void sync_destroy(sync_t *sync) {
    cnd_destroy(&sync->cnd[1]);
    cnd_destroy(&sync->cnd[0]);
    mtx_destroy(&sync->mtx);
}

bool sync_check(sync_t *sync) {
    if (likely(!atomic_load_explicit(&sync->cnt, memory_order_relaxed))) {
        return true;
    }
    if (unlikely(mtx_lock(&sync->mtx) != thrd_success)) {
        abort();
    }
    sync->rdy = true;
    do {
        if (cnd_signal(&sync->cnd[0]) != thrd_success ||
            cnd_wait(&sync->cnd[1], &sync->mtx) != thrd_success) {
            abort();
        }
    } while (unlikely(atomic_load_explicit(&sync->cnt, memory_order_relaxed)));
    bool run = sync->run;
    sync->rdy = false;
    if (unlikely(mtx_unlock(&sync->mtx) != thrd_success)) {
        abort();
    }
    if (likely(run)) {
        return true;
    }
    sync_destroy(sync);
    return false;
}

void sync_enter(sync_t *sync) {
    if (unlikely(mtx_lock(&sync->mtx) != thrd_success)) {
        abort();
    }
    (void)atomic_fetch_add_explicit(&sync->cnt, 1, memory_order_relaxed);
    while (unlikely(!sync->rdy)) {
        if (unlikely(cnd_wait(&sync->cnd[0], &sync->mtx) != thrd_success)) {
            abort();
        }
    }
}

void sync_leave(sync_t *sync) {
    bool done = atomic_fetch_sub_explicit(&sync->cnt, 1, memory_order_relaxed) == 1;
    if (unlikely(mtx_unlock(&sync->mtx) != thrd_success ||
                 cnd_signal(&sync->cnd[done]) != thrd_success)) {
        abort();
    }
}
