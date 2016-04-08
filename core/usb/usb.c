#include "usb.h"
#include "../emu.h"
#include "../schedule.h"
#include "../interrupt.h"

#include <string.h>
#include <stdio.h>

/* Global GPT state */
usb_state_t usb;

static uint8_t usb_read(uint16_t pio, bool peek) {
    uint8_t byte = 0;
    (void)peek;
    if (pio < sizeof usb.regs)
        byte = ((uint8_t *)&usb.regs)[pio & 0x1FF];
    return byte;
}

static void usb_write(uint16_t pio, uint8_t byte, bool poke) {
    uint8_t index = (pio >> 2) & 0xFF;
    uint8_t bit_offset = (index & 3) << 3;
    (void)poke;
    switch (index) {
        case 0x04: // USBCMD - USB Command Register
            write8(usb.regs.hcor.data[0], bit_offset, byte &   0xFF0BFF >> bit_offset); // W mask (V or RO)
            break;
        case 0x05: // USBSTS - USB Status Register
            usb.regs.hcor.data[1] &= ~((uint32_t)byte << bit_offset & 0x3F);           // WC mask (V or RO)
            break;
        case 0x06: // USBINTR - USB Interrupt Enable Register
            write8(usb.regs.hcor.data[2], bit_offset, byte &       0x3F >> bit_offset); // W mask (V)
            break;
        case 0x07: // FRINDEX - Frame Index Register
            write8(usb.regs.hcor.data[3], bit_offset, byte &     0x3FFF >> bit_offset); // W mask (V)
            break;
        case 0x09: // PERIODICLISTBASE - Periodic Frame List Base Address Register
            write8(usb.regs.hcor.data[5], bit_offset, byte &     ~0xFFF >> bit_offset); // V mask (W)
            break;
        case 0x0A: // ASYNCLISTADDR - Current Asynchronous List Address Register
            write8(usb.regs.hcor.data[6], bit_offset, byte &      ~0x1F >> bit_offset); // V mask (W)
            break;
        case 0x0C: // unknown
            write8(usb.regs.hcor.data[8], bit_offset, byte &   0x1F0000 >> bit_offset); // W mask (RO)
            break;
        case 0x10: // Miscellaneous Register
            write8(usb.regs.miscr,        bit_offset, byte &      0xFFF >> bit_offset); // W mask (V)
            break;
        case 0x11: // unknown
            write8(usb.regs.rsvd2[0],     bit_offset, byte &     0x7FFF >> bit_offset); // W mask (V)
            break;
        case 0x12: // unknown
            write8(usb.regs.rsvd2[1],     bit_offset, byte &      0xFFF >> bit_offset); // W mask (V)
            break;
        case 0x20: // OTG Control Status Register
            write8(usb.regs.otgcsr,       bit_offset, byte & 0x1A00FFF7 >> bit_offset); // W mask (V)
            break;
        case 0x21: // OTG Interrupt Status Register
            usb.regs.otgisr &= ~((uint32_t)byte << bit_offset & 0x1F71);               // WC mask (V)
            break;
        case 0x22: // OTG Interrupt Enable Register
            write8(usb.regs.otgier,       bit_offset, byte &     0x1F71 >> bit_offset); // W mask (V)
            break;
        case 0x30: // Global Interrupt Status Register
            usb.regs.isr &= ~((uint32_t)byte << bit_offset & 0xF);                     // WC mask (V)
            break;
        case 0x31: // Global Interrupt Mask Register
            write8(usb.regs.imr,          bit_offset, byte &        0xF >> bit_offset); // W mask (V)
            break;
        case 0x40: // Device Control Register
            write8(usb.regs.dev_ctrl,     bit_offset, byte &      0x2AF >> bit_offset); // W mask (V or RO)
            break;
        case 0x41: // Device Address Register
            write8(usb.regs.dev_addr,     bit_offset, byte &       0xFF >> bit_offset); // W mask (V)
            break;
        case 0x42: // Device Test Register
            write8(usb.regs.dev_test,     bit_offset, byte &       0x7A >> bit_offset); // W mask (V)
            break;
        case 0x44: // SOF Mask Timer Register
            write8(usb.regs.sof_mtr,      bit_offset, byte &     0xFFFF >> bit_offset); // W mask (V)
            break;
        case 0x45: // PHY Test Mode Selector Register
            write8(usb.regs.phy_tmsr,     bit_offset, byte &       0x1F >> bit_offset); // W mask (V)
            break;
        case 0x46: // unknown
            write8(usb.regs.rsvd5[0],     bit_offset, byte &       0x3F >> bit_offset); // W mask (V)
            break;
        case 0x48: // CX FIFO Register
            write8(usb.regs.cxfifo,       bit_offset, byte &        0x7 >> bit_offset); // W mask (V or RO)
            break;
        case 0x49: // IDLE Counter Register
            write8(usb.regs.idle,         bit_offset, byte &        0x7 >> bit_offset); // W mask (V)
            break;
        case 0x4C: // Group Interrupt Mask Register
            write8(usb.regs.gimr,         bit_offset, byte &        0x7 >> bit_offset); // W mask (V)
            break;
        case 0x4D: // Group Interrupt Mask Register 0
            write8(usb.regs.gimr0,        bit_offset, byte &       0x3F >> bit_offset); // W mask (V)
            break;
        case 0x4E: // Group Interrupt Mask Register 1
            write8(usb.regs.gimr1,        bit_offset, byte &    0xF00FF >> bit_offset); // W mask (V)
            break;
        case 0x4F: // Group Interrupt Mask Register 2
            write8(usb.regs.gimr2,        bit_offset, byte &      0x7FF >> bit_offset); // W mask (V)
            break;
        case 0x50: // Group Interrupt Status Register
            usb.regs.gisr &= ~((uint32_t)byte << bit_offset & 0x7);                    // WC mask (V)
            break;
        case 0x51: // Group Interrupt Status Register 0
            usb.regs.gisr0 &= ~((uint32_t)byte << bit_offset & 0x3F);                  // WC mask (V)
            break;
        case 0x52: // Group Interrupt Status Register 1
            usb.regs.gisr1 &= ~((uint32_t)byte << bit_offset & 0xF00FF);               // WC mask (V)
            break;
        case 0x53: // Group Interrupt Status Register 2
            usb.regs.gisr2 &= ~((uint32_t)byte << bit_offset & 0x7FF);                 // WC mask (V)
            break;
        case 0x54: // Receive Zero-Length-Packet Register
            write8(usb.regs.rxzlp,        bit_offset, byte &       0xFF >> bit_offset); // W mask (V)
            break;
        case 0x55: // Transfer Zero-Length-Packet Register
            write8(usb.regs.txzlp,        bit_offset, byte &       0xFF >> bit_offset); // W mask (V)
            break;
        case 0x56: // ISOC Error/Abort Status Register
            write8(usb.regs.isoeasr,      bit_offset, byte &   0xFF00FF >> bit_offset); // W mask (V)
            break;
        case 0x58: case 0x59: case 0x5A: case 0x5B:
        case 0x5C: case 0x5D: case 0x5E: case 0x5F: // IN Endpoint Register
            write8(usb.regs.iep[index&7], bit_offset, byte &     0xFFFF >> bit_offset); // W mask (V)
        case 0x60: case 0x61: case 0x62: case 0x63:
        case 0x64: case 0x65: case 0x66: case 0x67: // OUT Endpoint Register
            write8(usb.regs.oep[index&7], bit_offset, byte &     0x1FFF >> bit_offset); // W mask (V)
            break;
        case 0x68: // Endpoint Map Register (EP1 ~ 4)
        case 0x69: // Endpoint Map Register (EP5 ~ 8)
        case 0x6F: // DMA Address Register
            ((uint8_t *)&usb.regs)[pio & 0x1FF] = byte;                                 // W
            break;
        case 0x6A: // FIFO Map Register
        case 0x6B: // FIFO Configuration Register
            ((uint8_t *)&usb.regs)[pio & 0x1FF] = byte & 0x3F;                          // W
            break;
        case 0x6C: // DMA Target FIFO Register
            write8(usb.regs.dma_fifo,     bit_offset, byte &       0x1F >> bit_offset); // W mask (V)
            break;
        case 0x6E: // DMA Control Register
            write8(usb.regs.dma_ctrl,     bit_offset, byte & 0x81FFFF17 >> bit_offset); // W mask (V)
            break;
    }
}

void usb_reset(void) {
    usb.regs.hcor.data[0] = 0x00080B00;
    usb.regs.hcor.data[1] = 0x00001000;
    usb.regs.hcor.data[2] = 0x00000000;
    usb.regs.hcor.data[3] = 0x00000000;
    usb.regs.hcor.data[4] = 0x00000000;
    usb.regs.hcor.data[8] = 0x00000800;
}

static const eZ80portrange_t device = {
    .read  = usb_read,
    .write = usb_write
};

eZ80portrange_t init_usb(void) {
    memset(&usb, 0, sizeof usb);
    usb.regs.hccr.data[0] = sizeof usb.regs.hccr |
        0x0100 << 16; // v1.0
    usb.regs.hccr.data[1] = 1; // 1 port
    usb.regs.hccr.data[2] =
        1 << 2 | // async sched park (supported)
        1 << 1 | // prog frame list (supported)
        0 << 0;  // interface (32-bit)
    usb_reset();
    gui_console_printf("[CEmu] Initialized USB...\n");
    return device;
}

bool usb_save(FILE *image) {
    return fwrite(&usb, sizeof(usb), 1, image) == 1;
}

bool usb_restore(FILE *image) {
    return fread(&usb, sizeof(usb), 1, image) == 1;
}
