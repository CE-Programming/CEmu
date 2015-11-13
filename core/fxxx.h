#ifndef FXXX_H
#define FXXX_H

#include <core/apb.h>

#ifdef __cplusplus
extern "C" {
#endif

// Standard FXXX state
struct fxxx_state {
};

// Type definitions
typedef struct fxxx_state fxxx_state_t;

// Global unk3 state
extern fxxx_state_t fxxx;

// Avbailable functions
eZ80portrange_t init_fxxx(void);

#ifdef __cplusplus
}
#endif

#endif
