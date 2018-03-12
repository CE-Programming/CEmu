#ifndef SHA256_H
#define SHA256_H

#ifdef __cplusplus
extern "C" {
#endif

#include "port.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct sha256_state {
    uint32_t block[16];
    uint32_t state[8];
    uint16_t last;
} sha256_state_t;

eZ80portrange_t init_sha256(void);
void sha256_reset(void);
bool sha256_restore(FILE *image);
bool sha256_save(FILE *image);

#ifdef __cplusplus
}
#endif

#endif
