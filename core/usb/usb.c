#include "usb.h"
#include "../cpu.h"
#include "../debug/debug.h"
#include "../emu.h"
#include "../interrupt.h"
#include "../mem.h"
#include "../schedule.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

extern void debugInstruction(void);

/* Global GPT state */
usb_state_t usb;
static struct timeval zero_tv = {};

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

uint8_t usb_status(void) {
    return (usb.regs.otgcsr & OTGCSR_A_VBUS_VLD ? 0x80 : 0) | (usb.regs.otgcsr & OTGCSR_DEV_B ? 0x40 : 0);
//  return ((usb.regs.otgcsr & OTGCSR_DEV_B ? OTGCSR_A_BUSREQ : (usb.regs.otgcsr & (OTGCSR_A_BUSDROP | OTGCSR_A_BUSREQ)) == OTGCSR_A_BUSREQ) ? 0x80 : 0) | (usb.regs.otgcsr & (OTGCSR_DEV_B | OTGCSR_ROLE_D) ? 0x40 : 0);
}

static void usb_update(void) {
    intrpt_set(INT_USB, (usb.regs.isr =
                         ((usb.regs.gisr = (usb.regs.gisr2 & ~usb.regs.gimr2 ? GISR_GRP2 : 0) |
                                           (usb.regs.gisr1 & ~usb.regs.gimr1 ? GISR_GRP1 : 0) |
                                           (usb.regs.gisr0 & ~usb.regs.gimr0 ? GISR_GRP0 : 0))
                          & ~usb.regs.gimr && usb.regs.dev_ctrl & DEVCTRL_GIRQ_EN ? ISR_DEV : 0) |
                         (usb.regs.otgisr      & usb.regs.otgier ? ISR_OTG : 0) |
                         (usb.regs.hcor.usbsts & usb.regs.hcor.usbintr ? ISR_HOST : 0)) & ~usb.regs.imr);
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
}

// Plug A:
//  plug:
//   OTGCSR_DEV_B -> OTGCSR_DEV_A
//   OTGCSR_ROLE_D -> OTGCSR_ROLE_H
//   ? -> 1 OTGISR_IDCHG
//   ? -> 1 OTGISR_RLCHG
//  unplug:
//   OTGCSR_DEV_A -> OTGCSR_DEV_B
//   OTGCSR_ROLE_H -> OTGCSR_ROLE_D
//   ? -> 1 OTGISR_APRM
//   ? -> 1 OTGISR_IDCHG
//   ? -> 1 OTGISR_RLCHG
static void usb_plug_a(void) {
    usb_unplug_b();
    usb.regs.otgcsr &= ~(OTGCSR_DEV_B | OTGCSR_ROLE_D);
    usb.regs.sof_fnr = 0;
    usb_otg_int(OTGISR_IDCHG | OTGISR_RLCHG);
}
static void usb_unplug_a(void) {
    if (usb_update_status_change(usb.regs.hcor.portsc, PORTSC_J_STATE | PORTSC_CONN_STATUS, PORTSC_CONN_CHANGE, false)) {
        usb_host_int(USBSTS_PORT_CHANGE);
    }
    usb.regs.otgcsr |= OTGCSR_DEV_B | OTGCSR_ROLE_D;
    usb_otg_int(OTGISR_APRM | OTGISR_IDCHG | OTGISR_RLCHG);
    usb_plug_b();
}

void usb_setup(const uint8_t *setup) {
    usb.regs.cxfifo &= ~0x3F;
    memcpy(usb.ep0_data, setup, sizeof usb.ep0_data);
    usb.ep0_idx = 0;
    usb_grp0_int(GISR0_CXSETUP);
}

void usb_send_pkt(const void *data, uint32_t size) {
    usb.data = (void *)data;
    usb.len = size;
    usb.regs.fifocsr[0] = (usb.regs.fifocsr[0] & FIFOCSR_RESET) | size;
    usb.regs.cxfifo &= ~CXFIFO_FIFOE_FIFO0;
    usb_grp1_int(GISR1_RX_FIFO(0));
}

//static uint8_t ep0_init[] = { 0x80, 0x06, 0x02, 0x02, 0x00, 0x00, 0x40, 0x00 };
//static uint8_t ep0_init[] = { 0x80, 0x06, 0x03, 0x03, 0x09, 0x04, 0x40, 0x00 };
//static uint8_t ep0_init[] = { 0x00, 0x05, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00 };
static void usb_event_no_libusb(enum sched_item_id event) {
    static const uint8_t set_addr[]   = { 0x00, 0x05, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00 };
    static const uint8_t set_config[] = { 0x00, 0x09, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00 };
    static const uint8_t rdy_pkt_00[] = { 0x00, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00, 0x04, 0x00 };
    static const uint8_t rdy_pkt_01[] = { 0x00, 0x00, 0x00, 0x10, 0x04, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x01, 0x00, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x07, 0xd0 };
    switch (usb.state) {
        case 0: // reset
            if (usb.regs.dev_ctrl & 0x80) {
                usb_grp2_int(GISR2_RESET);
                usb.state++;
            }
            break;
        case 1: // set addr
            if (!(usb.regs.gisr2 & GISR2_RESET)) {
                usb_setup(set_addr);
                usb.state++;
            }
            break;
        case 2: // set config
            if (usb.regs.cxfifo & CXFIFO_CXFIN) {
                usb_setup(set_config);
                usb.state++;
            }
            break;
        case 3: // rdy_pkt_00
            if (usb.regs.cxfifo & CXFIFO_CXFIN) {
                usb.regs.cxfifo &= ~CXFIFO_CXFIN;
                usb_send_pkt(rdy_pkt_00, sizeof rdy_pkt_00);
                usb.state++;
            }
            break;
        case 4: // rdy_pkt_01
            if (!(usb.regs.gisr1 & GISR1_RX_FIFO(0))) {
                usb_send_pkt(rdy_pkt_01, sizeof rdy_pkt_01);
                usb.state++;
            }
            break;
    }
    sched_repeat(event, 8);
}

