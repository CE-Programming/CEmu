#include "device.h"

int usb_disconnected_device(usb_event_t *event) {
    event->type = USB_NO_EVENT;
    return USB_SUCCESS;
}
