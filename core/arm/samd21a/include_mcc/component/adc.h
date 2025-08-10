/**
 * \brief Component description for ADC
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
#ifndef _SAMD21_ADC_COMPONENT_H_
#define _SAMD21_ADC_COMPONENT_H_

/* ************************************************************************** */
/*   SOFTWARE API DEFINITION FOR ADC                                          */
/* ************************************************************************** */

/* -------- ADC_CTRLA : (ADC Offset: 0x00) (R/W 8) Control A -------- */
#define ADC_CTRLA_RESETVALUE                  _U_(0x00)                                            /**<  (ADC_CTRLA) Control A  Reset Value */

#define ADC_CTRLA_SWRST_Pos                   _U_(0)                                               /**< (ADC_CTRLA) Software Reset Position */
#define ADC_CTRLA_SWRST_Msk                   (_U_(0x1) << ADC_CTRLA_SWRST_Pos)                    /**< (ADC_CTRLA) Software Reset Mask */
#define ADC_CTRLA_SWRST(value)                (ADC_CTRLA_SWRST_Msk & ((value) << ADC_CTRLA_SWRST_Pos))
#define ADC_CTRLA_ENABLE_Pos                  _U_(1)                                               /**< (ADC_CTRLA) Enable Position */
#define ADC_CTRLA_ENABLE_Msk                  (_U_(0x1) << ADC_CTRLA_ENABLE_Pos)                   /**< (ADC_CTRLA) Enable Mask */
#define ADC_CTRLA_ENABLE(value)               (ADC_CTRLA_ENABLE_Msk & ((value) << ADC_CTRLA_ENABLE_Pos))
#define ADC_CTRLA_RUNSTDBY_Pos                _U_(2)                                               /**< (ADC_CTRLA) Run in Standby Position */
#define ADC_CTRLA_RUNSTDBY_Msk                (_U_(0x1) << ADC_CTRLA_RUNSTDBY_Pos)                 /**< (ADC_CTRLA) Run in Standby Mask */
#define ADC_CTRLA_RUNSTDBY(value)             (ADC_CTRLA_RUNSTDBY_Msk & ((value) << ADC_CTRLA_RUNSTDBY_Pos))
#define ADC_CTRLA_Msk                         _U_(0x07)                                            /**< (ADC_CTRLA) Register Mask  */


/* -------- ADC_REFCTRL : (ADC Offset: 0x01) (R/W 8) Reference Control -------- */
#define ADC_REFCTRL_RESETVALUE                _U_(0x00)                                            /**<  (ADC_REFCTRL) Reference Control  Reset Value */

#define ADC_REFCTRL_REFSEL_Pos                _U_(0)                                               /**< (ADC_REFCTRL) Reference Selection Position */
#define ADC_REFCTRL_REFSEL_Msk                (_U_(0xF) << ADC_REFCTRL_REFSEL_Pos)                 /**< (ADC_REFCTRL) Reference Selection Mask */
#define ADC_REFCTRL_REFSEL(value)             (ADC_REFCTRL_REFSEL_Msk & ((value) << ADC_REFCTRL_REFSEL_Pos))
#define   ADC_REFCTRL_REFSEL_INT1V_Val        _U_(0x0)                                             /**< (ADC_REFCTRL) 1.0V voltage reference  */
#define   ADC_REFCTRL_REFSEL_INTVCC0_Val      _U_(0x1)                                             /**< (ADC_REFCTRL) 1/1.48 VDDANA  */
#define   ADC_REFCTRL_REFSEL_INTVCC1_Val      _U_(0x2)                                             /**< (ADC_REFCTRL) 1/2 VDDANA (only for VDDANA > 2.0V)  */
#define   ADC_REFCTRL_REFSEL_AREFA_Val        _U_(0x3)                                             /**< (ADC_REFCTRL) External reference A  */
#define   ADC_REFCTRL_REFSEL_AREFB_Val        _U_(0x4)                                             /**< (ADC_REFCTRL) External reference B  */
#define ADC_REFCTRL_REFSEL_INT1V              (ADC_REFCTRL_REFSEL_INT1V_Val << ADC_REFCTRL_REFSEL_Pos) /**< (ADC_REFCTRL) 1.0V voltage reference Position  */
#define ADC_REFCTRL_REFSEL_INTVCC0            (ADC_REFCTRL_REFSEL_INTVCC0_Val << ADC_REFCTRL_REFSEL_Pos) /**< (ADC_REFCTRL) 1/1.48 VDDANA Position  */
#define ADC_REFCTRL_REFSEL_INTVCC1            (ADC_REFCTRL_REFSEL_INTVCC1_Val << ADC_REFCTRL_REFSEL_Pos) /**< (ADC_REFCTRL) 1/2 VDDANA (only for VDDANA > 2.0V) Position  */
#define ADC_REFCTRL_REFSEL_AREFA              (ADC_REFCTRL_REFSEL_AREFA_Val << ADC_REFCTRL_REFSEL_Pos) /**< (ADC_REFCTRL) External reference A Position  */
#define ADC_REFCTRL_REFSEL_AREFB              (ADC_REFCTRL_REFSEL_AREFB_Val << ADC_REFCTRL_REFSEL_Pos) /**< (ADC_REFCTRL) External reference B Position  */
#define ADC_REFCTRL_REFCOMP_Pos               _U_(7)                                               /**< (ADC_REFCTRL) Reference Buffer Offset Compensation Enable Position */
#define ADC_REFCTRL_REFCOMP_Msk               (_U_(0x1) << ADC_REFCTRL_REFCOMP_Pos)                /**< (ADC_REFCTRL) Reference Buffer Offset Compensation Enable Mask */
#define ADC_REFCTRL_REFCOMP(value)            (ADC_REFCTRL_REFCOMP_Msk & ((value) << ADC_REFCTRL_REFCOMP_Pos))
#define ADC_REFCTRL_Msk                       _U_(0x8F)                                            /**< (ADC_REFCTRL) Register Mask  */


/* -------- ADC_AVGCTRL : (ADC Offset: 0x02) (R/W 8) Average Control -------- */
#define ADC_AVGCTRL_RESETVALUE                _U_(0x00)                                            /**<  (ADC_AVGCTRL) Average Control  Reset Value */

