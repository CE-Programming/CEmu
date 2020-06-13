/**
 * \brief Component description for GCLK
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
#ifndef _SAMD21_GCLK_COMPONENT_H_
#define _SAMD21_GCLK_COMPONENT_H_

/* ************************************************************************** */
/*   SOFTWARE API DEFINITION FOR GCLK                                         */
/* ************************************************************************** */

/* -------- GCLK_CTRL : (GCLK Offset: 0x00) (R/W 8) Control -------- */
#define GCLK_CTRL_RESETVALUE                  _U_(0x00)                                            /**<  (GCLK_CTRL) Control  Reset Value */

#define GCLK_CTRL_SWRST_Pos                   _U_(0)                                               /**< (GCLK_CTRL) Software Reset Position */
#define GCLK_CTRL_SWRST_Msk                   (_U_(0x1) << GCLK_CTRL_SWRST_Pos)                    /**< (GCLK_CTRL) Software Reset Mask */
#define GCLK_CTRL_SWRST(value)                (GCLK_CTRL_SWRST_Msk & ((value) << GCLK_CTRL_SWRST_Pos))
#define GCLK_CTRL_Msk                         _U_(0x01)                                            /**< (GCLK_CTRL) Register Mask  */


/* -------- GCLK_STATUS : (GCLK Offset: 0x01) ( R/ 8) Status -------- */
#define GCLK_STATUS_RESETVALUE                _U_(0x00)                                            /**<  (GCLK_STATUS) Status  Reset Value */

#define GCLK_STATUS_SYNCBUSY_Pos              _U_(7)                                               /**< (GCLK_STATUS) Synchronization Busy Status Position */
#define GCLK_STATUS_SYNCBUSY_Msk              (_U_(0x1) << GCLK_STATUS_SYNCBUSY_Pos)               /**< (GCLK_STATUS) Synchronization Busy Status Mask */
#define GCLK_STATUS_SYNCBUSY(value)           (GCLK_STATUS_SYNCBUSY_Msk & ((value) << GCLK_STATUS_SYNCBUSY_Pos))
#define GCLK_STATUS_Msk                       _U_(0x80)                                            /**< (GCLK_STATUS) Register Mask  */


/* -------- GCLK_CLKCTRL : (GCLK Offset: 0x02) (R/W 16) Generic Clock Control -------- */
#define GCLK_CLKCTRL_RESETVALUE               _U_(0x00)                                            /**<  (GCLK_CLKCTRL) Generic Clock Control  Reset Value */

