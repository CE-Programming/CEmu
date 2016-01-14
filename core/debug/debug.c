#include "debug.h"
#include "../emu.h"
#include "../asic.h"

volatile bool in_debugger = false;
debug_state_t debugger;

uint8_t debug_port_read_byte(const uint32_t addr) {
    return apb_map[port_range(addr)].range->read_in(addr_range(addr));
}

/* okay, so looking at the data inside the asic should be okay when using this function, */
/* since it is called outside of cpu_execute(). Which means no read/write errors. */
void openDebugger(int reason, uint32_t addr) {
    debugger.cpu_cycles = cpu.cycles;
    gui_debugger_entered_or_left(in_debugger = true);

    if (debugger.stepOverAddress < 0x1000000) {
        debugger.data.block[debugger.stepOverAddress] &= ~DBG_STEP_OVER_BREAKPOINT;
        debugger.stepOverAddress = UINT32_C(0xFFFFFFFF);
    }

    gui_debugger_send_command(reason, addr);

    do {
        gui_emu_sleep();
    } while(in_debugger);

    gui_debugger_entered_or_left(in_debugger = false);
    cpu.cycles = debugger.cpu_cycles;
    if (cpu_events & EVENT_DEBUG_STEP) {
        cpu.next = cpu.cycles + 1;
    }
}
