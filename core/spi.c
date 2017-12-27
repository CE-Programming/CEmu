#include "spi.h"
#include "emu.h"
#include "schedule.h"

spi_state_t spi;

/* Read from the SPI range of ports */
static uint8_t spi_read(const uint16_t pio, bool peek) {
    (void)peek;
    (void)pio;
    return 0;
}

/* Write to the SPI range of ports */
static void spi_write(const uint16_t pio, const uint8_t byte, bool poke) {
    (void)poke;
    (void)byte;
    (void)pio;
    return;
}

static const eZ80portrange_t pspi = {
    .read  = spi_read,
    .write = spi_write
};

eZ80portrange_t init_spi(void) {
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
