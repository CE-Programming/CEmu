/**
 * \brief Component description for EIC
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
#ifndef _SAMD21_EIC_COMPONENT_H_
#define _SAMD21_EIC_COMPONENT_H_

/* ************************************************************************** */
/*   SOFTWARE API DEFINITION FOR EIC                                          */
/* ************************************************************************** */

/* -------- EIC_CTRL : (EIC Offset: 0x00) (R/W 8) Control -------- */
#define EIC_CTRL_RESETVALUE                   _U_(0x00)                                            /**<  (EIC_CTRL) Control  Reset Value */

#define EIC_CTRL_SWRST_Pos                    _U_(0)                                               /**< (EIC_CTRL) Software Reset Position */
#define EIC_CTRL_SWRST_Msk                    (_U_(0x1) << EIC_CTRL_SWRST_Pos)                     /**< (EIC_CTRL) Software Reset Mask */
#define EIC_CTRL_SWRST(value)                 (EIC_CTRL_SWRST_Msk & ((value) << EIC_CTRL_SWRST_Pos))
#define EIC_CTRL_ENABLE_Pos                   _U_(1)                                               /**< (EIC_CTRL) Enable Position */
#define EIC_CTRL_ENABLE_Msk                   (_U_(0x1) << EIC_CTRL_ENABLE_Pos)                    /**< (EIC_CTRL) Enable Mask */
#define EIC_CTRL_ENABLE(value)                (EIC_CTRL_ENABLE_Msk & ((value) << EIC_CTRL_ENABLE_Pos))
#define EIC_CTRL_Msk                          _U_(0x03)                                            /**< (EIC_CTRL) Register Mask  */


/* -------- EIC_STATUS : (EIC Offset: 0x01) ( R/ 8) Status -------- */
#define EIC_STATUS_RESETVALUE                 _U_(0x00)                                            /**<  (EIC_STATUS) Status  Reset Value */

#define EIC_STATUS_SYNCBUSY_Pos               _U_(7)                                               /**< (EIC_STATUS) Synchronization Busy Position */
#define EIC_STATUS_SYNCBUSY_Msk               (_U_(0x1) << EIC_STATUS_SYNCBUSY_Pos)                /**< (EIC_STATUS) Synchronization Busy Mask */
#define EIC_STATUS_SYNCBUSY(value)            (EIC_STATUS_SYNCBUSY_Msk & ((value) << EIC_STATUS_SYNCBUSY_Pos))
#define EIC_STATUS_Msk                        _U_(0x80)                                            /**< (EIC_STATUS) Register Mask  */


/* -------- EIC_NMICTRL : (EIC Offset: 0x02) (R/W 8) Non-Maskable Interrupt Control -------- */
#define EIC_NMICTRL_RESETVALUE                _U_(0x00)                                            /**<  (EIC_NMICTRL) Non-Maskable Interrupt Control  Reset Value */

#define EIC_NMICTRL_NMISENSE_Pos              _U_(0)                                               /**< (EIC_NMICTRL) Non-Maskable Interrupt Sense Position */
#define EIC_NMICTRL_NMISENSE_Msk              (_U_(0x7) << EIC_NMICTRL_NMISENSE_Pos)               /**< (EIC_NMICTRL) Non-Maskable Interrupt Sense Mask */
#define EIC_NMICTRL_NMISENSE(value)           (EIC_NMICTRL_NMISENSE_Msk & ((value) << EIC_NMICTRL_NMISENSE_Pos))
#define   EIC_NMICTRL_NMISENSE_NONE_Val       _U_(0x0)                                             /**< (EIC_NMICTRL) No detection  */
#define   EIC_NMICTRL_NMISENSE_RISE_Val       _U_(0x1)                                             /**< (EIC_NMICTRL) Rising-edge detection  */
#define   EIC_NMICTRL_NMISENSE_FALL_Val       _U_(0x2)                                             /**< (EIC_NMICTRL) Falling-edge detection  */
#define   EIC_NMICTRL_NMISENSE_BOTH_Val       _U_(0x3)                                             /**< (EIC_NMICTRL) Both-edges detection  */
#define   EIC_NMICTRL_NMISENSE_HIGH_Val       _U_(0x4)                                             /**< (EIC_NMICTRL) High-level detection  */
#define   EIC_NMICTRL_NMISENSE_LOW_Val        _U_(0x5)                                             /**< (EIC_NMICTRL) Low-level detection  */
#define EIC_NMICTRL_NMISENSE_NONE             (EIC_NMICTRL_NMISENSE_NONE_Val << EIC_NMICTRL_NMISENSE_Pos) /**< (EIC_NMICTRL) No detection Position  */
#define EIC_NMICTRL_NMISENSE_RISE             (EIC_NMICTRL_NMISENSE_RISE_Val << EIC_NMICTRL_NMISENSE_Pos) /**< (EIC_NMICTRL) Rising-edge detection Position  */
#define EIC_NMICTRL_NMISENSE_FALL             (EIC_NMICTRL_NMISENSE_FALL_Val << EIC_NMICTRL_NMISENSE_Pos) /**< (EIC_NMICTRL) Falling-edge detection Position  */
#define EIC_NMICTRL_NMISENSE_BOTH             (EIC_NMICTRL_NMISENSE_BOTH_Val << EIC_NMICTRL_NMISENSE_Pos) /**< (EIC_NMICTRL) Both-edges detection Position  */
#define EIC_NMICTRL_NMISENSE_HIGH             (EIC_NMICTRL_NMISENSE_HIGH_Val << EIC_NMICTRL_NMISENSE_Pos) /**< (EIC_NMICTRL) High-level detection Position  */
#define EIC_NMICTRL_NMISENSE_LOW              (EIC_NMICTRL_NMISENSE_LOW_Val << EIC_NMICTRL_NMISENSE_Pos) /**< (EIC_NMICTRL) Low-level detection Position  */
#define EIC_NMICTRL_NMIFILTEN_Pos             _U_(3)                                               /**< (EIC_NMICTRL) Non-Maskable Interrupt Filter Enable Position */
#define EIC_NMICTRL_NMIFILTEN_Msk             (_U_(0x1) << EIC_NMICTRL_NMIFILTEN_Pos)              /**< (EIC_NMICTRL) Non-Maskable Interrupt Filter Enable Mask */
#define EIC_NMICTRL_NMIFILTEN(value)          (EIC_NMICTRL_NMIFILTEN_Msk & ((value) << EIC_NMICTRL_NMIFILTEN_Pos))
#define EIC_NMICTRL_Msk                       _U_(0x0F)                                            /**< (EIC_NMICTRL) Register Mask  */


/* -------- EIC_NMIFLAG : (EIC Offset: 0x03) (R/W 8) Non-Maskable Interrupt Flag Status and Clear -------- */
#define EIC_NMIFLAG_RESETVALUE                _U_(0x00)                                            /**<  (EIC_NMIFLAG) Non-Maskable Interrupt Flag Status and Clear  Reset Value */

#define EIC_NMIFLAG_NMI_Pos                   _U_(0)                                               /**< (EIC_NMIFLAG) Non-Maskable Interrupt Position */
#define EIC_NMIFLAG_NMI_Msk                   (_U_(0x1) << EIC_NMIFLAG_NMI_Pos)                    /**< (EIC_NMIFLAG) Non-Maskable Interrupt Mask */
#define EIC_NMIFLAG_NMI(value)                (EIC_NMIFLAG_NMI_Msk & ((value) << EIC_NMIFLAG_NMI_Pos))
#define EIC_NMIFLAG_Msk                       _U_(0x01)                                            /**< (EIC_NMIFLAG) Register Mask  */


/* -------- EIC_EVCTRL : (EIC Offset: 0x04) (R/W 32) Event Control -------- */
#define EIC_EVCTRL_RESETVALUE                 _U_(0x00)                                            /**<  (EIC_EVCTRL) Event Control  Reset Value */

