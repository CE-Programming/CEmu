#ifndef SPSCQUEUE_H
#define SPSCQUEUE_H

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <threads.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t spsc_queue_index_t;
typedef uint32_t spsc_queue_entry_t;

#define SPSC_QUEUE_SIZE 0x10
#define SPSC_QUEUE_MASK_INDEX(x) ((spsc_queue_index_t)(x & (SPSC_QUEUE_SIZE - 1)))
#define SPSC_QUEUE_NEXT_INDEX(x) SPSC_QUEUE_MASK_INDEX(x + 1)
#define SPSC_QUEUE_INVALID_ENTRY ((spsc_queue_entry_t)~0)

typedef struct spsc_queue {
    spsc_queue_index_t enqueue, dequeue;
    spsc_queue_entry_t poke, peek;
    _Atomic(spsc_queue_entry_t) queue[SPSC_QUEUE_SIZE];
} spsc_queue_t;

bool spsc_queue_init(spsc_queue_t *queue);
void spsc_queue_clear(spsc_queue_t *queue);
void spsc_queue_destroy(spsc_queue_t *queue);

/* Producer thread only */
bool spsc_queue_flush(spsc_queue_t *queue);
bool spsc_queue_enqueue(spsc_queue_t *queue, spsc_queue_entry_t entry);

/* Consumer thread only */
spsc_queue_entry_t spsc_queue_peek(spsc_queue_t *queue);
spsc_queue_entry_t spsc_queue_dequeue(spsc_queue_t *queue);

#ifdef __cplusplus
}
#endif

#endif