#define GCLK_CLKCTRL_ID_Pos                   _U_(0)                                               /**< (GCLK_CLKCTRL) Generic Clock Selection ID Position */
#define GCLK_CLKCTRL_ID_Msk                   (_U_(0x3F) << GCLK_CLKCTRL_ID_Pos)                   /**< (GCLK_CLKCTRL) Generic Clock Selection ID Mask */
#define GCLK_CLKCTRL_ID(value)                (GCLK_CLKCTRL_ID_Msk & ((value) << GCLK_CLKCTRL_ID_Pos))
#define   GCLK_CLKCTRL_ID_DFLL48_Val          _U_(0x0)                                             /**< (GCLK_CLKCTRL) DFLL48  */
#define   GCLK_CLKCTRL_ID_FDPLL_Val           _U_(0x1)                                             /**< (GCLK_CLKCTRL) FDPLL  */
#define   GCLK_CLKCTRL_ID_FDPLL32K_Val        _U_(0x2)                                             /**< (GCLK_CLKCTRL) FDPLL32K  */
#define   GCLK_CLKCTRL_ID_WDT_Val             _U_(0x3)                                             /**< (GCLK_CLKCTRL) WDT  */
#define   GCLK_CLKCTRL_ID_RTC_Val             _U_(0x4)                                             /**< (GCLK_CLKCTRL) RTC  */
#define   GCLK_CLKCTRL_ID_EIC_Val             _U_(0x5)                                             /**< (GCLK_CLKCTRL) EIC  */
#define   GCLK_CLKCTRL_ID_USB_Val             _U_(0x6)                                             /**< (GCLK_CLKCTRL) USB  */
#define   GCLK_CLKCTRL_ID_EVSYS_0_Val         _U_(0x7)                                             /**< (GCLK_CLKCTRL) EVSYS_0  */
#define   GCLK_CLKCTRL_ID_EVSYS_1_Val         _U_(0x8)                                             /**< (GCLK_CLKCTRL) EVSYS_1  */
#define   GCLK_CLKCTRL_ID_EVSYS_2_Val         _U_(0x9)                                             /**< (GCLK_CLKCTRL) EVSYS_2  */
#define   GCLK_CLKCTRL_ID_EVSYS_3_Val         _U_(0xA)                                             /**< (GCLK_CLKCTRL) EVSYS_3  */
#define   GCLK_CLKCTRL_ID_EVSYS_4_Val         _U_(0xB)                                             /**< (GCLK_CLKCTRL) EVSYS_4  */
#define   GCLK_CLKCTRL_ID_EVSYS_5_Val         _U_(0xC)                                             /**< (GCLK_CLKCTRL) EVSYS_5  */
#define   GCLK_CLKCTRL_ID_EVSYS_6_Val         _U_(0xD)                                             /**< (GCLK_CLKCTRL) EVSYS_6  */
#define   GCLK_CLKCTRL_ID_EVSYS_7_Val         _U_(0xE)                                             /**< (GCLK_CLKCTRL) EVSYS_7  */
#define   GCLK_CLKCTRL_ID_EVSYS_8_Val         _U_(0xF)                                             /**< (GCLK_CLKCTRL) EVSYS_8  */
#define   GCLK_CLKCTRL_ID_EVSYS_9_Val         _U_(0x10)                                            /**< (GCLK_CLKCTRL) EVSYS_9  */
#define   GCLK_CLKCTRL_ID_EVSYS_10_Val        _U_(0x11)                                            /**< (GCLK_CLKCTRL) EVSYS_10  */
#define   GCLK_CLKCTRL_ID_EVSYS_11_Val        _U_(0x12)                                            /**< (GCLK_CLKCTRL) EVSYS_11  */
#define   GCLK_CLKCTRL_ID_SERCOMX_SLOW_Val    _U_(0x13)                                            /**< (GCLK_CLKCTRL) SERCOMX_SLOW  */
#define   GCLK_CLKCTRL_ID_SERCOM0_CORE_Val    _U_(0x14)                                            /**< (GCLK_CLKCTRL) SERCOM0_CORE  */
#define   GCLK_CLKCTRL_ID_SERCOM1_CORE_Val    _U_(0x15)                                            /**< (GCLK_CLKCTRL) SERCOM1_CORE  */
#define   GCLK_CLKCTRL_ID_SERCOM2_CORE_Val    _U_(0x16)                                            /**< (GCLK_CLKCTRL) SERCOM2_CORE  */
#define   GCLK_CLKCTRL_ID_SERCOM3_CORE_Val    _U_(0x17)                                            /**< (GCLK_CLKCTRL) SERCOM3_CORE  */
#define   GCLK_CLKCTRL_ID_SERCOM4_CORE_Val    _U_(0x18)                                            /**< (GCLK_CLKCTRL) SERCOM4_CORE  */
#define   GCLK_CLKCTRL_ID_SERCOM5_CORE_Val    _U_(0x19)                                            /**< (GCLK_CLKCTRL) SERCOM5_CORE  */
#define   GCLK_CLKCTRL_ID_TCC0_TCC1_Val       _U_(0x1A)                                            /**< (GCLK_CLKCTRL) TCC0_TCC1  */
#define   GCLK_CLKCTRL_ID_TCC2_TC3_Val        _U_(0x1B)                                            /**< (GCLK_CLKCTRL) TCC2_TC3  */
#define   GCLK_CLKCTRL_ID_TC4_TC5_Val         _U_(0x1C)                                            /**< (GCLK_CLKCTRL) TC4_TC5  */
#define   GCLK_CLKCTRL_ID_TC6_TC7_Val         _U_(0x1D)                                            /**< (GCLK_CLKCTRL) TC6_TC7  */
#define   GCLK_CLKCTRL_ID_ADC_Val             _U_(0x1E)                                            /**< (GCLK_CLKCTRL) ADC  */
#define   GCLK_CLKCTRL_ID_AC_DIG_Val          _U_(0x1F)                                            /**< (GCLK_CLKCTRL) AC_DIG  */
#define   GCLK_CLKCTRL_ID_AC_ANA_Val          _U_(0x20)                                            /**< (GCLK_CLKCTRL) AC_ANA  */
#define   GCLK_CLKCTRL_ID_DAC_Val             _U_(0x21)                                            /**< (GCLK_CLKCTRL) DAC  */
#define   GCLK_CLKCTRL_ID_I2S_0_Val           _U_(0x23)                                            /**< (GCLK_CLKCTRL) I2S_0  */
#define   GCLK_CLKCTRL_ID_I2S_1_Val           _U_(0x24)                                            /**< (GCLK_CLKCTRL) I2S_1  */
#define GCLK_CLKCTRL_ID_DFLL48                (GCLK_CLKCTRL_ID_DFLL48_Val << GCLK_CLKCTRL_ID_Pos)  /**< (GCLK_CLKCTRL) DFLL48 Position  */
#define GCLK_CLKCTRL_ID_FDPLL                 (GCLK_CLKCTRL_ID_FDPLL_Val << GCLK_CLKCTRL_ID_Pos)   /**< (GCLK_CLKCTRL) FDPLL Position  */
#define GCLK_CLKCTRL_ID_FDPLL32K              (GCLK_CLKCTRL_ID_FDPLL32K_Val << GCLK_CLKCTRL_ID_Pos) /**< (GCLK_CLKCTRL) FDPLL32K Position  */
#define GCLK_CLKCTRL_ID_WDT                   (GCLK_CLKCTRL_ID_WDT_Val << GCLK_CLKCTRL_ID_Pos)     /**< (GCLK_CLKCTRL) WDT Position  */
#define GCLK_CLKCTRL_ID_RTC                   (GCLK_CLKCTRL_ID_RTC_Val << GCLK_CLKCTRL_ID_Pos)     /**< (GCLK_CLKCTRL) RTC Position  */
#define GCLK_CLKCTRL_ID_EIC                   (GCLK_CLKCTRL_ID_EIC_Val << GCLK_CLKCTRL_ID_Pos)     /**< (GCLK_CLKCTRL) EIC Position  */
#define GCLK_CLKCTRL_ID_USB                   (GCLK_CLKCTRL_ID_USB_Val << GCLK_CLKCTRL_ID_Pos)     /**< (GCLK_CLKCTRL) USB Position  */
#define GCLK_CLKCTRL_ID_EVSYS_0               (GCLK_CLKCTRL_ID_EVSYS_0_Val << GCLK_CLKCTRL_ID_Pos) /**< (GCLK_CLKCTRL) EVSYS_0 Position  */
#define GCLK_CLKCTRL_ID_EVSYS_1               (GCLK_CLKCTRL_ID_EVSYS_1_Val << GCLK_CLKCTRL_ID_Pos) /**< (GCLK_CLKCTRL) EVSYS_1 Position  */
#define GCLK_CLKCTRL_ID_EVSYS_2               (GCLK_CLKCTRL_ID_EVSYS_2_Val << GCLK_CLKCTRL_ID_Pos) /**< (GCLK_CLKCTRL) EVSYS_2 Position  */
#define GCLK_CLKCTRL_ID_EVSYS_3               (GCLK_CLKCTRL_ID_EVSYS_3_Val << GCLK_CLKCTRL_ID_Pos) /**< (GCLK_CLKCTRL) EVSYS_3 Position  */
#define GCLK_CLKCTRL_ID_EVSYS_4               (GCLK_CLKCTRL_ID_EVSYS_4_Val << GCLK_CLKCTRL_ID_Pos) /**< (GCLK_CLKCTRL) EVSYS_4 Position  */
#define GCLK_CLKCTRL_ID_EVSYS_5               (GCLK_CLKCTRL_ID_EVSYS_5_Val << GCLK_CLKCTRL_ID_Pos) /**< (GCLK_CLKCTRL) EVSYS_5 Position  */
#define GCLK_CLKCTRL_ID_EVSYS_6               (GCLK_CLKCTRL_ID_EVSYS_6_Val << GCLK_CLKCTRL_ID_Pos) /**< (GCLK_CLKCTRL) EVSYS_6 Position  */
#define GCLK_CLKCTRL_ID_EVSYS_7               (GCLK_CLKCTRL_ID_EVSYS_7_Val << GCLK_CLKCTRL_ID_Pos) /**< (GCLK_CLKCTRL) EVSYS_7 Position  */
#define GCLK_CLKCTRL_ID_EVSYS_8               (GCLK_CLKCTRL_ID_EVSYS_8_Val << GCLK_CLKCTRL_ID_Pos) /**< (GCLK_CLKCTRL) EVSYS_8 Position  */
#define GCLK_CLKCTRL_ID_EVSYS_9               (GCLK_CLKCTRL_ID_EVSYS_9_Val << GCLK_CLKCTRL_ID_Pos) /**< (GCLK_CLKCTRL) EVSYS_9 Position  */
#define GCLK_CLKCTRL_ID_EVSYS_10              (GCLK_CLKCTRL_ID_EVSYS_10_Val << GCLK_CLKCTRL_ID_Pos) /**< (GCLK_CLKCTRL) EVSYS_10 Position  */
#define GCLK_CLKCTRL_ID_EVSYS_11              (GCLK_CLKCTRL_ID_EVSYS_11_Val << GCLK_CLKCTRL_ID_Pos) /**< (GCLK_CLKCTRL) EVSYS_11 Position  */
#define GCLK_CLKCTRL_ID_SERCOMX_SLOW          (GCLK_CLKCTRL_ID_SERCOMX_SLOW_Val << GCLK_CLKCTRL_ID_Pos) /**< (GCLK_CLKCTRL) SERCOMX_SLOW Position  */
#define GCLK_CLKCTRL_ID_SERCOM0_CORE          (GCLK_CLKCTRL_ID_SERCOM0_CORE_Val << GCLK_CLKCTRL_ID_Pos) /**< (GCLK_CLKCTRL) SERCOM0_CORE Position  */
#define GCLK_CLKCTRL_ID_SERCOM1_CORE          (GCLK_CLKCTRL_ID_SERCOM1_CORE_Val << GCLK_CLKCTRL_ID_Pos) /**< (GCLK_CLKCTRL) SERCOM1_CORE Position  */
#define GCLK_CLKCTRL_ID_SERCOM2_CORE          (GCLK_CLKCTRL_ID_SERCOM2_CORE_Val << GCLK_CLKCTRL_ID_Pos) /**< (GCLK_CLKCTRL) SERCOM2_CORE Position  */
#define GCLK_CLKCTRL_ID_SERCOM3_CORE          (GCLK_CLKCTRL_ID_SERCOM3_CORE_Val << GCLK_CLKCTRL_ID_Pos) /**< (GCLK_CLKCTRL) SERCOM3_CORE Position  */
#define GCLK_CLKCTRL_ID_SERCOM4_CORE          (GCLK_CLKCTRL_ID_SERCOM4_CORE_Val << GCLK_CLKCTRL_ID_Pos) /**< (GCLK_CLKCTRL) SERCOM4_CORE Position  */
#define GCLK_CLKCTRL_ID_SERCOM5_CORE          (GCLK_CLKCTRL_ID_SERCOM5_CORE_Val << GCLK_CLKCTRL_ID_Pos) /**< (GCLK_CLKCTRL) SERCOM5_CORE Position  */
#define GCLK_CLKCTRL_ID_TCC0_TCC1             (GCLK_CLKCTRL_ID_TCC0_TCC1_Val << GCLK_CLKCTRL_ID_Pos) /**< (GCLK_CLKCTRL) TCC0_TCC1 Position  */
#define GCLK_CLKCTRL_ID_TCC2_TC3              (GCLK_CLKCTRL_ID_TCC2_TC3_Val << GCLK_CLKCTRL_ID_Pos) /**< (GCLK_CLKCTRL) TCC2_TC3 Position  */
#define GCLK_CLKCTRL_ID_TC4_TC5               (GCLK_CLKCTRL_ID_TC4_TC5_Val << GCLK_CLKCTRL_ID_Pos) /**< (GCLK_CLKCTRL) TC4_TC5 Position  */
#define GCLK_CLKCTRL_ID_TC6_TC7               (GCLK_CLKCTRL_ID_TC6_TC7_Val << GCLK_CLKCTRL_ID_Pos) /**< (GCLK_CLKCTRL) TC6_TC7 Position  */
#define GCLK_CLKCTRL_ID_ADC                   (GCLK_CLKCTRL_ID_ADC_Val << GCLK_CLKCTRL_ID_Pos)     /**< (GCLK_CLKCTRL) ADC Position  */
#define GCLK_CLKCTRL_ID_AC_DIG                (GCLK_CLKCTRL_ID_AC_DIG_Val << GCLK_CLKCTRL_ID_Pos)  /**< (GCLK_CLKCTRL) AC_DIG Position  */
#define GCLK_CLKCTRL_ID_AC_ANA                (GCLK_CLKCTRL_ID_AC_ANA_Val << GCLK_CLKCTRL_ID_Pos)  /**< (GCLK_CLKCTRL) AC_ANA Position  */
#define GCLK_CLKCTRL_ID_DAC                   (GCLK_CLKCTRL_ID_DAC_Val << GCLK_CLKCTRL_ID_Pos)     /**< (GCLK_CLKCTRL) DAC Position  */
#define GCLK_CLKCTRL_ID_I2S_0                 (GCLK_CLKCTRL_ID_I2S_0_Val << GCLK_CLKCTRL_ID_Pos)   /**< (GCLK_CLKCTRL) I2S_0 Position  */
#define GCLK_CLKCTRL_ID_I2S_1                 (GCLK_CLKCTRL_ID_I2S_1_Val << GCLK_CLKCTRL_ID_Pos)   /**< (GCLK_CLKCTRL) I2S_1 Position  */
#define GCLK_CLKCTRL_GEN_Pos                  _U_(8)                                               /**< (GCLK_CLKCTRL) Generic Clock Generator Position */
#define GCLK_CLKCTRL_GEN_Msk                  (_U_(0xF) << GCLK_CLKCTRL_GEN_Pos)                   /**< (GCLK_CLKCTRL) Generic Clock Generator Mask */
#define GCLK_CLKCTRL_GEN(value)               (GCLK_CLKCTRL_GEN_Msk & ((value) << GCLK_CLKCTRL_GEN_Pos))
#define   GCLK_CLKCTRL_GEN_GCLK0_Val          _U_(0x0)                                             /**< (GCLK_CLKCTRL) Generic clock generator 0  */
#define   GCLK_CLKCTRL_GEN_GCLK1_Val          _U_(0x1)                                             /**< (GCLK_CLKCTRL) Generic clock generator 1  */
#define   GCLK_CLKCTRL_GEN_GCLK2_Val          _U_(0x2)                                             /**< (GCLK_CLKCTRL) Generic clock generator 2  */
#define   GCLK_CLKCTRL_GEN_GCLK3_Val          _U_(0x3)                                             /**< (GCLK_CLKCTRL) Generic clock generator 3  */
#define   GCLK_CLKCTRL_GEN_GCLK4_Val          _U_(0x4)                                             /**< (GCLK_CLKCTRL) Generic clock generator 4  */
#define   GCLK_CLKCTRL_GEN_GCLK5_Val          _U_(0x5)                                             /**< (GCLK_CLKCTRL) Generic clock generator 5  */
#define   GCLK_CLKCTRL_GEN_GCLK6_Val          _U_(0x6)                                             /**< (GCLK_CLKCTRL) Generic clock generator 6  */
#define   GCLK_CLKCTRL_GEN_GCLK7_Val          _U_(0x7)                                             /**< (GCLK_CLKCTRL) Generic clock generator 7  */
#define   GCLK_CLKCTRL_GEN_GCLK8_Val          _U_(0x8)                                             /**< (GCLK_CLKCTRL) Generic clock generator 8  */
#define GCLK_CLKCTRL_GEN_GCLK0                (GCLK_CLKCTRL_GEN_GCLK0_Val << GCLK_CLKCTRL_GEN_Pos) /**< (GCLK_CLKCTRL) Generic clock generator 0 Position  */
#define GCLK_CLKCTRL_GEN_GCLK1                (GCLK_CLKCTRL_GEN_GCLK1_Val << GCLK_CLKCTRL_GEN_Pos) /**< (GCLK_CLKCTRL) Generic clock generator 1 Position  */
#define GCLK_CLKCTRL_GEN_GCLK2                (GCLK_CLKCTRL_GEN_GCLK2_Val << GCLK_CLKCTRL_GEN_Pos) /**< (GCLK_CLKCTRL) Generic clock generator 2 Position  */
#define GCLK_CLKCTRL_GEN_GCLK3                (GCLK_CLKCTRL_GEN_GCLK3_Val << GCLK_CLKCTRL_GEN_Pos) /**< (GCLK_CLKCTRL) Generic clock generator 3 Position  */
#define GCLK_CLKCTRL_GEN_GCLK4                (GCLK_CLKCTRL_GEN_GCLK4_Val << GCLK_CLKCTRL_GEN_Pos) /**< (GCLK_CLKCTRL) Generic clock generator 4 Position  */
#define GCLK_CLKCTRL_GEN_GCLK5                (GCLK_CLKCTRL_GEN_GCLK5_Val << GCLK_CLKCTRL_GEN_Pos) /**< (GCLK_CLKCTRL) Generic clock generator 5 Position  */
#define GCLK_CLKCTRL_GEN_GCLK6                (GCLK_CLKCTRL_GEN_GCLK6_Val << GCLK_CLKCTRL_GEN_Pos) /**< (GCLK_CLKCTRL) Generic clock generator 6 Position  */
#define GCLK_CLKCTRL_GEN_GCLK7                (GCLK_CLKCTRL_GEN_GCLK7_Val << GCLK_CLKCTRL_GEN_Pos) /**< (GCLK_CLKCTRL) Generic clock generator 7 Position  */
#define GCLK_CLKCTRL_GEN_GCLK8                (GCLK_CLKCTRL_GEN_GCLK8_Val << GCLK_CLKCTRL_GEN_Pos) /**< (GCLK_CLKCTRL) Generic clock generator 8 Position  */
#define GCLK_CLKCTRL_CLKEN_Pos                _U_(14)                                              /**< (GCLK_CLKCTRL) Clock Enable Position */
#define GCLK_CLKCTRL_CLKEN_Msk                (_U_(0x1) << GCLK_CLKCTRL_CLKEN_Pos)                 /**< (GCLK_CLKCTRL) Clock Enable Mask */
#define GCLK_CLKCTRL_CLKEN(value)             (GCLK_CLKCTRL_CLKEN_Msk & ((value) << GCLK_CLKCTRL_CLKEN_Pos))
#define GCLK_CLKCTRL_WRTLOCK_Pos              _U_(15)                                              /**< (GCLK_CLKCTRL) Write Lock Position */
#define GCLK_CLKCTRL_WRTLOCK_Msk              (_U_(0x1) << GCLK_CLKCTRL_WRTLOCK_Pos)               /**< (GCLK_CLKCTRL) Write Lock Mask */
#define GCLK_CLKCTRL_WRTLOCK(value)           (GCLK_CLKCTRL_WRTLOCK_Msk & ((value) << GCLK_CLKCTRL_WRTLOCK_Pos))
#define GCLK_CLKCTRL_Msk                      _U_(0xCF3F)                                          /**< (GCLK_CLKCTRL) Register Mask  */