#define EIC_EVCTRL_EXTINTEO0_Pos              _U_(0)                                               /**< (EIC_EVCTRL) External Interrupt 0 Event Output Enable Position */
#define EIC_EVCTRL_EXTINTEO0_Msk              (_U_(0x1) << EIC_EVCTRL_EXTINTEO0_Pos)               /**< (EIC_EVCTRL) External Interrupt 0 Event Output Enable Mask */
#define EIC_EVCTRL_EXTINTEO0(value)           (EIC_EVCTRL_EXTINTEO0_Msk & ((value) << EIC_EVCTRL_EXTINTEO0_Pos))
#define EIC_EVCTRL_EXTINTEO1_Pos              _U_(1)                                               /**< (EIC_EVCTRL) External Interrupt 1 Event Output Enable Position */
#define EIC_EVCTRL_EXTINTEO1_Msk              (_U_(0x1) << EIC_EVCTRL_EXTINTEO1_Pos)               /**< (EIC_EVCTRL) External Interrupt 1 Event Output Enable Mask */
#define EIC_EVCTRL_EXTINTEO1(value)           (EIC_EVCTRL_EXTINTEO1_Msk & ((value) << EIC_EVCTRL_EXTINTEO1_Pos))
#define EIC_EVCTRL_EXTINTEO2_Pos              _U_(2)                                               /**< (EIC_EVCTRL) External Interrupt 2 Event Output Enable Position */
#define EIC_EVCTRL_EXTINTEO2_Msk              (_U_(0x1) << EIC_EVCTRL_EXTINTEO2_Pos)               /**< (EIC_EVCTRL) External Interrupt 2 Event Output Enable Mask */
#define EIC_EVCTRL_EXTINTEO2(value)           (EIC_EVCTRL_EXTINTEO2_Msk & ((value) << EIC_EVCTRL_EXTINTEO2_Pos))
#define EIC_EVCTRL_EXTINTEO3_Pos              _U_(3)                                               /**< (EIC_EVCTRL) External Interrupt 3 Event Output Enable Position */
#define EIC_EVCTRL_EXTINTEO3_Msk              (_U_(0x1) << EIC_EVCTRL_EXTINTEO3_Pos)               /**< (EIC_EVCTRL) External Interrupt 3 Event Output Enable Mask */
#define EIC_EVCTRL_EXTINTEO3(value)           (EIC_EVCTRL_EXTINTEO3_Msk & ((value) << EIC_EVCTRL_EXTINTEO3_Pos))
#define EIC_EVCTRL_EXTINTEO4_Pos              _U_(4)                                               /**< (EIC_EVCTRL) External Interrupt 4 Event Output Enable Position */
#define EIC_EVCTRL_EXTINTEO4_Msk              (_U_(0x1) << EIC_EVCTRL_EXTINTEO4_Pos)               /**< (EIC_EVCTRL) External Interrupt 4 Event Output Enable Mask */
#define EIC_EVCTRL_EXTINTEO4(value)           (EIC_EVCTRL_EXTINTEO4_Msk & ((value) << EIC_EVCTRL_EXTINTEO4_Pos))
#define EIC_EVCTRL_EXTINTEO5_Pos              _U_(5)                                               /**< (EIC_EVCTRL) External Interrupt 5 Event Output Enable Position */
#define EIC_EVCTRL_EXTINTEO5_Msk              (_U_(0x1) << EIC_EVCTRL_EXTINTEO5_Pos)               /**< (EIC_EVCTRL) External Interrupt 5 Event Output Enable Mask */
#define EIC_EVCTRL_EXTINTEO5(value)           (EIC_EVCTRL_EXTINTEO5_Msk & ((value) << EIC_EVCTRL_EXTINTEO5_Pos))
#define EIC_EVCTRL_EXTINTEO6_Pos              _U_(6)                                               /**< (EIC_EVCTRL) External Interrupt 6 Event Output Enable Position */
#define EIC_EVCTRL_EXTINTEO6_Msk              (_U_(0x1) << EIC_EVCTRL_EXTINTEO6_Pos)               /**< (EIC_EVCTRL) External Interrupt 6 Event Output Enable Mask */
#define EIC_EVCTRL_EXTINTEO6(value)           (EIC_EVCTRL_EXTINTEO6_Msk & ((value) << EIC_EVCTRL_EXTINTEO6_Pos))
#define EIC_EVCTRL_EXTINTEO7_Pos              _U_(7)                                               /**< (EIC_EVCTRL) External Interrupt 7 Event Output Enable Position */
#define EIC_EVCTRL_EXTINTEO7_Msk              (_U_(0x1) << EIC_EVCTRL_EXTINTEO7_Pos)               /**< (EIC_EVCTRL) External Interrupt 7 Event Output Enable Mask */
#define EIC_EVCTRL_EXTINTEO7(value)           (EIC_EVCTRL_EXTINTEO7_Msk & ((value) << EIC_EVCTRL_EXTINTEO7_Pos))
#define EIC_EVCTRL_EXTINTEO8_Pos              _U_(8)                                               /**< (EIC_EVCTRL) External Interrupt 8 Event Output Enable Position */
#define EIC_EVCTRL_EXTINTEO8_Msk              (_U_(0x1) << EIC_EVCTRL_EXTINTEO8_Pos)               /**< (EIC_EVCTRL) External Interrupt 8 Event Output Enable Mask */
#define EIC_EVCTRL_EXTINTEO8(value)           (EIC_EVCTRL_EXTINTEO8_Msk & ((value) << EIC_EVCTRL_EXTINTEO8_Pos))
#define EIC_EVCTRL_EXTINTEO9_Pos              _U_(9)                                               /**< (EIC_EVCTRL) External Interrupt 9 Event Output Enable Position */
#define EIC_EVCTRL_EXTINTEO9_Msk              (_U_(0x1) << EIC_EVCTRL_EXTINTEO9_Pos)               /**< (EIC_EVCTRL) External Interrupt 9 Event Output Enable Mask */
#define EIC_EVCTRL_EXTINTEO9(value)           (EIC_EVCTRL_EXTINTEO9_Msk & ((value) << EIC_EVCTRL_EXTINTEO9_Pos))
#define EIC_EVCTRL_EXTINTEO10_Pos             _U_(10)                                              /**< (EIC_EVCTRL) External Interrupt 10 Event Output Enable Position */
#define EIC_EVCTRL_EXTINTEO10_Msk             (_U_(0x1) << EIC_EVCTRL_EXTINTEO10_Pos)              /**< (EIC_EVCTRL) External Interrupt 10 Event Output Enable Mask */
#define EIC_EVCTRL_EXTINTEO10(value)          (EIC_EVCTRL_EXTINTEO10_Msk & ((value) << EIC_EVCTRL_EXTINTEO10_Pos))
#define EIC_EVCTRL_EXTINTEO11_Pos             _U_(11)                                              /**< (EIC_EVCTRL) External Interrupt 11 Event Output Enable Position */
#define EIC_EVCTRL_EXTINTEO11_Msk             (_U_(0x1) << EIC_EVCTRL_EXTINTEO11_Pos)              /**< (EIC_EVCTRL) External Interrupt 11 Event Output Enable Mask */
#define EIC_EVCTRL_EXTINTEO11(value)          (EIC_EVCTRL_EXTINTEO11_Msk & ((value) << EIC_EVCTRL_EXTINTEO11_Pos))
#define EIC_EVCTRL_EXTINTEO12_Pos             _U_(12)                                              /**< (EIC_EVCTRL) External Interrupt 12 Event Output Enable Position */
#define EIC_EVCTRL_EXTINTEO12_Msk             (_U_(0x1) << EIC_EVCTRL_EXTINTEO12_Pos)              /**< (EIC_EVCTRL) External Interrupt 12 Event Output Enable Mask */
#define EIC_EVCTRL_EXTINTEO12(value)          (EIC_EVCTRL_EXTINTEO12_Msk & ((value) << EIC_EVCTRL_EXTINTEO12_Pos))
#define EIC_EVCTRL_EXTINTEO13_Pos             _U_(13)                                              /**< (EIC_EVCTRL) External Interrupt 13 Event Output Enable Position */
#define EIC_EVCTRL_EXTINTEO13_Msk             (_U_(0x1) << EIC_EVCTRL_EXTINTEO13_Pos)              /**< (EIC_EVCTRL) External Interrupt 13 Event Output Enable Mask */
#define EIC_EVCTRL_EXTINTEO13(value)          (EIC_EVCTRL_EXTINTEO13_Msk & ((value) << EIC_EVCTRL_EXTINTEO13_Pos))
#define EIC_EVCTRL_EXTINTEO14_Pos             _U_(14)                                              /**< (EIC_EVCTRL) External Interrupt 14 Event Output Enable Position */
#define EIC_EVCTRL_EXTINTEO14_Msk             (_U_(0x1) << EIC_EVCTRL_EXTINTEO14_Pos)              /**< (EIC_EVCTRL) External Interrupt 14 Event Output Enable Mask */
#define EIC_EVCTRL_EXTINTEO14(value)          (EIC_EVCTRL_EXTINTEO14_Msk & ((value) << EIC_EVCTRL_EXTINTEO14_Pos))
#define EIC_EVCTRL_EXTINTEO15_Pos             _U_(15)                                              /**< (EIC_EVCTRL) External Interrupt 15 Event Output Enable Position */
#define EIC_EVCTRL_EXTINTEO15_Msk             (_U_(0x1) << EIC_EVCTRL_EXTINTEO15_Pos)              /**< (EIC_EVCTRL) External Interrupt 15 Event Output Enable Mask */
#define EIC_EVCTRL_EXTINTEO15(value)          (EIC_EVCTRL_EXTINTEO15_Msk & ((value) << EIC_EVCTRL_EXTINTEO15_Pos))
#define EIC_EVCTRL_Msk                        _U_(0x0000FFFF)                                      /**< (EIC_EVCTRL) Register Mask  */

#define EIC_EVCTRL_EXTINTEO_Pos               _U_(0)                                               /**< (EIC_EVCTRL Position) External Interrupt x5 Event Output Enable */
#define EIC_EVCTRL_EXTINTEO_Msk               (_U_(0xFFFF) << EIC_EVCTRL_EXTINTEO_Pos)             /**< (EIC_EVCTRL Mask) EXTINTEO */
#define EIC_EVCTRL_EXTINTEO(value)            (EIC_EVCTRL_EXTINTEO_Msk & ((value) << EIC_EVCTRL_EXTINTEO_Pos)) 

/* -------- EIC_INTENCLR : (EIC Offset: 0x08) (R/W 32) Interrupt Enable Clear -------- */
#define EIC_INTENCLR_RESETVALUE               _U_(0x00)                                            /**<  (EIC_INTENCLR) Interrupt Enable Clear  Reset Value */

