#ifndef UART_H
#define UART_H

#ifdef __cplusplus
extern "C" {
#endif

#include "port.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#define UART_FIFO_DEPTH 16

#define UART_LCR_CHAR_LEN (3 << 0)
#define UART_LCR_STOP_BITS (1 << 2)
#define UART_LCR_PARITY_ENABLE (1 << 3)
#define UART_LCR_PARITY_EVEN (1 << 4)
#define UART_LCR_PARITY_STICK (1 << 5)
#define UART_LCR_BREAK (1 << 6)

#define UART_MSR_CTS (1 << 0)
#define UART_MSR_DSR (1 << 1)
#define UART_MSR_RI (1 << 2)
#define UART_MSR_DCD (1 << 3)
#define UART_MSR_BITS (UART_MSR_CTS | UART_MSR_DSR | UART_MSR_RI | UART_MSR_DCD)

typedef struct uart_transfer {
    uint8_t ch;
    uint8_t lcr;
    uint16_t divisor;
} uart_transfer_t;

typedef struct uart_state {
    uint16_t divisor;
    uint8_t ier, isr;
    uint8_t fcr, lcr, mcr;
    uint8_t lsr, msr;
    uint8_t scratch;

    bool txActive;
    uint8_t tfvi, tfve, tsr;
    bool rxTimeout;
    uint8_t rxTimeoutChars;
    uint8_t rfvi, rfve, rxstatCount;
    uint8_t txfifo[UART_FIFO_DEPTH], rxfifo[UART_FIFO_DEPTH], rxstat[UART_FIFO_DEPTH];

    void (*transmit_char)(const uart_transfer_t *transfer);
    bool (*receive_char)(uart_transfer_t *transfer);
} uart_state_t;

extern uart_state_t uart;

void uart_set_modem_inputs(uint8_t msr);

eZ80portrange_t init_uart(void);
void uart_reset(void);
bool uart_restore(FILE *image);
bool uart_save(FILE *image);

#ifdef __cplusplus
}
#endif

#endif
