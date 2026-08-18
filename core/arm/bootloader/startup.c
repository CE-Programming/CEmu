/*
 * MIT License; see LICENSE.
 * Minimal Cortex-M0+ startup for the CEmu free SAMD21 bootloader.
 */

#include <stdint.h>

extern uint32_t _stack_top;
extern uint32_t _sidata;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;

void bootloader_main(void);

static void default_handler(void) {
    for (;;) {}
}

static void reset_handler(void) {
    uint32_t *source = &_sidata;
    for (uint32_t *dest = &_sdata; dest < &_edata; ++dest) {
        *dest = *source++;
    }
    for (uint32_t *dest = &_sbss; dest < &_ebss; ++dest) {
        *dest = 0;
    }
    bootloader_main();
    default_handler();
}

__attribute__((section(".vectors"), used))
const uintptr_t vector_table[48] = {
    (uintptr_t)&_stack_top,
    (uintptr_t)reset_handler,
    (uintptr_t)default_handler,
    (uintptr_t)default_handler,
    0, 0, 0, 0, 0, 0, 0,
    (uintptr_t)default_handler,
    0, 0,
    (uintptr_t)default_handler,
    (uintptr_t)default_handler,
    [16 ... 47] = (uintptr_t)default_handler,
};
