#include "asic.h"
#include "cpu.h"
#include "misc.h"
#include "mem.h"
#include "lcd.h"
#include "spi.h"
#include "usb/usb.h"
#include "bus.h"
#include "emu.h"
#include "flash.h"
#include "timers.h"
#include "sha256.h"
#include "keypad.h"
#include "control.h"
#include "schedule.h"
#include "interrupt.h"
#include "backlight.h"
#include "realclock.h"
#include "defines.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Global ASIC state */
asic_state_t asic;

/* Requested revision to apply on reset */
static asic_rev_t pending_asic_rev = ASIC_REV_AUTO;

#define MAX_RESET_PROCS 20

static void (*reset_procs[MAX_RESET_PROCS])(void);
static unsigned int reset_proc_count;

static void add_reset_proc(void (*proc)(void)) {
    if (reset_proc_count == MAX_RESET_PROCS) {
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
    port_map[0xD] = init_spi();
    port_map[0xE] = init_exxx();
    port_map[0xF] = init_fxxx();

    reset_proc_count = 0;

    /* Populate reset callbacks */
    add_reset_proc(sched_reset);
    add_reset_proc(mem_reset);
    add_reset_proc(lcd_reset);
    add_reset_proc(keypad_reset);
    add_reset_proc(gpt_reset);
    add_reset_proc(rtc_reset);
    add_reset_proc(watchdog_reset);
    add_reset_proc(cpu_reset);
    add_reset_proc(intrpt_reset);
    add_reset_proc(sha256_reset);
    add_reset_proc(usb_reset);
    add_reset_proc(control_reset);
    add_reset_proc(backlight_reset);
    add_reset_proc(spi_reset);

    gui_console_printf("[CEmu] Initialized Advanced Peripheral Bus...\n");
}

void asic_init(void) {
    /* First, initilize memory and CPU */
    mem_init();
    cpu_init();
    sched_init();

    /* Seed the numbers */
    srand(time(NULL));
    bus_init_rand(rand(), rand(), rand());

    plug_devices();
    gui_console_printf("[CEmu] Initialized ASIC...\n");
}

void asic_free(void) {
    usb_init_device(0, NULL, NULL, NULL);
    lcd_free();
    mem_free();
    gui_console_printf("[CEmu] Freed ASIC.\n");
}

void asic_reset(void) {
    asic.revision = (pending_asic_rev == ASIC_REV_AUTO) ? asic.auto_revision : pending_asic_rev;
    asic.im2 = (asic.revision < ASIC_REV_I);
    asic.serFlash = (asic.revision >= ASIC_REV_M);

    for (unsigned int i = 0; i < reset_proc_count; i++) {
        reset_procs[i]();
    }

    gui_report_reset();
}

void set_device_type(ti_device_t device) {
    asic.device = device;
}

void set_asic_revision(asic_rev_t revision) {
    pending_asic_rev = revision;
}

void set_asic_auto_revision(asic_rev_t revision) {
    asic.auto_revision = revision;
}

ti_device_t EMSCRIPTEN_KEEPALIVE get_device_type(void) {
    return asic.device;
}

asic_rev_t EMSCRIPTEN_KEEPALIVE get_asic_revision(void) {
    return asic.revision;
}

void set_cpu_clock(uint32_t new_rate) {
   sched_set_clock(CLOCK_CPU, new_rate);
}

bool asic_restore(FILE *image) {
    if (fread(&asic, sizeof(asic), 1, image) == 1
        && backlight_restore(image)
        && control_restore(image)
        && cpu_restore(image)
        && flash_restore(image)
        && intrpt_restore(image)
        && keypad_restore(image)
        && lcd_restore(image)
        && mem_restore(image)
        && watchdog_restore(image)
        && protect_restore(image)
        && rtc_restore(image)
        && sha256_restore(image)
        && gpt_restore(image)
        && usb_restore(image)
        && cxxx_restore(image)
        && spi_restore(image)
        && exxx_restore(image)
        && sched_restore(image)
        && fgetc(image) == EOF)
    {
        gui_report_reset();
        return true;
    }
    return false;
}

bool asic_save(FILE *image) {
    return fwrite(&asic, sizeof(asic), 1, image) == 1
           && backlight_save(image)
           && control_save(image)
           && cpu_save(image)
           && flash_save(image)
           && intrpt_save(image)
           && keypad_save(image)
           && lcd_save(image)
           && mem_save(image)
           && watchdog_save(image)
           && protect_save(image)
           && rtc_save(image)
           && sha256_save(image)
           && gpt_save(image)
           && usb_save(image)
           && cxxx_save(image)
           && spi_save(image)
           && exxx_save(image)
           && sched_save(image);
}