#define EIC_INTENCLR_EXTINT0_Pos              _U_(0)                                               /**< (EIC_INTENCLR) External Interrupt 0 Enable Position */
#define EIC_INTENCLR_EXTINT0_Msk              (_U_(0x1) << EIC_INTENCLR_EXTINT0_Pos)               /**< (EIC_INTENCLR) External Interrupt 0 Enable Mask */
#define EIC_INTENCLR_EXTINT0(value)           (EIC_INTENCLR_EXTINT0_Msk & ((value) << EIC_INTENCLR_EXTINT0_Pos))
#define EIC_INTENCLR_EXTINT1_Pos              _U_(1)                                               /**< (EIC_INTENCLR) External Interrupt 1 Enable Position */
#define EIC_INTENCLR_EXTINT1_Msk              (_U_(0x1) << EIC_INTENCLR_EXTINT1_Pos)               /**< (EIC_INTENCLR) External Interrupt 1 Enable Mask */
#define EIC_INTENCLR_EXTINT1(value)           (EIC_INTENCLR_EXTINT1_Msk & ((value) << EIC_INTENCLR_EXTINT1_Pos))
#define EIC_INTENCLR_EXTINT2_Pos              _U_(2)                                               /**< (EIC_INTENCLR) External Interrupt 2 Enable Position */
#define EIC_INTENCLR_EXTINT2_Msk              (_U_(0x1) << EIC_INTENCLR_EXTINT2_Pos)               /**< (EIC_INTENCLR) External Interrupt 2 Enable Mask */
#define EIC_INTENCLR_EXTINT2(value)           (EIC_INTENCLR_EXTINT2_Msk & ((value) << EIC_INTENCLR_EXTINT2_Pos))
#define EIC_INTENCLR_EXTINT3_Pos              _U_(3)                                               /**< (EIC_INTENCLR) External Interrupt 3 Enable Position */
#define EIC_INTENCLR_EXTINT3_Msk              (_U_(0x1) << EIC_INTENCLR_EXTINT3_Pos)               /**< (EIC_INTENCLR) External Interrupt 3 Enable Mask */
#define EIC_INTENCLR_EXTINT3(value)           (EIC_INTENCLR_EXTINT3_Msk & ((value) << EIC_INTENCLR_EXTINT3_Pos))
#define EIC_INTENCLR_EXTINT4_Pos              _U_(4)                                               /**< (EIC_INTENCLR) External Interrupt 4 Enable Position */
#define EIC_INTENCLR_EXTINT4_Msk              (_U_(0x1) << EIC_INTENCLR_EXTINT4_Pos)               /**< (EIC_INTENCLR) External Interrupt 4 Enable Mask */
#define EIC_INTENCLR_EXTINT4(value)           (EIC_INTENCLR_EXTINT4_Msk & ((value) << EIC_INTENCLR_EXTINT4_Pos))
#define EIC_INTENCLR_EXTINT5_Pos              _U_(5)                                               /**< (EIC_INTENCLR) External Interrupt 5 Enable Position */
#define EIC_INTENCLR_EXTINT5_Msk              (_U_(0x1) << EIC_INTENCLR_EXTINT5_Pos)               /**< (EIC_INTENCLR) External Interrupt 5 Enable Mask */
#define EIC_INTENCLR_EXTINT5(value)           (EIC_INTENCLR_EXTINT5_Msk & ((value) << EIC_INTENCLR_EXTINT5_Pos))
#define EIC_INTENCLR_EXTINT6_Pos              _U_(6)                                               /**< (EIC_INTENCLR) External Interrupt 6 Enable Position */
#define EIC_INTENCLR_EXTINT6_Msk              (_U_(0x1) << EIC_INTENCLR_EXTINT6_Pos)               /**< (EIC_INTENCLR) External Interrupt 6 Enable Mask */
#define EIC_INTENCLR_EXTINT6(value)           (EIC_INTENCLR_EXTINT6_Msk & ((value) << EIC_INTENCLR_EXTINT6_Pos))
#define EIC_INTENCLR_EXTINT7_Pos              _U_(7)                                               /**< (EIC_INTENCLR) External Interrupt 7 Enable Position */
#define EIC_INTENCLR_EXTINT7_Msk              (_U_(0x1) << EIC_INTENCLR_EXTINT7_Pos)               /**< (EIC_INTENCLR) External Interrupt 7 Enable Mask */
#define EIC_INTENCLR_EXTINT7(value)           (EIC_INTENCLR_EXTINT7_Msk & ((value) << EIC_INTENCLR_EXTINT7_Pos))
#define EIC_INTENCLR_EXTINT8_Pos              _U_(8)                                               /**< (EIC_INTENCLR) External Interrupt 8 Enable Position */
#define EIC_INTENCLR_EXTINT8_Msk              (_U_(0x1) << EIC_INTENCLR_EXTINT8_Pos)               /**< (EIC_INTENCLR) External Interrupt 8 Enable Mask */
#define EIC_INTENCLR_EXTINT8(value)           (EIC_INTENCLR_EXTINT8_Msk & ((value) << EIC_INTENCLR_EXTINT8_Pos))
#define EIC_INTENCLR_EXTINT9_Pos              _U_(9)                                               /**< (EIC_INTENCLR) External Interrupt 9 Enable Position */
#define EIC_INTENCLR_EXTINT9_Msk              (_U_(0x1) << EIC_INTENCLR_EXTINT9_Pos)               /**< (EIC_INTENCLR) External Interrupt 9 Enable Mask */
#define EIC_INTENCLR_EXTINT9(value)           (EIC_INTENCLR_EXTINT9_Msk & ((value) << EIC_INTENCLR_EXTINT9_Pos))
#define EIC_INTENCLR_EXTINT10_Pos             _U_(10)                                              /**< (EIC_INTENCLR) External Interrupt 10 Enable Position */
#define EIC_INTENCLR_EXTINT10_Msk             (_U_(0x1) << EIC_INTENCLR_EXTINT10_Pos)              /**< (EIC_INTENCLR) External Interrupt 10 Enable Mask */
#define EIC_INTENCLR_EXTINT10(value)          (EIC_INTENCLR_EXTINT10_Msk & ((value) << EIC_INTENCLR_EXTINT10_Pos))
#define EIC_INTENCLR_EXTINT11_Pos             _U_(11)                                              /**< (EIC_INTENCLR) External Interrupt 11 Enable Position */
#define EIC_INTENCLR_EXTINT11_Msk             (_U_(0x1) << EIC_INTENCLR_EXTINT11_Pos)              /**< (EIC_INTENCLR) External Interrupt 11 Enable Mask */
#define EIC_INTENCLR_EXTINT11(value)          (EIC_INTENCLR_EXTINT11_Msk & ((value) << EIC_INTENCLR_EXTINT11_Pos))
#define EIC_INTENCLR_EXTINT12_Pos             _U_(12)                                              /**< (EIC_INTENCLR) External Interrupt 12 Enable Position */
#define EIC_INTENCLR_EXTINT12_Msk             (_U_(0x1) << EIC_INTENCLR_EXTINT12_Pos)              /**< (EIC_INTENCLR) External Interrupt 12 Enable Mask */
#define EIC_INTENCLR_EXTINT12(value)          (EIC_INTENCLR_EXTINT12_Msk & ((value) << EIC_INTENCLR_EXTINT12_Pos))
#define EIC_INTENCLR_EXTINT13_Pos             _U_(13)                                              /**< (EIC_INTENCLR) External Interrupt 13 Enable Position */
#define EIC_INTENCLR_EXTINT13_Msk             (_U_(0x1) << EIC_INTENCLR_EXTINT13_Pos)              /**< (EIC_INTENCLR) External Interrupt 13 Enable Mask */
#define EIC_INTENCLR_EXTINT13(value)          (EIC_INTENCLR_EXTINT13_Msk & ((value) << EIC_INTENCLR_EXTINT13_Pos))
#define EIC_INTENCLR_EXTINT14_Pos             _U_(14)                                              /**< (EIC_INTENCLR) External Interrupt 14 Enable Position */
#define EIC_INTENCLR_EXTINT14_Msk             (_U_(0x1) << EIC_INTENCLR_EXTINT14_Pos)              /**< (EIC_INTENCLR) External Interrupt 14 Enable Mask */
#define EIC_INTENCLR_EXTINT14(value)          (EIC_INTENCLR_EXTINT14_Msk & ((value) << EIC_INTENCLR_EXTINT14_Pos))
#define EIC_INTENCLR_EXTINT15_Pos             _U_(15)                                              /**< (EIC_INTENCLR) External Interrupt 15 Enable Position */
#define EIC_INTENCLR_EXTINT15_Msk             (_U_(0x1) << EIC_INTENCLR_EXTINT15_Pos)              /**< (EIC_INTENCLR) External Interrupt 15 Enable Mask */
#define EIC_INTENCLR_EXTINT15(value)          (EIC_INTENCLR_EXTINT15_Msk & ((value) << EIC_INTENCLR_EXTINT15_Pos))
#define EIC_INTENCLR_Msk                      _U_(0x0000FFFF)                                      /**< (EIC_INTENCLR) Register Mask  */

#define EIC_INTENCLR_EXTINT_Pos               _U_(0)                                               /**< (EIC_INTENCLR Position) External Interrupt x5 Enable */
#define EIC_INTENCLR_EXTINT_Msk               (_U_(0xFFFF) << EIC_INTENCLR_EXTINT_Pos)             /**< (EIC_INTENCLR Mask) EXTINT */
#define EIC_INTENCLR_EXTINT(value)            (EIC_INTENCLR_EXTINT_Msk & ((value) << EIC_INTENCLR_EXTINT_Pos)) 

/* -------- EIC_INTENSET : (EIC Offset: 0x0C) (R/W 32) Interrupt Enable Set -------- */
#define EIC_INTENSET_RESETVALUE               _U_(0x00)                                            /**<  (EIC_INTENSET) Interrupt Enable Set  Reset Value */

#define EIC_INTENSET_EXTINT0_Pos              _U_(0)                                               /**< (EIC_INTENSET) External Interrupt 0 Enable Position */
#define EIC_INTENSET_EXTINT0_Msk              (_U_(0x1) << EIC_INTENSET_EXTINT0_Pos)               /**< (EIC_INTENSET) External Interrupt 0 Enable Mask */
#define EIC_INTENSET_EXTINT0(value)           (EIC_INTENSET_EXTINT0_Msk & ((value) << EIC_INTENSET_EXTINT0_Pos))
#define EIC_INTENSET_EXTINT1_Pos              _U_(1)                                               /**< (EIC_INTENSET) External Interrupt 1 Enable Position */
#define EIC_INTENSET_EXTINT1_Msk              (_U_(0x1) << EIC_INTENSET_EXTINT1_Pos)               /**< (EIC_INTENSET) External Interrupt 1 Enable Mask */
#define EIC_INTENSET_EXTINT1(value)           (EIC_INTENSET_EXTINT1_Msk & ((value) << EIC_INTENSET_EXTINT1_Pos))
#define EIC_INTENSET_EXTINT2_Pos              _U_(2)                                               /**< (EIC_INTENSET) External Interrupt 2 Enable Position */
#define EIC_INTENSET_EXTINT2_Msk              (_U_(0x1) << EIC_INTENSET_EXTINT2_Pos)               /**< (EIC_INTENSET) External Interrupt 2 Enable Mask */
#define EIC_INTENSET_EXTINT2(value)           (EIC_INTENSET_EXTINT2_Msk & ((value) << EIC_INTENSET_EXTINT2_Pos))
#define EIC_INTENSET_EXTINT3_Pos              _U_(3)                                               /**< (EIC_INTENSET) External Interrupt 3 Enable Position */
#define EIC_INTENSET_EXTINT3_Msk              (_U_(0x1) << EIC_INTENSET_EXTINT3_Pos)               /**< (EIC_INTENSET) External Interrupt 3 Enable Mask */
#define EIC_INTENSET_EXTINT3(value)           (EIC_INTENSET_EXTINT3_Msk & ((value) << EIC_INTENSET_EXTINT3_Pos))
#define EIC_INTENSET_EXTINT4_Pos              _U_(4)                                               /**< (EIC_INTENSET) External Interrupt 4 Enable Position */
#define EIC_INTENSET_EXTINT4_Msk              (_U_(0x1) << EIC_INTENSET_EXTINT4_Pos)               /**< (EIC_INTENSET) External Interrupt 4 Enable Mask */
#define EIC_INTENSET_EXTINT4(value)           (EIC_INTENSET_EXTINT4_Msk & ((value) << EIC_INTENSET_EXTINT4_Pos))
#define EIC_INTENSET_EXTINT5_Pos              _U_(5)                                               /**< (EIC_INTENSET) External Interrupt 5 Enable Position */
#define EIC_INTENSET_EXTINT5_Msk              (_U_(0x1) << EIC_INTENSET_EXTINT5_Pos)               /**< (EIC_INTENSET) External Interrupt 5 Enable Mask */
#define EIC_INTENSET_EXTINT5(value)           (EIC_INTENSET_EXTINT5_Msk & ((value) << EIC_INTENSET_EXTINT5_Pos))
#define EIC_INTENSET_EXTINT6_Pos              _U_(6)                                               /**< (EIC_INTENSET) External Interrupt 6 Enable Position */
#define EIC_INTENSET_EXTINT6_Msk              (_U_(0x1) << EIC_INTENSET_EXTINT6_Pos)               /**< (EIC_INTENSET) External Interrupt 6 Enable Mask */
#define EIC_INTENSET_EXTINT6(value)           (EIC_INTENSET_EXTINT6_Msk & ((value) << EIC_INTENSET_EXTINT6_Pos))
#define EIC_INTENSET_EXTINT7_Pos              _U_(7)                                               /**< (EIC_INTENSET) External Interrupt 7 Enable Position */
#define EIC_INTENSET_EXTINT7_Msk              (_U_(0x1) << EIC_INTENSET_EXTINT7_Pos)               /**< (EIC_INTENSET) External Interrupt 7 Enable Mask */
#define EIC_INTENSET_EXTINT7(value)           (EIC_INTENSET_EXTINT7_Msk & ((value) << EIC_INTENSET_EXTINT7_Pos))
#define EIC_INTENSET_EXTINT8_Pos              _U_(8)                                               /**< (EIC_INTENSET) External Interrupt 8 Enable Position */
#define EIC_INTENSET_EXTINT8_Msk              (_U_(0x1) << EIC_INTENSET_EXTINT8_Pos)               /**< (EIC_INTENSET) External Interrupt 8 Enable Mask */
#define EIC_INTENSET_EXTINT8(value)           (EIC_INTENSET_EXTINT8_Msk & ((value) << EIC_INTENSET_EXTINT8_Pos))
#define EIC_INTENSET_EXTINT9_Pos              _U_(9)                                               /**< (EIC_INTENSET) External Interrupt 9 Enable Position */
#define EIC_INTENSET_EXTINT9_Msk              (_U_(0x1) << EIC_INTENSET_EXTINT9_Pos)               /**< (EIC_INTENSET) External Interrupt 9 Enable Mask */
#define EIC_INTENSET_EXTINT9(value)           (EIC_INTENSET_EXTINT9_Msk & ((value) << EIC_INTENSET_EXTINT9_Pos))
#define EIC_INTENSET_EXTINT10_Pos             _U_(10)                                              /**< (EIC_INTENSET) External Interrupt 10 Enable Position */
#define EIC_INTENSET_EXTINT10_Msk             (_U_(0x1) << EIC_INTENSET_EXTINT10_Pos)              /**< (EIC_INTENSET) External Interrupt 10 Enable Mask */
#define EIC_INTENSET_EXTINT10(value)          (EIC_INTENSET_EXTINT10_Msk & ((value) << EIC_INTENSET_EXTINT10_Pos))
#define EIC_INTENSET_EXTINT11_Pos             _U_(11)                                              /**< (EIC_INTENSET) External Interrupt 11 Enable Position */
#define EIC_INTENSET_EXTINT11_Msk             (_U_(0x1) << EIC_INTENSET_EXTINT11_Pos)              /**< (EIC_INTENSET) External Interrupt 11 Enable Mask */
#define EIC_INTENSET_EXTINT11(value)          (EIC_INTENSET_EXTINT11_Msk & ((value) << EIC_INTENSET_EXTINT11_Pos))
#define EIC_INTENSET_EXTINT12_Pos             _U_(12)                                              /**< (EIC_INTENSET) External Interrupt 12 Enable Position */
#define EIC_INTENSET_EXTINT12_Msk             (_U_(0x1) << EIC_INTENSET_EXTINT12_Pos)              /**< (EIC_INTENSET) External Interrupt 12 Enable Mask */
#define EIC_INTENSET_EXTINT12(value)          (EIC_INTENSET_EXTINT12_Msk & ((value) << EIC_INTENSET_EXTINT12_Pos))
#define EIC_INTENSET_EXTINT13_Pos             _U_(13)                                              /**< (EIC_INTENSET) External Interrupt 13 Enable Position */
#define EIC_INTENSET_EXTINT13_Msk             (_U_(0x1) << EIC_INTENSET_EXTINT13_Pos)              /**< (EIC_INTENSET) External Interrupt 13 Enable Mask */
#define EIC_INTENSET_EXTINT13(value)          (EIC_INTENSET_EXTINT13_Msk & ((value) << EIC_INTENSET_EXTINT13_Pos))
#define EIC_INTENSET_EXTINT14_Pos             _U_(14)                                              /**< (EIC_INTENSET) External Interrupt 14 Enable Position */
#define EIC_INTENSET_EXTINT14_Msk             (_U_(0x1) << EIC_INTENSET_EXTINT14_Pos)              /**< (EIC_INTENSET) External Interrupt 14 Enable Mask */
#define EIC_INTENSET_EXTINT14(value)          (EIC_INTENSET_EXTINT14_Msk & ((value) << EIC_INTENSET_EXTINT14_Pos))
#define EIC_INTENSET_EXTINT15_Pos             _U_(15)                                              /**< (EIC_INTENSET) External Interrupt 15 Enable Position */
#define EIC_INTENSET_EXTINT15_Msk             (_U_(0x1) << EIC_INTENSET_EXTINT15_Pos)              /**< (EIC_INTENSET) External Interrupt 15 Enable Mask */
#define EIC_INTENSET_EXTINT15(value)          (EIC_INTENSET_EXTINT15_Msk & ((value) << EIC_INTENSET_EXTINT15_Pos))
#define EIC_INTENSET_Msk                      _U_(0x0000FFFF)                                      /**< (EIC_INTENSET) Register Mask  */

