#include "spi.h"
#include "bus.h"
#include "cpu.h"
#include "debug/debug.h"
#include "emu.h"
#include "interrupt.h"
#include "schedule.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

spi_state_t spi;

static bool spi_scan_line(uint16_t row) {
    if (unlikely(row > SPI_LAST_ROW)) {
        spi.mode |= SPI_MODE_IGNORE;
        return false;
    }
    spi.mode &= ~SPI_MODE_IGNORE;
    if (unlikely(spi.mode & SPI_MODE_PARTIAL) &&
        spi.partialStart > spi.partialEnd ?
        spi.partialStart > row && row > spi.partialEnd :
        spi.partialStart > row || row > spi.partialEnd) {
        spi.mode |= SPI_MODE_BLANK;
    } else {
        spi.mode &= ~SPI_MODE_BLANK;
    }
    spi.row = spi.dstRow = spi.srcRow = row;
    if (unlikely(spi.mode & SPI_MODE_SCROLL)) {
        uint16_t top = spi.topArea, bot = SPI_LAST_ROW - spi.bottomArea;
        if (row >= top && row <= bot) {
            spi.srcRow += spi.scrollStart - top;
            if (spi.srcRow > bot) {
                spi.srcRow -= SPI_NUM_ROWS - spi.topArea - spi.bottomArea;
            }
            spi.srcRow &= 0x1FF;
        }
    }
    if (unlikely(spi.mac & SPI_MAC_VRO)) {
        spi.dstRow = SPI_LAST_ROW - spi.dstRow;
        spi.srcRow = SPI_LAST_ROW - spi.srcRow;
    }
    if (unlikely(spi.mac & SPI_MAC_HRO)) {
        spi.col = SPI_LAST_COL;
        spi.colDir = -1;
    } else {
        spi.col = 0;
        spi.colDir = 1;
    }
    return true;
}

bool spi_hsync(void) {
    return spi_scan_line(spi.row + 1);
}

static void spi_reset_mregs(void) {
    if (unlikely(spi.mac & SPI_MAC_RCX)) {
        spi.rowReg = spi.rowStart;
        spi.colReg = spi.colStart;
    } else {
        spi.rowReg = spi.colStart;
        spi.colReg = spi.rowStart;
    }
}

bool spi_vsync(void) {
    if (likely(spi.ifCtl & SPI_IC_CTRL_DATA)) {
        spi_reset_mregs();
    }
    return spi_scan_line(0);
}

bool spi_refresh_pixel(void) {
    uint8_t *pixel, red, green, blue;
    if (unlikely(spi.mode & SPI_MODE_IGNORE)) {
        return false;
    }
    if (unlikely(spi.mode & (SPI_MODE_SLEEP | SPI_MODE_OFF | SPI_MODE_BLANK))) {
        red = green = blue = ~0;
    } else {
        if (unlikely(spi.srcRow > SPI_LAST_ROW)) {
            red = bus_rand();
            green = bus_rand();
            blue = bus_rand();
        } else {
            pixel = spi.frame[spi.srcRow][spi.col];
            red = pixel[SPI_RED];
            green = pixel[SPI_GREEN];
            blue = pixel[SPI_BLUE];
        }
        if (!likely(spi.mac & SPI_MAC_BGR)) { /* eor */
            uint8_t temp = red;
            red = blue;
            blue = temp;
        }
        if (unlikely(spi.mode & SPI_MODE_INVERT)) {
            red = ~red;
            green = ~green;
            blue = ~blue;
        }
        if (unlikely(spi.mode & SPI_MODE_IDLE)) {
            red = (int8_t)red >> 7;
            green = (int8_t)green >> 7;
            blue = (int8_t)blue >> 7;
        }
    }
    pixel = spi.display[spi.col][spi.dstRow];
    pixel[SPI_RED] = red;
    pixel[SPI_GREEN] = green;
    pixel[SPI_BLUE] = blue;
    pixel[SPI_ALPHA] = ~0;
    spi.col += spi.colDir;
    if (unlikely(spi.col > SPI_LAST_COL)) {
        spi.mode |= SPI_MODE_IGNORE;
        return false;
    }
    return true;
}

