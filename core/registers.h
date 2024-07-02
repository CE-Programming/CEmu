/*
 * Copyright (c) 2014 The KnightOS Group
 * Modified to support the eZ80 processor by CEmu developers
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated 
 * documentation files (the "Software"), to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions 
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED 
 * TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF 
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
*/

#ifndef REGISTERS_H
#define REGISTERS_H

#include "defines.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

typedef union {
    uint16_t hl, hls;
#if CEMU_BYTE_ORDER == CEMU_LITTLE_ENDIAN
    struct {
        uint8_t l;
        uint8_t h;
    };
#else
    struct {
        uint8_t h;
        uint8_t l;
    };
#endif
} short_reg_t;

typedef union {
    uint32_t hl;
#if CEMU_BYTE_ORDER == CEMU_LITTLE_ENDIAN
    struct {
        uint16_t hls;
        uint16_t hls_pad;
    };
    struct {
        uint8_t l;
        uint8_t h;
        uint8_t hlu;
        uint8_t hlu_pad;
    };
#else
    struct {
        uint16_t hls_pad;
        uint16_t hls;
    };
    struct {
        uint8_t hlu_pad;
        uint8_t hlu;
        uint8_t h;
        uint8_t l;
    };
#endif
} long_reg_t;

typedef struct {
    union {
        short_reg_t af;
        uint16_t AF;
        struct {
#if CEMU_BYTE_ORDER == CEMU_BIG_ENDIAN
            uint8_t A;
#endif
            union {
                uint8_t F;
#if CEMU_BITFIELD_ORDER == CEMU_LITTLE_ENDIAN
                struct {
                    uint8_t C  : 1;
                    uint8_t N  : 1;
                    uint8_t PV : 1;
                    uint8_t _3 : 1;
                    uint8_t H  : 1;
                    uint8_t _5 : 1;
                    uint8_t Z  : 1;
                    uint8_t S  : 1;
                } flags;
#else
                struct {
                    uint8_t S  : 1;
                    uint8_t Z  : 1;
                    uint8_t _5 : 1;
                    uint8_t H  : 1;
                    uint8_t _3 : 1;
                    uint8_t PV : 1;
                    uint8_t N  : 1;
                    uint8_t C  : 1;
                } flags;
#endif
            };
#if CEMU_BYTE_ORDER == CEMU_LITTLE_ENDIAN
            uint8_t A;
#endif
        };
    };
#if CEMU_BYTE_ORDER == CEMU_LITTLE_ENDIAN
# define DEF_LONG_REG(name, PAIR, HIGH, LOW)    \
    union {                                     \
        long_reg_t name;                        \
        uint32_t PAIR;                          \
        struct {                                \
            uint16_t PAIR##S;                   \
            uint16_t PAIR##S_pad;               \
        };                                      \
        struct {                                \
            uint8_t LOW;                        \
            uint8_t HIGH;                       \
            uint8_t PAIR##U;                    \
            uint8_t PAIR##U_pad;                \
        };                                      \
    }
#else
# define DEF_LONG_REG(name, PAIR, HIGH, LOW)    \
    union {                                     \
        long_reg_t name;                        \
        uint32_t PAIR;                          \
        struct {                                \
            uint16_t PAIR##S_pad;               \
            uint16_t PAIR##S;                   \
        };                                      \
        struct {                                \
            uint8_t PAIR##U_pad;                \
            uint8_t PAIR##U;                    \
            uint8_t HIGH;                       \
            uint8_t LOW;                        \
        };                                      \
    }
#endif
    DEF_LONG_REG(bc, BC, B, C);
    DEF_LONG_REG(de, DE, D, E);
    union {
        long_reg_t index[4];
        struct {
            DEF_LONG_REG(hl, HL, H, L);
            uint32_t _HL;
            DEF_LONG_REG(ix, IX, IXH, IXL);
            DEF_LONG_REG(iy, IY, IYH, IYL);
        };
    };
    uint16_t _AF;
    uint32_t _BC, _DE;
    union {
        long_reg_t stack[2];
        struct {
#if CEMU_BYTE_ORDER == CEMU_LITTLE_ENDIAN
            uint16_t SPS;
            uint16_t SPS_pad;
#else
            uint16_t SPS_pad;
            uint16_t SPS;
#endif
            uint32_t SPL;
        };
    };
    DEF_LONG_REG(pc, PC, PCH, PCL);
    uint16_t I;
    uint8_t R, MBASE;
} eZ80registers_t;
#undef DEF_LONG_REG

typedef enum {
    FLAG_S =  1 << 7,
    FLAG_Z =  1 << 6,
    FLAG_5 =  1 << 5,
    FLAG_H =  1 << 4,
    FLAG_3 =  1 << 3,
    FLAG_PV = 1 << 2,
    FLAG_N  = 1 << 1,
    FLAG_C  = 1 << 0,
    FLAG_NONE = 0
} eZ80flags;

int parity(uint8_t x);

/* S Z 5 H 3 PV N C */
#define cpuflag_s(a)  ((a) ? FLAG_S  : 0)
#define cpuflag_h(a)  ((a) ? FLAG_H  : 0)
#define cpuflag_pv(a) ((a) ? FLAG_PV : 0)
#define cpuflag_c(a)  ((a) ? FLAG_C  : 0)

#define cpuflag_carry_w(a,mode) cpuflag_c((a) & (0x10000<<(mode<<3)))
#define cpuflag_carry_b(a) cpuflag_c((a) & 0x100)

#define cpuflag_sign_w(a,mode) cpuflag_s((a) & (0x8000<<(mode<<3)))
#define cpuflag_sign_b(a) cpuflag_s((a) & 0x80)

#define cpuflag_parity(a) cpuflag_pv(!parity(a))

#define cpuflag_undef(a) ((a) & (FLAG_3 | FLAG_5))

#define cpuflag_overflow_b_add(op1, op2, result) cpuflag_pv(((op1) ^ (result)) & ((op2) ^ (result)) & 0x80)
#define cpuflag_overflow_b_sub(op1, op2, result) cpuflag_pv(((op1) ^ (op2)) & ((op1) ^ (result)) & 0x80)
#define cpuflag_overflow_w_add(op1, op2, result, mode) cpuflag_pv(((op1) ^ (result)) & ((op2) ^ (result)) & (0x8000 << ((mode) << 3)))
#define cpuflag_overflow_w_sub(op1, op2, result, mode) cpuflag_pv(((op1) ^ (op2)) & ((op1) ^ (result)) & (0x8000 << ((mode) << 3)))

#define cpuflag_halfcarry_b_add(op1, op2, carry) cpuflag_h((((op1) & 0x00f) + ((op2) & 0x00f) + (carry)) & 0x0010)
#define cpuflag_halfcarry_b_sub(op1, op2, carry) cpuflag_h((((op1) & 0x00f) - ((op2) & 0x00f) - (carry)) & 0x0010)
#define cpuflag_halfcarry_w_add(op1, op2, carry) cpuflag_h((((op1) & 0xfff) + ((op2) & 0xfff) + (carry)) & 0x1000)
#define cpuflag_halfcarry_w_sub(op1, op2, carry) cpuflag_h((((op1) & 0xfff) - ((op2) & 0xfff) - (carry)) & 0x1000)

#define cpuflag_subtract(a)   ((a) ? FLAG_N : 0)
#define cpuflag_zero(a)       ((a) ? 0 : FLAG_Z)

#define mask_mode(a, mode)  (a &= (0xFFFF<<(mode<<3))|0xFF)

#ifdef __cplusplus
}
#endif

#endif
