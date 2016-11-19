#ifndef INTERRUPT_H
#define INTERRUPT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "port.h"

#define INT_ON        (1 <<  0)
#define INT_TIMER1    (1 <<  1)
#define INT_TIMER2    (1 <<  2)
#define INT_TIMER3    (1 <<  3)
#define INT_OSTMR     (1 <<  4)
#define INT_KEYPAD    (1 << 10)
#define INT_LCD       (1 << 11)
#define INT_RTC       (1 << 12)
#define INT_PWR       (1 << 15)
#define INT_WAKE      (1 << 19)

typedef struct interrupt_state {
    uint32_t status   : 22;
    uint32_t          :  2;
    uint32_t enabled  : 22;
    uint32_t          :  2;
    uint32_t latched  : 22;
    uint32_t          :  2;
    uint32_t inverted : 22;
    uint32_t          :  2;
} interrupt_state_t;

/* External INTERRUPT state */
extern interrupt_state_t intrpt[2];

/* Available Functions */
eZ80portrange_t init_intrpt(void);
void intrpt_reset(void);
void intrpt_pulse(uint32_t int_num);
void intrpt_set(uint32_t int_num, bool set);

/* Save/Restore */
typedef struct emu_image emu_image;
bool intrpt_restore(const emu_image*);
bool intrpt_save(emu_image*);

#ifdef __cplusplus
}
#endif

#endif
