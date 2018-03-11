#ifndef EMU_H
#define EMU_H

#ifdef __cplusplus
extern "C" {
#endif

#include "schedule.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

extern volatile bool exiting;

enum {
    EMU_LOAD_OKAY,
    EMU_LOAD_FAIL,
    EMU_LOAD_NOTROM,
    EMU_LOAD_RESTORED
};

/* emulator functions for frontend use */
int emu_load(bool image, const char *path);             /* load an emulator rom or image, returns an integer in the above enum */
bool emu_save(bool image, const char *path);            /* save an emulator rom or image */
void emu_loop(void);                                    /* core emulation loop, call after emu_load */
void emu_exit(void);                                    /* exit emulation, not thread safe! */
void emu_throttle_event(enum sched_item_id id);         /* throttle event, no need to call or use */

/* gui callbacks called by the core */
/* if you want to port CEmu to another platform, simply reimplement these callbacks */
/* if you want debugging support, don't forget about the debug callbacks as well */
void gui_do_stuff(void);                                /* perform tasks such as sending files, opening debugger */
void gui_emu_sleep(unsigned long microseconds);         /* sleep for the specified microseconds (not critical to have) */
void gui_console_printf(const char *format, ...);       /* printf from the core to stdout */
void gui_console_err_printf(const char *format, ...);   /* printf from the core to stderr */
void gui_throttle(void);                                /* throttling to get correct emulation speed */

#ifdef DEBUG_SUPPORT
void gui_debug_open(int reason, uint32_t data);         /* open the gui debugger */
void gui_debug_close(void);                             /* disable the gui debugger if called */
#endif

#ifdef __cplusplus
}
#endif

#endif
