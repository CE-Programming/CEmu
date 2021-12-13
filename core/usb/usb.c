#include "usb.h"
#include "../cpu.h"
#include "../emu.h"
#include "../mem.h"
#include "../schedule.h"
#include "../interrupt.h"
#include "../debug/debug.h"

#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CONTROL_MPS 0x40

/* Global GPT state */
usb_state_t usb;

typedef enum usb_qtype {
    QTYPE_ITD, QTYPE_QH, QTYPE_SITD, QTYPE_FSTN
} usb_qtype_t;
typedef union usb_qlink {
    uint32_t val;
    bool term : 1;
    struct {
        int : 1;
        usb_qtype_t type : 2;
    };
    struct {
        uint32_t : 1;
        uint32_t nak_cnt : 4; // overlay alt
        uint32_t ptr : 27;
    };
} usb_qlink_t;
typedef union usb_qbuf {
    uint32_t val;
    struct { uint32_t off : 12, ptr : 20; };             // qtd/sitd index 0
    struct { uint8_t dev_addr : 7, : 1, endpt : 4; };    //      itd index 0
    uint8_t c_prog_mask;                                 //      qtd index 1
    struct { uint8_t xact_count : 3, xact_pos : 2; };    //     sitd index 1
    struct { uint16_t max_pkt_size : 11; bool io : 1; }; //      itd index 1
    struct { uint32_t frame_tag : 5, s_bytes : 7; };     //      qtd index 2
    uint8_t mult : 2;                                    //      itd index 2
} usb_qbuf_t;
typedef struct usb_qtd {
    usb_qlink_t next, alt;
    bool ping : 1, split : 1, missed : 1, xact_err : 1, babble : 1, buf_err : 1, halted : 1, active : 1;
    uint8_t pid : 2, cerr : 2, page : 3, ioc : 1;
    uint16_t length : 15, dt : 1;
    usb_qbuf_t bufs[5];
} usb_qtd_t;
typedef struct usb_qh {
    usb_qlink_t horiz;
    uint16_t dev_addr : 7, i : 1, endpt : 4, eps : 2, dtc : 1, h : 1, max_pkt_size : 11, c : 1, nak_rl : 4;
    uint8_t s_mask, c_mask;
    uint16_t hub_addr : 7, port_num : 7, mult : 2;
    usb_qlink_t cur;
    usb_qtd_t overlay;
} usb_qh_t;
typedef struct usb_sitd {
    usb_qlink_t next;
    uint8_t dev_addr : 7, : 1, endpt : 4, : 4, hub_addr : 7, : 1, port_num : 7;
    bool io : 1;
    uint8_t s_mask, c_mask;
    uint16_t : 16;
    bool : 1, split : 1, missed : 1, xact_err : 1, babble : 1, buf_err : 1, err : 1, active : 1;
    uint8_t c_prog_mask;
    uint16_t length : 10, : 4, page : 1, ioc : 1;
    usb_qbuf_t bufs[2];
    usb_qlink_t back;
} usb_sitd_t;
typedef struct usb_itd {
    usb_qlink_t next;
    struct {
        uint16_t off : 12, page : 3, ioc : 1, length : 12, xact_err : 1, babble : 1, buf_err : 1, active : 1;
    } xacts[8];
    usb_qbuf_t bufs[7];
} usb_itd_t;
typedef struct usb_fstn {
    usb_qlink_t normal, back;
} usb_fstn_t;

static_assert(sizeof(usb_qbuf_t) == 0x04, "usb_qbuf_t does not have a size of 0x04");
static_assert(offsetof(usb_qtd_t, next) == 0x00, "Next qTD Pointer is not at offset 0x00 of usb_qtd_t");
static_assert(offsetof(usb_qtd_t, alt) == 0x04, "Alternate Next qTD Pointer is not at offset 0x04 of usb_qtd_t");
static_assert(offsetof(usb_qtd_t, bufs) == 0x0C, "Buffer Pointers are not at offset 0x0C of usb_qtd_t");
static_assert(sizeof(usb_qtd_t) == 0x20, "usb_qtd_t does not have a size of 0x20");
static_assert(offsetof(usb_qh_t, horiz) == 0x00, "Queue Head Horizontal Link Pointer is not at offset 0x00 of usb_qh_t");
static_assert(offsetof(usb_qh_t, cur) == 0x0C, "Current qTD Pointer is not at offset 0x0C of usb_qh_t");
static_assert(offsetof(usb_qh_t, s_mask) == 0x08, "µFrame S-mask is not at offset 0x08 of usb_qh_t");
static_assert(offsetof(usb_qh_t, c_mask) == 0x09, "µFrame C-mask is not at offset 0x09 of usb_qh_t");
static_assert(offsetof(usb_qh_t, overlay) == 0x10, "Transfer Overlay is not at offset 0x10 of usb_qh_t");
static_assert(sizeof(usb_qh_t) == 0x30, "usb_qh_t does not have a size of 0x30");
static_assert(offsetof(usb_sitd_t, next) == 0x00, "Next Link Pointer is not at offset 0x00 of usb_sitd_t");
static_assert(offsetof(usb_sitd_t, s_mask) == 0x08, "µFrame S-mask is not at offset 0x08 of usb_sitd_t");
static_assert(offsetof(usb_sitd_t, c_mask) == 0x09, "µFrame S-mask is not at offset 0x09 of usb_sitd_t");
static_assert(offsetof(usb_sitd_t, c_prog_mask) == 0x0D, "µFrame C-prog-mask is not at offset 0x0D of usb_sitd_t");
static_assert(offsetof(usb_sitd_t, bufs) == 0x10, "Buffer Pointers are not at offset 0x10 of usb_sitd_t");
static_assert(offsetof(usb_sitd_t, back) == 0x18, "Back Pointer is not at offset 0x18 of usb_sitd_t");
static_assert(sizeof(usb_sitd_t) == 0x1C, "usb_sitd_t does not have a size of 0x1C");
static_assert(offsetof(usb_itd_t, next) == 0x00, "Next Link Pointer is not at offset 0x00 of usb_itd_t");
static_assert(offsetof(usb_itd_t, xacts) == 0x04, "Transaction Infos are not at offset 0x04 of usb_itd_t");
static_assert(offsetof(usb_itd_t, bufs) == 0x24, "Buffer Pointers are not at offset 0x24 of usb_itd_t");
static_assert(sizeof(usb_itd_t) == 0x40, "usb_itd_t does not have a size of 0x40");
static_assert(offsetof(usb_fstn_t, normal) == 0x00, "Normal Path Link Pointer is not at offset 0x00 of usb_fstn_t");
static_assert(offsetof(usb_fstn_t, back) == 0x04, "Back Path Link Pointer is not at offset 0x04 of usb_fstn_t");
static_assert(sizeof(usb_fstn_t) == 0x08, "usb_fstn_t does not have a size of 0x08");

