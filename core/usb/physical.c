#include "device.h"

#include "../emu.h"

#ifdef LIBUSB_SUPPORT

#include <libusb.h>

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PORT_DEPTH 7

#define NODE_EMPTY(head) \
    ((head)->next == (head))
#define NODE_ITEM(type, item)     \
    ((type *)((uint8_t *)(item) - \
              offsetof(type, node)))
#define NODE_FIRST(type, head) \
    (NODE_EMPTY(head) ? NULL : NODE_ITEM(type, (head)->next))
#define NODE_FOREACH(current, head)                 \
    for (node_t *next = (head)->next;               \
         ((current) = next != (head)                \
              ? NODE_ITEM(typeof(*(current)), next) \
              : NULL), next = next->next, (current); )

typedef struct node node_t;
typedef struct transfer transfer_t;
typedef struct endpoint endpoint_t;
typedef struct device device_t;
typedef struct context context_t;

struct node {
    node_t *prev, *next;
};

enum transfer_state {
    TRANSFER_STATE_SUBMITTED, // must be 0
    TRANSFER_STATE_COMPLETED,
    TRANSFER_STATE_PENDING,
    TRANSFER_STATE_NONE,
};

struct transfer {
    struct libusb_transfer *transfer;
    int state, alloc_length;
};

struct endpoint {
    transfer_t transfer;
};

struct device {
    node_t node;
    struct libusb_device_handle *handle;
    endpoint_t endpoints[0x20];
    uint8_t address : 7;
    enum device_state {
        DEVICE_STATE_ATTACHED,
        DEVICE_STATE_POWERED,
        DEVICE_STATE_DEFAULT_OR_ADDRESS,
        DEVICE_STATE_CONFIGURED,
    } state : 2;
};

struct context {
    libusb_context *context;
    node_t devices;
    bool events_handled : 1;
};

static void node_init(node_t *node) {
    node->prev = node->next = node;
}

static void node_remove(node_t *node) {
    node->prev->next = node->next;
    node->next->prev = node->prev;
    node->prev = node->next = node;
}

static void node_add(node_t *list, node_t *node) {
    node_remove(node);
    (node->prev = list->prev)->next = node;
    (node->next = list)->prev = node;
}

static int errno_from_libusb_error(ssize_t error) {
    if (error >= LIBUSB_SUCCESS) {
        return USB_SUCCESS;
    }
    switch (error) {
        case LIBUSB_ERROR_IO:            return EIO;
        case LIBUSB_ERROR_INVALID_PARAM: return EINVAL;
        case LIBUSB_ERROR_ACCESS:        return EACCES;
        case LIBUSB_ERROR_NO_DEVICE:     return ENODEV;
        case LIBUSB_ERROR_NOT_FOUND:     return ENOENT;
        case LIBUSB_ERROR_BUSY:          return EBUSY;
        case LIBUSB_ERROR_TIMEOUT:       return ETIMEDOUT;
        case LIBUSB_ERROR_OVERFLOW:      return ENOBUFS;
        case LIBUSB_ERROR_PIPE:          return EPIPE;
        case LIBUSB_ERROR_INTERRUPTED:   return EINTR;
        case LIBUSB_ERROR_NO_MEM:        return ENOMEM;
        case LIBUSB_ERROR_NOT_SUPPORTED: return ENOTSUP;
        default:
        case LIBUSB_ERROR_OTHER:         return EPERM;
    }
}

static enum libusb_transfer_status transfer_status_from_libusb_error(enum libusb_error error) {
    if (error >= LIBUSB_SUCCESS) {
        return LIBUSB_TRANSFER_COMPLETED;
    }
    switch (error) {
        default:
        case LIBUSB_ERROR_IO:
        case LIBUSB_ERROR_INVALID_PARAM:
        case LIBUSB_ERROR_ACCESS:
        case LIBUSB_ERROR_BUSY:
        case LIBUSB_ERROR_PIPE:
        case LIBUSB_ERROR_INTERRUPTED:
        case LIBUSB_ERROR_NO_MEM:
        case LIBUSB_ERROR_NOT_SUPPORTED:
        case LIBUSB_ERROR_OTHER:         return LIBUSB_TRANSFER_ERROR;
        case LIBUSB_ERROR_TIMEOUT:       return LIBUSB_TRANSFER_TIMED_OUT;
        case LIBUSB_ERROR_NOT_FOUND:     return LIBUSB_TRANSFER_STALL;
        case LIBUSB_ERROR_NO_DEVICE:     return LIBUSB_TRANSFER_NO_DEVICE;
        case LIBUSB_ERROR_OVERFLOW:      return LIBUSB_TRANSFER_OVERFLOW;
    }
}

