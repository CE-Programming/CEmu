#ifndef H_SHA256
#define H_SHA256

#ifdef __cplusplus
extern "C" {
#endif

#include "port.h"

PACK(typedef struct sha256_state {
    uint32_t hash_block[16];
    uint32_t hash_state[8];
    uint16_t last_index;
}) sha256_state_t;

eZ80portrange_t init_sha256(void);
void sha256_reset(void);

/* Save/Restore */
typedef struct emu_image emu_image;
bool sha256_restore(const emu_image*);
bool sha256_save(emu_image*);

#ifdef __cplusplus
}
#endif

#endif
