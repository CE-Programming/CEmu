#ifndef H_USB
#define H_USB

#ifdef __cplusplus
extern "C" {
#endif

#include "../port.h"

#include <assert.h>
#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "fotg210.h"
#ifdef HAS_LIBUSB
#include <libusb.h>
#endif

typedef enum usb_qtype {
    QTYPE_ITD, QTYPE_QH, QTYPE_SITD, QTYPE_FSTN
} usb_qtype_t;
typedef union usb_qlink {
    uint32_t val;
    struct {
        bool term : 1;
        usb_qtype_t type : 2;
    };
    struct {
        bool : 1;
        uint8_t nak_cnt : 4; // overlay alt
        uint32_t ptr : 27;
    };
} usb_qlink_t;
typedef union usb_qbuf {
    uint32_t val;
    struct { uint32_t off : 12, ptr : 20; };        // index 0
    uint8_t c_prog_mask;                            // index 1
    struct { uint8_t frame_tag : 5, s_bytes : 7; }; // index 2
} usb_qbuf_t;
typedef struct usb_qtd {
    usb_qlink_t next, alt;
    bool ping : 1, split : 1, missed : 1, xact_err : 1, babble : 1, buf_err : 1, halted : 1, active : 1;
    uint16_t pid : 2, cerr : 2, c_page : 3, ioc : 1, total_bytes : 15, dt : 1;
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
static_assert(offsetof(usb_qtd_t, next) == 0x00, "Next qTD Pointer is not at offset 0x00");
static_assert(offsetof(usb_qtd_t, alt) == 0x04, "Alternate Next qTD Pointer is not at offset 0x04");
static_assert(offsetof(usb_qtd_t, bufs) == 0x0C, "Buffer Pointers are not at offset 0x0C");
static_assert(sizeof(usb_qtd_t) == 0x20, "usb_qtd_t does not have a size of 0x20");
static_assert(offsetof(usb_qh_t, horiz) == 0x00, "Queue Head Horizontal Link Pointer is not at offset 0x00");
static_assert(offsetof(usb_qh_t, cur) == 0x0C, "Current qTD Pointer is not at offset 0x0C");
static_assert(offsetof(usb_qh_t, overlay) == 0x10, "Transfer Overlay is not at offset 0x10");
static_assert(sizeof(usb_qh_t) == 0x30, "usb_qh_t does not have a size of 0x30");
/* Standard USB state */
typedef struct usb_state {
    struct fotg210_regs regs;
    uint8_t ep0_data[8]; /* 0x1d0: EP0 Setup Packet PIO Register */
    uint8_t ep0_idx;
    uint8_t state;
    uint8_t *data;
    uint8_t len;
    int delay;
#ifdef HAS_LIBUSB
    libusb_context *ctx;
    libusb_device *dev;
#endif
    struct libusb_transfer *xfer;
    bool wait : 1, process : 1, control : 1;
    enum  {
        USB_HC_STATE_PERIOD,
        USB_HC_STATE_ASYNC
    } hc_state : 1;
    enum {
          USB_NAK_CNT_WAIT_FOR_START_EVENT,
          USB_NAK_CNT_DO_RELOAD,
          USB_NAK_CNT_WAIT_FOR_LIST_HEAD,
    } nak_cnt_reload_state : 2;
    usb_qh_t *fake_recl_head;
    libusb_hotplug_callback_handle hotplug_handle;
} usb_state_t;

extern usb_state_t usb;

eZ80portrange_t init_usb(void);
void usb_reset_core(void);
void usb_reset_aux(void);
void usb_reset_otg(void);
void usb_reset(void);
bool usb_restore(FILE *image);
bool usb_save(FILE *image);
void usb_free(void);

void usb_host_int(uint8_t);
void usb_otg_int(uint16_t);
void usb_grp0_int(uint8_t);
void usb_grp1_int(uint32_t);
void usb_grp2_int(uint16_t);
uint8_t usb_status(void);

void usb_setup(const uint8_t *);
void usb_send_pkt(const void *, uint32_t);

/* api functions */
void emu_usb_detach(void);
void emu_usb_attach(int vid, int pid);

#ifdef __cplusplus
}
#endif

#endif
