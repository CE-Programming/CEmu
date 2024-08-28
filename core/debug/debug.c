#ifdef DEBUG_SUPPORT

#include "debug.h"
#include "../atomics.h"
#include "../mem.h"
#include "../emu.h"
#include "../cpu.h"
#include "../flash.h"
#include "../vat.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

debug_state_t debug;

typedef struct debug_atomics {
    _Atomic(int) flags;
    _Atomic(bool) open;
} debug_atomics_t;

static debug_atomics_t debug_atomics;

void debug_init(void) {
    debug_clear_step();
    debug.stackIndex = debug.stackSize = 0;
    debug.stack = (debug_stack_entry_t*)calloc(DBG_STACK_SIZE, sizeof(debug_stack_entry_t));
    debug.addr = (uint8_t*)calloc(DBG_ADDR_SIZE, sizeof(uint8_t));
    debug.port = (uint8_t*)calloc(DBG_PORT_SIZE, sizeof(uint8_t));
    debug.bufPos = debug.bufErrPos = 0;
    debug_atomics.open = false;
    debug_disable_basic_mode();
    gui_console_printf("[CEmu] Initialized Debugger...\n");
}

void debug_free(void) {
    free(debug.stack);
    free(debug.addr);
    free(debug.port);
    gui_console_printf("[CEmu] Freed Debugger.\n");
}

bool debug_is_open(void) {
    return atomic_load_explicit(&debug_atomics.open, memory_order_relaxed);
}

int debug_get_flags(void) {
    return atomic_load_explicit(&debug_atomics.flags, memory_order_relaxed);
}

void debug_open(int reason, uint32_t data) {
    if ((cpu_check_signals() & CPU_SIGNAL_EXIT) || debug_is_open() || ((debug_get_flags() & DBG_IGNORE) && (reason >= DBG_BREAKPOINT && reason <= DBG_PORT_WRITE))) {
        return;
    }

    debug_clear_step();

    /* fixup reason for basic debugger */
    if (debug.basicMode == true) {
        if (reason == DBG_WATCHPOINT_WRITE) {
            if (data == DBG_BASIC_CURPC+2) {
                if (debug.basicDeferPC) {
                    return;
                }
                reason = DBG_BASIC_CURPC_WRITE;
            } else if (data == DBG_BASIC_BEGPC+2) {
                /* in the case where the program is returning from a subprogram, the updates
                   to the pc haven't finished yet on endpc or program name, so defer to the
                   program name write */
                debug.basicDeferPC = true;
                reason = DBG_BASIC_BEGPC_WRITE;
            } else if (data == DBG_BASIC_ENDPC+2) {
                debug.basicDeferPC = false;
                reason = DBG_BASIC_ENDPC_WRITE;
            } else if (data == DBG_BASIC_BASIC_PROG+8) {
                debug.basicDeferPC = false;
                reason = DBG_BASIC_BASIC_PROG_WRITE;
            }
        }
        if (reason == DBG_WATCHPOINT_READ) {
            if (data == DBG_BASIC_SYSHOOKFLAG2) {
                debug.basicDeferPC = false;
                /* verify basic program execution */
                if ((mem_peek_byte(DBG_BASIC_NEWDISPF) & DBG_BASIC_PROGEXECUTING_BIT) &&
                    (mem_peek_byte(DBG_BASIC_CMDFLAGS) & DBG_BASIC_CMDEXEC_BIT)) {

                    /* check current pc for instruction "bit 1,(iy+$36)" */
                    static const uint8_t instr[] = { 0xFD, 0xCB, 0x36, 0x4E };
                    const void *ptr = phys_mem_ptr(cpu.registers.PC - sizeof(instr), sizeof(instr));
                    if(ptr && !memcmp(ptr, instr, sizeof(instr))) {
                        debug.basicLastHookPC = cpu.registers.PC;
                        reason = DBG_BASIC_CURPC_WRITE;
                    }
                } else {
                    return;
                }
            }
        }
        if (debug.stepBasic && (reason == DBG_BASIC_CURPC_WRITE || reason == DBG_BASIC_BASIC_PROG_WRITE)) {
            uint32_t offset = mem_peek_long(DBG_BASIC_CURPC) - mem_peek_long(DBG_BASIC_BEGPC);
            /* Allow self-looping with Step In, but only if the hook PC is the same as what was stepped from */
            bool inRange = offset >= debug.stepBasicBegin + (!debug.stepBasicNext && debug.basicLastHookPC == debug.stepBasicFromPC) &&
                           offset <= debug.stepBasicEnd &&
                           !strncmp(debug.stepBasicPrgm, phys_mem_ptr(DBG_BASIC_BASIC_PROG, 9), 9);
            if (!inRange ^ debug.stepBasicNext) {
                reason = DBG_BASIC_STEP;
            }
        }

        if (!debug.basicModeLive && reason > DBG_BASIC_LIVE_START && reason < DBG_BASIC_LIVE_END) {
            return;
        }
    }

    debug.cpuCycles = cpu.cycles;
    debug.cpuNext = cpu.next;
    debug.cpuBaseCycles = cpu.baseCycles;
    debug.cpuHaltCycles = cpu.haltCycles;
    debug.totalCycles += sched_total_cycles();
    debug.dmaCycles += cpu.dmaCycles;
    debug.flashCacheMisses = cpu.flashCacheMisses;
    debug.flashTotalAccesses = cpu.flashTotalAccesses;
    debug.flashWaitStates = flash.waitStates;
    debug.flashDelayCycles = cpu.flashDelayCycles;

    debug_atomics.open = true;
    gui_debug_open(reason, data);
    debug_atomics.open = false;

    cpu.next = debug.cpuNext;
    cpu.cycles = debug.cpuCycles;
    cpu.baseCycles = debug.cpuBaseCycles;
    cpu.haltCycles = debug.cpuHaltCycles;
    debug.dmaCycles -= cpu.dmaCycles;
    debug.totalCycles -= sched_total_cycles();
    cpu.flashCacheMisses = debug.flashCacheMisses;
    cpu.flashTotalAccesses = debug.flashTotalAccesses;
    cpu.flashDelayCycles = debug.flashDelayCycles;
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
        debug_atomics.flags |= mask;
    } else {
        debug_atomics.flags &= ~mask;
    }
}