#define EIC_INTENSET_EXTINT_Pos               _U_(0)                                               /**< (EIC_INTENSET Position) External Interrupt x5 Enable */
#define EIC_INTENSET_EXTINT_Msk               (_U_(0xFFFF) << EIC_INTENSET_EXTINT_Pos)             /**< (EIC_INTENSET Mask) EXTINT */
#define EIC_INTENSET_EXTINT(value)            (EIC_INTENSET_EXTINT_Msk & ((value) << EIC_INTENSET_EXTINT_Pos)) 

/* -------- EIC_INTFLAG : (EIC Offset: 0x10) (R/W 32) Interrupt Flag Status and Clear -------- */
#define EIC_INTFLAG_RESETVALUE                _U_(0x00)                                            /**<  (EIC_INTFLAG) Interrupt Flag Status and Clear  Reset Value */

#define EIC_INTFLAG_EXTINT0_Pos               _U_(0)                                               /**< (EIC_INTFLAG) External Interrupt 0 Position */
#define EIC_INTFLAG_EXTINT0_Msk               (_U_(0x1) << EIC_INTFLAG_EXTINT0_Pos)                /**< (EIC_INTFLAG) External Interrupt 0 Mask */
#define EIC_INTFLAG_EXTINT0(value)            (EIC_INTFLAG_EXTINT0_Msk & ((value) << EIC_INTFLAG_EXTINT0_Pos))
#define EIC_INTFLAG_EXTINT1_Pos               _U_(1)                                               /**< (EIC_INTFLAG) External Interrupt 1 Position */
#define EIC_INTFLAG_EXTINT1_Msk               (_U_(0x1) << EIC_INTFLAG_EXTINT1_Pos)                /**< (EIC_INTFLAG) External Interrupt 1 Mask */
#define EIC_INTFLAG_EXTINT1(value)            (EIC_INTFLAG_EXTINT1_Msk & ((value) << EIC_INTFLAG_EXTINT1_Pos))
#define EIC_INTFLAG_EXTINT2_Pos               _U_(2)                                               /**< (EIC_INTFLAG) External Interrupt 2 Position */
#define EIC_INTFLAG_EXTINT2_Msk               (_U_(0x1) << EIC_INTFLAG_EXTINT2_Pos)                /**< (EIC_INTFLAG) External Interrupt 2 Mask */
#define EIC_INTFLAG_EXTINT2(value)            (EIC_INTFLAG_EXTINT2_Msk & ((value) << EIC_INTFLAG_EXTINT2_Pos))
#define EIC_INTFLAG_EXTINT3_Pos               _U_(3)                                               /**< (EIC_INTFLAG) External Interrupt 3 Position */
#define EIC_INTFLAG_EXTINT3_Msk               (_U_(0x1) << EIC_INTFLAG_EXTINT3_Pos)                /**< (EIC_INTFLAG) External Interrupt 3 Mask */
#define EIC_INTFLAG_EXTINT3(value)            (EIC_INTFLAG_EXTINT3_Msk & ((value) << EIC_INTFLAG_EXTINT3_Pos))
#define EIC_INTFLAG_EXTINT4_Pos               _U_(4)                                               /**< (EIC_INTFLAG) External Interrupt 4 Position */
#define EIC_INTFLAG_EXTINT4_Msk               (_U_(0x1) << EIC_INTFLAG_EXTINT4_Pos)                /**< (EIC_INTFLAG) External Interrupt 4 Mask */
#define EIC_INTFLAG_EXTINT4(value)            (EIC_INTFLAG_EXTINT4_Msk & ((value) << EIC_INTFLAG_EXTINT4_Pos))
#define EIC_INTFLAG_EXTINT5_Pos               _U_(5)                                               /**< (EIC_INTFLAG) External Interrupt 5 Position */
#define EIC_INTFLAG_EXTINT5_Msk               (_U_(0x1) << EIC_INTFLAG_EXTINT5_Pos)                /**< (EIC_INTFLAG) External Interrupt 5 Mask */
#define EIC_INTFLAG_EXTINT5(value)            (EIC_INTFLAG_EXTINT5_Msk & ((value) << EIC_INTFLAG_EXTINT5_Pos))
#define EIC_INTFLAG_EXTINT6_Pos               _U_(6)                                               /**< (EIC_INTFLAG) External Interrupt 6 Position */
#define EIC_INTFLAG_EXTINT6_Msk               (_U_(0x1) << EIC_INTFLAG_EXTINT6_Pos)                /**< (EIC_INTFLAG) External Interrupt 6 Mask */
#define EIC_INTFLAG_EXTINT6(value)            (EIC_INTFLAG_EXTINT6_Msk & ((value) << EIC_INTFLAG_EXTINT6_Pos))
#define EIC_INTFLAG_EXTINT7_Pos               _U_(7)                                               /**< (EIC_INTFLAG) External Interrupt 7 Position */
#define EIC_INTFLAG_EXTINT7_Msk               (_U_(0x1) << EIC_INTFLAG_EXTINT7_Pos)                /**< (EIC_INTFLAG) External Interrupt 7 Mask */
#define EIC_INTFLAG_EXTINT7(value)            (EIC_INTFLAG_EXTINT7_Msk & ((value) << EIC_INTFLAG_EXTINT7_Pos))
#define EIC_INTFLAG_EXTINT8_Pos               _U_(8)                                               /**< (EIC_INTFLAG) External Interrupt 8 Position */
#define EIC_INTFLAG_EXTINT8_Msk               (_U_(0x1) << EIC_INTFLAG_EXTINT8_Pos)                /**< (EIC_INTFLAG) External Interrupt 8 Mask */
#define EIC_INTFLAG_EXTINT8(value)            (EIC_INTFLAG_EXTINT8_Msk & ((value) << EIC_INTFLAG_EXTINT8_Pos))
#define EIC_INTFLAG_EXTINT9_Pos               _U_(9)                                               /**< (EIC_INTFLAG) External Interrupt 9 Position */
#define EIC_INTFLAG_EXTINT9_Msk               (_U_(0x1) << EIC_INTFLAG_EXTINT9_Pos)                /**< (EIC_INTFLAG) External Interrupt 9 Mask */
#define EIC_INTFLAG_EXTINT9(value)            (EIC_INTFLAG_EXTINT9_Msk & ((value) << EIC_INTFLAG_EXTINT9_Pos))
#define EIC_INTFLAG_EXTINT10_Pos              _U_(10)                                              /**< (EIC_INTFLAG) External Interrupt 10 Position */
#define EIC_INTFLAG_EXTINT10_Msk              (_U_(0x1) << EIC_INTFLAG_EXTINT10_Pos)               /**< (EIC_INTFLAG) External Interrupt 10 Mask */
#define EIC_INTFLAG_EXTINT10(value)           (EIC_INTFLAG_EXTINT10_Msk & ((value) << EIC_INTFLAG_EXTINT10_Pos))
#define EIC_INTFLAG_EXTINT11_Pos              _U_(11)                                              /**< (EIC_INTFLAG) External Interrupt 11 Position */
#define EIC_INTFLAG_EXTINT11_Msk              (_U_(0x1) << EIC_INTFLAG_EXTINT11_Pos)               /**< (EIC_INTFLAG) External Interrupt 11 Mask */
#define EIC_INTFLAG_EXTINT11(value)           (EIC_INTFLAG_EXTINT11_Msk & ((value) << EIC_INTFLAG_EXTINT11_Pos))
#define EIC_INTFLAG_EXTINT12_Pos              _U_(12)                                              /**< (EIC_INTFLAG) External Interrupt 12 Position */
#define EIC_INTFLAG_EXTINT12_Msk              (_U_(0x1) << EIC_INTFLAG_EXTINT12_Pos)               /**< (EIC_INTFLAG) External Interrupt 12 Mask */
#define EIC_INTFLAG_EXTINT12(value)           (EIC_INTFLAG_EXTINT12_Msk & ((value) << EIC_INTFLAG_EXTINT12_Pos))
#define EIC_INTFLAG_EXTINT13_Pos              _U_(13)                                              /**< (EIC_INTFLAG) External Interrupt 13 Position */
#define EIC_INTFLAG_EXTINT13_Msk              (_U_(0x1) << EIC_INTFLAG_EXTINT13_Pos)               /**< (EIC_INTFLAG) External Interrupt 13 Mask */
#define EIC_INTFLAG_EXTINT13(value)           (EIC_INTFLAG_EXTINT13_Msk & ((value) << EIC_INTFLAG_EXTINT13_Pos))
#define EIC_INTFLAG_EXTINT14_Pos              _U_(14)                                              /**< (EIC_INTFLAG) External Interrupt 14 Position */
#define EIC_INTFLAG_EXTINT14_Msk              (_U_(0x1) << EIC_INTFLAG_EXTINT14_Pos)               /**< (EIC_INTFLAG) External Interrupt 14 Mask */
#define EIC_INTFLAG_EXTINT14(value)           (EIC_INTFLAG_EXTINT14_Msk & ((value) << EIC_INTFLAG_EXTINT14_Pos))
#define EIC_INTFLAG_EXTINT15_Pos              _U_(15)                                              /**< (EIC_INTFLAG) External Interrupt 15 Position */
#define EIC_INTFLAG_EXTINT15_Msk              (_U_(0x1) << EIC_INTFLAG_EXTINT15_Pos)               /**< (EIC_INTFLAG) External Interrupt 15 Mask */
#define EIC_INTFLAG_EXTINT15(value)           (EIC_INTFLAG_EXTINT15_Msk & ((value) << EIC_INTFLAG_EXTINT15_Pos))
#define EIC_INTFLAG_Msk                       _U_(0x0000FFFF)                                      /**< (EIC_INTFLAG) Register Mask  */

