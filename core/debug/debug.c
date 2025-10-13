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
    if ((cpu_check_signals() & CPU_SIGNAL_EXIT) || debug_is_open()) {
        return;
    }

    /* fixup reason for basic debugger */
    if (debug.basicMode) {
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
            bool inRange = offset >= (uint32_t)debug.stepBasicBegin + (!debug.stepBasicNext && debug.basicLastHookPC == debug.stepBasicFromPC) &&
                           offset <= (uint32_t)debug.stepBasicEnd &&
                           !strncmp(debug.stepBasicPrgm, phys_mem_ptr(DBG_BASIC_BASIC_PROG, 9), 9);
            if (!inRange ^ debug.stepBasicNext) {
                reason = DBG_BASIC_STEP;
            }
        }

        if (!debug.basicModeLive && reason > DBG_BASIC_LIVE_START && reason < DBG_BASIC_LIVE_END) {
            return;
        }
    }

    if ((debug_get_flags() & DBG_IGNORE) && (reason >= DBG_BREAKPOINT && reason <= DBG_REG_WRITE)) {
        return;
    }

    debug_clear_step();

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

static bool reg_bit_get(const uint64_t mask, const unsigned id) {
    return (id < 64) && ((mask >> id) & 1u);
}

static void reg_bit_set(uint64_t *mask, const unsigned id, const bool set) {
    if (id >= 64) { return; }
    if (set) {
        *mask |= (1ull << id);
    } else {
        *mask &= ~(1ull << id);
    }
}

#define BIT(x) (1ull << (x))
static const uint64_t dbg_reg_trigger_mask[DBG_REG_COUNT] = {
    [DBG_REG_A]   = BIT(DBG_REG_A)  | BIT(DBG_REG_AF),
    [DBG_REG_F]   = BIT(DBG_REG_F)  | BIT(DBG_REG_AF),
    [DBG_REG_AF]  = BIT(DBG_REG_AF) | BIT(DBG_REG_A)  | BIT(DBG_REG_F),

    [DBG_REG_B]   = BIT(DBG_REG_B)  | BIT(DBG_REG_BC),
    [DBG_REG_C]   = BIT(DBG_REG_C)  | BIT(DBG_REG_BC),
    [DBG_REG_BC]  = BIT(DBG_REG_BC) | BIT(DBG_REG_B)  | BIT(DBG_REG_C),

    [DBG_REG_D]   = BIT(DBG_REG_D)  | BIT(DBG_REG_DE),
    [DBG_REG_E]   = BIT(DBG_REG_E)  | BIT(DBG_REG_DE),
    [DBG_REG_DE]  = BIT(DBG_REG_DE) | BIT(DBG_REG_D)  | BIT(DBG_REG_E),

    [DBG_REG_H]   = BIT(DBG_REG_H)  | BIT(DBG_REG_HL),
    [DBG_REG_L]   = BIT(DBG_REG_L)  | BIT(DBG_REG_HL),
    [DBG_REG_HL]  = BIT(DBG_REG_HL) | BIT(DBG_REG_H)  | BIT(DBG_REG_L),

    [DBG_REG_IXH] = BIT(DBG_REG_IXH) | BIT(DBG_REG_IX),
    [DBG_REG_IXL] = BIT(DBG_REG_IXL) | BIT(DBG_REG_IX),
    [DBG_REG_IX]  = BIT(DBG_REG_IX)  | BIT(DBG_REG_IXH) | BIT(DBG_REG_IXL),

    [DBG_REG_IYH] = BIT(DBG_REG_IYH) | BIT(DBG_REG_IY),
    [DBG_REG_IYL] = BIT(DBG_REG_IYL) | BIT(DBG_REG_IY),
    [DBG_REG_IY]  = BIT(DBG_REG_IY)  | BIT(DBG_REG_IYH) | BIT(DBG_REG_IYL),

    [DBG_REG_AP]  = BIT(DBG_REG_AP)  | BIT(DBG_REG_AFP),
    [DBG_REG_FP]  = BIT(DBG_REG_FP)  | BIT(DBG_REG_AFP),
    [DBG_REG_AFP] = BIT(DBG_REG_AFP) | BIT(DBG_REG_AP)  | BIT(DBG_REG_FP),

    [DBG_REG_BP]  = BIT(DBG_REG_BP)  | BIT(DBG_REG_BCP),
    [DBG_REG_CP]  = BIT(DBG_REG_CP)  | BIT(DBG_REG_BCP),
    [DBG_REG_BCP] = BIT(DBG_REG_BCP) | BIT(DBG_REG_BP)  | BIT(DBG_REG_CP),

    [DBG_REG_DP]  = BIT(DBG_REG_DP)  | BIT(DBG_REG_DEP),
    [DBG_REG_EP]  = BIT(DBG_REG_EP)  | BIT(DBG_REG_DEP),
    [DBG_REG_DEP] = BIT(DBG_REG_DEP) | BIT(DBG_REG_DP)  | BIT(DBG_REG_EP),

    [DBG_REG_HP]  = BIT(DBG_REG_HP)  | BIT(DBG_REG_HLP),
    [DBG_REG_LP]  = BIT(DBG_REG_LP)  | BIT(DBG_REG_HLP),
    [DBG_REG_HLP] = BIT(DBG_REG_HLP) | BIT(DBG_REG_HP)  | BIT(DBG_REG_LP),

    [DBG_REG_SPS]   = BIT(DBG_REG_SPS),
    [DBG_REG_SPL]   = BIT(DBG_REG_SPL),
    [DBG_REG_PC]    = BIT(DBG_REG_PC),
    [DBG_REG_I]     = BIT(DBG_REG_I),
    [DBG_REG_R]     = BIT(DBG_REG_R),
    [DBG_REG_MBASE] = BIT(DBG_REG_MBASE),
};

