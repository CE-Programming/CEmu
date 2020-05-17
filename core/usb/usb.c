#include "usb.h"
#include "../cpu.h"
#include "../emu.h"
#include "../mem.h"
#include "../schedule.h"
#include "../interrupt.h"
#include "../debug/debug.h"

#include <errno.h>
#include <string.h>
#include <stdio.h>

#define CONTROL_MPS 0x40

void debugInstruction(void);

/* Global GPT state */
usb_state_t usb;

static void usb_host_reset(void);

static void usb_update(void) {
    intrpt_set(INT_USB, (usb.regs.isr =
                         ( (usb.regs.gisr = (usb.regs.gisr2 & ~usb.regs.gimr2 ? GISR_GRP2 : 0) |
                                            (usb.regs.gisr1 & ~usb.regs.gimr1 ? GISR_GRP1 : 0) |
                                            (usb.regs.gisr0 & ~usb.regs.gimr0 ? GISR_GRP0 : 0) )
                                               & ~usb.regs.gimr &&
                           usb.regs.dev_ctrl   & DEVCTRL_GIRQ_EN       ? ISR_DEV  : 0) |
                         (usb.regs.otgisr      & usb.regs.otgier       ? ISR_OTG  : 0) |
                         (usb.regs.hcor.usbsts & usb.regs.hcor.usbintr ? ISR_HOST : 0) ) & ~usb.regs.imr);
}
void usb_host_int(uint8_t which) {
    usb.regs.hcor.usbsts |= which & 0x3F;
    usb_update();
}
void usb_otg_int(uint16_t which) {
    usb.regs.otgisr |= which & OTGISR_MASK;
    usb_update();
}
void usb_grp0_int(uint8_t which) {
    usb.regs.gisr0 |= which & GIMR0_MASK;
    usb_update();
}
void usb_grp1_int(uint32_t which) {
    usb.regs.gisr1 |= which & GIMR1_MASK;
    usb_update();
}
void usb_grp2_int(uint16_t which) {
    usb.regs.gisr2 |= which & GIMR2_MASK;
    usb_update();
}

// Plug A:
//  plug:
//   OTGCSR_DEV_B -> OTGCSR_DEV_A
//   OTGCSR_ROLE_D -> OTGCSR_ROLE_H
//  unplug:
//   OTGCSR_DEV_A -> OTGCSR_DEV_B
//   OTGCSR_ROLE_H -> OTGCSR_ROLE_D

// Connected Plug B:
//  plug:
//   0 -> 1 OTGCSR_A_VBUS_VLD
//   0 -> 1 OTGCSR_A_SESS_VLD
//   0 -> 1 OTGCSR_B_SESS_VLD
//   1 -> 0 OTGCSR_B_SESS_END
//   0 -> 1 GISR2_RESUME
//  unplug:
//   1 -> 0 OTGCSR_A_VBUS_VLD
//   1 -> 0 GISR2_RESUME
//   .
//   1 -> 0 OTGCSR_A_SESS_VLD
//   1 -> 0 OTGCSR_B_SESS_VLD
//   ...
//   0 -> 1 OTGCSR_B_SESS_END
static void usb_plug(void) {
    usb.regs.otgcsr |= OTGCSR_A_VBUS_VLD | OTGCSR_A_SESS_VLD | OTGCSR_B_SESS_VLD;
    usb.regs.otgcsr &= ~OTGCSR_B_SESS_END;
    usb_grp2_int(GISR2_RESUME);
}