#define ADC_AVGCTRL_SAMPLENUM_Pos             _U_(0)                                               /**< (ADC_AVGCTRL) Number of Samples to be Collected Position */
#define ADC_AVGCTRL_SAMPLENUM_Msk             (_U_(0xF) << ADC_AVGCTRL_SAMPLENUM_Pos)              /**< (ADC_AVGCTRL) Number of Samples to be Collected Mask */
#define ADC_AVGCTRL_SAMPLENUM(value)          (ADC_AVGCTRL_SAMPLENUM_Msk & ((value) << ADC_AVGCTRL_SAMPLENUM_Pos))
#define   ADC_AVGCTRL_SAMPLENUM_1_Val         _U_(0x0)                                             /**< (ADC_AVGCTRL) 1 sample  */
#define   ADC_AVGCTRL_SAMPLENUM_2_Val         _U_(0x1)                                             /**< (ADC_AVGCTRL) 2 samples  */
#define   ADC_AVGCTRL_SAMPLENUM_4_Val         _U_(0x2)                                             /**< (ADC_AVGCTRL) 4 samples  */
#define   ADC_AVGCTRL_SAMPLENUM_8_Val         _U_(0x3)                                             /**< (ADC_AVGCTRL) 8 samples  */
#define   ADC_AVGCTRL_SAMPLENUM_16_Val        _U_(0x4)                                             /**< (ADC_AVGCTRL) 16 samples  */
#define   ADC_AVGCTRL_SAMPLENUM_32_Val        _U_(0x5)                                             /**< (ADC_AVGCTRL) 32 samples  */
#define   ADC_AVGCTRL_SAMPLENUM_64_Val        _U_(0x6)                                             /**< (ADC_AVGCTRL) 64 samples  */
#define   ADC_AVGCTRL_SAMPLENUM_128_Val       _U_(0x7)                                             /**< (ADC_AVGCTRL) 128 samples  */
#define   ADC_AVGCTRL_SAMPLENUM_256_Val       _U_(0x8)                                             /**< (ADC_AVGCTRL) 256 samples  */
#define   ADC_AVGCTRL_SAMPLENUM_512_Val       _U_(0x9)                                             /**< (ADC_AVGCTRL) 512 samples  */
#define   ADC_AVGCTRL_SAMPLENUM_1024_Val      _U_(0xA)                                             /**< (ADC_AVGCTRL) 1024 samples  */
#define ADC_AVGCTRL_SAMPLENUM_1               (ADC_AVGCTRL_SAMPLENUM_1_Val << ADC_AVGCTRL_SAMPLENUM_Pos) /**< (ADC_AVGCTRL) 1 sample Position  */
#define ADC_AVGCTRL_SAMPLENUM_2               (ADC_AVGCTRL_SAMPLENUM_2_Val << ADC_AVGCTRL_SAMPLENUM_Pos) /**< (ADC_AVGCTRL) 2 samples Position  */
#define ADC_AVGCTRL_SAMPLENUM_4               (ADC_AVGCTRL_SAMPLENUM_4_Val << ADC_AVGCTRL_SAMPLENUM_Pos) /**< (ADC_AVGCTRL) 4 samples Position  */
#define ADC_AVGCTRL_SAMPLENUM_8               (ADC_AVGCTRL_SAMPLENUM_8_Val << ADC_AVGCTRL_SAMPLENUM_Pos) /**< (ADC_AVGCTRL) 8 samples Position  */
#define ADC_AVGCTRL_SAMPLENUM_16              (ADC_AVGCTRL_SAMPLENUM_16_Val << ADC_AVGCTRL_SAMPLENUM_Pos) /**< (ADC_AVGCTRL) 16 samples Position  */
#define ADC_AVGCTRL_SAMPLENUM_32              (ADC_AVGCTRL_SAMPLENUM_32_Val << ADC_AVGCTRL_SAMPLENUM_Pos) /**< (ADC_AVGCTRL) 32 samples Position  */
#define ADC_AVGCTRL_SAMPLENUM_64              (ADC_AVGCTRL_SAMPLENUM_64_Val << ADC_AVGCTRL_SAMPLENUM_Pos) /**< (ADC_AVGCTRL) 64 samples Position  */
#define ADC_AVGCTRL_SAMPLENUM_128             (ADC_AVGCTRL_SAMPLENUM_128_Val << ADC_AVGCTRL_SAMPLENUM_Pos) /**< (ADC_AVGCTRL) 128 samples Position  */
#define ADC_AVGCTRL_SAMPLENUM_256             (ADC_AVGCTRL_SAMPLENUM_256_Val << ADC_AVGCTRL_SAMPLENUM_Pos) /**< (ADC_AVGCTRL) 256 samples Position  */
#define ADC_AVGCTRL_SAMPLENUM_512             (ADC_AVGCTRL_SAMPLENUM_512_Val << ADC_AVGCTRL_SAMPLENUM_Pos) /**< (ADC_AVGCTRL) 512 samples Position  */
#define ADC_AVGCTRL_SAMPLENUM_1024            (ADC_AVGCTRL_SAMPLENUM_1024_Val << ADC_AVGCTRL_SAMPLENUM_Pos) /**< (ADC_AVGCTRL) 1024 samples Position  */
#define ADC_AVGCTRL_ADJRES_Pos                _U_(4)                                               /**< (ADC_AVGCTRL) Adjusting Result / Division Coefficient Position */
#define ADC_AVGCTRL_ADJRES_Msk                (_U_(0x7) << ADC_AVGCTRL_ADJRES_Pos)                 /**< (ADC_AVGCTRL) Adjusting Result / Division Coefficient Mask */
#define ADC_AVGCTRL_ADJRES(value)             (ADC_AVGCTRL_ADJRES_Msk & ((value) << ADC_AVGCTRL_ADJRES_Pos))
#define ADC_AVGCTRL_Msk                       _U_(0x7F)                                            /**< (ADC_AVGCTRL) Register Mask  */


/* -------- ADC_SAMPCTRL : (ADC Offset: 0x03) (R/W 8) Sampling Time Control -------- */
#define ADC_SAMPCTRL_RESETVALUE               _U_(0x00)                                            /**<  (ADC_SAMPCTRL) Sampling Time Control  Reset Value */

#define ADC_SAMPCTRL_SAMPLEN_Pos              _U_(0)                                               /**< (ADC_SAMPCTRL) Sampling Time Length Position */
#define ADC_SAMPCTRL_SAMPLEN_Msk              (_U_(0x3F) << ADC_SAMPCTRL_SAMPLEN_Pos)              /**< (ADC_SAMPCTRL) Sampling Time Length Mask */
#define ADC_SAMPCTRL_SAMPLEN(value)           (ADC_SAMPCTRL_SAMPLEN_Msk & ((value) << ADC_SAMPCTRL_SAMPLEN_Pos))
#define ADC_SAMPCTRL_Msk                      _U_(0x3F)                                            /**< (ADC_SAMPCTRL) Register Mask  */


/* -------- ADC_CTRLB : (ADC Offset: 0x04) (R/W 16) Control B -------- */
#define ADC_CTRLB_RESETVALUE                  _U_(0x00)                                            /**<  (ADC_CTRLB) Control B  Reset Value */

#define ADC_CTRLB_DIFFMODE_Pos                _U_(0)                                               /**< (ADC_CTRLB) Differential Mode Position */
#define ADC_CTRLB_DIFFMODE_Msk                (_U_(0x1) << ADC_CTRLB_DIFFMODE_Pos)                 /**< (ADC_CTRLB) Differential Mode Mask */
#define ADC_CTRLB_DIFFMODE(value)             (ADC_CTRLB_DIFFMODE_Msk & ((value) << ADC_CTRLB_DIFFMODE_Pos))
#define ADC_CTRLB_LEFTADJ_Pos                 _U_(1)                                               /**< (ADC_CTRLB) Left-Adjusted Result Position */
#define ADC_CTRLB_LEFTADJ_Msk                 (_U_(0x1) << ADC_CTRLB_LEFTADJ_Pos)                  /**< (ADC_CTRLB) Left-Adjusted Result Mask */
#define ADC_CTRLB_LEFTADJ(value)              (ADC_CTRLB_LEFTADJ_Msk & ((value) << ADC_CTRLB_LEFTADJ_Pos))
#define ADC_CTRLB_FREERUN_Pos                 _U_(2)                                               /**< (ADC_CTRLB) Free Running Mode Position */
#define ADC_CTRLB_FREERUN_Msk                 (_U_(0x1) << ADC_CTRLB_FREERUN_Pos)                  /**< (ADC_CTRLB) Free Running Mode Mask */
#define ADC_CTRLB_FREERUN(value)              (ADC_CTRLB_FREERUN_Msk & ((value) << ADC_CTRLB_FREERUN_Pos))
#define ADC_CTRLB_CORREN_Pos                  _U_(3)                                               /**< (ADC_CTRLB) Digital Correction Logic Enabled Position */
#define ADC_CTRLB_CORREN_Msk                  (_U_(0x1) << ADC_CTRLB_CORREN_Pos)                   /**< (ADC_CTRLB) Digital Correction Logic Enabled Mask */
#define ADC_CTRLB_CORREN(value)               (ADC_CTRLB_CORREN_Msk & ((value) << ADC_CTRLB_CORREN_Pos))
#define ADC_CTRLB_RESSEL_Pos                  _U_(4)                                               /**< (ADC_CTRLB) Conversion Result Resolution Position */
#define ADC_CTRLB_RESSEL_Msk                  (_U_(0x3) << ADC_CTRLB_RESSEL_Pos)                   /**< (ADC_CTRLB) Conversion Result Resolution Mask */
#define ADC_CTRLB_RESSEL(value)               (ADC_CTRLB_RESSEL_Msk & ((value) << ADC_CTRLB_RESSEL_Pos))
#define   ADC_CTRLB_RESSEL_12BIT_Val          _U_(0x0)                                             /**< (ADC_CTRLB) 12-bit result  */
#define   ADC_CTRLB_RESSEL_16BIT_Val          _U_(0x1)                                             /**< (ADC_CTRLB) 16-bit averaging mode  */
#define   ADC_CTRLB_RESSEL_10BIT_Val          _U_(0x2)                                             /**< (ADC_CTRLB) 10-bit result  */
#define   ADC_CTRLB_RESSEL_8BIT_Val           _U_(0x3)                                             /**< (ADC_CTRLB) 8-bit result  */
#define ADC_CTRLB_RESSEL_12BIT                (ADC_CTRLB_RESSEL_12BIT_Val << ADC_CTRLB_RESSEL_Pos) /**< (ADC_CTRLB) 12-bit result Position  */
#define ADC_CTRLB_RESSEL_16BIT                (ADC_CTRLB_RESSEL_16BIT_Val << ADC_CTRLB_RESSEL_Pos) /**< (ADC_CTRLB) 16-bit averaging mode Position  */
#define ADC_CTRLB_RESSEL_10BIT                (ADC_CTRLB_RESSEL_10BIT_Val << ADC_CTRLB_RESSEL_Pos) /**< (ADC_CTRLB) 10-bit result Position  */
#define ADC_CTRLB_RESSEL_8BIT                 (ADC_CTRLB_RESSEL_8BIT_Val << ADC_CTRLB_RESSEL_Pos)  /**< (ADC_CTRLB) 8-bit result Position  */
#define ADC_CTRLB_PRESCALER_Pos               _U_(8)                                               /**< (ADC_CTRLB) Prescaler Configuration Position */
#define ADC_CTRLB_PRESCALER_Msk               (_U_(0x7) << ADC_CTRLB_PRESCALER_Pos)                /**< (ADC_CTRLB) Prescaler Configuration Mask */
#define ADC_CTRLB_PRESCALER(value)            (ADC_CTRLB_PRESCALER_Msk & ((value) << ADC_CTRLB_PRESCALER_Pos))
#define   ADC_CTRLB_PRESCALER_DIV4_Val        _U_(0x0)                                             /**< (ADC_CTRLB) Peripheral clock divided by 4  */
#define   ADC_CTRLB_PRESCALER_DIV8_Val        _U_(0x1)                                             /**< (ADC_CTRLB) Peripheral clock divided by 8  */
#define   ADC_CTRLB_PRESCALER_DIV16_Val       _U_(0x2)                                             /**< (ADC_CTRLB) Peripheral clock divided by 16  */
#define   ADC_CTRLB_PRESCALER_DIV32_Val       _U_(0x3)                                             /**< (ADC_CTRLB) Peripheral clock divided by 32  */
#define   ADC_CTRLB_PRESCALER_DIV64_Val       _U_(0x4)                                             /**< (ADC_CTRLB) Peripheral clock divided by 64  */
#define   ADC_CTRLB_PRESCALER_DIV128_Val      _U_(0x5)                                             /**< (ADC_CTRLB) Peripheral clock divided by 128  */
#define   ADC_CTRLB_PRESCALER_DIV256_Val      _U_(0x6)                                             /**< (ADC_CTRLB) Peripheral clock divided by 256  */
#define   ADC_CTRLB_PRESCALER_DIV512_Val      _U_(0x7)                                             /**< (ADC_CTRLB) Peripheral clock divided by 512  */
#define ADC_CTRLB_PRESCALER_DIV4              (ADC_CTRLB_PRESCALER_DIV4_Val << ADC_CTRLB_PRESCALER_Pos) /**< (ADC_CTRLB) Peripheral clock divided by 4 Position  */
#define ADC_CTRLB_PRESCALER_DIV8              (ADC_CTRLB_PRESCALER_DIV8_Val << ADC_CTRLB_PRESCALER_Pos) /**< (ADC_CTRLB) Peripheral clock divided by 8 Position  */
#define ADC_CTRLB_PRESCALER_DIV16             (ADC_CTRLB_PRESCALER_DIV16_Val << ADC_CTRLB_PRESCALER_Pos) /**< (ADC_CTRLB) Peripheral clock divided by 16 Position  */
#define ADC_CTRLB_PRESCALER_DIV32             (ADC_CTRLB_PRESCALER_DIV32_Val << ADC_CTRLB_PRESCALER_Pos) /**< (ADC_CTRLB) Peripheral clock divided by 32 Position  */
#define ADC_CTRLB_PRESCALER_DIV64             (ADC_CTRLB_PRESCALER_DIV64_Val << ADC_CTRLB_PRESCALER_Pos) /**< (ADC_CTRLB) Peripheral clock divided by 64 Position  */
#define ADC_CTRLB_PRESCALER_DIV128            (ADC_CTRLB_PRESCALER_DIV128_Val << ADC_CTRLB_PRESCALER_Pos) /**< (ADC_CTRLB) Peripheral clock divided by 128 Position  */
#define ADC_CTRLB_PRESCALER_DIV256            (ADC_CTRLB_PRESCALER_DIV256_Val << ADC_CTRLB_PRESCALER_Pos) /**< (ADC_CTRLB) Peripheral clock divided by 256 Position  */
#define ADC_CTRLB_PRESCALER_DIV512            (ADC_CTRLB_PRESCALER_DIV512_Val << ADC_CTRLB_PRESCALER_Pos) /**< (ADC_CTRLB) Peripheral clock divided by 512 Position  */
#define ADC_CTRLB_Msk                         _U_(0x073F)                                          /**< (ADC_CTRLB) Register Mask  */


