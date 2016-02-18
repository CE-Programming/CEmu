#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "emu.h"
#include "cpu.h"
#include "mem.h"
#include "lcd.h"
#include "usb.h"
#include "asic.h"
#include "misc.h"
#include "flash.h"
#include "keypad.h"
#include "sha256.h"
#include "timers.h"
#include "control.h"
#include "schedule.h"
#include "backlight.h"
#include "interrupt.h"
#include "realclock.h"

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

    /* Port ranges 0x0 -> 0xF */
    asic.portRange[0x0] = init_control();
    asic.portRange[0x1] = init_flash();
    asic.portRange[0x2] = init_sha256();
    asic.portRange[0x3] = init_usb();
    asic.portRange[0x4] = init_lcd();
    asic.portRange[0x5] = init_intrpt();
    asic.portRange[0x6] = init_watchdog();
    asic.portRange[0x7] = init_gpt();
    asic.portRange[0x8] = init_rtc();
    asic.portRange[0x9] = init_protected();
    asic.portRange[0xA] = init_keypad();
    asic.portRange[0xB] = init_backlight();
    asic.portRange[0xC] = init_cxxx();
    asic.portRange[0xD] = init_dxxx();
    asic.portRange[0xE] = init_exxx();
    asic.portRange[0xF] = init_fxxx();

    /* Populate APB ports */
    for(i=0; i<=0xF; i++) {
        apb_set_map(i, &asic.portRange[i]);
    }

    reset_proc_count = 0;

    /* Populate reset callbacks */
    add_reset_proc(lcd_reset);
    add_reset_proc(keypad_reset);
    add_reset_proc(gpt_reset);
    add_reset_proc(rtc_reset);
    add_reset_proc(watchdog_reset);
    add_reset_proc(cpu_reset);

    gui_console_printf("[CEmu] Initialized APB...\n");
}

void asic_init(void) {
    /* First, initilize memory and LCD */
    mem_init();
    cpu_init();

    asic.mem = &mem;
    asic.cpu = &cpu;

    sched.clockRates[CLOCK_CPU] = 6000000;
    sched.clockRates[CLOCK_APB] = 50000000;

    plug_devices();
    gui_console_printf("[CEmu] Initialized ASIC...\n");
}

void asic_free(void) {
    /* make sure the LCD doesn't use unalloced mem */
    lcd.upcurr = lcd.upbase = 0;
    mem_free();
    asic.mem = NULL;
    asic.cpu = NULL;
    gui_console_printf("[CEmu] Freed ASIC.\n");
}

void asic_reset(void) {
    unsigned int i;

    sched.clockRates[CLOCK_CPU] = 6000000;
    sched.clockRates[CLOCK_APB] = 50000000;

    for(i = 0; i < reset_proc_count; i++) {
        reset_procs[i]();
    }
}

void set_device_type(ti_device_t device) {
    asic.deviceType = device;
}

ti_device_t get_device_type(void) {
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

bool asic_restore(const emu_image *s) {
    asic.deviceType = s->deviceType;

    return backlight_restore(s)
           && control_restore(s)
           && cpu_restore(s)
           && flash_restore(s)
           && intrpt_restore(s)
           && keypad_restore(s)
           && lcd_restore(s)
           && mem_restore(s)
           && watchdog_restore(s)
           && protect_restore(s)
           && rtc_restore(s)
           && sha256_restore(s)
           && gpt_restore(s)
           && usb_restore(s)
           && cxxx_restore(s)
           && dxxx_restore(s)
           && exxx_restore(s)
           && sched_restore(s);
}

bool asic_save(emu_image *s) {
    s->deviceType = asic.deviceType;

    return backlight_save(s)
           && control_save(s)
           && cpu_save(s)
           && flash_save(s)
           && intrpt_save(s)
           && keypad_save(s)
           && lcd_save(s)
           && mem_save(s)
           && watchdog_save(s)
           && protect_save(s)
           && rtc_save(s)
           && sha256_save(s)
           && gpt_save(s)
           && usb_save(s)
           && cxxx_save(s)
           && dxxx_save(s)
           && exxx_save(s)
           && sched_save(s);
}