static int usb_dispatch_event(void) {
    while (true) {
        int error = usb.device(&usb.event);
        if (error) {
            return error;
        }
        switch (usb.event.type) {
            case USB_INIT_EVENT:
                // do nothing
                break;
            case USB_RESET_EVENT:
                usb_grp2_int(GISR2_RESET);
                continue;
            case USB_TRANSFER_EVENT: {
                usb_transfer_info_t *transfer = &usb.event.info.transfer;
                uint8_t endpoint = transfer->endpoint;
                if (!endpoint) {
                    if (transfer->setup) {
                        usb.regs.cxfifo &= ~(CXFIFO_CXFIN | CXFIFO_TSTPKTFIN | CXFIFO_CXSTALL | CXFIFO_CXFIFOCLR);
                        usb.regs.cxfifo |= CXFIFO_CXFIFOE;
                        memcpy(usb.ep0_data, transfer->buffer, sizeof usb.ep0_data);
                        usb.ep0_idx = 0;
                        usb_grp0_int(GISR0_CXSETUP);
                    } else if (transfer->length < CONTROL_MPS) {
                        CXFIFO_SET_BYTES(usb.regs.cxfifo, transfer->length);
                        usb_grp0_int(GISR0_CXEND);
                    } else {
                        CXFIFO_SET_BYTES(usb.regs.cxfifo, CONTROL_MPS);
                        usb.regs.cxfifo |= CXFIFO_CXFIFOF;
                    }
                } else if (--endpoint < 8) {
                    if (transfer->length) {
                        uint8_t fifo = EPMAP_GET_OUT(usb.regs.epmap[endpoint]);
                        transfer->max_pkt_size =
                            EP_MAXPS((transfer->direction ? usb.regs.iep : usb.regs.oep)[endpoint]);
                        usb.regs.cxfifo &= ~CXFIFO_FIFOE(fifo);
                        usb.regs.gisr1 &= ~GISR1_IN_FIFO(fifo);
                        if (transfer->length >= transfer->max_pkt_size) {
                            usb.regs.fifocsr[fifo] = (usb.regs.fifocsr[fifo] & FIFOCSR_RESET) |
                                transfer->max_pkt_size;
                            usb_grp1_int(GISR1_OUT_FIFO(fifo));
                        } else {
                            usb.regs.fifocsr[fifo] = (usb.regs.fifocsr[fifo] & FIFOCSR_RESET) |
                                transfer->length;
                            usb_grp1_int(GISR1_RX_FIFO(fifo));
                        }
                    } else {
                        usb.regs.rxzlp |= 1 << endpoint;
                        usb_grp2_int(GISR2_ZLPRX);
                        continue;
                    }
                }
                break;
            }
            case USB_TIMER_EVENT: {
                usb_timer_info_t *timer = &usb.event.info.timer;
                switch (timer->mode) {
                    case USB_TIMER_NONE:
                        sched_clear(SCHED_USB_DEVICE);
                        break;
                    case USB_TIMER_RELATIVE_MODE:
                        sched_repeat(SCHED_USB_DEVICE, timer->useconds);
                        break;
                    case USB_TIMER_ABSOLUTE_MODE:
                        sched_set(SCHED_USB_DEVICE, timer->useconds);
                        break;
                }
                break;
            }
            case USB_DESTROY_EVENT:
                usb.device(&usb.event);
                usb.device = usb_disconnected_device;
                break;
        }
        break;
    }
    return 0;
}

int usb_init_device(int argc, const char *const *argv) {
    usb.event.type = USB_DESTROY_EVENT;
    usb.device(&usb.event);
    usb.event.type = USB_INIT_EVENT;
    usb_init_info_t *init = &usb.event.info.init;
    init->argc = argc;
    init->argv = argv;
    if (argc < 1) {
        usb.device = usb_disconnected_device;
    } else if (!strcasecmp(argv[0], "dusb")) {
        usb.device = usb_dusb_device;
    } else {
        return ENOENT;
    }
    int error = usb_dispatch_event();
    if (!error) {
        usb_plug();
    }
    return error;
}

static void usb_event(enum sched_item_id event) {
    (void)event;
}

static void usb_device_event(enum sched_item_id event) {
    (void)event;
    usb.event.type = USB_TIMER_EVENT;
    usb_dispatch_event();
}

uint8_t usb_status(void) {
    return (usb.regs.otgcsr & (OTGCSR_A_VBUS_VLD | OTGCSR_A_SESS_VLD | OTGCSR_B_SESS_VLD) ? 0x80 : 0) |
        (usb.regs.otgcsr & (OTGCSR_DEV_B | OTGCSR_ROLE_D) ? 0x40 : 0);
}

static uint8_t usb_ep0_idx_update(void) {
    if (usb.regs.dma_fifo & DMAFIFO_CX) {
        usb.regs.gisr0 &= ~GISR0_CXSETUP;
        return usb.ep0_idx++;
    }
    return usb.ep0_idx;
}

static uint8_t usb_read(uint16_t pio, bool peek) {
    uint8_t value = 0;
    if (pio < sizeof usb.regs) {
        value = ((uint8_t *)&usb.regs)[pio];
    } else if (pio < (peek ? 0x1d8 : 0x1d4)) {
        value = usb.ep0_data[peek ? (usb.ep0_idx & 4) ^ (pio & 7) : (usb_ep0_idx_update() & 4) | (pio & 3)];
    }
    return value;
}

