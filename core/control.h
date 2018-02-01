#ifndef CONTROL_H
#define CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "port.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

enum {
    BATTERY_DISCHARGED,
    BATTERY_0,
    BATTERY_1,
    BATTERY_2,
    BATTERY_3,
    BATTERY_4
};

typedef struct control_state {
    uint8_t ports[0x80];
    uint8_t cpuSpeed;
    bool usbSelfPowered;
    bool usbBusPowered;

    uint8_t setBatteryStatus;
    uint8_t readBatteryStatus;
    bool batteryCharging;

    uint32_t privileged;
    uint32_t stackLimit;
    uint32_t protectedStart;
    uint32_t protectedEnd;
    uint8_t protectionStatus;
    uint8_t protectedPortsUnlocked;
    uint8_t flashUnlocked;

    bool off;
} control_state_t;

/* Global CONTROL state */
extern control_state_t control;

/* Available Functions */
eZ80portrange_t init_control(void);
void control_reset(void);

/* Save/Restore */
bool control_restore(FILE *image);
bool control_save(FILE *image);
bool protected_ports_unlocked(void);
bool flash_unlocked(void);
bool unprivileged_code(void);

#ifdef __cplusplus
}
#endif

#endif
