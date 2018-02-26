#ifdef DEBUG_SUPPORT

#include <stdio.h>

#include "disasm.h"
#include "debug.h"
#include "../mem.h"
#include "../emu.h"
#include "../cpu.h"

volatile bool inDebugger = false;
debug_state_t debugger;

#ifndef __EMSCRIPTEN__
#include <condition_variable>
static std::mutex debugM;
static std::condition_variable debugCV;
#endif

void debugger_init(void) {
    debugger.stepOverInstrEnd = -1;
    debugger.data.block = (uint8_t*)calloc(0x1000000, 1);    /* Allocate Debug memory */
    debugger.data.ports = (uint8_t*)calloc(0x10000, 1);      /* Allocate Debug Port Monitor */
    debugger.buffer = (char*)malloc(SIZEOF_DBG_BUFFER);      /* Used for printing to the console */
    debugger.bufferErr = (char*)malloc(SIZEOF_DBG_BUFFER);   /* Used for printing to the console */
    debugger.bufferPos = 0;
    debugger.bufferErrPos = 0;

    gui_console_printf("[CEmu] Initialized Debugger...\n");
}

void debugger_free(void) {
    free(debugger.data.block);
    free(debugger.data.ports);
    free(debugger.buffer);
    free(debugger.bufferErr);
    gui_console_printf("[CEmu] Freed Debugger.\n");
}

uint8_t debug_peek_byte(uint32_t addr) {
    uint8_t value = mem_peek_byte(addr), data;

    if ((data = debugger.data.block[addr])) {
        disasmHighlight.rWatch |= data & DBG_MASK_READ ? true : false;
        disasmHighlight.wWatch |= data & DBG_MASK_WRITE ? true : false;
        disasmHighlight.xBreak |= data & DBG_MASK_EXEC ? true : false;
        if (data & DBG_INST_START_MARKER && disasmHighlight.addr < 0) {
            disasmHighlight.addr = addr;
        }
    }

    if (cpu.registers.PC == addr) {
        disasmHighlight.pc = true;
    }

    return value;
}

void close_debugger(void) {
#ifndef __EMSCRIPTEN__
    debugM.lock();
    inDebugger = false;
    debugCV.notify_all();
    debugM.unlock();
#else
    inDebugger = false;
#endif
}

void open_debugger(int reason, uint32_t data) {
    if (exiting || inDebugger || (debugger.ignore && (reason >= DBG_EXEC_BREAKPOINT && reason <= DBG_PORT_WRITE))) {
        return;
    }

    if (reason == DBG_STEP && debugger.stepOverFirstStep) {
        if ((cpu.events & EVENT_DEBUG_STEP_OUT) ||
           ((cpu.events & EVENT_DEBUG_STEP_NEXT) && !(debugger.data.block[cpu.registers.PC] & DBG_MASK_TEMP_EXEC))) {
            debugger.stepOverFirstStep = false;
            gui_debugger_raise_or_disable(false);
            return;
        }
        debug_clear_temp_break();
    }

    if (reason == DBG_EXEC_BREAKPOINT && (cpu.events & EVENT_DEBUG_STEP)) {
        return;
    }

    debugger.cpuCycles = cpu.cycles;
    debugger.cpuNext = cpu.next;
    debugger.cpuBaseCycles = cpu.baseCycles;
    debugger.cpuHaltCycles = cpu.haltCycles;
    debugger.totalCycles += sched_total_cycles();
    debugger.dmaCycles += cpu.dmaCycles;

    if (debugger.bufferPos) {
        debugger.buffer[debugger.bufferPos] = '\0';
        gui_console_printf("%s", debugger.buffer);
        debugger.bufferPos = 0;
    }

    if (debugger.bufferErrPos) {
        debugger.bufferErr[debugger.bufferErrPos] = '\0';
        gui_console_err_printf("%s", debugger.bufferErr);
        debugger.bufferErrPos = 0;
    }

#ifndef __EMSCRIPTEN__
    std::unique_lock<std::mutex> lock(debugM);
#endif
    gui_debugger_send_command(reason, data);
    inDebugger = true;
#ifndef __EMSCRIPTEN__
    while (inDebugger) {
        debugCV.wait(lock);
    }
#endif

    cpu.next = debugger.cpuNext;
    cpu.cycles = debugger.cpuCycles;
    cpu.baseCycles = debugger.cpuBaseCycles;
    cpu.haltCycles = debugger.cpuHaltCycles;
    debugger.dmaCycles -= cpu.dmaCycles;
    debugger.totalCycles -= sched_total_cycles();

    if (cpu.events & EVENT_DEBUG_STEP && !cpu.halted) {
        cpu.next = cpu.cycles + 1;
    }
}