/* -------- GCLK_GENCTRL : (GCLK Offset: 0x04) (R/W 32) Generic Clock Generator Control -------- */
#define GCLK_GENCTRL_RESETVALUE               _U_(0x00)                                            /**<  (GCLK_GENCTRL) Generic Clock Generator Control  Reset Value */

#define GCLK_GENCTRL_ID_Pos                   _U_(0)                                               /**< (GCLK_GENCTRL) Generic Clock Generator Selection Position */
#define GCLK_GENCTRL_ID_Msk                   (_U_(0xF) << GCLK_GENCTRL_ID_Pos)                    /**< (GCLK_GENCTRL) Generic Clock Generator Selection Mask */
#define GCLK_GENCTRL_ID(value)                (GCLK_GENCTRL_ID_Msk & ((value) << GCLK_GENCTRL_ID_Pos))
#define GCLK_GENCTRL_SRC_Pos                  _U_(8)                                               /**< (GCLK_GENCTRL) Source Select Position */
#define GCLK_GENCTRL_SRC_Msk                  (_U_(0x1F) << GCLK_GENCTRL_SRC_Pos)                  /**< (GCLK_GENCTRL) Source Select Mask */
#define GCLK_GENCTRL_SRC(value)               (GCLK_GENCTRL_SRC_Msk & ((value) << GCLK_GENCTRL_SRC_Pos))
#define   GCLK_GENCTRL_SRC_XOSC_Val           _U_(0x0)                                             /**< (GCLK_GENCTRL) XOSC oscillator output  */
#define   GCLK_GENCTRL_SRC_GCLKIN_Val         _U_(0x1)                                             /**< (GCLK_GENCTRL) Generator input pad  */
#define   GCLK_GENCTRL_SRC_GCLKGEN1_Val       _U_(0x2)                                             /**< (GCLK_GENCTRL) Generic clock generator 1 output  */
#define   GCLK_GENCTRL_SRC_OSCULP32K_Val      _U_(0x3)                                             /**< (GCLK_GENCTRL) OSCULP32K oscillator output  */
#define   GCLK_GENCTRL_SRC_OSC32K_Val         _U_(0x4)                                             /**< (GCLK_GENCTRL) OSC32K oscillator output  */
#define   GCLK_GENCTRL_SRC_XOSC32K_Val        _U_(0x5)                                             /**< (GCLK_GENCTRL) XOSC32K oscillator output  */
#define   GCLK_GENCTRL_SRC_OSC8M_Val          _U_(0x6)                                             /**< (GCLK_GENCTRL) OSC8M oscillator output  */
#define   GCLK_GENCTRL_SRC_DFLL48M_Val        _U_(0x7)                                             /**< (GCLK_GENCTRL) DFLL48M output  */
#define   GCLK_GENCTRL_SRC_DPLL96M_Val        _U_(0x8)                                             /**< (GCLK_GENCTRL) DPLL96M output  */
#define GCLK_GENCTRL_SRC_XOSC                 (GCLK_GENCTRL_SRC_XOSC_Val << GCLK_GENCTRL_SRC_Pos)  /**< (GCLK_GENCTRL) XOSC oscillator output Position  */
#define GCLK_GENCTRL_SRC_GCLKIN               (GCLK_GENCTRL_SRC_GCLKIN_Val << GCLK_GENCTRL_SRC_Pos) /**< (GCLK_GENCTRL) Generator input pad Position  */
#define GCLK_GENCTRL_SRC_GCLKGEN1             (GCLK_GENCTRL_SRC_GCLKGEN1_Val << GCLK_GENCTRL_SRC_Pos) /**< (GCLK_GENCTRL) Generic clock generator 1 output Position  */
#define GCLK_GENCTRL_SRC_OSCULP32K            (GCLK_GENCTRL_SRC_OSCULP32K_Val << GCLK_GENCTRL_SRC_Pos) /**< (GCLK_GENCTRL) OSCULP32K oscillator output Position  */
#define GCLK_GENCTRL_SRC_OSC32K               (GCLK_GENCTRL_SRC_OSC32K_Val << GCLK_GENCTRL_SRC_Pos) /**< (GCLK_GENCTRL) OSC32K oscillator output Position  */
#define GCLK_GENCTRL_SRC_XOSC32K              (GCLK_GENCTRL_SRC_XOSC32K_Val << GCLK_GENCTRL_SRC_Pos) /**< (GCLK_GENCTRL) XOSC32K oscillator output Position  */
#define GCLK_GENCTRL_SRC_OSC8M                (GCLK_GENCTRL_SRC_OSC8M_Val << GCLK_GENCTRL_SRC_Pos) /**< (GCLK_GENCTRL) OSC8M oscillator output Position  */
#define GCLK_GENCTRL_SRC_DFLL48M              (GCLK_GENCTRL_SRC_DFLL48M_Val << GCLK_GENCTRL_SRC_Pos) /**< (GCLK_GENCTRL) DFLL48M output Position  */
#define GCLK_GENCTRL_SRC_DPLL96M              (GCLK_GENCTRL_SRC_DPLL96M_Val << GCLK_GENCTRL_SRC_Pos) /**< (GCLK_GENCTRL) DPLL96M output Position  */
#define GCLK_GENCTRL_GENEN_Pos                _U_(16)                                              /**< (GCLK_GENCTRL) Generic Clock Generator Enable Position */
#define GCLK_GENCTRL_GENEN_Msk                (_U_(0x1) << GCLK_GENCTRL_GENEN_Pos)                 /**< (GCLK_GENCTRL) Generic Clock Generator Enable Mask */
#define GCLK_GENCTRL_GENEN(value)             (GCLK_GENCTRL_GENEN_Msk & ((value) << GCLK_GENCTRL_GENEN_Pos))
#define GCLK_GENCTRL_IDC_Pos                  _U_(17)                                              /**< (GCLK_GENCTRL) Improve Duty Cycle Position */
#define GCLK_GENCTRL_IDC_Msk                  (_U_(0x1) << GCLK_GENCTRL_IDC_Pos)                   /**< (GCLK_GENCTRL) Improve Duty Cycle Mask */
#define GCLK_GENCTRL_IDC(value)               (GCLK_GENCTRL_IDC_Msk & ((value) << GCLK_GENCTRL_IDC_Pos))
#define GCLK_GENCTRL_OOV_Pos                  _U_(18)                                              /**< (GCLK_GENCTRL) Output Off Value Position */
#define GCLK_GENCTRL_OOV_Msk                  (_U_(0x1) << GCLK_GENCTRL_OOV_Pos)                   /**< (GCLK_GENCTRL) Output Off Value Mask */
#define GCLK_GENCTRL_OOV(value)               (GCLK_GENCTRL_OOV_Msk & ((value) << GCLK_GENCTRL_OOV_Pos))
#define GCLK_GENCTRL_OE_Pos                   _U_(19)                                              /**< (GCLK_GENCTRL) Output Enable Position */
#define GCLK_GENCTRL_OE_Msk                   (_U_(0x1) << GCLK_GENCTRL_OE_Pos)                    /**< (GCLK_GENCTRL) Output Enable Mask */
#define GCLK_GENCTRL_OE(value)                (GCLK_GENCTRL_OE_Msk & ((value) << GCLK_GENCTRL_OE_Pos))
#define GCLK_GENCTRL_DIVSEL_Pos               _U_(20)                                              /**< (GCLK_GENCTRL) Divide Selection Position */
#define GCLK_GENCTRL_DIVSEL_Msk               (_U_(0x1) << GCLK_GENCTRL_DIVSEL_Pos)                /**< (GCLK_GENCTRL) Divide Selection Mask */
#define GCLK_GENCTRL_DIVSEL(value)            (GCLK_GENCTRL_DIVSEL_Msk & ((value) << GCLK_GENCTRL_DIVSEL_Pos))
#define   GCLK_GENCTRL_DIVSEL_DIV1_Val        _U_(0x0)                                             /**< (GCLK_GENCTRL) Divide input directly by divider factor  */
#define   GCLK_GENCTRL_DIVSEL_DIV2_Val        _U_(0x1)                                             /**< (GCLK_GENCTRL) Divide input by 2^(divider factor+ 1)  */
#define GCLK_GENCTRL_DIVSEL_DIV1              (GCLK_GENCTRL_DIVSEL_DIV1_Val << GCLK_GENCTRL_DIVSEL_Pos) /**< (GCLK_GENCTRL) Divide input directly by divider factor Position  */
#define GCLK_GENCTRL_DIVSEL_DIV2              (GCLK_GENCTRL_DIVSEL_DIV2_Val << GCLK_GENCTRL_DIVSEL_Pos) /**< (GCLK_GENCTRL) Divide input by 2^(divider factor+ 1) Position  */
#define GCLK_GENCTRL_RUNSTDBY_Pos             _U_(21)                                              /**< (GCLK_GENCTRL) Run in Standby Position */
#define GCLK_GENCTRL_RUNSTDBY_Msk             (_U_(0x1) << GCLK_GENCTRL_RUNSTDBY_Pos)              /**< (GCLK_GENCTRL) Run in Standby Mask */
#define GCLK_GENCTRL_RUNSTDBY(value)          (GCLK_GENCTRL_RUNSTDBY_Msk & ((value) << GCLK_GENCTRL_RUNSTDBY_Pos))
#define GCLK_GENCTRL_Msk                      _U_(0x003F1F0F)                                      /**< (GCLK_GENCTRL) Register Mask  */


