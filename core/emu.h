#ifndef EMU_H
#define EMU_H

#ifdef __cplusplus
extern "C" {
#endif

#include "schedule.h"
#include "bootver.h"
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
bool emu_set_run_rate(uint32_t rate);                     /* how many ticks per second for emu_run */
uint32_t emu_get_run_rate(void);                          /* getter for the above */
void emu_reset(void);                                     /* reset emulation as if the reset button was pressed */
void emu_exit(void);                                      /* exit emulation */

/* gui callbacks called by the core */
/* if you want to port CEmu to another platform, simply reimplement these callbacks */
/* if you want debugging support, don't forget about the debug callbacks as well */
void gui_console_clear(void);                             /* sent to clear the console */
void gui_console_printf(const char *format, ...);         /* printf from the core to stdout */

/* called at reset or state load, indicates hardware info and allows specifying a revision on reset
 * params:
 *   boot_ver: boot code version if found, else NULL
 *   loaded_rev: ASIC_REV_AUTO on reset, or loaded revision on state load
 *   default_rev: default revision to use when returning ASIC_REV_AUTO
 *   python: determined edition based on certificate, can be overridden by updating its value
 * returns:
 *   hardware revision to use; ignored when loading state */
asic_rev_t gui_handle_reset(const boot_ver_t* boot_ver, asic_rev_t loaded_rev, asic_rev_t default_rev, bool* python);

#ifdef DEBUG_SUPPORT
void gui_debug_open(int reason, uint32_t data);           /* open the gui debugger */
void gui_debug_close(void);                               /* disable the gui debugger if called */
#endif

#ifdef __cplusplus
}
#endif

#endif
