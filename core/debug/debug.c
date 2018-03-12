#ifdef DEBUG_SUPPORT

#include "debug.h"
#include "../mem.h"
#include "../emu.h"
#include "../cpu.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

debug_state_t debug;

void debug_init(void) {
    debug.stepOverEnd = 0xFFFFFFFFU;
    debug.addr = (uint8_t*)calloc(0x1000000, 1);
    debug.port = (uint8_t*)calloc(0x10000, 1);
    debug.bufPos = debug.bufErrPos = 0;
    debug.open = false;
    gui_console_printf("[CEmu] Initialized Debugger...\n");
}

void debug_free(void) {
    free(debug.addr);
    free(debug.port);
    gui_console_printf("[CEmu] Freed Debugger.\n");
}

bool debug_is_open(void) {
    return debug.open;
}

void debug_open(int reason, uint32_t data) {
    if (exiting || debug.open || (debug.ignore && (reason >= DBG_BREAKPOINT && reason <= DBG_PORT_WRITE))) {
        return;
    }

    if (reason == DBG_STEP && debug.stepOverFirstStep) {
        if ((cpu.events & EVENT_DEBUG_STEP_OUT) ||
           ((cpu.events & EVENT_DEBUG_STEP_NEXT) && !(debug.addr[cpu.registers.PC] & DBG_MASK_TEMP))) {
            debug.stepOverFirstStep = false;
            gui_debug_close();
            return;
        }
        debug_step_reset();
    }

    if (reason == DBG_BREAKPOINT && (cpu.events & EVENT_DEBUG_STEP)) {
        return;
    }

    debug_step_reset();

    debug.cpuCycles = cpu.cycles;
    debug.cpuNext = cpu.next;
    debug.cpuBaseCycles = cpu.baseCycles;
    debug.cpuHaltCycles = cpu.haltCycles;
    debug.totalCycles += sched_total_cycles();
    debug.dmaCycles += cpu.dmaCycles;

    debug.open = true;
    gui_debug_open(reason, data);
    debug.open = false;

    cpu.next = debug.cpuNext;
    cpu.cycles = debug.cpuCycles;
    cpu.baseCycles = debug.cpuBaseCycles;
    cpu.haltCycles = debug.cpuHaltCycles;
    debug.dmaCycles -= cpu.dmaCycles;
    debug.totalCycles -= sched_total_cycles();

    if (cpu.events & EVENT_DEBUG_STEP && !cpu.halted) {
        cpu.next = cpu.cycles + 1;
    }
}

void debug_watch(uint32_t addr, int mask, bool set) {
    if (set) {
        debug.addr[addr] |= mask;
    } else {
        debug.addr[addr] &= ~mask;
    }
}

void debug_ports(uint16_t addr, int mask, bool set) {
    if (set) {
        debug.addr[addr] |= mask;
    } else {
        debug.addr[addr] &= ~mask;
    }
}

void debug_flag(int mask, bool set) {
    if (set) {
        debug.flags |= mask;
    } else {
        debug.flags &= ~mask;
    }
    debug.ignore = debug.flags & DBG_IGNORE;
    debug.commands = debug.flags & DBG_SOFT_COMMANDS;
    debug.openOnReset = debug.flags & DBG_OPEN_ON_RESET;
}

void debug_step(int mode, uint32_t addr) {
    debug_step_reset();
    debug.stepOverEnd = addr;
    debug.stepOverFirstStep = false;
    debug.stepOverMode = cpu.ADL;
    debug.stepOutSPL = 0;
    debug.stepOutSPS = 0;
    debug.stepOutWait = -1;

    switch (mode) {
        case DBG_STEP_IN:
            cpu.events |= EVENT_DEBUG_STEP;
            break;
        case DBG_STEP_OVER:
            cpu.events |= EVENT_DEBUG_STEP | EVENT_DEBUG_STEP_OVER;
            break;
        case DBG_STEP_NEXT:
            debug.stepOverFirstStep = true;
            cpu.events |= EVENT_DEBUG_STEP | EVENT_DEBUG_STEP_NEXT;
            break;
        case DBG_STEP_OUT:
            debug.stepOverFirstStep = true;
            debug.stepOutSPL = cpu.registers.SPL + 1;
            debug.stepOutSPS = cpu.registers.SPS + 1;
            debug.stepOutWait = 0;
            cpu.events |= EVENT_DEBUG_STEP | EVENT_DEBUG_STEP_OUT;
            break;
        case DBG_RUN_UNTIL:
            debug.stepOverEnd = addr;
            cpu.events &= ~(EVENT_DEBUG_STEP | EVENT_DEBUG_STEP_OVER | EVENT_DEBUG_STEP_OUT | EVENT_DEBUG_STEP_NEXT);
            break;
        default:
            break;
    }

    if (mode != DBG_STEP_OUT) {
        debug.addr[debug.stepOverEnd] |= DBG_MASK_TEMP;
    }
}

void debug_step_switch(void) {
    if (cpu.events & EVENT_DEBUG_STEP_OVER) {
        debug.stepOverFirstStep = true;
        debug.stepOutSPL = cpu.registers.SPL + 1;
        debug.stepOutSPS = cpu.registers.SPS + 1;
        debug.stepOutWait = 0;
        cpu.events &= ~EVENT_DEBUG_STEP_OVER;
        cpu.events |= EVENT_DEBUG_STEP | EVENT_DEBUG_STEP_OUT;
    }
}

void debug_step_reset(void) {
    cpu.events &= ~(EVENT_DEBUG_STEP | EVENT_DEBUG_STEP_OUT | EVENT_DEBUG_STEP_OVER);
    if (debug.stepOverEnd != 0xFFFFFFFFU) {
        do {
            debug.addr[debug.stepOverEnd] &= ~DBG_MASK_TEMP;
            debug.stepOverEnd = cpu_mask_mode(debug.stepOverEnd - 1, debug.stepOverMode);
        } while (debug.addr[debug.stepOverEnd] & DBG_MASK_TEMP);
    }
    debug.stepOverEnd = 0xFFFFFFFFU;
}

void debug_set_pc(uint32_t addr) {
    cpu_flush(addr, cpu.ADL);
}

#endif
