/**
 * \brief Component description for RTC
 *
 * Copyright (c) 2019 Microchip Technology Inc. and its subsidiaries.
 *
 * Subject to your compliance with these terms, you may use Microchip software and any derivatives
 * exclusively with Microchip products. It is your responsibility to comply with third party license
 * terms applicable to your use of third party software (including open source software) that may
 * accompany Microchip software.
 *
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY,
 * APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND
 * FITNESS FOR A PARTICULAR PURPOSE.
 *
 * IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL
 * LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF
 * MICROCHIP HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE FULLEST EXTENT
 * ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT
 * EXCEED THE AMOUNT OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 */

/* file generated from device description version 2019-05-20T21:16:53Z */
#ifndef _SAMD21_RTC_COMPONENT_H_
#define _SAMD21_RTC_COMPONENT_H_

/* ************************************************************************** */
/*   SOFTWARE API DEFINITION FOR RTC                                          */
/* ************************************************************************** */

/* -------- RTC_MODE0_CTRL : (RTC Offset: 0x00) (R/W 16) MODE0 Control -------- */
#define RTC_MODE0_CTRL_RESETVALUE             _U_(0x00)                                            /**<  (RTC_MODE0_CTRL) MODE0 Control  Reset Value */

#define RTC_MODE0_CTRL_SWRST_Pos              _U_(0)                                               /**< (RTC_MODE0_CTRL) Software Reset Position */
#define RTC_MODE0_CTRL_SWRST_Msk              (_U_(0x1) << RTC_MODE0_CTRL_SWRST_Pos)               /**< (RTC_MODE0_CTRL) Software Reset Mask */
#define RTC_MODE0_CTRL_SWRST(value)           (RTC_MODE0_CTRL_SWRST_Msk & ((value) << RTC_MODE0_CTRL_SWRST_Pos))
#define RTC_MODE0_CTRL_ENABLE_Pos             _U_(1)                                               /**< (RTC_MODE0_CTRL) Enable Position */
#define RTC_MODE0_CTRL_ENABLE_Msk             (_U_(0x1) << RTC_MODE0_CTRL_ENABLE_Pos)              /**< (RTC_MODE0_CTRL) Enable Mask */
#define RTC_MODE0_CTRL_ENABLE(value)          (RTC_MODE0_CTRL_ENABLE_Msk & ((value) << RTC_MODE0_CTRL_ENABLE_Pos))
#define RTC_MODE0_CTRL_MODE_Pos               _U_(2)                                               /**< (RTC_MODE0_CTRL) Operating Mode Position */
#define RTC_MODE0_CTRL_MODE_Msk               (_U_(0x3) << RTC_MODE0_CTRL_MODE_Pos)                /**< (RTC_MODE0_CTRL) Operating Mode Mask */
#define RTC_MODE0_CTRL_MODE(value)            (RTC_MODE0_CTRL_MODE_Msk & ((value) << RTC_MODE0_CTRL_MODE_Pos))
#define   RTC_MODE0_CTRL_MODE_COUNT32_Val     _U_(0x0)                                             /**< (RTC_MODE0_CTRL) Mode 0: 32-bit Counter  */
#define   RTC_MODE0_CTRL_MODE_COUNT16_Val     _U_(0x1)                                             /**< (RTC_MODE0_CTRL) Mode 1: 16-bit Counter  */
#define   RTC_MODE0_CTRL_MODE_CLOCK_Val       _U_(0x2)                                             /**< (RTC_MODE0_CTRL) Mode 2: Clock/Calendar  */
#define RTC_MODE0_CTRL_MODE_COUNT32           (RTC_MODE0_CTRL_MODE_COUNT32_Val << RTC_MODE0_CTRL_MODE_Pos) /**< (RTC_MODE0_CTRL) Mode 0: 32-bit Counter Position  */
#define RTC_MODE0_CTRL_MODE_COUNT16           (RTC_MODE0_CTRL_MODE_COUNT16_Val << RTC_MODE0_CTRL_MODE_Pos) /**< (RTC_MODE0_CTRL) Mode 1: 16-bit Counter Position  */
#define RTC_MODE0_CTRL_MODE_CLOCK             (RTC_MODE0_CTRL_MODE_CLOCK_Val << RTC_MODE0_CTRL_MODE_Pos) /**< (RTC_MODE0_CTRL) Mode 2: Clock/Calendar Position  */
#define RTC_MODE0_CTRL_MATCHCLR_Pos           _U_(7)                                               /**< (RTC_MODE0_CTRL) Clear on Match Position */
#define RTC_MODE0_CTRL_MATCHCLR_Msk           (_U_(0x1) << RTC_MODE0_CTRL_MATCHCLR_Pos)            /**< (RTC_MODE0_CTRL) Clear on Match Mask */
#define RTC_MODE0_CTRL_MATCHCLR(value)        (RTC_MODE0_CTRL_MATCHCLR_Msk & ((value) << RTC_MODE0_CTRL_MATCHCLR_Pos))
#define RTC_MODE0_CTRL_PRESCALER_Pos          _U_(8)                                               /**< (RTC_MODE0_CTRL) Prescaler Position */
#define RTC_MODE0_CTRL_PRESCALER_Msk          (_U_(0xF) << RTC_MODE0_CTRL_PRESCALER_Pos)           /**< (RTC_MODE0_CTRL) Prescaler Mask */
#define RTC_MODE0_CTRL_PRESCALER(value)       (RTC_MODE0_CTRL_PRESCALER_Msk & ((value) << RTC_MODE0_CTRL_PRESCALER_Pos))
#define   RTC_MODE0_CTRL_PRESCALER_DIV1_Val   _U_(0x0)                                             /**< (RTC_MODE0_CTRL) CLK_RTC_CNT = GCLK_RTC/1  */
#define   RTC_MODE0_CTRL_PRESCALER_DIV2_Val   _U_(0x1)                                             /**< (RTC_MODE0_CTRL) CLK_RTC_CNT = GCLK_RTC/2  */
#define   RTC_MODE0_CTRL_PRESCALER_DIV4_Val   _U_(0x2)                                             /**< (RTC_MODE0_CTRL) CLK_RTC_CNT = GCLK_RTC/4  */
#define   RTC_MODE0_CTRL_PRESCALER_DIV8_Val   _U_(0x3)                                             /**< (RTC_MODE0_CTRL) CLK_RTC_CNT = GCLK_RTC/8  */
#define   RTC_MODE0_CTRL_PRESCALER_DIV16_Val  _U_(0x4)                                             /**< (RTC_MODE0_CTRL) CLK_RTC_CNT = GCLK_RTC/16  */
#define   RTC_MODE0_CTRL_PRESCALER_DIV32_Val  _U_(0x5)                                             /**< (RTC_MODE0_CTRL) CLK_RTC_CNT = GCLK_RTC/32  */
#define   RTC_MODE0_CTRL_PRESCALER_DIV64_Val  _U_(0x6)                                             /**< (RTC_MODE0_CTRL) CLK_RTC_CNT = GCLK_RTC/64  */
#define   RTC_MODE0_CTRL_PRESCALER_DIV128_Val _U_(0x7)                                             /**< (RTC_MODE0_CTRL) CLK_RTC_CNT = GCLK_RTC/128  */
#define   RTC_MODE0_CTRL_PRESCALER_DIV256_Val _U_(0x8)                                             /**< (RTC_MODE0_CTRL) CLK_RTC_CNT = GCLK_RTC/256  */
#define   RTC_MODE0_CTRL_PRESCALER_DIV512_Val _U_(0x9)                                             /**< (RTC_MODE0_CTRL) CLK_RTC_CNT = GCLK_RTC/512  */
#define   RTC_MODE0_CTRL_PRESCALER_DIV1024_Val _U_(0xA)                                             /**< (RTC_MODE0_CTRL) CLK_RTC_CNT = GCLK_RTC/1024  */
#define RTC_MODE0_CTRL_PRESCALER_DIV1         (RTC_MODE0_CTRL_PRESCALER_DIV1_Val << RTC_MODE0_CTRL_PRESCALER_Pos) /**< (RTC_MODE0_CTRL) CLK_RTC_CNT = GCLK_RTC/1 Position  */
#define RTC_MODE0_CTRL_PRESCALER_DIV2         (RTC_MODE0_CTRL_PRESCALER_DIV2_Val << RTC_MODE0_CTRL_PRESCALER_Pos) /**< (RTC_MODE0_CTRL) CLK_RTC_CNT = GCLK_RTC/2 Position  */
#define RTC_MODE0_CTRL_PRESCALER_DIV4         (RTC_MODE0_CTRL_PRESCALER_DIV4_Val << RTC_MODE0_CTRL_PRESCALER_Pos) /**< (RTC_MODE0_CTRL) CLK_RTC_CNT = GCLK_RTC/4 Position  */
#define RTC_MODE0_CTRL_PRESCALER_DIV8         (RTC_MODE0_CTRL_PRESCALER_DIV8_Val << RTC_MODE0_CTRL_PRESCALER_Pos) /**< (RTC_MODE0_CTRL) CLK_RTC_CNT = GCLK_RTC/8 Position  */
#define RTC_MODE0_CTRL_PRESCALER_DIV16        (RTC_MODE0_CTRL_PRESCALER_DIV16_Val << RTC_MODE0_CTRL_PRESCALER_Pos) /**< (RTC_MODE0_CTRL) CLK_RTC_CNT = GCLK_RTC/16 Position  */
#define RTC_MODE0_CTRL_PRESCALER_DIV32        (RTC_MODE0_CTRL_PRESCALER_DIV32_Val << RTC_MODE0_CTRL_PRESCALER_Pos) /**< (RTC_MODE0_CTRL) CLK_RTC_CNT = GCLK_RTC/32 Position  */
#define RTC_MODE0_CTRL_PRESCALER_DIV64        (RTC_MODE0_CTRL_PRESCALER_DIV64_Val << RTC_MODE0_CTRL_PRESCALER_Pos) /**< (RTC_MODE0_CTRL) CLK_RTC_CNT = GCLK_RTC/64 Position  */
#define RTC_MODE0_CTRL_PRESCALER_DIV128       (RTC_MODE0_CTRL_PRESCALER_DIV128_Val << RTC_MODE0_CTRL_PRESCALER_Pos) /**< (RTC_MODE0_CTRL) CLK_RTC_CNT = GCLK_RTC/128 Position  */
#define RTC_MODE0_CTRL_PRESCALER_DIV256       (RTC_MODE0_CTRL_PRESCALER_DIV256_Val << RTC_MODE0_CTRL_PRESCALER_Pos) /**< (RTC_MODE0_CTRL) CLK_RTC_CNT = GCLK_RTC/256 Position  */
#define RTC_MODE0_CTRL_PRESCALER_DIV512       (RTC_MODE0_CTRL_PRESCALER_DIV512_Val << RTC_MODE0_CTRL_PRESCALER_Pos) /**< (RTC_MODE0_CTRL) CLK_RTC_CNT = GCLK_RTC/512 Position  */
#define RTC_MODE0_CTRL_PRESCALER_DIV1024      (RTC_MODE0_CTRL_PRESCALER_DIV1024_Val << RTC_MODE0_CTRL_PRESCALER_Pos) /**< (RTC_MODE0_CTRL) CLK_RTC_CNT = GCLK_RTC/1024 Position  */
#define RTC_MODE0_CTRL_Msk                    _U_(0x0F8F)                                          /**< (RTC_MODE0_CTRL) Register Mask  */


/* -------- RTC_MODE1_CTRL : (RTC Offset: 0x00) (R/W 16) MODE1 Control -------- */
#define RTC_MODE1_CTRL_RESETVALUE             _U_(0x00)                                            /**<  (RTC_MODE1_CTRL) MODE1 Control  Reset Value */

