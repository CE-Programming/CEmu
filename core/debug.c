#include "core/debug.h"
#include "core/emu.h"

volatile bool in_debugger = false;

/* okay, so looking at the data inside the asic should be okay when using this function, */
/* since it is called outside of cpu_execute(). Which means no read/write errors. */
void debugger(enum DBG_REASON reason, uint32_t addr) {
    gui_debugger_entered_or_left(in_debugger = true);
    do {
      /* TODO: debugger stuff */
      /* Such as step, step over, etc */
        sleep();
    } while(in_debugger);
    gui_debugger_entered_or_left(in_debugger = false);
}
