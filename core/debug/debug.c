#ifdef DEBUG_SUPPORT

#include <stdio.h>

#include "disasm.h"
#include "debug.h"
#include "../mem.h"
#include "../emu.h"
#include "../asic.h"

volatile bool inDebugger = false;
debug_state_t debugger;

void debugger_init(void) {
    debugger.stepOverInstrEnd = -1;
    debugger.data.block = (uint8_t*)calloc(0x1000000, sizeof(uint8_t));    /* Allocate Debug memory */
    debugger.data.ports = (uint8_t*)calloc(0x10000, sizeof(uint8_t));      /* Allocate Debug Port Monitor */
    debugger.buffer = (char*)calloc(SIZEOF_DBG_BUFFER, sizeof(char));    /* Used for printing to the console */
    debugger.currentBuffPos = 0;

    debugger.runUntilSet = false;
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
uint8_t debug_read_byte(uint32_t address) {
    uint8_t *ptr, value = 0, debugData;

    address &= 0xFFFFFF;
    if (address < 0xE00000) {
        if ((ptr = phys_mem_ptr(address, 1))) {
            value = *ptr;
        }
    } else {
        value = debug_port_read_byte(mmio_range(address)<<12 | addr_range(address));
    }

    if ((debugData = debugger.data.block[address])) {
        disasmHighlight.hit_read_breakpoint |= debugData & DBG_READ_BREAKPOINT;
        disasmHighlight.hit_write_breakpoint |= debugData & DBG_WRITE_BREAKPOINT;
        disasmHighlight.hit_exec_breakpoint |= debugData & DBG_EXEC_BREAKPOINT;
        disasmHighlight.hit_run_breakpoint |= debugData & DBG_RUN_UNTIL_BREAKPOINT;
        if (debugData & DBG_INST_START_MARKER && disasmHighlight.inst_address < 0) {
            disasmHighlight.inst_address = address;
        }
    }

    if (cpu.registers.PC == address) {
        disasmHighlight.hit_pc = true;
    }

    return value;
}
uint16_t debug_read_short(uint32_t address) {
    return debug_read_byte(address)
         | debug_read_byte(address + 1) << 8;
}
uint32_t debug_read_long(uint32_t address) {
    return debug_read_byte(address)
         | debug_read_byte(address + 1) << 8
         | debug_read_byte(address + 2) << 16;
}
uint32_t debug_read_word(uint32_t address, bool mode) {
    return mode ? debug_read_long(address) : debug_read_short(address);
}
void debug_write_byte(uint32_t address, uint8_t value) {
    uint8_t *ptr;
    address &= 0xFFFFFF;
    if (address < 0xE00000) {
        if ((ptr = phys_mem_ptr(address, 1))) {
            *ptr = value;
        }
    } else {
        debug_port_write_byte(mmio_range(address)<<12 | addr_range(address), value);
    }
}

uint8_t debug_port_read_byte(uint32_t address) {
    return apb_map[port_range(address)].range->read_in(addr_range(address));
}
void debug_port_write_byte(uint32_t address, uint8_t value) {
    apb_map[port_range(address)].range->write_out(addr_range(address), value);
}

/* okay, so looking at the data inside the asic should be okay when using this function, */
/* since it is called outside of cpu_execute(). Which means no read/write errors. */
void open_debugger(int reason, uint32_t data) {
    if (inDebugger) {
        return; // don't recurse
    }

    fprintf(stderr,
            "[open_debugger] Opening debugger for reason %i, cpuEvents=0x%02x, stepOverFirstStep=%i, stepOverCall=%i\n",
            reason, cpuEvents, debugger.stepOverFirstStep, debugger.stepOverCall);

    if ((reason == DBG_STEP) && debugger.stepOverFirstStep) {
        if (((cpuEvents & EVENT_DEBUG_STEP_NEXT)
                && !(debugger.data.block[cpu.registers.PC] & DBG_STEP_OVER_BREAKPOINT)) || (cpuEvents & EVENT_DEBUG_STEP_OUT)) {
            debugger.stepOverFirstStep = false;
            fprintf(stderr, "[open_debugger] stepOverFirstStep=false\n");
            gui_debugger_raise_or_disable(inDebugger = false);
            return;
        }
        debug_clear_step_over();
    }

    debugger.cpu_cycles = cpu.cycles;
    debugger.cpu_next = cpu.next;
    gui_debugger_raise_or_disable(inDebugger = true);
    gui_debugger_send_command(reason, data);

    do {
        gui_emu_sleep();
    } while(inDebugger);

    if (debugger.stepOverRequested) {
        debug_clear_step_over();
    }

    cpu.next = debugger.cpu_next;
    cpu.cycles = debugger.cpu_cycles;

    if (cpuEvents & EVENT_DEBUG_STEP) {
        cpu.next = debugger.cpu_cycles + 1;
    }
}

void debug_switch_step_mode(void) {
    if (cpuEvents & EVENT_DEBUG_STEP_OVER) {
        debugger.stepOverFirstStep = true;
        debugger.stepOutSPL = cpu.registers.SPL + 1;
        debugger.stepOutSPS = cpu.registers.SPS + 1;
        debugger.stepOutWait = 0;
        fprintf(stderr, "[setDebugStepOutMode] stepOverFirstStep=true\n");
        fprintf(stderr, "[setDebugStepOutMode] stepOverInstrEnd=0x%08x\n", debugger.stepOverInstrEnd);
        fprintf(stderr, "[setDebugStepOutMode] stepOutSPL=0x%08x\n", debugger.stepOutSPL);
        fprintf(stderr, "[setDebugStepOutMode] stepOutSPS=0x%08x\n", debugger.stepOutSPS);
        fprintf(stderr, "[setDebugStepOutMode] stepOutWait=%i\n", debugger.stepOutWait);
        cpuEvents &= ~EVENT_DEBUG_STEP_OVER;
        cpuEvents |= EVENT_DEBUG_STEP | EVENT_DEBUG_STEP_OUT;
    }
}

void debug_breakpoint_set(uint32_t address, unsigned int type, bool set) {
    if (set) {
        debugger.data.block[address] |= type;
    } else {
        debugger.data.block[address] &= ~type;
    }
}

void debug_toggle_run_until(uint32_t address) {
    if (address == debugger.runUntilAddress) {
        debugger.data.block[address] &= ~DBG_RUN_UNTIL_BREAKPOINT;
        debugger.runUntilAddress = 0xFFFFFFFF;
        debugger.runUntilSet = false;
    } else {
        if (debugger.runUntilSet) {
            debugger.data.block[debugger.runUntilAddress] &= ~DBG_RUN_UNTIL_BREAKPOINT;
        }
        debugger.data.block[address] |= DBG_RUN_UNTIL_BREAKPOINT;
        debugger.runUntilAddress = address;
        debugger.runUntilSet = true;
    }
}

void debug_clear_run_until(void) {
    if (debugger.runUntilSet == true) {
        debugger.data.block[debugger.runUntilAddress] &= ~DBG_RUN_UNTIL_BREAKPOINT;
        debugger.runUntilAddress = 0xFFFFFFFF;
        debugger.runUntilSet = false;
    }
}

void debug_clear_step_over(void) {
    fprintf(stderr, "[debug_clear_step_over] Clearing step over(?) at 0x%08x\n", debugger.stepOverInstrEnd);
    cpuEvents &= ~(EVENT_DEBUG_STEP_OVER | EVENT_DEBUG_STEP_NEXT);
    if (debugger.stepOverInstrEnd < 0x1000000) {
        for (int i = debugger.stepOverInstrEnd - debugger.stepOverInstrSize;
                i <= (int)(debugger.stepOverInstrEnd + debugger.stepOverExtendSize); i++) {
            debugger.data.block[i & 0xFFFFFF] &= ~DBG_STEP_OVER_BREAKPOINT;
            debugger.data.block[i & 0xFFFF] &= ~DBG_STEP_OVER_BREAKPOINT;
        }
        fprintf(stderr, "[debug_clear_step_over] Cleared step over at 0x%08x\n", debugger.stepOverInstrEnd);
        debugger.stepOverInstrEnd = -1;
    }
}

void debug_set_pc_address(uint32_t address) {
    cpu_flush(address, cpu.ADL);
}

void debug_breakpoint_remove(uint32_t address) {
    debug_breakpoint_set(address, ~DBG_NO_HANDLE, false);
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
