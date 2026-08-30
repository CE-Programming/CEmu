#ifndef ARM_H
#define ARM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef struct arm arm_t;

typedef enum arm_bootloader_type {
    ARM_BOOTLOADER_UNKNOWN,
    ARM_BOOTLOADER_CEMU_FREE,
    ARM_BOOTLOADER_TI_UF2,
} arm_bootloader_type_t;

typedef struct arm_cpu_snapshot {
    uint32_t registers[16];
    uint32_t alternate_stack_pointer;
    uint64_t active_exceptions;
    bool overflow, carry, zero, negative;
    bool primask, process_stack, exception, wait_for_interrupt, svc_pending;
    struct {
        uint32_t control, reload, current, calibration;
    } systick;
    struct {
        uint32_t interrupt_enable, interrupt_pending, priorities[8];
    } nvic;
    struct {
        uint32_t interrupt_control, vector_table, application_interrupt_reset_control;
        uint32_t system_control, system_priorities[2];
    } scb;
    uint64_t cycles, cycle_limit;
    bool sleeping;
} arm_cpu_snapshot_t;

#define ARM_BOOTLOADER_DESCRIPTION_SIZE 64

#ifdef __cplusplus
extern "C" {
#endif

arm_t *arm_create(void);
void arm_destroy(arm_t *arm);

/* Thread-safe */
void arm_set_time(arm_t *arm, uint64_t cycles);
void arm_advance_to(arm_t *arm, uint64_t cycles);
void arm_run_until(arm_t *arm, uint64_t cycles);
void arm_pause(arm_t *arm);
uint64_t arm_get_time(arm_t *arm);
bool arm_get_cpu_snapshot(arm_t *arm, arm_cpu_snapshot_t *snapshot);
uint8_t arm_read_byte(arm_t *arm, uint32_t address);
uint16_t arm_read_half(arm_t *arm, uint32_t address);
uint32_t arm_read_word(arm_t *arm, uint32_t address);
void arm_write_byte(arm_t *arm, uint32_t address, uint8_t value);
void arm_write_half(arm_t *arm, uint32_t address, uint16_t value);
void arm_write_word(arm_t *arm, uint32_t address, uint32_t value);
void arm_reset(arm_t *arm);
bool arm_load(arm_t *arm, const char *path);
arm_bootloader_type_t arm_get_bootloader_info(arm_t *arm, char *desc, size_t desc_size);
bool arm_save_flash(arm_t *arm, FILE *image);
bool arm_restore_flash(arm_t *arm, FILE *image);
bool arm_save_state(arm_t *arm, FILE *image);
bool arm_restore_state(arm_t *arm, FILE *image);
void arm_spi_sel(arm_t *arm, bool low);
uint8_t arm_spi_peek(arm_t *arm, uint32_t *res);
uint8_t arm_spi_xfer(arm_t *arm, uint32_t val, uint32_t *res);
bool arm_usart_send(arm_t *arm, uint8_t val);
bool arm_usart_recv(arm_t *arm, uint8_t *val);

#ifdef __cplusplus
}
#endif

#endif
