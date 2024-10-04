#include "asic.h"
#include "cpu.h"
#include "misc.h"
#include "mem.h"
#include "lcd.h"
#include "panel.h"
#include "spi.h"
#include "uart.h"
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
#include "cert.h"

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Global ASIC state */
asic_state_t asic;

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
    port_map[0xE] = init_uart();
    port_map[0xF] = init_fxxx();
    init_panel();

    reset_proc_count = 0;

    /* Populate reset callbacks */
    add_reset_proc(sched_reset);
    add_reset_proc(flash_reset);
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
    add_reset_proc(panel_reset);
    add_reset_proc(spi_reset);
    add_reset_proc(uart_reset);

    gui_console_printf("[CEmu] Initialized Advanced Peripheral Bus...\n");
}

static asic_rev_t report_reset(asic_rev_t loaded_rev, bool* python) {
    /* Parse boot code routines to determine version. */
    asic_rev_t default_rev = ASIC_REV_A;
    boot_ver_t boot_ver;
    bool gotVer = bootver_parse(mem.flash.block, &boot_ver);
    if (gotVer) {
        gui_console_printf("[CEmu] Boot code version: %u.%u.%u.%04u\n",
            boot_ver.major, boot_ver.minor, boot_ver.revision, boot_ver.build);

        /* Determine the newest ASIC revision that is compatible */
        for (int rev = ASIC_REV_A; rev <= ASIC_REV_M; rev++) {
            if (bootver_check_rev(&boot_ver, (asic_rev_t)rev)) {
                default_rev = rev;
            }
        }

        /* By default, ignore Python Edition in certificate if boot code is too old */
        if (loaded_rev == ASIC_REV_AUTO && default_rev < ASIC_REV_M) {
            *python = false;
        }
    }
    else {
        gui_console_err_printf("[CEmu] Could not determine boot code version.\n");
    }
    gui_console_printf("[CEmu] Default ASIC revision is Rev %c.\n", "AIM"[(int)default_rev - 1]);

    loaded_rev = gui_handle_reset((gotVer ? &boot_ver : NULL), loaded_rev, default_rev, python);
    return (loaded_rev != ASIC_REV_AUTO) ? loaded_rev : default_rev;
}

static void set_features() {
    asic.im2 = (asic.revision < ASIC_REV_I);
    asic.serFlash = (asic.revision >= ASIC_REV_M);
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
    usb_plug_device(0, NULL, NULL, NULL);
    lcd_free();
    mem_free();
    gui_console_printf("[CEmu] Freed ASIC.\n");
}

void asic_reset(void) {
    /* Update the Python state first, so it can be read by the reset handler if needed */
    static const uint16_t path[] = { 0x0330, 0x0430 };
    asic.python = !cert_field_find_path(mem.flash.block + 0x3B0001, SIZE_FLASH_SECTOR_64K, path, 2, NULL, NULL);
    asic.revision = report_reset(ASIC_REV_AUTO, &asic.python);
    set_features();

    for (unsigned int i = 0; i < reset_proc_count; i++) {
        reset_procs[i]();
    }
}

void set_device_type(ti_device_t device) {
    asic.device = device;
}

ti_device_t EMSCRIPTEN_KEEPALIVE get_device_type(void) {
    return asic.device;
}

asic_rev_t EMSCRIPTEN_KEEPALIVE get_asic_revision(void) {
    return asic.revision;
}

bool EMSCRIPTEN_KEEPALIVE get_asic_python(void) {
    return asic.python;
}

void set_cpu_clock(uint32_t new_rate) {
   sched_set_clock(CLOCK_CPU, new_rate);
}

bool asic_restore(FILE *image) {
    if (fread(&asic, offsetof(asic_state_t, im2), 1, image) != 1) {
        return false;
    }
    set_features();
    if (backlight_restore(image)
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
     && panel_restore(image)
     && spi_restore(image)
     && uart_restore(image)
     && sched_restore(image)
     && fgetc(image) == EOF)
    {
        bool python = asic.python;
        (void)report_reset(asic.revision, &python);
        return true;
    }
    return false;
}

bool asic_save(FILE *image) {
    return fwrite(&asic, offsetof(asic_state_t, im2), 1, image) == 1
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
           && panel_save(image)
           && spi_save(image)
           && uart_save(image)
           && sched_save(image);
}
