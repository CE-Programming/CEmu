#ifndef FLASHPORT_H
#define FLASHPORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "port.h"

/* Standard FLASH state */
PACK(typedef struct flash_state {
    uint8_t ports[0x100];
    uint8_t addedWaitStates;
    uint32_t mask;
    uint8_t mapped : 1;
    uint8_t map    : 4;
}) flash_state_t;

/* Global flash state */
extern flash_state_t flash;

/* Avbailable functions */
eZ80portrange_t init_flash(void);
int flash_open(const char *filename);

/* Save/Restore */
typedef struct emu_image emu_image;
bool flash_restore(const emu_image*);
bool flash_save(emu_image*);

#ifdef __cplusplus
}
#endif

#endif
