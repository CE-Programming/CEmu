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

typedef enum {
    EMU_STATE_VALID,
    EMU_STATE_INVALID,
    EMU_STATE_NOT_A_CE
} emu_state_t;

typedef enum {
    EMU_DATA_IMAGE,
    EMU_DATA_ROM,
    EMU_DATA_RAM,
} emu_data_t;

/* emulator functions for frontend use */
/* these should only be called from the emulation thread if multithreaded */
emu_state_t emu_load(emu_data_t type, const char *path);  /* load an emulator state */
bool emu_save(emu_data_t type, const char *path);         /* save an emulator state */
void emu_run(uint64_t ticks);                             /* core emulation function, call after emu_load */
void emu_set_run_rate(uint32_t rate);                     /* how many ticks per second for emu_run */
void emu_reset(void);                                     /* reset emulation as if the reset button was pressed */
void emu_exit(void);                                      /* exit emulation */

/* gui callbacks called by the core */
/* if you want to port CEmu to another platform, simply reimplement these callbacks */
/* if you want debugging support, don't forget about the debug callbacks as well */
void gui_console_clear(void);                             /* sent to clear the console */
void gui_console_printf(const char *format, ...);         /* printf from the core to stdout */
void gui_console_err_printf(const char *format, ...);     /* printf from the core to stderr */

#ifdef DEBUG_SUPPORT
void gui_debug_open(int reason, uint32_t data);           /* open the gui debugger */
void gui_debug_close(void);                               /* disable the gui debugger if called */
#endif

#ifdef __cplusplus
}
#endif

#endif
