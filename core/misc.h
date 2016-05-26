#ifndef MISC_H
#define MISC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "port.h"

PACK(typedef struct watchdog_state {
    uint32_t count;              /* Standard WATCHDOG state */
    uint32_t load;
    uint16_t restart;
    uint32_t control;
    uint32_t status;
    uint32_t intrptLength;
    uint32_t revision;
}) watchdog_state_t;

PACK(typedef struct protected_state {  /* Standard PROTECTED state */
    bool locked;
    uint8_t ledState;
    uint8_t unknown_ports[0x100];
}) protected_state_t;

PACK(typedef struct cxxx_state {
    uint8_t ports[0x100];         /* Standard CXXX state */
}) cxxx_state_t;
PACK(typedef struct dxxx_state {       /* Standard DXXX state */
    uint8_t dummy;                /* Silence warning, remove if other fields are added. */
}) dxxx_state_t;
PACK(typedef struct exxx_state {
    uint8_t ports[0x80];          /* Standard EXXX state */
}) exxx_state_t;
PACK(typedef struct fxxx_state {       /* Standard FXXX state */
    uint8_t dummy;                /* Silence warning, remove if other fields are added. */
}) fxxx_state_t;

extern watchdog_state_t watchdog;   /* Global WATCHDOG state */
extern protected_state_t protect;   /* Global PROTECT state */
extern cxxx_state_t cxxx;           /* Global CXXX state */
extern dxxx_state_t dxxx;           /* Global DXXX state */
extern exxx_state_t exxx;           /* Global EXXX state */
extern fxxx_state_t fxxx;           /* Global FXXX state */

/* Available functions */
void watchdog_reset(void);
eZ80portrange_t init_watchdog(void);
eZ80portrange_t init_protected(void);
eZ80portrange_t init_cxxx(void);
eZ80portrange_t init_dxxx(void);
eZ80portrange_t init_exxx(void);
eZ80portrange_t init_fxxx(void);

/* Save/Restore */
typedef struct emu_image emu_image;
bool watchdog_restore(const emu_image*);
bool watchdog_save(emu_image*);
bool protect_restore(const emu_image*);
bool protect_save(emu_image*);
bool cxxx_restore(const emu_image*);
bool cxxx_save(emu_image*);
bool dxxx_restore(const emu_image*);
bool dxxx_save(emu_image*);
bool exxx_restore(const emu_image*);
bool exxx_save(emu_image*);

#ifdef __cplusplus
}
#endif

#endif
