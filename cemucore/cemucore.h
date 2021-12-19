/*
 * Copyright (c) 2015-2021 CE Programming.
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

#include <stdbool.h>
#include <stdint.h>

#ifdef CEMUCORE_STATIC
# define export
#else
# ifdef _WIN32
#  ifdef CEMUCORE_EXPORT
#   define export __declspec(dllexport)
#  else
#   define export __declspec(dllimport)
#  endif
# else
#  define export __attribute__((visibility("default")))
# endif
#endif

typedef enum cemucore_sig
{
    CEMUCORE_SIG_DEV_CHANGED,
    CEMUCORE_SIG_FLASH_SIZE_CHANGED,
    CEMUCORE_SIG_TRANSFER_TOTAL,
    CEMUCORE_SIG_TRANSFER_PROGRESS,
    CEMUCORE_SIG_TRANSFER_COMPLETE,
    CEMUCORE_SIG_LCD_FRAME,
    CEMUCORE_SIG_SOFT_CMD,
} cemucore_sig_t;

typedef enum cemucore_create_flags
{
#ifndef CEMUCORE_NOTHREADS
    CEMUCORE_CREATE_FLAG_THREADED = 1 << 0,
#endif
} cemucore_create_flags_t;

typedef enum cemucore_prop
{
    CEMUCORE_PROP_DEV,
    CEMUCORE_PROP_REG,
    CEMUCORE_PROP_REG_SHADOW,
    CEMUCORE_PROP_KEY,
    CEMUCORE_PROP_FLASH_SIZE,
    CEMUCORE_PROP_MEMORY,
    CEMUCORE_PROP_FLASH,
    CEMUCORE_PROP_RAM,
    CEMUCORE_PROP_PORT,
#ifndef CEMUCORE_NODEBUG
    CEMUCORE_PROP_MEMORY_DEBUG_FLAGS,
    CEMUCORE_PROP_FLASH_DEBUG_FLAGS,
    CEMUCORE_PROP_RAM_DEBUG_FLAGS,
    CEMUCORE_PROP_PORT_DEBUG_FLAGS,
#endif
    CEMUCORE_PROP_GPIO_ENABLE,
    CEMUCORE_PROP_TRANSFER,
} cemucore_prop_t;

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

typedef enum cemucore_transfer
{
    CEMUCORE_TRANSFER_TOTAL,
    CEMUCORE_TRANSFER_PROGRESS,
    CEMUCORE_TRANSFER_REMAINING,
    CEMUCORE_TRANSFER_ERROR,
} cemucore_transfer_t;

typedef enum cemucore_reg
{
    /* 1-bit flags */
    CEMUCORE_FLAG_C,
    CEMUCORE_FLAG_N,
    CEMUCORE_FLAG_PV,
    CEMUCORE_FLAG_X,
    CEMUCORE_FLAG_HC,
    CEMUCORE_FLAG_Y,
    CEMUCORE_FLAG_Z,
    CEMUCORE_FLAG_S,

    /* 8-bit registers */
    CEMUCORE_REG_F,
    CEMUCORE_REG_A,
    CEMUCORE_REG_C,
    CEMUCORE_REG_B,
    CEMUCORE_REG_BCU,
    CEMUCORE_REG_E,
    CEMUCORE_REG_D,
    CEMUCORE_REG_DEU,
    CEMUCORE_REG_L,
    CEMUCORE_REG_H,
    CEMUCORE_REG_HLU,
    CEMUCORE_REG_IXL,
    CEMUCORE_REG_IXH,
    CEMUCORE_REG_IXU,
    CEMUCORE_REG_IYL,
    CEMUCORE_REG_IYH,
    CEMUCORE_REG_IYU,
    CEMUCORE_REG_R,
    CEMUCORE_REG_MB,

    /* 16-bit registers */
    CEMUCORE_REG_AF,
    CEMUCORE_REG_BC,
    CEMUCORE_REG_DE,
    CEMUCORE_REG_HL,
    CEMUCORE_REG_IX,
    CEMUCORE_REG_IY,
    CEMUCORE_REG_SPS,
    CEMUCORE_REG_I,

    /* 24-bit registers */
    CEMUCORE_REG_UBC,
    CEMUCORE_REG_UDE,
    CEMUCORE_REG_UHL,
    CEMUCORE_REG_UIX,
    CEMUCORE_REG_UIY,
    CEMUCORE_REG_SPL,
    CEMUCORE_REG_PC,
    CEMUCORE_REG_RPC,
} cemucore_reg_t;

typedef enum cemucore_dbg_flags
{
    CEMUCORE_DEBUG_WATCH_READ  = 1 << 0,
    CEMUCORE_DEBUG_WATCH_WRITE = 1 << 1,
    CEMUCORE_DEBUG_WATCH_EXEC  = 1 << 2,
} cemucore_dbg_flags_t;

typedef struct cemucore cemucore_t;

#ifdef __cplusplus
extern "C"
{
#endif

typedef void (*cemucore_sig_handler_t)(cemucore_sig_t, void *);

export cemucore_t *cemucore_create(cemucore_create_flags_t create_flags, cemucore_sig_handler_t sig_handler, void *sig_handler_data);
export cemucore_t *cemucore_destroy(cemucore_t *core);

export int32_t cemucore_get(cemucore_t *core, cemucore_prop_t prop, int32_t addr);
export void cemucore_get_buffer(cemucore_t *core, cemucore_prop_t prop, int32_t addr, void *buf, uint32_t len);
export void cemucore_set(cemucore_t *core, cemucore_prop_t prop, int32_t addr, int32_t val);
export void cemucore_set_buffer(cemucore_t *core, cemucore_prop_t prop, int32_t addr, const void *buf, uint32_t len);
export int cemucore_command(cemucore_t *core, const char *const *command);

export bool cemucore_sleep(cemucore_t *core);
export bool cemucore_wake(cemucore_t *core);

#ifdef __cplusplus
}
#endif

#undef export

#endif
