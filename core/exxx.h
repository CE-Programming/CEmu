#ifndef EXXX_H
#define EXXX_H

#include <core/apb.h>

#ifdef __cplusplus
extern "C" {
#endif

// Standard EXXX state
struct exxx_state {
        uint8_t ports[0x80];  // mirrored ever 0x80 bytes
};

// Type definitions
typedef struct exxx_state exxx_state_t;

// Global unk3 state
extern exxx_state_t exxx;

// Avbailable functions
eZ80portrange_t init_exxx();

#ifdef __cplusplus
}
#endif

#endif
