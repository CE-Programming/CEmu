#include "core/backlight.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Global BACKLIGHT state */
backlight_state_t backlight;

/* Read from the 0xBXXX range of ports */
static uint8_t backlight_read(const uint16_t pio) {
    uint8_t addr = pio & 0xFF;
    uint8_t read_byte = 0;

    switch (addr) {
        case 0x08:
        case 0x09:
        case 0x0A:
        case 0x0B: //Ports B008-B00B always read 0
            read_byte = 0;
            break;
        case 0x0D:
        case 0x0E:
        case 0x0F: //Ports B00D-B00F always read 0
            read_byte = 0;
            break;
        case 0x10:
        case 0x11:
        case 0x12:
        case 0x13: //Ports B010-B013 always read 0
            read_byte = 0;
            break;
        case 0x1C:
        case 0x1D:
        case 0x1E:
        case 0x1F: //Ports B01C-B01F always read 0
            read_byte = 0;
            break;
        case 0x24:
            read_byte = backlight.brightness;
            break;
        default:
            read_byte = backlight.ports[addr];
            break;
    }
    return read_byte;
}

/* Write to the 0xBXXX range of ports */
static void backlight_write(const uint16_t pio, const uint8_t byte)
{
    uint8_t addr = pio & 0xFF;

    switch (addr) {
        case 0x01:
        case 0x02:
        case 0x03:
        case 0x0B: //Fixed?
            break;
        case 0x21:
        case 0x22:
        case 0x25:
        case 0x26:
            if(byte != 0) {
                backlight.brightness = 0x00;
            }
            break;
        case 0x24:
            backlight.brightness = byte;
            break;
        default:
            backlight.ports[addr] = byte;
            break;
    }
}

static const eZ80portrange_t device = {
    .read_in    = backlight_read,
    .write_out  = backlight_write
};

eZ80portrange_t init_backlight(void) {
    int i;
    /* Initialize device to default state */
    for(i = 0; i<0x100; i++) {
        backlight.ports[i] = 0x00;
    }
    backlight.ports[0x00] = 0x64;
    backlight.ports[0x01] = 0x64;
    backlight.ports[0x02] = 0x61;
    backlight.ports[0x03] = 0x4C;
    backlight.ports[0x20] = 0xFF; /* backlight scaler? (unimplemented) */
    backlight.brightness = 0xFF;  /* backlight level (PWM)             */

    return device;
}