static usb_transfer_status_t transfer_status_from_libusb_status(enum libusb_transfer_status status) {
    switch (status) {
        case LIBUSB_TRANSFER_COMPLETED: return USB_TRANSFER_COMPLETED;
        default:
        case LIBUSB_TRANSFER_ERROR:
        case LIBUSB_TRANSFER_TIMED_OUT:
        case LIBUSB_TRANSFER_CANCELLED:
        case LIBUSB_TRANSFER_NO_DEVICE: return USB_TRANSFER_ERRORED;
        case LIBUSB_TRANSFER_STALL:     return USB_TRANSFER_STALLED;
        case LIBUSB_TRANSFER_OVERFLOW:  return USB_TRANSFER_OVERFLOWED;
    }
}

static void transfer_init(transfer_t *transfer) {
    transfer->transfer = NULL;
    transfer->state = TRANSFER_STATE_NONE;
    transfer->alloc_length = 0;
}

static void endpoint_init(endpoint_t *endpoint) {
    transfer_init(&endpoint->transfer);
}

static int device_attach(context_t *context, struct libusb_device *libusb_device) {
    device_t *device = malloc(sizeof(device_t));
    if (!device) {
        return ENOMEM;
    }
    node_init(&device->node);
    int error = errno_from_libusb_error(libusb_open(libusb_device, &device->handle));
    if (error != USB_SUCCESS) {
        free(device);
        device = NULL;
        return error;
    }
    for (uint8_t index = 0; index != 0x20; ++index) {
        endpoint_init(&device->endpoints[index]);
    }
    device->address = 0;
    device->state = DEVICE_STATE_ATTACHED;
    node_add(&context->devices, &device->node);
    return error;
}

static void LIBUSB_CALL transfer_completed(struct libusb_transfer *libusb_transfer) {
    transfer_t *transfer = libusb_transfer->user_data;
    transfer->state = TRANSFER_STATE_COMPLETED;
}

static void transfer_append(transfer_t *transfer, const void *src, uint32_t length) {
    struct libusb_transfer *libusb_transfer = transfer->transfer;
    uint8_t *dest = libusb_transfer->buffer;
    uint32_t remaining = libusb_transfer->length - libusb_transfer->actual_length;
    if (libusb_transfer->type == LIBUSB_TRANSFER_TYPE_CONTROL) {
        dest = libusb_control_transfer_get_data(libusb_transfer);
        remaining -= sizeof(struct libusb_control_setup);
    }
    dest += libusb_transfer->actual_length;
    if (length > remaining) {
        length = remaining;
    }
    memcpy(dest, src, length);
    libusb_transfer->actual_length += length;
}

static int device_cleanup_transfer(device_t *device, transfer_t *transfer) {
    int error = USB_SUCCESS;
    if (transfer->state == TRANSFER_STATE_NONE) {
        return error;
    }
    struct libusb_transfer *libusb_transfer = transfer->transfer;
    switch (libusb_transfer->type) {
        case LIBUSB_TRANSFER_TYPE_CONTROL: {
            struct libusb_control_setup *setup = libusb_control_transfer_get_setup(libusb_transfer);
            switch (setup->bRequest) {
                case LIBUSB_REQUEST_SET_ADDRESS:
                    if (setup->bmRequestType == (LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_STANDARD | LIBUSB_RECIPIENT_DEVICE) && setup->wValue && setup->wValue < 0x80 && !setup->wIndex && !setup->wLength) {
                        device->address = setup->wValue;
                    }
                    break;
            }
            break;
        }
    }
    transfer->state = TRANSFER_STATE_NONE;
    return error;
}

