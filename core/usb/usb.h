#ifndef H_USB
#define H_USB

#ifdef __cplusplus
extern "C" {
#endif

#include "../port.h"

#include <stdint.h>
#include <stdio.h>
#include "fotg210.h"

typedef struct usb_state {
    struct fotg210_regs regs;
} usb_state_t;

extern usb_state_t usb;

eZ80portrange_t init_usb(void);
void usb_reset(void);
bool usb_restore(FILE *image);
bool usb_save(FILE *image);

#ifdef __cplusplus
}
#endif

#endif
