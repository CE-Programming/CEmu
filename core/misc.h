#ifndef MISC_H
#define MISC_H

#include "core/apb.h"

#ifdef __cplusplus
extern "C" {
#endif

struct cxxx_state {
    uint8_t ports[0x100]; /* Standard CXXX state */
};
struct exxx_state {
    uint8_t ports[0x80];  /* Standard EXXX state */
};
struct fxxx_state {       /* Standard FXXX state */
    uint8_t dummy;        /* Silence warning, remove if other fields are added. */
};

/* Type definitions */
typedef struct cxxx_state cxxx_state_t;
typedef struct exxx_state exxx_state_t;
typedef struct fxxx_state fxxx_state_t;

extern cxxx_state_t cxxx;   /* Global CXXX state */
extern exxx_state_t exxx;   /* Global EXXX state */
extern fxxx_state_t fxxx;   /* Global FXXX state */

/* Available functions */
eZ80portrange_t init_exxx(void);
eZ80portrange_t init_cxxx(void);
eZ80portrange_t init_fxxx(void);

#ifdef __cplusplus
}
#endif

#endif
