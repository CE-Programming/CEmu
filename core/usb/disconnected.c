#include "device.h"

int usb_disconnected_device(usb_event_t *event) {
    event->type = USB_INIT_EVENT;
    return 0;
}
