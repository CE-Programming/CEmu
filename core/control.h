#ifndef CONTROLPORT_H
#define CONTROLPORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "apb.h"

typedef struct control_state {
    uint8_t ports[0x80];
    uint8_t cpu_speed;
    uint8_t device_type;
    uint8_t unknown_flag_0; /* coresponds with 0x5E in ROM file. */
    uint8_t unknown_g_Bd;
    uint8_t unknown_g_Xb;
} control_state_t;

/* Global CONTROL state */
extern control_state_t control;

/* Available Functions */
void free_control(void *_state);
eZ80portrange_t init_control(void);

#ifdef __cplusplus
}
#endif

#endif