void debug_step(int mode, uint32_t addr) {
    switch (mode) {
        case DBG_STEP_IN:
            debug.step = true;
            break;
        case DBG_STEP_OVER:
            debug.step = true;
            debug.stepOver = true;
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
        case DBG_BASIC_STEP_IN:
        case DBG_BASIC_STEP_NEXT:
            gui_debug_close();
            debug.stepBasic = true;
            debug.stepBasicNext = (mode == DBG_BASIC_STEP_NEXT);
            debug.stepBasicFromPC = debug.basicLastHookPC;
            debug.stepBasicBegin = addr;
            debug.stepBasicEnd = addr >> 16;
            debug_get_executing_basic_prgm(debug.stepBasicPrgm);
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

/* internal breakpoints not visible in gui */
/* the gui should automatically update breakpoints, so it should be */
/* fine if asm or C also uses these addresses */
void debug_enable_basic_mode(bool fetches, bool live) {
    debug.basicMode = true;
    debug.basicModeLive = live;
    debug_watch(DBG_BASIC_BEGPC+2, DBG_MASK_WRITE, fetches);
    debug_watch(DBG_BASIC_CURPC+2, DBG_MASK_WRITE, fetches);
    //debug_watch(DBG_BASIC_ENDPC+2, DBG_MASK_WRITE, fetches);
    debug_watch(DBG_BASIC_BASIC_PROG+8, DBG_MASK_WRITE, fetches);
    debug_watch(DBG_BASIC_SYSHOOKFLAG2, DBG_MASK_READ, !fetches);
}

void debug_disable_basic_mode(void) {
    debug.basicMode = false;
    debug.basicModeLive = false;
    debug_watch(DBG_BASIC_BEGPC+2, DBG_MASK_WRITE, false);
    debug_watch(DBG_BASIC_CURPC+2, DBG_MASK_WRITE, false);
    //debug_watch(DBG_BASIC_ENDPC+2, DBG_MASK_WRITE, false);
    debug_watch(DBG_BASIC_BASIC_PROG+8, DBG_MASK_WRITE, false);
    debug_watch(DBG_BASIC_SYSHOOKFLAG2, DBG_MASK_READ, false);
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
