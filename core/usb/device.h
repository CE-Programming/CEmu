#ifndef H_USB_DEVICE
#define H_USB_DEVICE

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

enum { USB_SUCCESS = 0 };

typedef enum usb_event_type {
    USB_NO_EVENT,
    USB_INIT_EVENT,
    USB_POWER_EVENT,
    USB_RESET_EVENT,
    USB_TRANSFER_REQUEST_EVENT,
    USB_TRANSFER_RESPONSE_EVENT,
    USB_TIMER_EVENT,
    USB_DESTROY_EVENT,
} usb_event_type_t;

typedef struct usb_init_info {
    int argc;
    const char *const *argv;
} usb_init_info_t;

typedef enum usb_speed {
    USB_FULL_SPEED,
    USB_LOW_SPEED,
    USB_HIGH_SPEED,
    USB_SUPER_SPEED,
} usb_speed_t;

typedef enum usb_transfer_type {
    USB_CONTROL_TRANSFER,
    USB_ISOCHRONOUS_TRANSFER,
    USB_BULK_TRANSFER,
    USB_INTERRUPT_TRANSFER,
    USB_SETUP_TRANSFER,
} usb_transfer_type_t;

typedef enum usb_transfer_status {
    USB_TRANSFER_COMPLETED,
    USB_TRANSFER_STALLED,
    USB_TRANSFER_OVERFLOWED,
    USB_TRANSFER_BABBLED,
    USB_TRANSFER_ERRORED,
} usb_transfer_status_t;

typedef struct usb_transfer_info {
    uint8_t *buffer;
    uint16_t length, max_pkt_size;
    usb_transfer_status_t status : 8;
    uint8_t address : 7, : 1, endpoint : 4;
    usb_transfer_type_t type : 3;
    bool direction : 1;
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

typedef bool usb_progress_handler_t(void *context, int value, int total);

typedef struct usb_event {
    usb_progress_handler_t *progress_handler;
    void *progress_context, *context;
    bool host : 1;
    usb_speed_t speed : 2;
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

usb_device_t usb_disconnected_device, usb_dusb_device, usb_physical_device, usb_msd_device;

#ifdef __cplusplus
}
#endif

#endif
