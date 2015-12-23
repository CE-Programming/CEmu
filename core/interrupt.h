/* Declarations for interrupt.c */

#ifndef _H_INTERRUPT
#define _H_INTERRUPT

#include <core/apb.h>

#ifdef __cplusplus
extern "C" {
#endif

#define INT_ON        0
#define INT_TIMER0    1
#define INT_TIMER1    2
#define INT_TIMER3    3
#define INT_OSTMR     4
#define INT_KEYPAD   10
#define INT_LCD      11
#define INT_RTC      12
#define INT_PWR      15  // Probably power bit. Probably.

typedef struct interrupt_request {
    uint32_t status   : 22;
    uint32_t          :  2;
    uint32_t enabled  : 22;
    uint32_t          :  2;
    uint32_t latched  : 22;
    uint32_t          :  2;
    uint32_t inverted : 22;
    uint32_t          :  2;
} interrupt_request_t;

typedef struct interrupt_state {
    uint32_t status;
    interrupt_request_t request[2];
} interrupt_state_t;

/* External INTERRUPT state */
extern interrupt_state_t intrpt;

/* Available Functions */
eZ80portrange_t init_intrpt(void);
void intrpt_reset(void);
void intrpt_set(uint32_t int_num, int on);

#ifdef __cplusplus
}
#endif

#endif
