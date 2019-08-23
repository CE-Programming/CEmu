#ifndef ARMCPU_H
#define ARMCPU_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum arm_exception_number {
    ARM_Exception_Reset = 1,
    ARM_Exception_NMI = 2,
    ARM_Exception_HardFault = 3,
    ARM_Exception_SVCall = 11,
    ARM_Exception_PendSV = 14,
    ARM_Exception_SysTick = 15,
} arm_exception_number_t;

typedef union arm_cpu_state {
    uint32_t r[32];
    struct {
        uint32_t r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, sp, lr, pc, altsp;
        uint8_t excNum : 6;
        bool v, c, z, n, pm, spsel, mode;
    };
} arm_cpu_state_t;

extern arm_cpu_state_t arm_cpu;

void arm_cpu_reset(void);
void arm_execute(void);

#ifdef __cplusplus
}
#endif

#endif
