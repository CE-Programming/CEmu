#ifndef _H_USB
#define _H_USB

#include <core/cpu.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Standard USB state */
struct usb_state {
    uint8_t dummy;
};

/* Type definitions */
typedef struct usb_state usb_state_t;

/* Global GPT state */
extern usb_state_t usb;

/* Available Functions */
eZ80portrange_t init_usb(void);
void usb_reset(void);

#ifdef __cplusplus
}
#endif

#endif