#define RTC_MODE1_CTRL_SWRST_Pos              _U_(0)                                               /**< (RTC_MODE1_CTRL) Software Reset Position */
#define RTC_MODE1_CTRL_SWRST_Msk              (_U_(0x1) << RTC_MODE1_CTRL_SWRST_Pos)               /**< (RTC_MODE1_CTRL) Software Reset Mask */
#define RTC_MODE1_CTRL_SWRST(value)           (RTC_MODE1_CTRL_SWRST_Msk & ((value) << RTC_MODE1_CTRL_SWRST_Pos))
#define RTC_MODE1_CTRL_ENABLE_Pos             _U_(1)                                               /**< (RTC_MODE1_CTRL) Enable Position */
#define RTC_MODE1_CTRL_ENABLE_Msk             (_U_(0x1) << RTC_MODE1_CTRL_ENABLE_Pos)              /**< (RTC_MODE1_CTRL) Enable Mask */
#define RTC_MODE1_CTRL_ENABLE(value)          (RTC_MODE1_CTRL_ENABLE_Msk & ((value) << RTC_MODE1_CTRL_ENABLE_Pos))
#define RTC_MODE1_CTRL_MODE_Pos               _U_(2)                                               /**< (RTC_MODE1_CTRL) Operating Mode Position */
#define RTC_MODE1_CTRL_MODE_Msk               (_U_(0x3) << RTC_MODE1_CTRL_MODE_Pos)                /**< (RTC_MODE1_CTRL) Operating Mode Mask */
#define RTC_MODE1_CTRL_MODE(value)            (RTC_MODE1_CTRL_MODE_Msk & ((value) << RTC_MODE1_CTRL_MODE_Pos))
#define   RTC_MODE1_CTRL_MODE_COUNT32_Val     _U_(0x0)                                             /**< (RTC_MODE1_CTRL) Mode 0: 32-bit Counter  */
#define   RTC_MODE1_CTRL_MODE_COUNT16_Val     _U_(0x1)                                             /**< (RTC_MODE1_CTRL) Mode 1: 16-bit Counter  */
#define   RTC_MODE1_CTRL_MODE_CLOCK_Val       _U_(0x2)                                             /**< (RTC_MODE1_CTRL) Mode 2: Clock/Calendar  */
#define RTC_MODE1_CTRL_MODE_COUNT32           (RTC_MODE1_CTRL_MODE_COUNT32_Val << RTC_MODE1_CTRL_MODE_Pos) /**< (RTC_MODE1_CTRL) Mode 0: 32-bit Counter Position  */
#define RTC_MODE1_CTRL_MODE_COUNT16           (RTC_MODE1_CTRL_MODE_COUNT16_Val << RTC_MODE1_CTRL_MODE_Pos) /**< (RTC_MODE1_CTRL) Mode 1: 16-bit Counter Position  */
#define RTC_MODE1_CTRL_MODE_CLOCK             (RTC_MODE1_CTRL_MODE_CLOCK_Val << RTC_MODE1_CTRL_MODE_Pos) /**< (RTC_MODE1_CTRL) Mode 2: Clock/Calendar Position  */
#define RTC_MODE1_CTRL_PRESCALER_Pos          _U_(8)                                               /**< (RTC_MODE1_CTRL) Prescaler Position */
#define RTC_MODE1_CTRL_PRESCALER_Msk          (_U_(0xF) << RTC_MODE1_CTRL_PRESCALER_Pos)           /**< (RTC_MODE1_CTRL) Prescaler Mask */
#define RTC_MODE1_CTRL_PRESCALER(value)       (RTC_MODE1_CTRL_PRESCALER_Msk & ((value) << RTC_MODE1_CTRL_PRESCALER_Pos))
#define   RTC_MODE1_CTRL_PRESCALER_DIV1_Val   _U_(0x0)                                             /**< (RTC_MODE1_CTRL) CLK_RTC_CNT = GCLK_RTC/1  */
#define   RTC_MODE1_CTRL_PRESCALER_DIV2_Val   _U_(0x1)                                             /**< (RTC_MODE1_CTRL) CLK_RTC_CNT = GCLK_RTC/2  */
#define   RTC_MODE1_CTRL_PRESCALER_DIV4_Val   _U_(0x2)                                             /**< (RTC_MODE1_CTRL) CLK_RTC_CNT = GCLK_RTC/4  */
#define   RTC_MODE1_CTRL_PRESCALER_DIV8_Val   _U_(0x3)                                             /**< (RTC_MODE1_CTRL) CLK_RTC_CNT = GCLK_RTC/8  */
#define   RTC_MODE1_CTRL_PRESCALER_DIV16_Val  _U_(0x4)                                             /**< (RTC_MODE1_CTRL) CLK_RTC_CNT = GCLK_RTC/16  */
#define   RTC_MODE1_CTRL_PRESCALER_DIV32_Val  _U_(0x5)                                             /**< (RTC_MODE1_CTRL) CLK_RTC_CNT = GCLK_RTC/32  */
#define   RTC_MODE1_CTRL_PRESCALER_DIV64_Val  _U_(0x6)                                             /**< (RTC_MODE1_CTRL) CLK_RTC_CNT = GCLK_RTC/64  */
#define   RTC_MODE1_CTRL_PRESCALER_DIV128_Val _U_(0x7)                                             /**< (RTC_MODE1_CTRL) CLK_RTC_CNT = GCLK_RTC/128  */
#define   RTC_MODE1_CTRL_PRESCALER_DIV256_Val _U_(0x8)                                             /**< (RTC_MODE1_CTRL) CLK_RTC_CNT = GCLK_RTC/256  */
#define   RTC_MODE1_CTRL_PRESCALER_DIV512_Val _U_(0x9)                                             /**< (RTC_MODE1_CTRL) CLK_RTC_CNT = GCLK_RTC/512  */
#define   RTC_MODE1_CTRL_PRESCALER_DIV1024_Val _U_(0xA)                                             /**< (RTC_MODE1_CTRL) CLK_RTC_CNT = GCLK_RTC/1024  */
#define RTC_MODE1_CTRL_PRESCALER_DIV1         (RTC_MODE1_CTRL_PRESCALER_DIV1_Val << RTC_MODE1_CTRL_PRESCALER_Pos) /**< (RTC_MODE1_CTRL) CLK_RTC_CNT = GCLK_RTC/1 Position  */
#define RTC_MODE1_CTRL_PRESCALER_DIV2         (RTC_MODE1_CTRL_PRESCALER_DIV2_Val << RTC_MODE1_CTRL_PRESCALER_Pos) /**< (RTC_MODE1_CTRL) CLK_RTC_CNT = GCLK_RTC/2 Position  */
#define RTC_MODE1_CTRL_PRESCALER_DIV4         (RTC_MODE1_CTRL_PRESCALER_DIV4_Val << RTC_MODE1_CTRL_PRESCALER_Pos) /**< (RTC_MODE1_CTRL) CLK_RTC_CNT = GCLK_RTC/4 Position  */
#define RTC_MODE1_CTRL_PRESCALER_DIV8         (RTC_MODE1_CTRL_PRESCALER_DIV8_Val << RTC_MODE1_CTRL_PRESCALER_Pos) /**< (RTC_MODE1_CTRL) CLK_RTC_CNT = GCLK_RTC/8 Position  */
#define RTC_MODE1_CTRL_PRESCALER_DIV16        (RTC_MODE1_CTRL_PRESCALER_DIV16_Val << RTC_MODE1_CTRL_PRESCALER_Pos) /**< (RTC_MODE1_CTRL) CLK_RTC_CNT = GCLK_RTC/16 Position  */
#define RTC_MODE1_CTRL_PRESCALER_DIV32        (RTC_MODE1_CTRL_PRESCALER_DIV32_Val << RTC_MODE1_CTRL_PRESCALER_Pos) /**< (RTC_MODE1_CTRL) CLK_RTC_CNT = GCLK_RTC/32 Position  */
#define RTC_MODE1_CTRL_PRESCALER_DIV64        (RTC_MODE1_CTRL_PRESCALER_DIV64_Val << RTC_MODE1_CTRL_PRESCALER_Pos) /**< (RTC_MODE1_CTRL) CLK_RTC_CNT = GCLK_RTC/64 Position  */
#define RTC_MODE1_CTRL_PRESCALER_DIV128       (RTC_MODE1_CTRL_PRESCALER_DIV128_Val << RTC_MODE1_CTRL_PRESCALER_Pos) /**< (RTC_MODE1_CTRL) CLK_RTC_CNT = GCLK_RTC/128 Position  */
#define RTC_MODE1_CTRL_PRESCALER_DIV256       (RTC_MODE1_CTRL_PRESCALER_DIV256_Val << RTC_MODE1_CTRL_PRESCALER_Pos) /**< (RTC_MODE1_CTRL) CLK_RTC_CNT = GCLK_RTC/256 Position  */
#define RTC_MODE1_CTRL_PRESCALER_DIV512       (RTC_MODE1_CTRL_PRESCALER_DIV512_Val << RTC_MODE1_CTRL_PRESCALER_Pos) /**< (RTC_MODE1_CTRL) CLK_RTC_CNT = GCLK_RTC/512 Position  */
#define RTC_MODE1_CTRL_PRESCALER_DIV1024      (RTC_MODE1_CTRL_PRESCALER_DIV1024_Val << RTC_MODE1_CTRL_PRESCALER_Pos) /**< (RTC_MODE1_CTRL) CLK_RTC_CNT = GCLK_RTC/1024 Position  */
#define RTC_MODE1_CTRL_Msk                    _U_(0x0F0F)                                          /**< (RTC_MODE1_CTRL) Register Mask  */


/* -------- RTC_MODE2_CTRL : (RTC Offset: 0x00) (R/W 16) MODE2 Control -------- */
#define RTC_MODE2_CTRL_RESETVALUE             _U_(0x00)                                            /**<  (RTC_MODE2_CTRL) MODE2 Control  Reset Value */

#define RTC_MODE2_CTRL_SWRST_Pos              _U_(0)                                               /**< (RTC_MODE2_CTRL) Software Reset Position */
#define RTC_MODE2_CTRL_SWRST_Msk              (_U_(0x1) << RTC_MODE2_CTRL_SWRST_Pos)               /**< (RTC_MODE2_CTRL) Software Reset Mask */
#define RTC_MODE2_CTRL_SWRST(value)           (RTC_MODE2_CTRL_SWRST_Msk & ((value) << RTC_MODE2_CTRL_SWRST_Pos))
#define RTC_MODE2_CTRL_ENABLE_Pos             _U_(1)                                               /**< (RTC_MODE2_CTRL) Enable Position */
#define RTC_MODE2_CTRL_ENABLE_Msk             (_U_(0x1) << RTC_MODE2_CTRL_ENABLE_Pos)              /**< (RTC_MODE2_CTRL) Enable Mask */
#define RTC_MODE2_CTRL_ENABLE(value)          (RTC_MODE2_CTRL_ENABLE_Msk & ((value) << RTC_MODE2_CTRL_ENABLE_Pos))
#define RTC_MODE2_CTRL_MODE_Pos               _U_(2)                                               /**< (RTC_MODE2_CTRL) Operating Mode Position */
#define RTC_MODE2_CTRL_MODE_Msk               (_U_(0x3) << RTC_MODE2_CTRL_MODE_Pos)                /**< (RTC_MODE2_CTRL) Operating Mode Mask */
#define RTC_MODE2_CTRL_MODE(value)            (RTC_MODE2_CTRL_MODE_Msk & ((value) << RTC_MODE2_CTRL_MODE_Pos))
#define   RTC_MODE2_CTRL_MODE_COUNT32_Val     _U_(0x0)                                             /**< (RTC_MODE2_CTRL) Mode 0: 32-bit Counter  */
#define   RTC_MODE2_CTRL_MODE_COUNT16_Val     _U_(0x1)                                             /**< (RTC_MODE2_CTRL) Mode 1: 16-bit Counter  */
#define   RTC_MODE2_CTRL_MODE_CLOCK_Val       _U_(0x2)                                             /**< (RTC_MODE2_CTRL) Mode 2: Clock/Calendar  */
#define RTC_MODE2_CTRL_MODE_COUNT32           (RTC_MODE2_CTRL_MODE_COUNT32_Val << RTC_MODE2_CTRL_MODE_Pos) /**< (RTC_MODE2_CTRL) Mode 0: 32-bit Counter Position  */
#define RTC_MODE2_CTRL_MODE_COUNT16           (RTC_MODE2_CTRL_MODE_COUNT16_Val << RTC_MODE2_CTRL_MODE_Pos) /**< (RTC_MODE2_CTRL) Mode 1: 16-bit Counter Position  */
#define RTC_MODE2_CTRL_MODE_CLOCK             (RTC_MODE2_CTRL_MODE_CLOCK_Val << RTC_MODE2_CTRL_MODE_Pos) /**< (RTC_MODE2_CTRL) Mode 2: Clock/Calendar Position  */
#define RTC_MODE2_CTRL_CLKREP_Pos             _U_(6)                                               /**< (RTC_MODE2_CTRL) Clock Representation Position */
#define RTC_MODE2_CTRL_CLKREP_Msk             (_U_(0x1) << RTC_MODE2_CTRL_CLKREP_Pos)              /**< (RTC_MODE2_CTRL) Clock Representation Mask */
#define RTC_MODE2_CTRL_CLKREP(value)          (RTC_MODE2_CTRL_CLKREP_Msk & ((value) << RTC_MODE2_CTRL_CLKREP_Pos))
#define RTC_MODE2_CTRL_MATCHCLR_Pos           _U_(7)                                               /**< (RTC_MODE2_CTRL) Clear on Match Position */
#define RTC_MODE2_CTRL_MATCHCLR_Msk           (_U_(0x1) << RTC_MODE2_CTRL_MATCHCLR_Pos)            /**< (RTC_MODE2_CTRL) Clear on Match Mask */
#define RTC_MODE2_CTRL_MATCHCLR(value)        (RTC_MODE2_CTRL_MATCHCLR_Msk & ((value) << RTC_MODE2_CTRL_MATCHCLR_Pos))
#define RTC_MODE2_CTRL_PRESCALER_Pos          _U_(8)                                               /**< (RTC_MODE2_CTRL) Prescaler Position */
#define RTC_MODE2_CTRL_PRESCALER_Msk          (_U_(0xF) << RTC_MODE2_CTRL_PRESCALER_Pos)           /**< (RTC_MODE2_CTRL) Prescaler Mask */
#define RTC_MODE2_CTRL_PRESCALER(value)       (RTC_MODE2_CTRL_PRESCALER_Msk & ((value) << RTC_MODE2_CTRL_PRESCALER_Pos))
#define   RTC_MODE2_CTRL_PRESCALER_DIV1_Val   _U_(0x0)                                             /**< (RTC_MODE2_CTRL) CLK_RTC_CNT = GCLK_RTC/1  */
#define   RTC_MODE2_CTRL_PRESCALER_DIV2_Val   _U_(0x1)                                             /**< (RTC_MODE2_CTRL) CLK_RTC_CNT = GCLK_RTC/2  */
#define   RTC_MODE2_CTRL_PRESCALER_DIV4_Val   _U_(0x2)                                             /**< (RTC_MODE2_CTRL) CLK_RTC_CNT = GCLK_RTC/4  */
#define   RTC_MODE2_CTRL_PRESCALER_DIV8_Val   _U_(0x3)                                             /**< (RTC_MODE2_CTRL) CLK_RTC_CNT = GCLK_RTC/8  */
#define   RTC_MODE2_CTRL_PRESCALER_DIV16_Val  _U_(0x4)                                             /**< (RTC_MODE2_CTRL) CLK_RTC_CNT = GCLK_RTC/16  */
#define   RTC_MODE2_CTRL_PRESCALER_DIV32_Val  _U_(0x5)                                             /**< (RTC_MODE2_CTRL) CLK_RTC_CNT = GCLK_RTC/32  */
#define   RTC_MODE2_CTRL_PRESCALER_DIV64_Val  _U_(0x6)                                             /**< (RTC_MODE2_CTRL) CLK_RTC_CNT = GCLK_RTC/64  */
#define   RTC_MODE2_CTRL_PRESCALER_DIV128_Val _U_(0x7)                                             /**< (RTC_MODE2_CTRL) CLK_RTC_CNT = GCLK_RTC/128  */
#define   RTC_MODE2_CTRL_PRESCALER_DIV256_Val _U_(0x8)                                             /**< (RTC_MODE2_CTRL) CLK_RTC_CNT = GCLK_RTC/256  */
#define   RTC_MODE2_CTRL_PRESCALER_DIV512_Val _U_(0x9)                                             /**< (RTC_MODE2_CTRL) CLK_RTC_CNT = GCLK_RTC/512  */
#define   RTC_MODE2_CTRL_PRESCALER_DIV1024_Val _U_(0xA)                                             /**< (RTC_MODE2_CTRL) CLK_RTC_CNT = GCLK_RTC/1024  */
#define RTC_MODE2_CTRL_PRESCALER_DIV1         (RTC_MODE2_CTRL_PRESCALER_DIV1_Val << RTC_MODE2_CTRL_PRESCALER_Pos) /**< (RTC_MODE2_CTRL) CLK_RTC_CNT = GCLK_RTC/1 Position  */
#define RTC_MODE2_CTRL_PRESCALER_DIV2         (RTC_MODE2_CTRL_PRESCALER_DIV2_Val << RTC_MODE2_CTRL_PRESCALER_Pos) /**< (RTC_MODE2_CTRL) CLK_RTC_CNT = GCLK_RTC/2 Position  */
#define RTC_MODE2_CTRL_PRESCALER_DIV4         (RTC_MODE2_CTRL_PRESCALER_DIV4_Val << RTC_MODE2_CTRL_PRESCALER_Pos) /**< (RTC_MODE2_CTRL) CLK_RTC_CNT = GCLK_RTC/4 Position  */
#define RTC_MODE2_CTRL_PRESCALER_DIV8         (RTC_MODE2_CTRL_PRESCALER_DIV8_Val << RTC_MODE2_CTRL_PRESCALER_Pos) /**< (RTC_MODE2_CTRL) CLK_RTC_CNT = GCLK_RTC/8 Position  */
#define RTC_MODE2_CTRL_PRESCALER_DIV16        (RTC_MODE2_CTRL_PRESCALER_DIV16_Val << RTC_MODE2_CTRL_PRESCALER_Pos) /**< (RTC_MODE2_CTRL) CLK_RTC_CNT = GCLK_RTC/16 Position  */
#define RTC_MODE2_CTRL_PRESCALER_DIV32        (RTC_MODE2_CTRL_PRESCALER_DIV32_Val << RTC_MODE2_CTRL_PRESCALER_Pos) /**< (RTC_MODE2_CTRL) CLK_RTC_CNT = GCLK_RTC/32 Position  */
#define RTC_MODE2_CTRL_PRESCALER_DIV64        (RTC_MODE2_CTRL_PRESCALER_DIV64_Val << RTC_MODE2_CTRL_PRESCALER_Pos) /**< (RTC_MODE2_CTRL) CLK_RTC_CNT = GCLK_RTC/64 Position  */
#define RTC_MODE2_CTRL_PRESCALER_DIV128       (RTC_MODE2_CTRL_PRESCALER_DIV128_Val << RTC_MODE2_CTRL_PRESCALER_Pos) /**< (RTC_MODE2_CTRL) CLK_RTC_CNT = GCLK_RTC/128 Position  */
#define RTC_MODE2_CTRL_PRESCALER_DIV256       (RTC_MODE2_CTRL_PRESCALER_DIV256_Val << RTC_MODE2_CTRL_PRESCALER_Pos) /**< (RTC_MODE2_CTRL) CLK_RTC_CNT = GCLK_RTC/256 Position  */
#define RTC_MODE2_CTRL_PRESCALER_DIV512       (RTC_MODE2_CTRL_PRESCALER_DIV512_Val << RTC_MODE2_CTRL_PRESCALER_Pos) /**< (RTC_MODE2_CTRL) CLK_RTC_CNT = GCLK_RTC/512 Position  */
#define RTC_MODE2_CTRL_PRESCALER_DIV1024      (RTC_MODE2_CTRL_PRESCALER_DIV1024_Val << RTC_MODE2_CTRL_PRESCALER_Pos) /**< (RTC_MODE2_CTRL) CLK_RTC_CNT = GCLK_RTC/1024 Position  */
#define RTC_MODE2_CTRL_Msk                    _U_(0x0FCF)                                          /**< (RTC_MODE2_CTRL) Register Mask  */


