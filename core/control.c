#include <string.h>

#include "control.h"
#include "asic.h"
#include "emu.h"

/* Global CONTROL state */
control_state_t control;

/* Read from the 0x0XXX range of ports */
static uint8_t control_read(const uint16_t pio) {
    uint8_t index = pio & 0x7F;

    uint8_t value;

    switch (index) {
        case 0x01:
            value = control.cpuSpeed & 19;
            break;
        case 0x02:
            /* Set bit 1 to set battery state */
            value = control.ports[index] | 1;
            break;
        case 0x03:
            value = get_device_type();
            break;
        case 0x0B:
            /* bit 2 set if charging */
            control.ports[index] |= !(control.ports[0x0A] & 2)<<1;
            value = control.ports[index];
            break;
        case 0x0F:
            value = control.ports[index];
            if(control.USBConnected)    { value |= 0x80; }
            if(control.noPlugAInserted) { value |= 0x40; }
            break;
        case 0x28:
            value = control.ports[index] | 0x08;
            break;
        default:
            value = control.ports[index];
            break;
    }
    return value;
}

/* Write to the 0x0XXX range of ports */
static void control_write(const uint16_t pio, const uint8_t byte) {
    uint8_t index = pio & 0x7F;

    switch (index) {
        case 0x01:
            control.cpuSpeed = byte & 19;
            switch(control.cpuSpeed & 3) {
                case 0:
                    set_cpu_clock_rate(6e6);  /* 6 MHz  */
                    break;
                case 1:
                    set_cpu_clock_rate(12e6); /* 12 MHz */
                    break;
                case 2:
                    set_cpu_clock_rate(24e6); /* 24 MHz */
                    break;
                case 3:
                    set_cpu_clock_rate(48e6); /* 48 MHz */
                    break;
                default:
                    break;
            }
            gui_console_printf("CPU clock rate set to: %d MHz\n", 6*(1<<(control.cpuSpeed & 3)));
            break;
        case 0x06:
            control.ports[index] = byte & 7;
            break;
        case 0x0D:
            control.ports[index] = (byte & 0xF) << 4 | (byte & 0xF);
            break;
        case 0x0F:
            control.ports[index] = byte & 3;
            break;
        case 0x28:
            mem.flash.locked = (byte & 4) == 0;
            control.ports[index] = byte & 247;
            break;
        default:
            control.ports[index] = byte;
            break;
    }
}

static const eZ80portrange_t device = {
    .read_in    = control_read,
    .write_out  = control_write
};

eZ80portrange_t init_control(void) {
    memset(control.ports, 0, sizeof control.ports);

    return device;
}
