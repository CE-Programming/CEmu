#ifndef LCD_SPI_H
#define LCD_SPI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../port.h"
#include "lcd.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct spi_state {
    bool sleep;
    bool partial;
    bool invert;
    uint8_t gamma;
    bool power;
    uint16_t colStart, colEnd, colCur;
    uint16_t rowStart, rowEnd, rowCur;
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

/* Global CONTROL state */
extern spi_state_t spi;

/* Available Functions */
eZ80portrange_t init_spi(void);

/* Save/Restore */
bool spi_restore(FILE *image);
bool spi_save(FILE *image);

/* Functions */
void spi_reset(void);

#ifdef __cplusplus
}
#endif

#endif
