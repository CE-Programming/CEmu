#ifndef SPI_H
#define SPI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "port.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#define SPI_WIDTH 32
#define SPI_RXFIFO_DEPTH 16
#define SPI_TXFIFO_DEPTH 16
#define SPI_FEATURES 0xE

typedef struct spi_state {
    uint32_t cr0, cr1, cr2, intCtrl;
    uint32_t rxFrame, txFrame, deviceFrame;
    uint8_t transferBits, deviceBits;
    uint8_t rfvi, rfve, tfvi, tfve;
    uint8_t intStatus;
    bool arm;
    uint32_t rxFifo[SPI_RXFIFO_DEPTH], txFifo[SPI_TXFIFO_DEPTH];

    void (*device_select)(bool);
    uint8_t (*device_peek)(uint32_t*);
    uint8_t (*device_transfer)(uint32_t, uint32_t*);
} spi_state_t;

extern spi_state_t spi;

eZ80portrange_t init_spi(void);
void spi_device_select(bool arm);
void spi_reset(void);
bool spi_restore(FILE *image);
bool spi_save(FILE *image);

#ifdef __cplusplus
}
#endif

#endif
