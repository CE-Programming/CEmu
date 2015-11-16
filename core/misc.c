#include "core/misc.h"

cxxx_state_t cxxx; // Global CXXX state
exxx_state_t exxx; // Global EXXX state
fxxx_state_t fxxx; // Global FXXX state

// Read from the 0xCXXX range of ports
static uint8_t cxxx_read(const uint16_t pio) {

  uint8_t addr = pio&0xFF;
  uint8_t read_byte;

  read_byte = cxxx.ports[addr];

  return read_byte;
}

// Write to the 0xCXXX range of ports
static void cxxx_write(const uint16_t pio, const uint8_t byte)
{
    uint8_t addr = pio & 0xFF;

    cxxx.ports[addr] = byte;
}

static const eZ80portrange_t pcxxx = {
    .read_in = cxxx_read,
    .write_out = cxxx_write
};

eZ80portrange_t init_cxxx() {
    int i;
    // Initialize device to default state
    for(i = 0; i<0x80; i++) {
        cxxx.ports[i] = 0;
    }

    return pcxxx;
}

/* ============================================= */

// Read from the 0xEXXX range of ports
static uint8_t exxx_read(const uint16_t pio) {

  uint8_t addr = pio&0x7F;
  uint8_t read_byte;

  switch (addr) {
      case 0x14:
                 read_byte = 32 | exxx.ports[addr];
                 break;
      default:
                 read_byte = exxx.ports[addr];
                 break;
  }
  return read_byte;
}

// Write to the 0xEXXX range of ports
static void exxx_write(const uint16_t pio, const uint8_t byte)
{
    uint8_t addr = pio & 0x7F;
    exxx.ports[addr] = byte;
}

static const eZ80portrange_t pexxx = {
    .read_in = exxx_read,
    .write_out = exxx_write
};

eZ80portrange_t init_exxx() {
    int i;
    // Initialize device to default state
    for(i = 0; i<0x80; i++) {
        exxx.ports[i] = 0;
    }

    return pexxx;
}

/* ============================================= */

// Read from the 0xFXXX range of ports
static void fxxx_write(const uint16_t pio, const uint8_t value) {
    //printf("Wrote to unmapped range: 0x%04X <- 0x%02X", pio, value);
}

// Read from the 0xFXXX range of ports
static uint8_t fxxx_read(const uint16_t pio) {
    //printf("Read from unmapped range: 0x%04X", pio);
    return 0x00;
}

static const eZ80portrange_t pfxxx = {
    .read_in = fxxx_read,
    .write_out = fxxx_write
};

eZ80portrange_t init_fxxx(void) {
    return pfxxx;
}
