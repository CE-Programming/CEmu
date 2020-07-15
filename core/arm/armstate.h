#ifndef ARMSTATE_H
#define ARMSTATE_H

#include "arm.h"
#include "armcpu.h"
#include "armmem.h"
#include "spscqueue.h"
#include "sync.h"

#include <stdbool.h>
#include <threads.h>

struct arm {
    sync_t sync;
    arm_cpu_t cpu;
    arm_mem_t mem;
    spsc_queue_t usart[2];
    thrd_t thrd;
    bool debug;
};

#endif