static int device_intercept_transfer(device_t *device, transfer_t *transfer) {
    enum libusb_error error;
    struct libusb_transfer *libusb_transfer = transfer->transfer;
    if (libusb_transfer->type != LIBUSB_TRANSFER_TYPE_CONTROL) {
        return USB_SUCCESS;
    }
    struct libusb_device_handle *handle = libusb_transfer->dev_handle;
    enum libusb_transfer_status *status = &libusb_transfer->status;
    struct libusb_device_descriptor device_desc;
    struct libusb_config_descriptor *config_desc = NULL;
    struct libusb_control_setup *setup = libusb_control_transfer_get_setup(libusb_transfer);
    int configValueInt = 0;
    uint8_t type = setup->wValue >> 8, index = setup->wValue, iface, alt, endpt, configValueByte;
    *status = LIBUSB_TRANSFER_STALL;
    switch (setup->bRequest) {
        case LIBUSB_REQUEST_SET_ADDRESS:
            if (setup->bmRequestType == (LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_STANDARD | LIBUSB_RECIPIENT_DEVICE) && setup->wValue && setup->wValue < 0x80 && !setup->wIndex && !setup->wLength) {
                *status = LIBUSB_TRANSFER_COMPLETED;
            }
            break;
        case LIBUSB_REQUEST_GET_DESCRIPTOR:
            switch (type) {
                case LIBUSB_DT_DEVICE:
                    if (setup->bmRequestType == (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_STANDARD | LIBUSB_RECIPIENT_DEVICE) && !index && !setup->wIndex &&
                        (*status = transfer_status_from_libusb_error(libusb_get_device_descriptor(libusb_get_device(handle), &device_desc))) == LIBUSB_TRANSFER_COMPLETED) {
                        transfer_append(transfer, &device_desc, sizeof(struct libusb_device_descriptor));
                    }
                    break;
                case LIBUSB_DT_CONFIG:
                    if (setup->bmRequestType == (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_STANDARD | LIBUSB_RECIPIENT_DEVICE) && !setup->wIndex &&
                        (*status = transfer_status_from_libusb_error(libusb_get_config_descriptor(libusb_get_device(handle), index, &config_desc))) == LIBUSB_TRANSFER_COMPLETED) {
                        transfer_append(transfer, config_desc, config_desc->bLength);
                        transfer_append(transfer, config_desc->extra, config_desc->extra_length);
                        for (iface = 0; iface != config_desc->bNumInterfaces; ++iface) {
                            for (alt = 0; alt != config_desc->interface[iface].num_altsetting; ++alt) {
                                transfer_append(transfer, &config_desc->interface[iface].altsetting[alt], config_desc->interface[iface].altsetting[alt].bLength);
                                transfer_append(transfer, config_desc->interface[iface].altsetting[alt].extra, config_desc->interface[iface].altsetting[alt].extra_length);
                                for (endpt = 0; endpt != config_desc->interface[iface].altsetting[alt].bNumEndpoints; ++endpt) {
                                    transfer_append(transfer, &config_desc->interface[iface].altsetting[alt].endpoint[endpt], config_desc->interface[iface].altsetting[alt].endpoint[endpt].bLength);
                                    transfer_append(transfer, config_desc->interface[iface].altsetting[alt].endpoint[endpt].extra, config_desc->interface[iface].altsetting[alt].endpoint[endpt].extra_length);
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
                    return USB_SUCCESS;
            }
            break;
        case LIBUSB_REQUEST_SET_DESCRIPTOR:
            return USB_SUCCESS;
        case LIBUSB_REQUEST_GET_CONFIGURATION:
            if (device->state >= DEVICE_STATE_DEFAULT_OR_ADDRESS && (*status = device->state >= DEVICE_STATE_CONFIGURED ? transfer_status_from_libusb_error(libusb_get_configuration(handle, &configValueInt)) : LIBUSB_TRANSFER_COMPLETED) == LIBUSB_TRANSFER_COMPLETED) {
                configValueByte = configValueInt;
                transfer_append(transfer, &configValueByte, sizeof(configValueByte));
            }
            break;
        case LIBUSB_REQUEST_SET_CONFIGURATION:
            if (device->state >= DEVICE_STATE_DEFAULT_OR_ADDRESS && device->address && setup->bmRequestType == (LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_STANDARD | LIBUSB_RECIPIENT_DEVICE) && !type && !setup->wIndex) {
                if (libusb_get_active_config_descriptor(libusb_get_device(handle), &config_desc) == LIBUSB_SUCCESS) {
                    for (iface = 0; iface != config_desc->bNumInterfaces; ++iface) {
                        gui_console_printf("[USB] Info: Kernel driver was active on interface %u: %s!\n", iface, libusb_kernel_driver_active(handle, iface) ? "yes" : "no");
                        libusb_detach_kernel_driver(handle, iface);
                        libusb_release_interface(handle, iface);
                    }
                    libusb_free_config_descriptor(config_desc);
                    config_desc = NULL;
                }
                if ((*status = transfer_status_from_libusb_error(error = libusb_get_config_descriptor_by_value(libusb_get_device(handle), index, &config_desc))) == LIBUSB_TRANSFER_COMPLETED &&
                    (*status = transfer_status_from_libusb_error(error = libusb_set_configuration(handle, index))) == LIBUSB_TRANSFER_COMPLETED) {
                    device->state = DEVICE_STATE_CONFIGURED;
                    for (iface = 0; iface != config_desc->bNumInterfaces; ++iface) {
                        gui_console_printf("[USB] Info: Kernel driver now active on interface %u: %s!\n", iface, libusb_kernel_driver_active(handle, iface) ? "yes" : "no");
                        libusb_claim_interface(handle, iface);
                    }
                } else {
                    gui_console_printf("[USB] Error: Set configuration failed: %s!\n", libusb_error_name(error));
                }
            }
            break;
        case LIBUSB_REQUEST_GET_INTERFACE:
        case LIBUSB_REQUEST_SET_INTERFACE:
        case LIBUSB_REQUEST_SYNCH_FRAME:
        case LIBUSB_REQUEST_SET_SEL:
        case LIBUSB_SET_ISOCH_DELAY:
        default:
            return USB_SUCCESS;
    }
    libusb_free_config_descriptor(config_desc);
    libusb_transfer->callback(libusb_transfer);
    return USB_SUCCESS;
}

static int device_process_transfer(context_t *context, device_t *device, usb_event_t *event) {
    int error = USB_SUCCESS;
    usb_transfer_info_t *info = &event->info.transfer;
    if (!device) {
        event->type = USB_TRANSFER_RESPONSE_EVENT;
        info->status = USB_TRANSFER_ERRORED;
        return error;
    }
    uint8_t index = info->endpoint << 1;
    switch (info->type) {
        case USB_SETUP_TRANSFER:
        case USB_CONTROL_TRANSFER:
            break;
        case USB_BULK_TRANSFER:
        case USB_INTERRUPT_TRANSFER:
        case USB_ISOCHRONOUS_TRANSFER:
            index |= info->direction;
            break;
    }
    endpoint_t *endpoint = &device->endpoints[index];
    transfer_t *transfer = &endpoint->transfer;
    struct libusb_transfer *libusb_transfer = transfer->transfer;
    if (transfer->state == TRANSFER_STATE_SUBMITTED) {
        return error;
    }
    if (error == USB_SUCCESS && !libusb_transfer) {
        libusb_transfer = transfer->transfer = libusb_alloc_transfer(1);
        if (!libusb_transfer) {
            return ENOMEM;
        }
        libusb_transfer->flags = LIBUSB_TRANSFER_FREE_BUFFER;
        libusb_transfer->endpoint = index << 7 | index >> 1;
        libusb_transfer->dev_handle = device->handle;
        libusb_transfer->callback = transfer_completed;
        libusb_transfer->user_data = transfer;
    }
    if (error == USB_SUCCESS && transfer->state == TRANSFER_STATE_NONE) {
        uint32_t length = info->length;
        libusb_transfer->status = LIBUSB_TRANSFER_COMPLETED;
        libusb_transfer->length = info->length;
        libusb_transfer->actual_length = info->direction ? 0 : length;
        libusb_transfer->num_iso_packets = 0;
        switch (info->type) {
            default:
            case USB_CONTROL_TRANSFER:
                return USB_SUCCESS;
            case USB_SETUP_TRANSFER: {
                struct libusb_control_setup *setup = (struct libusb_control_setup *)info->buffer;
                if (info->direction || info->length != sizeof(struct libusb_control_setup)) {
                    return EINVAL;
                }
                if (!(setup->bmRequestType & 1 << 7) && setup->wLength) {
                    transfer->state = TRANSFER_STATE_PENDING;
                }
                libusb_transfer->type = LIBUSB_TRANSFER_TYPE_CONTROL;
                libusb_transfer->length += setup->wLength;
                libusb_transfer->actual_length = 0;
                break;
            }
            case USB_BULK_TRANSFER:
                libusb_transfer->type = LIBUSB_TRANSFER_TYPE_BULK;
                break;
            case USB_INTERRUPT_TRANSFER:
                libusb_transfer->type = LIBUSB_TRANSFER_TYPE_INTERRUPT;
                break;
            case USB_ISOCHRONOUS_TRANSFER:
                libusb_transfer->type = LIBUSB_TRANSFER_TYPE_ISOCHRONOUS;
                libusb_transfer->num_iso_packets = 1;
                libusb_set_iso_packet_lengths(libusb_transfer, libusb_transfer->length);
                break;
        }
        if (transfer->alloc_length < libusb_transfer->length) {
            free(libusb_transfer->buffer);
            libusb_transfer->buffer = malloc(libusb_transfer->length);
            if (!libusb_transfer->buffer) {
                transfer->alloc_length = 0;
                transfer->state = TRANSFER_STATE_NONE;
                return ENOMEM;
            }
            transfer->alloc_length = libusb_transfer->length;
        }
        memcpy(libusb_transfer->buffer, info->buffer, info->length);
    } else if (error == USB_SUCCESS && transfer->state == TRANSFER_STATE_PENDING) {
        if (info->type != USB_CONTROL_TRANSFER) {
            return EINVAL;
        }
        transfer_append(transfer, info->buffer, info->length);
    }
    if (error == USB_SUCCESS && transfer->state == TRANSFER_STATE_NONE) {
        error = device_intercept_transfer(device, transfer);
    }
    if (error == USB_SUCCESS && transfer->state == TRANSFER_STATE_NONE) {
        error = errno_from_libusb_error(libusb_submit_transfer(libusb_transfer));
        if (error == USB_SUCCESS) {
            transfer->state = TRANSFER_STATE_SUBMITTED;
        }
    }
    if (error == USB_SUCCESS && transfer->state == TRANSFER_STATE_SUBMITTED) {
        struct timeval tv = {
            .tv_sec = 0,
            .tv_usec = 10,
        };
        int error = errno_from_libusb_error(
                libusb_handle_events_timeout_completed(context->context, &tv, &transfer->state));
        if (error != USB_SUCCESS) {
            return error;
        }
    }
    if (error == USB_SUCCESS && transfer->state == TRANSFER_STATE_COMPLETED) {
        event->type = USB_TRANSFER_RESPONSE_EVENT;
        info->status = transfer_status_from_libusb_status(libusb_transfer->status);
        if (info->status != USB_TRANSFER_COMPLETED) {
            error = device_cleanup_transfer(device, transfer);
        }
        switch (info->type) {
            case USB_SETUP_TRANSFER:
                info->buffer = NULL;
                info->length = 0;
                break;
            case USB_CONTROL_TRANSFER:
                if (info->length) {
                    info->buffer = libusb_control_transfer_get_data(libusb_transfer);
                    info->length = libusb_transfer->actual_length;
                } else {
                    info->buffer = NULL;
                    error = device_cleanup_transfer(device, transfer);
                }
                break;
            case USB_BULK_TRANSFER:
            case USB_INTERRUPT_TRANSFER:
            case USB_ISOCHRONOUS_TRANSFER:
                info->buffer = libusb_transfer->buffer;
                info->length = libusb_transfer->actual_length;
                error = device_cleanup_transfer(device, transfer);
                break;
        }
    } else if (error == USB_SUCCESS && info->type == USB_SETUP_TRANSFER) {
        event->type = USB_TRANSFER_RESPONSE_EVENT;
        info->buffer = NULL;
        info->length = 0;
        info->status = USB_TRANSFER_COMPLETED;
    }
    return error;
}

static void transfer_cleanup(context_t *context, transfer_t *transfer) {
    if (transfer->transfer && transfer->state == TRANSFER_STATE_SUBMITTED) {
        int error = libusb_cancel_transfer(transfer->transfer);
        while (error == LIBUSB_SUCCESS && transfer->state == TRANSFER_STATE_SUBMITTED) {
            error = libusb_handle_events_completed(context->context, &transfer->state);
        }
    }
    transfer->alloc_length = 0;
    transfer->state = TRANSFER_STATE_NONE;
    libusb_free_transfer(transfer->transfer);
    transfer->transfer = NULL;
}

static void endpoint_cleanup(context_t *context, endpoint_t *endpoint) {
    transfer_cleanup(context, &endpoint->transfer);
}

static device_t *device_detach(context_t *context, device_t *device) {
    if (device) {
        for (uint8_t index = 0; index != 0x20; ++index) {
            endpoint_cleanup(context, &device->endpoints[index]);
        }
        libusb_close(device->handle);
        device->handle = NULL;
        node_remove(&device->node);
        free(device);
        device = NULL;
    }
    return device;
}

static int LIBUSB_CALL device_hotplugged(
        libusb_context *libusb_context, struct libusb_device *libusb_device,
        libusb_hotplug_event event, void *user_data) {
    context_t *context = user_data;
    if (context->context != libusb_context) {
        return true;
    }
    device_t *root = NODE_FIRST(device_t, &context->devices), *device;
    if (!root) {
        return true;
    }
    NODE_FOREACH(device, &context->devices) {
        if (libusb_get_device(device->handle) == libusb_device) {
            break;
        }
    }
    if (device) {
        if (event & LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT) {
            if (device == root) {
                NODE_FOREACH (device, &context->devices) {
                    root = device = device_detach(context, device);
                }
                return true;
            } else {
                device = device_detach(context, device);
            }
        }
    } else {
        if (event & LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED) {
            for (struct libusb_device *hub = libusb_device; hub; hub = libusb_get_parent(hub)) {
                if (hub == libusb_get_device(root->handle)) {
                    device_attach(context, libusb_device);
                    break;
                }
            }
        }
    }
    return false;
}

int usb_physical_device(usb_event_t *event) {
    int error = USB_SUCCESS;
    context_t *context = event->context;
    usb_event_type_t type = event->type;
    usb_init_info_t *init = &event->info.init;
    usb_transfer_info_t *transfer = &event->info.transfer;
    usb_timer_info_t *timer = &event->info.timer;
    device_t *device;
    event->host = false;
    event->speed = USB_FULL_SPEED;
    event->type = USB_INIT_EVENT;
    switch (type) {
        case USB_NO_EVENT:
            if (!context->events_handled) {
                struct timeval tv = {
                    .tv_sec = 0,
                    .tv_usec = 0,
                };
                error = errno_from_libusb_error(
                        libusb_handle_events_timeout(context->context, &tv));
                event->type = USB_TIMER_EVENT;
                timer->mode = USB_TIMER_ABSOLUTE_MODE;
                timer->useconds = 1000;
                if (libusb_get_next_timeout(context->context, &tv) == 1) {
                    int64_t useconds = tv.tv_sec * INT64_C(1000000) + tv.tv_usec;
                    if (useconds < timer->useconds) {
                        useconds = timer->useconds;
                    }
                }
                context->events_handled = true;
                return error;
            }
            event->type = USB_NO_EVENT;
            break;
        case USB_INIT_EVENT:
            event->context = NULL;
            if (init->argc != 2) {
                return EINVAL;
            }
            context = event->context = malloc(sizeof(context_t));
            if (!context) {
                return ENOMEM;
            }
            node_init(&context->devices);
            error = errno_from_libusb_error(libusb_init(&context->context));
            if (error != USB_SUCCESS) {
                return error;
            }
            if (libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG)) {
                libusb_hotplug_register_callback(
                        context->context,
                        LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED |
                        LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT,
                        0,
                        LIBUSB_HOTPLUG_MATCH_ANY,
                        LIBUSB_HOTPLUG_MATCH_ANY,
                        LIBUSB_HOTPLUG_MATCH_ANY,
                        device_hotplugged, context, NULL);
            }
            {
                int i, end;
                uint16_t vid, pid;
                uint8_t bus, addr;
                libusb_device **devices, *root = NULL;
                error = errno_from_libusb_error(
                        libusb_get_device_list(context->context, &devices));
                if (error != USB_SUCCESS) {
                    break;
                }
                error = ENODEV;
                if (sscanf(init->argv[1], "%4" SCNx16 " :%4" SCNx16 " %n", &vid, &pid, &end) == 2 &&
                    !init->argv[1][end]) {
                    for (libusb_device **device = devices; *device; ++device) {
                        struct libusb_device_descriptor descriptor;
                        if (libusb_get_device_descriptor(*device, &descriptor) == LIBUSB_SUCCESS &&
                            descriptor.idVendor == vid && descriptor.idProduct == pid) {
                            if (root) {
                                root = NULL;
                                break;
                            }
                            root = *device;
                        }
                    }
                } else if (sscanf(init->argv[1], "%" SCNu8 " #%" SCNu8 " %n", &bus, &addr, &end) == 2 &&
                           !init->argv[1][end]) {
                    for (libusb_device **device = devices; *device; ++device) {
                        if (libusb_get_bus_number(*device) == bus &&
                            libusb_get_device_address(*device) == addr) {
                            if (root) {
                                root = NULL;
                                break;
                            }
                            root = *device;
                        }
                    }
                } else if (sscanf(init->argv[1], "%" SCNu8 " %n", &bus, &end) == 1) {
                    uint8_t ports[MAX_PORT_DEPTH], num_ports = 0;
                    for (char format[] = "-%" SCNu8 " %n";
                         i = end, num_ports != MAX_PORT_DEPTH &&
                             sscanf(&init->argv[1][i], format, &ports[num_ports], &end) == 1;
                         format[0] = '.') {
                    }
                    if (init->argv[1][end]) {
                        error = EINVAL;
                    } else {
                        for (libusb_device **device = devices; *device; ++device) {
                            uint8_t device_ports[MAX_PORT_DEPTH];
                            if (libusb_get_port_numbers(*device, device_ports,
                                                        MAX_PORT_DEPTH) == num_ports &&
                                !memcmp(device_ports, ports, num_ports * sizeof(*ports))) {
                                if (root) {
                                    root = NULL;
                                    break;
                                }
                                root = *device;
                            }
                        }
                    }
                } else {
                    error = EINVAL;
                }
                if (root) {
                    error = device_attach(context, root);
                    if (error == USB_SUCCESS) {
                        for (libusb_device **device = devices; *device; ++device) {
                            device_hotplugged(context->context, *device,
                                              LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, context);
                        }
                    }
                }
                root = NULL;
                libusb_free_device_list(devices, true);
                devices = NULL;
            }
            break;
        case USB_POWER_EVENT:
        case USB_RESET_EVENT:
            device = NODE_FIRST(device_t, &context->devices);
            if (!device) {
                break;
            }
            if (type == USB_POWER_EVENT) {
                device->address = 0;
                device->state = DEVICE_STATE_POWERED;
            } else if (device->state >= DEVICE_STATE_POWERED && type == USB_RESET_EVENT) {
                error = errno_from_libusb_error(libusb_reset_device(device->handle));
                if (error == USB_SUCCESS) {
                    device->address = 0;
                    device->state = DEVICE_STATE_DEFAULT_OR_ADDRESS;
                }
            }
            break;
        case USB_TRANSFER_REQUEST_EVENT:
            NODE_FOREACH (device, &context->devices) {
                if (device->state <= DEVICE_STATE_POWERED ||
                    device->address != transfer->address) {
                    continue;
                }
                error = device_process_transfer(context, device, event);
                break;
            }
            break;
        case USB_TIMER_EVENT:
            break;
        case USB_DESTROY_EVENT:
            if (event->progress_handler) {
                event->progress_handler(event->progress_context, 0, 0);
                event->progress_handler = NULL;
            }
            if (!context) {
                return 0;
            }
            NODE_FOREACH (device, &context->devices) {
                device_detach(context, device);
            }
            if (context->context) {
                libusb_exit(context->context);
                context->context = NULL;
            }
            free(context);
            context = event->context = NULL;
            return 0;
        default:
            error = EINVAL;
            break;
    }
    context->events_handled = false;
    return error;
}

#else

int usb_physical_device(usb_event_t *event) {
    (void)event;
    return ENOSYS;
}

#endif
