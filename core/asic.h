#ifndef ASIC_H
#define ASIC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include <core/cpu.h>
#include <core/cxxx.h>
#include <core/exxx.h>
#include <core/fxxx.h>
#include <core/memory.h>
#include <core/interrupt.h>
#include <core/tidevices.h>
#include <core/keypad.h>
#include <core/controlport.h>
#include <core/flash.h>
#include <core/lcd.h>
#include <core/backlightport.h>

typedef void (*timer_tick)(void *);
typedef struct eZ80_hardware_timers eZ80_hardware_timers_t;
typedef struct eZ80_hardware_timer eZ80_hardware_timer_t;

enum {
	TIMER_IN_USE = (1 << 0),
	TIMER_ONE_SHOT = (1 << 1)
};

struct eZ80_hardware_timer {
	int cycles_until_tick;

	int flags;
	double frequency;
	timer_tick on_tick;
	void *data;
};

struct eZ80_hardware_timers {
	int max_timers;
	eZ80_hardware_timer_t *timers;
};

typedef enum {
	BATTERIES_REMOVED,
	BATTERIES_LOW,
	BATTERIES_GOOD
} battery_state;

struct asic_state {
	int stopped;
	ti_device_type device;
	battery_state battery;
	int battery_remove_check;
	int clock_rate;

	mem_state_t* mem;
	eZ80cpu_t *cpu;
	eZ80_hardware_timers_t* timers;
};

// Type definitions
typedef struct asic_state asic_state_t;

// Available Functions
void asic_init(ti_device_type);
void asic_free(void);

int asic_set_clock_rate(int);

int asic_add_timer(int, double, timer_tick, void *);
void asic_remove_timer(int);

uint8_t read_unimplemented_port(const uint16_t addr);

void write_unimplemented_port(const uint16_t addr, uint8_t value);

#ifdef __cplusplus
}
#endif

#endif
