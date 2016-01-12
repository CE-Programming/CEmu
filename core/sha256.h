#ifndef H_SHA256
#define H_SHA256

#ifdef __cplusplus
extern "C" {
#endif

#include "apb.h"

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