static void spi_update_pixel(uint8_t red, uint8_t green, uint8_t blue) {
    if (likely(spi.rowReg < 320 && spi.colReg < 240)) {
        uint8_t *pixel = spi.frame[spi.rowReg][spi.colReg];
        pixel[SPI_RED] = red;
        pixel[SPI_GREEN] = green;
        pixel[SPI_BLUE] = blue;
    }
    if (unlikely(spi.mac & SPI_MAC_RCX)) {
        if (unlikely(spi.colReg == spi.colEnd)) {
            if (unlikely(spi.rowReg == spi.rowEnd && spi.rowStart <= spi.rowEnd)) {
                spi.rowReg = spi.colReg = ~0;
            } else {
                spi.colReg = spi.colStart;
                spi.rowReg = (spi.rowReg + 1 - (spi.mac >> 6 & 2)) & 0x1FF;
            }
        } else if (spi.colReg < 0x100) {
            spi.colReg = (spi.colReg + 1 - (spi.mac >> 5 & 2)) & 0xFF;
        }
    } else {
        if (unlikely(spi.rowReg == spi.colEnd)) {
            if (unlikely(spi.colReg == spi.rowEnd && spi.rowStart <= spi.rowEnd)) {
                spi.rowReg = spi.colReg = ~0;
            } else {
                spi.rowReg = spi.colStart;
                spi.colReg = (spi.colReg + 1 - (spi.mac >> 5 & 2)) & 0xFF;
            }
        } else if (spi.rowReg < 0x200) {
            spi.rowReg = (spi.rowReg + 1 - (spi.mac >> 6 & 2)) & 0x1FF;
        }
    }
}

void spi_update_pixel_18bpp(uint8_t red, uint8_t green, uint8_t blue) {
    assert(red < 64 && green < 64 && blue < 64);
    spi_update_pixel(red << 2 | red >> 4, green << 2 | green >> 4, blue << 2 | blue >> 4);
}

void spi_update_pixel_16bpp(uint8_t red, uint8_t green, uint8_t blue) {
    assert(red < 32 && green < 64 && blue < 32);
    spi_update_pixel(spi.lut[red + 0], spi.lut[green + 32], spi.lut[blue + 96]);
}

void spi_update_pixel_12bpp(uint8_t red, uint8_t green, uint8_t blue) {
    assert(red < 16 && green < 16 && blue < 16);
    spi_update_pixel(spi.lut[(red << 1) + 0], spi.lut[(green << 2) + 32], spi.lut[(blue << 1) + 96]);
}

static void spi_sw_reset(void) {
    spi.cmd = 0;
    spi.param = 0;
    spi.gamma = 1;
    spi.mode = SPI_MODE_SLEEP | SPI_MODE_OFF;
    spi.colStart = 0;
    spi.colEnd = spi.mac & SPI_MAC_RCX ? SPI_LAST_COL : SPI_LAST_ROW;
    spi.rowStart = 0;
    spi.rowEnd = spi.mac & SPI_MAC_RCX ? SPI_LAST_ROW : SPI_LAST_COL;
    spi.topArea = 0;
    spi.scrollArea = SPI_NUM_ROWS;
    spi.bottomArea = 0;
    spi.partialStart = 0;
    spi.partialEnd = SPI_LAST_ROW;
    spi.scrollStart = 0;
    spi.tear = false;
}

static void spi_hw_reset(void) {
    spi.mac = 0;
    spi_sw_reset();
}

