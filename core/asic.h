#ifndef ASIC_H
#define ASIC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cpu.h"
#include "misc.h"
#include "mem.h"
#include "interrupt.h"
#include "tidevices.h"
#include "keypad.h"
#include "control.h"
#include "flash.h"
#include "lcd.h"
#include "backlight.h"
#include "timers.h"
#include "usb.h"
#include "realclock.h"
#include "sha256.h"

typedef struct asic_state {
    ti_device_t deviceType;
    bool shipModeEnabled;
} asic_state_t;

/* External Global ASIC state */
extern asic_state_t asic;

/* Available Functions */
void asic_init(void);
void asic_free(void);
void asic_reset(void);

void set_device_type(ti_device_t device);
ti_device_t get_device_type(void);
bool calc_is_off(void);

uint32_t set_cpu_clock_rate(uint32_t new_rate);

/* Save/Restore */
typedef struct emu_image emu_image;
bool asic_restore(const emu_image*);
bool asic_save(emu_image*);

#ifdef __cplusplus
}
#endif

#endif