/* -------- GCLK_GENDIV : (GCLK Offset: 0x08) (R/W 32) Generic Clock Generator Division -------- */
#define GCLK_GENDIV_RESETVALUE                _U_(0x00)                                            /**<  (GCLK_GENDIV) Generic Clock Generator Division  Reset Value */

#define GCLK_GENDIV_ID_Pos                    _U_(0)                                               /**< (GCLK_GENDIV) Generic Clock Generator Selection Position */
#define GCLK_GENDIV_ID_Msk                    (_U_(0xF) << GCLK_GENDIV_ID_Pos)                     /**< (GCLK_GENDIV) Generic Clock Generator Selection Mask */
#define GCLK_GENDIV_ID(value)                 (GCLK_GENDIV_ID_Msk & ((value) << GCLK_GENDIV_ID_Pos))
#define GCLK_GENDIV_DIV_Pos                   _U_(8)                                               /**< (GCLK_GENDIV) Division Factor Position */
#define GCLK_GENDIV_DIV_Msk                   (_U_(0xFFFF) << GCLK_GENDIV_DIV_Pos)                 /**< (GCLK_GENDIV) Division Factor Mask */
#define GCLK_GENDIV_DIV(value)                (GCLK_GENDIV_DIV_Msk & ((value) << GCLK_GENDIV_DIV_Pos))
#define GCLK_GENDIV_Msk                       _U_(0x00FFFF0F)                                      /**< (GCLK_GENDIV) Register Mask  */


