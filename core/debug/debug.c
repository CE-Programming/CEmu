#include "debug.h"
#include "../apb.h"
#include "../emu.h"

volatile bool in_debugger = false;

CEMU_INLINE uint8_t debug_port_read_byte(const uint32_t addr) {
    return apb_map[port_range(addr)].range->read_in(addr_range(addr));
}

/* okay, so looking at the data inside the asic should be okay when using this function, */
/* since it is called outside of cpu_execute(). Which means no read/write errors. */
void debugger(int reason, uint32_t addr) {
    gui_debugger_entered_or_left(in_debugger = true);
    gui_debugger_send_command(reason, addr);

    do {
      /* TODO: debugger stuff */
      /* Such as step, step over, etc */
        emu_sleep();
    } while(in_debugger);
    gui_debugger_entered_or_left(in_debugger = false);
}
