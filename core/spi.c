#include "spi.h"
#include "coproc.h"
#include "cpu.h"
#include "debug/debug.h"
#include "emu.h"
#include "interrupt.h"
#include "panel.h"
#include "schedule.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

spi_state_t spi;

static void null_spi_select(bool low) {
    (void)low;
}

static uint8_t null_spi_peek(uint32_t *rxData) {
    /* Hack to make OS 5.7.0 happy without a coprocessor */
    *rxData = 0xC3;
    return 8;
}

static uint8_t null_spi_transfer(uint32_t txData, uint32_t *rxData) {
    (void)txData;
    return null_spi_peek(rxData);
}

static void spi_set_device_funcs(void) {
    if (spi.arm) {
        if (asic.python) {
            spi.device_select = coproc_spi_select;
            spi.device_peek = coproc_spi_peek;
            spi.device_transfer = coproc_spi_transfer;
        } else {
            spi.device_select = null_spi_select;
            spi.device_peek = null_spi_peek;
            spi.device_transfer = null_spi_transfer;
        }
    } else {
        spi.device_select = panel_spi_select;
        spi.device_peek = panel_spi_peek;
        spi.device_transfer = panel_spi_transfer;
    }
}

static uint8_t spi_get_threshold_status(void) {
    uint8_t txThreshold = spi.intCtrl >> 12 & 0x1F;
    uint8_t rxThreshold = spi.intCtrl >> 7 & 0x1F;
    return (txThreshold && spi.tfve <= txThreshold) << 3
         | (rxThreshold && spi.rfve >= rxThreshold) << 2;
}

static void spi_update_thresholds(void) {
    if (unlikely(spi.intCtrl & (0x1F << 12 | 0x1F << 7))) {
        uint8_t statusDiff = (spi.intStatus & (1 << 3 | 1 << 2)) ^ spi_get_threshold_status();
        if (unlikely(statusDiff)) {
            spi.intStatus ^= statusDiff;
            intrpt_set(INT_SPI, spi.intCtrl & spi.intStatus);
        }
    }
}

static uint32_t spi_next_transfer(void) {
    if (spi.transferBits == 0) {
        bool txEnabled = (spi.cr2 >> 8 & 1);
        bool txAvailable = txEnabled && spi.tfve != 0;
        if (unlikely(spi.cr2 >> 7 & 1)) {
            /* If FLASH bit is reset and TX is enabled, only receive when the TX FIFO is non-empty */
            if (!(spi.cr0 >> 11 & 1) && txEnabled && spi.tfve == 0) {
                return 0;
            }
            /* Odd RX behavior, allow transfer after 15 entries only if TX FIFO is non-empty */
            if (spi.rfve >= SPI_RXFIFO_DEPTH - (spi.tfve == 0)) {
                return 0;
            }
        } else if (!txAvailable) {
            /* If not receiving, disallow transfer if TX FIFO is empty or disabled */
            return 0;
        }

        spi.transferBits = (spi.cr1 >> 16 & 0x1F) + 1;
        spi.txFrame = spi.txFifo[spi.tfvi & (SPI_TXFIFO_DEPTH - 1)] << (32 - spi.transferBits);
        if (likely(txAvailable)) {
            spi.tfvi++;
            spi.tfve--;
            spi_update_thresholds();
        } else if (unlikely(txEnabled)) {
            /* Set TX underflow if TX enabled */
            spi.intStatus |= 1 << 1;
            intrpt_set(INT_SPI, spi.intCtrl & spi.intStatus);
        }
    }

    if (unlikely(spi.deviceBits == 0)) {
        uint32_t rxData = 0;
        spi.deviceBits = spi.device_peek(&rxData);
        spi.deviceFrame = rxData << (32 - spi.deviceBits);
    }

    uint8_t bitCount = spi.transferBits < spi.deviceBits ? spi.transferBits : spi.deviceBits;
    return bitCount * ((spi.cr1 & 0xFFFF) + 1);
}

