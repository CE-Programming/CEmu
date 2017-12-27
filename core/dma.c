#include "dma.h"
#include "cpu.h"
#include "emu.h"

#include <string.h>

dma_state_t dma;

void dma_schedule(enum dma_item_index index, uint64_t when, dma_callback_t callback) {
    dma.items[index].when = when >= dma.next ? when : dma.next;
    dma.items[index].callback = callback;
}
void dma_delay(uint8_t pendingAccessDelay) {
    int i;
    while (dma.next < cpu_cycles()) {
        for (i = 0; i < DMA_NUM_ITEMS; i++) {
            if (dma.items[i].when < cpu_cycles()) {
                dma.next += dma.items[i].callback(i, dma.next);
            }
        }
    }
}

void dma_reset(void) {
    memset(&dma, 0, sizeof(dma));
}

bool dma_save(emu_image *s) {
    s->dma = dma;
    return true;
}

bool dma_restore(const emu_image *s) {
    dma = s->dma;
    return true;
}