static void usb_write(uint16_t pio, uint8_t value, bool poke) {
    uint8_t index = pio >> 2;
    uint8_t bit_offset = (pio & 3) << 3;
    (void)poke;
    switch (index) {
        case 0x010 >> 2: // USBCMD - USB Command Register
            write8(usb.regs.hcor.usbcmd, bit_offset, value &   0xFF0BFF >> bit_offset); // W mask (V or RO)
            usb.regs.hcor.usbsts &= ~0xD000;
            usb.regs.hcor.usbsts |= ~usb.regs.hcor.usbcmd << 12 & 0x1000;
            usb.regs.hcor.usbsts |= usb.regs.hcor.usbcmd << 10 & 0xC000;
            if ((uint32_t)value << bit_offset & 2) {
                usb_host_reset();
            }
            break;
        case 0x014 >> 2: // USBSTS - USB Status Register
            usb.regs.hcor.usbsts &= ~((uint32_t)value << bit_offset & 0x3F);                      // WC mask (V or RO)
            break;
        case 0x018 >> 2: // USBINTR - USB Interrupt Enable Register
            write8(usb.regs.hcor.usbintr,          bit_offset, value &        0x3F >> bit_offset); // W mask (V)
            break;
        case 0x01C >> 2: // FRINDEX - Frame Index Register
            write8(usb.regs.hcor.frindex,          bit_offset, value &      0x3FFF >> bit_offset); // W mask (V)
            break;
        case 0x024 >> 2: // PERIODICLISTBASE - Periodic Frame List Base Address Register
            write8(usb.regs.hcor.periodiclistbase, bit_offset, value &      ~0xFFF >> bit_offset); // V mask (W)
            break;
        case 0x028 >> 2: // ASYNCLISTADDR - Current Asynchronous List Address Register
            write8(usb.regs.hcor.asynclistaddr,    bit_offset, value &       ~0x1F >> bit_offset); // V mask (W)
            break;
        case 0x030 >> 2: // PORTSC - Port Status and Control Register
            usb.regs.hcor.portsc[0] &= ~((uint32_t)value << bit_offset & 0x2A);                  // WC mask (V or RO or W)
            write8(usb.regs.hcor.portsc[0],        bit_offset, value &    0x1F0000 >> bit_offset); // W mask (RO)
            break;
        case 0x040 >> 2: // Miscellaneous Register
            write8(usb.regs.miscr,                 bit_offset, value &       0xFFF >> bit_offset); // W mask (V)
            break;
        case 0x044 >> 2: // unknown
            write8(usb.regs.rsvd2[0],              bit_offset, value &      0x7FFF >> bit_offset); // W mask (V)
            break;
        case 0x048 >> 2: // unknown
            write8(usb.regs.rsvd2[1],              bit_offset, value &       0xFFF >> bit_offset); // W mask (V)
            break;
        case 0x080 >> 2: // OTG Control Status Register
            write8(usb.regs.otgcsr,                bit_offset, value &  0x1A00FFF7 >> bit_offset); // W mask (V)
            break;
        case 0x084 >> 2: // OTG Interrupt Status Register
            usb.regs.otgisr &= ~((uint32_t)value << bit_offset & OTGISR_MASK);                    // WC mask (V)
            break;
        case 0x088 >> 2: // OTG Interrupt Enable Register
            write8(usb.regs.otgier,                bit_offset, value & OTGISR_MASK >> bit_offset); // W mask (V)
            break;
        case 0x0C0 >> 2: // Global Interrupt Status Register
            usb.regs.isr &= ~((uint32_t)value << bit_offset & ISR_MASK);                          // WC mask (V)
            break;
        case 0x0C4 >> 2: // Global Interrupt Mask Register
            write8(usb.regs.imr,                   bit_offset, value &   IMR_MASK >> bit_offset); // W mask (V)
            break;
        case 0x100 >> 2: // Device Control Register
            write8(usb.regs.dev_ctrl,              bit_offset, value &       0x2AF >> bit_offset); // W mask (V or RO)
            break;
        case 0x104 >> 2: // Device Address Register
            write8(usb.regs.dev_addr,              bit_offset, value &        0xFF >> bit_offset); // W mask (V)
            break;
        case 0x108 >> 2: // Device Test Register
            write8(usb.regs.dev_test,              bit_offset, value &        0x7A >> bit_offset); // W mask (V)
            break;
        case 0x110 >> 2: // SOF Mask Timer Register
            write8(usb.regs.sof_mtr,               bit_offset, value &      0xFFFF >> bit_offset); // W mask (V)
            break;
        case 0x114 >> 2: // PHY Test Mode Selector Register
            write8(usb.regs.phy_tmsr,              bit_offset, value &        0x1F >> bit_offset); // W mask (V)
            break;
        case 0x118 >> 2: // unknown
            write8(usb.regs.rsvd5[0],              bit_offset, value &        0x3F >> bit_offset); // W mask (V)
            break;
        case 0x120 >> 2: // CX FIFO Register
            if (value & CXFIFO_CXFIN >> bit_offset) {
                usb.regs.gisr0 &= ~GISR0_CXEND;
                // Status Stage
                usb.event.type = USB_TRANSFER_EVENT;
                usb_transfer_info_t *transfer = &usb.event.info.transfer;
                transfer->buffer = NULL;
                transfer->length = 0;
                transfer->max_pkt_size = CONTROL_MPS;
                transfer->endpoint = 0;
                transfer->setup = false;
                transfer->direction = !(usb.ep0_data[0] >> 7);
                usb_dispatch_event();
            }
            break;
        case 0x124 >> 2: // IDLE Counter Register
            write8(usb.regs.idle,                  bit_offset, value &         0x7 >> bit_offset); // W mask (V)
            break;
        case 0x130 >> 2: // Group Interrupt Mask Register
            write8(usb.regs.gimr,                  bit_offset, value &   GIMR_MASK >> bit_offset); // W mask (V)
            break;
        case 0x134 >> 2: // Group Interrupt Mask Register 0
            write8(usb.regs.gimr0,                 bit_offset, value &  GIMR0_MASK >> bit_offset); // W mask (V)
            break;
        case 0x138 >> 2: // Group Interrupt Mask Register 1
            write8(usb.regs.gimr1,                 bit_offset, value &  GIMR1_MASK >> bit_offset); // W mask (V)
            break;
        case 0x13C >> 2: // Group Interrupt Mask Register 2
            write8(usb.regs.gimr2,                 bit_offset, value &  GIMR2_MASK >> bit_offset); // W mask (V)
            break;
        case 0x140 >> 2: // Group Interrupt Status Register
            usb.regs.gisr &= ~((uint32_t)value << bit_offset & GIMR_MASK);                       // WC mask (V)
            break;
        case 0x144 >> 2: // Group Interrupt Status Register 0
            usb.regs.gisr0 &= ~((uint32_t)value << bit_offset & GIMR0_MASK);                     // WC mask (V)
            break;
        case 0x148 >> 2: // Group Interrupt Status Register 1
            usb.regs.gisr1 &= ~((uint32_t)value << bit_offset & GIMR1_MASK);                     // WC mask (V)
            break;
        case 0x14C >> 2: // Group Interrupt Status Register 2
            usb.regs.gisr2 &= ~((uint32_t)value << bit_offset & GIMR2_MASK);                     // WC mask (V) [const mask]
            break;
        case 0x150 >> 2: // Receive Zero-Length-Packet Register
            write8(usb.regs.rxzlp,                 bit_offset, value &       0xFF >> bit_offset); // W mask (V)
            break;
        case 0x154 >> 2: // Transfer Zero-Length-Packet Register
            write8(usb.regs.txzlp,                 bit_offset, value &       0xFF >> bit_offset); // W mask (V)
            break;
        case 0x158 >> 2: // ISOC Error/Abort Status Register
            write8(usb.regs.isoeasr,               bit_offset, value &   0xFF00FF >> bit_offset); // W mask (V)
            break;
        case 0x160 >> 2: case 0x164 >> 2: case 0x168 >> 2: case 0x16C >> 2:
        case 0x170 >> 2: case 0x174 >> 2: case 0x178 >> 2: case 0x17C >> 2: // IN Endpoint Register
            write8(usb.regs.iep[index & 7],        bit_offset, value &     0xFFFF >> bit_offset); // W mask (V)
        case 0x180 >> 2: case 0x184 >> 2: case 0x188 >> 2: case 0x18C >> 2:
        case 0x190 >> 2: case 0x194 >> 2: case 0x198 >> 2: case 0x19C >> 2: // OUT Endpoint Register
            write8(usb.regs.oep[index & 7],        bit_offset, value &     0x1FFF >> bit_offset); // W mask (V)
            break;
        case 0x1A0 >> 2: case 0x1A4 >> 2: // Endpoint Map Register
        case 0x1CC >> 2: // DMA Address Register
            ((uint8_t *)&usb.regs)[pio] = value;                                                  // W
            break;
        case 0x1A8 >> 2: // FIFO Map Register
        case 0x1AC >> 2: // FIFO Configuration Register
            ((uint8_t *)&usb.regs)[pio] = value & 0x3F;                                           // W mask (V)
            break;
        case 0x1C0 >> 2: // DMA Target FIFO Register
            write8(usb.regs.dma_fifo,              bit_offset, value &       0x1F >> bit_offset); // W mask (V)
            break;
        case 0x1C8 >> 2: // DMA Control Register
            write8(usb.regs.dma_ctrl,              bit_offset, value & 0x81FFFF16 >> bit_offset); // W mask (V)
            if (value & DMACTRL_START >> bit_offset) {
                usb.event.type = USB_TRANSFER_EVENT;
                usb_transfer_info_t *transfer = &usb.event.info.transfer;
                transfer->length = DMACTRL_LEN(usb.regs.dma_ctrl);
                transfer->buffer = phys_mem_ptr(usb.regs.dma_addr, transfer->length);
                if (!transfer->buffer) {
                    usb_grp2_int(GISR2_DMAERR);
                    break;
                }
                transfer->direction = usb.regs.dma_ctrl & DMACTRL_MEM2FIFO;
                if (usb.regs.dma_fifo & DMAFIFO_CX) {
                    transfer->max_pkt_size = CONTROL_MPS;
                    transfer->endpoint = 0;
                    CXFIFO_SET_BYTES(usb.regs.cxfifo, 0);
                    usb.regs.cxfifo |= CXFIFO_CXFIFOE;
                    usb.regs.cxfifo &= ~CXFIFO_CXFIFOF;
                } else {
                    uint8_t *epmap = usb.regs.epmap;
                    for (transfer->endpoint = 1; transfer->endpoint <= 8;
                         ++transfer->endpoint, ++epmap) {
                        uint8_t fifo = *epmap >> (!transfer->direction << 2) & 3;
                        if (usb.regs.dma_fifo & 1 << fifo) {
                            transfer->max_pkt_size =
                                EP_MAXPS((transfer->direction ? usb.regs.iep
                                                              : usb.regs.oep)[transfer->endpoint]);
                            usb.regs.cxfifo |= CXFIFO_FIFOE(fifo);
                            usb.regs.gisr1 &= ~GISR1_RX_FIFO(fifo);
                            usb_grp1_int(GISR1_IN_FIFO(fifo));
                            usb.regs.fifocsr[fifo] &= FIFOCSR_RESET;
                            break;
                        }
                    }
                    if (transfer->endpoint > 8) {
                        usb_grp2_int(GISR2_DMAERR);
                        break;
                    }
                }
                transfer->setup = false;
                //usb_transfer_info_t debug_transfer = *transfer;
                usb_dispatch_event();
                //gui_console_printf("[USB] %c", debug_transfer.direction ? 'R' : 'S');
                //while (debug_transfer.length--) gui_console_printf(" %02X", *debug_transfer.buffer++);
                //gui_console_printf("\n");
                usb_grp2_int(GISR2_DMAFIN);
            }
            break;
        case 0x1D0 >> 2: // EP0 Data Register
            if (!poke) {
                usb_ep0_idx_update();
            }
            break;
    }
    usb_update();
}

