#ifndef BACKLIGHT_H
#define BACKLIGHT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "port.h"

#include <stdio.h>

typedef struct backlight_state {
    uint8_t ports[0x100];
    uint8_t brightness;
    float factor;
} backlight_state_t;

/* Global BACKLIGHT state */
extern backlight_state_t backlight;

eZ80portrange_t init_backlight(void);
void backlight_reset(void);

/* Save/Restore */
bool backlight_restore(FILE *image);
bool backlight_save(FILE *image);

#ifdef __cplusplus
}
#endif

#endif