void debug_switch_step_mode(void) {
    if (cpu.events & EVENT_DEBUG_STEP_OVER) {
        debugger.stepOverFirstStep = true;
        debugger.stepOutSPL = cpu.registers.SPL + 1;
        debugger.stepOutSPS = cpu.registers.SPS + 1;
        debugger.stepOutWait = 0;
        cpu.events &= ~EVENT_DEBUG_STEP_OVER;
        cpu.events |= EVENT_DEBUG_STEP | EVENT_DEBUG_STEP_OUT;
    }
}

void debug_breakwatch(uint32_t address, unsigned int mask, bool set) {
    if (set) {
        debugger.data.block[address] |= mask;
    } else {
        debugger.data.block[address] &= ~mask;
    }
}

void debug_clear_temp_break(void) {
    cpu.events &= ~(EVENT_DEBUG_STEP | EVENT_DEBUG_STEP_OUT | EVENT_DEBUG_STEP_OVER);
    if (debugger.stepOverInstrEnd != 0xFFFFFFFFU) {
        do {
            debugger.data.block[debugger.stepOverInstrEnd] &= ~DBG_MASK_TEMP_EXEC;
            debugger.stepOverInstrEnd = cpu_mask_mode(debugger.stepOverInstrEnd - 1, debugger.stepOverMode);
        } while (debugger.data.block[debugger.stepOverInstrEnd] & DBG_MASK_TEMP_EXEC);
    }
    debugger.stepOverInstrEnd = 0xFFFFFFFFU;
}

void debug_set_pc_address(uint32_t address) {
    cpu_flush(address, cpu.ADL);
}

void debug_breakpoint_remove(uint32_t address) {
    debug_breakwatch(address, ~DBG_MASK_NONE, false);
}

void debug_pmonitor_set(uint16_t address, unsigned int type, bool set) {
    if (set) {
        debugger.data.ports[address] |= type;
    } else {
        debugger.data.ports[address] &= ~type;
    }
}

void debug_pmonitor_remove(uint16_t address) {
    debug_pmonitor_set(address, ~DBG_MASK_NONE, false);
}

void debug_set_step_mode(int mode) {
    debug_clear_temp_break();
    disasm.baseAddress = cpu.registers.PC;
    disasm.adl = cpu.ADL;
    disassembleInstruction();
    debugger.stepOverInstrEnd = disasm.newAddress;
    debugger.stepOverFirstStep = false;
    debugger.stepOverMode = cpu.ADL;
    debugger.stepOutSPL = 0;
    debugger.stepOutSPS = 0;
    debugger.stepOutWait = -1;

    switch (mode) {
        case DBG_STEP_IN:
            cpu.events |= EVENT_DEBUG_STEP;
            break;
        case DBG_STEP_OVER:
            cpu.events |= EVENT_DEBUG_STEP | EVENT_DEBUG_STEP_OVER;
            break;
        case DBG_STEP_NEXT:
            debugger.stepOverFirstStep = true;
            cpu.events |= EVENT_DEBUG_STEP | EVENT_DEBUG_STEP_NEXT;
            break;
        case DBG_STEP_OUT:
            debugger.stepOverFirstStep = true;
            debugger.stepOutSPL = cpu.registers.SPL + 1;
            debugger.stepOutSPS = cpu.registers.SPS + 1;
            debugger.stepOutWait = 0;
            cpu.events |= EVENT_DEBUG_STEP | EVENT_DEBUG_STEP_OUT;
            break;
        case DBG_RUN_UNTIL:
            debugger.stepOverInstrEnd = debugger.runUntilAddress;
            cpu.events &= ~(EVENT_DEBUG_STEP | EVENT_DEBUG_STEP_OVER | EVENT_DEBUG_STEP_OUT | EVENT_DEBUG_STEP_NEXT);
            break;
        default:
            break;
    }

    if (mode != DBG_STEP_OUT) {
        debugger.data.block[debugger.stepOverInstrEnd] |= DBG_MASK_TEMP_EXEC;
    }
}

#endif
