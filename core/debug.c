#include "core/debug.h"
#include "core/emu.h"

debug_state_t emu_debug;

void debugger(enum DBG_REASON reason, uint32_t addr) {
    emu_debug.stopped = true;
    gui_debugger_entered();

    while( emu_debug.stopped ) {};
}
