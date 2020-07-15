#ifndef ARM_H
#define ARM_H

#include <stdbool.h>
#include <stdint.h>

typedef struct arm arm_t;

#ifdef __cplusplus
extern "C" {
#endif

arm_t *arm_create(void);
void arm_destroy(arm_t *arm);

/* Thread-safe */
void arm_reset(arm_t *arm);
bool arm_load(arm_t *arm, const char *path);
uint8_t arm_spi_sel(arm_t *arm, uint32_t *res);
uint8_t arm_spi_xfer(arm_t *arm, uint32_t val, uint32_t *res);
bool arm_usart_send(arm_t *arm, uint8_t val);
bool arm_usart_recv(arm_t *arm, uint8_t *val);

#ifdef __cplusplus
}
#endif

#endif
