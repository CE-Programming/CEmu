#ifdef DEBUG_SUPPORT

#ifndef PROFILER_H
#define PROFILER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../defines.h"
#include "../port.h"

#define PROFILE_GRANULARITY 0

typedef struct {
    uint32_t address_start;
    uint32_t address_end;
    uint64_t cycles;
} profiler_block_t;

typedef struct {
    uint64_t *profile_counters;
    uint32_t num_blocks;
    profiler_block_t **blocks;
    unsigned granularity;
} profiler_t;

extern profiler_t profiler;
profiler_block_t *add_profile_block(void);
void remove_profile_block(uint32_t block_entry);
void set_profiler_granularity(unsigned gran);
void profiler_init(void);
void profiler_free(void);

#ifdef __cplusplus
}
#endif

#endif

#endif
