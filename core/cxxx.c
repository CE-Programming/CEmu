#include "core/cxxx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global CXXX state
cxxx_state_t cxxx;

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

static const eZ80portrange_t device = {
    .read_in = cxxx_read,
    .write_out = cxxx_write
};

eZ80portrange_t init_cxxx() {
    int i;
    // Initialize device to default state
    for(i = 0; i<0x80; i++) {
        cxxx.ports[i] = 0;
    }

    return device;
}
