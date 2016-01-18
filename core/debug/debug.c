#ifndef EMBEDED_DEVICE

#include "debug.h"
#include "disasm.h"
#include "../mem.h"
#include "../emu.h"
#include "../asic.h"

volatile bool in_debugger = false;
debug_state_t debugger;

void debugger_init(void) {
    debugger.stepOverAddress = -1;
    debugger.data.block = (uint8_t*)calloc(0x1000000, sizeof(uint8_t));    /* Allocate Debug memory */
    debugger.data.ports = (uint8_t*)calloc(0x10000, sizeof(uint8_t));      /* Allocate Debug Port Monitor */

    debugger.runUntilSet = false;
    gui_console_printf("Initialized Debugger...\n");
}

void debugger_free(void) {
    if (debugger.data.block) {
        free(debugger.data.block);
    }
    if (debugger.data.ports) {
        free(debugger.data.ports);
    }
    gui_console_printf("Freed Debugger.\n");
}
uint8_t debug_read_byte(uint32_t address) {
    uint8_t *ptr, value = 0;

    address &= 0xFFFFFF;
    if (address < 0xE00000) {
        if ((ptr = phys_mem_ptr(address, 1))) {
            value = *ptr;
        }
    } else {
        value = debug_port_read_byte(mmio_range(address)<<12 | addr_range(address));
    }

    if (debugger.data.block[address]) {
        disasmHighlight.hit_read_breakpoint = debugger.data.block[address] & DBG_READ_BREAKPOINT;
        disasmHighlight.hit_write_breakpoint = debugger.data.block[address] & DBG_WRITE_BREAKPOINT;
        disasmHighlight.hit_exec_breakpoint = debugger.data.block[address] & DBG_EXEC_BREAKPOINT;
        disasmHighlight.hit_run_breakpoint = debugger.data.block[address] & DBG_RUN_UNTIL_BREAKPOINT;
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
void openDebugger(int reason, uint32_t address) {
    debugger.cpu_cycles = cpu.cycles;
    gui_debugger_entered_or_left(in_debugger = true);

    if (debugger.stepOverAddress < 0x1000000) {
        debugger.data.block[debugger.stepOverAddress] &= ~DBG_STEP_OVER_BREAKPOINT;
        debugger.stepOverAddress = UINT32_C(0xFFFFFFFF);
    }

    gui_debugger_send_command(reason, address);

    do {
        gui_emu_sleep();
    } while(in_debugger);

    gui_debugger_entered_or_left(in_debugger = false);
    cpu.cycles = debugger.cpu_cycles;
    if (cpu_events & EVENT_DEBUG_STEP) {
        cpu.next = cpu.cycles + 1;
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

void debug_set_pc_address(uint32_t address) {
    cpu_flush(address, cpu.ADL);
}

void debug_breakpoint_remove(uint32_t address, unsigned int type) {
    debug_breakpoint_set(address, type, false);
}

void debug_pmonitor_set(uint16_t address, unsigned int type, bool set) {
    if (set) {
        debugger.data.ports[address] |= type;
    } else {
        debugger.data.ports[address] &= ~type;
    }
}

void debug_pmonitor_remove(uint16_t address, unsigned int type) {
    debug_pmonitor_set(address, type, false);
}

#endif