/* -------- RTC_READREQ : (RTC Offset: 0x02) (R/W 16) Read Request -------- */
#define RTC_READREQ_RESETVALUE                _U_(0x10)                                            /**<  (RTC_READREQ) Read Request  Reset Value */

#define RTC_READREQ_ADDR_Pos                  _U_(0)                                               /**< (RTC_READREQ) Address Position */
#define RTC_READREQ_ADDR_Msk                  (_U_(0x3F) << RTC_READREQ_ADDR_Pos)                  /**< (RTC_READREQ) Address Mask */
#define RTC_READREQ_ADDR(value)               (RTC_READREQ_ADDR_Msk & ((value) << RTC_READREQ_ADDR_Pos))
#define RTC_READREQ_RCONT_Pos                 _U_(14)                                              /**< (RTC_READREQ) Read Continuously Position */
#define RTC_READREQ_RCONT_Msk                 (_U_(0x1) << RTC_READREQ_RCONT_Pos)                  /**< (RTC_READREQ) Read Continuously Mask */
#define RTC_READREQ_RCONT(value)              (RTC_READREQ_RCONT_Msk & ((value) << RTC_READREQ_RCONT_Pos))
#define RTC_READREQ_RREQ_Pos                  _U_(15)                                              /**< (RTC_READREQ) Read Request Position */
#define RTC_READREQ_RREQ_Msk                  (_U_(0x1) << RTC_READREQ_RREQ_Pos)                   /**< (RTC_READREQ) Read Request Mask */
#define RTC_READREQ_RREQ(value)               (RTC_READREQ_RREQ_Msk & ((value) << RTC_READREQ_RREQ_Pos))
#define RTC_READREQ_Msk                       _U_(0xC03F)                                          /**< (RTC_READREQ) Register Mask  */


/* -------- RTC_MODE0_EVCTRL : (RTC Offset: 0x04) (R/W 16) MODE0 Event Control -------- */
#define RTC_MODE0_EVCTRL_RESETVALUE           _U_(0x00)                                            /**<  (RTC_MODE0_EVCTRL) MODE0 Event Control  Reset Value */

#define RTC_MODE0_EVCTRL_PEREO0_Pos           _U_(0)                                               /**< (RTC_MODE0_EVCTRL) Periodic Interval 0 Event Output Enable Position */
#define RTC_MODE0_EVCTRL_PEREO0_Msk           (_U_(0x1) << RTC_MODE0_EVCTRL_PEREO0_Pos)            /**< (RTC_MODE0_EVCTRL) Periodic Interval 0 Event Output Enable Mask */
#define RTC_MODE0_EVCTRL_PEREO0(value)        (RTC_MODE0_EVCTRL_PEREO0_Msk & ((value) << RTC_MODE0_EVCTRL_PEREO0_Pos))
#define RTC_MODE0_EVCTRL_PEREO1_Pos           _U_(1)                                               /**< (RTC_MODE0_EVCTRL) Periodic Interval 1 Event Output Enable Position */
#define RTC_MODE0_EVCTRL_PEREO1_Msk           (_U_(0x1) << RTC_MODE0_EVCTRL_PEREO1_Pos)            /**< (RTC_MODE0_EVCTRL) Periodic Interval 1 Event Output Enable Mask */
#define RTC_MODE0_EVCTRL_PEREO1(value)        (RTC_MODE0_EVCTRL_PEREO1_Msk & ((value) << RTC_MODE0_EVCTRL_PEREO1_Pos))
#define RTC_MODE0_EVCTRL_PEREO2_Pos           _U_(2)                                               /**< (RTC_MODE0_EVCTRL) Periodic Interval 2 Event Output Enable Position */
#define RTC_MODE0_EVCTRL_PEREO2_Msk           (_U_(0x1) << RTC_MODE0_EVCTRL_PEREO2_Pos)            /**< (RTC_MODE0_EVCTRL) Periodic Interval 2 Event Output Enable Mask */
#define RTC_MODE0_EVCTRL_PEREO2(value)        (RTC_MODE0_EVCTRL_PEREO2_Msk & ((value) << RTC_MODE0_EVCTRL_PEREO2_Pos))
#define RTC_MODE0_EVCTRL_PEREO3_Pos           _U_(3)                                               /**< (RTC_MODE0_EVCTRL) Periodic Interval 3 Event Output Enable Position */
#define RTC_MODE0_EVCTRL_PEREO3_Msk           (_U_(0x1) << RTC_MODE0_EVCTRL_PEREO3_Pos)            /**< (RTC_MODE0_EVCTRL) Periodic Interval 3 Event Output Enable Mask */
#define RTC_MODE0_EVCTRL_PEREO3(value)        (RTC_MODE0_EVCTRL_PEREO3_Msk & ((value) << RTC_MODE0_EVCTRL_PEREO3_Pos))
#define RTC_MODE0_EVCTRL_PEREO4_Pos           _U_(4)                                               /**< (RTC_MODE0_EVCTRL) Periodic Interval 4 Event Output Enable Position */
#define RTC_MODE0_EVCTRL_PEREO4_Msk           (_U_(0x1) << RTC_MODE0_EVCTRL_PEREO4_Pos)            /**< (RTC_MODE0_EVCTRL) Periodic Interval 4 Event Output Enable Mask */
#define RTC_MODE0_EVCTRL_PEREO4(value)        (RTC_MODE0_EVCTRL_PEREO4_Msk & ((value) << RTC_MODE0_EVCTRL_PEREO4_Pos))
#define RTC_MODE0_EVCTRL_PEREO5_Pos           _U_(5)                                               /**< (RTC_MODE0_EVCTRL) Periodic Interval 5 Event Output Enable Position */
#define RTC_MODE0_EVCTRL_PEREO5_Msk           (_U_(0x1) << RTC_MODE0_EVCTRL_PEREO5_Pos)            /**< (RTC_MODE0_EVCTRL) Periodic Interval 5 Event Output Enable Mask */
#define RTC_MODE0_EVCTRL_PEREO5(value)        (RTC_MODE0_EVCTRL_PEREO5_Msk & ((value) << RTC_MODE0_EVCTRL_PEREO5_Pos))
#define RTC_MODE0_EVCTRL_PEREO6_Pos           _U_(6)                                               /**< (RTC_MODE0_EVCTRL) Periodic Interval 6 Event Output Enable Position */
#define RTC_MODE0_EVCTRL_PEREO6_Msk           (_U_(0x1) << RTC_MODE0_EVCTRL_PEREO6_Pos)            /**< (RTC_MODE0_EVCTRL) Periodic Interval 6 Event Output Enable Mask */
#define RTC_MODE0_EVCTRL_PEREO6(value)        (RTC_MODE0_EVCTRL_PEREO6_Msk & ((value) << RTC_MODE0_EVCTRL_PEREO6_Pos))
#define RTC_MODE0_EVCTRL_PEREO7_Pos           _U_(7)                                               /**< (RTC_MODE0_EVCTRL) Periodic Interval 7 Event Output Enable Position */
#define RTC_MODE0_EVCTRL_PEREO7_Msk           (_U_(0x1) << RTC_MODE0_EVCTRL_PEREO7_Pos)            /**< (RTC_MODE0_EVCTRL) Periodic Interval 7 Event Output Enable Mask */
#define RTC_MODE0_EVCTRL_PEREO7(value)        (RTC_MODE0_EVCTRL_PEREO7_Msk & ((value) << RTC_MODE0_EVCTRL_PEREO7_Pos))
#define RTC_MODE0_EVCTRL_CMPEO0_Pos           _U_(8)                                               /**< (RTC_MODE0_EVCTRL) Compare 0 Event Output Enable Position */
#define RTC_MODE0_EVCTRL_CMPEO0_Msk           (_U_(0x1) << RTC_MODE0_EVCTRL_CMPEO0_Pos)            /**< (RTC_MODE0_EVCTRL) Compare 0 Event Output Enable Mask */
#define RTC_MODE0_EVCTRL_CMPEO0(value)        (RTC_MODE0_EVCTRL_CMPEO0_Msk & ((value) << RTC_MODE0_EVCTRL_CMPEO0_Pos))
#define RTC_MODE0_EVCTRL_OVFEO_Pos            _U_(15)                                              /**< (RTC_MODE0_EVCTRL) Overflow Event Output Enable Position */
#define RTC_MODE0_EVCTRL_OVFEO_Msk            (_U_(0x1) << RTC_MODE0_EVCTRL_OVFEO_Pos)             /**< (RTC_MODE0_EVCTRL) Overflow Event Output Enable Mask */
#define RTC_MODE0_EVCTRL_OVFEO(value)         (RTC_MODE0_EVCTRL_OVFEO_Msk & ((value) << RTC_MODE0_EVCTRL_OVFEO_Pos))
#define RTC_MODE0_EVCTRL_Msk                  _U_(0x81FF)                                          /**< (RTC_MODE0_EVCTRL) Register Mask  */

#define RTC_MODE0_EVCTRL_PEREO_Pos            _U_(0)                                               /**< (RTC_MODE0_EVCTRL Position) Periodic Interval x Event Output Enable */
#define RTC_MODE0_EVCTRL_PEREO_Msk            (_U_(0xFF) << RTC_MODE0_EVCTRL_PEREO_Pos)            /**< (RTC_MODE0_EVCTRL Mask) PEREO */
#define RTC_MODE0_EVCTRL_PEREO(value)         (RTC_MODE0_EVCTRL_PEREO_Msk & ((value) << RTC_MODE0_EVCTRL_PEREO_Pos)) 
#define RTC_MODE0_EVCTRL_CMPEO_Pos            _U_(8)                                               /**< (RTC_MODE0_EVCTRL Position) Compare x Event Output Enable */
#define RTC_MODE0_EVCTRL_CMPEO_Msk            (_U_(0x1) << RTC_MODE0_EVCTRL_CMPEO_Pos)             /**< (RTC_MODE0_EVCTRL Mask) CMPEO */
#define RTC_MODE0_EVCTRL_CMPEO(value)         (RTC_MODE0_EVCTRL_CMPEO_Msk & ((value) << RTC_MODE0_EVCTRL_CMPEO_Pos)) 

/* -------- RTC_MODE1_EVCTRL : (RTC Offset: 0x04) (R/W 16) MODE1 Event Control -------- */
#define RTC_MODE1_EVCTRL_RESETVALUE           _U_(0x00)                                            /**<  (RTC_MODE1_EVCTRL) MODE1 Event Control  Reset Value */

