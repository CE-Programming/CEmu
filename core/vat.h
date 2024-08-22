#ifndef VAT_H
#define VAT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

typedef enum calc_var_type {
    CALC_VAR_TYPE_REAL = 0,
    CALC_VAR_TYPE_REAL_LIST,
    CALC_VAR_TYPE_MATRIX,
    CALC_VAR_TYPE_EQU,
    CALC_VAR_TYPE_STRING,
    CALC_VAR_TYPE_PROG,
    CALC_VAR_TYPE_PROT_PROG,
    CALC_VAR_TYPE_PICTURE,
    CALC_VAR_TYPE_GDB,
    CALC_VAR_TYPE_UNKNOWN,
    CALC_VAR_TYPE_UNKNOWN_EQU,
    CALC_VAR_TYPE_NEW_EQU,
    CALC_VAR_TYPE_CPLX,
    CALC_VAR_TYPE_CPLX_LIST,
    CALC_VAR_TYPE_UNDEF,
    CALC_VAR_TYPE_WINDOW,
    CALC_VAR_TYPE_RCL_WINDOW,
    CALC_VAR_TYPE_TABLE_RANGE,
    CALC_VAR_TYPE_LCD,
    CALC_VAR_TYPE_BACKUP,
    CALC_VAR_TYPE_APP,
    CALC_VAR_TYPE_APP_VAR,
    CALC_VAR_TYPE_TEMP_PROG,
    CALC_VAR_TYPE_GROUP,
    CALC_VAR_TYPE_REAL_FRAC,
    CALC_VAR_TYPE_MIXED_FRAC,
    CALC_VAR_TYPE_IMAGE,
    CALC_VAR_TYPE_CPLX_FRAC,
    CALC_VAR_TYPE_REAL_RADICAL,
    CALC_VAR_TYPE_CPLX_RADICAL,
    CALC_VAR_TYPE_CPLX_PI,
    CALC_VAR_TYPE_CPLX_PI_FRAC,
    CALC_VAR_TYPE_REAL_PI,
    CALC_VAR_TYPE_REAL_PI_FRAC,
    CALC_VAR_TYPE_OPERATING_SYSTEM = 0x23,
    CALC_VAR_TYPE_FLASH_APP,
    CALC_VAR_TYPE_CERTIFICATE,
    CALC_VAR_TYPE_APPID_LIST,
    CALC_VAR_TYPE_CERTIFICATE_MEMORY,
    CALC_VAR_TYPE_UNIT_CERTIFICATE,
    CALC_VAR_TYPE_CLOCK,
    CALC_VAR_TYPE_FLASH_LICENSE = 0x3E
} calc_var_type_t;

extern const char *calc_var_type_names[0x40];
const char *calc_var_name_to_utf8(const uint8_t name[8], uint8_t namelen, bool named);

typedef struct calc_var {
    uint32_t vat, address;
    uint8_t type1, type2, version, namelen, name[9], *data;
    calc_var_type_t type;
    uint16_t size;
    bool archived, named;
} calc_var_t;

void vat_search_init(calc_var_t *);
bool vat_search_next(calc_var_t *);
bool vat_search_find(const calc_var_t *, calc_var_t *);

int calc_var_compare_names(const calc_var_t *, const calc_var_t *);
calc_var_type_t calc_var_normalized_type(calc_var_type_t);
bool calc_var_is_list(const calc_var_t *);
bool calc_var_is_prog(const calc_var_t *);
bool calc_var_is_asmprog(const calc_var_t *);
bool calc_var_is_internal(const calc_var_t *);
bool calc_var_is_tokenized(const calc_var_t *);
bool calc_var_is_python_appvar(const calc_var_t *);

#ifdef __cplusplus
}
#endif

#endif
