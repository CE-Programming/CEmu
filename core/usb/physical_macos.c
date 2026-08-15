#include "physical_macos.h"

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/hid/IOHIDKeys.h>
#include <IOKit/hid/IOHIDManager.h>
#include <IOKit/hidsystem/IOHIDLib.h>

#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#ifdef CEMU_USB_TRACE
#include <stdio.h>
#define HID_TRACE(...) fprintf(stderr, "[USBHID] " __VA_ARGS__)
#else
#define HID_TRACE(...) ((void)0)
#endif

#define MAX_QUEUED_HID_REPORTS 32
#define MAX_HID_REPORT_LENGTH 65536

typedef struct physical_hid_report physical_hid_report_t;

struct physical_hid_report {
    physical_hid_report_t *next;
    size_t length;
    uint8_t data[];
};

struct physical_hid_device {
    IOHIDDeviceRef device;
    CFStringRef run_loop_mode;
    CFRunLoopRef run_loop;
    pthread_t thread;
    pthread_mutex_t mutex;
    pthread_cond_t condition;
    physical_hid_report_t *reports, **reports_tail;
    uint8_t *input_buffer;
    CFIndex input_buffer_length;
    size_t report_count;
    bool mutex_initialized, condition_initialized;
    bool device_opened, thread_started, thread_ready, shutdown, disconnected;
};

static long hid_number_property(IOHIDDeviceRef device, CFStringRef key) {
    CFTypeRef value = IOHIDDeviceGetProperty(device, key);
    long number = -1;
    if (value && CFGetTypeID(value) == CFNumberGetTypeID()) {
        CFNumberGetValue(value, kCFNumberLongType, &number);
    }
    return number;
}

static long hid_interface_number(IOHIDDeviceRef device) {
    io_registry_entry_t entry = IOHIDDeviceGetService(device);
    bool entry_owned = false;
    while (entry) {
        CFTypeRef value = IORegistryEntryCreateCFProperty(entry, CFSTR("bInterfaceNumber"), kCFAllocatorDefault, 0);
        long interface_number = -1;
        if (value && CFGetTypeID(value) == CFNumberGetTypeID()) {
            CFNumberGetValue(value, kCFNumberLongType, &interface_number);
        }
        if (value) {
            CFRelease(value);
        }
        if (interface_number >= 0) {
            if (entry_owned) {
                IOObjectRelease(entry);
            }
            return interface_number;
        }
        io_registry_entry_t parent = IO_OBJECT_NULL;
        kern_return_t result = IORegistryEntryGetParentEntry(entry, kIOServicePlane, &parent);
        if (entry_owned) {
            IOObjectRelease(entry);
        }
        if (result != KERN_SUCCESS) {
            break;
        }
        entry = parent;
        entry_owned = true;
    }
    return -1;
}

static void hid_discard_first_report(physical_hid_device_t *device) {
    physical_hid_report_t *report = device->reports;
    if (!report) {
        return;
    }
    device->reports = report->next;
    if (!device->reports) {
        device->reports_tail = &device->reports;
    }
    --device->report_count;
    free(report);
}

static void hid_report_callback(void *context, IOReturn result, void *sender,
                                IOHIDReportType type, uint32_t report_id,
                                uint8_t *report, CFIndex report_length) {
    (void)sender;
    (void)report_id;
    if (result != kIOReturnSuccess || type != kIOHIDReportTypeInput || report_length <= 0) {
        return;
    }
    physical_hid_device_t *device = context;
    physical_hid_report_t *queued =
        malloc(sizeof(*queued) + (size_t)report_length);
    if (!queued) {
        return;
    }
    queued->next = NULL;
    queued->length = (size_t)report_length;
    memcpy(queued->data, report, queued->length);
    HID_TRACE("input report length=%zu\n", queued->length);

    pthread_mutex_lock(&device->mutex);
    if (device->shutdown || device->disconnected) {
        pthread_mutex_unlock(&device->mutex);
        free(queued);
        return;
    }
    *device->reports_tail = queued;
    device->reports_tail = &queued->next;
    if (++device->report_count > MAX_QUEUED_HID_REPORTS) {
        hid_discard_first_report(device);
    }
    pthread_mutex_unlock(&device->mutex);
}

static void hid_removal_callback(void *context, IOReturn result, void *sender) {
    (void)result;
    (void)sender;
    physical_hid_device_t *device = context;
    pthread_mutex_lock(&device->mutex);
    device->disconnected = true;
    CFRunLoopRef run_loop = device->run_loop;
    if (run_loop) {
        CFRetain(run_loop);
    }
    pthread_mutex_unlock(&device->mutex);
    if (run_loop) {
        CFRunLoopStop(run_loop);
        CFRunLoopWakeUp(run_loop);
        CFRelease(run_loop);
    }
}

