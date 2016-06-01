#include <string.h>

#include "control.h"
#include "asic.h"
#include "emu.h"
#include "debug/debug.h"

/* Global CONTROL state */
control_state_t control;

/* Read from the 0x0XXX range of ports */
static uint8_t control_read(const uint16_t pio) {
    uint8_t index = (uint8_t)pio;

    uint8_t value;

    switch (index) {
        case 0x01:
            value = control.cpuSpeed;
            break;
        case 0x02:
            /* Set bit 1 to set battery state */
            value = control.readBatteryStatus;
            break;
        case 0x03:
            value = get_device_type();
            break;
        case 0x0B:
            /* bit 2 set if charging */
            value = control.ports[index] | (control.batteryCharging == true)<<1;
            break;
        case 0x0F:
            value = control.ports[index];
            if(control.USBBusPowered)    { value |= 0x80; }
            if(control.USBSelfPowered) { value |= 0x40; }
            break;
        case 0x1D:
        case 0x1E:
        case 0x1F:
            value = read8(control.privileged, (index - 0x1D) << 3);
            break;
        case 0x28:
            value = control.ports[index] | 0x08;
            break;
        case 0x3A:
        case 0x3B:
        case 0x3C:
            value = read8(control.stackLimit, (index - 0x3A) << 3);
            break;
        default:
            value = control.ports[index];
            break;
    }
    return value;
}

/* Write to the 0x0XXX range of ports */
static void control_write(const uint16_t pio, const uint8_t byte) {
    uint8_t index = (uint8_t)pio;

    switch (index) {
        case 0x00:
            control.ports[index] = byte;
            if(byte & 0x10) {
                cpuEvents |= EVENT_RESET;
            }
            switch (control.readBatteryStatus) {
                case 3: /* Battery Level is 0 */
                    control.readBatteryStatus = (control.setBatteryStatus == BATTERY_0) ? 0 : (byte == 0x83) ? 5 : 0;
                    break;
                case 5: /* Battery Level is 1 */
                    control.readBatteryStatus = (control.setBatteryStatus == BATTERY_1) ? 0 : (byte == 0x03) ? 7 : 0;
                    break;
                case 7: /* Battery Level is 2 */
                    control.readBatteryStatus = (control.setBatteryStatus == BATTERY_2) ? 0 : (byte == 0x83) ? 9 : 0;
                    break;
                case 9: /* Battery Level is 3 (Or 4) */
                    control.readBatteryStatus = (control.setBatteryStatus == BATTERY_3) ? 0 : (byte == 0x03) ? 11 : 0;
                    break;
            }
            break;
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
            gui_console_printf("[CEmu] CPU clock rate set to: %d MHz\n", 6*(1<<(control.cpuSpeed & 3)));
#ifdef DEBUG_SUPPORT
            if (cpuEvents & EVENT_DEBUG_STEP) {
                cpuEvents &= ~EVENT_DEBUG_STEP;
                open_debugger(DBG_STEP, 0);
            }
#endif
            break;
        case 0x06:
            control.ports[index] = byte & 7;
            break;
        case 0x07:
            control.readBatteryStatus = (byte & 0x90) ? 1 : 0;
            break;
        case 0x09:
            switch (control.readBatteryStatus) {
                case 1: /* Battery is bad */
                    control.readBatteryStatus = (control.setBatteryStatus == BATTERY_DISCHARGED) ? 0 : (byte & 0x80) ? 0 : 3;
                    break;
            }
            control.ports[index] = byte;

            /* Appears to enter low-power mode (For now; this will be fine) */
            if (byte == 0xD4) {
                asic.shipModeEnabled = true;
                control.ports[0] |= 0x40; // Turn calc off
                cpuEvents |= EVENT_RESET;
            }
            break;
        case 0x0A:
            control.readBatteryStatus += (control.readBatteryStatus == 3) ? 1 : 0;
            control.ports[index] = byte;
            break;
        case 0x0B:
        case 0x0C:
            control.readBatteryStatus = 0;
            break;
        case 0x0D:
            control.ports[index] = (byte & 0xF) << 4 | (byte & 0xF);
            break;
        case 0x0F:
            control.ports[index] = byte & 3;
            break;
        case 0x1D:
        case 0x1E:
        case 0x1F:
            write8(control.privileged, (index - 0x1D) << 3, byte);
            break;
        case 0x28:
            if (cpu.registers.PC < control.privileged) {
                mem.flash.locked = (byte & 4) == 0;
            }
            control.ports[index] = byte & 247;
            break;
        case 0x3A:
        case 0x3B:
        case 0x3C:
            write8(control.stackLimit, (index - 0x3A) << 3, byte);
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
    memset(&control, 0, sizeof control);
    gui_console_printf("[CEmu] Initialized Control Ports...\n");

    /* Set default state to full battery and not charging */
    control.batteryCharging = false;
    control.setBatteryStatus = BATTERY_4;
    control.privileged = 0;

    return device;
}

bool control_save(emu_image *s) {
    s->control = control;
    return true;
}

bool control_restore(const emu_image *s) {
    control = s->control;
    return true;
}
