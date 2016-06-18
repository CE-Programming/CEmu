#include <string.h>

#include "interrupt.h"
#include "emu.h"
#include "cpu.h"

interrupt_state_t intrpt;

static void update() {
    uint32_t status;
    size_t request;
    for (request = 0; request < sizeof(intrpt.request) / sizeof(*intrpt.request); request++) {
        status = intrpt.status ^ intrpt.request[request].inverted;
        intrpt.request[request].status = (status & ~intrpt.request[request].latched) |
            ((intrpt.request[request].status | status) & intrpt.request[request].latched);
    }
}

void intrpt_pulse(uint32_t int_num) {
    intrpt_set(int_num, true);
    intrpt_set(int_num, false);
}

void intrpt_set(uint32_t int_num, bool set) {
    if (set) {
        intrpt.status |= 1 << int_num;
    } else {
        intrpt.status &= ~(1 << int_num);
    }
    update();
}

void intrpt_reset() {
    memset(&intrpt, 0, sizeof(intrpt));
}

static uint8_t intrpt_read(uint16_t pio) {
    uint16_t index = pio >> 2 & 0x3F;
    uint8_t request = pio >> 5 & 1;
    uint8_t bit_offset = (pio & 3) << 3;

    uint8_t value = 0;

    static const uint32_t revision = 0x00010900;

    switch(index) {
        case 0:
        case 8:
            value = read8(intrpt.request[request].status, bit_offset);
            break;
        case 1:
        case 9:
            value = read8(intrpt.request[request].enabled, bit_offset);
            break;
        case 3:
        case 11:
            value = read8(intrpt.request[request].latched, bit_offset);
            break;
        case 4:
        case 12:
            value = read8(intrpt.request[request].inverted, bit_offset);
            break;
        case 5:
        case 13:
            value = read8(intrpt.request[request].status & intrpt.request[request].enabled, bit_offset);
            break;
        case 20:
            value = read8(revision, bit_offset);
            break;
        case 21:
            value = (bit_offset & 16) ? 0 : 22;
            break;
        default:
            break;
    }
    return value;
}

static void intrpt_write(uint16_t pio, uint8_t value) {
    uint16_t index = pio >> 2 & 0x3F;
    uint8_t request = pio >> 5 & 1;
    uint8_t bit_offset = (pio & 3) << 3;

    switch(index) {
        case 1:
        case 9:
            write8(intrpt.request[request].enabled, bit_offset, value);
            break;
        case 2:
        case 10:
            intrpt.request[request].status &= ~(((uint32_t)value << bit_offset) & intrpt.request[request].latched);
            break;
        case 3:
        case 11:
            write8(intrpt.request[request].latched, bit_offset, value);
            break;
        case 4:
        case 12:
            write8(intrpt.request[request].inverted, bit_offset, value);
            break;
    }
}

static const eZ80portrange_t device = {
    .read_in    = intrpt_read,
    .write_out  = intrpt_write
};

eZ80portrange_t init_intrpt(void) {
    gui_console_printf("[CEmu] Initialized Interrupt Controller...\n");
    return device;
}

bool intrpt_save(emu_image *s) {
    s->intrpt = intrpt;
    return true;
}

bool intrpt_restore(const emu_image *s) {
    intrpt = s->intrpt;
    return true;
}
