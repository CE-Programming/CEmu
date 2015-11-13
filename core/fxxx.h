#ifndef FXXX_H
#define FXXX_H

#include <core/apb.h>

#ifdef __cplusplus
extern "C" {
#endif

// Standard FXXX state
struct fxxx_state {
        uint8_t ports[0x100];  // No idea how big this is...
};

// Type definitions
typedef struct fxxx_state fxxx_state_t;

// Global unk3 state
extern fxxx_state_t fxxx;

// Avbailable functions
eZ80portrange_t init_fxxx();

#ifdef __cplusplus
}
#endif

#endif
