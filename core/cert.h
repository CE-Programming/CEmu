#ifndef CERT_H
#define CERT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "defines.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/* Adapted from libtifiles by the original author */
int cert_field_get(const uint8_t *data, uint32_t length, uint16_t *field_type, const uint8_t **contents, uint32_t *field_size);
int cert_field_next(const uint8_t **data, uint32_t *length);
int cert_field_find(const uint8_t *data, uint32_t length, uint16_t field_type, const uint8_t **contents, uint32_t *field_size);
int cert_field_find_path(const uint8_t *data, uint32_t length, const uint16_t *field_path, uint16_t field_path_len, const uint8_t **contents, uint32_t *field_size);

#ifdef __cplusplus
}
#endif

#endif
