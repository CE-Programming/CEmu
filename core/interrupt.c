#include "core/interrupt.h"
#include "core/emu.h"
#include <string.h>

interrupt_state_t intrpt;

static void update() {
    /* perform some interrupt updating things here */
}

void intrpt_set(uint32_t int_num, int on) {
    if (on) {
        intrpt.int_enable_mask |= 1 << int_num;
    } else {
        intrpt.int_enable_mask &= ~(1 << int_num);
    }
    update();
}

void intrpt_reset() {
    memset(&intrpt, 0, sizeof(intrpt));
}

static uint8_t intrpt_read(const uint16_t pio) {
    uint16_t index = pio&0xFF;
    uint8_t bit_offset = (index&3)<<3;

    uint8_t byte_read;

    if(index >= 0x20 && index <= 0x3F) index-= 0x20; /* Ports 5020-503F are identical in function to 0x5000-0x501F */

    switch(index) {
        case 0x00: case 0x01: case 0x02: case 0x03:
            byte_read = read8(intrpt.raw_status ^ intrpt.int_invr,bit_offset);
            break;
        case 0x04: case 0x05: case 0x06: case 0x07:
            byte_read = read8(intrpt.int_enable_mask,bit_offset);
            break;
        case 0x0C: case 0x0D: case 0x0E: case 0x0F:
            byte_read = read8(intrpt.int_latch,bit_offset);
            break;
        case 0x10: case 0x11: case 0x12: case 0x13:
            byte_read = read8(intrpt.int_invr,bit_offset);
            break;
        case 0x14: case 0x15: case 0x16: case 0x17:    /* Masked interrupt status (used by ISR). Should be equal to (0x5000 & 0x5004) */
            byte_read = read8(intrpt.raw_status & intrpt.int_enable_mask,bit_offset);
            break;
        case 0x50: case 0x51: case 0x52: case 0x53:
            byte_read = read8(intrpt.revision,bit_offset);
            break;
        case 0x54:
            byte_read = intrpt.f_irq;
            break;
        case 0x55:
            byte_read = intrpt.f_fiq;
            break;
        default:
            byte_read = 0;
            break;
    }
    return byte_read;
}

static void intrpt_write(const uint16_t pio, const uint8_t byte) {
    uint16_t index = pio&0xFF;
    uint8_t bit_offset = (index&3)<<3;

    switch(index) {
        case 0x04: case 0x05: case 0x06: case 0x07:
            write8(intrpt.int_enable_mask,bit_offset,byte);
            break;
        case 0x08: case 0x09: case 0x0A: case 0x0B:
            intrpt.raw_status &= ~((byte << bit_offset) & intrpt.int_latch);
            break;
        case 0x0C: case 0x0D: case 0x0E: case 0x0F:
            write8(intrpt.int_latch,bit_offset,byte);
            break;
        case 0x10: case 0x11: case 0x12: case 0x13:
            write8(intrpt.int_invr,bit_offset,byte);
            break;
        default:
            break;
    }
}

static const eZ80portrange_t device = {
    .read_in    = intrpt_read,
    .write_out  = intrpt_write
};

eZ80portrange_t init_intrpt(void) {
    intrpt.int_enable_mask = 0x00003011; // Default state
    intrpt.int_latch = 0x00000019;  // Default state
    intrpt.revision = 0x00010900;   // Revision register 1.9.0.
    intrpt.f_irq = 0x16; // unused
    intrpt.f_fiq = 0x16; // unused
    gui_console_printf("Initialized interrupt contoller...\n");
    return device;
}