/* -------- ADC_WINCTRL : (ADC Offset: 0x08) (R/W 8) Window Monitor Control -------- */
#define ADC_WINCTRL_RESETVALUE                _U_(0x00)                                            /**<  (ADC_WINCTRL) Window Monitor Control  Reset Value */

#define ADC_WINCTRL_WINMODE_Pos               _U_(0)                                               /**< (ADC_WINCTRL) Window Monitor Mode Position */
#define ADC_WINCTRL_WINMODE_Msk               (_U_(0x7) << ADC_WINCTRL_WINMODE_Pos)                /**< (ADC_WINCTRL) Window Monitor Mode Mask */
#define ADC_WINCTRL_WINMODE(value)            (ADC_WINCTRL_WINMODE_Msk & ((value) << ADC_WINCTRL_WINMODE_Pos))
#define   ADC_WINCTRL_WINMODE_DISABLE_Val     _U_(0x0)                                             /**< (ADC_WINCTRL) No window mode (default)  */
#define   ADC_WINCTRL_WINMODE_MODE1_Val       _U_(0x1)                                             /**< (ADC_WINCTRL) Mode 1: RESULT > WINLT  */
#define   ADC_WINCTRL_WINMODE_MODE2_Val       _U_(0x2)                                             /**< (ADC_WINCTRL) Mode 2: RESULT < WINUT  */
#define   ADC_WINCTRL_WINMODE_MODE3_Val       _U_(0x3)                                             /**< (ADC_WINCTRL) Mode 3: WINLT < RESULT < WINUT  */
#define   ADC_WINCTRL_WINMODE_MODE4_Val       _U_(0x4)                                             /**< (ADC_WINCTRL) Mode 4: !(WINLT < RESULT < WINUT)  */
#define ADC_WINCTRL_WINMODE_DISABLE           (ADC_WINCTRL_WINMODE_DISABLE_Val << ADC_WINCTRL_WINMODE_Pos) /**< (ADC_WINCTRL) No window mode (default) Position  */
#define ADC_WINCTRL_WINMODE_MODE1             (ADC_WINCTRL_WINMODE_MODE1_Val << ADC_WINCTRL_WINMODE_Pos) /**< (ADC_WINCTRL) Mode 1: RESULT > WINLT Position  */
#define ADC_WINCTRL_WINMODE_MODE2             (ADC_WINCTRL_WINMODE_MODE2_Val << ADC_WINCTRL_WINMODE_Pos) /**< (ADC_WINCTRL) Mode 2: RESULT < WINUT Position  */
#define ADC_WINCTRL_WINMODE_MODE3             (ADC_WINCTRL_WINMODE_MODE3_Val << ADC_WINCTRL_WINMODE_Pos) /**< (ADC_WINCTRL) Mode 3: WINLT < RESULT < WINUT Position  */
#define ADC_WINCTRL_WINMODE_MODE4             (ADC_WINCTRL_WINMODE_MODE4_Val << ADC_WINCTRL_WINMODE_Pos) /**< (ADC_WINCTRL) Mode 4: !(WINLT < RESULT < WINUT) Position  */
#define ADC_WINCTRL_Msk                       _U_(0x07)                                            /**< (ADC_WINCTRL) Register Mask  */


/* -------- ADC_SWTRIG : (ADC Offset: 0x0C) (R/W 8) Software Trigger -------- */
#define ADC_SWTRIG_RESETVALUE                 _U_(0x00)                                            /**<  (ADC_SWTRIG) Software Trigger  Reset Value */

#define ADC_SWTRIG_FLUSH_Pos                  _U_(0)                                               /**< (ADC_SWTRIG) ADC Conversion Flush Position */
#define ADC_SWTRIG_FLUSH_Msk                  (_U_(0x1) << ADC_SWTRIG_FLUSH_Pos)                   /**< (ADC_SWTRIG) ADC Conversion Flush Mask */
#define ADC_SWTRIG_FLUSH(value)               (ADC_SWTRIG_FLUSH_Msk & ((value) << ADC_SWTRIG_FLUSH_Pos))
#define ADC_SWTRIG_START_Pos                  _U_(1)                                               /**< (ADC_SWTRIG) ADC Start Conversion Position */
#define ADC_SWTRIG_START_Msk                  (_U_(0x1) << ADC_SWTRIG_START_Pos)                   /**< (ADC_SWTRIG) ADC Start Conversion Mask */
#define ADC_SWTRIG_START(value)               (ADC_SWTRIG_START_Msk & ((value) << ADC_SWTRIG_START_Pos))
#define ADC_SWTRIG_Msk                        _U_(0x03)                                            /**< (ADC_SWTRIG) Register Mask  */


/* -------- ADC_INPUTCTRL : (ADC Offset: 0x10) (R/W 32) Input Control -------- */
#define ADC_INPUTCTRL_RESETVALUE              _U_(0x00)                                            /**<  (ADC_INPUTCTRL) Input Control  Reset Value */