/** \brief GCLK register offsets definitions */
#define GCLK_CTRL_REG_OFST             (0x00)              /**< (GCLK_CTRL) Control Offset */
#define GCLK_STATUS_REG_OFST           (0x01)              /**< (GCLK_STATUS) Status Offset */
#define GCLK_CLKCTRL_REG_OFST          (0x02)              /**< (GCLK_CLKCTRL) Generic Clock Control Offset */
#define GCLK_GENCTRL_REG_OFST          (0x04)              /**< (GCLK_GENCTRL) Generic Clock Generator Control Offset */
#define GCLK_GENDIV_REG_OFST           (0x08)              /**< (GCLK_GENDIV) Generic Clock Generator Division Offset */

#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
/** \brief GCLK register API structure */
typedef struct
{  /* Generic Clock Generator */
  __IO  uint8_t                        GCLK_CTRL;          /**< Offset: 0x00 (R/W  8) Control */
  __I   uint8_t                        GCLK_STATUS;        /**< Offset: 0x01 (R/   8) Status */
  __IO  uint16_t                       GCLK_CLKCTRL;       /**< Offset: 0x02 (R/W  16) Generic Clock Control */
  __IO  uint32_t                       GCLK_GENCTRL;       /**< Offset: 0x04 (R/W  32) Generic Clock Generator Control */
  __IO  uint32_t                       GCLK_GENDIV;        /**< Offset: 0x08 (R/W  32) Generic Clock Generator Division */
} gclk_registers_t;


#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
#endif /* _SAMD21_GCLK_COMPONENT_H_ */
