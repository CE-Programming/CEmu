#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "asic.h"
#include "emu.h"
#include "cpu.h"
#include "mem.h"
#include "misc.h"
#include "interrupt.h"
#include "keypad.h"
#include "control.h"
#include "flash.h"
#include "lcd.h"
#include "backlight.h"
#include "timers.h"
#include "usb.h"
#include "sha256.h"
#include "realclock.h"
#include "schedule.h"

/* Global ASIC state */
asic_state_t asic;

void (*reset_procs[20])(void);
unsigned int reset_proc_count;

static void add_reset_proc(void (*proc)(void)) {
    if (reset_proc_count == sizeof(reset_procs)/sizeof(*reset_procs)) {
        abort();
    }
    reset_procs[reset_proc_count++] = proc;
}

static void plug_devices(void) {
    unsigned int i;

    /* Port ranges 0x0 -> 0xF*/
    asic.cpu->prange[0x0] = init_control();
    asic.cpu->prange[0x1] = init_flash();
    asic.cpu->prange[0x2] = init_sha256();
    asic.cpu->prange[0x3] = init_usb();
    asic.cpu->prange[0x4] = init_lcd();
    asic.cpu->prange[0x5] = init_intrpt();
    asic.cpu->prange[0x6] = init_watchdog();
    asic.cpu->prange[0x7] = init_gpt();
    asic.cpu->prange[0x8] = init_rtc();
    asic.cpu->prange[0x9] = init_protected();
    asic.cpu->prange[0xA] = init_keypad();
    asic.cpu->prange[0xB] = init_backlight();
    asic.cpu->prange[0xC] = init_cxxx();
    asic.cpu->prange[0xD] = init_dxxx();
    asic.cpu->prange[0xE] = init_exxx();
    asic.cpu->prange[0xF] = init_fxxx();

    /* Populate APB ports */
    for(i=0; i<=0xF; i++) {
        apb_set_map(i, &asic.cpu->prange[i]);
    }

    reset_proc_count = 0;

    /* Populate reset callbacks */
    add_reset_proc(lcd_reset);
    add_reset_proc(keypad_reset);
    add_reset_proc(gpt_reset);
    add_reset_proc(rtc_reset);
    add_reset_proc(watchdog_reset);
    add_reset_proc(mem_reset);

    gui_console_printf("[CEmu] Initialized APB...\n");
}

void asic_init(void) {
    /* First, initilize memory and LCD */
    mem_init();
    cpu_init();
#ifdef DEBUG_SUPPORT
    debugger_init();
#endif
    asic.mem = &mem;
    asic.cpu = &cpu;

    plug_devices();
    gui_console_printf("[CEmu] Initialized ASIC...\n");
}

void asic_free(void) {
    mem_free();
#ifdef DEBUG_SUPPORT
    debugger_free();
#endif
    asic.mem = NULL;
    asic.cpu = NULL;
    gui_console_printf("[CEmu] Freed ASIC.\n");
}

void asic_reset(void) {
    unsigned int i;

    sched.clockRates[CLOCK_CPU] = 48000000;
    sched.clockRates[CLOCK_APB] = 48000000;

    for(i = 0; i < reset_proc_count; i++) {
        reset_procs[i]();
    }
}

void set_device_type(ti_device_type device) {
    asic.deviceType = device;
}

ti_device_type get_device_type(void) {
    return asic.deviceType;
}

bool calc_is_off(void) {
    return control.ports[0] & 0x40 ? true : false;
}

uint32_t set_cpu_clock_rate(uint32_t new_rate) {
    uint32_t old_rate = sched.clockRates[CLOCK_CPU];
    uint32_t cpu_new_rate[1] = { new_rate };
    sched_set_clocks(1, cpu_new_rate);

    return old_rate;
}