typedef struct usb_traversal_state {
    union {
        usb_qh_t qh;
        usb_fstn_t fstn;
    };
    bool dirty : 1;
    enum {
        USB_NAK_CNT_WAIT_FOR_LIST_HEAD,
        USB_NAK_CNT_DO_RELOAD,
        USB_NAK_CNT_WAIT_FOR_START_EVENT,
    } nak_cnt_reload_state : 2;
    bool fake_recl : 1;
    int32_t fake_recl_head : 14;
    uint32_t fake_iter_cap : 14;
    uint32_t bit_times_remaining;
} usb_traversal_state_t;

static void usb_set_bits(uint32_t *reg, uint32_t bits, bool val) {
    *reg = val ? *reg | bits : *reg & ~bits;
}

static bool usb_update_status_change(uint32_t *reg, uint32_t status, uint32_t change, bool val) {
    uint32_t old = *reg, new = old;
    bool changed;
    usb_set_bits(&new, status, val);
    if ((changed = (old ^ new) & status)) {
        new |= change;
    }
    *reg = new;
    return changed;
}

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
static void usb_plug_b(void) {
    usb.regs.otgcsr |= OTGCSR_A_VBUS_VLD | OTGCSR_A_SESS_VLD | OTGCSR_B_SESS_VLD;
    usb.regs.otgcsr &= ~OTGCSR_B_SESS_END;
    usb.regs.sof_fnr = 0;
    usb_grp2_int(GISR2_RESUME);
}
static void usb_unplug_b(void) {
    usb.regs.otgcsr &= ~(OTGCSR_A_VBUS_VLD | OTGCSR_A_SESS_VLD | OTGCSR_B_SESS_VLD);
    usb.regs.otgcsr |= OTGCSR_B_SESS_END;
    usb_otg_int(OTGISR_BSESSEND);
}

// Plug A:
//  plug:
//   OTGCSR_DEV_B -> OTGCSR_DEV_A
//   OTGCSR_ROLE_D -> OTGCSR_ROLE_H
//  unplug:
//   OTGCSR_DEV_A -> OTGCSR_DEV_B
//   OTGCSR_ROLE_H -> OTGCSR_ROLE_D
static void usb_plug_a(void) {
    usb_unplug_b();
    usb.regs.otgcsr &= ~(OTGCSR_DEV_B | OTGCSR_ROLE_D);
    usb.regs.sof_fnr = 0;
    usb_otg_int(OTGISR_IDCHG | OTGISR_RLCHG);
}
static void usb_unplug_a(void) {
    usb.regs.hcor.portsc[0] &= ~PORTSC_J_STATE;
    if (usb_update_status_change(usb.regs.hcor.portsc, PORTSC_CONN_STATUS, PORTSC_CONN_CHANGE, false) ||
        usb_update_status_change(usb.regs.hcor.portsc, PORTSC_EN_STATUS, PORTSC_EN_CHANGE, false)) {
        usb_host_int(USBSTS_PORT_CHANGE);
    }
    usb.regs.otgcsr |= OTGCSR_DEV_B | OTGCSR_ROLE_D;
    usb_otg_int(OTGISR_APRM | OTGISR_IDCHG | OTGISR_RLCHG);
    usb_plug_b();
}

static void usb_plug(void) {
    if (usb.event.host) {
        usb_plug_b();
    } else {
        usb_plug_a();
    }
}
static void usb_unplug(void) {
    if (usb.event.host) {
        usb_unplug_b();
    } else {
        usb_unplug_a();
    }
}

static void usb_plug_complete(void) {
    if (usb.device != usb_disconnected_device) {
        usb_plug();
    } else {
        usb_unplug();
    }
}

static uint32_t usb_compute_packet_bit_times(uint32_t byte_count) {
    return 76 + 88 + 67 + 88 + 67 + byte_count * 8 * 7 / 6 + 721 + 48;
}

