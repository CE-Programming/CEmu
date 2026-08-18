#include "coproc.h"
#include "debug/debug.h"
#include "defines.h"
#include "emu.h"
#include "interrupt.h"
#include "schedule.h"

#include <string.h>

coproc_state_t coproc;

static const uint8_t coproc_image_magic[] = { 'C', 'A', 'R', 'M' };

void coproc_free(void) {
    arm_destroy(coproc.arm);
    memset(&coproc, 0, sizeof(coproc));
}

void coproc_reset(void) {
    gui_console_printf("[CEmu] Reset Coprocessor Interface...\n");
    if (asic.python && !coproc.arm) {
        coproc.arm = arm_create();
    }
    if (coproc.arm) {
        if (asic.python) {
            arm_reset(coproc.arm);
        } else {
            coproc_free();
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

bool coproc_save(FILE *image) {
    const uint8_t present = coproc.arm != NULL;
    return fwrite(coproc_image_magic, sizeof(coproc_image_magic), 1, image) == 1 &&
           fwrite(&present, sizeof(present), 1, image) == 1 &&
           (!present || (arm_save_flash(coproc.arm, image) &&
                         arm_save_state(coproc.arm, image)));
}

bool coproc_restore(FILE *image) {
    uint8_t magic[sizeof(coproc_image_magic)];
    uint8_t present;
    if (fread(magic, sizeof(magic), 1, image) != 1 ||
        memcmp(magic, coproc_image_magic, sizeof(magic)) != 0 ||
        fread(&present, sizeof(present), 1, image) != 1 || present > 1) {
        return false;
    }
    if (!present) {
        if (asic.python) {
            return false;
        }
        coproc_free();
        return true;
    }
    if (!asic.python) {
        return false;
    }
    arm_t *restored = arm_create();
    if (!restored) {
        return false;
    }
    if (!arm_restore_flash(restored, image) || !arm_restore_state(restored, image)) {
        arm_destroy(restored);
        return false;
    }
    coproc_free();
    coproc.arm = restored;
    return true;
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
