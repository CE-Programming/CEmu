#include "spi.h"
#include "emu.h"
#include "schedule.h"

spi_state_t spi;

static void spi_write_cmd(uint8_t value) {
    spi.cmd = value;
    spi.param = 0;
}

static void spi_write_param(uint8_t value) {
    (void)value;

    switch (spi.cmd) {
        case SPI_COMMAND:
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
static void spi_write(const uint16_t pio, const uint8_t value, bool poke) {
    (void)poke;

    if (pio == 0xD018) {
        spi.fifo |= (value & 7) << spi.shift;
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
    spi.param = 0;
    spi.cmd = SPI_COMMAND;
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
