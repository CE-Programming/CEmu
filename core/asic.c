#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
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
    /* Port ranges 0x0 -> 0xF */
    port_map[0x0] = init_control();
    port_map[0x1] = init_flash();
    port_map[0x2] = init_sha256();
    port_map[0x3] = init_usb();
    port_map[0x4] = init_lcd();
    port_map[0x5] = init_intrpt();
    port_map[0x6] = init_watchdog();
    port_map[0x7] = init_gpt();
    port_map[0x8] = init_rtc();
    port_map[0x9] = init_protected();
    port_map[0xA] = init_keypad();
    port_map[0xB] = init_backlight();
    port_map[0xC] = init_cxxx();
    port_map[0xD] = init_dxxx();
    port_map[0xE] = init_exxx();
    port_map[0xF] = init_fxxx();

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
    /* First, initilize memory and CPU */
    mem_init();
    cpu_init();

    asic.shipModeEnabled = false;

    plug_devices();
    gui_console_printf("[CEmu] Initialized ASIC...\n");
}

void asic_free(void) {
    /* make sure the LCD doesn't use unalloced mem */
    lcd.upcurr = lcd.upbase = 0;
    mem_free();
    gui_console_printf("[CEmu] Freed ASIC.\n");
}

void asic_reset(void) {
    unsigned int i;

    sched.clockRates[CLOCK_CPU] = 48000000;
    sched.clockRates[CLOCK_APB] = 78000000;

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
