#include "backlight.h"
#include "emu.h"

#include <stdio.h>
#include <string.h>

/* Global BACKLIGHT state */
backlight_state_t backlight;

/* Read from the 0xBXXX range of ports */
static uint8_t backlight_read(const uint16_t pio, bool peek) {
    uint8_t index = (pio >> 2) & 0xFF;
    uint8_t value;
    (void)peek;

    switch (index) {
        case 0x02: /* Ports B008-B00B always read 0 */
        case 0x03: /* Ports B00D-B00F always read 0 */
        case 0x04: /* Ports B00D-B00F always read 0 */
        case 0x07: /* Ports B01C-B01F always read 0 */
            value = 0;
            break;
        case 0x09:
            value = backlight.brightness;
            break;
        default:
            value = backlight.ports[index];
            break;
    }
    return value;
}

/* Write to the 0xBXXX range of ports */
static void backlight_write(const uint16_t pio, const uint8_t byte, bool poke) {
    uint8_t index = (uint8_t)pio;
    (void)poke;

    switch (index) {
        case 0x01:
        case 0x02:
        case 0x03:
        case 0x0B:
            break;
        case 0x21:
        case 0x22:
        case 0x25:
        case 0x26:
            if (byte) {
                backlight.brightness = 0;
            }
            break;
        case 0x24:
            backlight.brightness = byte;
            break;
        default:
            backlight.ports[index] = byte;
            break;
    }

    backlight.factor = (310 - (float)backlight.brightness) / (float)160.0;
}

static const eZ80portrange_t device = {
    .read  = backlight_read,
    .write = backlight_write
};

eZ80portrange_t init_backlight(void) {
    memset(backlight.ports, 0, sizeof backlight.ports);
    backlight.brightness = 0xFF;
    gui_console_printf("[CEmu] Initialized Backlight...\n");
    return device;
}

void backlight_reset(void) {
    memset(backlight.ports, 0, sizeof backlight.ports);
    backlight.ports[0x00] = 0x64;
    backlight.ports[0x01] = 0x64;
    backlight.ports[0x02] = 0x61;
    backlight.ports[0x03] = 0x4C;
    backlight.ports[0x20] = 0xFF; /* backlight scaler? (unimplemented) */
    backlight.brightness = 0xFF;  /* backlight level (PWM)             */
}

bool backlight_save(FILE *image) {
    return fwrite(&backlight, sizeof(backlight), 1, image) == 1;
}

bool backlight_restore(FILE *image) {
    return fread(&backlight, sizeof(backlight), 1, image) == 1;
}
