#ifndef EMU_H
#define EMU_H

#ifdef __cplusplus
extern "C" {
#endif

#include "schedule.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#define EVENT_NONE            0
#define EVENT_RESET           (1 << 0)
#ifdef DEBUG_SUPPORT
#define EVENT_DEBUG_STEP      (1 << 1)
#define EVENT_DEBUG_STEP_OVER (1 << 2)
#define EVENT_DEBUG_STEP_NEXT (1 << 3)
#define EVENT_DEBUG_STEP_OUT  (1 << 4)
#else
#define EVENT_DEBUG_STEP      0
#define EVENT_DEBUG_STEP_OVER 0
#define EVENT_DEBUG_STEP_NEXT 0
#define EVENT_DEBUG_STEP_OUT  0
#endif
#define EVENT_WAITING         (1 << 5)

/* Settings */
extern volatile bool exiting;
extern volatile bool emuCommands;

/* Reimplemented GUI callbacks */
void gui_do_stuff(void);
void gui_console_printf(const char *, ...);
void gui_debugger_raise_or_disable(bool);
void gui_console_err_printf(const char *, ...);
void gui_debugger_send_command(int, uint32_t);
void gui_emu_sleep(unsigned long ms);

bool emu_load(const char*, const char*);
void emu_loop(bool);
void emu_cleanup(void);
bool emu_save(const char*);
bool emu_save_rom(const char*);

void throttle_interval_event(enum sched_item_id id);
void throttle_timer_wait(void);

#ifdef __cplusplus
}
#endif

#endif