#define EIC_INTFLAG_EXTINT_Pos                _U_(0)                                               /**< (EIC_INTFLAG Position) External Interrupt x5 */
#define EIC_INTFLAG_EXTINT_Msk                (_U_(0xFFFF) << EIC_INTFLAG_EXTINT_Pos)              /**< (EIC_INTFLAG Mask) EXTINT */
#define EIC_INTFLAG_EXTINT(value)             (EIC_INTFLAG_EXTINT_Msk & ((value) << EIC_INTFLAG_EXTINT_Pos)) 

/* -------- EIC_WAKEUP : (EIC Offset: 0x14) (R/W 32) Wake-Up Enable -------- */
#define EIC_WAKEUP_RESETVALUE                 _U_(0x00)                                            /**<  (EIC_WAKEUP) Wake-Up Enable  Reset Value */

#define EIC_WAKEUP_WAKEUPEN0_Pos              _U_(0)                                               /**< (EIC_WAKEUP) External Interrupt 0 Wake-up Enable Position */
#define EIC_WAKEUP_WAKEUPEN0_Msk              (_U_(0x1) << EIC_WAKEUP_WAKEUPEN0_Pos)               /**< (EIC_WAKEUP) External Interrupt 0 Wake-up Enable Mask */
#define EIC_WAKEUP_WAKEUPEN0(value)           (EIC_WAKEUP_WAKEUPEN0_Msk & ((value) << EIC_WAKEUP_WAKEUPEN0_Pos))
#define EIC_WAKEUP_WAKEUPEN1_Pos              _U_(1)                                               /**< (EIC_WAKEUP) External Interrupt 1 Wake-up Enable Position */
#define EIC_WAKEUP_WAKEUPEN1_Msk              (_U_(0x1) << EIC_WAKEUP_WAKEUPEN1_Pos)               /**< (EIC_WAKEUP) External Interrupt 1 Wake-up Enable Mask */
#define EIC_WAKEUP_WAKEUPEN1(value)           (EIC_WAKEUP_WAKEUPEN1_Msk & ((value) << EIC_WAKEUP_WAKEUPEN1_Pos))
#define EIC_WAKEUP_WAKEUPEN2_Pos              _U_(2)                                               /**< (EIC_WAKEUP) External Interrupt 2 Wake-up Enable Position */
#define EIC_WAKEUP_WAKEUPEN2_Msk              (_U_(0x1) << EIC_WAKEUP_WAKEUPEN2_Pos)               /**< (EIC_WAKEUP) External Interrupt 2 Wake-up Enable Mask */
#define EIC_WAKEUP_WAKEUPEN2(value)           (EIC_WAKEUP_WAKEUPEN2_Msk & ((value) << EIC_WAKEUP_WAKEUPEN2_Pos))
#define EIC_WAKEUP_WAKEUPEN3_Pos              _U_(3)                                               /**< (EIC_WAKEUP) External Interrupt 3 Wake-up Enable Position */
#define EIC_WAKEUP_WAKEUPEN3_Msk              (_U_(0x1) << EIC_WAKEUP_WAKEUPEN3_Pos)               /**< (EIC_WAKEUP) External Interrupt 3 Wake-up Enable Mask */
#define EIC_WAKEUP_WAKEUPEN3(value)           (EIC_WAKEUP_WAKEUPEN3_Msk & ((value) << EIC_WAKEUP_WAKEUPEN3_Pos))
#define EIC_WAKEUP_WAKEUPEN4_Pos              _U_(4)                                               /**< (EIC_WAKEUP) External Interrupt 4 Wake-up Enable Position */
#define EIC_WAKEUP_WAKEUPEN4_Msk              (_U_(0x1) << EIC_WAKEUP_WAKEUPEN4_Pos)               /**< (EIC_WAKEUP) External Interrupt 4 Wake-up Enable Mask */
#define EIC_WAKEUP_WAKEUPEN4(value)           (EIC_WAKEUP_WAKEUPEN4_Msk & ((value) << EIC_WAKEUP_WAKEUPEN4_Pos))
#define EIC_WAKEUP_WAKEUPEN5_Pos              _U_(5)                                               /**< (EIC_WAKEUP) External Interrupt 5 Wake-up Enable Position */
#define EIC_WAKEUP_WAKEUPEN5_Msk              (_U_(0x1) << EIC_WAKEUP_WAKEUPEN5_Pos)               /**< (EIC_WAKEUP) External Interrupt 5 Wake-up Enable Mask */
#define EIC_WAKEUP_WAKEUPEN5(value)           (EIC_WAKEUP_WAKEUPEN5_Msk & ((value) << EIC_WAKEUP_WAKEUPEN5_Pos))
#define EIC_WAKEUP_WAKEUPEN6_Pos              _U_(6)                                               /**< (EIC_WAKEUP) External Interrupt 6 Wake-up Enable Position */
#define EIC_WAKEUP_WAKEUPEN6_Msk              (_U_(0x1) << EIC_WAKEUP_WAKEUPEN6_Pos)               /**< (EIC_WAKEUP) External Interrupt 6 Wake-up Enable Mask */
#define EIC_WAKEUP_WAKEUPEN6(value)           (EIC_WAKEUP_WAKEUPEN6_Msk & ((value) << EIC_WAKEUP_WAKEUPEN6_Pos))
#define EIC_WAKEUP_WAKEUPEN7_Pos              _U_(7)                                               /**< (EIC_WAKEUP) External Interrupt 7 Wake-up Enable Position */
#define EIC_WAKEUP_WAKEUPEN7_Msk              (_U_(0x1) << EIC_WAKEUP_WAKEUPEN7_Pos)               /**< (EIC_WAKEUP) External Interrupt 7 Wake-up Enable Mask */
#define EIC_WAKEUP_WAKEUPEN7(value)           (EIC_WAKEUP_WAKEUPEN7_Msk & ((value) << EIC_WAKEUP_WAKEUPEN7_Pos))
#define EIC_WAKEUP_WAKEUPEN8_Pos              _U_(8)                                               /**< (EIC_WAKEUP) External Interrupt 8 Wake-up Enable Position */
#define EIC_WAKEUP_WAKEUPEN8_Msk              (_U_(0x1) << EIC_WAKEUP_WAKEUPEN8_Pos)               /**< (EIC_WAKEUP) External Interrupt 8 Wake-up Enable Mask */
#define EIC_WAKEUP_WAKEUPEN8(value)           (EIC_WAKEUP_WAKEUPEN8_Msk & ((value) << EIC_WAKEUP_WAKEUPEN8_Pos))
#define EIC_WAKEUP_WAKEUPEN9_Pos              _U_(9)                                               /**< (EIC_WAKEUP) External Interrupt 9 Wake-up Enable Position */
#define EIC_WAKEUP_WAKEUPEN9_Msk              (_U_(0x1) << EIC_WAKEUP_WAKEUPEN9_Pos)               /**< (EIC_WAKEUP) External Interrupt 9 Wake-up Enable Mask */
#define EIC_WAKEUP_WAKEUPEN9(value)           (EIC_WAKEUP_WAKEUPEN9_Msk & ((value) << EIC_WAKEUP_WAKEUPEN9_Pos))
#define EIC_WAKEUP_WAKEUPEN10_Pos             _U_(10)                                              /**< (EIC_WAKEUP) External Interrupt 10 Wake-up Enable Position */
#define EIC_WAKEUP_WAKEUPEN10_Msk             (_U_(0x1) << EIC_WAKEUP_WAKEUPEN10_Pos)              /**< (EIC_WAKEUP) External Interrupt 10 Wake-up Enable Mask */
#define EIC_WAKEUP_WAKEUPEN10(value)          (EIC_WAKEUP_WAKEUPEN10_Msk & ((value) << EIC_WAKEUP_WAKEUPEN10_Pos))
#define EIC_WAKEUP_WAKEUPEN11_Pos             _U_(11)                                              /**< (EIC_WAKEUP) External Interrupt 11 Wake-up Enable Position */
#define EIC_WAKEUP_WAKEUPEN11_Msk             (_U_(0x1) << EIC_WAKEUP_WAKEUPEN11_Pos)              /**< (EIC_WAKEUP) External Interrupt 11 Wake-up Enable Mask */
#define EIC_WAKEUP_WAKEUPEN11(value)          (EIC_WAKEUP_WAKEUPEN11_Msk & ((value) << EIC_WAKEUP_WAKEUPEN11_Pos))
#define EIC_WAKEUP_WAKEUPEN12_Pos             _U_(12)                                              /**< (EIC_WAKEUP) External Interrupt 12 Wake-up Enable Position */
#define EIC_WAKEUP_WAKEUPEN12_Msk             (_U_(0x1) << EIC_WAKEUP_WAKEUPEN12_Pos)              /**< (EIC_WAKEUP) External Interrupt 12 Wake-up Enable Mask */
#define EIC_WAKEUP_WAKEUPEN12(value)          (EIC_WAKEUP_WAKEUPEN12_Msk & ((value) << EIC_WAKEUP_WAKEUPEN12_Pos))
#define EIC_WAKEUP_WAKEUPEN13_Pos             _U_(13)                                              /**< (EIC_WAKEUP) External Interrupt 13 Wake-up Enable Position */
#define EIC_WAKEUP_WAKEUPEN13_Msk             (_U_(0x1) << EIC_WAKEUP_WAKEUPEN13_Pos)              /**< (EIC_WAKEUP) External Interrupt 13 Wake-up Enable Mask */
#define EIC_WAKEUP_WAKEUPEN13(value)          (EIC_WAKEUP_WAKEUPEN13_Msk & ((value) << EIC_WAKEUP_WAKEUPEN13_Pos))
#define EIC_WAKEUP_WAKEUPEN14_Pos             _U_(14)                                              /**< (EIC_WAKEUP) External Interrupt 14 Wake-up Enable Position */
#define EIC_WAKEUP_WAKEUPEN14_Msk             (_U_(0x1) << EIC_WAKEUP_WAKEUPEN14_Pos)              /**< (EIC_WAKEUP) External Interrupt 14 Wake-up Enable Mask */
#define EIC_WAKEUP_WAKEUPEN14(value)          (EIC_WAKEUP_WAKEUPEN14_Msk & ((value) << EIC_WAKEUP_WAKEUPEN14_Pos))
#define EIC_WAKEUP_WAKEUPEN15_Pos             _U_(15)                                              /**< (EIC_WAKEUP) External Interrupt 15 Wake-up Enable Position */
#define EIC_WAKEUP_WAKEUPEN15_Msk             (_U_(0x1) << EIC_WAKEUP_WAKEUPEN15_Pos)              /**< (EIC_WAKEUP) External Interrupt 15 Wake-up Enable Mask */
#define EIC_WAKEUP_WAKEUPEN15(value)          (EIC_WAKEUP_WAKEUPEN15_Msk & ((value) << EIC_WAKEUP_WAKEUPEN15_Pos))
#define EIC_WAKEUP_Msk                        _U_(0x0000FFFF)                                      /**< (EIC_WAKEUP) Register Mask  */