#define RTC_MODE1_EVCTRL_PEREO0_Pos           _U_(0)                                               /**< (RTC_MODE1_EVCTRL) Periodic Interval 0 Event Output Enable Position */
#define RTC_MODE1_EVCTRL_PEREO0_Msk           (_U_(0x1) << RTC_MODE1_EVCTRL_PEREO0_Pos)            /**< (RTC_MODE1_EVCTRL) Periodic Interval 0 Event Output Enable Mask */
#define RTC_MODE1_EVCTRL_PEREO0(value)        (RTC_MODE1_EVCTRL_PEREO0_Msk & ((value) << RTC_MODE1_EVCTRL_PEREO0_Pos))
#define RTC_MODE1_EVCTRL_PEREO1_Pos           _U_(1)                                               /**< (RTC_MODE1_EVCTRL) Periodic Interval 1 Event Output Enable Position */
#define RTC_MODE1_EVCTRL_PEREO1_Msk           (_U_(0x1) << RTC_MODE1_EVCTRL_PEREO1_Pos)            /**< (RTC_MODE1_EVCTRL) Periodic Interval 1 Event Output Enable Mask */
#define RTC_MODE1_EVCTRL_PEREO1(value)        (RTC_MODE1_EVCTRL_PEREO1_Msk & ((value) << RTC_MODE1_EVCTRL_PEREO1_Pos))
#define RTC_MODE1_EVCTRL_PEREO2_Pos           _U_(2)                                               /**< (RTC_MODE1_EVCTRL) Periodic Interval 2 Event Output Enable Position */
#define RTC_MODE1_EVCTRL_PEREO2_Msk           (_U_(0x1) << RTC_MODE1_EVCTRL_PEREO2_Pos)            /**< (RTC_MODE1_EVCTRL) Periodic Interval 2 Event Output Enable Mask */
#define RTC_MODE1_EVCTRL_PEREO2(value)        (RTC_MODE1_EVCTRL_PEREO2_Msk & ((value) << RTC_MODE1_EVCTRL_PEREO2_Pos))
#define RTC_MODE1_EVCTRL_PEREO3_Pos           _U_(3)                                               /**< (RTC_MODE1_EVCTRL) Periodic Interval 3 Event Output Enable Position */
#define RTC_MODE1_EVCTRL_PEREO3_Msk           (_U_(0x1) << RTC_MODE1_EVCTRL_PEREO3_Pos)            /**< (RTC_MODE1_EVCTRL) Periodic Interval 3 Event Output Enable Mask */
#define RTC_MODE1_EVCTRL_PEREO3(value)        (RTC_MODE1_EVCTRL_PEREO3_Msk & ((value) << RTC_MODE1_EVCTRL_PEREO3_Pos))
#define RTC_MODE1_EVCTRL_PEREO4_Pos           _U_(4)                                               /**< (RTC_MODE1_EVCTRL) Periodic Interval 4 Event Output Enable Position */
#define RTC_MODE1_EVCTRL_PEREO4_Msk           (_U_(0x1) << RTC_MODE1_EVCTRL_PEREO4_Pos)            /**< (RTC_MODE1_EVCTRL) Periodic Interval 4 Event Output Enable Mask */
#define RTC_MODE1_EVCTRL_PEREO4(value)        (RTC_MODE1_EVCTRL_PEREO4_Msk & ((value) << RTC_MODE1_EVCTRL_PEREO4_Pos))
#define RTC_MODE1_EVCTRL_PEREO5_Pos           _U_(5)                                               /**< (RTC_MODE1_EVCTRL) Periodic Interval 5 Event Output Enable Position */
#define RTC_MODE1_EVCTRL_PEREO5_Msk           (_U_(0x1) << RTC_MODE1_EVCTRL_PEREO5_Pos)            /**< (RTC_MODE1_EVCTRL) Periodic Interval 5 Event Output Enable Mask */
#define RTC_MODE1_EVCTRL_PEREO5(value)        (RTC_MODE1_EVCTRL_PEREO5_Msk & ((value) << RTC_MODE1_EVCTRL_PEREO5_Pos))
#define RTC_MODE1_EVCTRL_PEREO6_Pos           _U_(6)                                               /**< (RTC_MODE1_EVCTRL) Periodic Interval 6 Event Output Enable Position */
#define RTC_MODE1_EVCTRL_PEREO6_Msk           (_U_(0x1) << RTC_MODE1_EVCTRL_PEREO6_Pos)            /**< (RTC_MODE1_EVCTRL) Periodic Interval 6 Event Output Enable Mask */
#define RTC_MODE1_EVCTRL_PEREO6(value)        (RTC_MODE1_EVCTRL_PEREO6_Msk & ((value) << RTC_MODE1_EVCTRL_PEREO6_Pos))
#define RTC_MODE1_EVCTRL_PEREO7_Pos           _U_(7)                                               /**< (RTC_MODE1_EVCTRL) Periodic Interval 7 Event Output Enable Position */
#define RTC_MODE1_EVCTRL_PEREO7_Msk           (_U_(0x1) << RTC_MODE1_EVCTRL_PEREO7_Pos)            /**< (RTC_MODE1_EVCTRL) Periodic Interval 7 Event Output Enable Mask */
#define RTC_MODE1_EVCTRL_PEREO7(value)        (RTC_MODE1_EVCTRL_PEREO7_Msk & ((value) << RTC_MODE1_EVCTRL_PEREO7_Pos))
#define RTC_MODE1_EVCTRL_CMPEO0_Pos           _U_(8)                                               /**< (RTC_MODE1_EVCTRL) Compare 0 Event Output Enable Position */
#define RTC_MODE1_EVCTRL_CMPEO0_Msk           (_U_(0x1) << RTC_MODE1_EVCTRL_CMPEO0_Pos)            /**< (RTC_MODE1_EVCTRL) Compare 0 Event Output Enable Mask */
#define RTC_MODE1_EVCTRL_CMPEO0(value)        (RTC_MODE1_EVCTRL_CMPEO0_Msk & ((value) << RTC_MODE1_EVCTRL_CMPEO0_Pos))
#define RTC_MODE1_EVCTRL_CMPEO1_Pos           _U_(9)                                               /**< (RTC_MODE1_EVCTRL) Compare 1 Event Output Enable Position */
#define RTC_MODE1_EVCTRL_CMPEO1_Msk           (_U_(0x1) << RTC_MODE1_EVCTRL_CMPEO1_Pos)            /**< (RTC_MODE1_EVCTRL) Compare 1 Event Output Enable Mask */
#define RTC_MODE1_EVCTRL_CMPEO1(value)        (RTC_MODE1_EVCTRL_CMPEO1_Msk & ((value) << RTC_MODE1_EVCTRL_CMPEO1_Pos))
#define RTC_MODE1_EVCTRL_OVFEO_Pos            _U_(15)                                              /**< (RTC_MODE1_EVCTRL) Overflow Event Output Enable Position */
#define RTC_MODE1_EVCTRL_OVFEO_Msk            (_U_(0x1) << RTC_MODE1_EVCTRL_OVFEO_Pos)             /**< (RTC_MODE1_EVCTRL) Overflow Event Output Enable Mask */
#define RTC_MODE1_EVCTRL_OVFEO(value)         (RTC_MODE1_EVCTRL_OVFEO_Msk & ((value) << RTC_MODE1_EVCTRL_OVFEO_Pos))
#define RTC_MODE1_EVCTRL_Msk                  _U_(0x83FF)                                          /**< (RTC_MODE1_EVCTRL) Register Mask  */

#define RTC_MODE1_EVCTRL_PEREO_Pos            _U_(0)                                               /**< (RTC_MODE1_EVCTRL Position) Periodic Interval x Event Output Enable */
#define RTC_MODE1_EVCTRL_PEREO_Msk            (_U_(0xFF) << RTC_MODE1_EVCTRL_PEREO_Pos)            /**< (RTC_MODE1_EVCTRL Mask) PEREO */
#define RTC_MODE1_EVCTRL_PEREO(value)         (RTC_MODE1_EVCTRL_PEREO_Msk & ((value) << RTC_MODE1_EVCTRL_PEREO_Pos)) 
#define RTC_MODE1_EVCTRL_CMPEO_Pos            _U_(8)                                               /**< (RTC_MODE1_EVCTRL Position) Compare x Event Output Enable */
#define RTC_MODE1_EVCTRL_CMPEO_Msk            (_U_(0x3) << RTC_MODE1_EVCTRL_CMPEO_Pos)             /**< (RTC_MODE1_EVCTRL Mask) CMPEO */
#define RTC_MODE1_EVCTRL_CMPEO(value)         (RTC_MODE1_EVCTRL_CMPEO_Msk & ((value) << RTC_MODE1_EVCTRL_CMPEO_Pos)) 

/* -------- RTC_MODE2_EVCTRL : (RTC Offset: 0x04) (R/W 16) MODE2 Event Control -------- */
#define RTC_MODE2_EVCTRL_RESETVALUE           _U_(0x00)                                            /**<  (RTC_MODE2_EVCTRL) MODE2 Event Control  Reset Value */

#define RTC_MODE2_EVCTRL_PEREO0_Pos           _U_(0)                                               /**< (RTC_MODE2_EVCTRL) Periodic Interval 0 Event Output Enable Position */
#define RTC_MODE2_EVCTRL_PEREO0_Msk           (_U_(0x1) << RTC_MODE2_EVCTRL_PEREO0_Pos)            /**< (RTC_MODE2_EVCTRL) Periodic Interval 0 Event Output Enable Mask */
#define RTC_MODE2_EVCTRL_PEREO0(value)        (RTC_MODE2_EVCTRL_PEREO0_Msk & ((value) << RTC_MODE2_EVCTRL_PEREO0_Pos))
#define RTC_MODE2_EVCTRL_PEREO1_Pos           _U_(1)                                               /**< (RTC_MODE2_EVCTRL) Periodic Interval 1 Event Output Enable Position */
#define RTC_MODE2_EVCTRL_PEREO1_Msk           (_U_(0x1) << RTC_MODE2_EVCTRL_PEREO1_Pos)            /**< (RTC_MODE2_EVCTRL) Periodic Interval 1 Event Output Enable Mask */
#define RTC_MODE2_EVCTRL_PEREO1(value)        (RTC_MODE2_EVCTRL_PEREO1_Msk & ((value) << RTC_MODE2_EVCTRL_PEREO1_Pos))
#define RTC_MODE2_EVCTRL_PEREO2_Pos           _U_(2)                                               /**< (RTC_MODE2_EVCTRL) Periodic Interval 2 Event Output Enable Position */
#define RTC_MODE2_EVCTRL_PEREO2_Msk           (_U_(0x1) << RTC_MODE2_EVCTRL_PEREO2_Pos)            /**< (RTC_MODE2_EVCTRL) Periodic Interval 2 Event Output Enable Mask */
#define RTC_MODE2_EVCTRL_PEREO2(value)        (RTC_MODE2_EVCTRL_PEREO2_Msk & ((value) << RTC_MODE2_EVCTRL_PEREO2_Pos))
#define RTC_MODE2_EVCTRL_PEREO3_Pos           _U_(3)                                               /**< (RTC_MODE2_EVCTRL) Periodic Interval 3 Event Output Enable Position */
#define RTC_MODE2_EVCTRL_PEREO3_Msk           (_U_(0x1) << RTC_MODE2_EVCTRL_PEREO3_Pos)            /**< (RTC_MODE2_EVCTRL) Periodic Interval 3 Event Output Enable Mask */
#define RTC_MODE2_EVCTRL_PEREO3(value)        (RTC_MODE2_EVCTRL_PEREO3_Msk & ((value) << RTC_MODE2_EVCTRL_PEREO3_Pos))
#define RTC_MODE2_EVCTRL_PEREO4_Pos           _U_(4)                                               /**< (RTC_MODE2_EVCTRL) Periodic Interval 4 Event Output Enable Position */
#define RTC_MODE2_EVCTRL_PEREO4_Msk           (_U_(0x1) << RTC_MODE2_EVCTRL_PEREO4_Pos)            /**< (RTC_MODE2_EVCTRL) Periodic Interval 4 Event Output Enable Mask */
#define RTC_MODE2_EVCTRL_PEREO4(value)        (RTC_MODE2_EVCTRL_PEREO4_Msk & ((value) << RTC_MODE2_EVCTRL_PEREO4_Pos))
#define RTC_MODE2_EVCTRL_PEREO5_Pos           _U_(5)                                               /**< (RTC_MODE2_EVCTRL) Periodic Interval 5 Event Output Enable Position */
#define RTC_MODE2_EVCTRL_PEREO5_Msk           (_U_(0x1) << RTC_MODE2_EVCTRL_PEREO5_Pos)            /**< (RTC_MODE2_EVCTRL) Periodic Interval 5 Event Output Enable Mask */
#define RTC_MODE2_EVCTRL_PEREO5(value)        (RTC_MODE2_EVCTRL_PEREO5_Msk & ((value) << RTC_MODE2_EVCTRL_PEREO5_Pos))
#define RTC_MODE2_EVCTRL_PEREO6_Pos           _U_(6)                                               /**< (RTC_MODE2_EVCTRL) Periodic Interval 6 Event Output Enable Position */
#define RTC_MODE2_EVCTRL_PEREO6_Msk           (_U_(0x1) << RTC_MODE2_EVCTRL_PEREO6_Pos)            /**< (RTC_MODE2_EVCTRL) Periodic Interval 6 Event Output Enable Mask */
#define RTC_MODE2_EVCTRL_PEREO6(value)        (RTC_MODE2_EVCTRL_PEREO6_Msk & ((value) << RTC_MODE2_EVCTRL_PEREO6_Pos))
#define RTC_MODE2_EVCTRL_PEREO7_Pos           _U_(7)                                               /**< (RTC_MODE2_EVCTRL) Periodic Interval 7 Event Output Enable Position */
#define RTC_MODE2_EVCTRL_PEREO7_Msk           (_U_(0x1) << RTC_MODE2_EVCTRL_PEREO7_Pos)            /**< (RTC_MODE2_EVCTRL) Periodic Interval 7 Event Output Enable Mask */
#define RTC_MODE2_EVCTRL_PEREO7(value)        (RTC_MODE2_EVCTRL_PEREO7_Msk & ((value) << RTC_MODE2_EVCTRL_PEREO7_Pos))
#define RTC_MODE2_EVCTRL_ALARMEO0_Pos         _U_(8)                                               /**< (RTC_MODE2_EVCTRL) Alarm 0 Event Output Enable Position */
#define RTC_MODE2_EVCTRL_ALARMEO0_Msk         (_U_(0x1) << RTC_MODE2_EVCTRL_ALARMEO0_Pos)          /**< (RTC_MODE2_EVCTRL) Alarm 0 Event Output Enable Mask */
#define RTC_MODE2_EVCTRL_ALARMEO0(value)      (RTC_MODE2_EVCTRL_ALARMEO0_Msk & ((value) << RTC_MODE2_EVCTRL_ALARMEO0_Pos))
#define RTC_MODE2_EVCTRL_OVFEO_Pos            _U_(15)                                              /**< (RTC_MODE2_EVCTRL) Overflow Event Output Enable Position */
#define RTC_MODE2_EVCTRL_OVFEO_Msk            (_U_(0x1) << RTC_MODE2_EVCTRL_OVFEO_Pos)             /**< (RTC_MODE2_EVCTRL) Overflow Event Output Enable Mask */
#define RTC_MODE2_EVCTRL_OVFEO(value)         (RTC_MODE2_EVCTRL_OVFEO_Msk & ((value) << RTC_MODE2_EVCTRL_OVFEO_Pos))
#define RTC_MODE2_EVCTRL_Msk                  _U_(0x81FF)                                          /**< (RTC_MODE2_EVCTRL) Register Mask  */

#define RTC_MODE2_EVCTRL_PEREO_Pos            _U_(0)                                               /**< (RTC_MODE2_EVCTRL Position) Periodic Interval x Event Output Enable */
#define RTC_MODE2_EVCTRL_PEREO_Msk            (_U_(0xFF) << RTC_MODE2_EVCTRL_PEREO_Pos)            /**< (RTC_MODE2_EVCTRL Mask) PEREO */
#define RTC_MODE2_EVCTRL_PEREO(value)         (RTC_MODE2_EVCTRL_PEREO_Msk & ((value) << RTC_MODE2_EVCTRL_PEREO_Pos)) 
#define RTC_MODE2_EVCTRL_ALARMEO_Pos          _U_(8)                                               /**< (RTC_MODE2_EVCTRL Position) Alarm x Event Output Enable */
#define RTC_MODE2_EVCTRL_ALARMEO_Msk          (_U_(0x1) << RTC_MODE2_EVCTRL_ALARMEO_Pos)           /**< (RTC_MODE2_EVCTRL Mask) ALARMEO */
#define RTC_MODE2_EVCTRL_ALARMEO(value)       (RTC_MODE2_EVCTRL_ALARMEO_Msk & ((value) << RTC_MODE2_EVCTRL_ALARMEO_Pos)) 

/* -------- RTC_MODE0_INTENCLR : (RTC Offset: 0x06) (R/W 8) MODE0 Interrupt Enable Clear -------- */
#define RTC_MODE0_INTENCLR_RESETVALUE         _U_(0x00)                                            /**<  (RTC_MODE0_INTENCLR) MODE0 Interrupt Enable Clear  Reset Value */