static void usb_write_back_qtd(usb_qh_t *qh) {
    usb_qtd_t qtd;
    mem_dma_read(&qtd, qh->cur.ptr << 5, sizeof(qtd));
    qtd.ping = qh->overlay.ping;
    qtd.split = qh->overlay.split;
    qtd.missed = qh->overlay.missed;
    qtd.xact_err = qh->overlay.xact_err;
    qtd.babble = qh->overlay.babble;
    qtd.buf_err = qh->overlay.buf_err;
    qtd.halted = qh->overlay.halted;
    qtd.active = qh->overlay.active;
    qtd.cerr = qh->overlay.cerr;
    qtd.length = qh->overlay.length;
    mem_dma_write(&qtd, qh->cur.ptr << 5, sizeof(qtd));
    // TODO: defer these interrupts?
    if (qh->overlay.halted) {
        usb_host_int(USBSTS_USBERRINT);
    }
    if (qh->overlay.ioc) {
        usb_host_int(USBSTS_USBINT);
    }
}
static void usb_qh_completed(usb_qh_t *qh) {
    qh->overlay.active = false;
    usb_write_back_qtd(qh);
}
static void usb_qh_halted(usb_qh_t *qh) {
    qh->overlay.halted = true;
    usb_qh_completed(qh);
}

static bool usb_qtd_gather(uint8_t *dst, usb_qtd_t *qtd, uint32_t *len) {
    uint16_t block_len;
    if (qtd->length > 0x5000) {
        return true;
    }
    if (qtd->pid & 1) {
        *len -= qtd->length;
        return false;
    }
    while (qtd->length && *len) {
        if (qtd->page >= 5) {
            return true;
        }
        block_len = (1 << 12) - qtd->bufs[0].off;
        if (block_len > qtd->length) {
            block_len = qtd->length;
        }
        if (block_len > *len) {
            block_len = *len;
        }
        mem_dma_read(dst, qtd->bufs[qtd->page].ptr << 12 | qtd->bufs[0].off, block_len);
        dst += block_len;
        qtd->bufs[0].off += block_len;
        qtd->page += !qtd->bufs[0].off;
        qtd->length -= block_len;
        *len -= block_len;
    }
    return qtd->length;
}
static bool usb_qtd_scatter(usb_qtd_t *qtd, const uint8_t *src, uint32_t *len) {
    uint16_t block_len;
    if (qtd->length > 0x5000) {
        return true;
    }
    if (!(qtd->pid & 1)) {
        return false;
    }
    while (qtd->length && *len) {
        if (qtd->page >= 5) {
            return true;
        }
        block_len = (1 << 12) - qtd->bufs[0].off;
        if (block_len > qtd->length) {
            block_len = qtd->length;
        }
        if (block_len > *len) {
            block_len = *len;
        }
        mem_dma_write(src, qtd->bufs[qtd->page].ptr << 12 | qtd->bufs[0].off, block_len);
        src += block_len;
        qtd->bufs[0].off += block_len;
        qtd->page += !qtd->bufs[0].off;
        qtd->length -= block_len;
        *len -= block_len;
    }
    return *len;
}

static int usb_dispatch_event(usb_traversal_state_t *state) {
    int error = 0;
    usb_transfer_info_t *transfer = &usb.event.info.transfer;
    usb_timer_info_t *timer = &usb.event.info.timer;
    do {
        error = usb.device(&usb.event);
        if (error) {
            usb.event.type = USB_DESTROY_EVENT;
        }
        switch (usb.event.type) {
            case USB_NO_EVENT:
                return error;
            case USB_INIT_EVENT:
                break;
            case USB_POWER_EVENT:
                // TODO: pull impl out of plug?
                break;
            case USB_RESET_EVENT:
                usb_grp2_int(GISR2_RESET);
                break;
            case USB_TRANSFER_REQUEST_EVENT:
                if (usb.event.host) {
                    uint8_t endpoint = transfer->endpoint;
                    if (!endpoint) {
                        if (transfer->type == USB_SETUP_TRANSFER) {
                            usb.regs.cxfifo &= ~(CXFIFO_CXFIN | CXFIFO_TSTPKTFIN | CXFIFO_CXSTALL | CXFIFO_CXFIFOCLR);
                            usb.regs.cxfifo |= CXFIFO_CXFIFOE;
                            memcpy(usb.ep0_data, transfer->buffer, sizeof(usb.ep0_data));
                            usb.ep0_idx = 0;
                            usb_grp0_int(GISR0_CXSETUP);
                        } else if (transfer->length < CONTROL_MPS) {
                            usb.regs.cxfifo = CXFIFO_SET_BYTES(usb.regs.cxfifo, transfer->length);
                            usb_grp0_int(GISR0_CXEND);
                        } else {
                            usb.regs.cxfifo = CXFIFO_SET_BYTES(usb.regs.cxfifo, CONTROL_MPS);
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
                            //gui_console_printf("usb_grp2_int(%s);\n", "GISR2_ZLPRX");
                            usb.event.type = USB_TRANSFER_REQUEST_EVENT;
                            transfer->status = USB_TRANSFER_COMPLETED;
                            continue;
                        }
                    }
                }
                break;
            case USB_TRANSFER_RESPONSE_EVENT:
                if (usb.event.host) {
                    if (!transfer->direction) {
                        mem_dma_write(transfer->buffer, usb.regs.dma_addr, DMACTRL_LEN(usb.regs.dma_ctrl));
                    }
                    if (transfer->length) {
                        //gui_console_printf("usb_grp2_int(%s);\n", transfer->status == USB_TRANSFER_COMPLETED ? "GISR2_DMAFIN" : "GISR2_DMAERR");
                        usb_grp2_int(transfer->status == USB_TRANSFER_COMPLETED ? GISR2_DMAFIN : GISR2_DMAERR);
                    }
                } else if (state) {
                    state->fake_recl = true;
                    state->fake_recl_head = -1;
                    usb.regs.hcor.usbsts |= USBSTS_RECLAMATION;
                    state->qh.overlay.alt.nak_cnt = state->qh.nak_rl;
                    switch (transfer->type) {
                        case USB_SETUP_TRANSFER:
                        case USB_CONTROL_TRANSFER:
                        case USB_BULK_TRANSFER:
                        case USB_INTERRUPT_TRANSFER:
                            if (transfer->status != USB_TRANSFER_COMPLETED) {
                                gui_console_printf("[USB] Error: Transfer failed: %d\n", transfer->status);
                            }
                            uint32_t length = transfer->length;
                            if (transfer->direction) {
                                state->bit_times_remaining -= usb_compute_packet_bit_times(length);
                            }
                            switch (transfer->status) {
                                case USB_TRANSFER_COMPLETED:
                                    if (usb_qtd_scatter(&state->qh.overlay, transfer->buffer, &length)) {
                                        state->qh.overlay.missed = true;
                                        usb_qh_halted(&state->qh);
                                    } else {
                                        usb_qh_completed(&state->qh);
                                    }
                                    break;
                                case USB_TRANSFER_STALLED:
                                    usb_qh_halted(&state->qh);
                                    break;
                                case USB_TRANSFER_OVERFLOWED:
                                    state->qh.overlay.buf_err = true;
                                    usb_qh_halted(&state->qh);
                                    break;
                                case USB_TRANSFER_BABBLED:
                                    state->qh.overlay.babble = true;
                                    usb_qh_halted(&state->qh);
                                    break;
                                case USB_TRANSFER_ERRORED:
                                    state->qh.overlay.cerr = 0;
                                    state->qh.overlay.xact_err = true;
                                    usb_qh_halted(&state->qh);
                                    break;
                                default:
                                    gui_console_printf("[USB] Error: Transfer response with unexpected status %d!\n", transfer->status);
                                    usb_qh_halted(&state->qh);
                                    break;
                            }
                            break;
                        default:
                            gui_console_printf("[USB] Error: Transfer response with unexpected type %d!\n", transfer->type);
                            usb_qh_halted(&state->qh);
                            break;
                    }
                }
                break;
            case USB_TIMER_EVENT:
                //gui_console_printf("[USB] Warning: Waiting for %d us.\n", timer->useconds);
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
            case USB_DESTROY_EVENT:
                usb.device(&usb.event);
                usb.device = usb_disconnected_device;
                usb_plug_complete();
                break;
        }
        usb.event.type = USB_NO_EVENT;
    } while (!error);
    return error;
}

