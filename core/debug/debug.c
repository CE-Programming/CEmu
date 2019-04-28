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
    debug_set_mode(DBG_MODE_ASM);
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
    if (debug.mode == DBG_MODE_BASIC && !debug.basicLiveExecution) {
        return;
    }

    debug_clear_step();

    debug.cpuCycles = cpu.cycles;
    debug.cpuNext = cpu.next;
    debug.cpuBaseCycles = cpu.baseCycles;
    debug.cpuHaltCycles = cpu.haltCycles;
    debug.totalCycles += sched_total_cycles();
    debug.dmaCycles += cpu.dmaCycles;

    /* fixup reason for basic debugger */
    if (debug.mode == DBG_MODE_BASIC && debug.basicLiveExecution) {
        if (reason == DBG_WATCHPOINT_WRITE) {
            if (data == DBG_BASIC_CURPC + 1) {
                reason = DBG_BASIC_CURPC_WRITE;
            }
            else if (data == DBG_BASIC_BEGPC + 1) {
                reason = DBG_BASIC_BEGPC_WRITE;
            }
            else if (data == DBG_BASIC_ENDPC + 1) {
                reason = DBG_BASIC_ENDPC_WRITE;
            }
        }
    }

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
            gui_debug_close();
            debug.stepOut = debug.stackIndex;
            break;
        case DBG_STEP_NEXT:
        case DBG_RUN_UNTIL:
            gui_debug_close();
            debug.tempExec = addr;
            break;
        case DBG_BASIC_STEP:
            gui_debug_close();
            debug.stepBasic = true;
            break;
    }
}

void debug_clear_step(void) {
    debug.step = debug.stepOver = false;
    debug.tempExec = debug.stepOut = ~0u;
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
    if (debug.step) {
        if (debug.stepOver) {
            gui_debug_close();
        } else {
            debug.step = false;
            debug_open(DBG_STEP, cpu.registers.PC);
        }
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
        gui_debug_close();
        debug.step = debug.stepOver = false;
        debug.stepOut = index;
    }
}

void debug_record_ret(uint32_t retAddr, bool mode) {
    uint32_t stack = cpu_address_mode(cpu.registers.stack[mode].hl, mode),
        index = debug.stackIndex, size = debug.stackSize;
    debug_stack_entry_t *entry;
    bool found = false, stepOut = false, stepOutMatch;
    while (size--) {
        stepOutMatch = index == debug.stepOut;
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
        stepOut |= stepOutMatch;
    }
    if (found && stepOut) {
        debug.step = true;
        debug.stepOut = ~0u;
    }
}

void debug_set_pc(uint32_t addr) {
    cpu_flush(addr, cpu.ADL);
}

#define DBG_BASIC_NEWDISPF 0xD00088
#define DBG_BASIC_CMDFLAGS 0xD0008C
#define DBG_BASIC_CMDEXEC_BIT (1 << 6)
#define DBG_BASIC_PROGEXECUTING_BIT (1 << 1)

/* internal breakpoints not visible in gui */
/* the gui should automatically update breakpoints, so it should be */
/* fine if asm or C also uses these addresses */
void debug_set_mode(debug_mode_t mode) {
    debug_watch(DBG_BASIC_BEGPC + 1, DBG_MASK_WRITE, mode == DBG_MODE_BASIC);
    debug_watch(DBG_BASIC_CURPC + 1, DBG_MASK_WRITE, mode == DBG_MODE_BASIC);
    debug_watch(DBG_BASIC_ENDPC + 1, DBG_MASK_WRITE, mode == DBG_MODE_BASIC);
    debug.mode = mode;
}

bool debug_get_executing_basic_prgm(char *name) {
    if (name == NULL) {
        return false;
    }

    /* check if a basic program is executing */
    if ((mem_peek_byte(DBG_BASIC_NEWDISPF) & DBG_BASIC_PROGEXECUTING_BIT) &&
        (mem_peek_byte(DBG_BASIC_CMDFLAGS) & DBG_BASIC_CMDEXEC_BIT)) {

        /* return the executing program */
        virt_mem_cpy(name, DBG_BASIC_BASIC_PROG, 9);
        name[9] = '\0';

        return true;
    } else {
        return false;
    }
}

#endif
