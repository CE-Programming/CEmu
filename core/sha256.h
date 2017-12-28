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
    uint32_t hash_block[16];
    uint32_t hash_state[8];
    uint16_t last_index;
} sha256_state_t;

eZ80portrange_t init_sha256(void);
void sha256_reset(void);

/* Save/Restore */
bool sha256_restore(FILE *image);
bool sha256_save(FILE *image);

#ifdef __cplusplus
}
#endif

#endif
