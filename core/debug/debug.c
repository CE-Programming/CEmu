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
    debug_clear_step();
    debug.stackIndex = debug.stackSize = 0;
    debug.stack = (debug_stack_entry_t*)calloc(DBG_STACK_SIZE, sizeof(debug_stack_entry_t));
    debug.addr = (uint8_t*)calloc(DBG_ADDR_SIZE, sizeof(uint8_t));
    debug.port = (uint8_t*)calloc(DBG_PORT_SIZE, sizeof(uint8_t));
    debug.bufPos = debug.bufErrPos = 0;
    debug.open = false;
    gui_console_printf("[CEmu] Initialized Debugger...\n");
}

void debug_free(void) {
    free(debug.stack);
    free(debug.addr);
    free(debug.port);
    gui_console_printf("[CEmu] Freed Debugger.\n");
}

bool debug_is_open(void) {
    return debug.open;
}

void debug_open(int reason, uint32_t data) {
    if (cpu.abort == CPU_ABORT_EXIT || debug.open || (debug.ignore && (reason >= DBG_BREAKPOINT && reason <= DBG_PORT_WRITE))) {
        return;
    }

    debug_clear_step();

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
}

void debug_watch(uint32_t addr, int mask, bool set) {
    addr &= 0xFFFFFF;
    if (set) {
        debug.addr[addr] |= mask;
    } else {
        debug.addr[addr] &= ~mask;
    }
}

void debug_ports(uint16_t addr, int mask, bool set) {
    addr &= 0xFFFF;
    if (set) {
        debug.port[addr] |= mask;
    } else {
        debug.port[addr] &= ~mask;
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
    switch (mode) {
        case DBG_STEP_IN:
            debug.step = true;
            break;
        case DBG_STEP_OVER:
            debug.step = debug.stepOver = true;
            break;
        case DBG_STEP_OUT:
            debug.stepOut = debug.stackIndex;
            break;
        case DBG_STEP_NEXT:
        case DBG_RUN_UNTIL:
            debug.tempExec = addr;
            break;
    }
}

void debug_clear_step(void) {
    debug.step = debug.stepOver = false;
    debug.tempExec = debug.stepOut = ~0;
}

void debug_inst_start(void) {
    uint32_t pc = cpu.registers.PC;
    debug.addr[pc] |= DBG_INST_START_MARKER;
    if (debug.step && !(debug.addr[pc] & DBG_MASK_EXEC) && pc != debug.tempExec) {
        debug.step = debug.stepOver = false;
        debug_open(DBG_STEP, cpu.registers.PC);
    }
}

void debug_inst_fetch(void) {
    uint32_t pc = cpu.registers.PC;
    debug.addr[pc] |= DBG_INST_MARKER;
    if (debug.addr[pc] & DBG_MASK_EXEC) {
        debug_open(DBG_BREAKPOINT, pc);
    } else if (pc == debug.tempExec) {
        debug_open(DBG_STEP, pc);
    }
}

void debug_inst_repeat(void) {
    if (debug.step && !debug.stepOver) {
        debug.step = false;
        debug_open(DBG_STEP, cpu.registers.PC);
    }
}

void debug_record_call(uint32_t retAddr, bool mode) {
    uint32_t stack = cpu_address_mode(cpu.registers.stack[mode].hl, mode);
    uint32_t index = (debug.stackIndex + 1) & DBG_STACK_MASK;
    debug_stack_entry_t *entry = &debug.stack[index];
    entry->mode = mode;
    entry->popped = false;
    entry->stack = stack;
    entry->retAddr = retAddr;
    entry->range = 1;
    debug.stackIndex = index;
    if (debug.stackSize < DBG_STACK_SIZE) {
        debug.stackSize++;
    }
    if (debug.stepOver) {
        debug.step = debug.stepOver = false;
        debug.stepOut = index;
    }
}

void debug_record_ret(uint32_t retAddr, bool mode) {
    uint32_t stack = cpu_address_mode(cpu.registers.stack[mode].hl, mode);
    uint32_t index = debug.stackIndex;
    uint32_t size = debug.stackSize;
    debug_stack_entry_t *entry;
    bool found = false, stepOut = false;
    while (size--) {
        stepOut |= index == debug.stepOut;
        entry = &debug.stack[index];
        index = (index - 1) & DBG_STACK_MASK;
        if (mode == entry->mode && (stack == entry->stack || entry->popped) &&
            retAddr - entry->retAddr <= entry->range) {
            debug.stackIndex = index;
            debug.stackSize = size;
            found = true;
        } else if (found) {
            break;
        }
    }
    if (found && stepOut) {
        debug.step = true;
        debug.stepOut = ~0;
    }
}

void debug_set_pc(uint32_t addr) {
    cpu_flush(addr, cpu.ADL);
}

#endif
