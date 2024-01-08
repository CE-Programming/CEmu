#ifndef BOOTVER_H
#define BOOTVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "asic.h"

#define SIZE_BOOTCODE 0x20000

    typedef struct {
        uint8_t major;
        uint8_t minor;
        uint8_t revision;
        uint16_t build;
    } boot_ver_t;

    bool bootver_parse(const uint8_t* data, boot_ver_t* boot_ver);

    bool bootver_check_ver(const boot_ver_t* ver, const boot_ver_t* check);

    bool bootver_check_rev(const boot_ver_t* ver, asic_rev_t rev);

#ifdef __cplusplus
}
#endif

#endif