static void usb_host_sys_err(void) {
    asm("int3");
    usb.regs.hcor.usbcmd &= ~USBCMD_RUN;
    usb.regs.hcor.usbsts |= USBSTS_HCHALTED;
    usb_host_int(USBSTS_HOST_SYS_ERR);
    gui_console_printf("[USB] Warning: Fatal host controller error!\n");
}

#define ASYNC_ITER_CAP 64
static void usb_process_async(void) {
    usb_qh_t *qh;
    usb_qlink_t link;
    usb_qtd_t *qtd;
    enum { WAIT_FOR_LIST_QH, DO_RELOAD, WAIT_FOR_START_EVENT } nak_state = WAIT_FOR_LIST_QH;
    enum { FETCH_QH, ADVANCE_QUEUE, EXECUTE_TRANSACTION, WRITE_BACK_QTD, FOLLOW_QH_HORIZONTAL_POINTER } host_state = FETCH_QH;
    uint8_t qh_transaction_counter;
    bool consider;
    usb.regs.hcor.usbsts |= USBSTS_RECLAMATION;
    for (int i = 0; i < ASYNC_ITER_CAP; i++) {
        switch (host_state) {
            case FETCH_QH:
                if (!(qh = ram_dma_ptr(usb.regs.hcor.asynclistaddr, sizeof *qh))) {
                    return usb_host_sys_err();
                } if (qh->h) {
                    if (nak_state != WAIT_FOR_START_EVENT) {
                        nak_state++;
                    } if (!qh->s_mask) {
                        if (!(usb.regs.hcor.usbsts & USBSTS_RECLAMATION)) {
                            return;
                        }
                        usb.regs.hcor.usbsts &= ~USBSTS_RECLAMATION;
                    }
                } if (qh->overlay.halted) {
                    host_state = FOLLOW_QH_HORIZONTAL_POINTER;
                } else if (qh->overlay.active) {
                    host_state = EXECUTE_TRANSACTION;
                } else if (qh->i) {
                    host_state = FOLLOW_QH_HORIZONTAL_POINTER;
                } else {
                    host_state = ADVANCE_QUEUE;
                }
                break;
            case ADVANCE_QUEUE:
                link.term = true;
                if (qh->overlay.total_bytes) {
                    link = qh->overlay.alt;
                } if (link.term) {
                    link = qh->overlay.next;
                } if (link.term) {
                    host_state = FOLLOW_QH_HORIZONTAL_POINTER;
                } else if (!(qtd = ram_dma_ptr(link.ptr << 5, sizeof *qtd))) {
                    return usb_host_sys_err();
                } else if (qtd->active) {
                    bool dt = qh->dtc ? qtd->dt : qh->overlay.dt;
                    bool ping = qh->eps != 2 ? qtd->ping : qh->overlay.ping;
                    qh->cur = link;
                    qh->overlay = *qtd;
                    qh->overlay.alt.nak_cnt = qh->nak_rl;
                    qh->overlay.ping = ping;
                    qh->overlay.dt = dt;
                    qh->overlay.bufs[1].c_prog_mask = 0;
                    qh->overlay.bufs[2].frame_tag = 0;
                    host_state = EXECUTE_TRANSACTION;
                } else {
                    host_state = FOLLOW_QH_HORIZONTAL_POINTER;
                }
                break;
            case EXECUTE_TRANSACTION:
                if (qh->s_mask) {
                    consider = qh->s_mask >> (usb.regs.hcor.frindex & 7) & 1;
                } else {
                    if (nak_state == DO_RELOAD && qh->nak_rl) {
                        qh->overlay.alt.nak_cnt = qh->nak_rl;
                    }
                    consider = qh->overlay.alt.nak_cnt || !qh->nak_rl;
                }
                qh_transaction_counter = qh->mult;
                if (!qh->s_mask && !qh_transaction_counter) {
                    consider = false;
                }
                if (consider) {
                    usb.regs.hcor.usbsts |= USBSTS_RECLAMATION;

                }
                break;
            case WRITE_BACK_QTD:
                break;
            case FOLLOW_QH_HORIZONTAL_POINTER:
                usb.regs.hcor.asynclistaddr = qh->horiz.ptr << 5;
                return;
        }
    }
}
static void usb_process_period(void) {
    uint32_t entry = mem_peek(usb.regs.hcor.periodiclistbase + (usb.regs.hcor.frindex & (USBCMD_FRLIST_BYTES(usb.regs.hcor.usbcmd) - 1) << 3), 4);
}

static bool usb_gather_qtd(uint8_t *dst, usb_qtd_t *qtd, int *len) {
    void *src;
    uint16_t block_len;
    if (qtd->total_bytes > 0x5000) {
        return true;
    }
    if (qtd->pid & 1) {
        *len += qtd->total_bytes;
        return false;
    }
    while (qtd->total_bytes) {
        if (qtd->c_page >= 5) {
            return true;
        }
        if ((block_len = (1 << 12) - qtd->bufs[0].off) > qtd->total_bytes) {
            block_len = qtd->total_bytes;
        }
        src = ram_dma_ptr(qtd->bufs[qtd->c_page].ptr << 12 | qtd->bufs[0].off, block_len);
        if (!src) {
            return true;
        }
        memcpy(dst, src, block_len);
        dst += block_len;
        qtd->c_page += !(qtd->bufs[0].off += block_len);
        *len += block_len;
        qtd->total_bytes -= block_len;
    }
    return false;
}

static bool usb_scatter_qtd(usb_qtd_t *qtd, const uint8_t *src, uint16_t len) {
    void *dst;
    uint16_t block_len;
    if (qtd->total_bytes > 0x5000) {
        return true;
    }
    if (!(qtd->pid & 1)) {
        return false;
    }
    while (len) {
        if (qtd->c_page >= 5) {
            return true;
        }
        if ((block_len = (1 << 12) - qtd->bufs[0].off) > len) {
            block_len = len;
        }
        dst = ram_dma_ptr(qtd->bufs[qtd->c_page].ptr << 12 | qtd->bufs[0].off, block_len);
        if (!dst) {
            return true;
        }
        memcpy(dst, src, block_len);
        qtd->c_page += !(qtd->bufs[0].off += block_len);
        src += block_len;
        qtd->total_bytes -= block_len;
        len -= block_len;
    }
    return false;
}

