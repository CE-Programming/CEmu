#include "core/exxx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global EXXX state
exxx_state_t exxx;

// Read from the 0xEXXX range of ports
uint8_t exxx_read(const uint16_t pio) {

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
void exxx_write(const uint16_t pio, const uint8_t byte)
{
    uint8_t addr = pio & 0x7F;
    exxx.ports[addr] = byte;
}

eZ80portrange_t init_exxx() {
    int i;
    // Initialize device to default state
    for(i = 0; i<0x80; i++) {
        exxx.ports[i] = 0;
    }

    eZ80portrange_t device = {
        .read_in = exxx_read,
        .write_out = exxx_write
    };

    return device;
}