static void usb_host_reset(void) {
#define fill(array, value) memset(array, value, sizeof array)
#define clear(array) fill(array, 0)
    usb.regs.hcor.usbcmd                = 0x00080B00;
    usb.regs.hcor.usbsts                = 0x00001000;
    usb.regs.hcor.usbintr               = 0;
    usb.regs.hcor.frindex               = 0;
    usb.regs.hcor.ctrldssegment         = 0;
    clear(usb.regs.hcor.rsvd0);
    clear(usb.regs.hcor.portsc);
    clear(usb.regs.rsvd1);
}

void usb_reset(void) {
    int i;
    usb_host_reset();
    usb.regs.miscr                      = 0x00000181;
    clear(usb.regs.rsvd2);
    usb.regs.otgcsr                     = 0x00310E20;
    usb.regs.otgisr                     = 0;
    clear(usb.regs.rsvd3);
    usb.regs.isr                        = 0;
    clear(usb.regs.rsvd4);
    usb.regs.dev_ctrl                   = 0x00000020;
    usb.regs.dev_addr                   = 0;
    usb.regs.dev_test                   = 0;
    usb.regs.sof_fnr                    = 0;
    usb.regs.sof_mtr                    = 0;
    usb.regs.rsvd5[1]                   = 0;
    usb.regs.cxfifo                     = 0x00000F20;
    usb.regs.idle                       = 0;
    clear(usb.regs.rsvd6);
    usb.regs.gisr                       = 0;
    usb.regs.gisr0                      = 0;
    usb.regs.gisr1                      = 0x50000;
    usb.regs.gisr2                      = 0;
    usb.regs.rxzlp                      = 0;
    usb.regs.txzlp                      = 0;
    usb.regs.isoeasr                    = 0;
    clear(usb.regs.rsvd7);
    for (i = 0; i != 8; ++i)
    usb.regs.iep[i] = usb.regs.oep[i]   = 0x00000200;
    fill(usb.regs.epmap, 0xFF);
    fill(usb.regs.fifomap, 0x0F);
    usb.regs.fifocfg                    = 0;
    clear(usb.regs.fifocsr);
    usb.regs.dma_fifo                   = 0;
    clear(usb.regs.rsvd8);
    usb.regs.dma_ctrl                   = 0;
    clear(usb.ep0_data);
    usb.ep0_idx                         = 0;
    usb_grp2_int(GISR2_IDLE);         // because idle == 0 ms
    if (usb.device == usb_disconnected_device) {
        usb_otg_int(OTGISR_BSESSEND); // because otgcsr & OTGCSR_B_SESS_END
    } else {
        usb_plug();
    }
#undef clear
#undef fill
    sched.items[SCHED_USB].callback.event = usb_event;
    sched.items[SCHED_USB].clock = CLOCK_12M;
    sched.items[SCHED_USB_DEVICE].callback.event = usb_device_event;
    sched.items[SCHED_USB_DEVICE].clock = CLOCK_1M;
}

