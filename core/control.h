#ifndef CONTROLPORT_H
#define CONTROLPORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "port.h"

enum {
    BATTERY_DISCHARGED,
    BATTERY_0,
    BATTERY_1,
    BATTERY_2,
    BATTERY_3,
    BATTERY_4
};

PACK(typedef struct control_state {
    uint8_t ports[0x80];
    uint8_t cpuSpeed;
    bool USBSelfPowered;
    bool USBBusPowered;

    uint8_t setBatteryStatus;
    uint8_t readBatteryStatus;
    bool batteryCharging;

    uint32_t privileged;
    uint32_t stackLimit;
}) control_state_t;

/* Global CONTROL state */
extern control_state_t control;

/* Available Functions */
void free_control(void *_state);
eZ80portrange_t init_control(void);

/* Save/Restore */
typedef struct emu_image emu_image;
bool control_restore(const emu_image*);
bool control_save(emu_image*);

#ifdef __cplusplus
}
#endif

#endif