static void spi_event(enum sched_item_id id) {
    if (likely(spi.transferBits != 0)) {
        uint8_t bitCount = spi.transferBits < spi.deviceBits ? spi.transferBits : spi.deviceBits;
        spi.rxFrame <<= bitCount;
        /* Handle loopback */
        if (unlikely(spi.cr0 >> 7 & 1)) {
            spi.rxFrame |= spi.txFrame >> (32 - bitCount);
        }
        /* For now, allow only receives from coprocessor */
        else if (spi.arm) {
            spi.rxFrame |= spi.deviceFrame >> (32 - bitCount);
        }
        spi.deviceFrame <<= bitCount;
        spi.deviceFrame |= spi.txFrame >> (32 - bitCount);
        spi.txFrame <<= bitCount;

        spi.deviceBits -= bitCount;
        if (spi.deviceBits == 0) {
            uint32_t rxData = 0;
            spi.deviceBits = spi.device_transfer(spi.deviceFrame, &rxData);
            spi.deviceFrame = rxData << (32 - spi.deviceBits);
        }

        spi.transferBits -= bitCount;
        if (spi.transferBits == 0 && unlikely(spi.cr2 >> 7 & 1)) {
            /* Note: rfve bound check was performed when starting the transfer */
            spi.rxFifo[(spi.rfvi + spi.rfve++) & (SPI_RXFIFO_DEPTH - 1)] = spi.rxFrame;
            spi_update_thresholds();
        }
    }

    uint32_t ticks = spi_next_transfer();
    if (ticks) {
        sched_repeat(id, ticks);
    }
}

static void spi_update(void) {
    spi_update_thresholds();
    if (!(spi.cr2 & 1)) {
        if (spi.deviceBits != 0 || sched_active(SCHED_SPI)) {
            spi.device_select(false);
            spi.deviceFrame = spi.deviceBits = 0;
            spi.txFrame = spi.transferBits = 0;
            sched_clear(SCHED_SPI);
        }
    } else if (!sched_active(SCHED_SPI)) {
        assert(spi.transferBits == 0);
        if (spi.deviceBits == 0) {
            spi.device_select(true);
            /* Delay one bit length after enable and before first transfer */
            sched_set(SCHED_SPI, (spi.cr1 & 0xFFFF) + 1);
        } else {
            uint32_t ticks = spi_next_transfer();
            if (ticks) {
                sched_set(SCHED_SPI, ticks);
            }
        }
    }
}

/* Read from the SPI range of ports */
/* TODO: something with asic.hasWorkingSPIReads probably */
static uint8_t spi_read(uint16_t addr, bool peek) {
    static const uint16_t cr0_masks[16] = {
        0xF18C, 0xF8EF, 0xF0AC, 0xF3FC, 0xF18C, 0xF4C0, 0xF3AC, 0xF3AC,
        0xF18C, 0xFBEF, 0xF0AC, 0xF3FC, 0xF18C, 0xF4C0, 0xF3AC, 0xF3AC
    };

    uint32_t shift = (addr & 3) << 3, value = 0;
    switch (addr >> 2) {
        case 0x00 >> 2: // CR0
            value = spi.cr0;
            value &= cr0_masks[value >> 12 & 0xF];
            break;
        case 0x04 >> 2: // CR1
            value = spi.cr1;
            break;
        case 0x08 >> 2: // CR2
            value = spi.cr2;
            break;
        case 0x0C >> 2: // STATUS
            value = spi.tfve << 12 | spi.rfve << 4 |
                (spi.transferBits != 0) << 2 |
                (spi.tfve != SPI_TXFIFO_DEPTH) << 1 | (spi.rfve == SPI_RXFIFO_DEPTH) << 0;
            break;
        case 0x10 >> 2: // INTCTRL
            value = spi.intCtrl;
            break;
        case 0x14 >> 2: // INTSTATUS
            value = spi.intStatus;
            if (!peek) {
                spi.intStatus &= ~(1 << 1 | 1 << 0);
                intrpt_set(INT_SPI, spi.intCtrl & spi.intStatus);
            }
            break;
        case 0x18 >> 2: // DATA
            value = spi.rxFifo[spi.rfvi & (SPI_RXFIFO_DEPTH - 1)];
            if (!peek && !shift && spi.rfve) {
                spi.rfvi++;
                spi.rfve--;
                spi_update();
            }
            break;
        case 0x60 >> 2: // REVISION
            value = 0x01 << 16 | 0x21 << 8 | 0x00 << 0;
            break;
        case 0x1C >> 2:
        case 0x64 >> 2: // FEATURE
            value = SPI_FEATURES << 24 | (SPI_TXFIFO_DEPTH - 1) << 16 |
                (SPI_RXFIFO_DEPTH - 1) << 8 | (SPI_WIDTH - 1) << 0;
            break;
    }
    return value >> shift;
}

