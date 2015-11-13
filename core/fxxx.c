#include "core/fxxx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global FXXX state
fxxx_state_t fxxx;

// Read from the 0xFXXX range of ports
uint8_t fxxx_read(const uint16_t pio) {
    return pio&0;
}

eZ80portrange_t init_fxxx() {
    eZ80portrange_t device = {
        .read_in = fxxx_read,
        .write_out = NULL
    };

    return device;
}
