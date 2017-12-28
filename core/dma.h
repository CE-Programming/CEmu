#ifndef DMA_H
#define DMA_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Lowest priority to highest priority */
enum dma_item_index {
    DMA_LCD,
    DMA_USB,
    DMA_NUM_ITEMS
};

typedef uint8_t (*dma_proc_t)(enum dma_item_index index, uint64_t when, uint64_t now);

struct dma_item {
    dma_proc_t proc;
    uint64_t when;
};

typedef struct dma_state {
    struct dma_item items[DMA_NUM_ITEMS];
    uint64_t now;
} dma_state_t;

/* Global DMA state */
extern dma_state_t dma;

void dma_delay(uint8_t pendingAccessDelay);
void dma_reset(void);

/* Save/Restore */
bool dma_restore(FILE *image);
bool dma_save(FILE *image);

#ifdef __cplusplus
}
#endif

#endif