#define RTC_MODE0_INTENCLR_CMP0_Pos           _U_(0)                                               /**< (RTC_MODE0_INTENCLR) Compare 0 Interrupt Enable Position */
#define RTC_MODE0_INTENCLR_CMP0_Msk           (_U_(0x1) << RTC_MODE0_INTENCLR_CMP0_Pos)            /**< (RTC_MODE0_INTENCLR) Compare 0 Interrupt Enable Mask */
#define RTC_MODE0_INTENCLR_CMP0(value)        (RTC_MODE0_INTENCLR_CMP0_Msk & ((value) << RTC_MODE0_INTENCLR_CMP0_Pos))
#define RTC_MODE0_INTENCLR_SYNCRDY_Pos        _U_(6)                                               /**< (RTC_MODE0_INTENCLR) Synchronization Ready Interrupt Enable Position */
#define RTC_MODE0_INTENCLR_SYNCRDY_Msk        (_U_(0x1) << RTC_MODE0_INTENCLR_SYNCRDY_Pos)         /**< (RTC_MODE0_INTENCLR) Synchronization Ready Interrupt Enable Mask */
#define RTC_MODE0_INTENCLR_SYNCRDY(value)     (RTC_MODE0_INTENCLR_SYNCRDY_Msk & ((value) << RTC_MODE0_INTENCLR_SYNCRDY_Pos))
#define RTC_MODE0_INTENCLR_OVF_Pos            _U_(7)                                               /**< (RTC_MODE0_INTENCLR) Overflow Interrupt Enable Position */
#define RTC_MODE0_INTENCLR_OVF_Msk            (_U_(0x1) << RTC_MODE0_INTENCLR_OVF_Pos)             /**< (RTC_MODE0_INTENCLR) Overflow Interrupt Enable Mask */
#define RTC_MODE0_INTENCLR_OVF(value)         (RTC_MODE0_INTENCLR_OVF_Msk & ((value) << RTC_MODE0_INTENCLR_OVF_Pos))
#define RTC_MODE0_INTENCLR_Msk                _U_(0xC1)                                            /**< (RTC_MODE0_INTENCLR) Register Mask  */

#define RTC_MODE0_INTENCLR_CMP_Pos            _U_(0)                                               /**< (RTC_MODE0_INTENCLR Position) Compare x Interrupt Enable */
#define RTC_MODE0_INTENCLR_CMP_Msk            (_U_(0x1) << RTC_MODE0_INTENCLR_CMP_Pos)             /**< (RTC_MODE0_INTENCLR Mask) CMP */
#define RTC_MODE0_INTENCLR_CMP(value)         (RTC_MODE0_INTENCLR_CMP_Msk & ((value) << RTC_MODE0_INTENCLR_CMP_Pos)) 

/* -------- RTC_MODE1_INTENCLR : (RTC Offset: 0x06) (R/W 8) MODE1 Interrupt Enable Clear -------- */
#define RTC_MODE1_INTENCLR_RESETVALUE         _U_(0x00)                                            /**<  (RTC_MODE1_INTENCLR) MODE1 Interrupt Enable Clear  Reset Value */

#define RTC_MODE1_INTENCLR_CMP0_Pos           _U_(0)                                               /**< (RTC_MODE1_INTENCLR) Compare 0 Interrupt Enable Position */
#define RTC_MODE1_INTENCLR_CMP0_Msk           (_U_(0x1) << RTC_MODE1_INTENCLR_CMP0_Pos)            /**< (RTC_MODE1_INTENCLR) Compare 0 Interrupt Enable Mask */
#define RTC_MODE1_INTENCLR_CMP0(value)        (RTC_MODE1_INTENCLR_CMP0_Msk & ((value) << RTC_MODE1_INTENCLR_CMP0_Pos))
#define RTC_MODE1_INTENCLR_CMP1_Pos           _U_(1)                                               /**< (RTC_MODE1_INTENCLR) Compare 1 Interrupt Enable Position */
#define RTC_MODE1_INTENCLR_CMP1_Msk           (_U_(0x1) << RTC_MODE1_INTENCLR_CMP1_Pos)            /**< (RTC_MODE1_INTENCLR) Compare 1 Interrupt Enable Mask */
#define RTC_MODE1_INTENCLR_CMP1(value)        (RTC_MODE1_INTENCLR_CMP1_Msk & ((value) << RTC_MODE1_INTENCLR_CMP1_Pos))
#define RTC_MODE1_INTENCLR_SYNCRDY_Pos        _U_(6)                                               /**< (RTC_MODE1_INTENCLR) Synchronization Ready Interrupt Enable Position */
#define RTC_MODE1_INTENCLR_SYNCRDY_Msk        (_U_(0x1) << RTC_MODE1_INTENCLR_SYNCRDY_Pos)         /**< (RTC_MODE1_INTENCLR) Synchronization Ready Interrupt Enable Mask */
#define RTC_MODE1_INTENCLR_SYNCRDY(value)     (RTC_MODE1_INTENCLR_SYNCRDY_Msk & ((value) << RTC_MODE1_INTENCLR_SYNCRDY_Pos))
#define RTC_MODE1_INTENCLR_OVF_Pos            _U_(7)                                               /**< (RTC_MODE1_INTENCLR) Overflow Interrupt Enable Position */
#define RTC_MODE1_INTENCLR_OVF_Msk            (_U_(0x1) << RTC_MODE1_INTENCLR_OVF_Pos)             /**< (RTC_MODE1_INTENCLR) Overflow Interrupt Enable Mask */
#define RTC_MODE1_INTENCLR_OVF(value)         (RTC_MODE1_INTENCLR_OVF_Msk & ((value) << RTC_MODE1_INTENCLR_OVF_Pos))
#define RTC_MODE1_INTENCLR_Msk                _U_(0xC3)                                            /**< (RTC_MODE1_INTENCLR) Register Mask  */

#define RTC_MODE1_INTENCLR_CMP_Pos            _U_(0)                                               /**< (RTC_MODE1_INTENCLR Position) Compare x Interrupt Enable */
#define RTC_MODE1_INTENCLR_CMP_Msk            (_U_(0x3) << RTC_MODE1_INTENCLR_CMP_Pos)             /**< (RTC_MODE1_INTENCLR Mask) CMP */
#define RTC_MODE1_INTENCLR_CMP(value)         (RTC_MODE1_INTENCLR_CMP_Msk & ((value) << RTC_MODE1_INTENCLR_CMP_Pos)) 

/* -------- RTC_MODE2_INTENCLR : (RTC Offset: 0x06) (R/W 8) MODE2 Interrupt Enable Clear -------- */
#define RTC_MODE2_INTENCLR_RESETVALUE         _U_(0x00)                                            /**<  (RTC_MODE2_INTENCLR) MODE2 Interrupt Enable Clear  Reset Value */

#define RTC_MODE2_INTENCLR_ALARM0_Pos         _U_(0)                                               /**< (RTC_MODE2_INTENCLR) Alarm 0 Interrupt Enable Position */
#define RTC_MODE2_INTENCLR_ALARM0_Msk         (_U_(0x1) << RTC_MODE2_INTENCLR_ALARM0_Pos)          /**< (RTC_MODE2_INTENCLR) Alarm 0 Interrupt Enable Mask */
#define RTC_MODE2_INTENCLR_ALARM0(value)      (RTC_MODE2_INTENCLR_ALARM0_Msk & ((value) << RTC_MODE2_INTENCLR_ALARM0_Pos))
#define RTC_MODE2_INTENCLR_SYNCRDY_Pos        _U_(6)                                               /**< (RTC_MODE2_INTENCLR) Synchronization Ready Interrupt Enable Position */
#define RTC_MODE2_INTENCLR_SYNCRDY_Msk        (_U_(0x1) << RTC_MODE2_INTENCLR_SYNCRDY_Pos)         /**< (RTC_MODE2_INTENCLR) Synchronization Ready Interrupt Enable Mask */
#define RTC_MODE2_INTENCLR_SYNCRDY(value)     (RTC_MODE2_INTENCLR_SYNCRDY_Msk & ((value) << RTC_MODE2_INTENCLR_SYNCRDY_Pos))
#define RTC_MODE2_INTENCLR_OVF_Pos            _U_(7)                                               /**< (RTC_MODE2_INTENCLR) Overflow Interrupt Enable Position */
#define RTC_MODE2_INTENCLR_OVF_Msk            (_U_(0x1) << RTC_MODE2_INTENCLR_OVF_Pos)             /**< (RTC_MODE2_INTENCLR) Overflow Interrupt Enable Mask */
#define RTC_MODE2_INTENCLR_OVF(value)         (RTC_MODE2_INTENCLR_OVF_Msk & ((value) << RTC_MODE2_INTENCLR_OVF_Pos))
#define RTC_MODE2_INTENCLR_Msk                _U_(0xC1)                                            /**< (RTC_MODE2_INTENCLR) Register Mask  */

#define RTC_MODE2_INTENCLR_ALARM_Pos          _U_(0)                                               /**< (RTC_MODE2_INTENCLR Position) Alarm x Interrupt Enable */
#define RTC_MODE2_INTENCLR_ALARM_Msk          (_U_(0x1) << RTC_MODE2_INTENCLR_ALARM_Pos)           /**< (RTC_MODE2_INTENCLR Mask) ALARM */
#define RTC_MODE2_INTENCLR_ALARM(value)       (RTC_MODE2_INTENCLR_ALARM_Msk & ((value) << RTC_MODE2_INTENCLR_ALARM_Pos)) 

/* -------- RTC_MODE0_INTENSET : (RTC Offset: 0x07) (R/W 8) MODE0 Interrupt Enable Set -------- */
#define RTC_MODE0_INTENSET_RESETVALUE         _U_(0x00)                                            /**<  (RTC_MODE0_INTENSET) MODE0 Interrupt Enable Set  Reset Value */

#define RTC_MODE0_INTENSET_CMP0_Pos           _U_(0)                                               /**< (RTC_MODE0_INTENSET) Compare 0 Interrupt Enable Position */
#define RTC_MODE0_INTENSET_CMP0_Msk           (_U_(0x1) << RTC_MODE0_INTENSET_CMP0_Pos)            /**< (RTC_MODE0_INTENSET) Compare 0 Interrupt Enable Mask */
#define RTC_MODE0_INTENSET_CMP0(value)        (RTC_MODE0_INTENSET_CMP0_Msk & ((value) << RTC_MODE0_INTENSET_CMP0_Pos))
#define RTC_MODE0_INTENSET_SYNCRDY_Pos        _U_(6)                                               /**< (RTC_MODE0_INTENSET) Synchronization Ready Interrupt Enable Position */
#define RTC_MODE0_INTENSET_SYNCRDY_Msk        (_U_(0x1) << RTC_MODE0_INTENSET_SYNCRDY_Pos)         /**< (RTC_MODE0_INTENSET) Synchronization Ready Interrupt Enable Mask */
#define RTC_MODE0_INTENSET_SYNCRDY(value)     (RTC_MODE0_INTENSET_SYNCRDY_Msk & ((value) << RTC_MODE0_INTENSET_SYNCRDY_Pos))
#define RTC_MODE0_INTENSET_OVF_Pos            _U_(7)                                               /**< (RTC_MODE0_INTENSET) Overflow Interrupt Enable Position */
#define RTC_MODE0_INTENSET_OVF_Msk            (_U_(0x1) << RTC_MODE0_INTENSET_OVF_Pos)             /**< (RTC_MODE0_INTENSET) Overflow Interrupt Enable Mask */
#define RTC_MODE0_INTENSET_OVF(value)         (RTC_MODE0_INTENSET_OVF_Msk & ((value) << RTC_MODE0_INTENSET_OVF_Pos))
#define RTC_MODE0_INTENSET_Msk                _U_(0xC1)                                            /**< (RTC_MODE0_INTENSET) Register Mask  */

#define RTC_MODE0_INTENSET_CMP_Pos            _U_(0)                                               /**< (RTC_MODE0_INTENSET Position) Compare x Interrupt Enable */
#define RTC_MODE0_INTENSET_CMP_Msk            (_U_(0x1) << RTC_MODE0_INTENSET_CMP_Pos)             /**< (RTC_MODE0_INTENSET Mask) CMP */
#define RTC_MODE0_INTENSET_CMP(value)         (RTC_MODE0_INTENSET_CMP_Msk & ((value) << RTC_MODE0_INTENSET_CMP_Pos)) 

/* -------- RTC_MODE1_INTENSET : (RTC Offset: 0x07) (R/W 8) MODE1 Interrupt Enable Set -------- */
#define RTC_MODE1_INTENSET_RESETVALUE         _U_(0x00)                                            /**<  (RTC_MODE1_INTENSET) MODE1 Interrupt Enable Set  Reset Value */

#define RTC_MODE1_INTENSET_CMP0_Pos           _U_(0)                                               /**< (RTC_MODE1_INTENSET) Compare 0 Interrupt Enable Position */
#define RTC_MODE1_INTENSET_CMP0_Msk           (_U_(0x1) << RTC_MODE1_INTENSET_CMP0_Pos)            /**< (RTC_MODE1_INTENSET) Compare 0 Interrupt Enable Mask */
#define RTC_MODE1_INTENSET_CMP0(value)        (RTC_MODE1_INTENSET_CMP0_Msk & ((value) << RTC_MODE1_INTENSET_CMP0_Pos))
#define RTC_MODE1_INTENSET_CMP1_Pos           _U_(1)                                               /**< (RTC_MODE1_INTENSET) Compare 1 Interrupt Enable Position */
#define RTC_MODE1_INTENSET_CMP1_Msk           (_U_(0x1) << RTC_MODE1_INTENSET_CMP1_Pos)            /**< (RTC_MODE1_INTENSET) Compare 1 Interrupt Enable Mask */
#define RTC_MODE1_INTENSET_CMP1(value)        (RTC_MODE1_INTENSET_CMP1_Msk & ((value) << RTC_MODE1_INTENSET_CMP1_Pos))
#define RTC_MODE1_INTENSET_SYNCRDY_Pos        _U_(6)                                               /**< (RTC_MODE1_INTENSET) Synchronization Ready Interrupt Enable Position */
#define RTC_MODE1_INTENSET_SYNCRDY_Msk        (_U_(0x1) << RTC_MODE1_INTENSET_SYNCRDY_Pos)         /**< (RTC_MODE1_INTENSET) Synchronization Ready Interrupt Enable Mask */
#define RTC_MODE1_INTENSET_SYNCRDY(value)     (RTC_MODE1_INTENSET_SYNCRDY_Msk & ((value) << RTC_MODE1_INTENSET_SYNCRDY_Pos))
#define RTC_MODE1_INTENSET_OVF_Pos            _U_(7)                                               /**< (RTC_MODE1_INTENSET) Overflow Interrupt Enable Position */
#define RTC_MODE1_INTENSET_OVF_Msk            (_U_(0x1) << RTC_MODE1_INTENSET_OVF_Pos)             /**< (RTC_MODE1_INTENSET) Overflow Interrupt Enable Mask */
#define RTC_MODE1_INTENSET_OVF(value)         (RTC_MODE1_INTENSET_OVF_Msk & ((value) << RTC_MODE1_INTENSET_OVF_Pos))
#define RTC_MODE1_INTENSET_Msk                _U_(0xC3)                                            /**< (RTC_MODE1_INTENSET) Register Mask  */

