/*
 * Copyright (c) 2015-2024 CE Programming.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CEMUCORE_H
#define CEMUCORE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum cemucore_error
{
    CEMUCORE_SUCCESS,
    CEMUCORE_ERROR_INVALID,
    CEMUCORE_ERROR_FAILED,
} cemucore_error_t;

typedef enum cemucore_dev
{
    CEMUCORE_DEV_UNKNOWN,
    CEMUCORE_DEV_TI84PCE,
    CEMUCORE_DEV_TI84PCEPE,
    CEMUCORE_DEV_TI83PCE,
    CEMUCORE_DEV_TI83PCEEP,
    CEMUCORE_DEV_TI84PCET,
    CEMUCORE_DEV_TI84PCETPE,
} cemucore_dev_t;

typedef enum cemucore_data_type
{
    CEMUCORE_DATA_IMAGE,
    CEMUCORE_DATA_ROM,
    CEMUCORE_DATA_RAM,
} cemucore_data_type_t;

struct cemucore_debug_info
{
    int reason;
    uint32_t data;
};

struct cemucore_load_info
{
    cemucore_data_type_t type;
    const char *path;
};

struct cemucore_save_info
{
    cemucore_data_type_t type;
    const char *path;
};

struct cemucore_core_info
{
    asic_rev_t loaded_rev;
    asic_rev_t default_rev;
    boot_ver_t boot_ver;
    bool python;
};

cemucore_error_t cemucore_load(struct cemucore_load_info *load, struct cemucore_core_info *core);
cemucore_error_t cemucore_save(struct cemucore_save_info *save);
cemucore_error_t cemucore_run(uint64_t ticks);
cemucore_error_t cemucore_stop(void);

void cemucore_console_printf(const char *format, ...);
void cemucore_console_err_printf(const char *format, ...);
void cemucore_console_clear(void);

#ifdef DEBUG_SUPPORT
void cemucore_debug_open(cemucore_debug_info_t *info);
void cemucore_debug_close(void);
#endif

#ifdef __cplusplus
}
#endif

#endif