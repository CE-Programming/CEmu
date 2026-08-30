#include "coproc.h"
#include "debug/debug.h"
#include "defines.h"
#include "emu.h"
#include "interrupt.h"
#include "schedule.h"

#include <string.h>

coproc_state_t coproc;

static const uint8_t coproc_image_magic[] = { 'C', 'A', 'R', 'M' };

static uint64_t coproc_cycle(void) {
    return sched_total_time(CLOCK_48M);
}

static void coproc_log_bootloader(void) {
    char description[ARM_BOOTLOADER_DESCRIPTION_SIZE];
    (void)arm_get_bootloader_info(coproc.arm, description, sizeof(description));
    gui_console_printf("[CEmu] ARM bootloader: %s.\n", description);
}

void coproc_advance(void) {
    if (coproc.arm) {
        arm_advance_to(coproc.arm, coproc_cycle());
    }
}

void coproc_pause(void) {
    if (coproc.arm) {
        arm_pause(coproc.arm);
    }
}

void coproc_resume(void) {
    coproc_advance();
}

void coproc_free(void) {
    arm_destroy(coproc.arm);
    memset(&coproc, 0, sizeof(coproc));
}

void coproc_reset(void) {
    gui_console_printf("[CEmu] Reset Coprocessor Interface...\n");
    if (asic.python && !coproc.arm) {
        coproc.arm = arm_create();
        if (coproc.arm) {
            gui_console_printf("[CEmu] Loaded bundled ARM flash.\n");
            coproc_log_bootloader();
            arm_set_time(coproc.arm, coproc_cycle());
        }
    }
    if (coproc.arm) {
        if (asic.python) {
            arm_reset(coproc.arm);
            /* sched_reset() precedes this reset callback, so rebase the ARM
             * budget to the reset scheduler epoch as well. */
            arm_set_time(coproc.arm, coproc_cycle());
        } else {
            coproc_free();
        }
    }
}

bool coproc_load(const char *path) {
    if (asic.python && !coproc.arm) {
        coproc.arm = arm_create();
        if (coproc.arm) {
            arm_set_time(coproc.arm, coproc_cycle());
        }
    }
    if (coproc.arm) {
        bool success = arm_load(coproc.arm, path);
        if (success) {
            gui_console_printf("[CEmu] Loaded ARM flash override: %s.\n", path);
            coproc_log_bootloader();
            arm_set_time(coproc.arm, coproc_cycle());
        }
        return success;
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
    arm_set_time(restored, coproc_cycle());
    coproc_free();
    coproc.arm = restored;
    gui_console_printf("[CEmu] Restored ARM flash from emulator image.\n");
    coproc_log_bootloader();
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