#define ADC_INPUTCTRL_MUXPOS_Pos              _U_(0)                                               /**< (ADC_INPUTCTRL) Positive Mux Input Selection Position */
#define ADC_INPUTCTRL_MUXPOS_Msk              (_U_(0x1F) << ADC_INPUTCTRL_MUXPOS_Pos)              /**< (ADC_INPUTCTRL) Positive Mux Input Selection Mask */
#define ADC_INPUTCTRL_MUXPOS(value)           (ADC_INPUTCTRL_MUXPOS_Msk & ((value) << ADC_INPUTCTRL_MUXPOS_Pos))
#define   ADC_INPUTCTRL_MUXPOS_PIN0_Val       _U_(0x0)                                             /**< (ADC_INPUTCTRL) ADC AIN0 Pin  */
#define   ADC_INPUTCTRL_MUXPOS_PIN1_Val       _U_(0x1)                                             /**< (ADC_INPUTCTRL) ADC AIN1 Pin  */
#define   ADC_INPUTCTRL_MUXPOS_PIN2_Val       _U_(0x2)                                             /**< (ADC_INPUTCTRL) ADC AIN2 Pin  */
#define   ADC_INPUTCTRL_MUXPOS_PIN3_Val       _U_(0x3)                                             /**< (ADC_INPUTCTRL) ADC AIN3 Pin  */
#define   ADC_INPUTCTRL_MUXPOS_PIN4_Val       _U_(0x4)                                             /**< (ADC_INPUTCTRL) ADC AIN4 Pin  */
#define   ADC_INPUTCTRL_MUXPOS_PIN5_Val       _U_(0x5)                                             /**< (ADC_INPUTCTRL) ADC AIN5 Pin  */
#define   ADC_INPUTCTRL_MUXPOS_PIN6_Val       _U_(0x6)                                             /**< (ADC_INPUTCTRL) ADC AIN6 Pin  */
#define   ADC_INPUTCTRL_MUXPOS_PIN7_Val       _U_(0x7)                                             /**< (ADC_INPUTCTRL) ADC AIN7 Pin  */
#define   ADC_INPUTCTRL_MUXPOS_PIN8_Val       _U_(0x8)                                             /**< (ADC_INPUTCTRL) ADC AIN8 Pin  */
#define   ADC_INPUTCTRL_MUXPOS_PIN9_Val       _U_(0x9)                                             /**< (ADC_INPUTCTRL) ADC AIN9 Pin  */
#define   ADC_INPUTCTRL_MUXPOS_PIN10_Val      _U_(0xA)                                             /**< (ADC_INPUTCTRL) ADC AIN10 Pin  */
#define   ADC_INPUTCTRL_MUXPOS_PIN11_Val      _U_(0xB)                                             /**< (ADC_INPUTCTRL) ADC AIN11 Pin  */
#define   ADC_INPUTCTRL_MUXPOS_PIN12_Val      _U_(0xC)                                             /**< (ADC_INPUTCTRL) ADC AIN12 Pin  */
#define   ADC_INPUTCTRL_MUXPOS_PIN13_Val      _U_(0xD)                                             /**< (ADC_INPUTCTRL) ADC AIN13 Pin  */
#define   ADC_INPUTCTRL_MUXPOS_PIN14_Val      _U_(0xE)                                             /**< (ADC_INPUTCTRL) ADC AIN14 Pin  */
#define   ADC_INPUTCTRL_MUXPOS_PIN15_Val      _U_(0xF)                                             /**< (ADC_INPUTCTRL) ADC AIN15 Pin  */
#define   ADC_INPUTCTRL_MUXPOS_PIN16_Val      _U_(0x10)                                            /**< (ADC_INPUTCTRL) ADC AIN16 Pin  */
#define   ADC_INPUTCTRL_MUXPOS_PIN17_Val      _U_(0x11)                                            /**< (ADC_INPUTCTRL) ADC AIN17 Pin  */
#define   ADC_INPUTCTRL_MUXPOS_PIN18_Val      _U_(0x12)                                            /**< (ADC_INPUTCTRL) ADC AIN18 Pin  */
#define   ADC_INPUTCTRL_MUXPOS_PIN19_Val      _U_(0x13)                                            /**< (ADC_INPUTCTRL) ADC AIN19 Pin  */
#define   ADC_INPUTCTRL_MUXPOS_TEMP_Val       _U_(0x18)                                            /**< (ADC_INPUTCTRL) Temperature Reference  */
#define   ADC_INPUTCTRL_MUXPOS_BANDGAP_Val    _U_(0x19)                                            /**< (ADC_INPUTCTRL) Bandgap Voltage  */
#define   ADC_INPUTCTRL_MUXPOS_SCALEDCOREVCC_Val _U_(0x1A)                                            /**< (ADC_INPUTCTRL) 1/4  Scaled Core Supply  */
#define   ADC_INPUTCTRL_MUXPOS_SCALEDIOVCC_Val _U_(0x1B)                                            /**< (ADC_INPUTCTRL) 1/4  Scaled I/O Supply  */
#define   ADC_INPUTCTRL_MUXPOS_DAC_Val        _U_(0x1C)                                            /**< (ADC_INPUTCTRL) DAC Output  */
#define ADC_INPUTCTRL_MUXPOS_PIN0             (ADC_INPUTCTRL_MUXPOS_PIN0_Val << ADC_INPUTCTRL_MUXPOS_Pos) /**< (ADC_INPUTCTRL) ADC AIN0 Pin Position  */
#define ADC_INPUTCTRL_MUXPOS_PIN1             (ADC_INPUTCTRL_MUXPOS_PIN1_Val << ADC_INPUTCTRL_MUXPOS_Pos) /**< (ADC_INPUTCTRL) ADC AIN1 Pin Position  */
#define ADC_INPUTCTRL_MUXPOS_PIN2             (ADC_INPUTCTRL_MUXPOS_PIN2_Val << ADC_INPUTCTRL_MUXPOS_Pos) /**< (ADC_INPUTCTRL) ADC AIN2 Pin Position  */
#define ADC_INPUTCTRL_MUXPOS_PIN3             (ADC_INPUTCTRL_MUXPOS_PIN3_Val << ADC_INPUTCTRL_MUXPOS_Pos) /**< (ADC_INPUTCTRL) ADC AIN3 Pin Position  */
#define ADC_INPUTCTRL_MUXPOS_PIN4             (ADC_INPUTCTRL_MUXPOS_PIN4_Val << ADC_INPUTCTRL_MUXPOS_Pos) /**< (ADC_INPUTCTRL) ADC AIN4 Pin Position  */
#define ADC_INPUTCTRL_MUXPOS_PIN5             (ADC_INPUTCTRL_MUXPOS_PIN5_Val << ADC_INPUTCTRL_MUXPOS_Pos) /**< (ADC_INPUTCTRL) ADC AIN5 Pin Position  */
#define ADC_INPUTCTRL_MUXPOS_PIN6             (ADC_INPUTCTRL_MUXPOS_PIN6_Val << ADC_INPUTCTRL_MUXPOS_Pos) /**< (ADC_INPUTCTRL) ADC AIN6 Pin Position  */
#define ADC_INPUTCTRL_MUXPOS_PIN7             (ADC_INPUTCTRL_MUXPOS_PIN7_Val << ADC_INPUTCTRL_MUXPOS_Pos) /**< (ADC_INPUTCTRL) ADC AIN7 Pin Position  */
#define ADC_INPUTCTRL_MUXPOS_PIN8             (ADC_INPUTCTRL_MUXPOS_PIN8_Val << ADC_INPUTCTRL_MUXPOS_Pos) /**< (ADC_INPUTCTRL) ADC AIN8 Pin Position  */
#define ADC_INPUTCTRL_MUXPOS_PIN9             (ADC_INPUTCTRL_MUXPOS_PIN9_Val << ADC_INPUTCTRL_MUXPOS_Pos) /**< (ADC_INPUTCTRL) ADC AIN9 Pin Position  */
#define ADC_INPUTCTRL_MUXPOS_PIN10            (ADC_INPUTCTRL_MUXPOS_PIN10_Val << ADC_INPUTCTRL_MUXPOS_Pos) /**< (ADC_INPUTCTRL) ADC AIN10 Pin Position  */
#define ADC_INPUTCTRL_MUXPOS_PIN11            (ADC_INPUTCTRL_MUXPOS_PIN11_Val << ADC_INPUTCTRL_MUXPOS_Pos) /**< (ADC_INPUTCTRL) ADC AIN11 Pin Position  */
#define ADC_INPUTCTRL_MUXPOS_PIN12            (ADC_INPUTCTRL_MUXPOS_PIN12_Val << ADC_INPUTCTRL_MUXPOS_Pos) /**< (ADC_INPUTCTRL) ADC AIN12 Pin Position  */
#define ADC_INPUTCTRL_MUXPOS_PIN13            (ADC_INPUTCTRL_MUXPOS_PIN13_Val << ADC_INPUTCTRL_MUXPOS_Pos) /**< (ADC_INPUTCTRL) ADC AIN13 Pin Position  */
#define ADC_INPUTCTRL_MUXPOS_PIN14            (ADC_INPUTCTRL_MUXPOS_PIN14_Val << ADC_INPUTCTRL_MUXPOS_Pos) /**< (ADC_INPUTCTRL) ADC AIN14 Pin Position  */
#define ADC_INPUTCTRL_MUXPOS_PIN15            (ADC_INPUTCTRL_MUXPOS_PIN15_Val << ADC_INPUTCTRL_MUXPOS_Pos) /**< (ADC_INPUTCTRL) ADC AIN15 Pin Position  */
#define ADC_INPUTCTRL_MUXPOS_PIN16            (ADC_INPUTCTRL_MUXPOS_PIN16_Val << ADC_INPUTCTRL_MUXPOS_Pos) /**< (ADC_INPUTCTRL) ADC AIN16 Pin Position  */
#define ADC_INPUTCTRL_MUXPOS_PIN17            (ADC_INPUTCTRL_MUXPOS_PIN17_Val << ADC_INPUTCTRL_MUXPOS_Pos) /**< (ADC_INPUTCTRL) ADC AIN17 Pin Position  */
#define ADC_INPUTCTRL_MUXPOS_PIN18            (ADC_INPUTCTRL_MUXPOS_PIN18_Val << ADC_INPUTCTRL_MUXPOS_Pos) /**< (ADC_INPUTCTRL) ADC AIN18 Pin Position  */
#define ADC_INPUTCTRL_MUXPOS_PIN19            (ADC_INPUTCTRL_MUXPOS_PIN19_Val << ADC_INPUTCTRL_MUXPOS_Pos) /**< (ADC_INPUTCTRL) ADC AIN19 Pin Position  */
#define ADC_INPUTCTRL_MUXPOS_TEMP             (ADC_INPUTCTRL_MUXPOS_TEMP_Val << ADC_INPUTCTRL_MUXPOS_Pos) /**< (ADC_INPUTCTRL) Temperature Reference Position  */
#define ADC_INPUTCTRL_MUXPOS_BANDGAP          (ADC_INPUTCTRL_MUXPOS_BANDGAP_Val << ADC_INPUTCTRL_MUXPOS_Pos) /**< (ADC_INPUTCTRL) Bandgap Voltage Position  */
#define ADC_INPUTCTRL_MUXPOS_SCALEDCOREVCC    (ADC_INPUTCTRL_MUXPOS_SCALEDCOREVCC_Val << ADC_INPUTCTRL_MUXPOS_Pos) /**< (ADC_INPUTCTRL) 1/4  Scaled Core Supply Position  */
#define ADC_INPUTCTRL_MUXPOS_SCALEDIOVCC      (ADC_INPUTCTRL_MUXPOS_SCALEDIOVCC_Val << ADC_INPUTCTRL_MUXPOS_Pos) /**< (ADC_INPUTCTRL) 1/4  Scaled I/O Supply Position  */
#define ADC_INPUTCTRL_MUXPOS_DAC              (ADC_INPUTCTRL_MUXPOS_DAC_Val << ADC_INPUTCTRL_MUXPOS_Pos) /**< (ADC_INPUTCTRL) DAC Output Position  */
#define ADC_INPUTCTRL_MUXNEG_Pos              _U_(8)                                               /**< (ADC_INPUTCTRL) Negative Mux Input Selection Position */
#define ADC_INPUTCTRL_MUXNEG_Msk              (_U_(0x1F) << ADC_INPUTCTRL_MUXNEG_Pos)              /**< (ADC_INPUTCTRL) Negative Mux Input Selection Mask */
#define ADC_INPUTCTRL_MUXNEG(value)           (ADC_INPUTCTRL_MUXNEG_Msk & ((value) << ADC_INPUTCTRL_MUXNEG_Pos))
#define   ADC_INPUTCTRL_MUXNEG_PIN0_Val       _U_(0x0)                                             /**< (ADC_INPUTCTRL) ADC AIN0 Pin  */
#define   ADC_INPUTCTRL_MUXNEG_PIN1_Val       _U_(0x1)                                             /**< (ADC_INPUTCTRL) ADC AIN1 Pin  */
#define   ADC_INPUTCTRL_MUXNEG_PIN2_Val       _U_(0x2)                                             /**< (ADC_INPUTCTRL) ADC AIN2 Pin  */
#define   ADC_INPUTCTRL_MUXNEG_PIN3_Val       _U_(0x3)                                             /**< (ADC_INPUTCTRL) ADC AIN3 Pin  */
#define   ADC_INPUTCTRL_MUXNEG_PIN4_Val       _U_(0x4)                                             /**< (ADC_INPUTCTRL) ADC AIN4 Pin  */
#define   ADC_INPUTCTRL_MUXNEG_PIN5_Val       _U_(0x5)                                             /**< (ADC_INPUTCTRL) ADC AIN5 Pin  */
#define   ADC_INPUTCTRL_MUXNEG_PIN6_Val       _U_(0x6)                                             /**< (ADC_INPUTCTRL) ADC AIN6 Pin  */
#define   ADC_INPUTCTRL_MUXNEG_PIN7_Val       _U_(0x7)                                             /**< (ADC_INPUTCTRL) ADC AIN7 Pin  */
#define   ADC_INPUTCTRL_MUXNEG_GND_Val        _U_(0x18)                                            /**< (ADC_INPUTCTRL) Internal Ground  */
#define   ADC_INPUTCTRL_MUXNEG_IOGND_Val      _U_(0x19)                                            /**< (ADC_INPUTCTRL) I/O Ground  */
#define ADC_INPUTCTRL_MUXNEG_PIN0             (ADC_INPUTCTRL_MUXNEG_PIN0_Val << ADC_INPUTCTRL_MUXNEG_Pos) /**< (ADC_INPUTCTRL) ADC AIN0 Pin Position  */
#define ADC_INPUTCTRL_MUXNEG_PIN1             (ADC_INPUTCTRL_MUXNEG_PIN1_Val << ADC_INPUTCTRL_MUXNEG_Pos) /**< (ADC_INPUTCTRL) ADC AIN1 Pin Position  */
#define ADC_INPUTCTRL_MUXNEG_PIN2             (ADC_INPUTCTRL_MUXNEG_PIN2_Val << ADC_INPUTCTRL_MUXNEG_Pos) /**< (ADC_INPUTCTRL) ADC AIN2 Pin Position  */
#define ADC_INPUTCTRL_MUXNEG_PIN3             (ADC_INPUTCTRL_MUXNEG_PIN3_Val << ADC_INPUTCTRL_MUXNEG_Pos) /**< (ADC_INPUTCTRL) ADC AIN3 Pin Position  */
#define ADC_INPUTCTRL_MUXNEG_PIN4             (ADC_INPUTCTRL_MUXNEG_PIN4_Val << ADC_INPUTCTRL_MUXNEG_Pos) /**< (ADC_INPUTCTRL) ADC AIN4 Pin Position  */
#define ADC_INPUTCTRL_MUXNEG_PIN5             (ADC_INPUTCTRL_MUXNEG_PIN5_Val << ADC_INPUTCTRL_MUXNEG_Pos) /**< (ADC_INPUTCTRL) ADC AIN5 Pin Position  */
#define ADC_INPUTCTRL_MUXNEG_PIN6             (ADC_INPUTCTRL_MUXNEG_PIN6_Val << ADC_INPUTCTRL_MUXNEG_Pos) /**< (ADC_INPUTCTRL) ADC AIN6 Pin Position  */
#define ADC_INPUTCTRL_MUXNEG_PIN7             (ADC_INPUTCTRL_MUXNEG_PIN7_Val << ADC_INPUTCTRL_MUXNEG_Pos) /**< (ADC_INPUTCTRL) ADC AIN7 Pin Position  */
#define ADC_INPUTCTRL_MUXNEG_GND              (ADC_INPUTCTRL_MUXNEG_GND_Val << ADC_INPUTCTRL_MUXNEG_Pos) /**< (ADC_INPUTCTRL) Internal Ground Position  */
#define ADC_INPUTCTRL_MUXNEG_IOGND            (ADC_INPUTCTRL_MUXNEG_IOGND_Val << ADC_INPUTCTRL_MUXNEG_Pos) /**< (ADC_INPUTCTRL) I/O Ground Position  */
#define ADC_INPUTCTRL_INPUTSCAN_Pos           _U_(16)                                              /**< (ADC_INPUTCTRL) Number of Input Channels Included in Scan Position */
#define ADC_INPUTCTRL_INPUTSCAN_Msk           (_U_(0xF) << ADC_INPUTCTRL_INPUTSCAN_Pos)            /**< (ADC_INPUTCTRL) Number of Input Channels Included in Scan Mask */
#define ADC_INPUTCTRL_INPUTSCAN(value)        (ADC_INPUTCTRL_INPUTSCAN_Msk & ((value) << ADC_INPUTCTRL_INPUTSCAN_Pos))
#define ADC_INPUTCTRL_INPUTOFFSET_Pos         _U_(20)                                              /**< (ADC_INPUTCTRL) Positive Mux Setting Offset Position */
#define ADC_INPUTCTRL_INPUTOFFSET_Msk         (_U_(0xF) << ADC_INPUTCTRL_INPUTOFFSET_Pos)          /**< (ADC_INPUTCTRL) Positive Mux Setting Offset Mask */
#define ADC_INPUTCTRL_INPUTOFFSET(value)      (ADC_INPUTCTRL_INPUTOFFSET_Msk & ((value) << ADC_INPUTCTRL_INPUTOFFSET_Pos))
#define ADC_INPUTCTRL_GAIN_Pos                _U_(24)                                              /**< (ADC_INPUTCTRL) Gain Factor Selection Position */
#define ADC_INPUTCTRL_GAIN_Msk                (_U_(0xF) << ADC_INPUTCTRL_GAIN_Pos)                 /**< (ADC_INPUTCTRL) Gain Factor Selection Mask */
#define ADC_INPUTCTRL_GAIN(value)             (ADC_INPUTCTRL_GAIN_Msk & ((value) << ADC_INPUTCTRL_GAIN_Pos))
#define   ADC_INPUTCTRL_GAIN_1X_Val           _U_(0x0)                                             /**< (ADC_INPUTCTRL) 1x  */
#define   ADC_INPUTCTRL_GAIN_2X_Val           _U_(0x1)                                             /**< (ADC_INPUTCTRL) 2x  */
#define   ADC_INPUTCTRL_GAIN_4X_Val           _U_(0x2)                                             /**< (ADC_INPUTCTRL) 4x  */
#define   ADC_INPUTCTRL_GAIN_8X_Val           _U_(0x3)                                             /**< (ADC_INPUTCTRL) 8x  */
#define   ADC_INPUTCTRL_GAIN_16X_Val          _U_(0x4)                                             /**< (ADC_INPUTCTRL) 16x  */
#define   ADC_INPUTCTRL_GAIN_DIV2_Val         _U_(0xF)                                             /**< (ADC_INPUTCTRL) 1/2x  */
#define ADC_INPUTCTRL_GAIN_1X                 (ADC_INPUTCTRL_GAIN_1X_Val << ADC_INPUTCTRL_GAIN_Pos) /**< (ADC_INPUTCTRL) 1x Position  */
#define ADC_INPUTCTRL_GAIN_2X                 (ADC_INPUTCTRL_GAIN_2X_Val << ADC_INPUTCTRL_GAIN_Pos) /**< (ADC_INPUTCTRL) 2x Position  */
#define ADC_INPUTCTRL_GAIN_4X                 (ADC_INPUTCTRL_GAIN_4X_Val << ADC_INPUTCTRL_GAIN_Pos) /**< (ADC_INPUTCTRL) 4x Position  */
#define ADC_INPUTCTRL_GAIN_8X                 (ADC_INPUTCTRL_GAIN_8X_Val << ADC_INPUTCTRL_GAIN_Pos) /**< (ADC_INPUTCTRL) 8x Position  */
#define ADC_INPUTCTRL_GAIN_16X                (ADC_INPUTCTRL_GAIN_16X_Val << ADC_INPUTCTRL_GAIN_Pos) /**< (ADC_INPUTCTRL) 16x Position  */
#define ADC_INPUTCTRL_GAIN_DIV2               (ADC_INPUTCTRL_GAIN_DIV2_Val << ADC_INPUTCTRL_GAIN_Pos) /**< (ADC_INPUTCTRL) 1/2x Position  */
#define ADC_INPUTCTRL_Msk                     _U_(0x0FFF1F1F)                                      /**< (ADC_INPUTCTRL) Register Mask  */


