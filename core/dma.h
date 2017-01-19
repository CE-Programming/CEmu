#ifndef DMA_H
#define DMA_H

#include "defines.h"

PACK(typedef struct dma_state {
    uint64_t ram;
    uint64_t lcd;
    uint64_t usb;
}) dma_state_t;

extern dma_state_t dma;

#endif