static void usb_host_sys_err(void) {
    usb.regs.hcor.usbcmd &= ~USBCMD_RUN;
    usb.regs.hcor.usbsts |= USBSTS_HCHALTED;
    usb_host_int(USBSTS_HOST_SYS_ERR);
    gui_console_printf("[USB] Warning: Fatal host controller error!\n");
}

static void usb_qh_execute(usb_traversal_state_t *state) {
    if (state->qh.i) {
        state->qh.overlay.active = false;
    }
    if (!state->qh.overlay.active || state->qh.overlay.halted) {
        return;
    }
    if (state->qh.s_mask) {
        // Interrupt Transfer Pre-condition Criteria
        if (!(state->qh.s_mask & 1 << (usb.regs.hcor.frindex & 7))) {
            return;
        }
    } else {
        // Asynchronous Transfer Pre-operations
        if (state->nak_cnt_reload_state == USB_NAK_CNT_DO_RELOAD) {
            state->qh.overlay.alt.nak_cnt = state->qh.nak_rl;
            state->dirty = true;
        }
    }
    // Transfer Type Independent Pre-operations
    uint32_t max_bit_times = usb_compute_packet_bit_times(state->qh.max_pkt_size);
    for (uint8_t qHTransactionCounter = state->qh.mult;
         qHTransactionCounter && state->qh.overlay.active && state->bit_times_remaining >= max_bit_times;
         --qHTransactionCounter) {
        // Asynchronous Transfer Pre-condition Criteria
        if (state->qh.nak_rl) {
            if (!state->qh.s_mask && !state->qh.overlay.alt.nak_cnt) {
                return;
            }
            state->qh.overlay.alt.nak_cnt--;
        }
        state->dirty = true;
        uint32_t length = sizeof(usb.buffer);
        if (state->qh.overlay.pid > 1 + state->qh.c ||
            usb_qtd_gather(usb.buffer, &state->qh.overlay, &length)) {
            state->qh.overlay.cerr = 0;
            state->qh.overlay.xact_err = true;
            usb_qh_halted(&state->qh);
            return;
        }
        usb.event.type = USB_TRANSFER_REQUEST_EVENT;
        usb.event.info.transfer.buffer = usb.buffer;
        usb.event.info.transfer.length = sizeof(usb.buffer) - length;
        usb.event.info.transfer.max_pkt_size = state->qh.max_pkt_size;
        usb.event.info.transfer.status = USB_TRANSFER_COMPLETED;
        usb.event.info.transfer.address = state->qh.dev_addr;
        usb.event.info.transfer.endpoint = state->qh.endpt;
        usb.event.info.transfer.type =
            state->qh.c ? state->qh.overlay.pid == 2 ? USB_SETUP_TRANSFER : USB_CONTROL_TRANSFER
                        : state->qh.s_mask ? USB_INTERRUPT_TRANSFER : USB_BULK_TRANSFER;
        usb.event.info.transfer.direction = state->qh.overlay.pid & 1;
        if (!usb.event.info.transfer.direction) {
            state->bit_times_remaining -= usb_compute_packet_bit_times(usb.event.info.transfer.length);
        }
        usb_dispatch_event(state);
    }
}

