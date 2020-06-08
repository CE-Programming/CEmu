#ifndef SYNC_H
#define SYNC_H

#include <stdatomic.h>
#include <stdbool.h>
#include <threads.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sync {
    mtx_t mtx;
    cnd_t cnd[2];
    bool run, rdy;
    atomic_uint cnt;
} sync_t;

bool sync_init(sync_t *sync);
void sync_destroy(sync_t *sync);

/* Target thread only */
bool sync_check(sync_t *sync);

/* Thread-safe, not target thread */
void sync_enter(sync_t *sync);
void sync_leave(sync_t *sync);

#ifdef __cplusplus
}
#endif

#endif
