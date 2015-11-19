/* Declarations for debug.c */
#ifndef DEBUG_H
#define DEBUG_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#include "core/asic.h"

#ifdef __cplusplus
extern "C" {
#endif

extern volatile bool in_debugger;

enum DBG_REASON {
    DBG_USER,
    DBG_EXCEPTION,
    DBG_EXEC_BREAKPOINT,
    DBG_READ_BREAKPOINT,
    DBG_WRITE_BREAKPOINT,
};

void *virt_mem_ptr(uint32_t addr, uint32_t size);
void debugger(enum DBG_REASON reason, uint32_t addr);

#ifdef __cplusplus
}
#endif

#endif
