#ifndef ASIC_H
#define ASIC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#include <core/cpu.h>
#include <core/misc.h>
#include <core/memory.h>
#include <core/interrupt.h>
#include <core/tidevices.h>
#include <core/keypad.h>
#include <core/controlport.h>
#include <core/flash.h>
#include <core/lcd.h>
#include <core/backlightport.h>

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

    mem_state_t* mem;
    eZ80cpu_t *cpu;
};

/* Type definitions */
typedef struct asic_state asic_state_t;

/* External Global ASIC state */
extern asic_state_t asic;

/* Available Functions */
void asic_init(ti_device_type);
void asic_free(void);
void asic_reset(void);

uint32_t set_cpu_clock_rate(uint32_t new_rate);

uint8_t read_unimplemented_port(const uint16_t addr);
void write_unimplemented_port(const uint16_t addr, uint8_t value);

#ifdef __cplusplus
}
#endif

#endif