static void usb_qh_advance(usb_traversal_state_t *state) {
    if (state->qh.i || state->qh.overlay.active || state->qh.overlay.halted) {
        return;
    }
    usb_qlink_t link;
    link.term = true;
    if (state->qh.overlay.length) {
        link = state->qh.overlay.alt;
    }
    if (link.term) {
        link = state->qh.overlay.next;
    }
    if (link.term) {
        return;
    }
    usb_qtd_t qtd;
    mem_dma_read(&qtd, link.ptr << 5, sizeof(qtd));
    if (!qtd.active) {
        return;
    }
    bool dt = state->qh.dtc ? qtd.dt : state->qh.overlay.dt;
    bool ping = state->qh.eps != USB_HIGH_SPEED ? qtd.ping : state->qh.overlay.ping;
    state->qh.cur = link;
    state->qh.overlay = qtd;
    state->qh.overlay.alt.nak_cnt = state->qh.nak_rl;
    state->qh.overlay.ping = ping;
    state->qh.overlay.dt = dt;
    state->qh.overlay.bufs[1].c_prog_mask = 0;
    state->qh.overlay.bufs[2].frame_tag = 0;
    state->dirty = true;
}

static void usb_schedule_traverse(usb_qlink_t *link) {
    // start event
    usb_traversal_state_t state;
    state.nak_cnt_reload_state = USB_NAK_CNT_WAIT_FOR_LIST_HEAD;
    state.fake_recl = true;
    state.fake_recl_head = -1;
    state.fake_iter_cap = 1 << 10;
    usb.regs.hcor.usbsts |= USBSTS_RECLAMATION;
    switch (usb.event.speed) {
        case USB_FULL_SPEED:
            state.bit_times_remaining = UINT64_C(480) * 1000u * 1000u / 8000u;
            break;
        case USB_LOW_SPEED:
            state.bit_times_remaining = UINT64_C(1500) * 1000u / 1000u;
            break;
        case USB_HIGH_SPEED:
            state.bit_times_remaining = UINT64_C(12) * 1000u * 1000u / 1000u;
            break;
        case USB_SUPER_SPEED:
            state.bit_times_remaining = UINT64_C(5) * 1000u * 1000u * 1000u / 8000u;
            break;
    }
    while (!link->term) {
        if (!--state.fake_iter_cap) {
            static bool warned = false;
            if (!warned) {
                gui_console_printf("[USB] Warning: Very long asynchronous list!\n");
                warned = true;
            }
            return;
        }
        switch (link->type) {
            case QTYPE_ITD:
                return usb_host_sys_err();
            case QTYPE_QH:
                if (state.fake_recl_head == (link->ptr & 0x3FFF)) {
                    static bool warned = false;
                    if (!warned) {
                        gui_console_printf("[USB] Warning: No reclamation head!\n");
                        warned = true;
                    }
                    if (!state.fake_recl) {
                        return;
                    }
                    state.fake_recl = false;
                }
                if (state.fake_recl_head == -1) {
                    state.fake_recl_head = link->ptr & 0x3FFF;
                }
                // 4.10.1 Fetch Queue Head
                mem_dma_read(&state.qh, link->ptr << 5, sizeof(state.qh));
                state.dirty = false;
                if (!state.qh.s_mask && state.qh.h) {
                    state.fake_recl = true;
                    state.fake_recl_head = -1;
                    // update nak counter reload state
                    if (state.nak_cnt_reload_state != USB_NAK_CNT_WAIT_FOR_START_EVENT) {
                        ++state.nak_cnt_reload_state;
                    }
                    // empty schedule detection
                    if (!(usb.regs.hcor.usbsts & USBSTS_RECLAMATION)) {
                        return;
                    }
                    // update reclamation
                    usb.regs.hcor.usbsts &= ~USBSTS_RECLAMATION;
                }
                // 4.10.2 Advance Queue
                usb_qh_advance(&state);
                // 4.10.3 Execute Transaction
                usb_qh_execute(&state);
                // 4.10.15 Follow Queue Head Horizontal Pointer
                if (state.dirty) {
                    mem_dma_write(&state.qh, link->ptr << 5, sizeof(state.qh));
                }
                *link = state.qh.horiz;
                break;
            case QTYPE_SITD:
                return usb_host_sys_err();
            case QTYPE_FSTN:
                mem_dma_read(&state.fstn, link->ptr << 5, sizeof(state.fstn));
                *link = state.fstn.normal;
                break;
        }
    }
}

static void usb_reset_core(void) {
#define fill(array, value) memset(array, value, sizeof(array))
#define clear(array) fill(array, 0)
    usb.regs.hcor.usbcmd                = 0x00080B00;
    usb.regs.hcor.usbsts                = 0x00001000;
    usb.regs.hcor.usbintr               = 0;
    usb.regs.hcor.frindex               = 0;
    usb.regs.hcor.ctrldssegment         = 0;
}

static void usb_reset_aux(void) {
    clear(usb.regs.hcor.rsvd0);
    clear(usb.regs.hcor.portsc);
    clear(usb.regs.rsvd1);
    usb.regs.miscr                      = 0x00000181;
    clear(usb.regs.rsvd2);
}

static void usb_reset_otg(void) {
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
    for (int i = 0; i != 8; ++i)
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
    usb_grp2_int(GISR2_IDLE); // because idle == 0 ms
#undef clear
#undef fill
}

