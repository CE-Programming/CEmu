#ifndef SPIPORT_H
#define SPIPORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "port.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct spi_state {
    bool sleep;
    bool partial;
    bool invert;
    uint8_t gamma;
    bool power;
    uint16_t colStart, colEnd;
    uint16_t rowStart, rowEnd;
    uint8_t frame[360][240][3];
    uint8_t lut[128];
    uint16_t partialStart, partialEnd;
    uint16_t topArea, scrollArea, bottomArea, scrollStart;
    uint8_t MAC;
    bool tear;
    bool idle;
    uint16_t tearLine;
    uint8_t gammaCorrection[2][16];
    uint16_t fifo;
    uint8_t shift;
    uint8_t cmd;
    uint16_t param;
} spi_state_t;

enum { SPI_COMMAND };

/* Global CONTROL state */
extern spi_state_t spi;

/* Available Functions */
eZ80portrange_t init_spi(void);

/* Save/Restore */
typedef struct emu_image emu_image;
bool spi_restore(const emu_image*);
bool spi_save(emu_image*);

/* Functions */
void spi_reset(void);

#ifdef __cplusplus
}
#endif

#endif
