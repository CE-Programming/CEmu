#ifndef EMU_H
#define EMU_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

/* CPU events */
extern uint32_t cpuEvents;

#define EVENT_NONE            0
#define EVENT_RESET           1
#ifdef DEBUG_SUPPORT
#define EVENT_DEBUG_STEP      2
#define EVENT_DEBUG_STEP_OVER 4
#define EVENT_DEBUG_STEP_NEXT 8
#define EVENT_DEBUG_STEP_OUT  16
#endif
#define EVENT_WAITING         32

/* Settings */
extern volatile bool exiting;
extern volatile bool emu_allow_instruction_commands;

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

void throttle_interval_event(int index);
void throttle_timer_wait(void);

#ifdef __cplusplus
}
#endif

#endif