int usb_plug_device(int argc, const char *const *argv,
                    usb_progress_handler_t *progress_handler, void *progress_context) {
    //gui_console_printf("usb");
    //for (int i = 0; i < argc; ++i) {
    //    gui_console_printf(" \"%s\"", argv[i]);
    //}
    //gui_console_printf("\n");
    if (!usb.device) {
        usb.device = usb_disconnected_device;
    }
    usb.event.type = USB_DESTROY_EVENT;
    usb.device(&usb.event);
    usb_plug_complete();
    usb.event.progress_handler = progress_handler;
    usb.event.progress_context = progress_context;
    usb.event.context = NULL;
    usb.event.speed = USB_FULL_SPEED;
    usb.event.type = USB_INIT_EVENT;
    usb.event.info.init.argc = argc;
    usb.event.info.init.argv = argv;
    if (argc < 1) {
        usb.device = usb_disconnected_device;
    } else if (!strcasecmp(argv[0], "dusb")) {
        usb.device = usb_dusb_device;
    } else if (!strcasecmp(argv[0], "physical")) {
        usb.device = usb_physical_device;
    } else if (!strcasecmp(argv[0], "msd")) {
        usb.device = usb_msd_device;
    } else {
        return ENOEXEC;
    }
    int error = usb_dispatch_event(NULL);
    usb_plug_complete();
    return error;
}

static void usb_event(enum sched_item_id event) {
    bool high_speed = false;
    if (usb.regs.otgcsr & OTGCSR_A_VBUS_VLD) {
        if (usb.regs.otgcsr & OTGCSR_ROLE_D) {
            high_speed = usb.regs.dev_ctrl & DEVCTRL_HS;
            if (high_speed) {
                usb.regs.sof_fnr += 1 << 11;
                usb.regs.sof_fnr &= (1 << 14) - 1;
            }
            if (!SOFFNR_UFN(usb.regs.sof_fnr)) {
                usb.regs.sof_fnr = SOFFNR_FNR(usb.regs.sof_fnr + 1);
            }
        } else {
            high_speed = usb.regs.otgcsr & OTGCSR_SPD_HIGH;
            if (usb.regs.hcor.usbcmd & USBCMD_RUN) {
                usb_qlink_t link;
                usb.regs.hcor.frindex += high_speed ? 1 : 8;
                usb.regs.hcor.frindex &= (1 << 14) - 1;
                uint32_t frame = usb.regs.hcor.frindex >> 3 &
                    (USBCMD_FRLIST_ELTS(usb.regs.hcor.usbcmd) - 1);
                if (!frame) {
                    usb_host_int(USBSTS_FRAME_LIST_OVER);
                }
                if (usb.regs.hcor.usbsts & USBSTS_PERIOD_SCHED) {
                    mem_dma_read(&link, (usb.regs.hcor.periodiclistbase & ~((1 << 12) - 1)) |
                                 frame << 2, sizeof(link));
                    usb_schedule_traverse(&link);
                }
                if (usb.regs.hcor.usbsts & USBSTS_ASYNC_SCHED) {
                    link.term = false;
                    link.type = QTYPE_QH;
                    link.ptr = usb.regs.hcor.asynclistaddr >> 5;
                    usb_schedule_traverse(&link);
                    usb.regs.hcor.asynclistaddr = link.val;
                }
                if (usb.regs.hcor.usbcmd & USBCMD_ASYNC_ADV_DRBL) {
                    usb_host_int(USBSTS_ASYNC_ADV);
                    usb.regs.hcor.usbcmd &= ~USBCMD_ASYNC_ADV_DRBL;
                }
            }
        }
    }
    sched_repeat(event, high_speed ? 1 : 8);
}

static void usb_device_event(enum sched_item_id event) {
    (void)event;
    usb.event.type = USB_TIMER_EVENT;
    usb_dispatch_event(NULL);
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
    if (pio < sizeof(usb.regs)) {
        if (unlikely(usb.regs.dma_ctrl >> 31 & usb.regs.phy_tmsr) && pio >= 0x044 && pio < 0x1C8) {
            value = 0;
        } else if (pio >= 0x1A0 && pio < 0x1AC) {
            value = usb.regBytes[pio];
        } else {
            value = read8(usb.regWords[pio >> 2], (pio & 3) << 3);
        }
    } else if (pio < (peek ? 0x1d8 : 0x1d4)) {
        value = usb.ep0_data[peek ? (usb.ep0_idx & 4) ^ (pio & 7) : (usb_ep0_idx_update() & 4) | (pio & 3)];
    }
    //if (pio != 0x082 && pio != 0x10C && pio != 0x10D && pio != 0x10E)
    //    gui_console_printf("%06x: 3%03hx -> %02hhx\n", cpu.registers.PC, pio, value);
    return value;
}