/* -------- ADC_EVCTRL : (ADC Offset: 0x14) (R/W 8) Event Control -------- */
#define ADC_EVCTRL_RESETVALUE                 _U_(0x00)                                            /**<  (ADC_EVCTRL) Event Control  Reset Value */

#define ADC_EVCTRL_STARTEI_Pos                _U_(0)                                               /**< (ADC_EVCTRL) Start Conversion Event In Position */
#define ADC_EVCTRL_STARTEI_Msk                (_U_(0x1) << ADC_EVCTRL_STARTEI_Pos)                 /**< (ADC_EVCTRL) Start Conversion Event In Mask */
#define ADC_EVCTRL_STARTEI(value)             (ADC_EVCTRL_STARTEI_Msk & ((value) << ADC_EVCTRL_STARTEI_Pos))
#define ADC_EVCTRL_SYNCEI_Pos                 _U_(1)                                               /**< (ADC_EVCTRL) Synchronization Event In Position */
#define ADC_EVCTRL_SYNCEI_Msk                 (_U_(0x1) << ADC_EVCTRL_SYNCEI_Pos)                  /**< (ADC_EVCTRL) Synchronization Event In Mask */
#define ADC_EVCTRL_SYNCEI(value)              (ADC_EVCTRL_SYNCEI_Msk & ((value) << ADC_EVCTRL_SYNCEI_Pos))
#define ADC_EVCTRL_RESRDYEO_Pos               _U_(4)                                               /**< (ADC_EVCTRL) Result Ready Event Out Position */
#define ADC_EVCTRL_RESRDYEO_Msk               (_U_(0x1) << ADC_EVCTRL_RESRDYEO_Pos)                /**< (ADC_EVCTRL) Result Ready Event Out Mask */
#define ADC_EVCTRL_RESRDYEO(value)            (ADC_EVCTRL_RESRDYEO_Msk & ((value) << ADC_EVCTRL_RESRDYEO_Pos))
#define ADC_EVCTRL_WINMONEO_Pos               _U_(5)                                               /**< (ADC_EVCTRL) Window Monitor Event Out Position */
#define ADC_EVCTRL_WINMONEO_Msk               (_U_(0x1) << ADC_EVCTRL_WINMONEO_Pos)                /**< (ADC_EVCTRL) Window Monitor Event Out Mask */
#define ADC_EVCTRL_WINMONEO(value)            (ADC_EVCTRL_WINMONEO_Msk & ((value) << ADC_EVCTRL_WINMONEO_Pos))
#define ADC_EVCTRL_Msk                        _U_(0x33)                                            /**< (ADC_EVCTRL) Register Mask  */


