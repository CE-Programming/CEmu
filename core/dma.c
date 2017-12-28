#include "dma.h"
#include "cpu.h"
#include "emu.h"
#include "debug/debug.h"

#include <stdlib.h>
#include <string.h>

dma_state_t dma;

void dma_delay(uint8_t pendingAccessDelay) {
    unsigned int i, nexti;
    uint64_t now = dma.now, cycles = cpu_cycles(), next;

    cpu.cycles += pendingAccessDelay;
    return;

    while (true) {
        nexti = DMA_NUM_ITEMS;
        next = now > cycles ? now : cycles;
        for (i = 0; i < DMA_NUM_ITEMS; i++) {
            if (dma.items[i].when <= next) {
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
        if (now < next) {
            now = next;
        }
        dma.items[nexti].when = ~0;
        if (dma.items[nexti].proc) {
            now += dma.items[nexti].proc(nexti, next, now);
        }
    }
}

void dma_reset(void) {
    unsigned int i;
    for (i = 0; i < DMA_NUM_ITEMS; i++) {
        dma.items[i].proc = NULL;
        dma.items[i].when = ~0;
    }
    dma.now = 0;
}

bool dma_save(FILE *image) {
    return fwrite(&dma, sizeof(dma), 1, image) == 1;
}

bool dma_restore(FILE *image) {
    bool ret;
    unsigned int i;
    dma_proc_t procs[DMA_NUM_ITEMS];

    for (i = 0; i < DMA_NUM_ITEMS; i++) {
        procs[i] = dma.items[i].proc;
    }

    ret = fread(&dma, sizeof(dma), 1, image) == 1;

    for (i = 0; i < DMA_NUM_ITEMS; i++) {
        dma.items[i].proc = procs[i];
    }

    return ret;
}
