#ifdef DEBUG_SUPPORT

#include <stdlib.h>
#include "profiler.h"

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
    unsigned i = 0;
    while(profiler.blocks[i++]) {
        free(profiler.blocks[i]);
        profiler.blocks[i] = NULL;
    }
    free(profiler.profile_counters);
    free(profiler.blocks);
    profiler.blocks = NULL;
    profiler.num_blocks = 0;
}

profiler_block_t *add_profile_block(void) {
    profiler.blocks = (profiler_block_t**)realloc(profiler.blocks, (profiler.num_blocks+2)*(sizeof(profiler_block_t*)));
    if(profiler.blocks) {
        profiler.blocks[profiler.num_blocks] = (profiler_block_t*)malloc(sizeof(profiler_block_t));
        if(profiler.blocks[profiler.num_blocks]) {
            profiler.blocks[profiler.num_blocks]->address_start = 0;
            profiler.blocks[profiler.num_blocks]->address_end = 0;
            profiler.blocks[profiler.num_blocks]->cycles = 0;
        } else {
            /* Just die if we get here */
            free(profiler.blocks[profiler.num_blocks]);
            abort();
        }
    } else {
        /* Just die if we get here too */
        free(profiler.blocks);
        abort();
    }
    profiler.num_blocks++;
    profiler.blocks[profiler.num_blocks] = NULL;
    return profiler.blocks[profiler.num_blocks-1];
}

void remove_profile_block(uint32_t block_entry) {
    uint32_t pos = block_entry+1;
    if(!profiler.num_blocks) {
        return;
    }
    free(profiler.blocks[block_entry]);
    profiler.blocks[block_entry] = NULL;
    while(profiler.blocks[pos]) {
        profiler.blocks[pos-1] = profiler.blocks[pos];
        pos++;
    }
    profiler.blocks[profiler.num_blocks] = NULL;
    profiler.num_blocks--;
}

#endif
