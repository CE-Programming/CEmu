#include <string.h>

#include "control.h"
#include "asic.h"
#include "lcd.h"
#include "emu.h"
#include "debug/debug.h"

/* Global CONTROL state */
control_state_t control;

/* Read from the 0x0XXX range of ports */
static uint8_t control_read(const uint16_t pio, bool peek) {
    uint8_t index = (uint8_t)pio;

    uint8_t value;
    (void)peek;

    switch (index) {
        case 0x02:
            value = control.readBatteryStatus;
            break;
        case 0x03:
            value = get_device_type();
            break;
        case 0x06:
            value = control.mmioUnlocked;
            break;
        case 0x08:
            value = 0x7F;
            break;
        case 0x0B:
            /* bit 1 set if charging */
            value = control.ports[index] | (control.batteryCharging == true) << 1;
            break;
        case 0x0F:
            value = control.ports[index];
            if (control.usbBusPowered)    { value |= 0x80; }
            if (control.usbSelfPowered) { value |= 0x40; }
            break;
        case 0x1C:
            value = 0x80;
            break;
        case 0x1D: case 0x1E: case 0x1F:
            value = read8(control.privileged, (index - 0x1D) << 3);
            break;
        case 0x20: case 0x21: case 0x22:
            value = read8(control.protectedStart, (index - 0x20) << 3);
            break;
        case 0x23: case 0x24: case 0x25:
            value = read8(control.protectedEnd, (index - 0x23) << 3);
            break;
        case 0x28:
            value = control.flashUnlocked;
            break;
        case 0x3A: case 0x3B: case 0x3C:
            value = read8(control.stackLimit, (index - 0x3A) << 3);
            break;
        case 0x3D:
            value = control.protectionStatus;
            break;
        default:
            value = control.ports[index];
            break;
    }
    return value;
}

/* Write to the 0x0XXX range of ports */
static void control_write(const uint16_t pio, const uint8_t byte, bool poke) {
    unsigned int i;
    uint8_t index = (uint8_t)pio;
    (void)poke;

    switch (index) {
        case 0x00:
            control.ports[index] = byte & 0x93;
            if (byte & (1 << 4)) {
                cpu_crash("[CEmu] Reset caused by writing to bit 4 of port 0.\n");
            }
            /* Setting this bit turns off the calc (not implemented yet) */
            if (byte & (1 << 6)) {
                control.off = true;
            }
            switch (control.readBatteryStatus) {
                case 3: /* Battery Level is 0 */
                    control.readBatteryStatus = (control.setBatteryStatus == BATTERY_0) ? 0 : (byte & 0x80) ? 5 : 0;
                    break;
                case 5: /* Battery Level is 1 */
                    control.readBatteryStatus = (control.setBatteryStatus == BATTERY_1) ? 0 : (byte & 0x80) ? 0 : 7;
                    break;
                case 7: /* Battery Level is 2 */
                    control.readBatteryStatus = (control.setBatteryStatus == BATTERY_2) ? 0 : (byte & 0x80) ? 9 : 0;
                    break;
                case 9: /* Battery Level is 3 (Or 4) */
                    control.readBatteryStatus = (control.setBatteryStatus == BATTERY_3) ? 0 : (byte & 0x80) ? 0 : 11;
                    break;
            }
            break;
        case 0x01:
            control.ports[index] = byte & 19;
            control.cpuSpeed = byte & 3;
            switch(control.cpuSpeed) {
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
            break;
        case 0x05:
            if (control.ports[index] & (1 << 6) && !(byte & (1 << 6))) {
                cpu_crash("[CEmu] Reset caused by resetting bit 6 of port 0x0005.\n");
            }
            control.ports[index] = byte & 7;
            break;
        case 0x06:
            control.mmioUnlocked = byte & 7;
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

            /* Appears to enter low-power mode (this will be fine for now) */
            if (byte == 0xD4) {
                control.ports[0] |= 1 << 6;
                asic.resetOnWake = true;
                lcd.control &= ~0x800;
                gui_console_printf("[CEmu] Reset caused by entering sleep mode.\n", cpu.registers.PC);
#ifdef DEBUG_SUPPORT
                if (debugger.resetOpensDebugger) {
                    open_debugger(DBG_MISC_RESET, cpu.registers.PC);
                }
#endif
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
            if (!(byte & (1 << 1))) {
                cpu_crash("[CEmu] Reset caused by resetting bit 1 of port 0x000D.\n");
            }
            /* This bit disables vram and makes it garbage */
            if (!(byte & (1 << 3))) {
                lcd_disable();
                for (i = LCD_RAM_OFFSET; i < LCD_RAM_OFFSET + LCD_BYTE_SIZE; i++) {
                    mem.ram.block[i] = rand();
                }
            } else {
                lcd_enable();
            }
            control.ports[index] = (byte & 0xF) << 4 | (byte & 0xF);
            break;
        case 0x0F:
            control.ports[index] = byte & 3;
            break;
        case 0x1D: case 0x1E: case 0x1F:
            write8(control.privileged, (index - 0x1D) << 3, byte);
            break;
        case 0x20: case 0x21: case 0x22:
            write8(control.protectedStart, (index - 0x20) << 3, byte);
            break;
        case 0x23: case 0x24: case 0x25:
            write8(control.protectedEnd, (index - 0x23) << 3, byte);
            break;
        case 0x28:
            control.flashUnlocked = (control.flashUnlocked | 5) & byte;
            break;
        case 0x29:
            control.ports[index] = byte & 1;
            break;
        case 0x3A: case 0x3B: case 0x3C:
            write8(control.stackLimit, (index - 0x3A) << 3, byte);
            break;
        case 0x3E:
            control.protectionStatus &= ~byte;
            break;
        default:
            control.ports[index] = byte;
            break;
    }
}

static const eZ80portrange_t device = {
    .read  = control_read,
    .write = control_write
};

eZ80portrange_t init_control(void) {
    memset(&control, 0, sizeof control);
    gui_console_printf("[CEmu] Initialized Control Ports...\n");

    /* Set default state to full battery and not charging */
    control.batteryCharging = false;
    control.setBatteryStatus = BATTERY_4;
    control.privileged = 0xFFFFFF;
    control.protectedStart = control.protectedEnd = 0xD1887C;
    control.protectionStatus = 0;
    control.off = false;

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

bool mmio_unlocked(void) {
    return (control.mmioUnlocked & 1 << 2) == 1 << 2;
}

bool flash_unlocked(void) {
    return (control.flashUnlocked & 3 << 2) == 3 << 2;
}

bool unprivileged_code(void) {
    return cpu.registers.rawPC > control.privileged &&
        (cpu.registers.rawPC < control.protectedStart ||
         cpu.registers.rawPC > control.protectedEnd);
}
