/* Declarations for sha256.c */

#ifndef _H_SHA256
#define _H_SHA256

#include <core/cpu.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sha256_state {
    uint32_t hash_state[8];
    uint32_t hash_block[16];
} sha256_state_t;

/* Type Definitions */
typedef struct sha256_state sha256_state_t;

eZ80portrange_t init_sha256(void);
void sha256_reset(void);

#ifdef __cplusplus
}
#endif

#endif
