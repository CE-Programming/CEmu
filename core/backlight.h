#ifndef BACKLIGHTPORT_H
#define BACKLIGHTPORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "apb.h"

typedef struct backlight_state {
    uint8_t ports[0x100];
    uint8_t brightness;
} backlight_state_t;

/* Global BACKLIGHT state */
extern backlight_state_t backlight;

eZ80portrange_t init_backlight(void);

#ifdef __cplusplus
}
#endif

#endif
