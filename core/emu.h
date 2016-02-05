#ifndef EMU_H
#define EMU_H

#ifdef __cplusplus
extern "C" {
#endif

#include "defines.h"

/* CPU events */
extern uint32_t cpu_events;

#define EVENT_NONE            0
#define EVENT_RESET           1
#ifdef DEBUG_SUPPORT
#define EVENT_DEBUG_STEP      2
#define EVENT_DEBUG_STEP_OVER 4
#define EVENT_DEBUG_STEP_OUT  8
#endif
#define EVENT_WAITING         16

/* Settings */
extern volatile bool exiting;

/* ROM image */
extern const char *rom_image;

/* Reimplemented GUI callbacks */
void gui_do_stuff(void);
void gui_console_printf(const char *, ...);
void gui_debugger_entered_or_left(bool);
void gui_debugger_send_command(int, uint32_t);
void gui_render_gif_frame(void);
void gui_emu_sleep(void);

bool emu_start(void);
void emu_loop(bool reset);
void emu_cleanup(void);

void throttle_interval_event(int index);
void throttle_timer_wait(void);

#ifdef __cplusplus
}
#endif

#endif