/* -------- ADC_INTENCLR : (ADC Offset: 0x16) (R/W 8) Interrupt Enable Clear -------- */
#define ADC_INTENCLR_RESETVALUE               _U_(0x00)                                            /**<  (ADC_INTENCLR) Interrupt Enable Clear  Reset Value */

#define ADC_INTENCLR_RESRDY_Pos               _U_(0)                                               /**< (ADC_INTENCLR) Result Ready Interrupt Enable Position */
#define ADC_INTENCLR_RESRDY_Msk               (_U_(0x1) << ADC_INTENCLR_RESRDY_Pos)                /**< (ADC_INTENCLR) Result Ready Interrupt Enable Mask */
#define ADC_INTENCLR_RESRDY(value)            (ADC_INTENCLR_RESRDY_Msk & ((value) << ADC_INTENCLR_RESRDY_Pos))
#define ADC_INTENCLR_OVERRUN_Pos              _U_(1)                                               /**< (ADC_INTENCLR) Overrun Interrupt Enable Position */
#define ADC_INTENCLR_OVERRUN_Msk              (_U_(0x1) << ADC_INTENCLR_OVERRUN_Pos)               /**< (ADC_INTENCLR) Overrun Interrupt Enable Mask */
#define ADC_INTENCLR_OVERRUN(value)           (ADC_INTENCLR_OVERRUN_Msk & ((value) << ADC_INTENCLR_OVERRUN_Pos))
#define ADC_INTENCLR_WINMON_Pos               _U_(2)                                               /**< (ADC_INTENCLR) Window Monitor Interrupt Enable Position */
#define ADC_INTENCLR_WINMON_Msk               (_U_(0x1) << ADC_INTENCLR_WINMON_Pos)                /**< (ADC_INTENCLR) Window Monitor Interrupt Enable Mask */
#define ADC_INTENCLR_WINMON(value)            (ADC_INTENCLR_WINMON_Msk & ((value) << ADC_INTENCLR_WINMON_Pos))
#define ADC_INTENCLR_SYNCRDY_Pos              _U_(3)                                               /**< (ADC_INTENCLR) Synchronization Ready Interrupt Enable Position */
#define ADC_INTENCLR_SYNCRDY_Msk              (_U_(0x1) << ADC_INTENCLR_SYNCRDY_Pos)               /**< (ADC_INTENCLR) Synchronization Ready Interrupt Enable Mask */
#define ADC_INTENCLR_SYNCRDY(value)           (ADC_INTENCLR_SYNCRDY_Msk & ((value) << ADC_INTENCLR_SYNCRDY_Pos))
#define ADC_INTENCLR_Msk                      _U_(0x0F)                                            /**< (ADC_INTENCLR) Register Mask  */


/* -------- ADC_INTENSET : (ADC Offset: 0x17) (R/W 8) Interrupt Enable Set -------- */
#define ADC_INTENSET_RESETVALUE               _U_(0x00)                                            /**<  (ADC_INTENSET) Interrupt Enable Set  Reset Value */