#define RTC_MODE1_INTENSET_CMP_Pos            _U_(0)                                               /**< (RTC_MODE1_INTENSET Position) Compare x Interrupt Enable */
#define RTC_MODE1_INTENSET_CMP_Msk            (_U_(0x3) << RTC_MODE1_INTENSET_CMP_Pos)             /**< (RTC_MODE1_INTENSET Mask) CMP */
#define RTC_MODE1_INTENSET_CMP(value)         (RTC_MODE1_INTENSET_CMP_Msk & ((value) << RTC_MODE1_INTENSET_CMP_Pos)) 

/* -------- RTC_MODE2_INTENSET : (RTC Offset: 0x07) (R/W 8) MODE2 Interrupt Enable Set -------- */
#define RTC_MODE2_INTENSET_RESETVALUE         _U_(0x00)                                            /**<  (RTC_MODE2_INTENSET) MODE2 Interrupt Enable Set  Reset Value */

#define RTC_MODE2_INTENSET_ALARM0_Pos         _U_(0)                                               /**< (RTC_MODE2_INTENSET) Alarm 0 Interrupt Enable Position */
#define RTC_MODE2_INTENSET_ALARM0_Msk         (_U_(0x1) << RTC_MODE2_INTENSET_ALARM0_Pos)          /**< (RTC_MODE2_INTENSET) Alarm 0 Interrupt Enable Mask */
#define RTC_MODE2_INTENSET_ALARM0(value)      (RTC_MODE2_INTENSET_ALARM0_Msk & ((value) << RTC_MODE2_INTENSET_ALARM0_Pos))
#define RTC_MODE2_INTENSET_SYNCRDY_Pos        _U_(6)                                               /**< (RTC_MODE2_INTENSET) Synchronization Ready Interrupt Enable Position */
#define RTC_MODE2_INTENSET_SYNCRDY_Msk        (_U_(0x1) << RTC_MODE2_INTENSET_SYNCRDY_Pos)         /**< (RTC_MODE2_INTENSET) Synchronization Ready Interrupt Enable Mask */
#define RTC_MODE2_INTENSET_SYNCRDY(value)     (RTC_MODE2_INTENSET_SYNCRDY_Msk & ((value) << RTC_MODE2_INTENSET_SYNCRDY_Pos))
#define RTC_MODE2_INTENSET_OVF_Pos            _U_(7)                                               /**< (RTC_MODE2_INTENSET) Overflow Interrupt Enable Position */
#define RTC_MODE2_INTENSET_OVF_Msk            (_U_(0x1) << RTC_MODE2_INTENSET_OVF_Pos)             /**< (RTC_MODE2_INTENSET) Overflow Interrupt Enable Mask */
#define RTC_MODE2_INTENSET_OVF(value)         (RTC_MODE2_INTENSET_OVF_Msk & ((value) << RTC_MODE2_INTENSET_OVF_Pos))
#define RTC_MODE2_INTENSET_Msk                _U_(0xC1)                                            /**< (RTC_MODE2_INTENSET) Register Mask  */

#define RTC_MODE2_INTENSET_ALARM_Pos          _U_(0)                                               /**< (RTC_MODE2_INTENSET Position) Alarm x Interrupt Enable */
#define RTC_MODE2_INTENSET_ALARM_Msk          (_U_(0x1) << RTC_MODE2_INTENSET_ALARM_Pos)           /**< (RTC_MODE2_INTENSET Mask) ALARM */
#define RTC_MODE2_INTENSET_ALARM(value)       (RTC_MODE2_INTENSET_ALARM_Msk & ((value) << RTC_MODE2_INTENSET_ALARM_Pos)) 

/* -------- RTC_MODE0_INTFLAG : (RTC Offset: 0x08) (R/W 8) MODE0 Interrupt Flag Status and Clear -------- */
#define RTC_MODE0_INTFLAG_RESETVALUE          _U_(0x00)                                            /**<  (RTC_MODE0_INTFLAG) MODE0 Interrupt Flag Status and Clear  Reset Value */

#define RTC_MODE0_INTFLAG_CMP0_Pos            _U_(0)                                               /**< (RTC_MODE0_INTFLAG) Compare 0 Position */
#define RTC_MODE0_INTFLAG_CMP0_Msk            (_U_(0x1) << RTC_MODE0_INTFLAG_CMP0_Pos)             /**< (RTC_MODE0_INTFLAG) Compare 0 Mask */
#define RTC_MODE0_INTFLAG_CMP0(value)         (RTC_MODE0_INTFLAG_CMP0_Msk & ((value) << RTC_MODE0_INTFLAG_CMP0_Pos))
#define RTC_MODE0_INTFLAG_SYNCRDY_Pos         _U_(6)                                               /**< (RTC_MODE0_INTFLAG) Synchronization Ready Position */
#define RTC_MODE0_INTFLAG_SYNCRDY_Msk         (_U_(0x1) << RTC_MODE0_INTFLAG_SYNCRDY_Pos)          /**< (RTC_MODE0_INTFLAG) Synchronization Ready Mask */
#define RTC_MODE0_INTFLAG_SYNCRDY(value)      (RTC_MODE0_INTFLAG_SYNCRDY_Msk & ((value) << RTC_MODE0_INTFLAG_SYNCRDY_Pos))
#define RTC_MODE0_INTFLAG_OVF_Pos             _U_(7)                                               /**< (RTC_MODE0_INTFLAG) Overflow Position */
#define RTC_MODE0_INTFLAG_OVF_Msk             (_U_(0x1) << RTC_MODE0_INTFLAG_OVF_Pos)              /**< (RTC_MODE0_INTFLAG) Overflow Mask */
#define RTC_MODE0_INTFLAG_OVF(value)          (RTC_MODE0_INTFLAG_OVF_Msk & ((value) << RTC_MODE0_INTFLAG_OVF_Pos))
#define RTC_MODE0_INTFLAG_Msk                 _U_(0xC1)                                            /**< (RTC_MODE0_INTFLAG) Register Mask  */

#define RTC_MODE0_INTFLAG_CMP_Pos             _U_(0)                                               /**< (RTC_MODE0_INTFLAG Position) Compare x */
#define RTC_MODE0_INTFLAG_CMP_Msk             (_U_(0x1) << RTC_MODE0_INTFLAG_CMP_Pos)              /**< (RTC_MODE0_INTFLAG Mask) CMP */
#define RTC_MODE0_INTFLAG_CMP(value)          (RTC_MODE0_INTFLAG_CMP_Msk & ((value) << RTC_MODE0_INTFLAG_CMP_Pos)) 

/* -------- RTC_MODE1_INTFLAG : (RTC Offset: 0x08) (R/W 8) MODE1 Interrupt Flag Status and Clear -------- */
#define RTC_MODE1_INTFLAG_RESETVALUE          _U_(0x00)                                            /**<  (RTC_MODE1_INTFLAG) MODE1 Interrupt Flag Status and Clear  Reset Value */

#define RTC_MODE1_INTFLAG_CMP0_Pos            _U_(0)                                               /**< (RTC_MODE1_INTFLAG) Compare 0 Position */
#define RTC_MODE1_INTFLAG_CMP0_Msk            (_U_(0x1) << RTC_MODE1_INTFLAG_CMP0_Pos)             /**< (RTC_MODE1_INTFLAG) Compare 0 Mask */
#define RTC_MODE1_INTFLAG_CMP0(value)         (RTC_MODE1_INTFLAG_CMP0_Msk & ((value) << RTC_MODE1_INTFLAG_CMP0_Pos))
#define RTC_MODE1_INTFLAG_CMP1_Pos            _U_(1)                                               /**< (RTC_MODE1_INTFLAG) Compare 1 Position */
#define RTC_MODE1_INTFLAG_CMP1_Msk            (_U_(0x1) << RTC_MODE1_INTFLAG_CMP1_Pos)             /**< (RTC_MODE1_INTFLAG) Compare 1 Mask */
#define RTC_MODE1_INTFLAG_CMP1(value)         (RTC_MODE1_INTFLAG_CMP1_Msk & ((value) << RTC_MODE1_INTFLAG_CMP1_Pos))
#define RTC_MODE1_INTFLAG_SYNCRDY_Pos         _U_(6)                                               /**< (RTC_MODE1_INTFLAG) Synchronization Ready Position */
#define RTC_MODE1_INTFLAG_SYNCRDY_Msk         (_U_(0x1) << RTC_MODE1_INTFLAG_SYNCRDY_Pos)          /**< (RTC_MODE1_INTFLAG) Synchronization Ready Mask */
#define RTC_MODE1_INTFLAG_SYNCRDY(value)      (RTC_MODE1_INTFLAG_SYNCRDY_Msk & ((value) << RTC_MODE1_INTFLAG_SYNCRDY_Pos))
#define RTC_MODE1_INTFLAG_OVF_Pos             _U_(7)                                               /**< (RTC_MODE1_INTFLAG) Overflow Position */
#define RTC_MODE1_INTFLAG_OVF_Msk             (_U_(0x1) << RTC_MODE1_INTFLAG_OVF_Pos)              /**< (RTC_MODE1_INTFLAG) Overflow Mask */
#define RTC_MODE1_INTFLAG_OVF(value)          (RTC_MODE1_INTFLAG_OVF_Msk & ((value) << RTC_MODE1_INTFLAG_OVF_Pos))
#define RTC_MODE1_INTFLAG_Msk                 _U_(0xC3)                                            /**< (RTC_MODE1_INTFLAG) Register Mask  */

#define RTC_MODE1_INTFLAG_CMP_Pos             _U_(0)                                               /**< (RTC_MODE1_INTFLAG Position) Compare x */
#define RTC_MODE1_INTFLAG_CMP_Msk             (_U_(0x3) << RTC_MODE1_INTFLAG_CMP_Pos)              /**< (RTC_MODE1_INTFLAG Mask) CMP */
#define RTC_MODE1_INTFLAG_CMP(value)          (RTC_MODE1_INTFLAG_CMP_Msk & ((value) << RTC_MODE1_INTFLAG_CMP_Pos)) 

/* -------- RTC_MODE2_INTFLAG : (RTC Offset: 0x08) (R/W 8) MODE2 Interrupt Flag Status and Clear -------- */
#define RTC_MODE2_INTFLAG_RESETVALUE          _U_(0x00)                                            /**<  (RTC_MODE2_INTFLAG) MODE2 Interrupt Flag Status and Clear  Reset Value */

#define RTC_MODE2_INTFLAG_ALARM0_Pos          _U_(0)                                               /**< (RTC_MODE2_INTFLAG) Alarm 0 Position */
#define RTC_MODE2_INTFLAG_ALARM0_Msk          (_U_(0x1) << RTC_MODE2_INTFLAG_ALARM0_Pos)           /**< (RTC_MODE2_INTFLAG) Alarm 0 Mask */
#define RTC_MODE2_INTFLAG_ALARM0(value)       (RTC_MODE2_INTFLAG_ALARM0_Msk & ((value) << RTC_MODE2_INTFLAG_ALARM0_Pos))
#define RTC_MODE2_INTFLAG_SYNCRDY_Pos         _U_(6)                                               /**< (RTC_MODE2_INTFLAG) Synchronization Ready Position */
#define RTC_MODE2_INTFLAG_SYNCRDY_Msk         (_U_(0x1) << RTC_MODE2_INTFLAG_SYNCRDY_Pos)          /**< (RTC_MODE2_INTFLAG) Synchronization Ready Mask */
#define RTC_MODE2_INTFLAG_SYNCRDY(value)      (RTC_MODE2_INTFLAG_SYNCRDY_Msk & ((value) << RTC_MODE2_INTFLAG_SYNCRDY_Pos))
#define RTC_MODE2_INTFLAG_OVF_Pos             _U_(7)                                               /**< (RTC_MODE2_INTFLAG) Overflow Position */
#define RTC_MODE2_INTFLAG_OVF_Msk             (_U_(0x1) << RTC_MODE2_INTFLAG_OVF_Pos)              /**< (RTC_MODE2_INTFLAG) Overflow Mask */
#define RTC_MODE2_INTFLAG_OVF(value)          (RTC_MODE2_INTFLAG_OVF_Msk & ((value) << RTC_MODE2_INTFLAG_OVF_Pos))
#define RTC_MODE2_INTFLAG_Msk                 _U_(0xC1)                                            /**< (RTC_MODE2_INTFLAG) Register Mask  */

#define RTC_MODE2_INTFLAG_ALARM_Pos           _U_(0)                                               /**< (RTC_MODE2_INTFLAG Position) Alarm x */
#define RTC_MODE2_INTFLAG_ALARM_Msk           (_U_(0x1) << RTC_MODE2_INTFLAG_ALARM_Pos)            /**< (RTC_MODE2_INTFLAG Mask) ALARM */
#define RTC_MODE2_INTFLAG_ALARM(value)        (RTC_MODE2_INTFLAG_ALARM_Msk & ((value) << RTC_MODE2_INTFLAG_ALARM_Pos)) 

/* -------- RTC_STATUS : (RTC Offset: 0x0A) (R/W 8) Status -------- */
#define RTC_STATUS_RESETVALUE                 _U_(0x00)                                            /**<  (RTC_STATUS) Status  Reset Value */

#define RTC_STATUS_SYNCBUSY_Pos               _U_(7)                                               /**< (RTC_STATUS) Synchronization Busy Position */
#define RTC_STATUS_SYNCBUSY_Msk               (_U_(0x1) << RTC_STATUS_SYNCBUSY_Pos)                /**< (RTC_STATUS) Synchronization Busy Mask */
#define RTC_STATUS_SYNCBUSY(value)            (RTC_STATUS_SYNCBUSY_Msk & ((value) << RTC_STATUS_SYNCBUSY_Pos))
#define RTC_STATUS_Msk                        _U_(0x80)                                            /**< (RTC_STATUS) Register Mask  */


/* -------- RTC_DBGCTRL : (RTC Offset: 0x0B) (R/W 8) Debug Control -------- */
#define RTC_DBGCTRL_RESETVALUE                _U_(0x00)                                            /**<  (RTC_DBGCTRL) Debug Control  Reset Value */

#define RTC_DBGCTRL_DBGRUN_Pos                _U_(0)                                               /**< (RTC_DBGCTRL) Run During Debug Position */
#define RTC_DBGCTRL_DBGRUN_Msk                (_U_(0x1) << RTC_DBGCTRL_DBGRUN_Pos)                 /**< (RTC_DBGCTRL) Run During Debug Mask */
#define RTC_DBGCTRL_DBGRUN(value)             (RTC_DBGCTRL_DBGRUN_Msk & ((value) << RTC_DBGCTRL_DBGRUN_Pos))
#define RTC_DBGCTRL_Msk                       _U_(0x01)                                            /**< (RTC_DBGCTRL) Register Mask  */


