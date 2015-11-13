/* Declarations for keypad.c */

#ifndef CONTROLPORT_H
#define CONTROLPORT_H

#include <core/cpu.h>
#include <core/memory.h>

#ifdef __cplusplus
extern "C" {
#endif

struct control_state {
        uint8_t ports[0x80];
        uint8_t device_type;
        uint8_t unknown_flag_0; // coresponds with 0x5E in ROM file.
        uint8_t unknown_g_Bd;
        uint8_t unknown_g_Xb;
};

typedef struct control_state control_state_t;

// Global CONTROL state
extern control_state_t control;

// Available Functions
eZ80portrange_t init_control();

#ifdef __cplusplus
}
#endif

#endif
