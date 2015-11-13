#include "core/fxxx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global FXXX state
fxxx_state_t fxxx;

// Read from the 0xFXXX range of ports
static void fxxx_write(const uint16_t pio, const uint8_t value) {
    //printf("Wrote to unmapped range: 0x%04X <- 0x%02X", pio, value);
}

// Read from the 0xFXXX range of ports
static uint8_t fxxx_read(const uint16_t pio) {
    //printf("Read from unmapped range: 0x%04X", pio);
    return 0x00;
}

static const eZ80portrange_t device = {
    .read_in = fxxx_read,
    .write_out = fxxx_write
};

eZ80portrange_t init_fxxx(void) {
    return device;
}