/* -------- RTC_FREQCORR : (RTC Offset: 0x0C) (R/W 8) Frequency Correction -------- */
#define RTC_FREQCORR_RESETVALUE               _U_(0x00)                                            /**<  (RTC_FREQCORR) Frequency Correction  Reset Value */

#define RTC_FREQCORR_VALUE_Pos                _U_(0)                                               /**< (RTC_FREQCORR) Correction Value Position */
#define RTC_FREQCORR_VALUE_Msk                (_U_(0x7F) << RTC_FREQCORR_VALUE_Pos)                /**< (RTC_FREQCORR) Correction Value Mask */
#define RTC_FREQCORR_VALUE(value)             (RTC_FREQCORR_VALUE_Msk & ((value) << RTC_FREQCORR_VALUE_Pos))
#define RTC_FREQCORR_SIGN_Pos                 _U_(7)                                               /**< (RTC_FREQCORR) Correction Sign Position */
#define RTC_FREQCORR_SIGN_Msk                 (_U_(0x1) << RTC_FREQCORR_SIGN_Pos)                  /**< (RTC_FREQCORR) Correction Sign Mask */
#define RTC_FREQCORR_SIGN(value)              (RTC_FREQCORR_SIGN_Msk & ((value) << RTC_FREQCORR_SIGN_Pos))
#define RTC_FREQCORR_Msk                      _U_(0xFF)                                            /**< (RTC_FREQCORR) Register Mask  */


/* -------- RTC_MODE0_COUNT : (RTC Offset: 0x10) (R/W 32) MODE0 Counter Value -------- */
#define RTC_MODE0_COUNT_RESETVALUE            _U_(0x00)                                            /**<  (RTC_MODE0_COUNT) MODE0 Counter Value  Reset Value */

#define RTC_MODE0_COUNT_COUNT_Pos             _U_(0)                                               /**< (RTC_MODE0_COUNT) Counter Value Position */
#define RTC_MODE0_COUNT_COUNT_Msk             (_U_(0xFFFFFFFF) << RTC_MODE0_COUNT_COUNT_Pos)       /**< (RTC_MODE0_COUNT) Counter Value Mask */
#define RTC_MODE0_COUNT_COUNT(value)          (RTC_MODE0_COUNT_COUNT_Msk & ((value) << RTC_MODE0_COUNT_COUNT_Pos))
#define RTC_MODE0_COUNT_Msk                   _U_(0xFFFFFFFF)                                      /**< (RTC_MODE0_COUNT) Register Mask  */


/* -------- RTC_MODE1_COUNT : (RTC Offset: 0x10) (R/W 16) MODE1 Counter Value -------- */
#define RTC_MODE1_COUNT_RESETVALUE            _U_(0x00)                                            /**<  (RTC_MODE1_COUNT) MODE1 Counter Value  Reset Value */

#define RTC_MODE1_COUNT_COUNT_Pos             _U_(0)                                               /**< (RTC_MODE1_COUNT) Counter Value Position */
#define RTC_MODE1_COUNT_COUNT_Msk             (_U_(0xFFFF) << RTC_MODE1_COUNT_COUNT_Pos)           /**< (RTC_MODE1_COUNT) Counter Value Mask */
#define RTC_MODE1_COUNT_COUNT(value)          (RTC_MODE1_COUNT_COUNT_Msk & ((value) << RTC_MODE1_COUNT_COUNT_Pos))
#define RTC_MODE1_COUNT_Msk                   _U_(0xFFFF)                                          /**< (RTC_MODE1_COUNT) Register Mask  */


/* -------- RTC_MODE2_CLOCK : (RTC Offset: 0x10) (R/W 32) MODE2 Clock Value -------- */
#define RTC_MODE2_CLOCK_RESETVALUE            _U_(0x00)                                            /**<  (RTC_MODE2_CLOCK) MODE2 Clock Value  Reset Value */

#define RTC_MODE2_CLOCK_SECOND_Pos            _U_(0)                                               /**< (RTC_MODE2_CLOCK) Second Position */
#define RTC_MODE2_CLOCK_SECOND_Msk            (_U_(0x3F) << RTC_MODE2_CLOCK_SECOND_Pos)            /**< (RTC_MODE2_CLOCK) Second Mask */
#define RTC_MODE2_CLOCK_SECOND(value)         (RTC_MODE2_CLOCK_SECOND_Msk & ((value) << RTC_MODE2_CLOCK_SECOND_Pos))
#define RTC_MODE2_CLOCK_MINUTE_Pos            _U_(6)                                               /**< (RTC_MODE2_CLOCK) Minute Position */
#define RTC_MODE2_CLOCK_MINUTE_Msk            (_U_(0x3F) << RTC_MODE2_CLOCK_MINUTE_Pos)            /**< (RTC_MODE2_CLOCK) Minute Mask */
#define RTC_MODE2_CLOCK_MINUTE(value)         (RTC_MODE2_CLOCK_MINUTE_Msk & ((value) << RTC_MODE2_CLOCK_MINUTE_Pos))
#define RTC_MODE2_CLOCK_HOUR_Pos              _U_(12)                                              /**< (RTC_MODE2_CLOCK) Hour Position */
#define RTC_MODE2_CLOCK_HOUR_Msk              (_U_(0x1F) << RTC_MODE2_CLOCK_HOUR_Pos)              /**< (RTC_MODE2_CLOCK) Hour Mask */
#define RTC_MODE2_CLOCK_HOUR(value)           (RTC_MODE2_CLOCK_HOUR_Msk & ((value) << RTC_MODE2_CLOCK_HOUR_Pos))
#define   RTC_MODE2_CLOCK_HOUR_AM_Val         _U_(0x0)                                             /**< (RTC_MODE2_CLOCK) AM when CLKREP in 12-hour  */
#define   RTC_MODE2_CLOCK_HOUR_PM_Val         _U_(0x10)                                            /**< (RTC_MODE2_CLOCK) PM when CLKREP in 12-hour  */
#define RTC_MODE2_CLOCK_HOUR_AM               (RTC_MODE2_CLOCK_HOUR_AM_Val << RTC_MODE2_CLOCK_HOUR_Pos) /**< (RTC_MODE2_CLOCK) AM when CLKREP in 12-hour Position  */
#define RTC_MODE2_CLOCK_HOUR_PM               (RTC_MODE2_CLOCK_HOUR_PM_Val << RTC_MODE2_CLOCK_HOUR_Pos) /**< (RTC_MODE2_CLOCK) PM when CLKREP in 12-hour Position  */
#define RTC_MODE2_CLOCK_DAY_Pos               _U_(17)                                              /**< (RTC_MODE2_CLOCK) Day Position */
#define RTC_MODE2_CLOCK_DAY_Msk               (_U_(0x1F) << RTC_MODE2_CLOCK_DAY_Pos)               /**< (RTC_MODE2_CLOCK) Day Mask */
#define RTC_MODE2_CLOCK_DAY(value)            (RTC_MODE2_CLOCK_DAY_Msk & ((value) << RTC_MODE2_CLOCK_DAY_Pos))
#define RTC_MODE2_CLOCK_MONTH_Pos             _U_(22)                                              /**< (RTC_MODE2_CLOCK) Month Position */
#define RTC_MODE2_CLOCK_MONTH_Msk             (_U_(0xF) << RTC_MODE2_CLOCK_MONTH_Pos)              /**< (RTC_MODE2_CLOCK) Month Mask */
#define RTC_MODE2_CLOCK_MONTH(value)          (RTC_MODE2_CLOCK_MONTH_Msk & ((value) << RTC_MODE2_CLOCK_MONTH_Pos))
#define RTC_MODE2_CLOCK_YEAR_Pos              _U_(26)                                              /**< (RTC_MODE2_CLOCK) Year Position */
#define RTC_MODE2_CLOCK_YEAR_Msk              (_U_(0x3F) << RTC_MODE2_CLOCK_YEAR_Pos)              /**< (RTC_MODE2_CLOCK) Year Mask */
#define RTC_MODE2_CLOCK_YEAR(value)           (RTC_MODE2_CLOCK_YEAR_Msk & ((value) << RTC_MODE2_CLOCK_YEAR_Pos))
#define RTC_MODE2_CLOCK_Msk                   _U_(0xFFFFFFFF)                                      /**< (RTC_MODE2_CLOCK) Register Mask  */


/* -------- RTC_MODE1_PER : (RTC Offset: 0x14) (R/W 16) MODE1 Counter Period -------- */
#define RTC_MODE1_PER_RESETVALUE              _U_(0x00)                                            /**<  (RTC_MODE1_PER) MODE1 Counter Period  Reset Value */

#define RTC_MODE1_PER_PER_Pos                 _U_(0)                                               /**< (RTC_MODE1_PER) Counter Period Position */
#define RTC_MODE1_PER_PER_Msk                 (_U_(0xFFFF) << RTC_MODE1_PER_PER_Pos)               /**< (RTC_MODE1_PER) Counter Period Mask */
#define RTC_MODE1_PER_PER(value)              (RTC_MODE1_PER_PER_Msk & ((value) << RTC_MODE1_PER_PER_Pos))
#define RTC_MODE1_PER_Msk                     _U_(0xFFFF)                                          /**< (RTC_MODE1_PER) Register Mask  */


/* -------- RTC_MODE0_COMP : (RTC Offset: 0x18) (R/W 32) MODE0 Compare n Value -------- */
#define RTC_MODE0_COMP_RESETVALUE             _U_(0x00)                                            /**<  (RTC_MODE0_COMP) MODE0 Compare n Value  Reset Value */

#define RTC_MODE0_COMP_COMP_Pos               _U_(0)                                               /**< (RTC_MODE0_COMP) Compare Value Position */
#define RTC_MODE0_COMP_COMP_Msk               (_U_(0xFFFFFFFF) << RTC_MODE0_COMP_COMP_Pos)         /**< (RTC_MODE0_COMP) Compare Value Mask */
#define RTC_MODE0_COMP_COMP(value)            (RTC_MODE0_COMP_COMP_Msk & ((value) << RTC_MODE0_COMP_COMP_Pos))
#define RTC_MODE0_COMP_Msk                    _U_(0xFFFFFFFF)                                      /**< (RTC_MODE0_COMP) Register Mask  */


/* -------- RTC_MODE1_COMP : (RTC Offset: 0x18) (R/W 16) MODE1 Compare n Value -------- */
#define RTC_MODE1_COMP_RESETVALUE             _U_(0x00)                                            /**<  (RTC_MODE1_COMP) MODE1 Compare n Value  Reset Value */

#define RTC_MODE1_COMP_COMP_Pos               _U_(0)                                               /**< (RTC_MODE1_COMP) Compare Value Position */
#define RTC_MODE1_COMP_COMP_Msk               (_U_(0xFFFF) << RTC_MODE1_COMP_COMP_Pos)             /**< (RTC_MODE1_COMP) Compare Value Mask */
#define RTC_MODE1_COMP_COMP(value)            (RTC_MODE1_COMP_COMP_Msk & ((value) << RTC_MODE1_COMP_COMP_Pos))
#define RTC_MODE1_COMP_Msk                    _U_(0xFFFF)                                          /**< (RTC_MODE1_COMP) Register Mask  */


/* -------- RTC_MODE2_ALARM : (RTC Offset: 0x18) (R/W 32) MODE2_ALARM Alarm n Value -------- */
#define RTC_MODE2_ALARM_RESETVALUE            _U_(0x00)                                            /**<  (RTC_MODE2_ALARM) MODE2_ALARM Alarm n Value  Reset Value */

#define RTC_MODE2_ALARM_SECOND_Pos            _U_(0)                                               /**< (RTC_MODE2_ALARM) Second Position */
#define RTC_MODE2_ALARM_SECOND_Msk            (_U_(0x3F) << RTC_MODE2_ALARM_SECOND_Pos)            /**< (RTC_MODE2_ALARM) Second Mask */
#define RTC_MODE2_ALARM_SECOND(value)         (RTC_MODE2_ALARM_SECOND_Msk & ((value) << RTC_MODE2_ALARM_SECOND_Pos))
#define RTC_MODE2_ALARM_MINUTE_Pos            _U_(6)                                               /**< (RTC_MODE2_ALARM) Minute Position */
#define RTC_MODE2_ALARM_MINUTE_Msk            (_U_(0x3F) << RTC_MODE2_ALARM_MINUTE_Pos)            /**< (RTC_MODE2_ALARM) Minute Mask */
#define RTC_MODE2_ALARM_MINUTE(value)         (RTC_MODE2_ALARM_MINUTE_Msk & ((value) << RTC_MODE2_ALARM_MINUTE_Pos))
#define RTC_MODE2_ALARM_HOUR_Pos              _U_(12)                                              /**< (RTC_MODE2_ALARM) Hour Position */
#define RTC_MODE2_ALARM_HOUR_Msk              (_U_(0x1F) << RTC_MODE2_ALARM_HOUR_Pos)              /**< (RTC_MODE2_ALARM) Hour Mask */
#define RTC_MODE2_ALARM_HOUR(value)           (RTC_MODE2_ALARM_HOUR_Msk & ((value) << RTC_MODE2_ALARM_HOUR_Pos))
#define   RTC_MODE2_ALARM_HOUR_AM_Val         _U_(0x0)                                             /**< (RTC_MODE2_ALARM) Morning hour  */
#define   RTC_MODE2_ALARM_HOUR_PM_Val         _U_(0x10)                                            /**< (RTC_MODE2_ALARM) Afternoon hour  */
#define RTC_MODE2_ALARM_HOUR_AM               (RTC_MODE2_ALARM_HOUR_AM_Val << RTC_MODE2_ALARM_HOUR_Pos) /**< (RTC_MODE2_ALARM) Morning hour Position  */
#define RTC_MODE2_ALARM_HOUR_PM               (RTC_MODE2_ALARM_HOUR_PM_Val << RTC_MODE2_ALARM_HOUR_Pos) /**< (RTC_MODE2_ALARM) Afternoon hour Position  */
#define RTC_MODE2_ALARM_DAY_Pos               _U_(17)                                              /**< (RTC_MODE2_ALARM) Day Position */
#define RTC_MODE2_ALARM_DAY_Msk               (_U_(0x1F) << RTC_MODE2_ALARM_DAY_Pos)               /**< (RTC_MODE2_ALARM) Day Mask */
#define RTC_MODE2_ALARM_DAY(value)            (RTC_MODE2_ALARM_DAY_Msk & ((value) << RTC_MODE2_ALARM_DAY_Pos))
#define RTC_MODE2_ALARM_MONTH_Pos             _U_(22)                                              /**< (RTC_MODE2_ALARM) Month Position */
#define RTC_MODE2_ALARM_MONTH_Msk             (_U_(0xF) << RTC_MODE2_ALARM_MONTH_Pos)              /**< (RTC_MODE2_ALARM) Month Mask */
#define RTC_MODE2_ALARM_MONTH(value)          (RTC_MODE2_ALARM_MONTH_Msk & ((value) << RTC_MODE2_ALARM_MONTH_Pos))
#define RTC_MODE2_ALARM_YEAR_Pos              _U_(26)                                              /**< (RTC_MODE2_ALARM) Year Position */
#define RTC_MODE2_ALARM_YEAR_Msk              (_U_(0x3F) << RTC_MODE2_ALARM_YEAR_Pos)              /**< (RTC_MODE2_ALARM) Year Mask */
#define RTC_MODE2_ALARM_YEAR(value)           (RTC_MODE2_ALARM_YEAR_Msk & ((value) << RTC_MODE2_ALARM_YEAR_Pos))
#define RTC_MODE2_ALARM_Msk                   _U_(0xFFFFFFFF)                                      /**< (RTC_MODE2_ALARM) Register Mask  */


