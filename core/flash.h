#ifndef FLASHPORT_H
#define FLASHPORT_H

#include <core/cpu.h>

#ifdef __cplusplus
extern "C" {
#endif

// Standard FLASH state
struct flash_state {
        uint8_t ports[0x100];
        uint8_t added_wait_states;
        uint8_t map;
};

// Type definitions
typedef struct flash_state flash_state_t;

// Global flash state
extern flash_state_t flash;

// Avbailable functions
eZ80portrange_t init_flash(void);
int flash_open(const char *filename);

#ifdef __cplusplus
}
#endif

#endif