static void *hid_run_loop(void *context) {
    physical_hid_device_t *device = context;
    CFRunLoopRef run_loop = CFRunLoopGetCurrent();
    CFRetain(run_loop);
    IOHIDDeviceScheduleWithRunLoop(device->device, run_loop, device->run_loop_mode);

    pthread_mutex_lock(&device->mutex);
    device->run_loop = run_loop;
    device->thread_ready = true;
    pthread_cond_signal(&device->condition);
    pthread_mutex_unlock(&device->mutex);

    while (true) {
        pthread_mutex_lock(&device->mutex);
        bool done = device->shutdown || device->disconnected;
        pthread_mutex_unlock(&device->mutex);
        if (done) {
            break;
        }
        SInt32 status = CFRunLoopRunInMode(device->run_loop_mode, 1, false);
        if (status == kCFRunLoopRunFinished || status == kCFRunLoopRunStopped) {
            pthread_mutex_lock(&device->mutex);
            done = device->shutdown || device->disconnected;
            pthread_mutex_unlock(&device->mutex);
            if (!done) {
                continue;
            }
            break;
        }
    }

    IOHIDDeviceUnscheduleFromRunLoop(device->device, run_loop, device->run_loop_mode);
    pthread_mutex_lock(&device->mutex);
    device->run_loop = NULL;
    pthread_mutex_unlock(&device->mutex);
    CFRelease(run_loop);
    return NULL;
}

static CFMutableDictionaryRef hid_matching_dictionary(uint16_t vendor_id, uint16_t product_id) {
    CFMutableDictionaryRef matching = CFDictionaryCreateMutable(
        kCFAllocatorDefault, 2, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    if (!matching) {
        return NULL;
    }
    int vendor = vendor_id, product = product_id;
    CFNumberRef vendor_number = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &vendor);
    CFNumberRef product_number = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &product);
    if (!vendor_number || !product_number) {
        if (vendor_number) {
            CFRelease(vendor_number);
        }
        if (product_number) {
            CFRelease(product_number);
        }
        CFRelease(matching);
        return NULL;
    }
    CFDictionarySetValue(matching, CFSTR(kIOHIDVendorIDKey), vendor_number);
    CFDictionarySetValue(matching, CFSTR(kIOHIDProductIDKey), product_number);
    CFRelease(vendor_number);
    CFRelease(product_number);
    return matching;
}

