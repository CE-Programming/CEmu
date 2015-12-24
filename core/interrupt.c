#include "core/interrupt.h"
#include "core/emu.h"
#include <string.h>

interrupt_state_t intrpt;

static void update() {
    uint32_t status;
    status = intrpt.status ^ intrpt.inverted;
    intrpt.status = (status & ~intrpt.latched) | ((intrpt.status | status) & intrpt.latched);
}

void intrpt_set(uint32_t int_num, int on) {
    if (on) {
        intrpt.status |= 1 << int_num;
    } else {
        intrpt.status &= ~(1 << int_num);
    }
    update();
}

void intrpt_reset() {
    memset(&intrpt, 0, sizeof(intrpt));
    gui_console_printf("Interrupt controller Reset.");
}

static uint8_t intrpt_read(uint16_t pio) {
    uint16_t index = pio & 0xFF;
    uint8_t bit_offset = (pio & 3) << 3;

    static const uint32_t revision = 0x00010900;
    static const uint8_t fiq_irq_val = 0x16;

    /* 	Ports 5020-503F are identical in function to 5000-501F */
    if((index < 0x40) && (index > 0x1F)) {
        index -= 0x20;
    }

    switch(index) {
        case 0x00: case 0x01: case 0x02: case 0x03:
            return read8(intrpt.status, bit_offset);
        case 0x04: case 0x05: case 0x06: case 0x07:
            return read8(intrpt.enabled, bit_offset);
        case 0x0C: case 0x0D: case 0x0E: case 0x0F:
            return read8(intrpt.latched, bit_offset);
        case 0x10: case 0x11: case 0x12: case 0x13:
            return read8(intrpt.inverted, bit_offset);
        case 0x14: case 0x15: case 0x16: case 0x17:
            return read8(intrpt.status & intrpt.enabled, bit_offset);
        case 0x50: case 0x51: case 0x52: case 0x53:
            return read8(revision, bit_offset);
        case 0x54: case 0x55:
            return fiq_irq_val;
        default:
            break;
    }

    /* Return 0 if unimplemented */
    return 0;
}

static void intrpt_write(uint16_t pio, uint8_t byte) {
    uint16_t index = pio & 0xFF;
    uint8_t bit_offset = (pio & 3) << 3;

    /* 	Ports 5020-503F are identical in function to 5000-501F */
    if((index < 0x40) && (index > 0x1F)) {
        index -= 0x20;
    }

    switch(index) {
        case 0x04: case 0x05: case 0x06: case 0x07:
            write8(intrpt.enabled, bit_offset, byte);
            return;
        case 0x08: case 0x09: case 0x0A: case 0x0B:
            write8(intrpt.status, bit_offset, intrpt.status & ~(byte & intrpt.latched));
            return;
        case 0x0C: case 0x0D: case 0x0E: case 0x0F:
            write8(intrpt.latched, bit_offset, byte);
            return;
        case 0x10: case 0x11: case 0x12: case 0x13:
            write8(intrpt.inverted, bit_offset, byte);
            return;
    }
}

static const eZ80portrange_t device = {
    .read_in    = intrpt_read,
    .write_out  = intrpt_write
};

eZ80portrange_t init_intrpt(void) {
    memset(&intrpt, 0, sizeof(intrpt));
    intrpt.enabled = 0x00003011;  /* Default state */
    intrpt.latched = 0x00000019;  /* Default state */
    gui_console_printf("Initialized interrupt contoller...\n");
    return device;
}
