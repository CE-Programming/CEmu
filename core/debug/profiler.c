#ifdef DEBUG_SUPPORT

#include <stdlib.h>
#include "profiler.h"
#include "debug.h"

profiler_t profiler;

void profiler_init(void) {
    profiler.granularity = 0;
    profiler.blocks = NULL;
    profiler.num_blocks = 0;
    profiler.profile_counters = calloc(0x1000000 >> profiler.granularity, sizeof(uint64_t));
}

void set_profiler_granularity(unsigned gran) {
    profiler_free();
    profiler.granularity = gran;
    profiler.profile_counters = calloc(0x1000000 >> profiler.granularity, sizeof(uint64_t));
}

void profiler_free(void) {
    if (profiler.blocks) {
        unsigned i = 0, j;
        while (profiler.blocks[i]) {
            /* clear it from the debugging block */
            for(j = profiler.blocks[i]->start_addr; j < profiler.blocks[i]->end_addr; j++) {
                debugger.data.block[j] &= ~DBG_IS_PROFILED;
            }
            free(profiler.blocks[i]);
            profiler.blocks[i] = NULL;
            i++;
        }
    }
    free(profiler.profile_counters);
    free(profiler.blocks);
    profiler.profile_counters = NULL;
    profiler.blocks = NULL;
    profiler.num_blocks = 0;
}

profiler_block_t *add_profile_block(uint32_t start_addr, uint32_t end_addr, uint64_t cycles) {
    unsigned num = profiler.num_blocks;

    profiler.blocks = realloc(profiler.blocks, (num+2)*(sizeof(profiler_block_t*)));
    if (profiler.blocks) {
        profiler.blocks[num] = malloc(sizeof(profiler_block_t));
        if (profiler.blocks[num]) {
            profiler.blocks[num]->start_addr = start_addr;
            profiler.blocks[num]->end_addr = end_addr;
            profiler.blocks[num]->cycles = cycles;
        } else {
            /* Just die if we get here */
            abort();
        }
    } else {
        /* Just die if we get here too */
        abort();
    }
    profiler.num_blocks++;
    profiler.blocks[profiler.num_blocks] = NULL;
    return profiler.blocks[num];
}

void update_profiler_block(unsigned block, uint32_t start_addr, uint32_t end_addr) {
    uint32_t i;
    for(i = profiler.blocks[block]->start_addr; i < profiler.blocks[block]->end_addr; i++) {
        debugger.data.block[i] &= ~DBG_IS_PROFILED;
        profiler.profile_counters[i] = 0;
    }
    for(i = start_addr; i < end_addr; i++) {
        debugger.data.block[i] |= DBG_IS_PROFILED;
    }
    profiler.blocks[block]->start_addr = start_addr;
    profiler.blocks[block]->end_addr = end_addr;
}

void update_profiler_cycles(void) {
    unsigned i, j;
    for(i = 0; i < profiler.num_blocks; i++) {
        for(j = profiler.blocks[i]->start_addr; j < profiler.blocks[i]->end_addr; j++) {
            profiler.blocks[i]->cycles += profiler.profile_counters[j];
        }
    }
}

void remove_profile_block(uint32_t block_entry) {
    uint32_t pos = block_entry+1, i;
    if (!profiler.num_blocks) {
        return;
    }
    /* clear it from the debugging block */
    for(i = profiler.blocks[block_entry]->start_addr; i < profiler.blocks[block_entry]->end_addr; i++) {
        debugger.data.block[i] &= ~DBG_IS_PROFILED;
    }
    free(profiler.blocks[block_entry]);
    profiler.blocks[block_entry] = NULL;
    while (profiler.blocks[pos]) {
        profiler.blocks[pos-1] = profiler.blocks[pos];
        pos++;
    }
    profiler.blocks[profiler.num_blocks] = NULL;
    profiler.blocks = realloc(profiler.blocks, (profiler.num_blocks+1)*(sizeof(profiler_block_t*)));
    if (!profiler.blocks) {
        abort();
    }
    profiler.num_blocks--;
}

#endif