static void spi_write_cmd(uint8_t value) {
    spi.cmd = value;
    spi.param = 0;

    switch (spi.cmd) {
        case 0x00:
            break;
        case 0x01:
            spi_sw_reset();
            break;
        case 0x10:
            spi.mode |= SPI_MODE_SLEEP;
            break;
        case 0x11:
            spi.mode &= ~SPI_MODE_SLEEP;
            break;
        case 0x12:
            spi.mode |= SPI_MODE_PARTIAL;
            spi.scrollStart = 0;
            break;
        case 0x13:
            spi.mode &= ~(SPI_MODE_PARTIAL | SPI_MODE_SCROLL);
            spi.scrollStart = 0;
            break;
        case 0x20:
            spi.mode &= ~SPI_MODE_INVERT;
            break;
        case 0x21:
            spi.mode |= SPI_MODE_INVERT;
            break;
        case 0x28:
            spi.mode |= SPI_MODE_OFF;
            break;
        case 0x29:
            spi.mode &= ~SPI_MODE_OFF;
            break;
        case 0x2C:
            spi_reset_mregs();
            break;
        case 0x34:
            spi.tear = false;
            break;
        case 0x35:
            spi.tear = true;
            break;
        case 0x38:
            spi.mode &= ~SPI_MODE_IDLE;
            break;
        case 0x39:
            spi.mode |= SPI_MODE_IDLE;
            break;
        default:
            break;
    }
}

static void spi_write_param(uint8_t value) {
    uint8_t word_param = spi.param >> 1;
    uint8_t bit_offset = ~spi.param << 3 & 8;

    switch (spi.cmd) {
        case 0x26:
            if (spi.param == 0) {
                spi.gamma = value;
            }
            break;
        case 0x2A:
            switch (word_param) {
                case 0:
                    write8(spi.colStart, bit_offset, value & 0x1FF >> bit_offset);
                    break;
                case 1:
                    write8(spi.colEnd, bit_offset, value & 0x1FF >> bit_offset);
                    break;
                default:
                    break;
            }
            break;
        case 0x2B:
            switch (word_param) {
                case 0:
                    write8(spi.rowStart, bit_offset, value & 0x1FF >> bit_offset);
                    break;
                case 1:
                    write8(spi.rowEnd, bit_offset, value & 0x1FF >> bit_offset);
                    break;
                default:
                    break;
            }
            break;
        case 0x2C:
        case 0x3C:
            if (unlikely(!(spi.ifCtl & SPI_IC_CTRL_DATA))) {
                switch (spi.ifBpp & 7) {
                    default:
                    case 6: /* 18bpp */
                        switch (spi.param % 3) {
                            case 0:
                                spi.ifBlue = value >> 2;
                                break;
                            case 1:
                                spi.ifGreen = value >> 2;
                                break;
                            case 2:
                                spi.ifRed = value >> 2;
                                spi_update_pixel_18bpp(spi.ifRed, spi.ifGreen, spi.ifBlue);
                                break;
                        }
                        break;
                    case 5: /* 16bpp */
                        switch (spi.param % 2) {
                            case 0:
                                spi.ifBlue = value >> 3;
                                spi.ifGreen = value << 3 & 0x38;
                                break;
                            case 1:
                                spi.ifGreen |= value >> 5;
                                spi.ifRed = value & 0x1F;
                                spi_update_pixel_16bpp(spi.ifRed, spi.ifGreen, spi.ifBlue);
                                break;
                        }
                        break;
                    case 3: /* 12bpp */
                        switch (spi.param % 3) {
                            case 0:
                                spi.ifBlue = value >> 4;
                                spi.ifGreen = value & 0xF;
                                break;
                            case 1:
                                spi.ifRed = value >> 4;
                                spi_update_pixel_12bpp(spi.ifRed, spi.ifGreen, spi.ifBlue);
                                spi.ifBlue = value & 0xF;
                                break;
                            case 2:
                                spi.ifGreen = value >> 4;
                                spi.ifRed = value & 0xF;
                                spi_update_pixel_12bpp(spi.ifRed, spi.ifGreen, spi.ifBlue);
                                break;
                        }
                        break;
                }
            }
            break;
        case 0x2D:
            break;
        case 0x30:
            switch (word_param) {
                case 0:
                    write8(spi.partialStart, bit_offset, value & 0x1FF >> bit_offset);
                    break;
                case 1:
                    write8(spi.partialEnd, bit_offset, value & 0x1FF >> bit_offset);
                    break;
                default:
                    break;
            }
            break;
        case 0x33:
            switch (word_param) {
                case 0:
                    write8(spi.topArea, bit_offset, value & 0x1FF >> bit_offset);
                    break;
                case 1:
                    write8(spi.scrollArea, bit_offset, value & 0x1FF >> bit_offset);
                    break;
                case 2:
                    write8(spi.bottomArea, bit_offset, value & 0x1FF >> bit_offset);
                    break;
                default:
                    break;
            }
            break;
        case 0x36:
            if (spi.param == 0) {
                spi.mac = value;
            }
            break;
        case 0x37:
            switch (word_param) {
                case 0:
                    write8(spi.scrollStart, bit_offset, value & 0x1FF >> bit_offset);
                    spi.mode |= SPI_MODE_SCROLL;
                    break;
                default:
                    break;
            }
            break;
        case 0x3A:
            switch (word_param) {
                case 0:
                    spi.ifBpp = value;
                    break;
                default:
                    break;
            }
            break;
        case 0xB0:
            switch (word_param) {
                case 0:
                    spi.ifCtl = value;
                    break;
                case 1:
                    break;
                default:
                    break;
            }
            break;
        case 0xE0:
            spi.gammaCorrection[0][spi.param] = value;
            break;
        case 0xE1:
            spi.gammaCorrection[1][spi.param] = value;
            break;
        default:
            break;
    }

    spi.param++;
}

