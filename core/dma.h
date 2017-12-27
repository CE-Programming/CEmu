#ifndef DMA_H
#define DMA_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

enum dma_item_index {
    DMA_LCD,
    DMA_USB,
    DMA_NUM_ITEMS
};

typedef uint8_t (*dma_callback_t)(enum dma_item_index index, uint64_t when);

struct dma_item {
    uint64_t when;
    dma_callback_t callback;
};

typedef struct dma_state {
    struct dma_item items[DMA_NUM_ITEMS];
    uint64_t next;
} dma_state_t;

/* Global DMA state */
extern dma_state_t dma;

void dma_schedule(enum dma_item_index index, uint64_t when, dma_callback_t callback);
void dma_delay(uint8_t pendingAccessDelay);

void dma_reset(void);

/* Save/Restore */
typedef struct emu_image emu_image;
bool dma_restore(const emu_image*);
bool dma_save(emu_image*);

#ifdef __cplusplus
}
#endif

#endif
