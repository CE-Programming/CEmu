#ifndef H_USB_DEVICE
#define H_USB_DEVICE

#include <stdbool.h>
#include <stdint.h>

typedef enum usb_event_type {
    USB_INIT_EVENT,
    USB_RESET_EVENT,
    USB_TRANSFER_EVENT,
    USB_TIMER_EVENT,
    USB_DESTROY_EVENT,
} usb_event_type_t;

typedef enum usb_transfer_type {
    USB_CONTROL_TRANSFER,
    USB_ISOCHRONOUS_TRANSFER,
    USB_BULK_TRANSFER,
    USB_INTERRUPT_TRANSFER,
    USB_SETUP_TRANSFER,
} usb_transfer_type_t;

typedef struct usb_init_info {
    int argc;
    const char *const *argv;
} usb_init_info_t;

typedef struct usb_transfer_info {
    uint8_t *buffer;
    uint16_t length, max_pkt_size;
    uint8_t endpoint : 4, : 2;
    bool setup : 1, direction : 1;
} usb_transfer_info_t;

typedef enum usb_timer_mode {
    USB_TIMER_NONE,
    USB_TIMER_ABSOLUTE_MODE,
    USB_TIMER_RELATIVE_MODE,
} usb_timer_mode_t;

typedef struct usb_timer_info {
    usb_timer_mode_t mode;
    uint32_t useconds;
} usb_timer_info_t;

typedef struct usb_event {
    void *context;
    usb_event_type_t type;
    union {
        usb_init_info_t init;
        usb_transfer_info_t transfer;
        usb_timer_info_t timer;
    } info;
} usb_event_t;

typedef int usb_device_t(usb_event_t *event);

#ifdef __cplusplus
extern "C" {
#endif

usb_device_t usb_disconnected_device, usb_dusb_device;

#ifdef __cplusplus
}
#endif

#endif
