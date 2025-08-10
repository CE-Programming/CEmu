#include "coproc.h"
#include "debug/debug.h"
#include "defines.h"
#include "emu.h"
#include "interrupt.h"
#include "schedule.h"

#include <string.h>

coproc_state_t coproc;

void coproc_reset(void) {
    gui_console_printf("[CEmu] Reset Coprocessor Interface...\n");
    if (asic.python && !coproc.arm) {
        coproc.arm = arm_create();
    }
    if (coproc.arm) {
        if (asic.python) {
            arm_reset(coproc.arm);
        } else {
            arm_destroy(coproc.arm);
            memset(&coproc, 0, sizeof(coproc));
        }
    }
}

bool coproc_load(const char *path) {
    if (asic.python && !coproc.arm) {
        coproc.arm = arm_create();
    }
    if (coproc.arm) {
        return arm_load(coproc.arm, path);
    } else {
        return false;
    }
}

void coproc_uart_transmit(const uart_transfer_t *transfer) {
    /* TODO: Send line control & divisor as well */
    arm_usart_send(coproc.arm, transfer->ch);
}

bool coproc_uart_receive(uart_transfer_t *transfer) {
    /* TODO: Populate line control & divisor based on ARM config */
    transfer->lcr = 3;
    transfer->divisor = 13;
    return arm_usart_recv(coproc.arm, &transfer->ch);
}

void coproc_spi_select(bool low) {
    arm_spi_sel(coproc.arm, low);
}

uint8_t coproc_spi_peek(uint32_t *rxData) {
    return arm_spi_peek(coproc.arm, rxData);
}

uint8_t coproc_spi_transfer(uint32_t txData, uint32_t *rxData) {
    return arm_spi_xfer(coproc.arm, txData, rxData);
}