/* Write to the SPI range of ports */
static void spi_write(uint16_t addr, uint8_t byte, bool poke) {
    (void)poke;
    uint32_t shift = (addr & 3) << 3, value = (uint32_t)byte << shift, mask = ~(0xFFu << shift);
    bool stateChanged = false;
    switch (addr >> 2) {
        case 0x00 >> 2: // CR0
            if ((spi.cr0 ^ value) & ~mask & (1 << 11)) {
                stateChanged = true;
            }
            value &= 0xFFFF;
            spi.cr0 &= mask;
            spi.cr0 |= value;
            break;
        case 0x04 >> 2: // CR1
            value &= 0x7FFFFF;
            spi.cr1 &= mask;
            spi.cr1 |= value;
            break;
        case 0x08 >> 2: // CR2
            if (value & 1 << 2) {
                spi.rfvi = spi.rfve = 0;
                stateChanged = true;
            }
            if (value & 1 << 3) {
                spi.tfvi = spi.tfve = 0;
                stateChanged = true;
            }
            if ((spi.cr2 ^ value) & ~mask & (1 << 8 | 1 << 7 | 1 << 0)) {
                stateChanged = true;
            }
            value &= 0xF83;
            spi.cr2 &= mask;
            spi.cr2 |= value;
            break;
        case 0x10 >> 2: // INTCTRL
            value &= 0x1FFBF;
            spi.intCtrl &= mask;
            spi.intCtrl |= value;
            spi.intStatus &= ~(1 << 3 | 1 << 2);
            spi.intStatus |= spi_get_threshold_status();
            intrpt_set(INT_SPI, spi.intCtrl & spi.intStatus);
            break;
        case 0x18 >> 2: { // DATA
            uint32_t *fifoEntry = &spi.txFifo[(spi.tfvi + spi.tfve) & (SPI_TXFIFO_DEPTH - 1)];
            *fifoEntry &= mask;
            *fifoEntry |= value;
            if (!shift && spi.tfve != SPI_TXFIFO_DEPTH) {
                spi.tfve++;
                stateChanged = true;
            }
            break;
        }
    }
    if (stateChanged) {
        spi_update();
    }
}

static const eZ80portrange_t pspi = {
    .read  = spi_read,
    .write = spi_write
};

static void spi_init_events(void) {
    sched_init_event(SCHED_SPI, CLOCK_24M, spi_event);
}

void spi_reset(void) {
    memset(&spi, 0, sizeof(spi));
    spi_set_device_funcs();
    spi_init_events();
}

void spi_device_select(bool arm) {
    if (spi.arm != arm) {
        spi.device_select(false);
        spi.arm = arm;
        spi_set_device_funcs();
        spi_update();
    }
}

eZ80portrange_t init_spi(void) {
    spi_reset();
    gui_console_printf("[CEmu] Initialized Serial Peripheral Interface...\n");
    return pspi;
}

bool spi_save(FILE *image) {
    return fwrite(&spi, offsetof(spi_state_t, device_select), 1, image) == 1;
}

bool spi_restore(FILE *image) {
    spi_init_events();
    if (fread(&spi, offsetof(spi_state_t, device_select), 1, image) == 1) {
        spi_set_device_funcs();
        return true;
    }
    return false;
}
