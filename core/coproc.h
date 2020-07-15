#ifndef COPROC_H
#define COPROC_H

#include "arm/arm.h"
#include "port.h"
#include "uart.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct coproc_state {
    arm_t *arm;
} coproc_state_t;

#ifdef __cplusplus
extern "C" {
#endif

extern coproc_state_t state;

void coproc_reset(void);
bool coproc_load(const char *path);
void coproc_uart_transmit(const uart_transfer_t *transfer);
bool coproc_uart_receive(uart_transfer_t *transfer);
void coproc_spi_select(bool low);
uint8_t coproc_spi_peek(uint32_t *rxData);
uint8_t coproc_spi_transfer(uint32_t txData, uint32_t *rxData);

#ifdef __cplusplus
}
#endif

#endif