#define EIC_WAKEUP_WAKEUPEN_Pos               _U_(0)                                               /**< (EIC_WAKEUP Position) External Interrupt x5 Wake-up Enable */
#define EIC_WAKEUP_WAKEUPEN_Msk               (_U_(0xFFFF) << EIC_WAKEUP_WAKEUPEN_Pos)             /**< (EIC_WAKEUP Mask) WAKEUPEN */
#define EIC_WAKEUP_WAKEUPEN(value)            (EIC_WAKEUP_WAKEUPEN_Msk & ((value) << EIC_WAKEUP_WAKEUPEN_Pos)) 

/* -------- EIC_CONFIG : (EIC Offset: 0x18) (R/W 32) Configuration n -------- */
#define EIC_CONFIG_RESETVALUE                 _U_(0x00)                                            /**<  (EIC_CONFIG) Configuration n  Reset Value */

#define EIC_CONFIG_SENSE0_Pos                 _U_(0)                                               /**< (EIC_CONFIG) Input Sense 0 Configuration Position */
#define EIC_CONFIG_SENSE0_Msk                 (_U_(0x7) << EIC_CONFIG_SENSE0_Pos)                  /**< (EIC_CONFIG) Input Sense 0 Configuration Mask */
#define EIC_CONFIG_SENSE0(value)              (EIC_CONFIG_SENSE0_Msk & ((value) << EIC_CONFIG_SENSE0_Pos))
#define   EIC_CONFIG_SENSE0_NONE_Val          _U_(0x0)                                             /**< (EIC_CONFIG) No detection  */
#define   EIC_CONFIG_SENSE0_RISE_Val          _U_(0x1)                                             /**< (EIC_CONFIG) Rising-edge detection  */
#define   EIC_CONFIG_SENSE0_FALL_Val          _U_(0x2)                                             /**< (EIC_CONFIG) Falling-edge detection  */
#define   EIC_CONFIG_SENSE0_BOTH_Val          _U_(0x3)                                             /**< (EIC_CONFIG) Both-edges detection  */
#define   EIC_CONFIG_SENSE0_HIGH_Val          _U_(0x4)                                             /**< (EIC_CONFIG) High-level detection  */
#define   EIC_CONFIG_SENSE0_LOW_Val           _U_(0x5)                                             /**< (EIC_CONFIG) Low-level detection  */
#define EIC_CONFIG_SENSE0_NONE                (EIC_CONFIG_SENSE0_NONE_Val << EIC_CONFIG_SENSE0_Pos) /**< (EIC_CONFIG) No detection Position  */
#define EIC_CONFIG_SENSE0_RISE                (EIC_CONFIG_SENSE0_RISE_Val << EIC_CONFIG_SENSE0_Pos) /**< (EIC_CONFIG) Rising-edge detection Position  */
#define EIC_CONFIG_SENSE0_FALL                (EIC_CONFIG_SENSE0_FALL_Val << EIC_CONFIG_SENSE0_Pos) /**< (EIC_CONFIG) Falling-edge detection Position  */
#define EIC_CONFIG_SENSE0_BOTH                (EIC_CONFIG_SENSE0_BOTH_Val << EIC_CONFIG_SENSE0_Pos) /**< (EIC_CONFIG) Both-edges detection Position  */
#define EIC_CONFIG_SENSE0_HIGH                (EIC_CONFIG_SENSE0_HIGH_Val << EIC_CONFIG_SENSE0_Pos) /**< (EIC_CONFIG) High-level detection Position  */
#define EIC_CONFIG_SENSE0_LOW                 (EIC_CONFIG_SENSE0_LOW_Val << EIC_CONFIG_SENSE0_Pos) /**< (EIC_CONFIG) Low-level detection Position  */
#define EIC_CONFIG_FILTEN0_Pos                _U_(3)                                               /**< (EIC_CONFIG) Filter 0 Enable Position */
#define EIC_CONFIG_FILTEN0_Msk                (_U_(0x1) << EIC_CONFIG_FILTEN0_Pos)                 /**< (EIC_CONFIG) Filter 0 Enable Mask */
#define EIC_CONFIG_FILTEN0(value)             (EIC_CONFIG_FILTEN0_Msk & ((value) << EIC_CONFIG_FILTEN0_Pos))
#define EIC_CONFIG_SENSE1_Pos                 _U_(4)                                               /**< (EIC_CONFIG) Input Sense 1 Configuration Position */
#define EIC_CONFIG_SENSE1_Msk                 (_U_(0x7) << EIC_CONFIG_SENSE1_Pos)                  /**< (EIC_CONFIG) Input Sense 1 Configuration Mask */
#define EIC_CONFIG_SENSE1(value)              (EIC_CONFIG_SENSE1_Msk & ((value) << EIC_CONFIG_SENSE1_Pos))
#define   EIC_CONFIG_SENSE1_NONE_Val          _U_(0x0)                                             /**< (EIC_CONFIG) No detection  */
#define   EIC_CONFIG_SENSE1_RISE_Val          _U_(0x1)                                             /**< (EIC_CONFIG) Rising edge detection  */
#define   EIC_CONFIG_SENSE1_FALL_Val          _U_(0x2)                                             /**< (EIC_CONFIG) Falling edge detection  */
#define   EIC_CONFIG_SENSE1_BOTH_Val          _U_(0x3)                                             /**< (EIC_CONFIG) Both edges detection  */
#define   EIC_CONFIG_SENSE1_HIGH_Val          _U_(0x4)                                             /**< (EIC_CONFIG) High level detection  */
#define   EIC_CONFIG_SENSE1_LOW_Val           _U_(0x5)                                             /**< (EIC_CONFIG) Low level detection  */
#define EIC_CONFIG_SENSE1_NONE                (EIC_CONFIG_SENSE1_NONE_Val << EIC_CONFIG_SENSE1_Pos) /**< (EIC_CONFIG) No detection Position  */
#define EIC_CONFIG_SENSE1_RISE                (EIC_CONFIG_SENSE1_RISE_Val << EIC_CONFIG_SENSE1_Pos) /**< (EIC_CONFIG) Rising edge detection Position  */
#define EIC_CONFIG_SENSE1_FALL                (EIC_CONFIG_SENSE1_FALL_Val << EIC_CONFIG_SENSE1_Pos) /**< (EIC_CONFIG) Falling edge detection Position  */
#define EIC_CONFIG_SENSE1_BOTH                (EIC_CONFIG_SENSE1_BOTH_Val << EIC_CONFIG_SENSE1_Pos) /**< (EIC_CONFIG) Both edges detection Position  */
#define EIC_CONFIG_SENSE1_HIGH                (EIC_CONFIG_SENSE1_HIGH_Val << EIC_CONFIG_SENSE1_Pos) /**< (EIC_CONFIG) High level detection Position  */
#define EIC_CONFIG_SENSE1_LOW                 (EIC_CONFIG_SENSE1_LOW_Val << EIC_CONFIG_SENSE1_Pos) /**< (EIC_CONFIG) Low level detection Position  */
#define EIC_CONFIG_FILTEN1_Pos                _U_(7)                                               /**< (EIC_CONFIG) Filter 1 Enable Position */
#define EIC_CONFIG_FILTEN1_Msk                (_U_(0x1) << EIC_CONFIG_FILTEN1_Pos)                 /**< (EIC_CONFIG) Filter 1 Enable Mask */
#define EIC_CONFIG_FILTEN1(value)             (EIC_CONFIG_FILTEN1_Msk & ((value) << EIC_CONFIG_FILTEN1_Pos))
#define EIC_CONFIG_SENSE2_Pos                 _U_(8)                                               /**< (EIC_CONFIG) Input Sense 2 Configuration Position */
#define EIC_CONFIG_SENSE2_Msk                 (_U_(0x7) << EIC_CONFIG_SENSE2_Pos)                  /**< (EIC_CONFIG) Input Sense 2 Configuration Mask */
#define EIC_CONFIG_SENSE2(value)              (EIC_CONFIG_SENSE2_Msk & ((value) << EIC_CONFIG_SENSE2_Pos))
#define   EIC_CONFIG_SENSE2_NONE_Val          _U_(0x0)                                             /**< (EIC_CONFIG) No detection  */
#define   EIC_CONFIG_SENSE2_RISE_Val          _U_(0x1)                                             /**< (EIC_CONFIG) Rising edge detection  */
#define   EIC_CONFIG_SENSE2_FALL_Val          _U_(0x2)                                             /**< (EIC_CONFIG) Falling edge detection  */
#define   EIC_CONFIG_SENSE2_BOTH_Val          _U_(0x3)                                             /**< (EIC_CONFIG) Both edges detection  */
#define   EIC_CONFIG_SENSE2_HIGH_Val          _U_(0x4)                                             /**< (EIC_CONFIG) High level detection  */
#define   EIC_CONFIG_SENSE2_LOW_Val           _U_(0x5)                                             /**< (EIC_CONFIG) Low level detection  */
#define EIC_CONFIG_SENSE2_NONE                (EIC_CONFIG_SENSE2_NONE_Val << EIC_CONFIG_SENSE2_Pos) /**< (EIC_CONFIG) No detection Position  */
#define EIC_CONFIG_SENSE2_RISE                (EIC_CONFIG_SENSE2_RISE_Val << EIC_CONFIG_SENSE2_Pos) /**< (EIC_CONFIG) Rising edge detection Position  */
#define EIC_CONFIG_SENSE2_FALL                (EIC_CONFIG_SENSE2_FALL_Val << EIC_CONFIG_SENSE2_Pos) /**< (EIC_CONFIG) Falling edge detection Position  */
#define EIC_CONFIG_SENSE2_BOTH                (EIC_CONFIG_SENSE2_BOTH_Val << EIC_CONFIG_SENSE2_Pos) /**< (EIC_CONFIG) Both edges detection Position  */
#define EIC_CONFIG_SENSE2_HIGH                (EIC_CONFIG_SENSE2_HIGH_Val << EIC_CONFIG_SENSE2_Pos) /**< (EIC_CONFIG) High level detection Position  */
#define EIC_CONFIG_SENSE2_LOW                 (EIC_CONFIG_SENSE2_LOW_Val << EIC_CONFIG_SENSE2_Pos) /**< (EIC_CONFIG) Low level detection Position  */
#define EIC_CONFIG_FILTEN2_Pos                _U_(11)                                              /**< (EIC_CONFIG) Filter 2 Enable Position */
#define EIC_CONFIG_FILTEN2_Msk                (_U_(0x1) << EIC_CONFIG_FILTEN2_Pos)                 /**< (EIC_CONFIG) Filter 2 Enable Mask */
#define EIC_CONFIG_FILTEN2(value)             (EIC_CONFIG_FILTEN2_Msk & ((value) << EIC_CONFIG_FILTEN2_Pos))
#define EIC_CONFIG_SENSE3_Pos                 _U_(12)                                              /**< (EIC_CONFIG) Input Sense 3 Configuration Position */
#define EIC_CONFIG_SENSE3_Msk                 (_U_(0x7) << EIC_CONFIG_SENSE3_Pos)                  /**< (EIC_CONFIG) Input Sense 3 Configuration Mask */
#define EIC_CONFIG_SENSE3(value)              (EIC_CONFIG_SENSE3_Msk & ((value) << EIC_CONFIG_SENSE3_Pos))
#define   EIC_CONFIG_SENSE3_NONE_Val          _U_(0x0)                                             /**< (EIC_CONFIG) No detection  */
#define   EIC_CONFIG_SENSE3_RISE_Val          _U_(0x1)                                             /**< (EIC_CONFIG) Rising edge detection  */
#define   EIC_CONFIG_SENSE3_FALL_Val          _U_(0x2)                                             /**< (EIC_CONFIG) Falling edge detection  */
#define   EIC_CONFIG_SENSE3_BOTH_Val          _U_(0x3)                                             /**< (EIC_CONFIG) Both edges detection  */
#define   EIC_CONFIG_SENSE3_HIGH_Val          _U_(0x4)                                             /**< (EIC_CONFIG) High level detection  */
#define   EIC_CONFIG_SENSE3_LOW_Val           _U_(0x5)                                             /**< (EIC_CONFIG) Low level detection  */
#define EIC_CONFIG_SENSE3_NONE                (EIC_CONFIG_SENSE3_NONE_Val << EIC_CONFIG_SENSE3_Pos) /**< (EIC_CONFIG) No detection Position  */
#define EIC_CONFIG_SENSE3_RISE                (EIC_CONFIG_SENSE3_RISE_Val << EIC_CONFIG_SENSE3_Pos) /**< (EIC_CONFIG) Rising edge detection Position  */
#define EIC_CONFIG_SENSE3_FALL                (EIC_CONFIG_SENSE3_FALL_Val << EIC_CONFIG_SENSE3_Pos) /**< (EIC_CONFIG) Falling edge detection Position  */
#define EIC_CONFIG_SENSE3_BOTH                (EIC_CONFIG_SENSE3_BOTH_Val << EIC_CONFIG_SENSE3_Pos) /**< (EIC_CONFIG) Both edges detection Position  */
#define EIC_CONFIG_SENSE3_HIGH                (EIC_CONFIG_SENSE3_HIGH_Val << EIC_CONFIG_SENSE3_Pos) /**< (EIC_CONFIG) High level detection Position  */
#define EIC_CONFIG_SENSE3_LOW                 (EIC_CONFIG_SENSE3_LOW_Val << EIC_CONFIG_SENSE3_Pos) /**< (EIC_CONFIG) Low level detection Position  */
#define EIC_CONFIG_FILTEN3_Pos                _U_(15)                                              /**< (EIC_CONFIG) Filter 3 Enable Position */
#define EIC_CONFIG_FILTEN3_Msk                (_U_(0x1) << EIC_CONFIG_FILTEN3_Pos)                 /**< (EIC_CONFIG) Filter 3 Enable Mask */
#define EIC_CONFIG_FILTEN3(value)             (EIC_CONFIG_FILTEN3_Msk & ((value) << EIC_CONFIG_FILTEN3_Pos))
#define EIC_CONFIG_SENSE4_Pos                 _U_(16)                                              /**< (EIC_CONFIG) Input Sense 4 Configuration Position */
#define EIC_CONFIG_SENSE4_Msk                 (_U_(0x7) << EIC_CONFIG_SENSE4_Pos)                  /**< (EIC_CONFIG) Input Sense 4 Configuration Mask */
#define EIC_CONFIG_SENSE4(value)              (EIC_CONFIG_SENSE4_Msk & ((value) << EIC_CONFIG_SENSE4_Pos))
#define   EIC_CONFIG_SENSE4_NONE_Val          _U_(0x0)                                             /**< (EIC_CONFIG) No detection  */
#define   EIC_CONFIG_SENSE4_RISE_Val          _U_(0x1)                                             /**< (EIC_CONFIG) Rising edge detection  */
#define   EIC_CONFIG_SENSE4_FALL_Val          _U_(0x2)                                             /**< (EIC_CONFIG) Falling edge detection  */
#define   EIC_CONFIG_SENSE4_BOTH_Val          _U_(0x3)                                             /**< (EIC_CONFIG) Both edges detection  */
#define   EIC_CONFIG_SENSE4_HIGH_Val          _U_(0x4)                                             /**< (EIC_CONFIG) High level detection  */
#define   EIC_CONFIG_SENSE4_LOW_Val           _U_(0x5)                                             /**< (EIC_CONFIG) Low level detection  */
#define EIC_CONFIG_SENSE4_NONE                (EIC_CONFIG_SENSE4_NONE_Val << EIC_CONFIG_SENSE4_Pos) /**< (EIC_CONFIG) No detection Position  */
#define EIC_CONFIG_SENSE4_RISE                (EIC_CONFIG_SENSE4_RISE_Val << EIC_CONFIG_SENSE4_Pos) /**< (EIC_CONFIG) Rising edge detection Position  */
#define EIC_CONFIG_SENSE4_FALL                (EIC_CONFIG_SENSE4_FALL_Val << EIC_CONFIG_SENSE4_Pos) /**< (EIC_CONFIG) Falling edge detection Position  */
#define EIC_CONFIG_SENSE4_BOTH                (EIC_CONFIG_SENSE4_BOTH_Val << EIC_CONFIG_SENSE4_Pos) /**< (EIC_CONFIG) Both edges detection Position  */
#define EIC_CONFIG_SENSE4_HIGH                (EIC_CONFIG_SENSE4_HIGH_Val << EIC_CONFIG_SENSE4_Pos) /**< (EIC_CONFIG) High level detection Position  */
#define EIC_CONFIG_SENSE4_LOW                 (EIC_CONFIG_SENSE4_LOW_Val << EIC_CONFIG_SENSE4_Pos) /**< (EIC_CONFIG) Low level detection Position  */
#define EIC_CONFIG_FILTEN4_Pos                _U_(19)                                              /**< (EIC_CONFIG) Filter 4 Enable Position */
#define EIC_CONFIG_FILTEN4_Msk                (_U_(0x1) << EIC_CONFIG_FILTEN4_Pos)                 /**< (EIC_CONFIG) Filter 4 Enable Mask */
#define EIC_CONFIG_FILTEN4(value)             (EIC_CONFIG_FILTEN4_Msk & ((value) << EIC_CONFIG_FILTEN4_Pos))
#define EIC_CONFIG_SENSE5_Pos                 _U_(20)                                              /**< (EIC_CONFIG) Input Sense 5 Configuration Position */
#define EIC_CONFIG_SENSE5_Msk                 (_U_(0x7) << EIC_CONFIG_SENSE5_Pos)                  /**< (EIC_CONFIG) Input Sense 5 Configuration Mask */
#define EIC_CONFIG_SENSE5(value)              (EIC_CONFIG_SENSE5_Msk & ((value) << EIC_CONFIG_SENSE5_Pos))
#define   EIC_CONFIG_SENSE5_NONE_Val          _U_(0x0)                                             /**< (EIC_CONFIG) No detection  */
#define   EIC_CONFIG_SENSE5_RISE_Val          _U_(0x1)                                             /**< (EIC_CONFIG) Rising edge detection  */
#define   EIC_CONFIG_SENSE5_FALL_Val          _U_(0x2)                                             /**< (EIC_CONFIG) Falling edge detection  */
#define   EIC_CONFIG_SENSE5_BOTH_Val          _U_(0x3)                                             /**< (EIC_CONFIG) Both edges detection  */
#define   EIC_CONFIG_SENSE5_HIGH_Val          _U_(0x4)                                             /**< (EIC_CONFIG) High level detection  */
#define   EIC_CONFIG_SENSE5_LOW_Val           _U_(0x5)                                             /**< (EIC_CONFIG) Low level detection  */
#define EIC_CONFIG_SENSE5_NONE                (EIC_CONFIG_SENSE5_NONE_Val << EIC_CONFIG_SENSE5_Pos) /**< (EIC_CONFIG) No detection Position  */
#define EIC_CONFIG_SENSE5_RISE                (EIC_CONFIG_SENSE5_RISE_Val << EIC_CONFIG_SENSE5_Pos) /**< (EIC_CONFIG) Rising edge detection Position  */
#define EIC_CONFIG_SENSE5_FALL                (EIC_CONFIG_SENSE5_FALL_Val << EIC_CONFIG_SENSE5_Pos) /**< (EIC_CONFIG) Falling edge detection Position  */
#define EIC_CONFIG_SENSE5_BOTH                (EIC_CONFIG_SENSE5_BOTH_Val << EIC_CONFIG_SENSE5_Pos) /**< (EIC_CONFIG) Both edges detection Position  */
#define EIC_CONFIG_SENSE5_HIGH                (EIC_CONFIG_SENSE5_HIGH_Val << EIC_CONFIG_SENSE5_Pos) /**< (EIC_CONFIG) High level detection Position  */
#define EIC_CONFIG_SENSE5_LOW                 (EIC_CONFIG_SENSE5_LOW_Val << EIC_CONFIG_SENSE5_Pos) /**< (EIC_CONFIG) Low level detection Position  */
#define EIC_CONFIG_FILTEN5_Pos                _U_(23)                                              /**< (EIC_CONFIG) Filter 5 Enable Position */
#define EIC_CONFIG_FILTEN5_Msk                (_U_(0x1) << EIC_CONFIG_FILTEN5_Pos)                 /**< (EIC_CONFIG) Filter 5 Enable Mask */
#define EIC_CONFIG_FILTEN5(value)             (EIC_CONFIG_FILTEN5_Msk & ((value) << EIC_CONFIG_FILTEN5_Pos))
#define EIC_CONFIG_SENSE6_Pos                 _U_(24)                                              /**< (EIC_CONFIG) Input Sense 6 Configuration Position */
#define EIC_CONFIG_SENSE6_Msk                 (_U_(0x7) << EIC_CONFIG_SENSE6_Pos)                  /**< (EIC_CONFIG) Input Sense 6 Configuration Mask */
#define EIC_CONFIG_SENSE6(value)              (EIC_CONFIG_SENSE6_Msk & ((value) << EIC_CONFIG_SENSE6_Pos))
#define   EIC_CONFIG_SENSE6_NONE_Val          _U_(0x0)                                             /**< (EIC_CONFIG) No detection  */
#define   EIC_CONFIG_SENSE6_RISE_Val          _U_(0x1)                                             /**< (EIC_CONFIG) Rising edge detection  */
#define   EIC_CONFIG_SENSE6_FALL_Val          _U_(0x2)                                             /**< (EIC_CONFIG) Falling edge detection  */
#define   EIC_CONFIG_SENSE6_BOTH_Val          _U_(0x3)                                             /**< (EIC_CONFIG) Both edges detection  */
#define   EIC_CONFIG_SENSE6_HIGH_Val          _U_(0x4)                                             /**< (EIC_CONFIG) High level detection  */
#define   EIC_CONFIG_SENSE6_LOW_Val           _U_(0x5)                                             /**< (EIC_CONFIG) Low level detection  */
#define EIC_CONFIG_SENSE6_NONE                (EIC_CONFIG_SENSE6_NONE_Val << EIC_CONFIG_SENSE6_Pos) /**< (EIC_CONFIG) No detection Position  */
#define EIC_CONFIG_SENSE6_RISE                (EIC_CONFIG_SENSE6_RISE_Val << EIC_CONFIG_SENSE6_Pos) /**< (EIC_CONFIG) Rising edge detection Position  */
#define EIC_CONFIG_SENSE6_FALL                (EIC_CONFIG_SENSE6_FALL_Val << EIC_CONFIG_SENSE6_Pos) /**< (EIC_CONFIG) Falling edge detection Position  */
#define EIC_CONFIG_SENSE6_BOTH                (EIC_CONFIG_SENSE6_BOTH_Val << EIC_CONFIG_SENSE6_Pos) /**< (EIC_CONFIG) Both edges detection Position  */
#define EIC_CONFIG_SENSE6_HIGH                (EIC_CONFIG_SENSE6_HIGH_Val << EIC_CONFIG_SENSE6_Pos) /**< (EIC_CONFIG) High level detection Position  */
#define EIC_CONFIG_SENSE6_LOW                 (EIC_CONFIG_SENSE6_LOW_Val << EIC_CONFIG_SENSE6_Pos) /**< (EIC_CONFIG) Low level detection Position  */
#define EIC_CONFIG_FILTEN6_Pos                _U_(27)                                              /**< (EIC_CONFIG) Filter 6 Enable Position */
#define EIC_CONFIG_FILTEN6_Msk                (_U_(0x1) << EIC_CONFIG_FILTEN6_Pos)                 /**< (EIC_CONFIG) Filter 6 Enable Mask */
#define EIC_CONFIG_FILTEN6(value)             (EIC_CONFIG_FILTEN6_Msk & ((value) << EIC_CONFIG_FILTEN6_Pos))
#define EIC_CONFIG_SENSE7_Pos                 _U_(28)                                              /**< (EIC_CONFIG) Input Sense 7 Configuration Position */
#define EIC_CONFIG_SENSE7_Msk                 (_U_(0x7) << EIC_CONFIG_SENSE7_Pos)                  /**< (EIC_CONFIG) Input Sense 7 Configuration Mask */
#define EIC_CONFIG_SENSE7(value)              (EIC_CONFIG_SENSE7_Msk & ((value) << EIC_CONFIG_SENSE7_Pos))
#define   EIC_CONFIG_SENSE7_NONE_Val          _U_(0x0)                                             /**< (EIC_CONFIG) No detection  */
#define   EIC_CONFIG_SENSE7_RISE_Val          _U_(0x1)                                             /**< (EIC_CONFIG) Rising edge detection  */
#define   EIC_CONFIG_SENSE7_FALL_Val          _U_(0x2)                                             /**< (EIC_CONFIG) Falling edge detection  */
#define   EIC_CONFIG_SENSE7_BOTH_Val          _U_(0x3)                                             /**< (EIC_CONFIG) Both edges detection  */
#define   EIC_CONFIG_SENSE7_HIGH_Val          _U_(0x4)                                             /**< (EIC_CONFIG) High level detection  */
#define   EIC_CONFIG_SENSE7_LOW_Val           _U_(0x5)                                             /**< (EIC_CONFIG) Low level detection  */
#define EIC_CONFIG_SENSE7_NONE                (EIC_CONFIG_SENSE7_NONE_Val << EIC_CONFIG_SENSE7_Pos) /**< (EIC_CONFIG) No detection Position  */
#define EIC_CONFIG_SENSE7_RISE                (EIC_CONFIG_SENSE7_RISE_Val << EIC_CONFIG_SENSE7_Pos) /**< (EIC_CONFIG) Rising edge detection Position  */
#define EIC_CONFIG_SENSE7_FALL                (EIC_CONFIG_SENSE7_FALL_Val << EIC_CONFIG_SENSE7_Pos) /**< (EIC_CONFIG) Falling edge detection Position  */
#define EIC_CONFIG_SENSE7_BOTH                (EIC_CONFIG_SENSE7_BOTH_Val << EIC_CONFIG_SENSE7_Pos) /**< (EIC_CONFIG) Both edges detection Position  */
#define EIC_CONFIG_SENSE7_HIGH                (EIC_CONFIG_SENSE7_HIGH_Val << EIC_CONFIG_SENSE7_Pos) /**< (EIC_CONFIG) High level detection Position  */
#define EIC_CONFIG_SENSE7_LOW                 (EIC_CONFIG_SENSE7_LOW_Val << EIC_CONFIG_SENSE7_Pos) /**< (EIC_CONFIG) Low level detection Position  */
#define EIC_CONFIG_FILTEN7_Pos                _U_(31)                                              /**< (EIC_CONFIG) Filter 7 Enable Position */
#define EIC_CONFIG_FILTEN7_Msk                (_U_(0x1) << EIC_CONFIG_FILTEN7_Pos)                 /**< (EIC_CONFIG) Filter 7 Enable Mask */
#define EIC_CONFIG_FILTEN7(value)             (EIC_CONFIG_FILTEN7_Msk & ((value) << EIC_CONFIG_FILTEN7_Pos))
#define EIC_CONFIG_Msk                        _U_(0xFFFFFFFF)                                      /**< (EIC_CONFIG) Register Mask  */