static uint8_t panel_spi_select(uint32_t* rxData) {
    (void)rxData;
    /* The first transfer frame is always 9 bits */
    return 9;
}

static uint8_t panel_spi_transfer(uint32_t txData, uint32_t* rxData) {
    (void)rxData;
    (txData & 0x100 ? spi_write_param : spi_write_cmd)((uint8_t)txData);
    /* TODO: return different frame length after read commands */
    return 9;
}

static void panel_spi_deselect(void) {
}

static uint8_t null_spi_select(uint32_t* rxData) {
    (void)rxData;
    /* Set the device frame to the transfer size, to reduce scheduling */
    return (spi.cr1 >> 16 & 0x1F) + 1;
}

static uint8_t null_spi_transfer(uint32_t txData, uint32_t* rxData) {
    (void)txData;
    return null_spi_select(rxData);
}

static void null_spi_deselect(void) {
}

static void spi_set_device_funcs(void) {
    if (spi.arm) {
        spi.device_select = null_spi_select;
        spi.device_transfer = null_spi_transfer;
        spi.device_deselect = null_spi_deselect;
    } else {
        spi.device_select = panel_spi_select;
        spi.device_transfer = panel_spi_transfer;
        spi.device_deselect = panel_spi_deselect;
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
        bool txAvailable = (spi.cr2 >> 8 & 1) && spi.tfve != 0;
        if (unlikely(spi.cr2 >> 7 & 1)) {
            /* Odd RX behavior, allow transfer after 15 entries only if TX FIFO is non-empty */
            if (spi.rfve >= SPI_RXFIFO_DEPTH - (spi.tfve == 0)) {
                return 0;
            }
        } else if (!txAvailable) {
            /* If not receiving, disallow transfer if TX FIFO is empty or disabled */
            return 0;
        }

        spi.transferBits = (spi.cr1 >> 16 & 0x1F) + 1;
        if (likely(txAvailable)) {
            spi.txFrame = spi.txFifo[spi.tfvi++ & (SPI_TXFIFO_DEPTH - 1)] << (32 - spi.transferBits);
            spi.tfve--;
            spi_update_thresholds();
        } else {
            /* For now, just send 0 bits when no TX data available */
            spi.txFrame = 0;
            if (spi.cr2 >> 8 & 1) {
                /* Set TX underflow if TX enabled */
                spi.intStatus |= 1 << 1;
                intrpt_set(INT_SPI, spi.intCtrl & spi.intStatus);
            }
        }
    }

    uint8_t bitCount = spi.transferBits < spi.deviceBits ? spi.transferBits : spi.deviceBits;
    return bitCount * ((spi.cr1 & 0xFFFF) + 1);
}

