#ifndef ARMSTATE_H
#define ARMSTATE_H

#include "arm.h"
#include "armcpu.h"
#include "armmem.h"
#include "spscqueue.h"
#include "sync.h"

#include <stdbool.h>
#include <stddef.h>
#include "threading.h"

#ifdef COPROC_DEBUG_SUPPORT
enum { ARM_DEBUG_MAX_BREAKPOINTS = 64 };

typedef struct arm_debug_state {
    uint32_t breakpoints[ARM_DEBUG_MAX_BREAKPOINTS];
    size_t breakpoint_count;
    uint32_t skip_breakpoint;
    arm_debug_stop_reason_t stop_reason;
    bool attached;
    bool stopped;
    bool step_pending;
    bool skip_breakpoint_once;
} arm_debug_state_t;
#endif

struct arm {
    sync_t sync;
    arm_cpu_t cpu;
    arm_mem_t mem;
    spsc_queue_t usart[2];
    thrd_t thrd;
    uint64_t cycles;
    uint64_t cycle_limit;
#ifdef COPROC_DEBUG_SUPPORT
    arm_debug_state_t gdb;
#endif
    bool debug;
};

#endif
