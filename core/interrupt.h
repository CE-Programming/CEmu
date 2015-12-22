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

struct interrupt_state {
    uint32_t raw_status       : 22;  // Raw interrupt status (can be either raw signal or latched on signal change from low-to-high)
    uint32_t int_enable_mask  : 22;  // Interrupt enable mask
//  uint32_t int_ack          : 22;  // Interrupt acknowledge (used by ISR), seems to only affect latched status bits
    uint32_t int_latch        : 22;  // Determines whether bits of 5000 will latch. 0 means raw signal, 1 means latched
    uint32_t int_invr         : 22;  // Inverts the raw signal of the interrupts corresponding to each 1 bit. Can be used to latch on a high-to-low change

    uint8_t f_irq;
    uint8_t f_fiq;
};

/* Type definitions */
typedef struct interrupt_state interrupt_state_t;

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
