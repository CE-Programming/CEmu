#include "core/runloop.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <limits.h>

/* Why the heck does "get the current time" have to be so platform specific */
#ifdef EMSCRIPTEN
#include <emscripten.h>
#endif
#ifdef __APPLE__
#include <mach/mach_time.h>
#define ORWL_NANO (+1.0E-9)
#define ORWL_GIGA UINT64_C(1000000000)
static double orwl_timebase = 0.0;
static uint64_t orwl_timestart = 0;
#endif

long long get_time_nsec() {
#ifdef EMSCRIPTEN
	return emscripten_get_now() * 1000000;
#else
#ifdef __APPLE__
	if (!orwl_timestart) {
		mach_timebase_info_data_t tb;
		mach_timebase_info(&tb);
		orwl_timebase = tb.numer;
		orwl_timebase /= tb.denom;
		orwl_timestart = mach_absolute_time();
	}
	struct timespec t;
	double diff = (mach_absolute_time() - orwl_timestart) * orwl_timebase;
	t.tv_sec = diff * ORWL_NANO;
	t.tv_nsec = diff - (t.tv_sec * ORWL_GIGA);
	return t.tv_nsec;
#else
	struct timespec sp;
	clock_gettime(CLOCK_MONOTONIC, &sp);

	return sp.tv_sec * 1000000000 + sp.tv_nsec;
#endif
#endif
}

runloop_state_t runloop;

void runloop_init() {
	runloop.asic = &asic;
	runloop.last_end = get_time_nsec();
	int i;
	for (i = 0; i < asic.timers->max_timers; i++) {
		eZ80_hardware_timer_t *timer = &asic.timers->timers[i];
		if (timer->flags & TIMER_IN_USE) {
			timer->cycles_until_tick = asic.clock_rate / timer->frequency;
		}
	}

	runloop.ticks = calloc(sizeof(timer_tick_t), 40);
	runloop.max_tick_count = 40;
}

int runloop_compare(const void *first, const void *second) {
	const timer_tick_t *a = first;
	const timer_tick_t *b = second;

	return a->after_cycle - b->after_cycle;
}

void runloop_tick_cycles(int cycles) {
	int total_cycles = 0;
	int cycles_until_next_tick = cycles;
	int current_tick = 0;
	int i;
	for (i = 0; i < runloop.asic->timers->max_timers; i++) {
		eZ80_hardware_timer_t *timer = &runloop.asic->timers->timers[i];

		if (!(timer->flags & TIMER_IN_USE)) {
			continue;
		}

		int tot_cycles = cycles;
		if (timer->cycles_until_tick < tot_cycles) {
			retry:
			runloop.ticks[current_tick].index = i;
			runloop.ticks[current_tick].after_cycle = timer->cycles_until_tick + (cycles - tot_cycles);
			tot_cycles -= timer->cycles_until_tick;
			timer->cycles_until_tick = runloop.asic->clock_rate / timer->frequency;
			current_tick++;

			if (current_tick == runloop.max_tick_count) {
				runloop.max_tick_count += 10;
				runloop.ticks = realloc(runloop.ticks, sizeof(timer_tick_t) * runloop.max_tick_count);
			}

			if (timer->cycles_until_tick <= tot_cycles) {
				goto retry;
			}
		} else {
			timer->cycles_until_tick -= tot_cycles;
		}
	}

	qsort(runloop.ticks, current_tick, sizeof(timer_tick_t), runloop_compare);

	if (current_tick > 0) {
		cycles_until_next_tick = runloop.ticks[0].after_cycle;
	}

	int tick_i = 0;
	while (cycles > 0) {
		int ran = cycles_until_next_tick - cpu_execute(cycles_until_next_tick);

		total_cycles += ran;
		cycles -= ran;

		if (total_cycles >= runloop.ticks[tick_i].after_cycle) {
			tick_i++;
			if (tick_i <= current_tick) {
				int index = runloop.ticks[tick_i - 1].index;
				eZ80_hardware_timer_t *timer = &runloop.asic->timers->timers[index];
				timer->on_tick(timer->data);
				cycles_until_next_tick = runloop.ticks[tick_i].after_cycle - total_cycles;
			} else {
				cycles_until_next_tick = cycles;
			}
		}
	}
	runloop.spare_cycles = cycles;
}

void runloop_tick() {
	long long now = get_time_nsec();
	long long ticks_between = now - runloop.last_end;

	float seconds = (float)ticks_between / (float)1000000000;
	int cycles = seconds * (float)runloop.asic->clock_rate;

	if (cycles == 0)
		return;

	runloop_tick_cycles(cycles);
	runloop.last_end = now;
}