/* -------- RTC_MODE2_MASK : (RTC Offset: 0x1C) (R/W 8) MODE2_ALARM Alarm n Mask -------- */
#define RTC_MODE2_MASK_RESETVALUE             _U_(0x00)                                            /**<  (RTC_MODE2_MASK) MODE2_ALARM Alarm n Mask  Reset Value */

#define RTC_MODE2_MASK_SEL_Pos                _U_(0)                                               /**< (RTC_MODE2_MASK) Alarm Mask Selection Position */
#define RTC_MODE2_MASK_SEL_Msk                (_U_(0x7) << RTC_MODE2_MASK_SEL_Pos)                 /**< (RTC_MODE2_MASK) Alarm Mask Selection Mask */
#define RTC_MODE2_MASK_SEL(value)             (RTC_MODE2_MASK_SEL_Msk & ((value) << RTC_MODE2_MASK_SEL_Pos))
#define   RTC_MODE2_MASK_SEL_OFF_Val          _U_(0x0)                                             /**< (RTC_MODE2_MASK) Alarm Disabled  */
#define   RTC_MODE2_MASK_SEL_SS_Val           _U_(0x1)                                             /**< (RTC_MODE2_MASK) Match seconds only  */
#define   RTC_MODE2_MASK_SEL_MMSS_Val         _U_(0x2)                                             /**< (RTC_MODE2_MASK) Match seconds and minutes only  */
#define   RTC_MODE2_MASK_SEL_HHMMSS_Val       _U_(0x3)                                             /**< (RTC_MODE2_MASK) Match seconds, minutes, and hours only  */
#define   RTC_MODE2_MASK_SEL_DDHHMMSS_Val     _U_(0x4)                                             /**< (RTC_MODE2_MASK) Match seconds, minutes, hours, and days only  */
#define   RTC_MODE2_MASK_SEL_MMDDHHMMSS_Val   _U_(0x5)                                             /**< (RTC_MODE2_MASK) Match seconds, minutes, hours, days, and months only  */
#define   RTC_MODE2_MASK_SEL_YYMMDDHHMMSS_Val _U_(0x6)                                             /**< (RTC_MODE2_MASK) Match seconds, minutes, hours, days, months, and years  */
#define RTC_MODE2_MASK_SEL_OFF                (RTC_MODE2_MASK_SEL_OFF_Val << RTC_MODE2_MASK_SEL_Pos) /**< (RTC_MODE2_MASK) Alarm Disabled Position  */
#define RTC_MODE2_MASK_SEL_SS                 (RTC_MODE2_MASK_SEL_SS_Val << RTC_MODE2_MASK_SEL_Pos) /**< (RTC_MODE2_MASK) Match seconds only Position  */
#define RTC_MODE2_MASK_SEL_MMSS               (RTC_MODE2_MASK_SEL_MMSS_Val << RTC_MODE2_MASK_SEL_Pos) /**< (RTC_MODE2_MASK) Match seconds and minutes only Position  */
#define RTC_MODE2_MASK_SEL_HHMMSS             (RTC_MODE2_MASK_SEL_HHMMSS_Val << RTC_MODE2_MASK_SEL_Pos) /**< (RTC_MODE2_MASK) Match seconds, minutes, and hours only Position  */
#define RTC_MODE2_MASK_SEL_DDHHMMSS           (RTC_MODE2_MASK_SEL_DDHHMMSS_Val << RTC_MODE2_MASK_SEL_Pos) /**< (RTC_MODE2_MASK) Match seconds, minutes, hours, and days only Position  */
#define RTC_MODE2_MASK_SEL_MMDDHHMMSS         (RTC_MODE2_MASK_SEL_MMDDHHMMSS_Val << RTC_MODE2_MASK_SEL_Pos) /**< (RTC_MODE2_MASK) Match seconds, minutes, hours, days, and months only Position  */
#define RTC_MODE2_MASK_SEL_YYMMDDHHMMSS       (RTC_MODE2_MASK_SEL_YYMMDDHHMMSS_Val << RTC_MODE2_MASK_SEL_Pos) /**< (RTC_MODE2_MASK) Match seconds, minutes, hours, days, months, and years Position  */
#define RTC_MODE2_MASK_Msk                    _U_(0x07)                                            /**< (RTC_MODE2_MASK) Register Mask  */


/** \brief RTC register offsets definitions */
#define RTC_MODE0_CTRL_REG_OFST        (0x00)              /**< (RTC_MODE0_CTRL) MODE0 Control Offset */
#define RTC_MODE1_CTRL_REG_OFST        (0x00)              /**< (RTC_MODE1_CTRL) MODE1 Control Offset */
#define RTC_MODE2_CTRL_REG_OFST        (0x00)              /**< (RTC_MODE2_CTRL) MODE2 Control Offset */
#define RTC_READREQ_REG_OFST           (0x02)              /**< (RTC_READREQ) Read Request Offset */
#define RTC_MODE0_EVCTRL_REG_OFST      (0x04)              /**< (RTC_MODE0_EVCTRL) MODE0 Event Control Offset */
#define RTC_MODE1_EVCTRL_REG_OFST      (0x04)              /**< (RTC_MODE1_EVCTRL) MODE1 Event Control Offset */
#define RTC_MODE2_EVCTRL_REG_OFST      (0x04)              /**< (RTC_MODE2_EVCTRL) MODE2 Event Control Offset */
#define RTC_MODE0_INTENCLR_REG_OFST    (0x06)              /**< (RTC_MODE0_INTENCLR) MODE0 Interrupt Enable Clear Offset */
#define RTC_MODE1_INTENCLR_REG_OFST    (0x06)              /**< (RTC_MODE1_INTENCLR) MODE1 Interrupt Enable Clear Offset */
#define RTC_MODE2_INTENCLR_REG_OFST    (0x06)              /**< (RTC_MODE2_INTENCLR) MODE2 Interrupt Enable Clear Offset */
#define RTC_MODE0_INTENSET_REG_OFST    (0x07)              /**< (RTC_MODE0_INTENSET) MODE0 Interrupt Enable Set Offset */
#define RTC_MODE1_INTENSET_REG_OFST    (0x07)              /**< (RTC_MODE1_INTENSET) MODE1 Interrupt Enable Set Offset */
#define RTC_MODE2_INTENSET_REG_OFST    (0x07)              /**< (RTC_MODE2_INTENSET) MODE2 Interrupt Enable Set Offset */
#define RTC_MODE0_INTFLAG_REG_OFST     (0x08)              /**< (RTC_MODE0_INTFLAG) MODE0 Interrupt Flag Status and Clear Offset */
#define RTC_MODE1_INTFLAG_REG_OFST     (0x08)              /**< (RTC_MODE1_INTFLAG) MODE1 Interrupt Flag Status and Clear Offset */
#define RTC_MODE2_INTFLAG_REG_OFST     (0x08)              /**< (RTC_MODE2_INTFLAG) MODE2 Interrupt Flag Status and Clear Offset */
#define RTC_STATUS_REG_OFST            (0x0A)              /**< (RTC_STATUS) Status Offset */
#define RTC_DBGCTRL_REG_OFST           (0x0B)              /**< (RTC_DBGCTRL) Debug Control Offset */
#define RTC_FREQCORR_REG_OFST          (0x0C)              /**< (RTC_FREQCORR) Frequency Correction Offset */
#define RTC_MODE0_COUNT_REG_OFST       (0x10)              /**< (RTC_MODE0_COUNT) MODE0 Counter Value Offset */
#define RTC_MODE1_COUNT_REG_OFST       (0x10)              /**< (RTC_MODE1_COUNT) MODE1 Counter Value Offset */
#define RTC_MODE2_CLOCK_REG_OFST       (0x10)              /**< (RTC_MODE2_CLOCK) MODE2 Clock Value Offset */
#define RTC_MODE1_PER_REG_OFST         (0x14)              /**< (RTC_MODE1_PER) MODE1 Counter Period Offset */
#define RTC_MODE0_COMP_REG_OFST        (0x18)              /**< (RTC_MODE0_COMP) MODE0 Compare n Value Offset */
#define RTC_MODE1_COMP_REG_OFST        (0x18)              /**< (RTC_MODE1_COMP) MODE1 Compare n Value Offset */
#define RTC_MODE2_ALARM_REG_OFST       (0x18)              /**< (RTC_MODE2_ALARM) MODE2_ALARM Alarm n Value Offset */
#define RTC_MODE2_MASK_REG_OFST        (0x1C)              /**< (RTC_MODE2_MASK) MODE2_ALARM Alarm n Mask Offset */

#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
/** \brief RTC register API structure */
typedef struct
{  /* Real-Time Counter */
  __IO  uint16_t                       RTC_CTRL;           /**< Offset: 0x00 (R/W  16) MODE0 Control */
  __IO  uint16_t                       RTC_READREQ;        /**< Offset: 0x02 (R/W  16) Read Request */
  __IO  uint16_t                       RTC_EVCTRL;         /**< Offset: 0x04 (R/W  16) MODE0 Event Control */
  __IO  uint8_t                        RTC_INTENCLR;       /**< Offset: 0x06 (R/W  8) MODE0 Interrupt Enable Clear */
  __IO  uint8_t                        RTC_INTENSET;       /**< Offset: 0x07 (R/W  8) MODE0 Interrupt Enable Set */
  __IO  uint8_t                        RTC_INTFLAG;        /**< Offset: 0x08 (R/W  8) MODE0 Interrupt Flag Status and Clear */
  __I   uint8_t                        Reserved1[0x01];
  __IO  uint8_t                        RTC_STATUS;         /**< Offset: 0x0A (R/W  8) Status */
  __IO  uint8_t                        RTC_DBGCTRL;        /**< Offset: 0x0B (R/W  8) Debug Control */
  __IO  uint8_t                        RTC_FREQCORR;       /**< Offset: 0x0C (R/W  8) Frequency Correction */
  __I   uint8_t                        Reserved2[0x03];
  __IO  uint32_t                       RTC_COUNT;          /**< Offset: 0x10 (R/W  32) MODE0 Counter Value */
  __I   uint8_t                        Reserved3[0x04];
  __IO  uint32_t                       RTC_COMP;           /**< Offset: 0x18 (R/W  32) MODE0 Compare n Value */
} rtc_mode0_registers_t;

/** \brief RTC register API structure */
typedef struct
{  /* Real-Time Counter */
  __IO  uint16_t                       RTC_CTRL;           /**< Offset: 0x00 (R/W  16) MODE1 Control */
  __IO  uint16_t                       RTC_READREQ;        /**< Offset: 0x02 (R/W  16) Read Request */
  __IO  uint16_t                       RTC_EVCTRL;         /**< Offset: 0x04 (R/W  16) MODE1 Event Control */
  __IO  uint8_t                        RTC_INTENCLR;       /**< Offset: 0x06 (R/W  8) MODE1 Interrupt Enable Clear */
  __IO  uint8_t                        RTC_INTENSET;       /**< Offset: 0x07 (R/W  8) MODE1 Interrupt Enable Set */
  __IO  uint8_t                        RTC_INTFLAG;        /**< Offset: 0x08 (R/W  8) MODE1 Interrupt Flag Status and Clear */
  __I   uint8_t                        Reserved1[0x01];
  __IO  uint8_t                        RTC_STATUS;         /**< Offset: 0x0A (R/W  8) Status */
  __IO  uint8_t                        RTC_DBGCTRL;        /**< Offset: 0x0B (R/W  8) Debug Control */
  __IO  uint8_t                        RTC_FREQCORR;       /**< Offset: 0x0C (R/W  8) Frequency Correction */
  __I   uint8_t                        Reserved2[0x03];
  __IO  uint16_t                       RTC_COUNT;          /**< Offset: 0x10 (R/W  16) MODE1 Counter Value */
  __I   uint8_t                        Reserved3[0x02];
  __IO  uint16_t                       RTC_PER;            /**< Offset: 0x14 (R/W  16) MODE1 Counter Period */
  __I   uint8_t                        Reserved4[0x02];
  __IO  uint16_t                       RTC_COMP[2];        /**< Offset: 0x18 (R/W  16) MODE1 Compare n Value */
} rtc_mode1_registers_t;

/** \brief RTC register API structure */
typedef struct
{  /* Real-Time Counter */
  __IO  uint16_t                       RTC_CTRL;           /**< Offset: 0x00 (R/W  16) MODE2 Control */
  __IO  uint16_t                       RTC_READREQ;        /**< Offset: 0x02 (R/W  16) Read Request */
  __IO  uint16_t                       RTC_EVCTRL;         /**< Offset: 0x04 (R/W  16) MODE2 Event Control */
  __IO  uint8_t                        RTC_INTENCLR;       /**< Offset: 0x06 (R/W  8) MODE2 Interrupt Enable Clear */
  __IO  uint8_t                        RTC_INTENSET;       /**< Offset: 0x07 (R/W  8) MODE2 Interrupt Enable Set */
  __IO  uint8_t                        RTC_INTFLAG;        /**< Offset: 0x08 (R/W  8) MODE2 Interrupt Flag Status and Clear */
  __I   uint8_t                        Reserved1[0x01];
  __IO  uint8_t                        RTC_STATUS;         /**< Offset: 0x0A (R/W  8) Status */
  __IO  uint8_t                        RTC_DBGCTRL;        /**< Offset: 0x0B (R/W  8) Debug Control */
  __IO  uint8_t                        RTC_FREQCORR;       /**< Offset: 0x0C (R/W  8) Frequency Correction */
  __I   uint8_t                        Reserved2[0x03];
  __IO  uint32_t                       RTC_CLOCK;          /**< Offset: 0x10 (R/W  32) MODE2 Clock Value */
  __I   uint8_t                        Reserved3[0x04];
  __IO  uint32_t                       RTC_ALARM;          /**< Offset: 0x18 (R/W  32) MODE2_ALARM Alarm n Value */
  __IO  uint8_t                        RTC_MASK;           /**< Offset: 0x1C (R/W  8) MODE2_ALARM Alarm n Mask */
} rtc_mode2_registers_t;

/** \brief RTC hardware registers */
typedef union
{  /* Real-Time Counter */
       rtc_mode0_registers_t          MODE0;          /**< 32-bit Counter with Single 32-bit Compare */
       rtc_mode1_registers_t          MODE1;          /**< 16-bit Counter with Two 16-bit Compares */
       rtc_mode2_registers_t          MODE2;          /**< Clock/Calendar with Alarm */
} rtc_registers_t;


#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
#endif /* _SAMD21_RTC_COMPONENT_H_ */
