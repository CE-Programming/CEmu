#include "core/asic.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "core/cpu.h"
#include "core/memory.h"

#include "core/cxxx.h"
#include "core/exxx.h"
#include "core/fxxx.h"
#include "core/interrupt.h"
#include "core/runloop.h"
#include "core/keypad.h"
#include "core/controlport.h"
#include "core/flashport.h"
#include "core/lcd.h"
#include "core/backlightport.h"

// Global ASIC state
asic_state_t asic;

uint8_t read_unimplemented_port(const uint16_t addr) {
    //printf("Attempted to read unimplemented port: 0x%04X", addr);
    return 0;
}

void write_unimplemented_port(const uint16_t addr, uint8_t value) {
    //printf("Attempted to write unimplemented port: 0x%04X <- 0x%02X", addr, value);
}

void plug_devices() {
    /* Unimplemented devices */
    int i;
    eZ80portrange_t unimplemented_range = { read_unimplemented_port, write_unimplemented_port };
    for (i = 0; i < 0x10; i++) {
            asic.cpu->prange[i] = unimplemented_range;
    }

    // Port ranges 0x0 -> 0xF
    asic.cpu->prange[0x0] = init_control();
    asic.cpu->prange[0x1] = init_flash();
  //asic.cpu->prange[0x2] = init_sha256();
  //asic.cpu->prange[0x3] = init_usb();
    asic.cpu->prange[0x4] = init_lcd();
    asic.cpu->prange[0x5] = init_intrpt();
  //asic.cpu->prange[0x6] = init_wtchdog();
  //asic.cpu->prange[0x7] = init_gpt();
  //asic.cpu->prange[0x8] = init_rtc();
  //asic.cpu->prange[0x9] = init_protected();
    asic.cpu->prange[0xA] = init_keypad();
    asic.cpu->prange[0xB] = init_backlight();
    asic.cpu->prange[0xC] = init_cxxx();
  //asic.cpu->prange[0xD] = init_dxxx();
    asic.cpu->prange[0xE] = init_exxx();
    asic.cpu->prange[0xF] = init_fxxx();

    for(i=0; i<0x10; i++) {
        apb_set_map(i, &asic.cpu->prange[i]);       // mmio port handler
    }
}

void asic_init(ti_device_type type) {
    // First, initilize memory and LCD
    mem_init();
    lcd_init();
    cpu_init();

    asic.mem = &mem;
    asic.cpu = &cpu;

    asic.cpu->memory = asic.mem;
    asic.cpu->read_byte = memory_read_byte;
    asic.cpu->write_byte = memory_write_byte;
    asic.cpu->get_mem_wait_states = mem_wait_states;
    asic.battery = BATTERIES_GOOD;
    asic.device = type;
    asic.clock_rate = 6000000;

    asic.timers = calloc(sizeof(eZ80_hardware_timers_t), 1);
    asic.timers->max_timers = 20;
    asic.timers->timers = calloc(sizeof(eZ80_hardware_timer_t), 20);

    asic.stopped = 0;

    runloop_init();
    plug_devices();
}

void asic_free() {
    mem_free();
}

int asic_add_timer(int flags, double frequency, timer_tick tick, void *data) {
	eZ80_hardware_timer_t *timer = 0;
	int i;
	for (i = 0; i < asic.timers->max_timers; i++) {
		if (!(asic.timers->timers[i].flags & TIMER_IN_USE)) {
			timer = &asic.timers->timers[i];
			break;
		}

		if (i == asic.timers->max_timers - 1) {
			asic.timers->max_timers += 10;
			asic.timers->timers = realloc(asic.timers->timers, sizeof(eZ80_hardware_timer_t) * asic.timers->max_timers);
			eZ80_hardware_timer_t *ne = &asic.timers->timers[asic.timers->max_timers - 10];
			memset(ne, 0, sizeof(eZ80_hardware_timer_t) * 10);
		}
	}

	timer->cycles_until_tick = asic.clock_rate / frequency;
	timer->flags = flags | TIMER_IN_USE;
	timer->frequency = frequency;
	timer->on_tick = tick;
	timer->data = data;
	return i;
}

void asic_remove_timer(int index) {
	asic.timers->timers[index].flags &= ~TIMER_IN_USE;
}

int asic_set_clock_rate(int new_rate) {
	int old_rate = asic.clock_rate;

	int i;
	for (i = 0; i < asic.timers->max_timers; i++) {
		eZ80_hardware_timer_t *timer = &asic.timers->timers[i];
		if (timer->flags & TIMER_IN_USE) {
			timer->cycles_until_tick =
				new_rate / (timer->cycles_until_tick * timer->frequency);
		}
	}

	asic.clock_rate = new_rate;
	return old_rate;
}
