#ifndef CXXX_H
#define CXXX_H

#include <core/apb.h>

#ifdef __cplusplus
extern "C" {
#endif

// Standard CXXX state
struct cxxx_state {
        uint8_t ports[0x100];  // No idea how big this is...
};

// Type definitions
typedef struct cxxx_state cxxx_state_t;

// Global CXXX state
extern cxxx_state_t cxxx;

// Avbailable functions
eZ80portrange_t init_cxxx(void);

#ifdef __cplusplus
}
#endif

#endif
