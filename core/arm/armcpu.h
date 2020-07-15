#ifndef ARMCPU_H
#define ARMCPU_H

#include "arm.h"

#include <stdbool.h>
#include <stdint.h>

typedef enum arm_exception_number {
    ARM_Thread_Mode = 0,
    ARM_Exception_Reset = 1,
    ARM_Exception_NMI = 2,
    ARM_Exception_HardFault = 3,
    ARM_Exception_SVCall = 11,
    ARM_Exception_PendSV = 14,
    ARM_Exception_SysTick = 15,
    ARM_Exception_External = 16,
    ARM_Invalid_Exception = 49,
} arm_exception_number_t;

typedef struct arm_systick {
    uint32_t ctrl, load, val, calib;
} arm_systick_t;

typedef struct arm_nvic {
    uint32_t ier, ipr, ip[8];
} arm_nvic_t;

typedef struct arm_scb {
    uint32_t icsr, vtor, aircr, scr, shp[2];
} arm_scb_t;

typedef union arm_cpu {
    uint32_t r[16];
    struct {
        uint32_t r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, ip, sp, lr, pc, altsp;
        uint64_t active;
        bool v, c, z, n, pm, spsel, exc;
        arm_systick_t systick;
        arm_nvic_t nvic;
        arm_scb_t scb;
    };
} arm_cpu_t;

#ifdef __cplusplus
extern "C" {
#endif

void arm_cpu_reset(arm_t *arm);
bool arm_cpu_exception(arm_t *arm, arm_exception_number_t type);
void arm_cpu_execute(arm_t *arm);

#ifdef __cplusplus
}
#endif

#endif
