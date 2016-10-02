#ifndef EMU_H
#define EMU_H

#ifdef __cplusplus
extern "C" {
#endif

#include "defines.h"
#include "cpu.h"
#include "flash.h"
#include "mem.h"
#include "lcd.h"
#include "schedule.h"
#include "link.h"
#include "usb.h"
#include "interrupt.h"
#include "misc.h"
#include "keypad.h"
#include "realclock.h"
#include "sha256.h"
#include "tidevices.h"
#include "backlight.h"
#include "timers.h"
#include "control.h"

PACK(typedef struct emu_image {
    uint32_t version; // 0xCECEXXXX - XXXX is version number if the core is changed
    ti_device_t deviceType;
    eZ80cpu_t cpu;
    usb_state_t usb;
    flash_state_t flash;
    interrupt_state_t intrpt;
    watchdog_state_t watchdog;
    protected_state_t protect;
    cxxx_state_t cxxx;
    dxxx_state_t dxxx;
    exxx_state_t exxx;
    fxxx_state_t fxxx;
    keypad_state_t keypad;
    sched_state_t sched;
    rtc_state_t rtc;
    sha256_state_t sha256;
    general_timers_state_t gpt;
    backlight_state_t backlight;
    control_state_t control;
    lcd_state_t lcd;
    mem_state_t mem;
    uint8_t mem_flash[flash_size];
    uint8_t mem_ram[ram_size];
}) emu_image_t;

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
extern volatile bool emulationPaused;

/* Reimplemented GUI callbacks */
void gui_do_stuff(void);
void gui_entered_send_state(bool);
void gui_console_printf(const char *, ...);
void gui_debugger_raise_or_disable(bool);
void gui_console_err_printf(const char *, ...);
void gui_debugger_send_command(int, uint32_t);
void gui_render_gif_frame(void);
void gui_set_busy(bool);
void gui_emu_sleep(void);

bool emu_start(const char*,const char*);
void emu_loop(bool);
void emu_cleanup(void);
bool emu_save(const char*);
bool emu_save_rom(const char*);
void emu_set_emulation_paused(bool);

void throttle_interval_event(int index);
void throttle_timer_wait(void);

#ifdef __cplusplus
}
#endif

#endif
