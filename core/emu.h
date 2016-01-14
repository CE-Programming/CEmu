#ifndef EMU_H
#define EMU_H

#ifdef __cplusplus
extern "C" {
#endif

#include "defines.h"

extern uint32_t cpu_events;

#define EVENT_NONE            0
#define EVENT_RESET           1
#define EVENT_DEBUG_STEP      2
#define EVENT_DEBUG_STEP_OVER 4
#define EVENT_DEBUG_STEP_OUT  8
#define EVENT_WAITING         16

/* Settings */
extern volatile bool exiting;

enum { LOG_CPU, LOG_IO, LOG_FLASH, LOG_INTRPTS, LOG_COUNT, LOG_USB, LOG_GUI, MAX_LOG };
#define LOG_TYPE_TBL "CIFQ#UG"
extern int log_enabled[MAX_LOG];
void logprintf(int type, const char *str, ...);
void throttle_timer_wait();

/* ROM image */
extern const char *rom_image;

/* Reimplemented GUI callbacks */
void gui_do_stuff(bool wait);
void gui_console_printf(const char *, ...);
void gui_console_vprintf(const char *, va_list);
void gui_perror(const char *);
void gui_debugger_entered_or_left(bool);
void gui_debugger_send_command(int, uint32_t);
void gui_emu_yield(void);
void gui_emu_sleep(void);

bool emu_start(void);
void emu_loop(bool reset);
void emu_cleanup(void);

void throttle_interval_event(int index);

#ifdef __cplusplus
}
#endif

#endif