uint32_t debug_norm_reg_value(const unsigned regID, const uint32_t value) {
    switch (regID) {
        // 8 bit regs
        case DBG_REG_A: case DBG_REG_F: case DBG_REG_B: case DBG_REG_C:
        case DBG_REG_D: case DBG_REG_E: case DBG_REG_H: case DBG_REG_L:
        case DBG_REG_IXH: case DBG_REG_IXL: case DBG_REG_IYH: case DBG_REG_IYL:
        case DBG_REG_AP: case DBG_REG_FP: case DBG_REG_BP: case DBG_REG_CP:
        case DBG_REG_DP: case DBG_REG_EP: case DBG_REG_HP: case DBG_REG_LP:
        case DBG_REG_R: case DBG_REG_MBASE:
            return value & 0xFFu;
        // 16 bit regs
        case DBG_REG_AF: case DBG_REG_AFP: case DBG_REG_SPS: case DBG_REG_I:
            return value & 0xFFFFu;
        // 24 bit regs
        case DBG_REG_BC: case DBG_REG_BCP: case DBG_REG_DE: case DBG_REG_DEP:
        case DBG_REG_HL: case DBG_REG_HLP: case DBG_REG_IX: case DBG_REG_IY:
        case DBG_REG_SPL: case DBG_REG_PC:
            return value & 0xFFFFFFu;
        default:
            return value;
    }
}

void debug_reg_watch(const unsigned regID, const int mask, const bool set) {
    if (mask & DBG_MASK_READ) {
        reg_bit_set(&debug.reg_watch_r, regID, set);
    }

    if (mask & DBG_MASK_WRITE) {
        reg_bit_set(&debug.reg_watch_w, regID, set);
    }
}

int debug_reg_get_mask(const unsigned regID) {
    int mask = 0;

    if (reg_bit_get(debug.reg_watch_r, regID)) {
        mask |= DBG_MASK_READ;
    }

    if (reg_bit_get(debug.reg_watch_w, regID)) {
        mask |= DBG_MASK_WRITE;
    }

    return mask;
}

void debug_touch_reg_read(const unsigned regID) {
    if (unlikely(debug.reg_watch_r & dbg_reg_trigger_mask[regID])) {
        debug_open(DBG_REG_READ, regID);
    }
}

void debug_touch_reg_write(const unsigned regID, const uint32_t oldValue, const uint32_t new_value) {
    if (unlikely(debug.reg_watch_w & dbg_reg_trigger_mask[regID])) {
        const uint32_t old_v = debug_norm_reg_value(regID, oldValue);
        const uint32_t new_v = debug_norm_reg_value(regID, new_value);
        if (old_v != new_v) {
            debug_open(DBG_REG_WRITE, regID);
        }
    }
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

void debug_clear_basic_step(void) {
    debug.stepBasic = false;
    debug.stepBasicNext = false;
}

void debug_inst_start(void) {
    uint32_t pc = cpu.registers.PC;
    debug.addr[pc] |= DBG_INST_START_MARKER;
    if (unlikely(debug.step)) {
        if (!(debug.addr[pc] & DBG_MASK_EXEC) && pc != debug.tempExec) {
            debug.step = debug.stepOver = false;
            debug_open(DBG_STEP, cpu.registers.PC);
        }
    }
}

void debug_inst_fetch(void) {
    uint32_t pc = cpu.registers.PC;
    debug.addr[pc] |= DBG_INST_MARKER;
    if (unlikely(debug.addr[pc] & DBG_MASK_EXEC)) {
        debug_open(DBG_BREAKPOINT, pc);
    } else if (unlikely(pc == debug.tempExec)) {
        debug_open(DBG_STEP, pc);
    }
}

void debug_inst_repeat(void) {
    if (unlikely(debug.step)) {
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
    debug_clear_basic_step();
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
