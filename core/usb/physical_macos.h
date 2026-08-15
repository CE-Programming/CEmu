#ifndef CEMU_USB_PHYSICAL_MACOS_H
#define CEMU_USB_PHYSICAL_MACOS_H

#include <stddef.h>
#include <stdint.h>

typedef struct physical_hid_device physical_hid_device_t;

typedef enum physical_hid_open_result {
    PHYSICAL_HID_OPEN_SUCCESS,
    PHYSICAL_HID_OPEN_NOT_FOUND,
    PHYSICAL_HID_OPEN_AMBIGUOUS,
    PHYSICAL_HID_OPEN_PERMISSION_DENIED,
    PHYSICAL_HID_OPEN_FAILED,
} physical_hid_open_result_t;

typedef enum physical_hid_read_result {
    PHYSICAL_HID_READ_PENDING,
    PHYSICAL_HID_READ_COMPLETED,
    PHYSICAL_HID_READ_DISCONNECTED,
} physical_hid_read_result_t;

physical_hid_open_result_t physical_hid_open(uint16_t vendor_id, uint16_t product_id, uint8_t interface_number, physical_hid_device_t **result);
void physical_hid_close(physical_hid_device_t *device);
physical_hid_read_result_t physical_hid_read(physical_hid_device_t *device, uint8_t *buffer, size_t capacity, size_t *length);

#endif
