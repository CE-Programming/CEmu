#include "core/asic.h"
#include "core/emu.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "core/cpu.h"
#include "core/memory.h"

#include "core/misc.h"
#include "core/interrupt.h"
#include "core/keypad.h"
#include "core/controlport.h"
#include "core/flash.h"
#include "core/lcd.h"
#include "core/backlightport.h"
#include "core/timers.h"
#include "core/schedule.h"

/* Global ASIC state */
asic_state_t asic;

void (*reset_procs[20])(void);
unsigned int reset_proc_count;

static void add_reset_proc(void (*proc)(void))
{
    if (reset_proc_count == sizeof(reset_procs)/sizeof(*reset_procs)) {
        abort();
    }
    reset_procs[reset_proc_count++] = proc;
}

uint8_t read_unimplemented_port(const uint16_t addr) {
    /*printf("Attempted to read unimplemented port: 0x%04X", addr);*/
    return 0;
}

void write_unimplemented_port(const uint16_t addr, uint8_t value) {
    /*printf("Attempted to write unimplemented port: 0x%04X <- 0x%02X", addr, value);*/
}

static void plug_devices(void) {
    /* Unimplemented devices */
    int i;
    eZ80portrange_t unimplemented_range = { read_unimplemented_port, write_unimplemented_port };
    for (i=0; i<=0xF; i++) {
            asic.cpu->prange[i] = unimplemented_range;
    }

    /* Port ranges 0x0 -> 0xF*/
    asic.cpu->prange[0x0] = init_control();
    asic.cpu->prange[0x1] = init_flash();
  //asic.cpu->prange[0x2] = init_sha256();
  //asic.cpu->prange[0x3] = init_usb();
    asic.cpu->prange[0x4] = init_lcd();
    asic.cpu->prange[0x5] = init_intrpt();
  //asic.cpu->prange[0x6] = init_watchdog();
    asic.cpu->prange[0x7] = init_gpt();
  //asic.cpu->prange[0x8] = init_rtc();
  //asic.cpu->prange[0x9] = init_protected();
    asic.cpu->prange[0xA] = init_keypad();
    asic.cpu->prange[0xB] = init_backlight();
    asic.cpu->prange[0xC] = init_cxxx();
  //asic.cpu->prange[0xD] = init_dxxx();
    asic.cpu->prange[0xE] = init_exxx();
    asic.cpu->prange[0xF] = init_fxxx();

    /* Populate APB ports */
    for(i=0x0; i<=0xF; i++) {
        apb_set_map(i, &asic.cpu->prange[i]);
    }

    reset_proc_count = 0;

    /* Populate reset callbacks */
    add_reset_proc(lcd_reset);
    add_reset_proc(keypad_reset);
    add_reset_proc(gpt_reset);

    gui_console_printf("Initialized APB...\n");
}

void asic_init(ti_device_type type) {
    /* First, initilize memory and LCD */
    mem_init();
    cpu_init();

    asic.mem = &mem;
    asic.cpu = &cpu;

    asic.cpu->read_byte = memory_read_byte;
    asic.cpu->write_byte = memory_write_byte;
    asic.battery = BATTERIES_GOOD;
    asic.device = type;

    asic.stopped = 0;

    plug_devices();
    gui_console_printf("Initialized ASIC...\n");
}

void asic_free(void) {
    mem_free();
    gui_console_printf("Freed ASIC...\n");
}

void asic_reset(void) {
    unsigned int i;

    sched.clock_rates[CLOCK_CPU] = 48000000;
    sched.clock_rates[CLOCK_APB] = 48000000;

    for(i = 0; i < reset_proc_count; i++) {
        reset_procs[i]();
    }
}

uint32_t set_cpu_clock_rate(uint32_t new_rate) {
    uint32_t old_rate = sched.clock_rates[CLOCK_CPU];
    uint32_t cpu_new_rate[1] = { new_rate };
    sched_set_clocks(1, cpu_new_rate);

    return old_rate;
}
