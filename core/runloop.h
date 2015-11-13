#ifndef RUNLOOP_H
#define RUNLOOP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <core/asic.h>

typedef struct {
	int index;
	int after_cycle;
} timer_tick_t;

struct runloop_state {
	asic_state_t *asic;
	long long last_end;
	int spare_cycles;
	timer_tick_t *ticks;
	int max_tick_count;
};

// Typedefs
typedef struct runloop_state runloop_state_t;

// Externals
extern asic_state_t asic;

void runloop_init(void);
void runloop_tick_cycles(int);
void runloop_tick(void);

#ifdef __cplusplus
}
#endif

#endif
