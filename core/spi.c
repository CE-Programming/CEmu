#include "spi.h"
#include "emu.h"
#include "schedule.h"

#include <string.h>

spi_state_t spi;

static void spi_sw_reset(void) {
    spi.cmd = 0;
    spi.fifo = 0;
    spi.param = 0;
    spi.shift = 6;
    spi.gamma = 1;
    spi.sleep = true;
    spi.partial = false;
    spi.invert = false;
    spi.power = false;
    spi.colStart = 0;
    spi.colEnd = spi.MAC & 1 << 5 ? 0x13F : 0xEF;
    spi.rowStart = 0;
    spi.rowEnd = spi.MAC & 1 << 5 ? 0xEF : 0x13F;
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
    spi.MAC = 0;
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
            spi.rowCur = spi.rowStart;
            spi.colCur = spi.colStart;
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
    uint8_t sbit = spi.param >> 1;
    uint8_t mbit = ~spi.param & 1;
    (void)value;

    switch (spi.cmd) {
        case 0x26:
            if (spi.param == 0) {
                spi.gamma = value;
            }
            break;
        case 0x2A:
            switch (sbit) {
                case 0:
                    write8(spi.colStart, mbit, value);
                    break;
                case 1:
                    write8(spi.colEnd, mbit, value);
                    break;
                default:
                    break;
            }
            break;
        case 0x2B:
            switch (sbit) {
                case 0:
                    write8(spi.rowStart, mbit, value);
                    break;
                case 1:
                    write8(spi.rowEnd, mbit, value);
                    break;
                default:
                    break;
            }
            break;
        case 0x2C:
        case 0x3C:
            switch (sbit) {
                case 0:
                    write8(spi.colStart, mbit, value);
                    write8(spi.rowStart, mbit, value);
                    break;
                case 1:
                    write8(spi.colEnd, mbit, value);
                    write8(spi.rowEnd, mbit, value);
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
            switch (sbit) {
                case 0:
                    write8(spi.partialStart, mbit, value);
                    break;
                case 1:
                    write8(spi.partialEnd, mbit, value);
                    break;
                default:
                    break;
            }
            break;
        case 0x33:
            switch (sbit) {
                case 0:
                    write8(spi.topArea, mbit, value);
                    break;
                case 1:
                    write8(spi.scrollArea, mbit, value);
                    break;
                case 2:
                    write8(spi.bottomArea, mbit, value);
                    break;
                default:
                    break;
            }
            break;
        case 0x36:
            if (spi.param == 0) {
                spi.MAC = value;
            }
            break;
        case 0x37:
            switch (sbit) {
                case 0:
                    write8(spi.scrollStart, mbit, value);
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
        spi.fifo |= (byte & 7) << spi.shift;
        spi.shift -= 3;
        if (!spi.shift) {
            if (spi.fifo & 0x100) {
                spi_write_cmd(spi.fifo);
            } else {
                spi_write_param(spi.fifo);
            }
            spi.fifo = 0;
            spi.shift = 6;
        }
    }

    return;
}

static const eZ80portrange_t pspi = {
    .read  = spi_read,
    .write = spi_write
};


void spi_reset(void) {
    uint8_t i = 0, c;
    memset(&spi, 0, sizeof(spi));
    spi_hw_reset();
    for (c = 0; c < 1 << 5; c++)
        spi.lut[i++] = c << 1;
    for (c = 0; c < 1 << 6; c++)
        spi.lut[i++] = c << 0;
    for (c = 0; c < 1 << 5; c++)
        spi.lut[i++] = c << 1;
}

eZ80portrange_t init_spi(void) {
    spi_reset();
    return pspi;
}

bool spi_save(emu_image *s) {
    s->spi = spi;
    return true;
}

bool spi_restore(const emu_image *s) {
    spi = s->spi;
    return true;
}
