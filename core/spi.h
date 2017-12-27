#ifndef SPIPORT_H
#define SPIPORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "port.h"
#include <stdint.h>

typedef struct spi_state {
    uint8_t dummy;
} spi_state_t;

/* Global CONTROL state */
extern spi_state_t spi;

/* Available Functions */
eZ80portrange_t init_spi(void);

/* Save/Restore */
typedef struct emu_image emu_image;
bool spi_restore(const emu_image*);
bool spi_save(emu_image*);

#ifdef __cplusplus
}
#endif

#endif