static void usb_write(uint16_t pio, uint8_t value, bool poke) {
    uint8_t index = pio >> 2;
    uint8_t bit_offset = (pio & 3) << 3;
    uint32_t old;
    (void)poke;
    if (unlikely(usb.regs.dma_ctrl >> 31 & usb.regs.phy_tmsr) && index >= (0x044 >> 2) && index < (0x1C8 >> 2)) {
        return;
    }
    //gui_console_printf("%06x: 3%03hx <- %02hhx\n", cpu.registers.PC, pio, value);
    switch (index) {
        case 0x010 >> 2: // USBCMD - USB Command Register
            write8(usb.regs.hcor.usbcmd, bit_offset, value &   0xFF0BFF >> bit_offset); // W mask (V or RO)
            usb.regs.hcor.usbsts &= ~(USBSTS_ASYNC_SCHED | USBSTS_PERIOD_SCHED | USBSTS_HCHALTED);
            usb.regs.hcor.usbsts |=
                (usb.regs.hcor.usbcmd & USBCMD_ASYNC_SCHED ? USBSTS_ASYNC_SCHED : 0) |
                (usb.regs.hcor.usbcmd & USBCMD_PERIOD_SCHED ? USBSTS_PERIOD_SCHED : 0) |
                (usb.regs.hcor.usbcmd & USBCMD_RUN ? 0 : USBSTS_HCHALTED);
            if ((uint32_t)value << bit_offset & USBCMD_HCRESET) {
                usb_reset_core();
            }
            break;
        case 0x014 >> 2: // USBSTS - USB Status Register
            usb.regs.hcor.usbsts &= ~((uint32_t)value << bit_offset & 0x3F);                       // WC mask (V or RO)
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
            old = usb.regs.hcor.portsc[0];
            usb.regs.hcor.portsc[0] &= ~(((uint32_t)value << bit_offset & 0x2E) | (0x7F0100 & 0xFF << bit_offset)) ^
                PORTSC_EN_STATUS; // W[0/1]C mask (V or RO or W)
            usb.regs.hcor.portsc[0] |= (uint32_t)value << bit_offset & 0x7F0180; // W mask (RO)
            if ((old ^ usb.regs.hcor.portsc[0]) & PORTSC_RESET) {
                // TODO: actually powered by gpio.
                usb.event.type = old & PORTSC_RESET ? USB_RESET_EVENT : USB_POWER_EVENT;
                usb_dispatch_event(NULL);
                if (usb_update_status_change(usb.regs.hcor.portsc, PORTSC_EN_STATUS, PORTSC_EN_CHANGE, true)) {
                    usb_host_int(USBSTS_PORT_CHANGE);
                }
                usb.regs.otgcsr |= OTGCSR_A_VBUS_VLD | OTGCSR_A_SESS_VLD | OTGCSR_B_SESS_VLD;
                usb.regs.otgcsr &= ~OTGCSR_SPD_MASK;
                switch (usb.event.speed) {
                    default:
                        usb.event.speed = USB_HIGH_SPEED;
                        fallthrough;
                    case USB_HIGH_SPEED:
                        if (!(usb.regs.dev_ctrl & DEVCTRL_FS_FORCED)) {
                            usb.regs.otgcsr |= OTGCSR_SPD_HIGH;
                            break;
                        }
                        usb.event.speed = USB_FULL_SPEED;
                        fallthrough;
                    case USB_FULL_SPEED:
                        usb.regs.otgcsr |= OTGCSR_SPD_FULL;
                        break;
                    case USB_LOW_SPEED:
                        usb.regs.otgcsr |= OTGCSR_SPD_LOW;
                        break;
                }
            }
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
            if (usb_update_status_change(usb.regs.hcor.portsc, PORTSC_J_STATE | PORTSC_CONN_STATUS, PORTSC_CONN_CHANGE,
                                         (usb.regs.otgcsr & (OTGCSR_A_BUSDROP | OTGCSR_A_BUSREQ)) == OTGCSR_A_BUSREQ)) {
                usb_host_int(USBSTS_PORT_CHANGE);
            }
            break;
        case 0x084 >> 2: // OTG Interrupt Status Register
            usb.regs.otgisr &= ~((uint32_t)value << bit_offset & OTGISR_MASK);                     // WC mask (V)
            break;
        case 0x088 >> 2: // OTG Interrupt Enable Register
            write8(usb.regs.otgier,                bit_offset, value & OTGISR_MASK >> bit_offset); // W mask (V)
            break;
        case 0x0C0 >> 2: // Global Interrupt Status Register
            usb.regs.isr &= ~((uint32_t)value << bit_offset & ISR_MASK);                           // WC mask (V)
            break;
        case 0x0C4 >> 2: // Global Interrupt Mask Register
            write8(usb.regs.imr,                   bit_offset, value &    IMR_MASK >> bit_offset); // W mask (V)
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
                usb.event.type = USB_TRANSFER_RESPONSE_EVENT;
                usb_transfer_info_t *transfer = &usb.event.info.transfer;
                transfer->buffer = usb.buffer;
                transfer->length = 0;
                transfer->max_pkt_size = CONTROL_MPS;
                transfer->status = USB_TRANSFER_COMPLETED;
                transfer->address = DEVADDR_ADDR(usb.regs.dev_addr);
                transfer->endpoint = 0;
                transfer->type = USB_CONTROL_TRANSFER;
                transfer->direction = !(usb.ep0_data[0] >> 7);
                usb_dispatch_event(NULL);
            }
            break;
        case 0x124 >> 2: // IDLE Counter Register
            write8(usb.regs.idle,                  bit_offset, value &        0x7 >> bit_offset); // W mask (V)
            break;
        case 0x130 >> 2: // Group Interrupt Mask Register
            write8(usb.regs.gimr,                  bit_offset, value &  GIMR_MASK >> bit_offset); // W mask (V)
            break;
        case 0x134 >> 2: // Group Interrupt Mask Register 0
            write8(usb.regs.gimr0,                 bit_offset, value & GIMR0_MASK >> bit_offset); // W mask (V)
            break;
        case 0x138 >> 2: // Group Interrupt Mask Register 1
            write8(usb.regs.gimr1,                 bit_offset, value & GIMR1_MASK >> bit_offset); // W mask (V)
            break;
        case 0x13C >> 2: // Group Interrupt Mask Register 2
            write8(usb.regs.gimr2,                 bit_offset, value & GIMR2_MASK >> bit_offset); // W mask (V)
            break;
        case 0x140 >> 2: // Group Interrupt Status Register
            usb.regs.gisr &= ~((uint32_t)value << bit_offset & GIMR_MASK);                        // WC mask (V)
            break;
        case 0x144 >> 2: // Group Interrupt Status Register 0
            usb.regs.gisr0 &= ~((uint32_t)value << bit_offset & GIMR0_MASK);                      // WC mask (V)
            break;
        case 0x148 >> 2: // Group Interrupt Status Register 1
            usb.regs.gisr1 &= ~((uint32_t)value << bit_offset & GIMR1_MASK);                      // WC mask (V)
            break;
        case 0x14C >> 2: // Group Interrupt Status Register 2
            usb.regs.gisr2 &= ~((uint32_t)value << bit_offset & GIMR2_MASK);                      // WC mask (V) [const mask]
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
            break;
        case 0x180 >> 2: case 0x184 >> 2: case 0x188 >> 2: case 0x18C >> 2:
        case 0x190 >> 2: case 0x194 >> 2: case 0x198 >> 2: case 0x19C >> 2: // OUT Endpoint Register
            write8(usb.regs.oep[index & 7],        bit_offset, value &     0x1FFF >> bit_offset); // W mask (V)
            break;
        case 0x1A0 >> 2: case 0x1A4 >> 2: // Endpoint Map Register
            usb.regs.epmap[pio & 7] = value;                                                      // W
            break;
        case 0x1A8 >> 2: // FIFO Map Register
            usb.regs.fifomap[pio & 3] = value & 0x3F;                                             // W mask (V)
            break;
        case 0x1AC >> 2: // FIFO Configuration Register
            write8(usb.regs.fifocfg,               bit_offset, value &       0x3F);               // W mask (V)
            break;
        case 0x1C0 >> 2: // DMA Target FIFO Register
            write8(usb.regs.dma_fifo,              bit_offset, value &       0x1F >> bit_offset); // W mask (V)
            break;
        case 0x1C8 >> 2: // DMA Control Register
            write8(usb.regs.dma_ctrl,              bit_offset, value & 0x81FFFF16 >> bit_offset); // W mask (V)
            if (value & DMACTRL_START >> bit_offset) {
                usb.event.type = USB_TRANSFER_REQUEST_EVENT;
                usb_transfer_info_t *transfer = &usb.event.info.transfer;
                transfer->buffer = usb.buffer;
                transfer->length = DMACTRL_LEN(usb.regs.dma_ctrl);
                transfer->status = USB_TRANSFER_COMPLETED;
                transfer->address = DEVADDR_ADDR(usb.regs.dev_addr);
                transfer->direction = usb.regs.dma_ctrl & DMACTRL_MEM2FIFO;
                if (usb.regs.dma_fifo & DMAFIFO_CX) {
                    transfer->max_pkt_size = CONTROL_MPS;
                    transfer->endpoint = 0;
                    transfer->type = USB_CONTROL_TRANSFER;
                    usb.regs.cxfifo = CXFIFO_SET_BYTES(usb.regs.cxfifo, 0);
                    usb.regs.cxfifo |= CXFIFO_CXFIFOE;
                    usb.regs.cxfifo &= ~CXFIFO_CXFIFOF;
                } else {
                    uint8_t *epmap = usb.regs.epmap;
                    for (transfer->endpoint = 1; transfer->endpoint <= 8;
                         ++transfer->endpoint, ++epmap) {
                        uint8_t fifo = *epmap >> (!transfer->direction << 2) & 3;
                        if (usb.regs.dma_fifo & 1 << fifo) {
                            transfer->type = (usb.regs.fifocfg >>
                                              ((fifo << 3) + FIFOCFG_TYPE_SHIFT)) & FIFOCFG_TYPE_MASK;
                            transfer->max_pkt_size =
                                EP_MAXPS((transfer->direction ? usb.regs.iep
                                                              : usb.regs.oep)[transfer->endpoint - 1]);
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
                //usb_transfer_info_t debug_transfer = *transfer;
                mem_dma_read(transfer->buffer, usb.regs.dma_addr, DMACTRL_LEN(usb.regs.dma_ctrl));
                usb_dispatch_event(NULL);
                //gui_console_printf("[USB] %c", debug_transfer.direction ? 'R' : 'S');
                //for (uint32_t i = 0; i != DMACTRL_LEN(usb.regs.dma_ctrl); ++i)
                //    gui_console_printf(" %02X", debug_transfer.buffer[i]);
                //gui_console_printf("\n");
            }
            break;
        case 0x1CC >> 2: // DMA Address Register
            write8(usb.regs.dma_addr,              bit_offset, value);                            // W
            break;
        case 0x1D0 >> 2: // EP0 Data Register
            if (!poke) {
                usb_ep0_idx_update();
            }
            break;
    }
    usb_update();
}

static void usb_init_events(void) {
    sched_init_event(SCHED_USB, CLOCK_12M, usb_event);
    if (!sched_active(SCHED_USB_DEVICE)) {
        sched_init_event(SCHED_USB_DEVICE, CLOCK_1M, usb_device_event);
    }
}

void usb_reset(void) {
    usb_reset_core();
    usb_reset_aux();
    usb_reset_otg();
    usb_plug_complete();
    usb_init_events();
}

static void usb_init_hccr(void) {
    usb.regs.hccr.data[0] = sizeof(usb.regs.hccr) | 0x0100 << 16; // v1.0
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
    memset(&usb, 0, sizeof(usb));
    usb_init_hccr();
    usb_reset();
    gui_console_printf("[CEmu] Initialized USB...\n");
    return device;
}

bool usb_save(FILE *image) {
    return fwrite(&usb, offsetof(usb_state_t, device), 1, image) == 1;
}

bool usb_restore(FILE *image) {
    usb_init_events();
    void *context = usb.event.context;
    bool success = fread(&usb, offsetof(usb_state_t, device), 1, image) == 1;
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
