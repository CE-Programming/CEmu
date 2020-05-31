#include "device.h"

#include <stdio.h>
#include <stdlib.h>

typedef enum fuzz_command {
    FUZZ_NOOP_COMMAND,
    FUZZ_RESET_COMMAND,
    FUZZ_CONTROL_COMMAND,
    FUZZ_BULK_COMMAND_START,
    FUZZ_BULK_COMMAND_END = FUZZ_BULK_COMMAND_START + 64,
    FUZZ_TIMER_NONE_COMMAND,
    FUZZ_TIMER_ABSOLUTE_COMMAND,
    FUZZ_TIMER_RELATIVE_COMMAND,
} fuzz_command_t;

int usb_fuzz_device(usb_event_t *event) {
    if (event->type == USB_TRANSFER_EVENT && !event->info.transfer.direction)
        fread(event->info.transfer.buffer, event->info.transfer.length, 1, stdin);
    uint8_t command = FUZZ_NOOP_COMMAND;
    fread(&command, sizeof(command), 1, stdin);
    switch (command) {
        default:
            event->type = USB_INIT_EVENT;
            break;
        case FUZZ_RESET_COMMAND:
            event->type = USB_RESET_EVENT;
            break;
        case FUZZ_CONTROL_COMMAND: {
            static uint8_t setup[8];
            fread(setup, sizeof(setup), 1, stdin);
            event->type = USB_TRANSFER_EVENT;
            event->info.transfer.buffer = setup;
            event->info.transfer.length = sizeof(setup);
            event->info.transfer.endpoint = 0;
            event->info.transfer.setup = true;
            event->info.transfer.direction = false;
            break;
        }
        case FUZZ_BULK_COMMAND_START...FUZZ_BULK_COMMAND_END: {
            uint8_t endpoint = 0;
            fread(&endpoint, sizeof(endpoint), 1, stdin);
            event->type = USB_TRANSFER_EVENT;
            event->info.transfer.length = command - FUZZ_BULK_COMMAND_START;
            event->info.transfer.endpoint = endpoint;
            event->info.transfer.setup = false;
            event->info.transfer.direction = false;
            break;
        }
        case FUZZ_TIMER_NONE_COMMAND...FUZZ_TIMER_RELATIVE_COMMAND:
            event->type = USB_TIMER_EVENT;
            event->info.timer.mode = USB_TIMER_NONE + command - FUZZ_TIMER_NONE_COMMAND;
            fread(&event->info.timer.useconds, sizeof(event->info.timer.useconds), 1, stdin);
            break;
    }
    if (feof(stdin) || ferror(stdin)) {
        exit(0);
    }
    return 0;
}