/** \brief EIC register offsets definitions */
#define EIC_CTRL_REG_OFST              (0x00)              /**< (EIC_CTRL) Control Offset */
#define EIC_STATUS_REG_OFST            (0x01)              /**< (EIC_STATUS) Status Offset */
#define EIC_NMICTRL_REG_OFST           (0x02)              /**< (EIC_NMICTRL) Non-Maskable Interrupt Control Offset */
#define EIC_NMIFLAG_REG_OFST           (0x03)              /**< (EIC_NMIFLAG) Non-Maskable Interrupt Flag Status and Clear Offset */
#define EIC_EVCTRL_REG_OFST            (0x04)              /**< (EIC_EVCTRL) Event Control Offset */
#define EIC_INTENCLR_REG_OFST          (0x08)              /**< (EIC_INTENCLR) Interrupt Enable Clear Offset */
#define EIC_INTENSET_REG_OFST          (0x0C)              /**< (EIC_INTENSET) Interrupt Enable Set Offset */
#define EIC_INTFLAG_REG_OFST           (0x10)              /**< (EIC_INTFLAG) Interrupt Flag Status and Clear Offset */
#define EIC_WAKEUP_REG_OFST            (0x14)              /**< (EIC_WAKEUP) Wake-Up Enable Offset */
#define EIC_CONFIG_REG_OFST            (0x18)              /**< (EIC_CONFIG) Configuration n Offset */

