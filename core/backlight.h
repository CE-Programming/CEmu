#ifndef BACKLIGHTPORT_H
#define BACKLIGHTPORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "port.h"

PACK(typedef struct backlight_state {
    uint8_t ports[0x100];
    uint8_t brightness;
}) backlight_state_t;

/* Global BACKLIGHT state */
extern backlight_state_t backlight;

eZ80portrange_t init_backlight(void);

/* Save/Restore */
typedef struct emu_image emu_image;
bool backlight_restore(const emu_image*);
bool backlight_save(emu_image*);

#ifdef __cplusplus
}
#endif

#endif