#define ADC_INTENSET_RESRDY_Pos               _U_(0)                                               /**< (ADC_INTENSET) Result Ready Interrupt Enable Position */
#define ADC_INTENSET_RESRDY_Msk               (_U_(0x1) << ADC_INTENSET_RESRDY_Pos)                /**< (ADC_INTENSET) Result Ready Interrupt Enable Mask */
#define ADC_INTENSET_RESRDY(value)            (ADC_INTENSET_RESRDY_Msk & ((value) << ADC_INTENSET_RESRDY_Pos))
#define ADC_INTENSET_OVERRUN_Pos              _U_(1)                                               /**< (ADC_INTENSET) Overrun Interrupt Enable Position */
#define ADC_INTENSET_OVERRUN_Msk              (_U_(0x1) << ADC_INTENSET_OVERRUN_Pos)               /**< (ADC_INTENSET) Overrun Interrupt Enable Mask */
#define ADC_INTENSET_OVERRUN(value)           (ADC_INTENSET_OVERRUN_Msk & ((value) << ADC_INTENSET_OVERRUN_Pos))
#define ADC_INTENSET_WINMON_Pos               _U_(2)                                               /**< (ADC_INTENSET) Window Monitor Interrupt Enable Position */
#define ADC_INTENSET_WINMON_Msk               (_U_(0x1) << ADC_INTENSET_WINMON_Pos)                /**< (ADC_INTENSET) Window Monitor Interrupt Enable Mask */
#define ADC_INTENSET_WINMON(value)            (ADC_INTENSET_WINMON_Msk & ((value) << ADC_INTENSET_WINMON_Pos))
#define ADC_INTENSET_SYNCRDY_Pos              _U_(3)                                               /**< (ADC_INTENSET) Synchronization Ready Interrupt Enable Position */
#define ADC_INTENSET_SYNCRDY_Msk              (_U_(0x1) << ADC_INTENSET_SYNCRDY_Pos)               /**< (ADC_INTENSET) Synchronization Ready Interrupt Enable Mask */
#define ADC_INTENSET_SYNCRDY(value)           (ADC_INTENSET_SYNCRDY_Msk & ((value) << ADC_INTENSET_SYNCRDY_Pos))
#define ADC_INTENSET_Msk                      _U_(0x0F)                                            /**< (ADC_INTENSET) Register Mask  */


/* -------- ADC_INTFLAG : (ADC Offset: 0x18) (R/W 8) Interrupt Flag Status and Clear -------- */
#define ADC_INTFLAG_RESETVALUE                _U_(0x00)                                            /**<  (ADC_INTFLAG) Interrupt Flag Status and Clear  Reset Value */

#define ADC_INTFLAG_RESRDY_Pos                _U_(0)                                               /**< (ADC_INTFLAG) Result Ready Position */
#define ADC_INTFLAG_RESRDY_Msk                (_U_(0x1) << ADC_INTFLAG_RESRDY_Pos)                 /**< (ADC_INTFLAG) Result Ready Mask */
#define ADC_INTFLAG_RESRDY(value)             (ADC_INTFLAG_RESRDY_Msk & ((value) << ADC_INTFLAG_RESRDY_Pos))
#define ADC_INTFLAG_OVERRUN_Pos               _U_(1)                                               /**< (ADC_INTFLAG) Overrun Position */
#define ADC_INTFLAG_OVERRUN_Msk               (_U_(0x1) << ADC_INTFLAG_OVERRUN_Pos)                /**< (ADC_INTFLAG) Overrun Mask */
#define ADC_INTFLAG_OVERRUN(value)            (ADC_INTFLAG_OVERRUN_Msk & ((value) << ADC_INTFLAG_OVERRUN_Pos))
#define ADC_INTFLAG_WINMON_Pos                _U_(2)                                               /**< (ADC_INTFLAG) Window Monitor Position */
#define ADC_INTFLAG_WINMON_Msk                (_U_(0x1) << ADC_INTFLAG_WINMON_Pos)                 /**< (ADC_INTFLAG) Window Monitor Mask */
#define ADC_INTFLAG_WINMON(value)             (ADC_INTFLAG_WINMON_Msk & ((value) << ADC_INTFLAG_WINMON_Pos))
#define ADC_INTFLAG_SYNCRDY_Pos               _U_(3)                                               /**< (ADC_INTFLAG) Synchronization Ready Position */
#define ADC_INTFLAG_SYNCRDY_Msk               (_U_(0x1) << ADC_INTFLAG_SYNCRDY_Pos)                /**< (ADC_INTFLAG) Synchronization Ready Mask */
#define ADC_INTFLAG_SYNCRDY(value)            (ADC_INTFLAG_SYNCRDY_Msk & ((value) << ADC_INTFLAG_SYNCRDY_Pos))
#define ADC_INTFLAG_Msk                       _U_(0x0F)                                            /**< (ADC_INTFLAG) Register Mask  */


/* -------- ADC_STATUS : (ADC Offset: 0x19) ( R/ 8) Status -------- */
#define ADC_STATUS_RESETVALUE                 _U_(0x00)                                            /**<  (ADC_STATUS) Status  Reset Value */

#define ADC_STATUS_SYNCBUSY_Pos               _U_(7)                                               /**< (ADC_STATUS) Synchronization Busy Position */
#define ADC_STATUS_SYNCBUSY_Msk               (_U_(0x1) << ADC_STATUS_SYNCBUSY_Pos)                /**< (ADC_STATUS) Synchronization Busy Mask */
#define ADC_STATUS_SYNCBUSY(value)            (ADC_STATUS_SYNCBUSY_Msk & ((value) << ADC_STATUS_SYNCBUSY_Pos))
#define ADC_STATUS_Msk                        _U_(0x80)                                            /**< (ADC_STATUS) Register Mask  */


/* -------- ADC_RESULT : (ADC Offset: 0x1A) ( R/ 16) Result -------- */
#define ADC_RESULT_RESETVALUE                 _U_(0x00)                                            /**<  (ADC_RESULT) Result  Reset Value */

#define ADC_RESULT_RESULT_Pos                 _U_(0)                                               /**< (ADC_RESULT) Result Conversion Value Position */
#define ADC_RESULT_RESULT_Msk                 (_U_(0xFFFF) << ADC_RESULT_RESULT_Pos)               /**< (ADC_RESULT) Result Conversion Value Mask */
#define ADC_RESULT_RESULT(value)              (ADC_RESULT_RESULT_Msk & ((value) << ADC_RESULT_RESULT_Pos))
#define ADC_RESULT_Msk                        _U_(0xFFFF)                                          /**< (ADC_RESULT) Register Mask  */


/* -------- ADC_WINLT : (ADC Offset: 0x1C) (R/W 16) Window Monitor Lower Threshold -------- */
#define ADC_WINLT_RESETVALUE                  _U_(0x00)                                            /**<  (ADC_WINLT) Window Monitor Lower Threshold  Reset Value */

#define ADC_WINLT_WINLT_Pos                   _U_(0)                                               /**< (ADC_WINLT) Window Lower Threshold Position */
#define ADC_WINLT_WINLT_Msk                   (_U_(0xFFFF) << ADC_WINLT_WINLT_Pos)                 /**< (ADC_WINLT) Window Lower Threshold Mask */
#define ADC_WINLT_WINLT(value)                (ADC_WINLT_WINLT_Msk & ((value) << ADC_WINLT_WINLT_Pos))
#define ADC_WINLT_Msk                         _U_(0xFFFF)                                          /**< (ADC_WINLT) Register Mask  */


/* -------- ADC_WINUT : (ADC Offset: 0x20) (R/W 16) Window Monitor Upper Threshold -------- */
#define ADC_WINUT_RESETVALUE                  _U_(0x00)                                            /**<  (ADC_WINUT) Window Monitor Upper Threshold  Reset Value */

#define ADC_WINUT_WINUT_Pos                   _U_(0)                                               /**< (ADC_WINUT) Window Upper Threshold Position */
#define ADC_WINUT_WINUT_Msk                   (_U_(0xFFFF) << ADC_WINUT_WINUT_Pos)                 /**< (ADC_WINUT) Window Upper Threshold Mask */
#define ADC_WINUT_WINUT(value)                (ADC_WINUT_WINUT_Msk & ((value) << ADC_WINUT_WINUT_Pos))
#define ADC_WINUT_Msk                         _U_(0xFFFF)                                          /**< (ADC_WINUT) Register Mask  */


/* -------- ADC_GAINCORR : (ADC Offset: 0x24) (R/W 16) Gain Correction -------- */
#define ADC_GAINCORR_RESETVALUE               _U_(0x00)                                            /**<  (ADC_GAINCORR) Gain Correction  Reset Value */

#define ADC_GAINCORR_GAINCORR_Pos             _U_(0)                                               /**< (ADC_GAINCORR) Gain Correction Value Position */
#define ADC_GAINCORR_GAINCORR_Msk             (_U_(0xFFF) << ADC_GAINCORR_GAINCORR_Pos)            /**< (ADC_GAINCORR) Gain Correction Value Mask */
#define ADC_GAINCORR_GAINCORR(value)          (ADC_GAINCORR_GAINCORR_Msk & ((value) << ADC_GAINCORR_GAINCORR_Pos))
#define ADC_GAINCORR_Msk                      _U_(0x0FFF)                                          /**< (ADC_GAINCORR) Register Mask  */


/* -------- ADC_OFFSETCORR : (ADC Offset: 0x26) (R/W 16) Offset Correction -------- */
#define ADC_OFFSETCORR_RESETVALUE             _U_(0x00)                                            /**<  (ADC_OFFSETCORR) Offset Correction  Reset Value */