#ifdef HAS_LIBUSB
static void usb_xfer_append_data(struct libusb_transfer *xfer, const void *src, size_t len) {
    memcpy((xfer->type == LIBUSB_TRANSFER_TYPE_CONTROL ? libusb_control_transfer_get_data(xfer) : xfer->buffer) + xfer->actual_length, src, len);
    xfer->actual_length += len;
}

static void usb_write_back_qtd(usb_qh_t *qh) {
    usb_qtd_t *qtd;
    if (!(qtd = ram_dma_ptr(qh->cur.ptr << 5, sizeof *qtd))) {
        usb_host_sys_err();
        return;
    }
    qtd->ping = qh->overlay.ping;
    qtd->split = qh->overlay.split;
    qtd->missed = qh->overlay.missed;
    qtd->xact_err = qh->overlay.xact_err;
    qtd->babble = qh->overlay.babble;
    qtd->buf_err = qh->overlay.buf_err;
    qtd->halted = qh->overlay.halted;
    qtd->active = qh->overlay.active;
    qtd->cerr = qh->overlay.cerr;
    qtd->total_bytes = qh->overlay.total_bytes;
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

static void LIBUSB_CALL usb_process_xfer(struct libusb_transfer *xfer) {
    usb_qh_t *qh;
    uint8_t *buf = usb.xfer->buffer;
    xfer->length = 0;
    usb.wait = false;
    if (usb.process) {
        usb.process = false;
        //gui_console_printf("[USB] Callback called: %d\n", xfer->status);
        qh = xfer->user_data;
        switch (xfer->type) {
            case LIBUSB_TRANSFER_TYPE_CONTROL:
                buf = libusb_control_transfer_get_data(xfer);
                // FALLTHROUGH
            case LIBUSB_TRANSFER_TYPE_BULK:
                if (xfer->status) {
                    gui_console_printf("[USB] Error: Transfer failed: %s\n", libusb_error_name(xfer->status));
                }
                switch (xfer->status) {
                    case LIBUSB_TRANSFER_COMPLETED:
                        if (usb_scatter_qtd(&qh->overlay, buf, xfer->actual_length)) {
                            usb_host_sys_err();
                            return;
                        }
                        return usb_qh_completed(qh);
                    case LIBUSB_TRANSFER_ERROR:
                        qh->overlay.xact_err = true;
                        return usb_qh_halted(qh);
                    case LIBUSB_TRANSFER_TIMED_OUT:
                        qh->overlay.cerr = 0;
                        // FALLTHROUGH
                    case LIBUSB_TRANSFER_STALL:
                        return usb_qh_halted(qh);
                    case LIBUSB_TRANSFER_NO_DEVICE:
                        return;
                    case LIBUSB_TRANSFER_OVERFLOW:
                        qh->overlay.buf_err = true;
                        return usb_qh_halted(qh);
                    default:
                        asm("int3");
                        gui_console_printf("[USB] Error: Callback called with unknown status %d!\n", xfer->status);
                        return usb_qh_halted(qh);
                }
            default:
                asm("int3");
                gui_console_printf("[USB] Error: Callback called with unknown type %d!\n", xfer->type);
                return usb_qh_halted(qh);
        }
    }
}

static enum libusb_transfer_status libusb_status_from_error(enum libusb_error error) {
    switch (error) {
        case LIBUSB_SUCCESS:
            return LIBUSB_TRANSFER_COMPLETED;
        case LIBUSB_ERROR_IO:
        case LIBUSB_ERROR_INVALID_PARAM:
        case LIBUSB_ERROR_ACCESS:
        case LIBUSB_ERROR_BUSY:
        case LIBUSB_ERROR_PIPE:
        case LIBUSB_ERROR_INTERRUPTED:
        case LIBUSB_ERROR_NO_MEM:
        case LIBUSB_ERROR_NOT_SUPPORTED:
        case LIBUSB_ERROR_OTHER:
            return LIBUSB_TRANSFER_ERROR;
        case LIBUSB_ERROR_TIMEOUT:
            return LIBUSB_TRANSFER_TIMED_OUT;
        case LIBUSB_ERROR_NOT_FOUND:
            return LIBUSB_TRANSFER_STALL;
        case LIBUSB_ERROR_NO_DEVICE:
            return LIBUSB_TRANSFER_NO_DEVICE;
        case LIBUSB_ERROR_OVERFLOW:
            return LIBUSB_TRANSFER_OVERFLOW;
    }
    return LIBUSB_TRANSFER_ERROR;
}

static bool usb_execute_setup(struct libusb_transfer *xfer) {
    enum libusb_error err;
    struct libusb_config_descriptor *config = NULL;
    struct libusb_control_setup *setup = libusb_control_transfer_get_setup(xfer);
    void *data = libusb_control_transfer_get_data(xfer);
    uint8_t type = setup->wValue >> 8, index = setup->wValue, iface, alt, endpt;
    xfer->status = LIBUSB_TRANSFER_STALL;
    xfer->actual_length = 0;
    switch (setup->bRequest) {
        case LIBUSB_REQUEST_SET_ADDRESS:
            if (setup->bmRequestType == (LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_STANDARD | LIBUSB_RECIPIENT_DEVICE) && setup->wValue && setup->wValue < 0x80) {
                xfer->status = LIBUSB_TRANSFER_COMPLETED;
            }
            break;
        case LIBUSB_REQUEST_GET_DESCRIPTOR:
            switch (type) {
                case LIBUSB_DT_DEVICE:
                    if (setup->bmRequestType == (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_STANDARD | LIBUSB_RECIPIENT_DEVICE) && !index && !setup->wIndex &&
                        (xfer->status = libusb_status_from_error(libusb_get_device_descriptor(libusb_get_device(xfer->dev_handle), data))) == LIBUSB_TRANSFER_COMPLETED) {
                        xfer->actual_length = sizeof(struct libusb_device_descriptor);
                    }
                    break;
                case LIBUSB_DT_CONFIG:
                    if (setup->bmRequestType == (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_STANDARD | LIBUSB_RECIPIENT_DEVICE) && !setup->wIndex &&
                        (xfer->status = libusb_status_from_error(libusb_get_config_descriptor(libusb_get_device(xfer->dev_handle), index, &config))) == LIBUSB_TRANSFER_COMPLETED) {
                        usb_xfer_append_data(xfer, config, config->bLength);
                        usb_xfer_append_data(xfer, config->extra, config->extra_length);
                        for (iface = 0; iface != config->bNumInterfaces; ++iface) {
                            for (alt = 0; alt != config->interface[iface].num_altsetting; ++alt) {
                                usb_xfer_append_data(xfer, &config->interface[iface].altsetting[alt], config->interface[iface].altsetting[alt].bLength);
                                usb_xfer_append_data(xfer, config->interface[iface].altsetting[alt].extra, config->interface[iface].altsetting[alt].extra_length);
                                for (endpt = 0; endpt != config->interface[iface].altsetting[alt].bNumEndpoints; ++endpt) {
                                    usb_xfer_append_data(xfer, &config->interface[iface].altsetting[alt].endpoint[endpt], config->interface[iface].altsetting[alt].endpoint[endpt].bLength);
                                    usb_xfer_append_data(xfer, config->interface[iface].altsetting[alt].endpoint[endpt].extra, config->interface[iface].altsetting[alt].endpoint[endpt].extra_length);
                                }
                            }
                        }
                    }
                    break;
                case LIBUSB_DT_STRING:
                case LIBUSB_DT_INTERFACE:
                case LIBUSB_DT_ENDPOINT:
                case LIBUSB_DT_BOS:
                case LIBUSB_DT_DEVICE_CAPABILITY:
                case LIBUSB_DT_HID:
                case LIBUSB_DT_REPORT:
                case LIBUSB_DT_PHYSICAL:
                case LIBUSB_DT_HUB:
                case LIBUSB_DT_SUPERSPEED_HUB:
                case LIBUSB_DT_SS_ENDPOINT_COMPANION:
                    return false;
            }
            break;
        case LIBUSB_REQUEST_SET_DESCRIPTOR:
        case LIBUSB_REQUEST_GET_CONFIGURATION:
            return false;
        case LIBUSB_REQUEST_SET_CONFIGURATION:
            if (setup->bmRequestType == (LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_STANDARD | LIBUSB_RECIPIENT_DEVICE) && !type && !setup->wIndex) {
                if (libusb_get_active_config_descriptor(libusb_get_device(xfer->dev_handle), &config) == LIBUSB_SUCCESS) {
                    for (iface = 0; iface != config->bNumInterfaces; ++iface) {
                        gui_console_printf("[USB] Info: Kernel driver was active on interface %u: %s!\n", iface, libusb_kernel_driver_active(usb.xfer->dev_handle, iface) ? "yes" : "no");
                        libusb_detach_kernel_driver(usb.xfer->dev_handle, iface);
                        libusb_release_interface(usb.xfer->dev_handle, iface);
                    }
                    libusb_free_config_descriptor(config);
                    config = NULL;
                }
                if ((xfer->status = libusb_status_from_error(err = libusb_get_config_descriptor_by_value(libusb_get_device(xfer->dev_handle), index, &config))) == LIBUSB_TRANSFER_COMPLETED &&
                    (xfer->status = libusb_status_from_error(err = libusb_set_configuration(xfer->dev_handle, index))) == LIBUSB_TRANSFER_COMPLETED) {
                    for (iface = 0; iface != config->bNumInterfaces; ++iface) {
                        gui_console_printf("[USB] Info: Kernel driver now active on interface %u: %s!\n", iface, libusb_kernel_driver_active(usb.xfer->dev_handle, iface) ? "yes" : "no");
                        libusb_claim_interface(usb.xfer->dev_handle, iface);
                    }
                } else {
                    gui_console_printf("[USB] Error: Set configuration failed: %s!\n", libusb_error_name(err));
                }
            }
            break;
        case LIBUSB_REQUEST_GET_INTERFACE:
        case LIBUSB_REQUEST_SET_INTERFACE:
        case LIBUSB_REQUEST_SYNCH_FRAME:
        case LIBUSB_REQUEST_SET_SEL:
        case LIBUSB_SET_ISOCH_DELAY:
        default:
            return false;
    }
    if (xfer->actual_length > setup->wLength) {
        xfer->actual_length = setup->wLength;
    }
    if (xfer->status != LIBUSB_TRANSFER_COMPLETED) {
        //asm("int3");
    }
    libusb_free_config_descriptor(config);
    return true;
}

static void usb_execute_qh(usb_qh_t *qh) {
    usb_qtd_t *qtd = &qh->overlay;
    enum libusb_error err;
    usb.xfer->user_data = qh;
    usb.xfer->endpoint = qtd->pid << 7 | qh->endpt;
    if (usb.control && qtd->pid < 2) {
        usb.control = qtd->total_bytes;
        if (!qtd->total_bytes && !usb.xfer->length) { // Status Stage
            return usb_qh_completed(qh);
        } // Data Stage (or Status Stage with no Data)
        if (usb.xfer->length != 8 || usb_gather_qtd(libusb_control_transfer_get_data(usb.xfer), qtd, &usb.xfer->length)) {
            return usb_host_sys_err();
        }
        if (usb_execute_setup(usb.xfer)) {
            usb.wait = usb.process = true;
            return usb_process_xfer(usb.xfer);
        }
        usb.xfer->type = LIBUSB_TRANSFER_TYPE_CONTROL;
    } else {
        if (qtd->pid > 2 || usb_gather_qtd(usb.xfer->buffer, qtd, &usb.xfer->length)) {
            return usb_host_sys_err();
        }
        if ((usb.control = qtd->pid == 2)) { // Setup Stage
            return usb_qh_completed(qh);
        }
        usb.xfer->type = LIBUSB_TRANSFER_TYPE_BULK;
    }
    usb.wait = usb.process = true;
    if ((err = libusb_submit_transfer(usb.xfer))) {
        gui_console_printf("[USB] Error: Submit transfer failed: %s!\n", libusb_error_name(err));
    }
}

static bool usb_process_qh(usb_qh_t *qh) {
    uint8_t qHTransactionCounter;
    // Fetch QH
    if (!qh) {
        usb_host_sys_err();
        return true;
    }
    if (!qh->s_mask && qh->h) {
        if (!(usb.regs.hcor.usbsts & USBSTS_RECLAMATION)) {
            return true;
        }
        usb.regs.hcor.usbsts &= ~USBSTS_RECLAMATION;
        if (usb.nak_cnt_reload_state != USB_NAK_CNT_WAIT_FOR_START_EVENT) {
            // Either WAIT_FOR_LIST_HEAD -> DO_RELOAD or DO_RELOAD -> WAIT_FOR_START_EVENT
            usb.nak_cnt_reload_state--;
        }
    }
    if (qh->overlay.halted) {
        return false;
    }
    if (!qh->overlay.active) {
        usb_qlink_t link;
        usb_qtd_t *qtd;
        if (qh->i) {
            return false;
        }
        // Advance Queue
        link.term = true;
        if (qh->overlay.total_bytes) {
            link = qh->overlay.alt;
        }
        if (link.term) {
            link = qh->overlay.next;
        }
        if (link.term) {
            return false;
        }
        if (!(qtd = ram_dma_ptr(link.ptr << 5, sizeof *qtd))) {
            usb_host_sys_err();
            return true;
        }
        if (!qtd->active) {
            return false;
        }
        bool dt = qh->dtc ? qtd->dt : qh->overlay.dt;
        bool ping = qh->eps != 2 ? qtd->ping : qh->overlay.ping;
        qh->cur = link;
        qh->overlay = *qtd;
        qh->overlay.alt.nak_cnt = qh->nak_rl;
        qh->overlay.ping = ping;
        qh->overlay.dt = dt;
        qh->overlay.bufs[1].c_prog_mask = 0;
        qh->overlay.bufs[2].frame_tag = 0;
    }
    // Execute Transaction
    if (qh->s_mask) {
        // Interrupt Transfer Pre-condition Criteria
        if (!(qh->s_mask & 1 << (usb.regs.hcor.frindex & 7))) {
            return false;
        }
    } else {
        // Asynchronous Transfer Pre-operations
        if (usb.nak_cnt_reload_state == USB_NAK_CNT_DO_RELOAD) {
            qh->overlay.alt.nak_cnt = qh->nak_rl;
        }
        // Asynchronous Transfer Pre-condition Criteria
        if (!qh->overlay.alt.nak_cnt && qh->nak_rl) {
            return false;
        }
    }
    usb.regs.hcor.usbsts |= USBSTS_RECLAMATION;
    usb.fake_recl_head = NULL;
    for (qHTransactionCounter = qh->mult; qHTransactionCounter && qh->overlay.active;
         --qHTransactionCounter) {
        usb_execute_qh(qh);
    }
    return usb.wait;
}

static void usb_start_event(void) {
    usb.regs.hcor.usbsts |= USBSTS_RECLAMATION;
    usb.nak_cnt_reload_state = USB_NAK_CNT_WAIT_FOR_LIST_HEAD;
    usb.fake_recl_head = NULL;
}
#endif

static void usb_event(enum sched_item_id event) {
#ifdef HAS_LIBUSB
    bool high_speed = false;
    usb_qh_t *qh;
    uint8_t i = 0;
    enum libusb_error err;
    if (usb.dev && !usb.xfer->dev_handle && ++usb.delay > 100) {
        if ((err = libusb_open(usb.dev, &usb.xfer->dev_handle))) {
            gui_console_printf("[USB] Error: Open device: %s!\n", libusb_error_name(err));
            usb.xfer->dev_handle = NULL;
        } else {
            usb_plug_a();
        }
        usb.delay = 0;
    }
    if ((err = libusb_handle_events_timeout(usb.ctx, &zero_tv))) {
        gui_console_printf("[USB] Error: Handle events: %s!\n", libusb_error_name(err));
    }
    if (!usb.wait && usb.regs.otgcsr & OTGCSR_A_VBUS_VLD) {
        if (usb.regs.otgcsr & OTGCSR_DEV_B) {
            if ((high_speed = usb.regs.dev_ctrl & DEVCTRL_HS)) {
                usb.regs.sof_fnr += 1 << 11;
                usb.regs.sof_fnr &= (1 << 14) - 1;
            }
            if (!SOFFNR_UFN(usb.regs.sof_fnr)) {
                usb.regs.sof_fnr = SOFFNR_FNR(usb.regs.sof_fnr + 1);
            }
        } else {
            high_speed = usb.regs.otgcsr & OTGCSR_SPD_HIGH;
            while (usb.regs.hcor.usbcmd & USBCMD_RUN) {
                if (false && usb.regs.hcor.usbsts & USBSTS_PERIOD_SCHED) {
                    asm("int3");
                    usb_host_sys_err();
                    break;
                }
                if (usb.regs.hcor.usbsts & USBSTS_ASYNC_SCHED) {
                    usb_start_event();
                    while (true) {
                        qh = ram_dma_ptr(usb.regs.hcor.asynclistaddr, sizeof *qh);
                        if (usb_process_qh(qh)) {
                            break;
                        }
                        if (qh == usb.fake_recl_head) {
                            //gui_console_printf("[USB] Warning: No reclamation head!\n");
                            break;
                        }
                        if (!++i) {
                            gui_console_printf("[USB] Warning: Very long asynchronous list!\n");
                            break;
                        }
                        if (!usb.fake_recl_head) {
                            usb.fake_recl_head = qh;
                        }
                        // Follow QH Horizontal Pointer
                        usb.regs.hcor.asynclistaddr = qh->horiz.ptr << 5;
                    }
                }
                break;
            }
        }
    }
    sched_repeat(event, high_speed ? 1 : 8);
#else
    (void)event;
#endif
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
    //fprintf(stderr, "%06x: 3%03hx -> %02hhx\n", cpu.registers.PC, pio, value);
    return value;
}

static uint32_t usb_write_reg_masked(uint32_t *reg, uint32_t mask, uint8_t value, uint8_t bit_offset) {
    uint32_t old = *reg;
    mask &= 0xFF << bit_offset;
    return old ^ (*reg = (old & ~mask) | ((uint32_t)value << bit_offset & mask));
}

static void usb_write(uint16_t pio, uint8_t value, bool poke) {
    uint8_t index = pio >> 2;
    uint8_t bit_offset = (pio & 3) << 3;
    uint32_t old, changed;
#ifdef HAS_LIBUSB
    enum libusb_error err;
#endif
    (void)poke;
    //fprintf(stderr, "%06x: 3%03hx <- %02hhx\n", cpu.registers.PC, pio, value);
    switch (index) {
        case 0x010 >> 2: // USBCMD - USB Command Register
            changed = usb_write_reg_masked(&usb.regs.hcor.usbcmd, 0xFF0BFF, value, bit_offset);
            usb.regs.hcor.usbsts &= ~(USBSTS_ASYNC_SCHED | USBSTS_PERIOD_SCHED | USBSTS_HCHALTED);
            usb.regs.hcor.usbsts |=
                (usb.regs.hcor.usbcmd & USBCMD_ASYNC_SCHED ? USBSTS_ASYNC_SCHED : 0) |
                (usb.regs.hcor.usbcmd & USBCMD_PERIOD_SCHED ? USBSTS_PERIOD_SCHED : 0) |
                (usb.regs.hcor.usbcmd & USBCMD_RUN ? 0 : USBSTS_HCHALTED);
            if (usb.regs.hcor.usbcmd & USBCMD_RUN & changed) {
                usb.process = false;
            }
            if (usb.regs.hcor.usbcmd & USBCMD_HCRESET & changed) {
                usb_reset_core();
            }
            break;
        case 0x014 >> 2: // USBSTS - USB Status Register
            usb.regs.hcor.usbsts &= ~((uint32_t)value << bit_offset & 0x3F);                      // WC mask (V or RO)
            break;
        case 0x018 >> 2: // USBINTR - USB Interrupt Enable Register
            usb_write_reg_masked(&usb.regs.hcor.usbintr, 0x3F, value, bit_offset);
            break;
        case 0x01C >> 2: // FRINDEX - Frame Index Register
            usb_write_reg_masked(&usb.regs.hcor.frindex, 0x3FFF, value, bit_offset);
            break;
        case 0x024 >> 2: // PERIODICLISTBASE - Periodic Frame List Base Address Register
            usb_write_reg_masked(&usb.regs.hcor.periodiclistbase, ~0xFFF, value, bit_offset);
            break;
        case 0x028 >> 2: // ASYNCLISTADDR - Current Asynchronous List Address Register
            usb_write_reg_masked(&usb.regs.hcor.asynclistaddr, ~0x1F, value, bit_offset);
            break;
        case 0x030 >> 2: // PORTSC - Port Status and Control Register
            old = usb.regs.hcor.portsc[0];
            usb.regs.hcor.portsc[0] &= ~(((uint32_t)value << bit_offset & 0x2E) | 0x1F0100) ^ PORTSC_EN_STATUS; // W[0/1]C mask (V or RO or W)
            usb.regs.hcor.portsc[0] |= (uint32_t)value << bit_offset & 0x1F0100; // W mask (RO)
#ifdef HAS_LIBUSB
            if (usb.xfer->dev_handle && old & ~usb.regs.hcor.portsc[0] & PORTSC_RESET) {
                usb.control = false;
                if ((err = libusb_reset_device(usb.xfer->dev_handle))) {
                    gui_console_printf("[USB] Error: Reset device failed: %s!\n", libusb_error_name(err));
                } else {
                    if (usb_update_status_change(usb.regs.hcor.portsc, PORTSC_EN_STATUS, PORTSC_EN_CHANGE, true)) {
                        usb_host_int(USBSTS_PORT_CHANGE);
                    }
                    usb.regs.otgcsr |= OTGCSR_A_VBUS_VLD | OTGCSR_A_SESS_VLD | OTGCSR_B_SESS_VLD;
                    usb.regs.otgcsr &= ~OTGCSR_SPD_MASK;
                    switch (libusb_get_device_speed(libusb_get_device(usb.xfer->dev_handle))) {
                        case LIBUSB_SPEED_LOW:  usb.regs.otgcsr |= OTGCSR_SPD_LOW;  break;
                        case LIBUSB_SPEED_FULL: usb.regs.otgcsr |= OTGCSR_SPD_FULL; break;
                        case LIBUSB_SPEED_HIGH: usb.regs.otgcsr |= OTGCSR_SPD_HIGH; break;
                    }
                }
            }
#endif
            break;
        case 0x040 >> 2: // Miscellaneous Register
            usb_write_reg_masked(&usb.regs.miscr, 0xFFF, value, bit_offset);
            break;
        case 0x044 >> 2: // unknown
            usb_write_reg_masked(&usb.regs.rsvd2[0], 0x7FFF, value, bit_offset);
            break;
        case 0x048 >> 2: // unknown
            usb_write_reg_masked(&usb.regs.rsvd2[1], 0xFFF, value, bit_offset);
            break;
        case 0x080 >> 2: // OTG Control Status Register
            usb_write_reg_masked(&usb.regs.otgcsr, 0x1A00FFF7, value, bit_offset);
            if (usb_update_status_change(usb.regs.hcor.portsc, PORTSC_J_STATE | PORTSC_CONN_STATUS, PORTSC_CONN_CHANGE,
                                         (usb.regs.otgcsr & (OTGCSR_A_BUSDROP | OTGCSR_A_BUSREQ)) == OTGCSR_A_BUSREQ)) {
                usb_host_int(USBSTS_PORT_CHANGE);
            }
            break;
        case 0x084 >> 2: // OTG Interrupt Status Register
            usb.regs.otgisr &= ~((uint32_t)value << bit_offset & OTGISR_MASK);                    // WC mask (V)
            break;
        case 0x088 >> 2: // OTG Interrupt Enable Register
            usb_write_reg_masked(&usb.regs.otgier, OTGISR_MASK, value, bit_offset);
            break;
        case 0x0C0 >> 2: // Global Interrupt Status Register
            usb.regs.isr &= ~((uint32_t)value << bit_offset & ISR_MASK);                          // WC mask (V)
            break;
        case 0x0C4 >> 2: // Global Interrupt Mask Register
            usb_write_reg_masked(&usb.regs.imr, IMR_MASK, value, bit_offset);
            break;
        case 0x100 >> 2: // Device Control Register
            usb_write_reg_masked(&usb.regs.dev_ctrl, 0x2AF, value, bit_offset);
            break;
        case 0x104 >> 2: // Device Address Register
            usb_write_reg_masked(&usb.regs.dev_addr, 0xFF, value, bit_offset);
            break;
        case 0x108 >> 2: // Device Test Register
            usb_write_reg_masked(&usb.regs.dev_test, 0x7A, value, bit_offset);
            break;
        case 0x110 >> 2: // SOF Mask Timer Register
            usb_write_reg_masked(&usb.regs.sof_mtr, 0xFFFF, value, bit_offset);
            break;
        case 0x114 >> 2: // PHY Test Mode Selector Register
            usb_write_reg_masked(&usb.regs.phy_tmsr, 0x1F, value, bit_offset);
            break;
        case 0x118 >> 2: // unknown
            usb_write_reg_masked(&usb.regs.rsvd5[0], 0x3F, value, bit_offset);
            break;
        case 0x120 >> 2: // CX FIFO Register
            usb_write_reg_masked(&usb.regs.cxfifo, 7, value, bit_offset);
            if (usb.regs.cxfifo & CXFIFO_CXFIN) {
                usb.regs.gisr0 &= ~GISR0_CXEND;
            }
            break;
        case 0x124 >> 2: // IDLE Counter Register
            usb_write_reg_masked(&usb.regs.idle, 7, value, bit_offset);
            break;
        case 0x130 >> 2: // Group Interrupt Mask Register
            usb_write_reg_masked(&usb.regs.gimr, GIMR_MASK, value, bit_offset);
            break;
        case 0x134 >> 2: // Group Interrupt Mask Register 0
            usb_write_reg_masked(&usb.regs.gimr0, GIMR0_MASK, value, bit_offset);
            break;
        case 0x138 >> 2: // Group Interrupt Mask Register 1
            usb_write_reg_masked(&usb.regs.gimr1, GIMR1_MASK, value, bit_offset);
            break;
        case 0x13C >> 2: // Group Interrupt Mask Register 2
            usb_write_reg_masked(&usb.regs.gimr2, GIMR2_MASK, value, bit_offset);
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
            usb_write_reg_masked(&usb.regs.rxzlp, 0xFF, value, bit_offset);
            break;
        case 0x154 >> 2: // Transfer Zero-Length-Packet Register
            usb_write_reg_masked(&usb.regs.txzlp, 0xFF, value, bit_offset);
            break;
        case 0x158 >> 2: // ISOC Error/Abort Status Register
            usb_write_reg_masked(&usb.regs.isoeasr, 0xFF00FF, value, bit_offset);
            break;
        case 0x160 >> 2: case 0x164 >> 2: case 0x168 >> 2: case 0x16C >> 2:
        case 0x170 >> 2: case 0x174 >> 2: case 0x178 >> 2: case 0x17C >> 2: // IN Endpoint Register
            usb_write_reg_masked(&usb.regs.iep[index & 7], 0xFFFF, value, bit_offset);
        case 0x180 >> 2: case 0x184 >> 2: case 0x188 >> 2: case 0x18C >> 2:
        case 0x190 >> 2: case 0x194 >> 2: case 0x198 >> 2: case 0x19C >> 2: // OUT Endpoint Register
            usb_write_reg_masked(&usb.regs.oep[index & 7], 0x1FFF, value, bit_offset);
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
            usb_write_reg_masked(&usb.regs.dma_fifo, 0x1F, value, bit_offset);
            break;
        case 0x1C8 >> 2: // DMA Control Register
            usb_write_reg_masked(&usb.regs.dma_ctrl, 0x81FFFF17, value, bit_offset);
            if (usb.regs.dma_ctrl & DMACTRL_START) {
                uint32_t len = usb.regs.dma_ctrl >> 8 & 0x1ffff;
                uint8_t *mem = phys_mem_ptr(usb.regs.dma_addr, len);
                if (mem) {
                    if (usb.regs.dma_ctrl & DMACTRL_MEM2FIFO) {
                        gui_console_printf("[USB] ");
                        while (len--)
                            gui_console_printf("%02X", *mem++);
                        gui_console_printf("\n");
                    } else if (usb.data && usb.len) {
                        uint32_t rem = FIFOCSR_BYTES(usb.regs.fifocsr[0]);
                        uint32_t amt = len < rem ? len : rem;
                        memcpy(mem, usb.data, amt);
                        usb.regs.fifocsr[0] -= amt;
                        if (amt == rem) {
                            usb.regs.cxfifo |= CXFIFO_FIFOE_MASK;
                            usb.regs.gisr1 &= ~0xff;
                        }
                        usb.data += amt;
                    }
                    usb.regs.dma_ctrl &= ~DMACTRL_START;
                    usb_grp2_int(GISR2_DMAFIN);
                    break;
                }
                usb_grp2_int(GISR2_DMAERR);
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

void usb_reset_core(void) {
#define clear(array) memset(array, 0, sizeof array)
    usb.regs.hcor.usbcmd                = 0x00080B00;
    usb.regs.hcor.usbsts                = 0x00001000;
    usb.regs.hcor.usbintr               = 0;
    usb.regs.hcor.frindex               = 0;
    usb.regs.hcor.ctrldssegment         = 0;
}

void usb_reset_aux(void) {
    usb.regs.hcor.rsvd0[0] = usb.regs.hcor.portsc[0] = 0;
    clear(usb.regs.hcor.rsvd0);
    clear(usb.regs.hcor.portsc);
}

void usb_reset_otg(void) {
    int i;
    clear(usb.regs.rsvd1);
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
    usb.regs.epmap14 = usb.regs.epmap58 = 0xFFFFFFFF;
    usb.regs.fifomap                    = 0x0F0F0F0F;
    usb.regs.fifocfg                    = 0;
    clear(usb.regs.fifocsr);
    usb.regs.dma_fifo                   = 0;
    clear(usb.regs.rsvd8);
    usb.regs.dma_ctrl                   = 0;
    clear(usb.ep0_data);
    usb.ep0_idx                         = 0;
    usb_otg_int(OTGISR_BSESSEND); // because otgcsr & OTGCSR_B_SESS_END
    usb_grp2_int(GISR2_IDLE);     // because idle == 0 ms
#undef clear
}

void usb_reset(void) {
    usb_reset_core();
    usb_reset_aux();
    usb_reset_otg();
    usb.state = 0;
    usb.data  = NULL;
    usb.len   = 0;
    usb_plug_b();
#ifdef HAS_LIBUSB
    sched.items[SCHED_USB].callback.event = usb_event;
#else
    sched.items[SCHED_USB].callback.event = usb_event_no_libusb;
#endif
    sched.items[SCHED_USB].clock = CLOCK_USB;
    sched_clear(SCHED_USB_DMA);
}

static void usb_init_hccr(void) {
    usb.regs.hccr.data[0] = sizeof usb.regs.hccr | 0x0100 << 16; // v1.0
    usb.regs.hccr.data[1] = 1; // 1 port
    usb.regs.hccr.data[2] = 1 << 2 | // async sched park (supported)
                            1 << 1 | // prog frame list (supported)
                            0 << 0 ; // interface (32-bit)
    usb.regs.hccr.data[3] = 0; // No Port Routing Rules
}

#ifdef HAS_LIBUSB
static void usb_close(libusb_device_handle **dev_handle) {
    struct libusb_config_descriptor *config;
    uint8_t iface;
    if (!*dev_handle) {
        return;
    }
    if (libusb_get_active_config_descriptor(libusb_get_device(*dev_handle), &config) == LIBUSB_SUCCESS) {
        for (iface = 0; iface != config->bNumInterfaces; ++iface) {
            libusb_release_interface(*dev_handle, iface);
            libusb_attach_kernel_driver(*dev_handle, iface);
            gui_console_printf("[USB] Info: Kernel driver now active on interface %u: %s!\n", iface, libusb_kernel_driver_active(*dev_handle, iface) ? "yes" : "no");
        }
        libusb_free_config_descriptor(config);
    }
    libusb_close(*dev_handle);
    *dev_handle = NULL;
}

static int LIBUSB_CALL usb_hotplug(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event evt, void *data) {
    struct libusb_device_descriptor desc;
    uint16_t langid[2], manu[129], prod[129];
    int len;
    bool wasopen = usb.xfer->dev_handle;
    enum libusb_error err;
    switch (evt) {
        case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED:
            usb_close(&usb.xfer->dev_handle);
            usb.dev = dev;
            gui_console_printf("[USB] Device plugged.\n");
            break;
            if ((err = libusb_open(dev, &usb.xfer->dev_handle))) {
                gui_console_printf("[USB] Error: Open device failed: %s!\n", libusb_error_name(err));
            } else {
                usb_plug_a();
            }
            break;
            if (!libusb_get_device_descriptor(dev, &desc) &&
                !libusb_open(dev, &usb.xfer->dev_handle)) {
                len = libusb_get_string_descriptor(usb.xfer->dev_handle, 0, 0, (unsigned char *)langid, sizeof langid);
                if (len < (int)sizeof langid) break;
                len = libusb_get_string_descriptor(usb.xfer->dev_handle, desc.iManufacturer, langid[1], (unsigned char *)manu, sizeof manu - sizeof *manu);
                if (len < (int)sizeof *manu) break;
                manu[len >> 1] = 0;
                len = libusb_get_string_descriptor(usb.xfer->dev_handle, desc.iProduct, langid[1], (unsigned char *)prod, sizeof prod - sizeof *prod);
                if (len < (int)sizeof *prod) break;
                prod[len >> 1] = 0;
                gui_console_printf("[USB] %ls %ls connected.\n", &manu[1], &prod[1]);
                if (!wasopen) {
                    libusb_reset_device(usb.xfer->dev_handle);
                }
            }
            break;
        case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT:
            if (dev == usb.dev) {
                usb_unplug_a();
                usb_close(&usb.xfer->dev_handle);
                usb.dev = NULL;
                gui_console_printf("[USB] Device unplugged.\n");
            }
            break;
        default:
            gui_console_printf("Unhandled hotplug event: %d\n", evt);
            break;
    }
    return false;
}

static void init_libusb(void) {
    if (libusb_init(&usb.ctx)) {
        return;
    }
    if (!(usb.xfer = libusb_alloc_transfer(3)) || !(usb.xfer->buffer = malloc(0x5008))) {
        usb_free();
        return;
    }
    usb.xfer->callback = usb_process_xfer;
    //libusb_set_option(usb.ctx, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_INFO);
    libusb_hotplug_register_callback(usb.ctx, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, LIBUSB_HOTPLUG_ENUMERATE, 0x0951, 0x1666, 0, usb_hotplug, NULL, NULL);
}
#endif

static const eZ80portrange_t device = {
    .read  = usb_read,
    .write = usb_write
};

eZ80portrange_t init_usb(void) {
    memset(&usb, 0, sizeof usb);
    usb_init_hccr();
    usb_reset();
    gui_console_printf("[CEmu] Initialized USB...\n");
#ifdef HAS_LIBUSB
    init_libusb();
#endif
    return device;
}

void usb_free(void) {
#ifdef HAS_LIBUSB
    if (usb.xfer) {
        usb_close(&usb.xfer->dev_handle);
        free(usb.xfer->buffer);
        libusb_free_transfer(usb.xfer);
        usb.xfer = NULL;
    }
    if (usb.ctx) {
        libusb_exit(usb.ctx);
        usb.ctx = NULL;
    }
#endif
}

bool usb_save(FILE *image) {
    return fwrite(&usb, sizeof(usb), 1, image) == 1;
}

bool usb_restore(FILE *image) {
    bool success = fread(&usb, sizeof(usb), 1, image) == 1;
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
    usb.data                        = NULL;
    usb.len                         = 0;
    usb.xfer                        = NULL;
#ifdef HAS_LIBUSB
    usb.ctx                         = NULL;
    usb.dev                         = NULL;
    init_libusb();
#endif
    return success;
}