#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
/** \brief EIC register API structure */
typedef struct
{  /* External Interrupt Controller */
  __IO  uint8_t                        EIC_CTRL;           /**< Offset: 0x00 (R/W  8) Control */
  __I   uint8_t                        EIC_STATUS;         /**< Offset: 0x01 (R/   8) Status */
  __IO  uint8_t                        EIC_NMICTRL;        /**< Offset: 0x02 (R/W  8) Non-Maskable Interrupt Control */
  __IO  uint8_t                        EIC_NMIFLAG;        /**< Offset: 0x03 (R/W  8) Non-Maskable Interrupt Flag Status and Clear */
  __IO  uint32_t                       EIC_EVCTRL;         /**< Offset: 0x04 (R/W  32) Event Control */
  __IO  uint32_t                       EIC_INTENCLR;       /**< Offset: 0x08 (R/W  32) Interrupt Enable Clear */
  __IO  uint32_t                       EIC_INTENSET;       /**< Offset: 0x0C (R/W  32) Interrupt Enable Set */
  __IO  uint32_t                       EIC_INTFLAG;        /**< Offset: 0x10 (R/W  32) Interrupt Flag Status and Clear */
  __IO  uint32_t                       EIC_WAKEUP;         /**< Offset: 0x14 (R/W  32) Wake-Up Enable */
  __IO  uint32_t                       EIC_CONFIG[2];      /**< Offset: 0x18 (R/W  32) Configuration n */
} eic_registers_t;


#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
#endif /* _SAMD21_EIC_COMPONENT_H_ */
