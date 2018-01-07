#include "spi.h"
#include "emu.h"
#include "schedule.h"

#include <assert.h>
#include <string.h>

spi_state_t spi;

void spi_hsync(void) {
    spi.colCur = (spi.mac >> 2 & 1) * 239;
    spi.rowCur += 1 - (spi.mac >> 3 & 2);
}

static void spi_reset_mregs(void) {
    if (__builtin_expect(spi.mac >> 5 & 1, 0)) {
        spi.rowReg = spi.rowStart;
        spi.colReg = spi.colStart;
    } else {
        spi.rowReg = spi.colStart;
        spi.colReg = spi.rowStart;
    }
}

void spi_vsync(void) {
    spi_reset_mregs();
    spi_hsync();
    spi.rowCur = (spi.mac >> 4 & 1) * 319;
}

static uint32_t spi_idle(uint32_t pixel, uint32_t bit, uint32_t mask) {
    return pixel & bit ? pixel | mask : pixel & ~mask;
}

void spi_update_pixel(void) {
    if (spi.colCur < 240 && spi.rowCur < 320) {
        uint32_t pixel = 0xFF000000 |
            (spi.frame[spi.rowCur][spi.colCur][spi.mac >> 2 & 2] & 0x3F) << 18 |
            (spi.frame[spi.rowCur][spi.colCur][1] & 0x3F) << 10 |
            (spi.frame[spi.rowCur][spi.colCur][~spi.mac >> 2 & 2] & 0x3F) << 2;
        if (__builtin_expect(spi.invert, 0)) {
            pixel ^= 0xFFFFFF;
        }
        if (__builtin_expect(spi.idle, 0)) {
            pixel = spi_idle(pixel, 0x800000, 0xFF0000);
            pixel = spi_idle(pixel, 0x008000, 0x00FF00);
            pixel = spi_idle(pixel, 0x000080, 0x0000FF);
        }
        spi.display[spi.colCur][spi.rowCur] = pixel;
        spi.colCur += 1 - (spi.mac >> 1 & 2);
    }
}

void spi_process_pixel(uint8_t r, uint8_t g, uint8_t b) {
    assert(r < 32 && g < 64 && b < 32);
    if (spi.rowReg < 320 && spi.colReg < 240) {
        spi.frame[spi.rowReg][spi.colReg][0] = spi.lut[r +  0];
        spi.frame[spi.rowReg][spi.colReg][1] = spi.lut[g + 32];
        spi.frame[spi.rowReg][spi.colReg][2] = spi.lut[b + 96];
    }
    if (__builtin_expect(spi.mac >> 5 & 1, 0)) {
        if (__builtin_expect(spi.colReg == spi.colEnd, 0)) {
            spi.colReg = spi.colStart;
            spi.rowReg = (spi.rowReg + 1 - (spi.mac >> 6 & 2)) & 0xFF;
        } else {
            spi.colReg += 1 - (spi.mac >> 5 & 2);
        }
        spi.colReg &= 0x1FF;
    } else {
        if (__builtin_expect(spi.rowReg == spi.colEnd, 0)) {
            spi.rowReg = spi.colStart;
            spi.colReg = (spi.colReg + 1 - (spi.mac >> 5 & 2)) & 0xFF;
        } else {
            spi.rowReg += 1 - (spi.mac >> 6 & 2);
        }
        spi.rowReg &= 0x1FF;
    }
}

static void spi_sw_reset(void) {
    spi.cmd = 0;
    spi.fifo = 1;
    spi.param = 0;
    spi.gamma = 1;
    spi.sleep = true;
    spi.partial = false;
    spi.invert = false;
    spi.power = false;
    spi.colStart = 0;
    spi.colEnd = spi.mac & 1 << 5 ? 0xEF : 0x13F;
    spi.rowStart = 0;
    spi.rowEnd = spi.mac & 1 << 5 ? 0x13F : 0xEF;
    spi.topArea = 0;
    spi.scrollArea = 0x140;
    spi.bottomArea = 0;
    spi.partialStart = 0;
    spi.partialEnd = 0x13F;
    spi.scrollStart = 0;
    spi.tear = false;
    spi.idle = false;
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
            spi.sleep = true;
            break;
        case 0x11:
            spi.sleep = false;
            break;
        case 0x12:
            spi.partial = true;
            break;
        case 0x13:
            spi.partial = false;
            break;
        case 0x20:
            spi.invert = false;
            break;
        case 0x21:
            spi.invert = true;
            break;
        case 0x28:
            spi.power = false;
            break;
        case 0x29:
            spi.power = true;
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
            spi.idle = false;
            break;
        case 0x39:
            spi.idle = true;
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
                    write8(spi.colStart, bit_offset, value);
                    break;
                case 1:
                    write8(spi.colEnd, bit_offset, value);
                    break;
                default:
                    break;
            }
            break;
        case 0x2B:
            switch (word_param) {
                case 0:
                    write8(spi.rowStart, bit_offset, value);
                    break;
                case 1:
                    write8(spi.rowEnd, bit_offset, value);
                    break;
                default:
                    break;
            }
            break;
        case 0x2C:
        case 0x3C:
            switch (word_param) {
                case 0:
                    write8(spi.colStart, bit_offset, value);
                    write8(spi.rowStart, bit_offset, value);
                    break;
                case 1:
                    write8(spi.colEnd, bit_offset, value);
                    write8(spi.rowEnd, bit_offset, value);
                    break;
                default:
                    break;
            }
            break;
        case 0x2D:
            if (spi.param < 128) {
                spi.lut[spi.param] = value;
            }
            break;
        case 0x30:
            switch (word_param) {
                case 0:
                    write8(spi.partialStart, bit_offset, value);
                    break;
                case 1:
                    write8(spi.partialEnd, bit_offset, value);
                    break;
                default:
                    break;
            }
            break;
        case 0x33:
            switch (word_param) {
                case 0:
                    write8(spi.topArea, bit_offset, value);
                    break;
                case 1:
                    write8(spi.scrollArea, bit_offset, value);
                    break;
                case 2:
                    write8(spi.bottomArea, bit_offset, value);
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
                    write8(spi.scrollStart, bit_offset, value);
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

/* Read from the SPI range of ports */
static uint8_t spi_read(const uint16_t pio, bool peek) {
    (void)peek;
    (void)pio;
    return 0;
}

/* Write to the SPI range of ports */
static void spi_write(const uint16_t pio, const uint8_t byte, bool poke) {
    (void)poke;

    if (pio == 0x18) {
        spi.fifo = spi.fifo << 3 | (byte & 7);
        if (spi.fifo & 0x200) {
            if (spi.fifo & 0x100) {
                spi_write_param(spi.fifo);
            } else {
                spi_write_cmd(spi.fifo);
            }
            spi.fifo = 1;
        }
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
        spi.lut[i++] = c << 1;
    }
    for (c = 0; c < 1 << 6; c++) {
        spi.lut[i++] = c << 0;
    }
    for (c = 0; c < 1 << 5; c++) {
        spi.lut[i++] = c << 1;
    }
}

eZ80portrange_t init_spi(void) {
    spi_reset();
    gui_console_printf("[CEmu] Initialized SPI...\n");
    return pspi;
}

bool spi_save(FILE *image) {
    return fwrite(&spi, sizeof(spi), 1, image) == 1;
}

bool spi_restore(FILE *image) {
    return fread(&spi, sizeof(spi), 1, image) == 1;
}
