#include "dma.h"
#include "cpu.h"
#include "emu.h"
#include "debug/debug.h"

#include <stdlib.h>
#include <string.h>

dma_state_t dma;

void dma_delay(uint8_t pendingAccessDelay) {
    int i, nexti;
    uint64_t now = dma.now, cycles = cpu_cycles(), next;
    dma_callback_t callback;
    while (true) {
        nexti = DMA_NUM_ITEMS;
        next = now > cycles ? now : cycles;
        for (i = 0; i < DMA_NUM_ITEMS; i++) {
            if (dma.items[i].callback && dma.items[i].when <= next) {
                nexti = i;
                next = dma.items[i].when;
            }
        }
        if (nexti == DMA_NUM_ITEMS) {
            dma.now = next;
            next -= cycles;
            cpu.cycles += next + pendingAccessDelay;
#ifdef DEBUG_SUPPORT
            if (debugger.ignoreDmaCycles) {
                debugger.cycleCount -= next;
            }
#endif
            break;
        }
        callback = dma.items[nexti].callback;
        dma.items[nexti].callback = NULL;
        if (now < next) {
            now = next;
        }
        now += callback(nexti, now);
    }
}

void dma_reset(void) {
    memset(&dma, 0, sizeof(dma));
}

bool dma_save(FILE *image) {
    return fwrite(&dma, sizeof(dma), 1, image) == 1;
}

bool dma_restore(FILE *image) {
    return fread(&dma, sizeof(dma), 1, image) == 1;
}