physical_hid_open_result_t physical_hid_open(uint16_t vendor_id,
                                             uint16_t product_id,
                                             uint8_t interface_number,
                                             physical_hid_device_t **result) {
    *result = NULL;
    if (IOHIDCheckAccess(kIOHIDRequestTypeListenEvent) != kIOHIDAccessTypeGranted
        && !IOHIDRequestAccess(kIOHIDRequestTypeListenEvent)) {
        return PHYSICAL_HID_OPEN_PERMISSION_DENIED;
    }

    IOHIDManagerRef manager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
    CFMutableDictionaryRef matching = hid_matching_dictionary(vendor_id, product_id);
    if (!manager || !matching) {
        if (manager) {
            CFRelease(manager);
        }
        if (matching) {
            CFRelease(matching);
        }
        return PHYSICAL_HID_OPEN_FAILED;
    }
    IOHIDManagerSetDeviceMatching(manager, matching);
    CFRelease(matching);
    CFSetRef devices = IOHIDManagerCopyDevices(manager);
    CFRelease(manager);
    if (!devices || !CFSetGetCount(devices)) {
        if (devices) {
            CFRelease(devices);
        }
        return PHYSICAL_HID_OPEN_NOT_FOUND;
    }
    CFIndex device_count = CFSetGetCount(devices);
    IOHIDDeviceRef *device_values = calloc((size_t)device_count, sizeof(*device_values));
    if (!device_values) {
        CFRelease(devices);
        return PHYSICAL_HID_OPEN_FAILED;
    }
    CFSetGetValues(devices, (const void **)device_values);
    IOHIDDeviceRef hid_device = NULL;
    for (CFIndex index = 0; index < device_count; ++index) {
        if (hid_interface_number(device_values[index]) != interface_number) {
            continue;
        }
        if (hid_device) {
            free(device_values);
            CFRelease(devices);
            return PHYSICAL_HID_OPEN_AMBIGUOUS;
        }
        hid_device = device_values[index];
    }
    if (!hid_device) {
        free(device_values);
        CFRelease(devices);
        return PHYSICAL_HID_OPEN_NOT_FOUND;
    }
    CFRetain(hid_device);
    free(device_values);
    CFRelease(devices);

    physical_hid_device_t *device = calloc(1, sizeof(*device));
    if (!device) {
        CFRelease(hid_device);
        return PHYSICAL_HID_OPEN_FAILED;
    }
    device->device = hid_device;
    device->reports_tail = &device->reports;
    device->input_buffer_length = hid_number_property(hid_device, CFSTR(kIOHIDMaxInputReportSizeKey));
    if (device->input_buffer_length <= 0 || device->input_buffer_length > MAX_HID_REPORT_LENGTH) {
        physical_hid_close(device);
        return PHYSICAL_HID_OPEN_FAILED;
    }
    device->input_buffer = calloc((size_t)device->input_buffer_length, 1);
    device->run_loop_mode = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("CEmuHID_%p"), device);
    if (!device->input_buffer || !device->run_loop_mode) {
        physical_hid_close(device);
        return PHYSICAL_HID_OPEN_FAILED;
    }
    if (pthread_mutex_init(&device->mutex, NULL)) {
        physical_hid_close(device);
        return PHYSICAL_HID_OPEN_FAILED;
    }
    device->mutex_initialized = true;
    if (pthread_cond_init(&device->condition, NULL)) {
        physical_hid_close(device);
        return PHYSICAL_HID_OPEN_FAILED;
    }
    device->condition_initialized = true;

    IOReturn open_result = IOHIDDeviceOpen(hid_device, kIOHIDOptionsTypeNone);
    if (open_result != kIOReturnSuccess) {
        physical_hid_close(device);
        return open_result == kIOReturnNotPermitted
            ? PHYSICAL_HID_OPEN_PERMISSION_DENIED
            : PHYSICAL_HID_OPEN_FAILED;
    }
    device->device_opened = true;
    IOHIDDeviceRegisterInputReportCallback(
        hid_device, device->input_buffer, device->input_buffer_length,
        hid_report_callback, device);
    IOHIDDeviceRegisterRemovalCallback(hid_device, hid_removal_callback, device);
    if (pthread_create(&device->thread, NULL, hid_run_loop, device)) {
        physical_hid_close(device);
        return PHYSICAL_HID_OPEN_FAILED;
    }
    device->thread_started = true;
    pthread_mutex_lock(&device->mutex);
    while (!device->thread_ready) {
        pthread_cond_wait(&device->condition, &device->mutex);
    }
    pthread_mutex_unlock(&device->mutex);
    *result = device;
    HID_TRACE("opened %04x:%04x with input reports up to %ld bytes\n",
              vendor_id, product_id, (long)device->input_buffer_length);
    return PHYSICAL_HID_OPEN_SUCCESS;
}

void physical_hid_close(physical_hid_device_t *device) {
    if (!device) {
        return;
    }
    if (device->thread_started) {
        pthread_mutex_lock(&device->mutex);
        device->shutdown = true;
        CFRunLoopRef run_loop = device->run_loop;
        if (run_loop) {
            CFRetain(run_loop);
        }
        pthread_mutex_unlock(&device->mutex);
        if (run_loop) {
            CFRunLoopStop(run_loop);
            CFRunLoopWakeUp(run_loop);
            CFRelease(run_loop);
        }
        pthread_join(device->thread, NULL);
    }
    if (device->device_opened) {
        IOHIDDeviceClose(device->device, kIOHIDOptionsTypeNone);
    }
    if (device->device) {
        CFRelease(device->device);
    }
    if (device->run_loop_mode) {
        CFRelease(device->run_loop_mode);
    }
    while (device->reports) {
        hid_discard_first_report(device);
    }
    free(device->input_buffer);
    if (device->condition_initialized) {
        pthread_cond_destroy(&device->condition);
    }
    if (device->mutex_initialized) {
        pthread_mutex_destroy(&device->mutex);
    }
    free(device);
}

physical_hid_read_result_t physical_hid_read(physical_hid_device_t *device,
                                             uint8_t *buffer,
                                             size_t capacity,
                                             size_t *length) {
    *length = 0;
    pthread_mutex_lock(&device->mutex);
    if (device->disconnected) {
        pthread_mutex_unlock(&device->mutex);
        return PHYSICAL_HID_READ_DISCONNECTED;
    }
    physical_hid_report_t *report = device->reports;
    if (!report) {
        pthread_mutex_unlock(&device->mutex);
        return PHYSICAL_HID_READ_PENDING;
    }
    *length = report->length < capacity ? report->length : capacity;
    memcpy(buffer, report->data, *length);
    hid_discard_first_report(device);
    pthread_mutex_unlock(&device->mutex);
    return PHYSICAL_HID_READ_COMPLETED;
}
