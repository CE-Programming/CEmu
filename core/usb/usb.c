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
        byte = ((uint8_t *)&usb.regs)[pio];
    return byte;
}

static void usb_write(uint16_t pio, uint8_t byte, bool poke) {
    uint8_t index = pio >> 2;
    uint8_t bit_offset = (pio & 3) << 3;
    (void)poke;
    switch (index) {
        case 0x010 >> 2: // USBCMD - USB Command Register
            write8(usb.regs.hcor.data[0], bit_offset, byte &   0xFF0BFF >> bit_offset); // W mask (V or RO)
            usb.regs.hcor.data[1] = usb.regs.hcor.data[1] & ~0xD000;
            usb.regs.hcor.data[1] |= ~usb.regs.hcor.data[0] << 12 & 0x1000;
            usb.regs.hcor.data[1] |= usb.regs.hcor.data[0] << 10 & 0xC000;
            if ((uint32_t)byte << bit_offset & 2) {
                usb_reset();
            }
            break;
        case 0x014 >> 2: // USBSTS - USB Status Register
            usb.regs.hcor.data[1] &= ~((uint32_t)byte << bit_offset & 0x3F);           // WC mask (V or RO)
            break;
        case 0x018 >> 2: // USBINTR - USB Interrupt Enable Register
            write8(usb.regs.hcor.data[2], bit_offset, byte &       0x3F >> bit_offset); // W mask (V)
            break;
        case 0x01C >> 2: // FRINDEX - Frame Index Register
            write8(usb.regs.hcor.data[3], bit_offset, byte &     0x3FFF >> bit_offset); // W mask (V)
            break;
        case 0x024 >> 2: // PERIODICLISTBASE - Periodic Frame List Base Address Register
            write8(usb.regs.hcor.data[5], bit_offset, byte &     ~0xFFF >> bit_offset); // V mask (W)
            break;
        case 0x028 >> 2: // ASYNCLISTADDR - Current Asynchronous List Address Register
            write8(usb.regs.hcor.data[6], bit_offset, byte &      ~0x1F >> bit_offset); // V mask (W)
            break;
        case 0x030 >> 2: // PORTSC - Port Status and Control Register
            usb.regs.hcor.data[8] &= ~((uint32_t)byte << bit_offset & 0x2A);           // WC mask (V or RO or W)
            write8(usb.regs.hcor.data[8], bit_offset, byte &   0x1F0000 >> bit_offset); // W mask (RO)
            break;
        case 0x040 >> 2: // Miscellaneous Register
            write8(usb.regs.miscr,        bit_offset, byte &      0xFFF >> bit_offset); // W mask (V)
            break;
        case 0x044 >> 2: // unknown
            write8(usb.regs.rsvd2[0],     bit_offset, byte &     0x7FFF >> bit_offset); // W mask (V)
            break;
        case 0x048 >> 2: // unknown
            write8(usb.regs.rsvd2[1],     bit_offset, byte &      0xFFF >> bit_offset); // W mask (V)
            break;
        case 0x080 >> 2: // OTG Control Status Register
            write8(usb.regs.otgcsr,       bit_offset, byte & 0x1A00FFF7 >> bit_offset); // W mask (V)
            break;
        case 0x084 >> 2: // OTG Interrupt Status Register
            usb.regs.otgisr &= ~((uint32_t)byte << bit_offset & 0x1F71);               // WC mask (V)
            break;
        case 0x088 >> 2: // OTG Interrupt Enable Register
            write8(usb.regs.otgier,       bit_offset, byte &     0x1F71 >> bit_offset); // W mask (V)
            break;
        case 0x0C0 >> 2: // Global Interrupt Status Register
            usb.regs.isr &= ~((uint32_t)byte << bit_offset & 0xF);                     // WC mask (V)
            break;
        case 0x0C4 >> 2: // Global Interrupt Mask Register
            write8(usb.regs.imr,          bit_offset, byte &        0xF >> bit_offset); // W mask (V)
            break;
        case 0x100 >> 2: // Device Control Register
            write8(usb.regs.dev_ctrl,     bit_offset, byte &      0x2AF >> bit_offset); // W mask (V or RO)
            break;
        case 0x104 >> 2: // Device Address Register
            write8(usb.regs.dev_addr,     bit_offset, byte &       0xFF >> bit_offset); // W mask (V)
            break;
        case 0x108 >> 2: // Device Test Register
            write8(usb.regs.dev_test,     bit_offset, byte &       0x7A >> bit_offset); // W mask (V)
            break;
        case 0x110 >> 2: // SOF Mask Timer Register
            write8(usb.regs.sof_mtr,      bit_offset, byte &     0xFFFF >> bit_offset); // W mask (V)
            break;
        case 0x114 >> 2: // PHY Test Mode Selector Register
            write8(usb.regs.phy_tmsr,     bit_offset, byte &       0x1F >> bit_offset); // W mask (V)
            break;
        case 0x118 >> 2: // unknown
            write8(usb.regs.rsvd5[0],     bit_offset, byte &       0x3F >> bit_offset); // W mask (V)
            break;
        case 0x120 >> 2: // CX FIFO Register
            write8(usb.regs.cxfifo,       bit_offset, byte &        0x7 >> bit_offset); // W mask (V or RO)
            break;
        case 0x124 >> 2: // IDLE Counter Register
            write8(usb.regs.idle,         bit_offset, byte &        0x7 >> bit_offset); // W mask (V)
            break;
        case 0x130 >> 2: // Group Interrupt Mask Register
            write8(usb.regs.gimr,         bit_offset, byte &        0x7 >> bit_offset); // W mask (V)
            break;
        case 0x134 >> 2: // Group Interrupt Mask Register 0
            write8(usb.regs.gimr0,        bit_offset, byte &       0x3F >> bit_offset); // W mask (V)
            break;
        case 0x138 >> 2: // Group Interrupt Mask Register 1
            write8(usb.regs.gimr1,        bit_offset, byte &    0xF00FF >> bit_offset); // W mask (V)
            break;
        case 0x13C >> 2: // Group Interrupt Mask Register 2
            write8(usb.regs.gimr2,        bit_offset, byte &      0x7FF >> bit_offset); // W mask (V)
            break;
        case 0x140 >> 2: // Group Interrupt Status Register
            usb.regs.gisr &= ~((uint32_t)byte << bit_offset & 0x7);                    // WC mask (V)
            break;
        case 0x144 >> 2: // Group Interrupt Status Register 0
            usb.regs.gisr0 &= ~((uint32_t)byte << bit_offset & 0x3F);                  // WC mask (V)
            break;
        case 0x148 >> 2: // Group Interrupt Status Register 1
            usb.regs.gisr1 &= ~((uint32_t)byte << bit_offset & 0xF00FF);               // WC mask (V)
            break;
        case 0x14C >> 2: // Group Interrupt Status Register 2
            usb.regs.gisr2 &= ~((uint32_t)byte << bit_offset & 0x7FF & ~0x600);        // WC mask (V) [const mask]
            break;
        case 0x150 >> 2: // Receive Zero-Length-Packet Register
            write8(usb.regs.rxzlp,        bit_offset, byte &       0xFF >> bit_offset); // W mask (V)
            break;
        case 0x154 >> 2: // Transfer Zero-Length-Packet Register
            write8(usb.regs.txzlp,        bit_offset, byte &       0xFF >> bit_offset); // W mask (V)
            break;
        case 0x158 >> 2: // ISOC Error/Abort Status Register
            write8(usb.regs.isoeasr,      bit_offset, byte &   0xFF00FF >> bit_offset); // W mask (V)
            break;
        case 0x160 >> 2: case 0x164 >> 2: case 0x168 >> 2: case 0x16C >> 2:
        case 0x170 >> 2: case 0x174 >> 2: case 0x178 >> 2: case 0x17C >> 2: // IN Endpoint Register
            write8(usb.regs.iep[index&7], bit_offset, byte &     0xFFFF >> bit_offset); // W mask (V)
        case 0x180 >> 2: case 0x184 >> 2: case 0x188 >> 2: case 0x18C >> 2:
        case 0x190 >> 2: case 0x194 >> 2: case 0x198 >> 2: case 0x19C >> 2: // OUT Endpoint Register
            write8(usb.regs.oep[index&7], bit_offset, byte &     0x1FFF >> bit_offset); // W mask (V)
            break;
        case 0x1A0 >> 2: case 0x1A4 >> 2: // Endpoint Map Register
        case 0x1CC >> 2: // DMA Address Register
            ((uint8_t *)&usb.regs)[pio] = byte;                                         // W
            break;
        case 0x1A8 >> 2: // FIFO Map Register
        case 0x1AC >> 2: // FIFO Configuration Register
            ((uint8_t *)&usb.regs)[pio] = byte & 0x3F;                                  // W mask (V)
            break;
        case 0x1C0 >> 2: // DMA Target FIFO Register
            write8(usb.regs.dma_fifo,     bit_offset, byte &       0x1F >> bit_offset); // W mask (V)
            break;
        case 0x1C8 >> 2: // DMA Control Register
            write8(usb.regs.dma_ctrl,     bit_offset, byte & 0x81FFFF17 >> bit_offset); // W mask (V)
            break;
        case 0x1D0 >> 2: // EP0 Setup Packet PIO Register
            write8(usb.regs.ep0_data,     bit_offset, byte &    0x10900 >> bit_offset); // W mask (V)
            break;
    }
}

void usb_reset(void) {
    usb.regs.hcor.data[0] = 0x00080B00;
    usb.regs.hcor.data[1] = 0x00001000;
    usb.regs.hcor.data[2] = 0x00000000;
    usb.regs.hcor.data[3] = 0x00000000;
    usb.regs.hcor.data[4] = 0x00000000;
    usb.regs.hcor.data[8] = 0x00000000;
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
    usb.regs.miscr        = 0x00000181;
    usb.regs.otgcsr       = 0x00310F20;
    usb.regs.dev_ctrl     = 0x000002A4;
    usb.regs.gisr2        = 0x00000200;
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
