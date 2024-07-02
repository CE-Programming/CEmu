#ifndef H_USB_USB
#define H_USB_USB

#include "device.h"
#include "../port.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "fotg210.h"

typedef struct usb_state {
    union {
        struct fotg210_regs regs;
        uint32_t            regWords[sizeof(struct fotg210_regs) >> 2];
        uint8_t             regBytes[sizeof(struct fotg210_regs)];
    };
    uint8_t ep0_data[8]; /* 0x1d0: EP0 Setup Packet PIO Register */
    uint8_t ep0_idx;
    uint8_t fifo_data[4][1024], cxfifo_data[64];
    usb_event_t event;
    usb_device_t *device;
} usb_state_t;

extern usb_state_t usb;

#ifdef __cplusplus
extern "C" {
#endif

eZ80portrange_t init_usb(void);
void usb_reset(void);
bool usb_restore(FILE *image);
bool usb_save(FILE *image);

void usb_host_int(uint8_t);
void usb_otg_int(uint16_t);
void usb_grp0_int(uint8_t);
void usb_grp1_int(uint32_t);
void usb_grp2_int(uint16_t);
uint8_t usb_status(void);

int usb_init_device(int argc, const char *const *argv,
                    usb_progress_handler_t *progress_handler, void *progress_context);

#ifdef __cplusplus
}
#endif

#endif