static void usb_init_hccr(void) {
    usb.regs.hccr.data[0] = sizeof usb.regs.hccr | 0x0100 << 16; // v1.0
    usb.regs.hccr.data[1] = 1; // 1 port
    usb.regs.hccr.data[2] = 1 << 2 | // async sched park (supported)
                            1 << 1 | // prog frame list (supported)
                            0 << 0 ; // interface (32-bit)
    usb.regs.hccr.data[3] = 0; // No Port Routing Rules
    usb.device = usb_disconnected_device;
}

static const eZ80portrange_t device = {
    .read  = usb_read,
    .write = usb_write
};

eZ80portrange_t init_usb(void) {
    memset(&usb, 0, sizeof usb);
    usb_init_hccr();
    usb_reset();
    gui_console_printf("[CEmu] Initialized USB...\n");
    return device;
}

bool usb_save(FILE *image) {
    return fwrite(&usb, sizeof(usb), 1, image) == 1;
}

bool usb_restore(FILE *image) {
    usb_device_t *device = usb.device;
    void *context = usb.event.context;
    bool success = fread(&usb, sizeof(usb), 1, image) == 1;
    usb.device = device;
    usb.event.context = context;
    usb_init_hccr(); // hccr is read only
    // these bits are raz
    usb.regs.hcor.periodiclistbase &= 0xFFFFF000;
    usb.regs.hcor.asynclistaddr    &= 0xFFFFFFE0;
    usb.regs.otgier                &= OTGISR_MASK;
    usb.regs.imr                   &= IMR_MASK;
    usb.regs.phy_tmsr              &= 0x1F;
    usb.regs.rsvd5[0]              &= 0x3F;
    usb.regs.gimr                  &= GIMR_MASK;
    usb.regs.gimr0                 &= GIMR0_MASK;
    usb.regs.gimr1                 &= GIMR1_MASK;
    usb.regs.gimr2                 &= GIMR2_MASK;
    return success;
}
