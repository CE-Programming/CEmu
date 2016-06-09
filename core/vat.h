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
    CALC_VAR_TYPE_UNKNOWN1,
    CALC_VAR_TYPE_IMAGE,
    CALC_VAR_TYPE_CPLX_FRAC,
    CALC_VAR_TYPE_REAL_RADICAL,
    CALC_VAR_TYPE_CPLX_RADICAL,
    CALC_VAR_TYPE_CPLX_PI,
    CALC_VAR_TYPE_CPLX_PI_FRAC,
    CALC_VAR_TYPE_REAL_PI,
    CALC_VAR_TYPE_REAL_PI_FRAC,
    CALC_VAR_TYPE_UNKNOWN2,
    CALC_VAR_TYPE_OPERATING_SYSTEM,
    CALC_VAR_TYPE_FLASH_APP,
    CALC_VAR_TYPE_CERTIFICATE,
    CALC_VAR_TYPE_UNKNOWN3,
    CALC_VAR_TYPE_CERTIFICATE_MEMORY,
    CALC_VAR_TYPE_UNKNOWN4,
    CALC_VAR_TYPE_CLOCK,
    CALC_VAR_TYPE_UNKNOWN5,
    CALC_VAR_TYPE_UNKNOWN6,
    CALC_VAR_TYPE_UNKNOWN7,
    CALC_VAR_TYPE_UNKNOWN8,
    CALC_VAR_TYPE_UNKNOWN9,
    CALC_VAR_TYPE_UNKNOWN10,
    CALC_VAR_TYPE_UNKNOWN11,
    CALC_VAR_TYPE_UNKNOWN12,
    CALC_VAR_TYPE_UNKNOWN13,
    CALC_VAR_TYPE_UNKNOWN14,
    CALC_VAR_TYPE_UNKNOWN15,
    CALC_VAR_TYPE_UNKNOWN16,
    CALC_VAR_TYPE_UNKNOWN17,
    CALC_VAR_TYPE_UNKNOWN18,
    CALC_VAR_TYPE_UNKNOWN19,
    CALC_VAR_TYPE_UNKNOWN20,
    CALC_VAR_TYPE_UNKNOWN21,
    CALC_VAR_TYPE_UNKNOWN22,
    CALC_VAR_TYPE_UNKNOWN23,
    CALC_VAR_TYPE_UNKNOWN24,
    CALC_VAR_TYPE_FLASH_LICENSE,
    CALC_VAR_TYPE_UNKNOWN25
} calc_var_type_t;

extern const char *calc_var_type_names[0x40];
const char *calc_var_name_to_utf8(uint8_t name[8]);
const char *calc_var_name_to_ascii(uint8_t name[8]);

typedef struct calc_var {
    uint32_t vat, address;
    uint8_t type1, type2, version, namelen, name[9], *data;
    calc_var_type_t type;
    uint16_t size;
    bool archived;
} calc_var_t;

void vat_search_init(calc_var_t *);
bool vat_search_next(calc_var_t *);
bool vat_search_find(const calc_var_t *, calc_var_t *);

bool calc_var_is_prog(const calc_var_t *);
bool calc_var_is_asmprog(const calc_var_t *);
bool calc_var_is_internal(const calc_var_t *);

#ifdef __cplusplus
}
#endif

#endif
