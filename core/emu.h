#ifndef EMU_H
#define EMU_H

#include "atomics.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "schedule.h"
#include "asic.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

enum {
    EMU_LOAD_OKAY,
    EMU_LOAD_FAIL,
    EMU_LOAD_NOT_A_CE
};

/* emulator functions for frontend use */
/* these should only be called from the emulation thread if multithreaded */
int emu_load(bool image, const char *path);             /* load an emulator rom or image, returns an integer in the above enum */
bool emu_save(bool image, const char *path);            /* save an emulator rom or image */
void emu_loop(void);                                    /* core emulation loop, call after emu_load */
void emu_reset(void);                                   /* reset emulation as if the reset button was pressed */
void emu_exit(void);                                    /* exit emulation */

/* gui callbacks called by the core */
/* if you want to port CEmu to another platform, simply reimplement these callbacks */
/* if you want debugging support, don't forget about the debug callbacks as well */
void gui_do_stuff(void);                                /* perform tasks such as sending files, opening debugger */
void gui_throttle(void);                                /* throttling to get correct emulation speed */
void gui_console_printf(const char *format, ...);       /* printf from the core to stdout */
void gui_console_err_printf(const char *format, ...);   /* printf from the core to stderr */

#ifdef DEBUG_SUPPORT
void gui_debug_open(int reason, uint32_t data);         /* open the gui debugger */
void gui_debug_close(void);                             /* disable the gui debugger if called */
#endif

#ifdef __cplusplus
}
#endif

#endif