static void spi_event(enum sched_item_id id) {
    uint8_t bitCount = spi.transferBits < spi.deviceBits ? spi.transferBits : spi.deviceBits;
    spi.rxFrame <<= bitCount;
    /* Handle loopback */
    if (unlikely(spi.cr0 >> 7 & 1)) {
        spi.rxFrame |= spi.txFrame >> (32 - bitCount);
    }
    /* For working receives, use the following:
    else if (asic.spiRxAllowed) {
        spi.rxFrame |= spi.deviceFrame >> (32 - bitCount);
    }
    */
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

    uint32_t ticks = spi_next_transfer();
    if (ticks) {
        sched_repeat(id, ticks);
    }
}

static void spi_update(void) {
    spi_update_thresholds();
    if (!(spi.cr2 & 1)) {
        if (spi.deviceBits != 0) {
            spi.device_deselect();
            spi.deviceFrame = spi.deviceBits = 0;
            spi.txFrame = spi.transferBits = 0;
            sched_clear(SCHED_SPI);
        }
    } else if (spi.transferBits == 0) {
        if (spi.deviceBits == 0) {
            uint32_t rxData = 0;
            spi.deviceBits = spi.device_select(&rxData);
            spi.deviceFrame = rxData << (32 - spi.deviceBits);
        }

        uint32_t ticks = spi_next_transfer();
        if (ticks) {
            sched_set(SCHED_SPI, ticks);
        }
    }
}

/* Read from the SPI range of ports */
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
    uint32_t shift = (addr & 3) << 3, value = (uint32_t)byte << shift, mask = ~(0xFFu << shift);
    bool stateChanged = false;
    switch (addr >> 2) {
        case 0x00 >> 2: // CR0
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
        case 0x18 >> 2: // DATA
            spi.dtr &= mask;
            spi.dtr |= value;
            if (!shift && spi.tfve != SPI_TXFIFO_DEPTH) {
                spi.txFifo[(spi.tfvi + spi.tfve++) & (SPI_TXFIFO_DEPTH - 1)] = spi.dtr;
                stateChanged = true;
            }
            break;
    }
    if (stateChanged) {
        spi_update();
    }
}

static const eZ80portrange_t pspi = {
    .read  = spi_read,
    .write = spi_write
};


void spi_reset(void) {
    uint8_t i = 0, c;
    memset(&spi, 0, sizeof(spi));
    spi_hw_reset();
    for (c = 0; c < 1 << 5; c++) {
        spi.lut[i++] = c << 3 | c >> 2;
    }
    for (c = 0; c < 1 << 6; c++) {
        spi.lut[i++] = c << 2 | c >> 4;
    }
    for (c = 0; c < 1 << 5; c++) {
        spi.lut[i++] = c << 3 | c >> 2;
    }

    spi_set_device_funcs();

    sched.items[SCHED_SPI].callback.event = spi_event;
    sched.items[SCHED_SPI].clock = CLOCK_12M;
    sched_clear(SCHED_SPI);
}

void spi_device_select(bool arm) {
    if (spi.arm != arm) {
        spi.device_deselect();
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
    if (fread(&spi, offsetof(spi_state_t, device_select), 1, image) == 1) {
        spi_set_device_funcs();
        return true;
    }
    return false;
}