#define ADC_OFFSETCORR_OFFSETCORR_Pos         _U_(0)                                               /**< (ADC_OFFSETCORR) Offset Correction Value Position */
#define ADC_OFFSETCORR_OFFSETCORR_Msk         (_U_(0xFFF) << ADC_OFFSETCORR_OFFSETCORR_Pos)        /**< (ADC_OFFSETCORR) Offset Correction Value Mask */
#define ADC_OFFSETCORR_OFFSETCORR(value)      (ADC_OFFSETCORR_OFFSETCORR_Msk & ((value) << ADC_OFFSETCORR_OFFSETCORR_Pos))
#define ADC_OFFSETCORR_Msk                    _U_(0x0FFF)                                          /**< (ADC_OFFSETCORR) Register Mask  */


/* -------- ADC_CALIB : (ADC Offset: 0x28) (R/W 16) Calibration -------- */
#define ADC_CALIB_RESETVALUE                  _U_(0x00)                                            /**<  (ADC_CALIB) Calibration  Reset Value */

#define ADC_CALIB_LINEARITY_CAL_Pos           _U_(0)                                               /**< (ADC_CALIB) Linearity Calibration Value Position */
#define ADC_CALIB_LINEARITY_CAL_Msk           (_U_(0xFF) << ADC_CALIB_LINEARITY_CAL_Pos)           /**< (ADC_CALIB) Linearity Calibration Value Mask */
#define ADC_CALIB_LINEARITY_CAL(value)        (ADC_CALIB_LINEARITY_CAL_Msk & ((value) << ADC_CALIB_LINEARITY_CAL_Pos))
#define ADC_CALIB_BIAS_CAL_Pos                _U_(8)                                               /**< (ADC_CALIB) Bias Calibration Value Position */
#define ADC_CALIB_BIAS_CAL_Msk                (_U_(0x7) << ADC_CALIB_BIAS_CAL_Pos)                 /**< (ADC_CALIB) Bias Calibration Value Mask */
#define ADC_CALIB_BIAS_CAL(value)             (ADC_CALIB_BIAS_CAL_Msk & ((value) << ADC_CALIB_BIAS_CAL_Pos))
#define ADC_CALIB_Msk                         _U_(0x07FF)                                          /**< (ADC_CALIB) Register Mask  */


/* -------- ADC_DBGCTRL : (ADC Offset: 0x2A) (R/W 8) Debug Control -------- */
#define ADC_DBGCTRL_RESETVALUE                _U_(0x00)                                            /**<  (ADC_DBGCTRL) Debug Control  Reset Value */

#define ADC_DBGCTRL_DBGRUN_Pos                _U_(0)                                               /**< (ADC_DBGCTRL) Debug Run Position */
#define ADC_DBGCTRL_DBGRUN_Msk                (_U_(0x1) << ADC_DBGCTRL_DBGRUN_Pos)                 /**< (ADC_DBGCTRL) Debug Run Mask */
#define ADC_DBGCTRL_DBGRUN(value)             (ADC_DBGCTRL_DBGRUN_Msk & ((value) << ADC_DBGCTRL_DBGRUN_Pos))
#define ADC_DBGCTRL_Msk                       _U_(0x01)                                            /**< (ADC_DBGCTRL) Register Mask  */


/** \brief ADC register offsets definitions */
#define ADC_CTRLA_REG_OFST             (0x00)              /**< (ADC_CTRLA) Control A Offset */
#define ADC_REFCTRL_REG_OFST           (0x01)              /**< (ADC_REFCTRL) Reference Control Offset */
#define ADC_AVGCTRL_REG_OFST           (0x02)              /**< (ADC_AVGCTRL) Average Control Offset */
#define ADC_SAMPCTRL_REG_OFST          (0x03)              /**< (ADC_SAMPCTRL) Sampling Time Control Offset */
#define ADC_CTRLB_REG_OFST             (0x04)              /**< (ADC_CTRLB) Control B Offset */
#define ADC_WINCTRL_REG_OFST           (0x08)              /**< (ADC_WINCTRL) Window Monitor Control Offset */
#define ADC_SWTRIG_REG_OFST            (0x0C)              /**< (ADC_SWTRIG) Software Trigger Offset */
#define ADC_INPUTCTRL_REG_OFST         (0x10)              /**< (ADC_INPUTCTRL) Input Control Offset */
#define ADC_EVCTRL_REG_OFST            (0x14)              /**< (ADC_EVCTRL) Event Control Offset */
#define ADC_INTENCLR_REG_OFST          (0x16)              /**< (ADC_INTENCLR) Interrupt Enable Clear Offset */
#define ADC_INTENSET_REG_OFST          (0x17)              /**< (ADC_INTENSET) Interrupt Enable Set Offset */
#define ADC_INTFLAG_REG_OFST           (0x18)              /**< (ADC_INTFLAG) Interrupt Flag Status and Clear Offset */
#define ADC_STATUS_REG_OFST            (0x19)              /**< (ADC_STATUS) Status Offset */
#define ADC_RESULT_REG_OFST            (0x1A)              /**< (ADC_RESULT) Result Offset */
#define ADC_WINLT_REG_OFST             (0x1C)              /**< (ADC_WINLT) Window Monitor Lower Threshold Offset */
#define ADC_WINUT_REG_OFST             (0x20)              /**< (ADC_WINUT) Window Monitor Upper Threshold Offset */
#define ADC_GAINCORR_REG_OFST          (0x24)              /**< (ADC_GAINCORR) Gain Correction Offset */
#define ADC_OFFSETCORR_REG_OFST        (0x26)              /**< (ADC_OFFSETCORR) Offset Correction Offset */
#define ADC_CALIB_REG_OFST             (0x28)              /**< (ADC_CALIB) Calibration Offset */
#define ADC_DBGCTRL_REG_OFST           (0x2A)              /**< (ADC_DBGCTRL) Debug Control Offset */

#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
/** \brief ADC register API structure */
typedef struct
{  /* Analog Digital Converter */
  __IO  uint8_t                        ADC_CTRLA;          /**< Offset: 0x00 (R/W  8) Control A */
  __IO  uint8_t                        ADC_REFCTRL;        /**< Offset: 0x01 (R/W  8) Reference Control */
  __IO  uint8_t                        ADC_AVGCTRL;        /**< Offset: 0x02 (R/W  8) Average Control */
  __IO  uint8_t                        ADC_SAMPCTRL;       /**< Offset: 0x03 (R/W  8) Sampling Time Control */
  __IO  uint16_t                       ADC_CTRLB;          /**< Offset: 0x04 (R/W  16) Control B */
  __I   uint8_t                        Reserved1[0x02];
  __IO  uint8_t                        ADC_WINCTRL;        /**< Offset: 0x08 (R/W  8) Window Monitor Control */
  __I   uint8_t                        Reserved2[0x03];
  __IO  uint8_t                        ADC_SWTRIG;         /**< Offset: 0x0C (R/W  8) Software Trigger */
  __I   uint8_t                        Reserved3[0x03];
  __IO  uint32_t                       ADC_INPUTCTRL;      /**< Offset: 0x10 (R/W  32) Input Control */
  __IO  uint8_t                        ADC_EVCTRL;         /**< Offset: 0x14 (R/W  8) Event Control */
  __I   uint8_t                        Reserved4[0x01];
  __IO  uint8_t                        ADC_INTENCLR;       /**< Offset: 0x16 (R/W  8) Interrupt Enable Clear */
  __IO  uint8_t                        ADC_INTENSET;       /**< Offset: 0x17 (R/W  8) Interrupt Enable Set */
  __IO  uint8_t                        ADC_INTFLAG;        /**< Offset: 0x18 (R/W  8) Interrupt Flag Status and Clear */
  __I   uint8_t                        ADC_STATUS;         /**< Offset: 0x19 (R/   8) Status */
  __I   uint16_t                       ADC_RESULT;         /**< Offset: 0x1A (R/   16) Result */
  __IO  uint16_t                       ADC_WINLT;          /**< Offset: 0x1C (R/W  16) Window Monitor Lower Threshold */
  __I   uint8_t                        Reserved5[0x02];
  __IO  uint16_t                       ADC_WINUT;          /**< Offset: 0x20 (R/W  16) Window Monitor Upper Threshold */
  __I   uint8_t                        Reserved6[0x02];
  __IO  uint16_t                       ADC_GAINCORR;       /**< Offset: 0x24 (R/W  16) Gain Correction */
  __IO  uint16_t                       ADC_OFFSETCORR;     /**< Offset: 0x26 (R/W  16) Offset Correction */
  __IO  uint16_t                       ADC_CALIB;          /**< Offset: 0x28 (R/W  16) Calibration */
  __IO  uint8_t                        ADC_DBGCTRL;        /**< Offset: 0x2A (R/W  8) Debug Control */
} adc_registers_t;


#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
#endif /* _SAMD21_ADC_COMPONENT_H_ */
