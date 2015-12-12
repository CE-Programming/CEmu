#include "core/debug.h"
#include "core/emu.h"

volatile bool in_debugger = false;

void debugger(enum DBG_REASON reason, uint32_t addr) {
    gui_debugger_entered_or_left(in_debugger = true);

    //gdbstub_debugger(reason, addr);
    gui_debugger_entered_or_left(in_debugger = false);
}
