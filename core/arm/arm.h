#ifndef ARM_H
#define ARM_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct arm arm_t;

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
void arm_reset(arm_t *arm);
bool arm_load(arm_t *arm, const char *path);
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
