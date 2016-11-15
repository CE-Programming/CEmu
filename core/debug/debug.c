#ifdef DEBUG_SUPPORT

#include <stdio.h>

#include "disasm.h"
#include "debug.h"
#include "profiler.h"
#include "../mem.h"
#include "../emu.h"
#include "../cpu.h"

volatile bool inDebugger = false;
debug_state_t debugger;

void debugger_init(void) {
    debugger.stepOverInstrEnd = -1;
    debugger.data.block = (uint8_t*)calloc(0x1000000, sizeof(uint8_t));    /* Allocate Debug memory */
    debugger.data.ports = (uint8_t*)calloc(0x10000, sizeof(uint8_t));      /* Allocate Debug Port Monitor */
    debugger.buffer = (char*)malloc(SIZEOF_DBG_BUFFER * sizeof(char));     /* Used for printing to the console */
    debugger.errBuffer = (char*)malloc(SIZEOF_DBG_BUFFER * sizeof(char));  /* Used for printing to the console */
    debugger.currentBuffPos = debugger.currentErrBuffPos = 0;

    gui_console_printf("[CEmu] Initialized Debugger...\n");
}

void debugger_free(void) {
    if (debugger.data.block) {
        free(debugger.data.block);
    }
    if (debugger.data.ports) {
        free(debugger.data.ports);
    }
    if (debugger.buffer) {
        free(debugger.buffer);
    }
    gui_console_printf("[CEmu] Freed Debugger.\n");
}

uint8_t debug_peek_byte(uint32_t address) {
    uint8_t value = mem_peek_byte(address), debugData;

    if ((debugData = debugger.data.block[address])) {
        disasmHighlight.hit_read_watchpoint |= debugData & DBG_READ_WATCHPOINT;
        disasmHighlight.hit_write_watchpoint |= debugData & DBG_WRITE_WATCHPOINT;
        disasmHighlight.hit_exec_breakpoint |= debugData & DBG_EXEC_BREAKPOINT;
        if (debugData & DBG_INST_START_MARKER && disasmHighlight.inst_address < 0) {
            disasmHighlight.inst_address = address;
        }
    }

    if (cpu.registers.PC == address) {
        disasmHighlight.hit_pc = true;
    }

    return value;
}

void open_debugger(int reason, uint32_t data) {
    if (inDebugger) {
        /* Prevent recurse */
        return;
    }

    if ((reason == DBG_STEP) && debugger.stepOverFirstStep) {
        if (((cpuEvents & EVENT_DEBUG_STEP_NEXT)
                && !(debugger.data.block[cpu.registers.PC] & DBG_TEMP_EXEC_BREAKPOINT)) || (cpuEvents & EVENT_DEBUG_STEP_OUT)) {
            debugger.stepOverFirstStep = false;
            gui_debugger_raise_or_disable(inDebugger = false);
            return;
        }
        debug_clear_temp_break();
    }

    debugger.cpu_cycles = cpu.cycles;
    debugger.cpu_next = cpu.next;
    debugger.total_cycles = cpu.cycles + cpu.cycles_offset;

    if (debugger.currentBuffPos) {
        debugger.buffer[debugger.currentBuffPos] = '\0';
        gui_console_printf("%s", debugger.buffer);
        debugger.currentBuffPos = 0;
    }

    if (debugger.currentErrBuffPos) {
        debugger.errBuffer[debugger.currentErrBuffPos] = '\0';
        gui_console_err_printf("%s", debugger.errBuffer);
        debugger.currentErrBuffPos = 0;
    }

    inDebugger = true;
    gui_debugger_send_command(reason, data);

    while (inDebugger) {
        gui_emu_sleep(50);
    }

    cpu.next = debugger.cpu_next;
    cpu.cycles = debugger.cpu_cycles;
    cpu.cycles_offset = debugger.total_cycles - cpu.cycles;

    if (cpuEvents & EVENT_DEBUG_STEP) {
        cpu.next = cpu.cycles + 1;
    }
}

void debug_switch_step_mode(void) {
    if (cpuEvents & EVENT_DEBUG_STEP_OVER) {
        debugger.stepOverFirstStep = true;
        debugger.stepOutSPL = cpu.registers.SPL + 1;
        debugger.stepOutSPS = cpu.registers.SPS + 1;
        debugger.stepOutWait = 0;
        cpuEvents &= ~EVENT_DEBUG_STEP_OVER;
        cpuEvents |= EVENT_DEBUG_STEP | EVENT_DEBUG_STEP_OUT;
    }
}

void debug_breakwatch(uint32_t address, unsigned int type, bool set) {
    if (set) {
        debugger.data.block[address] |= type;
    } else {
        debugger.data.block[address] &= ~type;
    }
}

void debug_init_run_until(uint32_t address) {
    debugger.runUntilAddress = address;
}

void debug_clear_temp_break(void) {
    cpuEvents &= ~(EVENT_DEBUG_STEP | EVENT_DEBUG_STEP_OUT | EVENT_DEBUG_STEP_OVER);
    if (debugger.stepOverInstrEnd != 0xFFFFFFFFU) {
        do {
            debugger.data.block[debugger.stepOverInstrEnd] &= ~DBG_TEMP_EXEC_BREAKPOINT;
            debugger.stepOverInstrEnd = cpu_mask_mode(debugger.stepOverInstrEnd - 1, debugger.stepOverMode);
        } while (debugger.data.block[debugger.stepOverInstrEnd] & DBG_TEMP_EXEC_BREAKPOINT);
    }
    debugger.stepOverInstrEnd = 0xFFFFFFFFU;
}

void debug_set_pc_address(uint32_t address) {
    cpu_flush(address, cpu.ADL);
}

void debug_breakpoint_remove(uint32_t address) {
    debug_breakwatch(address, ~DBG_NO_HANDLE, false);
}

void debug_pmonitor_set(uint16_t address, unsigned int type, bool set) {
    if (set) {
        debugger.data.ports[address] |= type;
    } else {
        debugger.data.ports[address] &= ~type;
    }
}

void debug_pmonitor_remove(uint16_t address) {
    debug_pmonitor_set(address, ~DBG_NO_HANDLE, false);
}

#endif
