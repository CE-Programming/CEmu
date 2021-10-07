#include "device.h"

#include "../os/os.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum scsi_command {
    SCSI_TEST_UNIT_READY   = 0x00,
    SCSI_REQUEST_SENSE     = 0x03,
    SCSI_INQUIRY           = 0x12,
    SCSI_READ_CAPACITY     = 0x25,
    SCSI_READ_10           = 0x28,
    SCSI_WRITE_10          = 0x2A,
    SCSI_SYNCHRONIZE_CACHE = 0x35,
    SCSI_MODE_SENSE        = 0xC0,
} scsi_command_t;

typedef enum msd_status {
    MSD_COMMAND_PASSED,
    MSD_COMMAND_FAILED,
    MSD_PHASE_ERROR,
} msd_status_t;

typedef struct context {
    FILE *image;
    int64_t blocks;
    uint32_t tag;
    uint32_t length;
    uint8_t address : 7;
    bool configured : 1;
    enum {
        COMMAND_STATE,
        IN_STATE,
        OUT_STATE,
        STATUS_STATE,
    } state : 4;
    union {
        scsi_command_t command;
        msd_status_t status;
    };
    uint8_t shift;
    uint8_t setup[8];
} context_t;

int usb_msd_device(usb_event_t *event) {
    int error = USB_SUCCESS;
    context_t *context = event->context;
    usb_event_type_t type = event->type;
    usb_init_info_t *init = &event->info.init;
    usb_transfer_info_t *transfer = &event->info.transfer;
    event->host = false;
    event->speed = USB_FULL_SPEED;
    event->type = USB_NO_EVENT;
    switch (type) {
        case USB_NO_EVENT:
            break;
        case USB_INIT_EVENT:
            event->context = NULL;
            if (init->argc != 2) {
                return EINVAL;
            }
            event->context = context = malloc(sizeof(context_t));
            if (!context) {
                return ENOMEM;
            }
            context->image = fopen_utf8(init->argv[1], "r+b");
            context->address = 0;
            context->configured = false;
            memset(context->setup, 0, sizeof(context->setup));
            context->shift = 9;
            context->state = COMMAND_STATE;
            if (!context->image ||
                fseek(context->image, 0, SEEK_END) ||
                (context->blocks = ftell(context->image) >> context->shift) <= 0) {
                return errno;
            }
            break;
        case USB_POWER_EVENT:
            break;
        case USB_RESET_EVENT:
            context->address = 0;
            context->configured = false;
            break;
        case USB_TRANSFER_REQUEST_EVENT:
            event->type = USB_TRANSFER_RESPONSE_EVENT;
            if (transfer->address != context->address ||
                transfer->status != USB_TRANSFER_COMPLETED) {
                transfer->buffer = NULL;
                transfer->length = 0;
                transfer->status = USB_TRANSFER_ERRORED;
                break;
            }
            switch (transfer->type) {
                case USB_SETUP_TRANSFER:
                    memcpy(context->setup, transfer->buffer, sizeof(context->setup));
                    transfer->buffer = NULL;
                    transfer->length = 0;
                    transfer->status = USB_TRANSFER_COMPLETED;
                    break;
                case USB_CONTROL_TRANSFER: {
                    uint8_t bmRequestType = context->setup[0];
                    uint8_t bRequest = context->setup[1];
                    uint16_t wValue =
                        (uint16_t)context->setup[2] << 0 |
                        (uint16_t)context->setup[3] << 8;
                    uint16_t wIndex =
                        (uint16_t)context->setup[4] << 0 |
                        (uint16_t)context->setup[5] << 8;
                    uint16_t wLength =
                        (uint16_t)context->setup[6] << 0 |
                        (uint16_t)context->setup[7] << 8;
                    if (transfer->length > wLength) {
                        transfer->length = wLength;
                    }
                    if ((transfer->length && !transfer->direction) ||
                        transfer->status != USB_TRANSFER_COMPLETED) {
                        transfer->buffer = NULL;
                        transfer->length = 0;
                        transfer->status = USB_TRANSFER_STALLED;
                    } else if (bmRequestType == 0x00 && bRequest == 0x05 && wValue < 0x0080 && wIndex == 0x0000) {
                        context->address = wValue;
                        transfer->buffer = NULL;
                        transfer->length = 0;
                        transfer->status = USB_TRANSFER_COMPLETED;
                    } else if (bmRequestType == 0x80 && bRequest == 0x06 && wValue == 0x0100 && wIndex == 0x0000) {
                        static const uint8_t device_desc[] = {
                            0x12, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40,
                                        0x3C, 0xCE, 0x01, 0x08, 0x00, 0x01,
                                        0x00, 0x00, 0x00, 0x01,
                        };
                        transfer->buffer = (uint8_t *)device_desc;
                        if (transfer->length > sizeof(device_desc)) {
                            transfer->length = sizeof(device_desc);
                        }
                        transfer->status = USB_TRANSFER_COMPLETED;
                    } else if (bmRequestType == 0x80 && bRequest == 0x06 && wValue == 0x0200 && wIndex == 0x0000) {
                        static const uint8_t config_desc[] = {
                            0x09, 0x02, 0x20, 0x00, 0x01, 0x01, 0x00, 0x80, 0x64,
                            0x09, 0x04, 0x00, 0x00, 0x02, 0x08, 0x06, 0x50, 0x00,
                            0x07, 0x05, 0x81, 0x02, 0x40, 0x00, 0x00,
                            0x07, 0x05, 0x02, 0x02, 0x40, 0x00, 0x00,
                        };
                        transfer->buffer = (uint8_t *)config_desc;
                        if (transfer->length > sizeof(config_desc)) {
                            transfer->length = sizeof(config_desc);
                        }
                        transfer->status = USB_TRANSFER_COMPLETED;
                    } else if (bmRequestType == 0x21 && bRequest == 0xFF && wValue == 0x0000 && wIndex == 0x0000) {
                        transfer->buffer = NULL;
                        transfer->length = 0;
                        transfer->status = USB_TRANSFER_COMPLETED;
                    } else if (bmRequestType == 0xA1 && bRequest == 0xFE && wValue == 0x0000 && wIndex == 0x0000) {
                        static const uint8_t max_lun = 0;
                        transfer->buffer = (uint8_t *)&max_lun;
                        if (transfer->length > sizeof(max_lun)) {
                            transfer->length = sizeof(max_lun);
                        }
                        transfer->status = USB_TRANSFER_COMPLETED;
                    } else if (context->address && bmRequestType == 0x00 && bRequest == 0x09 && (wValue | 0x0001) == 0x0001 && wIndex == 0x0000) {
                        context->configured = wValue;
                        transfer->buffer = NULL;
                        transfer->length = 0;
                        transfer->status = USB_TRANSFER_COMPLETED;
                    } else {
                        transfer->buffer = NULL;
                        transfer->length = 0;
                        transfer->status = USB_TRANSFER_STALLED;
                    }
                    break;
                }
                case USB_BULK_TRANSFER:
                    switch (context->state) {
                        case COMMAND_STATE:
                            if (memcmp(transfer->buffer, "USBC", 4) ||
                                (transfer->buffer[12] & 0x7F) != 0x00 ||
                                transfer->buffer[13] != 0x00 ||
                                transfer->buffer[14] == 0x00 ||
                                transfer->buffer[14] > 0x10) {
                                transfer->buffer = NULL;
                                transfer->length = 0;
                                transfer->status = USB_TRANSFER_COMPLETED;
                                break;
                            }
                            context->tag =
                                (uint32_t)transfer->buffer[ 4] <<  0 |
                                (uint32_t)transfer->buffer[ 5] <<  8 |
                                (uint32_t)transfer->buffer[ 6] << 16 |
                                (uint32_t)transfer->buffer[ 7] << 24;
                            context->length =
                                (uint32_t)transfer->buffer[ 8] <<  0 |
                                (uint32_t)transfer->buffer[ 9] <<  8 |
                                (uint32_t)transfer->buffer[10] << 16 |
                                (uint32_t)transfer->buffer[11] << 24;
                            context->command = transfer->buffer[15];
                            if (context->length) {
                                context->state = transfer->buffer[12] & 0x80 ? IN_STATE : OUT_STATE;
                                switch (context->command) {
                                    case SCSI_READ_10:
                                    case SCSI_WRITE_10: {
                                        uint32_t lba =
                                            (uint32_t)transfer->buffer[17] << 24 |
                                            (uint32_t)transfer->buffer[18] << 16 |
                                            (uint32_t)transfer->buffer[19] <<  8 |
                                            (uint32_t)transfer->buffer[20] <<  0;
                                        uint16_t length =
                                            (uint16_t)transfer->buffer[22] << 8 |
                                            (uint16_t)transfer->buffer[23] << 0;
                                        if (lba > context->blocks) {
                                            length = 0;
                                        } else if (length > context->blocks - lba) {
                                            length = context->blocks - lba;
                                        }
                                        if (fseek(context->image, (long)lba << context->shift, SEEK_SET)) {
                                            length = 0;
                                        }
                                        length <<= context->shift;
                                        if (context->length > length) {
                                            context->length = length;
                                        }
                                        break;
                                    }
                                    default:
                                        break;
                                }
                            } else {
                                switch (context->command) {
                                    case SCSI_TEST_UNIT_READY:
                                        context->status = MSD_COMMAND_PASSED;
                                        break;
                                    default:
                                        context->status = MSD_PHASE_ERROR;
                                        break;
                                }
                                context->state = STATUS_STATE;
                            }
                            transfer->buffer = NULL;
                            transfer->length = 0;
                            transfer->status = USB_TRANSFER_COMPLETED;
                            break;
                        case IN_STATE:
                        case OUT_STATE:
                            if (transfer->length > context->length) {
                                transfer->length = context->length;
                            }
                            switch (context->command) {
                                case SCSI_TEST_UNIT_READY:
                                    if (context->state != IN_STATE) {
                                        transfer->buffer = NULL;
                                        transfer->length = 0;
                                        transfer->status = USB_TRANSFER_COMPLETED;
                                        context->status = MSD_COMMAND_FAILED;
                                        break;
                                    }
                                    transfer->buffer = NULL;
                                    transfer->length = 0;
                                    transfer->status = USB_TRANSFER_COMPLETED;
                                    context->status = MSD_COMMAND_PASSED;
                                    break;
                                case SCSI_REQUEST_SENSE:
                                    if (context->state != IN_STATE) {
                                        transfer->buffer = NULL;
                                        transfer->length = 0;
                                        transfer->status = USB_TRANSFER_COMPLETED;
                                        context->status = MSD_COMMAND_FAILED;
                                        break;
                                    }
                                    transfer->buffer[0] = 0x70;
                                    memset(transfer->buffer + 1, 0, transfer->length - 1);
                                    transfer->status = USB_TRANSFER_COMPLETED;
                                    context->status = MSD_COMMAND_PASSED;
                                    break;
                                case SCSI_INQUIRY: {
                                    if (context->state != IN_STATE) {
                                        transfer->buffer = NULL;
                                        transfer->length = 0;
                                        transfer->status = USB_TRANSFER_COMPLETED;
                                        context->status = MSD_COMMAND_FAILED;
                                        break;
                                    }
                                    static const uint8_t inquiry[] = {
                                        0x00, 0x00, 0x00, 0x00, 0x00,
                                    };
                                    transfer->buffer = (uint8_t *)inquiry;
                                    if (transfer->length > sizeof(inquiry)) {
                                        transfer->length = sizeof(inquiry);
                                    }
                                    transfer->status = USB_TRANSFER_COMPLETED;
                                    context->status = MSD_COMMAND_PASSED;
                                    break;
                                }
                                case SCSI_READ_CAPACITY: {
                                    if (context->state != IN_STATE) {
                                        transfer->buffer = NULL;
                                        transfer->length = 0;
                                        transfer->status = USB_TRANSFER_COMPLETED;
                                        context->status = MSD_COMMAND_FAILED;
                                        break;
                                    }
                                    uint64_t lba = context->blocks - 1;
                                    if (lba > UINT32_C(0xFFFFFFFF)) {
                                        lba = UINT32_C(0xFFFFFFFF);
                                    }
                                    uint32_t length = UINT32_C(1) << context->shift;
                                    transfer->buffer[0] = lba >> 24;
                                    transfer->buffer[1] = lba >> 16;
                                    transfer->buffer[2] = lba >>  8;
                                    transfer->buffer[3] = lba >>  0;
                                    transfer->buffer[4] = length >> 24;
                                    transfer->buffer[5] = length >> 16;
                                    transfer->buffer[6] = length >>  8;
                                    transfer->buffer[7] = length >>  0;
                                    if (transfer->length > 8) {
                                        transfer->length = 8;
                                    }
                                    transfer->status = USB_TRANSFER_COMPLETED;
                                    context->status = MSD_COMMAND_PASSED;
                                    break;
                                }
                                case SCSI_READ_10:
                                    if (context->state != IN_STATE) {
                                        transfer->buffer = NULL;
                                        transfer->length = 0;
                                        transfer->status = USB_TRANSFER_COMPLETED;
                                        context->status = MSD_COMMAND_FAILED;
                                        break;
                                    }
                                    transfer->length = fread(transfer->buffer, 1, transfer->length, context->image);
                                    transfer->status = USB_TRANSFER_COMPLETED;
                                    context->status = MSD_COMMAND_PASSED;
                                    break;
                                case SCSI_WRITE_10:
                                    if (context->state != OUT_STATE) {
                                        transfer->buffer = NULL;
                                        transfer->length = 0;
                                        transfer->status = USB_TRANSFER_COMPLETED;
                                        context->status = MSD_COMMAND_FAILED;
                                        break;
                                    }
                                    transfer->length = fwrite(transfer->buffer, 1, transfer->length, context->image);
                                    transfer->status = USB_TRANSFER_COMPLETED;
                                    context->status = MSD_COMMAND_PASSED;
                                    break;
                                default:
                                    transfer->buffer = NULL;
                                    transfer->length = 0;
                                    transfer->status = USB_TRANSFER_COMPLETED;
                                    context->status = MSD_COMMAND_FAILED;
                                    break;
                            }
                            context->length -= transfer->length;
                            context->state = transfer->status == USB_TRANSFER_COMPLETED ? STATUS_STATE : COMMAND_STATE;
                            break;
                        case STATUS_STATE:
                            if (transfer->length < 13) {
                                transfer->buffer = NULL;
                                transfer->length = 0;
                                transfer->status = USB_TRANSFER_STALLED;
                                break;
                            }
                            context->state = COMMAND_STATE;
                            transfer->buffer[ 0] = 'U';
                            transfer->buffer[ 1] = 'S';
                            transfer->buffer[ 2] = 'B';
                            transfer->buffer[ 3] = 'S';
                            transfer->buffer[ 4] = context->tag >>  0;
                            transfer->buffer[ 5] = context->tag >>  8;
                            transfer->buffer[ 6] = context->tag >> 16;
                            transfer->buffer[ 7] = context->tag >> 24;
                            transfer->buffer[ 8] = context->length >>  0;
                            transfer->buffer[ 9] = context->length >>  8;
                            transfer->buffer[10] = context->length >> 16;
                            transfer->buffer[11] = context->length >> 24;
                            transfer->buffer[12] = context->status;
                            transfer->length = 13;
                            transfer->status = USB_TRANSFER_COMPLETED;
                            break;
                    }
                    break;
                default:
                    break;
            }
            break;
        case USB_DESTROY_EVENT:
            if (event->progress_handler) {
                event->progress_handler(event->progress_context, 0, 0);
                event->progress_handler = NULL;
            }
            if (!context) {
                break;
            }
            if (context->image) {
                fclose(context->image);
                context->image = NULL;
            }
            free(context);
            event->context = context = NULL;
            break;
        default:
            return EINVAL;
    }
    return error;
}
