#include "spscqueue.h"

#include "../defines.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static_assert(SPSC_QUEUE_SIZE && !(SPSC_QUEUE_SIZE & (SPSC_QUEUE_SIZE - 1)),
              "Expected a non-zero power-of-two queue size.");

bool spsc_queue_init(spsc_queue_t *queue) {
    spsc_queue_clear(queue);
    return true;
}

void spsc_queue_clear(spsc_queue_t *queue) {
    spsc_queue_index_t index = 0;
    queue->enqueue = 0;
    queue->dequeue = 0;
    queue->poke = SPSC_QUEUE_INVALID_ENTRY;
    queue->peek = SPSC_QUEUE_INVALID_ENTRY;
    do {
        atomic_init(&queue->queue[index], SPSC_QUEUE_INVALID_ENTRY);
        index = SPSC_QUEUE_NEXT_INDEX(index);
    } while (index);
}

void spsc_queue_destroy(spsc_queue_t *queue) {
    (void)queue;
}

bool spsc_queue_flush(spsc_queue_t *queue) {
    spsc_queue_entry_t poke = queue->poke;
    bool success = true;
    if (unlikely(poke != SPSC_QUEUE_INVALID_ENTRY)) {
        spsc_queue_index_t index = SPSC_QUEUE_MASK_INDEX(queue->enqueue);
        spsc_queue_entry_t invalid = SPSC_QUEUE_INVALID_ENTRY;
        success = atomic_compare_exchange_weak_explicit(
                &queue->queue[index], &invalid, poke,
                memory_order_relaxed, memory_order_relaxed);
        if (likely(success)) {
            queue->enqueue = SPSC_QUEUE_NEXT_INDEX(index);
            queue->poke = SPSC_QUEUE_INVALID_ENTRY;
        }
    }
    return success;
}

bool spsc_queue_enqueue(spsc_queue_t *queue, spsc_queue_entry_t entry) {
    bool success;
    assert(entry != SPSC_QUEUE_INVALID_ENTRY &&
           "Cannot queue invalid entry.");
    success = spsc_queue_flush(queue);
    if (likely(success)) {
        queue->poke = entry;
    }
    return success;
}

spsc_queue_entry_t spsc_queue_peek(spsc_queue_t *queue) {
    spsc_queue_entry_t peek = queue->peek;
    if (likely(peek == SPSC_QUEUE_INVALID_ENTRY)) {
        spsc_queue_index_t index = SPSC_QUEUE_MASK_INDEX(queue->dequeue);
        peek = atomic_exchange_explicit(
                &queue->queue[index], SPSC_QUEUE_INVALID_ENTRY,
                memory_order_relaxed);
        if (likely(peek != SPSC_QUEUE_INVALID_ENTRY)) {
            queue->dequeue = SPSC_QUEUE_NEXT_INDEX(index);
            queue->peek = peek;
        }
    }
    return peek;
}

spsc_queue_entry_t spsc_queue_dequeue(spsc_queue_t *queue) {
    spsc_queue_entry_t entry = spsc_queue_peek(queue);
    queue->peek = SPSC_QUEUE_INVALID_ENTRY;
    return entry;
}
