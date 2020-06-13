/**
 * \brief Component description for I2S
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
#ifndef _SAMD21_I2S_COMPONENT_H_
#define _SAMD21_I2S_COMPONENT_H_

/* ************************************************************************** */
/*   SOFTWARE API DEFINITION FOR I2S                                          */
/* ************************************************************************** */

/* -------- I2S_CTRLA : (I2S Offset: 0x00) (R/W 8) Control A -------- */
#define I2S_CTRLA_RESETVALUE                  _U_(0x00)                                            /**<  (I2S_CTRLA) Control A  Reset Value */

#define I2S_CTRLA_SWRST_Pos                   _U_(0)                                               /**< (I2S_CTRLA) Software Reset Position */
#define I2S_CTRLA_SWRST_Msk                   (_U_(0x1) << I2S_CTRLA_SWRST_Pos)                    /**< (I2S_CTRLA) Software Reset Mask */
#define I2S_CTRLA_SWRST(value)                (I2S_CTRLA_SWRST_Msk & ((value) << I2S_CTRLA_SWRST_Pos))
#define I2S_CTRLA_ENABLE_Pos                  _U_(1)                                               /**< (I2S_CTRLA) Enable Position */
#define I2S_CTRLA_ENABLE_Msk                  (_U_(0x1) << I2S_CTRLA_ENABLE_Pos)                   /**< (I2S_CTRLA) Enable Mask */
#define I2S_CTRLA_ENABLE(value)               (I2S_CTRLA_ENABLE_Msk & ((value) << I2S_CTRLA_ENABLE_Pos))
#define I2S_CTRLA_CKEN0_Pos                   _U_(2)                                               /**< (I2S_CTRLA) Clock Unit 0 Enable Position */
#define I2S_CTRLA_CKEN0_Msk                   (_U_(0x1) << I2S_CTRLA_CKEN0_Pos)                    /**< (I2S_CTRLA) Clock Unit 0 Enable Mask */
#define I2S_CTRLA_CKEN0(value)                (I2S_CTRLA_CKEN0_Msk & ((value) << I2S_CTRLA_CKEN0_Pos))
#define I2S_CTRLA_CKEN1_Pos                   _U_(3)                                               /**< (I2S_CTRLA) Clock Unit 1 Enable Position */
#define I2S_CTRLA_CKEN1_Msk                   (_U_(0x1) << I2S_CTRLA_CKEN1_Pos)                    /**< (I2S_CTRLA) Clock Unit 1 Enable Mask */
#define I2S_CTRLA_CKEN1(value)                (I2S_CTRLA_CKEN1_Msk & ((value) << I2S_CTRLA_CKEN1_Pos))
#define I2S_CTRLA_SEREN0_Pos                  _U_(4)                                               /**< (I2S_CTRLA) Serializer 0 Enable Position */
#define I2S_CTRLA_SEREN0_Msk                  (_U_(0x1) << I2S_CTRLA_SEREN0_Pos)                   /**< (I2S_CTRLA) Serializer 0 Enable Mask */
#define I2S_CTRLA_SEREN0(value)               (I2S_CTRLA_SEREN0_Msk & ((value) << I2S_CTRLA_SEREN0_Pos))
#define I2S_CTRLA_SEREN1_Pos                  _U_(5)                                               /**< (I2S_CTRLA) Serializer 1 Enable Position */
#define I2S_CTRLA_SEREN1_Msk                  (_U_(0x1) << I2S_CTRLA_SEREN1_Pos)                   /**< (I2S_CTRLA) Serializer 1 Enable Mask */
#define I2S_CTRLA_SEREN1(value)               (I2S_CTRLA_SEREN1_Msk & ((value) << I2S_CTRLA_SEREN1_Pos))
#define I2S_CTRLA_Msk                         _U_(0x3F)                                            /**< (I2S_CTRLA) Register Mask  */

#define I2S_CTRLA_CKEN_Pos                    _U_(2)                                               /**< (I2S_CTRLA Position) Clock Unit x Enable */
#define I2S_CTRLA_CKEN_Msk                    (_U_(0x3) << I2S_CTRLA_CKEN_Pos)                     /**< (I2S_CTRLA Mask) CKEN */
#define I2S_CTRLA_CKEN(value)                 (I2S_CTRLA_CKEN_Msk & ((value) << I2S_CTRLA_CKEN_Pos)) 
#define I2S_CTRLA_SEREN_Pos                   _U_(4)                                               /**< (I2S_CTRLA Position) Serializer x Enable */
#define I2S_CTRLA_SEREN_Msk                   (_U_(0x3) << I2S_CTRLA_SEREN_Pos)                    /**< (I2S_CTRLA Mask) SEREN */
#define I2S_CTRLA_SEREN(value)                (I2S_CTRLA_SEREN_Msk & ((value) << I2S_CTRLA_SEREN_Pos)) 

/* -------- I2S_CLKCTRL : (I2S Offset: 0x04) (R/W 32) Clock Unit n Control -------- */
#define I2S_CLKCTRL_RESETVALUE                _U_(0x00)                                            /**<  (I2S_CLKCTRL) Clock Unit n Control  Reset Value */

#define I2S_CLKCTRL_SLOTSIZE_Pos              _U_(0)                                               /**< (I2S_CLKCTRL) Slot Size Position */
#define I2S_CLKCTRL_SLOTSIZE_Msk              (_U_(0x3) << I2S_CLKCTRL_SLOTSIZE_Pos)               /**< (I2S_CLKCTRL) Slot Size Mask */
#define I2S_CLKCTRL_SLOTSIZE(value)           (I2S_CLKCTRL_SLOTSIZE_Msk & ((value) << I2S_CLKCTRL_SLOTSIZE_Pos))
#define   I2S_CLKCTRL_SLOTSIZE_8_Val          _U_(0x0)                                             /**< (I2S_CLKCTRL) 8-bit Slot for Clock Unit n  */
#define   I2S_CLKCTRL_SLOTSIZE_16_Val         _U_(0x1)                                             /**< (I2S_CLKCTRL) 16-bit Slot for Clock Unit n  */
#define   I2S_CLKCTRL_SLOTSIZE_24_Val         _U_(0x2)                                             /**< (I2S_CLKCTRL) 24-bit Slot for Clock Unit n  */
#define   I2S_CLKCTRL_SLOTSIZE_32_Val         _U_(0x3)                                             /**< (I2S_CLKCTRL) 32-bit Slot for Clock Unit n  */
#define I2S_CLKCTRL_SLOTSIZE_8                (I2S_CLKCTRL_SLOTSIZE_8_Val << I2S_CLKCTRL_SLOTSIZE_Pos) /**< (I2S_CLKCTRL) 8-bit Slot for Clock Unit n Position  */
#define I2S_CLKCTRL_SLOTSIZE_16               (I2S_CLKCTRL_SLOTSIZE_16_Val << I2S_CLKCTRL_SLOTSIZE_Pos) /**< (I2S_CLKCTRL) 16-bit Slot for Clock Unit n Position  */
#define I2S_CLKCTRL_SLOTSIZE_24               (I2S_CLKCTRL_SLOTSIZE_24_Val << I2S_CLKCTRL_SLOTSIZE_Pos) /**< (I2S_CLKCTRL) 24-bit Slot for Clock Unit n Position  */
#define I2S_CLKCTRL_SLOTSIZE_32               (I2S_CLKCTRL_SLOTSIZE_32_Val << I2S_CLKCTRL_SLOTSIZE_Pos) /**< (I2S_CLKCTRL) 32-bit Slot for Clock Unit n Position  */
#define I2S_CLKCTRL_NBSLOTS_Pos               _U_(2)                                               /**< (I2S_CLKCTRL) Number of Slots in Frame Position */
#define I2S_CLKCTRL_NBSLOTS_Msk               (_U_(0x7) << I2S_CLKCTRL_NBSLOTS_Pos)                /**< (I2S_CLKCTRL) Number of Slots in Frame Mask */
#define I2S_CLKCTRL_NBSLOTS(value)            (I2S_CLKCTRL_NBSLOTS_Msk & ((value) << I2S_CLKCTRL_NBSLOTS_Pos))
#define I2S_CLKCTRL_FSWIDTH_Pos               _U_(5)                                               /**< (I2S_CLKCTRL) Frame Sync Width Position */
#define I2S_CLKCTRL_FSWIDTH_Msk               (_U_(0x3) << I2S_CLKCTRL_FSWIDTH_Pos)                /**< (I2S_CLKCTRL) Frame Sync Width Mask */
#define I2S_CLKCTRL_FSWIDTH(value)            (I2S_CLKCTRL_FSWIDTH_Msk & ((value) << I2S_CLKCTRL_FSWIDTH_Pos))
#define   I2S_CLKCTRL_FSWIDTH_SLOT_Val        _U_(0x0)                                             /**< (I2S_CLKCTRL) Frame Sync Pulse is 1 Slot wide (default for I2S protocol)  */
#define   I2S_CLKCTRL_FSWIDTH_HALF_Val        _U_(0x1)                                             /**< (I2S_CLKCTRL) Frame Sync Pulse is half a Frame wide  */
#define   I2S_CLKCTRL_FSWIDTH_BIT_Val         _U_(0x2)                                             /**< (I2S_CLKCTRL) Frame Sync Pulse is 1 Bit wide  */
#define   I2S_CLKCTRL_FSWIDTH_BURST_Val       _U_(0x3)                                             /**< (I2S_CLKCTRL) Clock Unit n operates in Burst mode, with a 1-bit wide Frame Sync pulse per Data sample, only when Data transfer is requested  */
#define I2S_CLKCTRL_FSWIDTH_SLOT              (I2S_CLKCTRL_FSWIDTH_SLOT_Val << I2S_CLKCTRL_FSWIDTH_Pos) /**< (I2S_CLKCTRL) Frame Sync Pulse is 1 Slot wide (default for I2S protocol) Position  */
#define I2S_CLKCTRL_FSWIDTH_HALF              (I2S_CLKCTRL_FSWIDTH_HALF_Val << I2S_CLKCTRL_FSWIDTH_Pos) /**< (I2S_CLKCTRL) Frame Sync Pulse is half a Frame wide Position  */
#define I2S_CLKCTRL_FSWIDTH_BIT               (I2S_CLKCTRL_FSWIDTH_BIT_Val << I2S_CLKCTRL_FSWIDTH_Pos) /**< (I2S_CLKCTRL) Frame Sync Pulse is 1 Bit wide Position  */
#define I2S_CLKCTRL_FSWIDTH_BURST             (I2S_CLKCTRL_FSWIDTH_BURST_Val << I2S_CLKCTRL_FSWIDTH_Pos) /**< (I2S_CLKCTRL) Clock Unit n operates in Burst mode, with a 1-bit wide Frame Sync pulse per Data sample, only when Data transfer is requested Position  */
#define I2S_CLKCTRL_BITDELAY_Pos              _U_(7)                                               /**< (I2S_CLKCTRL) Data Delay from Frame Sync Position */
#define I2S_CLKCTRL_BITDELAY_Msk              (_U_(0x1) << I2S_CLKCTRL_BITDELAY_Pos)               /**< (I2S_CLKCTRL) Data Delay from Frame Sync Mask */
#define I2S_CLKCTRL_BITDELAY(value)           (I2S_CLKCTRL_BITDELAY_Msk & ((value) << I2S_CLKCTRL_BITDELAY_Pos))
#define   I2S_CLKCTRL_BITDELAY_LJ_Val         _U_(0x0)                                             /**< (I2S_CLKCTRL) Left Justified (0 Bit Delay)  */
#define   I2S_CLKCTRL_BITDELAY_I2S_Val        _U_(0x1)                                             /**< (I2S_CLKCTRL) I2S (1 Bit Delay)  */
#define I2S_CLKCTRL_BITDELAY_LJ               (I2S_CLKCTRL_BITDELAY_LJ_Val << I2S_CLKCTRL_BITDELAY_Pos) /**< (I2S_CLKCTRL) Left Justified (0 Bit Delay) Position  */
#define I2S_CLKCTRL_BITDELAY_I2S              (I2S_CLKCTRL_BITDELAY_I2S_Val << I2S_CLKCTRL_BITDELAY_Pos) /**< (I2S_CLKCTRL) I2S (1 Bit Delay) Position  */
#define I2S_CLKCTRL_FSSEL_Pos                 _U_(8)                                               /**< (I2S_CLKCTRL) Frame Sync Select Position */
#define I2S_CLKCTRL_FSSEL_Msk                 (_U_(0x1) << I2S_CLKCTRL_FSSEL_Pos)                  /**< (I2S_CLKCTRL) Frame Sync Select Mask */
#define I2S_CLKCTRL_FSSEL(value)              (I2S_CLKCTRL_FSSEL_Msk & ((value) << I2S_CLKCTRL_FSSEL_Pos))
#define   I2S_CLKCTRL_FSSEL_SCKDIV_Val        _U_(0x0)                                             /**< (I2S_CLKCTRL) Divided Serial Clock n is used as Frame Sync n source  */
#define   I2S_CLKCTRL_FSSEL_FSPIN_Val         _U_(0x1)                                             /**< (I2S_CLKCTRL) FSn input pin is used as Frame Sync n source  */
#define I2S_CLKCTRL_FSSEL_SCKDIV              (I2S_CLKCTRL_FSSEL_SCKDIV_Val << I2S_CLKCTRL_FSSEL_Pos) /**< (I2S_CLKCTRL) Divided Serial Clock n is used as Frame Sync n source Position  */
#define I2S_CLKCTRL_FSSEL_FSPIN               (I2S_CLKCTRL_FSSEL_FSPIN_Val << I2S_CLKCTRL_FSSEL_Pos) /**< (I2S_CLKCTRL) FSn input pin is used as Frame Sync n source Position  */
#define I2S_CLKCTRL_FSINV_Pos                 _U_(11)                                              /**< (I2S_CLKCTRL) Frame Sync Invert Position */
#define I2S_CLKCTRL_FSINV_Msk                 (_U_(0x1) << I2S_CLKCTRL_FSINV_Pos)                  /**< (I2S_CLKCTRL) Frame Sync Invert Mask */
#define I2S_CLKCTRL_FSINV(value)              (I2S_CLKCTRL_FSINV_Msk & ((value) << I2S_CLKCTRL_FSINV_Pos))
#define I2S_CLKCTRL_SCKSEL_Pos                _U_(12)                                              /**< (I2S_CLKCTRL) Serial Clock Select Position */
#define I2S_CLKCTRL_SCKSEL_Msk                (_U_(0x1) << I2S_CLKCTRL_SCKSEL_Pos)                 /**< (I2S_CLKCTRL) Serial Clock Select Mask */
#define I2S_CLKCTRL_SCKSEL(value)             (I2S_CLKCTRL_SCKSEL_Msk & ((value) << I2S_CLKCTRL_SCKSEL_Pos))
#define   I2S_CLKCTRL_SCKSEL_MCKDIV_Val       _U_(0x0)                                             /**< (I2S_CLKCTRL) Divided Master Clock n is used as Serial Clock n source  */
#define   I2S_CLKCTRL_SCKSEL_SCKPIN_Val       _U_(0x1)                                             /**< (I2S_CLKCTRL) SCKn input pin is used as Serial Clock n source  */
#define I2S_CLKCTRL_SCKSEL_MCKDIV             (I2S_CLKCTRL_SCKSEL_MCKDIV_Val << I2S_CLKCTRL_SCKSEL_Pos) /**< (I2S_CLKCTRL) Divided Master Clock n is used as Serial Clock n source Position  */
#define I2S_CLKCTRL_SCKSEL_SCKPIN             (I2S_CLKCTRL_SCKSEL_SCKPIN_Val << I2S_CLKCTRL_SCKSEL_Pos) /**< (I2S_CLKCTRL) SCKn input pin is used as Serial Clock n source Position  */
#define I2S_CLKCTRL_MCKSEL_Pos                _U_(16)                                              /**< (I2S_CLKCTRL) Master Clock Select Position */
#define I2S_CLKCTRL_MCKSEL_Msk                (_U_(0x1) << I2S_CLKCTRL_MCKSEL_Pos)                 /**< (I2S_CLKCTRL) Master Clock Select Mask */
#define I2S_CLKCTRL_MCKSEL(value)             (I2S_CLKCTRL_MCKSEL_Msk & ((value) << I2S_CLKCTRL_MCKSEL_Pos))
#define   I2S_CLKCTRL_MCKSEL_GCLK_Val         _U_(0x0)                                             /**< (I2S_CLKCTRL) GCLK_I2S_n is used as Master Clock n source  */
#define   I2S_CLKCTRL_MCKSEL_MCKPIN_Val       _U_(0x1)                                             /**< (I2S_CLKCTRL) MCKn input pin is used as Master Clock n source  */
#define I2S_CLKCTRL_MCKSEL_GCLK               (I2S_CLKCTRL_MCKSEL_GCLK_Val << I2S_CLKCTRL_MCKSEL_Pos) /**< (I2S_CLKCTRL) GCLK_I2S_n is used as Master Clock n source Position  */
#define I2S_CLKCTRL_MCKSEL_MCKPIN             (I2S_CLKCTRL_MCKSEL_MCKPIN_Val << I2S_CLKCTRL_MCKSEL_Pos) /**< (I2S_CLKCTRL) MCKn input pin is used as Master Clock n source Position  */
#define I2S_CLKCTRL_MCKEN_Pos                 _U_(18)                                              /**< (I2S_CLKCTRL) Master Clock Enable Position */
#define I2S_CLKCTRL_MCKEN_Msk                 (_U_(0x1) << I2S_CLKCTRL_MCKEN_Pos)                  /**< (I2S_CLKCTRL) Master Clock Enable Mask */
#define I2S_CLKCTRL_MCKEN(value)              (I2S_CLKCTRL_MCKEN_Msk & ((value) << I2S_CLKCTRL_MCKEN_Pos))
#define I2S_CLKCTRL_MCKDIV_Pos                _U_(19)                                              /**< (I2S_CLKCTRL) Master Clock Division Factor Position */
#define I2S_CLKCTRL_MCKDIV_Msk                (_U_(0x1F) << I2S_CLKCTRL_MCKDIV_Pos)                /**< (I2S_CLKCTRL) Master Clock Division Factor Mask */
#define I2S_CLKCTRL_MCKDIV(value)             (I2S_CLKCTRL_MCKDIV_Msk & ((value) << I2S_CLKCTRL_MCKDIV_Pos))
#define I2S_CLKCTRL_MCKOUTDIV_Pos             _U_(24)                                              /**< (I2S_CLKCTRL) Master Clock Output Division Factor Position */
#define I2S_CLKCTRL_MCKOUTDIV_Msk             (_U_(0x1F) << I2S_CLKCTRL_MCKOUTDIV_Pos)             /**< (I2S_CLKCTRL) Master Clock Output Division Factor Mask */
#define I2S_CLKCTRL_MCKOUTDIV(value)          (I2S_CLKCTRL_MCKOUTDIV_Msk & ((value) << I2S_CLKCTRL_MCKOUTDIV_Pos))
#define I2S_CLKCTRL_FSOUTINV_Pos              _U_(29)                                              /**< (I2S_CLKCTRL) Frame Sync Output Invert Position */
#define I2S_CLKCTRL_FSOUTINV_Msk              (_U_(0x1) << I2S_CLKCTRL_FSOUTINV_Pos)               /**< (I2S_CLKCTRL) Frame Sync Output Invert Mask */
#define I2S_CLKCTRL_FSOUTINV(value)           (I2S_CLKCTRL_FSOUTINV_Msk & ((value) << I2S_CLKCTRL_FSOUTINV_Pos))
#define I2S_CLKCTRL_SCKOUTINV_Pos             _U_(30)                                              /**< (I2S_CLKCTRL) Serial Clock Output Invert Position */
#define I2S_CLKCTRL_SCKOUTINV_Msk             (_U_(0x1) << I2S_CLKCTRL_SCKOUTINV_Pos)              /**< (I2S_CLKCTRL) Serial Clock Output Invert Mask */
#define I2S_CLKCTRL_SCKOUTINV(value)          (I2S_CLKCTRL_SCKOUTINV_Msk & ((value) << I2S_CLKCTRL_SCKOUTINV_Pos))
#define I2S_CLKCTRL_MCKOUTINV_Pos             _U_(31)                                              /**< (I2S_CLKCTRL) Master Clock Output Invert Position */
#define I2S_CLKCTRL_MCKOUTINV_Msk             (_U_(0x1) << I2S_CLKCTRL_MCKOUTINV_Pos)              /**< (I2S_CLKCTRL) Master Clock Output Invert Mask */
#define I2S_CLKCTRL_MCKOUTINV(value)          (I2S_CLKCTRL_MCKOUTINV_Msk & ((value) << I2S_CLKCTRL_MCKOUTINV_Pos))
#define I2S_CLKCTRL_Msk                       _U_(0xFFFD19FF)                                      /**< (I2S_CLKCTRL) Register Mask  */


/* -------- I2S_INTENCLR : (I2S Offset: 0x0C) (R/W 16) Interrupt Enable Clear -------- */
#define I2S_INTENCLR_RESETVALUE               _U_(0x00)                                            /**<  (I2S_INTENCLR) Interrupt Enable Clear  Reset Value */

#define I2S_INTENCLR_RXRDY0_Pos               _U_(0)                                               /**< (I2S_INTENCLR) Receive Ready 0 Interrupt Enable Position */
#define I2S_INTENCLR_RXRDY0_Msk               (_U_(0x1) << I2S_INTENCLR_RXRDY0_Pos)                /**< (I2S_INTENCLR) Receive Ready 0 Interrupt Enable Mask */
#define I2S_INTENCLR_RXRDY0(value)            (I2S_INTENCLR_RXRDY0_Msk & ((value) << I2S_INTENCLR_RXRDY0_Pos))
#define I2S_INTENCLR_RXRDY1_Pos               _U_(1)                                               /**< (I2S_INTENCLR) Receive Ready 1 Interrupt Enable Position */
#define I2S_INTENCLR_RXRDY1_Msk               (_U_(0x1) << I2S_INTENCLR_RXRDY1_Pos)                /**< (I2S_INTENCLR) Receive Ready 1 Interrupt Enable Mask */
#define I2S_INTENCLR_RXRDY1(value)            (I2S_INTENCLR_RXRDY1_Msk & ((value) << I2S_INTENCLR_RXRDY1_Pos))
#define I2S_INTENCLR_RXOR0_Pos                _U_(4)                                               /**< (I2S_INTENCLR) Receive Overrun 0 Interrupt Enable Position */
#define I2S_INTENCLR_RXOR0_Msk                (_U_(0x1) << I2S_INTENCLR_RXOR0_Pos)                 /**< (I2S_INTENCLR) Receive Overrun 0 Interrupt Enable Mask */
#define I2S_INTENCLR_RXOR0(value)             (I2S_INTENCLR_RXOR0_Msk & ((value) << I2S_INTENCLR_RXOR0_Pos))
#define I2S_INTENCLR_RXOR1_Pos                _U_(5)                                               /**< (I2S_INTENCLR) Receive Overrun 1 Interrupt Enable Position */
#define I2S_INTENCLR_RXOR1_Msk                (_U_(0x1) << I2S_INTENCLR_RXOR1_Pos)                 /**< (I2S_INTENCLR) Receive Overrun 1 Interrupt Enable Mask */
#define I2S_INTENCLR_RXOR1(value)             (I2S_INTENCLR_RXOR1_Msk & ((value) << I2S_INTENCLR_RXOR1_Pos))
#define I2S_INTENCLR_TXRDY0_Pos               _U_(8)                                               /**< (I2S_INTENCLR) Transmit Ready 0 Interrupt Enable Position */
#define I2S_INTENCLR_TXRDY0_Msk               (_U_(0x1) << I2S_INTENCLR_TXRDY0_Pos)                /**< (I2S_INTENCLR) Transmit Ready 0 Interrupt Enable Mask */
#define I2S_INTENCLR_TXRDY0(value)            (I2S_INTENCLR_TXRDY0_Msk & ((value) << I2S_INTENCLR_TXRDY0_Pos))
#define I2S_INTENCLR_TXRDY1_Pos               _U_(9)                                               /**< (I2S_INTENCLR) Transmit Ready 1 Interrupt Enable Position */
#define I2S_INTENCLR_TXRDY1_Msk               (_U_(0x1) << I2S_INTENCLR_TXRDY1_Pos)                /**< (I2S_INTENCLR) Transmit Ready 1 Interrupt Enable Mask */
#define I2S_INTENCLR_TXRDY1(value)            (I2S_INTENCLR_TXRDY1_Msk & ((value) << I2S_INTENCLR_TXRDY1_Pos))
#define I2S_INTENCLR_TXUR0_Pos                _U_(12)                                              /**< (I2S_INTENCLR) Transmit Underrun 0 Interrupt Enable Position */
#define I2S_INTENCLR_TXUR0_Msk                (_U_(0x1) << I2S_INTENCLR_TXUR0_Pos)                 /**< (I2S_INTENCLR) Transmit Underrun 0 Interrupt Enable Mask */
#define I2S_INTENCLR_TXUR0(value)             (I2S_INTENCLR_TXUR0_Msk & ((value) << I2S_INTENCLR_TXUR0_Pos))
#define I2S_INTENCLR_TXUR1_Pos                _U_(13)                                              /**< (I2S_INTENCLR) Transmit Underrun 1 Interrupt Enable Position */
#define I2S_INTENCLR_TXUR1_Msk                (_U_(0x1) << I2S_INTENCLR_TXUR1_Pos)                 /**< (I2S_INTENCLR) Transmit Underrun 1 Interrupt Enable Mask */
#define I2S_INTENCLR_TXUR1(value)             (I2S_INTENCLR_TXUR1_Msk & ((value) << I2S_INTENCLR_TXUR1_Pos))
#define I2S_INTENCLR_Msk                      _U_(0x3333)                                          /**< (I2S_INTENCLR) Register Mask  */

#define I2S_INTENCLR_RXRDY_Pos                _U_(0)                                               /**< (I2S_INTENCLR Position) Receive Ready x Interrupt Enable */
#define I2S_INTENCLR_RXRDY_Msk                (_U_(0x3) << I2S_INTENCLR_RXRDY_Pos)                 /**< (I2S_INTENCLR Mask) RXRDY */
#define I2S_INTENCLR_RXRDY(value)             (I2S_INTENCLR_RXRDY_Msk & ((value) << I2S_INTENCLR_RXRDY_Pos)) 
#define I2S_INTENCLR_RXOR_Pos                 _U_(4)                                               /**< (I2S_INTENCLR Position) Receive Overrun x Interrupt Enable */
#define I2S_INTENCLR_RXOR_Msk                 (_U_(0x3) << I2S_INTENCLR_RXOR_Pos)                  /**< (I2S_INTENCLR Mask) RXOR */
#define I2S_INTENCLR_RXOR(value)              (I2S_INTENCLR_RXOR_Msk & ((value) << I2S_INTENCLR_RXOR_Pos)) 
#define I2S_INTENCLR_TXRDY_Pos                _U_(8)                                               /**< (I2S_INTENCLR Position) Transmit Ready x Interrupt Enable */
#define I2S_INTENCLR_TXRDY_Msk                (_U_(0x3) << I2S_INTENCLR_TXRDY_Pos)                 /**< (I2S_INTENCLR Mask) TXRDY */
#define I2S_INTENCLR_TXRDY(value)             (I2S_INTENCLR_TXRDY_Msk & ((value) << I2S_INTENCLR_TXRDY_Pos)) 
#define I2S_INTENCLR_TXUR_Pos                 _U_(12)                                              /**< (I2S_INTENCLR Position) Transmit Underrun x Interrupt Enable */
#define I2S_INTENCLR_TXUR_Msk                 (_U_(0x3) << I2S_INTENCLR_TXUR_Pos)                  /**< (I2S_INTENCLR Mask) TXUR */
#define I2S_INTENCLR_TXUR(value)              (I2S_INTENCLR_TXUR_Msk & ((value) << I2S_INTENCLR_TXUR_Pos)) 

/* -------- I2S_INTENSET : (I2S Offset: 0x10) (R/W 16) Interrupt Enable Set -------- */
#define I2S_INTENSET_RESETVALUE               _U_(0x00)                                            /**<  (I2S_INTENSET) Interrupt Enable Set  Reset Value */

#define I2S_INTENSET_RXRDY0_Pos               _U_(0)                                               /**< (I2S_INTENSET) Receive Ready 0 Interrupt Enable Position */
#define I2S_INTENSET_RXRDY0_Msk               (_U_(0x1) << I2S_INTENSET_RXRDY0_Pos)                /**< (I2S_INTENSET) Receive Ready 0 Interrupt Enable Mask */
#define I2S_INTENSET_RXRDY0(value)            (I2S_INTENSET_RXRDY0_Msk & ((value) << I2S_INTENSET_RXRDY0_Pos))
#define I2S_INTENSET_RXRDY1_Pos               _U_(1)                                               /**< (I2S_INTENSET) Receive Ready 1 Interrupt Enable Position */
#define I2S_INTENSET_RXRDY1_Msk               (_U_(0x1) << I2S_INTENSET_RXRDY1_Pos)                /**< (I2S_INTENSET) Receive Ready 1 Interrupt Enable Mask */
#define I2S_INTENSET_RXRDY1(value)            (I2S_INTENSET_RXRDY1_Msk & ((value) << I2S_INTENSET_RXRDY1_Pos))
#define I2S_INTENSET_RXOR0_Pos                _U_(4)                                               /**< (I2S_INTENSET) Receive Overrun 0 Interrupt Enable Position */
#define I2S_INTENSET_RXOR0_Msk                (_U_(0x1) << I2S_INTENSET_RXOR0_Pos)                 /**< (I2S_INTENSET) Receive Overrun 0 Interrupt Enable Mask */
#define I2S_INTENSET_RXOR0(value)             (I2S_INTENSET_RXOR0_Msk & ((value) << I2S_INTENSET_RXOR0_Pos))
#define I2S_INTENSET_RXOR1_Pos                _U_(5)                                               /**< (I2S_INTENSET) Receive Overrun 1 Interrupt Enable Position */
#define I2S_INTENSET_RXOR1_Msk                (_U_(0x1) << I2S_INTENSET_RXOR1_Pos)                 /**< (I2S_INTENSET) Receive Overrun 1 Interrupt Enable Mask */
#define I2S_INTENSET_RXOR1(value)             (I2S_INTENSET_RXOR1_Msk & ((value) << I2S_INTENSET_RXOR1_Pos))
#define I2S_INTENSET_TXRDY0_Pos               _U_(8)                                               /**< (I2S_INTENSET) Transmit Ready 0 Interrupt Enable Position */
#define I2S_INTENSET_TXRDY0_Msk               (_U_(0x1) << I2S_INTENSET_TXRDY0_Pos)                /**< (I2S_INTENSET) Transmit Ready 0 Interrupt Enable Mask */
#define I2S_INTENSET_TXRDY0(value)            (I2S_INTENSET_TXRDY0_Msk & ((value) << I2S_INTENSET_TXRDY0_Pos))
#define I2S_INTENSET_TXRDY1_Pos               _U_(9)                                               /**< (I2S_INTENSET) Transmit Ready 1 Interrupt Enable Position */
#define I2S_INTENSET_TXRDY1_Msk               (_U_(0x1) << I2S_INTENSET_TXRDY1_Pos)                /**< (I2S_INTENSET) Transmit Ready 1 Interrupt Enable Mask */
#define I2S_INTENSET_TXRDY1(value)            (I2S_INTENSET_TXRDY1_Msk & ((value) << I2S_INTENSET_TXRDY1_Pos))
#define I2S_INTENSET_TXUR0_Pos                _U_(12)                                              /**< (I2S_INTENSET) Transmit Underrun 0 Interrupt Enable Position */
#define I2S_INTENSET_TXUR0_Msk                (_U_(0x1) << I2S_INTENSET_TXUR0_Pos)                 /**< (I2S_INTENSET) Transmit Underrun 0 Interrupt Enable Mask */
#define I2S_INTENSET_TXUR0(value)             (I2S_INTENSET_TXUR0_Msk & ((value) << I2S_INTENSET_TXUR0_Pos))
#define I2S_INTENSET_TXUR1_Pos                _U_(13)                                              /**< (I2S_INTENSET) Transmit Underrun 1 Interrupt Enable Position */
#define I2S_INTENSET_TXUR1_Msk                (_U_(0x1) << I2S_INTENSET_TXUR1_Pos)                 /**< (I2S_INTENSET) Transmit Underrun 1 Interrupt Enable Mask */
#define I2S_INTENSET_TXUR1(value)             (I2S_INTENSET_TXUR1_Msk & ((value) << I2S_INTENSET_TXUR1_Pos))
#define I2S_INTENSET_Msk                      _U_(0x3333)                                          /**< (I2S_INTENSET) Register Mask  */

#define I2S_INTENSET_RXRDY_Pos                _U_(0)                                               /**< (I2S_INTENSET Position) Receive Ready x Interrupt Enable */
#define I2S_INTENSET_RXRDY_Msk                (_U_(0x3) << I2S_INTENSET_RXRDY_Pos)                 /**< (I2S_INTENSET Mask) RXRDY */
#define I2S_INTENSET_RXRDY(value)             (I2S_INTENSET_RXRDY_Msk & ((value) << I2S_INTENSET_RXRDY_Pos)) 
#define I2S_INTENSET_RXOR_Pos                 _U_(4)                                               /**< (I2S_INTENSET Position) Receive Overrun x Interrupt Enable */
#define I2S_INTENSET_RXOR_Msk                 (_U_(0x3) << I2S_INTENSET_RXOR_Pos)                  /**< (I2S_INTENSET Mask) RXOR */
#define I2S_INTENSET_RXOR(value)              (I2S_INTENSET_RXOR_Msk & ((value) << I2S_INTENSET_RXOR_Pos)) 
#define I2S_INTENSET_TXRDY_Pos                _U_(8)                                               /**< (I2S_INTENSET Position) Transmit Ready x Interrupt Enable */
#define I2S_INTENSET_TXRDY_Msk                (_U_(0x3) << I2S_INTENSET_TXRDY_Pos)                 /**< (I2S_INTENSET Mask) TXRDY */
#define I2S_INTENSET_TXRDY(value)             (I2S_INTENSET_TXRDY_Msk & ((value) << I2S_INTENSET_TXRDY_Pos)) 
#define I2S_INTENSET_TXUR_Pos                 _U_(12)                                              /**< (I2S_INTENSET Position) Transmit Underrun x Interrupt Enable */
#define I2S_INTENSET_TXUR_Msk                 (_U_(0x3) << I2S_INTENSET_TXUR_Pos)                  /**< (I2S_INTENSET Mask) TXUR */
#define I2S_INTENSET_TXUR(value)              (I2S_INTENSET_TXUR_Msk & ((value) << I2S_INTENSET_TXUR_Pos)) 

/* -------- I2S_INTFLAG : (I2S Offset: 0x14) (R/W 16) Interrupt Flag Status and Clear -------- */
#define I2S_INTFLAG_RESETVALUE                _U_(0x00)                                            /**<  (I2S_INTFLAG) Interrupt Flag Status and Clear  Reset Value */

#define I2S_INTFLAG_RXRDY0_Pos                _U_(0)                                               /**< (I2S_INTFLAG) Receive Ready 0 Position */
#define I2S_INTFLAG_RXRDY0_Msk                (_U_(0x1) << I2S_INTFLAG_RXRDY0_Pos)                 /**< (I2S_INTFLAG) Receive Ready 0 Mask */
#define I2S_INTFLAG_RXRDY0(value)             (I2S_INTFLAG_RXRDY0_Msk & ((value) << I2S_INTFLAG_RXRDY0_Pos))
#define I2S_INTFLAG_RXRDY1_Pos                _U_(1)                                               /**< (I2S_INTFLAG) Receive Ready 1 Position */
#define I2S_INTFLAG_RXRDY1_Msk                (_U_(0x1) << I2S_INTFLAG_RXRDY1_Pos)                 /**< (I2S_INTFLAG) Receive Ready 1 Mask */
#define I2S_INTFLAG_RXRDY1(value)             (I2S_INTFLAG_RXRDY1_Msk & ((value) << I2S_INTFLAG_RXRDY1_Pos))
#define I2S_INTFLAG_RXOR0_Pos                 _U_(4)                                               /**< (I2S_INTFLAG) Receive Overrun 0 Position */
#define I2S_INTFLAG_RXOR0_Msk                 (_U_(0x1) << I2S_INTFLAG_RXOR0_Pos)                  /**< (I2S_INTFLAG) Receive Overrun 0 Mask */
#define I2S_INTFLAG_RXOR0(value)              (I2S_INTFLAG_RXOR0_Msk & ((value) << I2S_INTFLAG_RXOR0_Pos))
#define I2S_INTFLAG_RXOR1_Pos                 _U_(5)                                               /**< (I2S_INTFLAG) Receive Overrun 1 Position */
#define I2S_INTFLAG_RXOR1_Msk                 (_U_(0x1) << I2S_INTFLAG_RXOR1_Pos)                  /**< (I2S_INTFLAG) Receive Overrun 1 Mask */
#define I2S_INTFLAG_RXOR1(value)              (I2S_INTFLAG_RXOR1_Msk & ((value) << I2S_INTFLAG_RXOR1_Pos))
#define I2S_INTFLAG_TXRDY0_Pos                _U_(8)                                               /**< (I2S_INTFLAG) Transmit Ready 0 Position */
#define I2S_INTFLAG_TXRDY0_Msk                (_U_(0x1) << I2S_INTFLAG_TXRDY0_Pos)                 /**< (I2S_INTFLAG) Transmit Ready 0 Mask */
#define I2S_INTFLAG_TXRDY0(value)             (I2S_INTFLAG_TXRDY0_Msk & ((value) << I2S_INTFLAG_TXRDY0_Pos))
#define I2S_INTFLAG_TXRDY1_Pos                _U_(9)                                               /**< (I2S_INTFLAG) Transmit Ready 1 Position */
#define I2S_INTFLAG_TXRDY1_Msk                (_U_(0x1) << I2S_INTFLAG_TXRDY1_Pos)                 /**< (I2S_INTFLAG) Transmit Ready 1 Mask */
#define I2S_INTFLAG_TXRDY1(value)             (I2S_INTFLAG_TXRDY1_Msk & ((value) << I2S_INTFLAG_TXRDY1_Pos))
#define I2S_INTFLAG_TXUR0_Pos                 _U_(12)                                              /**< (I2S_INTFLAG) Transmit Underrun 0 Position */
#define I2S_INTFLAG_TXUR0_Msk                 (_U_(0x1) << I2S_INTFLAG_TXUR0_Pos)                  /**< (I2S_INTFLAG) Transmit Underrun 0 Mask */
#define I2S_INTFLAG_TXUR0(value)              (I2S_INTFLAG_TXUR0_Msk & ((value) << I2S_INTFLAG_TXUR0_Pos))
#define I2S_INTFLAG_TXUR1_Pos                 _U_(13)                                              /**< (I2S_INTFLAG) Transmit Underrun 1 Position */
#define I2S_INTFLAG_TXUR1_Msk                 (_U_(0x1) << I2S_INTFLAG_TXUR1_Pos)                  /**< (I2S_INTFLAG) Transmit Underrun 1 Mask */
#define I2S_INTFLAG_TXUR1(value)              (I2S_INTFLAG_TXUR1_Msk & ((value) << I2S_INTFLAG_TXUR1_Pos))
#define I2S_INTFLAG_Msk                       _U_(0x3333)                                          /**< (I2S_INTFLAG) Register Mask  */

#define I2S_INTFLAG_RXRDY_Pos                 _U_(0)                                               /**< (I2S_INTFLAG Position) Receive Ready x */
#define I2S_INTFLAG_RXRDY_Msk                 (_U_(0x3) << I2S_INTFLAG_RXRDY_Pos)                  /**< (I2S_INTFLAG Mask) RXRDY */
#define I2S_INTFLAG_RXRDY(value)              (I2S_INTFLAG_RXRDY_Msk & ((value) << I2S_INTFLAG_RXRDY_Pos)) 
#define I2S_INTFLAG_RXOR_Pos                  _U_(4)                                               /**< (I2S_INTFLAG Position) Receive Overrun x */
#define I2S_INTFLAG_RXOR_Msk                  (_U_(0x3) << I2S_INTFLAG_RXOR_Pos)                   /**< (I2S_INTFLAG Mask) RXOR */
#define I2S_INTFLAG_RXOR(value)               (I2S_INTFLAG_RXOR_Msk & ((value) << I2S_INTFLAG_RXOR_Pos)) 
#define I2S_INTFLAG_TXRDY_Pos                 _U_(8)                                               /**< (I2S_INTFLAG Position) Transmit Ready x */
#define I2S_INTFLAG_TXRDY_Msk                 (_U_(0x3) << I2S_INTFLAG_TXRDY_Pos)                  /**< (I2S_INTFLAG Mask) TXRDY */
#define I2S_INTFLAG_TXRDY(value)              (I2S_INTFLAG_TXRDY_Msk & ((value) << I2S_INTFLAG_TXRDY_Pos)) 
#define I2S_INTFLAG_TXUR_Pos                  _U_(12)                                              /**< (I2S_INTFLAG Position) Transmit Underrun x */
#define I2S_INTFLAG_TXUR_Msk                  (_U_(0x3) << I2S_INTFLAG_TXUR_Pos)                   /**< (I2S_INTFLAG Mask) TXUR */
#define I2S_INTFLAG_TXUR(value)               (I2S_INTFLAG_TXUR_Msk & ((value) << I2S_INTFLAG_TXUR_Pos)) 

/* -------- I2S_SYNCBUSY : (I2S Offset: 0x18) ( R/ 16) Synchronization Status -------- */
#define I2S_SYNCBUSY_RESETVALUE               _U_(0x00)                                            /**<  (I2S_SYNCBUSY) Synchronization Status  Reset Value */

#define I2S_SYNCBUSY_SWRST_Pos                _U_(0)                                               /**< (I2S_SYNCBUSY) Software Reset Synchronization Status Position */
#define I2S_SYNCBUSY_SWRST_Msk                (_U_(0x1) << I2S_SYNCBUSY_SWRST_Pos)                 /**< (I2S_SYNCBUSY) Software Reset Synchronization Status Mask */
#define I2S_SYNCBUSY_SWRST(value)             (I2S_SYNCBUSY_SWRST_Msk & ((value) << I2S_SYNCBUSY_SWRST_Pos))
#define I2S_SYNCBUSY_ENABLE_Pos               _U_(1)                                               /**< (I2S_SYNCBUSY) Enable Synchronization Status Position */
#define I2S_SYNCBUSY_ENABLE_Msk               (_U_(0x1) << I2S_SYNCBUSY_ENABLE_Pos)                /**< (I2S_SYNCBUSY) Enable Synchronization Status Mask */
#define I2S_SYNCBUSY_ENABLE(value)            (I2S_SYNCBUSY_ENABLE_Msk & ((value) << I2S_SYNCBUSY_ENABLE_Pos))
#define I2S_SYNCBUSY_CKEN0_Pos                _U_(2)                                               /**< (I2S_SYNCBUSY) Clock Unit 0 Enable Synchronization Status Position */
#define I2S_SYNCBUSY_CKEN0_Msk                (_U_(0x1) << I2S_SYNCBUSY_CKEN0_Pos)                 /**< (I2S_SYNCBUSY) Clock Unit 0 Enable Synchronization Status Mask */
#define I2S_SYNCBUSY_CKEN0(value)             (I2S_SYNCBUSY_CKEN0_Msk & ((value) << I2S_SYNCBUSY_CKEN0_Pos))
#define I2S_SYNCBUSY_CKEN1_Pos                _U_(3)                                               /**< (I2S_SYNCBUSY) Clock Unit 1 Enable Synchronization Status Position */
#define I2S_SYNCBUSY_CKEN1_Msk                (_U_(0x1) << I2S_SYNCBUSY_CKEN1_Pos)                 /**< (I2S_SYNCBUSY) Clock Unit 1 Enable Synchronization Status Mask */
#define I2S_SYNCBUSY_CKEN1(value)             (I2S_SYNCBUSY_CKEN1_Msk & ((value) << I2S_SYNCBUSY_CKEN1_Pos))
#define I2S_SYNCBUSY_SEREN0_Pos               _U_(4)                                               /**< (I2S_SYNCBUSY) Serializer 0 Enable Synchronization Status Position */
#define I2S_SYNCBUSY_SEREN0_Msk               (_U_(0x1) << I2S_SYNCBUSY_SEREN0_Pos)                /**< (I2S_SYNCBUSY) Serializer 0 Enable Synchronization Status Mask */
#define I2S_SYNCBUSY_SEREN0(value)            (I2S_SYNCBUSY_SEREN0_Msk & ((value) << I2S_SYNCBUSY_SEREN0_Pos))
#define I2S_SYNCBUSY_SEREN1_Pos               _U_(5)                                               /**< (I2S_SYNCBUSY) Serializer 1 Enable Synchronization Status Position */
#define I2S_SYNCBUSY_SEREN1_Msk               (_U_(0x1) << I2S_SYNCBUSY_SEREN1_Pos)                /**< (I2S_SYNCBUSY) Serializer 1 Enable Synchronization Status Mask */
#define I2S_SYNCBUSY_SEREN1(value)            (I2S_SYNCBUSY_SEREN1_Msk & ((value) << I2S_SYNCBUSY_SEREN1_Pos))
#define I2S_SYNCBUSY_DATA0_Pos                _U_(8)                                               /**< (I2S_SYNCBUSY) Data 0 Synchronization Status Position */
#define I2S_SYNCBUSY_DATA0_Msk                (_U_(0x1) << I2S_SYNCBUSY_DATA0_Pos)                 /**< (I2S_SYNCBUSY) Data 0 Synchronization Status Mask */
#define I2S_SYNCBUSY_DATA0(value)             (I2S_SYNCBUSY_DATA0_Msk & ((value) << I2S_SYNCBUSY_DATA0_Pos))
#define I2S_SYNCBUSY_DATA1_Pos                _U_(9)                                               /**< (I2S_SYNCBUSY) Data 1 Synchronization Status Position */
#define I2S_SYNCBUSY_DATA1_Msk                (_U_(0x1) << I2S_SYNCBUSY_DATA1_Pos)                 /**< (I2S_SYNCBUSY) Data 1 Synchronization Status Mask */
#define I2S_SYNCBUSY_DATA1(value)             (I2S_SYNCBUSY_DATA1_Msk & ((value) << I2S_SYNCBUSY_DATA1_Pos))
#define I2S_SYNCBUSY_Msk                      _U_(0x033F)                                          /**< (I2S_SYNCBUSY) Register Mask  */

#define I2S_SYNCBUSY_CKEN_Pos                 _U_(2)                                               /**< (I2S_SYNCBUSY Position) Clock Unit x Enable Synchronization Status */
#define I2S_SYNCBUSY_CKEN_Msk                 (_U_(0x3) << I2S_SYNCBUSY_CKEN_Pos)                  /**< (I2S_SYNCBUSY Mask) CKEN */
#define I2S_SYNCBUSY_CKEN(value)              (I2S_SYNCBUSY_CKEN_Msk & ((value) << I2S_SYNCBUSY_CKEN_Pos)) 
#define I2S_SYNCBUSY_SEREN_Pos                _U_(4)                                               /**< (I2S_SYNCBUSY Position) Serializer x Enable Synchronization Status */
#define I2S_SYNCBUSY_SEREN_Msk                (_U_(0x3) << I2S_SYNCBUSY_SEREN_Pos)                 /**< (I2S_SYNCBUSY Mask) SEREN */
#define I2S_SYNCBUSY_SEREN(value)             (I2S_SYNCBUSY_SEREN_Msk & ((value) << I2S_SYNCBUSY_SEREN_Pos)) 
#define I2S_SYNCBUSY_DATA_Pos                 _U_(8)                                               /**< (I2S_SYNCBUSY Position) Data x Synchronization Status */
#define I2S_SYNCBUSY_DATA_Msk                 (_U_(0x3) << I2S_SYNCBUSY_DATA_Pos)                  /**< (I2S_SYNCBUSY Mask) DATA */
#define I2S_SYNCBUSY_DATA(value)              (I2S_SYNCBUSY_DATA_Msk & ((value) << I2S_SYNCBUSY_DATA_Pos)) 

/* -------- I2S_SERCTRL : (I2S Offset: 0x20) (R/W 32) Serializer n Control -------- */
#define I2S_SERCTRL_RESETVALUE                _U_(0x00)                                            /**<  (I2S_SERCTRL) Serializer n Control  Reset Value */

#define I2S_SERCTRL_SERMODE_Pos               _U_(0)                                               /**< (I2S_SERCTRL) Serializer Mode Position */
#define I2S_SERCTRL_SERMODE_Msk               (_U_(0x3) << I2S_SERCTRL_SERMODE_Pos)                /**< (I2S_SERCTRL) Serializer Mode Mask */
#define I2S_SERCTRL_SERMODE(value)            (I2S_SERCTRL_SERMODE_Msk & ((value) << I2S_SERCTRL_SERMODE_Pos))
#define   I2S_SERCTRL_SERMODE_RX_Val          _U_(0x0)                                             /**< (I2S_SERCTRL) Receive  */
#define   I2S_SERCTRL_SERMODE_TX_Val          _U_(0x1)                                             /**< (I2S_SERCTRL) Transmit  */
#define   I2S_SERCTRL_SERMODE_PDM2_Val        _U_(0x2)                                             /**< (I2S_SERCTRL) Receive one PDM data on each serial clock edge  */
#define I2S_SERCTRL_SERMODE_RX                (I2S_SERCTRL_SERMODE_RX_Val << I2S_SERCTRL_SERMODE_Pos) /**< (I2S_SERCTRL) Receive Position  */
#define I2S_SERCTRL_SERMODE_TX                (I2S_SERCTRL_SERMODE_TX_Val << I2S_SERCTRL_SERMODE_Pos) /**< (I2S_SERCTRL) Transmit Position  */
#define I2S_SERCTRL_SERMODE_PDM2              (I2S_SERCTRL_SERMODE_PDM2_Val << I2S_SERCTRL_SERMODE_Pos) /**< (I2S_SERCTRL) Receive one PDM data on each serial clock edge Position  */
#define I2S_SERCTRL_TXDEFAULT_Pos             _U_(2)                                               /**< (I2S_SERCTRL) Line Default Line when Slot Disabled Position */
#define I2S_SERCTRL_TXDEFAULT_Msk             (_U_(0x3) << I2S_SERCTRL_TXDEFAULT_Pos)              /**< (I2S_SERCTRL) Line Default Line when Slot Disabled Mask */
#define I2S_SERCTRL_TXDEFAULT(value)          (I2S_SERCTRL_TXDEFAULT_Msk & ((value) << I2S_SERCTRL_TXDEFAULT_Pos))
#define   I2S_SERCTRL_TXDEFAULT_ZERO_Val      _U_(0x0)                                             /**< (I2S_SERCTRL) Output Default Value is 0  */
#define   I2S_SERCTRL_TXDEFAULT_ONE_Val       _U_(0x1)                                             /**< (I2S_SERCTRL) Output Default Value is 1  */
#define   I2S_SERCTRL_TXDEFAULT_HIZ_Val       _U_(0x3)                                             /**< (I2S_SERCTRL) Output Default Value is high impedance  */
#define I2S_SERCTRL_TXDEFAULT_ZERO            (I2S_SERCTRL_TXDEFAULT_ZERO_Val << I2S_SERCTRL_TXDEFAULT_Pos) /**< (I2S_SERCTRL) Output Default Value is 0 Position  */
#define I2S_SERCTRL_TXDEFAULT_ONE             (I2S_SERCTRL_TXDEFAULT_ONE_Val << I2S_SERCTRL_TXDEFAULT_Pos) /**< (I2S_SERCTRL) Output Default Value is 1 Position  */
#define I2S_SERCTRL_TXDEFAULT_HIZ             (I2S_SERCTRL_TXDEFAULT_HIZ_Val << I2S_SERCTRL_TXDEFAULT_Pos) /**< (I2S_SERCTRL) Output Default Value is high impedance Position  */
#define I2S_SERCTRL_TXSAME_Pos                _U_(4)                                               /**< (I2S_SERCTRL) Transmit Data when Underrun Position */
#define I2S_SERCTRL_TXSAME_Msk                (_U_(0x1) << I2S_SERCTRL_TXSAME_Pos)                 /**< (I2S_SERCTRL) Transmit Data when Underrun Mask */
#define I2S_SERCTRL_TXSAME(value)             (I2S_SERCTRL_TXSAME_Msk & ((value) << I2S_SERCTRL_TXSAME_Pos))
#define   I2S_SERCTRL_TXSAME_ZERO_Val         _U_(0x0)                                             /**< (I2S_SERCTRL) Zero data transmitted in case of underrun  */
#define   I2S_SERCTRL_TXSAME_SAME_Val         _U_(0x1)                                             /**< (I2S_SERCTRL) Last data transmitted in case of underrun  */
#define I2S_SERCTRL_TXSAME_ZERO               (I2S_SERCTRL_TXSAME_ZERO_Val << I2S_SERCTRL_TXSAME_Pos) /**< (I2S_SERCTRL) Zero data transmitted in case of underrun Position  */
#define I2S_SERCTRL_TXSAME_SAME               (I2S_SERCTRL_TXSAME_SAME_Val << I2S_SERCTRL_TXSAME_Pos) /**< (I2S_SERCTRL) Last data transmitted in case of underrun Position  */
#define I2S_SERCTRL_CLKSEL_Pos                _U_(5)                                               /**< (I2S_SERCTRL) Clock Unit Selection Position */
#define I2S_SERCTRL_CLKSEL_Msk                (_U_(0x1) << I2S_SERCTRL_CLKSEL_Pos)                 /**< (I2S_SERCTRL) Clock Unit Selection Mask */
#define I2S_SERCTRL_CLKSEL(value)             (I2S_SERCTRL_CLKSEL_Msk & ((value) << I2S_SERCTRL_CLKSEL_Pos))
#define   I2S_SERCTRL_CLKSEL_CLK0_Val         _U_(0x0)                                             /**< (I2S_SERCTRL) Use Clock Unit 0  */
#define   I2S_SERCTRL_CLKSEL_CLK1_Val         _U_(0x1)                                             /**< (I2S_SERCTRL) Use Clock Unit 1  */
#define I2S_SERCTRL_CLKSEL_CLK0               (I2S_SERCTRL_CLKSEL_CLK0_Val << I2S_SERCTRL_CLKSEL_Pos) /**< (I2S_SERCTRL) Use Clock Unit 0 Position  */
#define I2S_SERCTRL_CLKSEL_CLK1               (I2S_SERCTRL_CLKSEL_CLK1_Val << I2S_SERCTRL_CLKSEL_Pos) /**< (I2S_SERCTRL) Use Clock Unit 1 Position  */
#define I2S_SERCTRL_SLOTADJ_Pos               _U_(7)                                               /**< (I2S_SERCTRL) Data Slot Formatting Adjust Position */
#define I2S_SERCTRL_SLOTADJ_Msk               (_U_(0x1) << I2S_SERCTRL_SLOTADJ_Pos)                /**< (I2S_SERCTRL) Data Slot Formatting Adjust Mask */
#define I2S_SERCTRL_SLOTADJ(value)            (I2S_SERCTRL_SLOTADJ_Msk & ((value) << I2S_SERCTRL_SLOTADJ_Pos))
#define   I2S_SERCTRL_SLOTADJ_RIGHT_Val       _U_(0x0)                                             /**< (I2S_SERCTRL) Data is right adjusted in slot  */
#define   I2S_SERCTRL_SLOTADJ_LEFT_Val        _U_(0x1)                                             /**< (I2S_SERCTRL) Data is left adjusted in slot  */
#define I2S_SERCTRL_SLOTADJ_RIGHT             (I2S_SERCTRL_SLOTADJ_RIGHT_Val << I2S_SERCTRL_SLOTADJ_Pos) /**< (I2S_SERCTRL) Data is right adjusted in slot Position  */
#define I2S_SERCTRL_SLOTADJ_LEFT              (I2S_SERCTRL_SLOTADJ_LEFT_Val << I2S_SERCTRL_SLOTADJ_Pos) /**< (I2S_SERCTRL) Data is left adjusted in slot Position  */
#define I2S_SERCTRL_DATASIZE_Pos              _U_(8)                                               /**< (I2S_SERCTRL) Data Word Size Position */
#define I2S_SERCTRL_DATASIZE_Msk              (_U_(0x7) << I2S_SERCTRL_DATASIZE_Pos)               /**< (I2S_SERCTRL) Data Word Size Mask */
#define I2S_SERCTRL_DATASIZE(value)           (I2S_SERCTRL_DATASIZE_Msk & ((value) << I2S_SERCTRL_DATASIZE_Pos))
#define   I2S_SERCTRL_DATASIZE_32_Val         _U_(0x0)                                             /**< (I2S_SERCTRL) 32 bits  */
#define   I2S_SERCTRL_DATASIZE_24_Val         _U_(0x1)                                             /**< (I2S_SERCTRL) 24 bits  */
#define   I2S_SERCTRL_DATASIZE_20_Val         _U_(0x2)                                             /**< (I2S_SERCTRL) 20 bits  */
#define   I2S_SERCTRL_DATASIZE_18_Val         _U_(0x3)                                             /**< (I2S_SERCTRL) 18 bits  */
#define   I2S_SERCTRL_DATASIZE_16_Val         _U_(0x4)                                             /**< (I2S_SERCTRL) 16 bits  */
#define   I2S_SERCTRL_DATASIZE_16C_Val        _U_(0x5)                                             /**< (I2S_SERCTRL) 16 bits compact stereo  */
#define   I2S_SERCTRL_DATASIZE_8_Val          _U_(0x6)                                             /**< (I2S_SERCTRL) 8 bits  */
#define   I2S_SERCTRL_DATASIZE_8C_Val         _U_(0x7)                                             /**< (I2S_SERCTRL) 8 bits compact stereo  */
#define I2S_SERCTRL_DATASIZE_32               (I2S_SERCTRL_DATASIZE_32_Val << I2S_SERCTRL_DATASIZE_Pos) /**< (I2S_SERCTRL) 32 bits Position  */
#define I2S_SERCTRL_DATASIZE_24               (I2S_SERCTRL_DATASIZE_24_Val << I2S_SERCTRL_DATASIZE_Pos) /**< (I2S_SERCTRL) 24 bits Position  */
#define I2S_SERCTRL_DATASIZE_20               (I2S_SERCTRL_DATASIZE_20_Val << I2S_SERCTRL_DATASIZE_Pos) /**< (I2S_SERCTRL) 20 bits Position  */
#define I2S_SERCTRL_DATASIZE_18               (I2S_SERCTRL_DATASIZE_18_Val << I2S_SERCTRL_DATASIZE_Pos) /**< (I2S_SERCTRL) 18 bits Position  */
#define I2S_SERCTRL_DATASIZE_16               (I2S_SERCTRL_DATASIZE_16_Val << I2S_SERCTRL_DATASIZE_Pos) /**< (I2S_SERCTRL) 16 bits Position  */
#define I2S_SERCTRL_DATASIZE_16C              (I2S_SERCTRL_DATASIZE_16C_Val << I2S_SERCTRL_DATASIZE_Pos) /**< (I2S_SERCTRL) 16 bits compact stereo Position  */
#define I2S_SERCTRL_DATASIZE_8                (I2S_SERCTRL_DATASIZE_8_Val << I2S_SERCTRL_DATASIZE_Pos) /**< (I2S_SERCTRL) 8 bits Position  */
#define I2S_SERCTRL_DATASIZE_8C               (I2S_SERCTRL_DATASIZE_8C_Val << I2S_SERCTRL_DATASIZE_Pos) /**< (I2S_SERCTRL) 8 bits compact stereo Position  */
#define I2S_SERCTRL_WORDADJ_Pos               _U_(12)                                              /**< (I2S_SERCTRL) Data Word Formatting Adjust Position */
#define I2S_SERCTRL_WORDADJ_Msk               (_U_(0x1) << I2S_SERCTRL_WORDADJ_Pos)                /**< (I2S_SERCTRL) Data Word Formatting Adjust Mask */
#define I2S_SERCTRL_WORDADJ(value)            (I2S_SERCTRL_WORDADJ_Msk & ((value) << I2S_SERCTRL_WORDADJ_Pos))
#define   I2S_SERCTRL_WORDADJ_RIGHT_Val       _U_(0x0)                                             /**< (I2S_SERCTRL) Data is right adjusted in word  */
#define   I2S_SERCTRL_WORDADJ_LEFT_Val        _U_(0x1)                                             /**< (I2S_SERCTRL) Data is left adjusted in word  */
#define I2S_SERCTRL_WORDADJ_RIGHT             (I2S_SERCTRL_WORDADJ_RIGHT_Val << I2S_SERCTRL_WORDADJ_Pos) /**< (I2S_SERCTRL) Data is right adjusted in word Position  */
#define I2S_SERCTRL_WORDADJ_LEFT              (I2S_SERCTRL_WORDADJ_LEFT_Val << I2S_SERCTRL_WORDADJ_Pos) /**< (I2S_SERCTRL) Data is left adjusted in word Position  */
#define I2S_SERCTRL_EXTEND_Pos                _U_(13)                                              /**< (I2S_SERCTRL) Data Formatting Bit Extension Position */
#define I2S_SERCTRL_EXTEND_Msk                (_U_(0x3) << I2S_SERCTRL_EXTEND_Pos)                 /**< (I2S_SERCTRL) Data Formatting Bit Extension Mask */
#define I2S_SERCTRL_EXTEND(value)             (I2S_SERCTRL_EXTEND_Msk & ((value) << I2S_SERCTRL_EXTEND_Pos))
#define   I2S_SERCTRL_EXTEND_ZERO_Val         _U_(0x0)                                             /**< (I2S_SERCTRL) Extend with zeroes  */
#define   I2S_SERCTRL_EXTEND_ONE_Val          _U_(0x1)                                             /**< (I2S_SERCTRL) Extend with ones  */
#define   I2S_SERCTRL_EXTEND_MSBIT_Val        _U_(0x2)                                             /**< (I2S_SERCTRL) Extend with Most Significant Bit  */
#define   I2S_SERCTRL_EXTEND_LSBIT_Val        _U_(0x3)                                             /**< (I2S_SERCTRL) Extend with Least Significant Bit  */
#define I2S_SERCTRL_EXTEND_ZERO               (I2S_SERCTRL_EXTEND_ZERO_Val << I2S_SERCTRL_EXTEND_Pos) /**< (I2S_SERCTRL) Extend with zeroes Position  */
#define I2S_SERCTRL_EXTEND_ONE                (I2S_SERCTRL_EXTEND_ONE_Val << I2S_SERCTRL_EXTEND_Pos) /**< (I2S_SERCTRL) Extend with ones Position  */
#define I2S_SERCTRL_EXTEND_MSBIT              (I2S_SERCTRL_EXTEND_MSBIT_Val << I2S_SERCTRL_EXTEND_Pos) /**< (I2S_SERCTRL) Extend with Most Significant Bit Position  */
#define I2S_SERCTRL_EXTEND_LSBIT              (I2S_SERCTRL_EXTEND_LSBIT_Val << I2S_SERCTRL_EXTEND_Pos) /**< (I2S_SERCTRL) Extend with Least Significant Bit Position  */
#define I2S_SERCTRL_BITREV_Pos                _U_(15)                                              /**< (I2S_SERCTRL) Data Formatting Bit Reverse Position */
#define I2S_SERCTRL_BITREV_Msk                (_U_(0x1) << I2S_SERCTRL_BITREV_Pos)                 /**< (I2S_SERCTRL) Data Formatting Bit Reverse Mask */
#define I2S_SERCTRL_BITREV(value)             (I2S_SERCTRL_BITREV_Msk & ((value) << I2S_SERCTRL_BITREV_Pos))
#define   I2S_SERCTRL_BITREV_MSBIT_Val        _U_(0x0)                                             /**< (I2S_SERCTRL) Transfer Data Most Significant Bit (MSB) first (default for I2S protocol)  */
#define   I2S_SERCTRL_BITREV_LSBIT_Val        _U_(0x1)                                             /**< (I2S_SERCTRL) Transfer Data Least Significant Bit (LSB) first  */
#define I2S_SERCTRL_BITREV_MSBIT              (I2S_SERCTRL_BITREV_MSBIT_Val << I2S_SERCTRL_BITREV_Pos) /**< (I2S_SERCTRL) Transfer Data Most Significant Bit (MSB) first (default for I2S protocol) Position  */
#define I2S_SERCTRL_BITREV_LSBIT              (I2S_SERCTRL_BITREV_LSBIT_Val << I2S_SERCTRL_BITREV_Pos) /**< (I2S_SERCTRL) Transfer Data Least Significant Bit (LSB) first Position  */
#define I2S_SERCTRL_SLOTDIS0_Pos              _U_(16)                                              /**< (I2S_SERCTRL) Slot 0 Disabled for this Serializer Position */
#define I2S_SERCTRL_SLOTDIS0_Msk              (_U_(0x1) << I2S_SERCTRL_SLOTDIS0_Pos)               /**< (I2S_SERCTRL) Slot 0 Disabled for this Serializer Mask */
#define I2S_SERCTRL_SLOTDIS0(value)           (I2S_SERCTRL_SLOTDIS0_Msk & ((value) << I2S_SERCTRL_SLOTDIS0_Pos))
#define I2S_SERCTRL_SLOTDIS1_Pos              _U_(17)                                              /**< (I2S_SERCTRL) Slot 1 Disabled for this Serializer Position */
#define I2S_SERCTRL_SLOTDIS1_Msk              (_U_(0x1) << I2S_SERCTRL_SLOTDIS1_Pos)               /**< (I2S_SERCTRL) Slot 1 Disabled for this Serializer Mask */
#define I2S_SERCTRL_SLOTDIS1(value)           (I2S_SERCTRL_SLOTDIS1_Msk & ((value) << I2S_SERCTRL_SLOTDIS1_Pos))
#define I2S_SERCTRL_SLOTDIS2_Pos              _U_(18)                                              /**< (I2S_SERCTRL) Slot 2 Disabled for this Serializer Position */
#define I2S_SERCTRL_SLOTDIS2_Msk              (_U_(0x1) << I2S_SERCTRL_SLOTDIS2_Pos)               /**< (I2S_SERCTRL) Slot 2 Disabled for this Serializer Mask */
#define I2S_SERCTRL_SLOTDIS2(value)           (I2S_SERCTRL_SLOTDIS2_Msk & ((value) << I2S_SERCTRL_SLOTDIS2_Pos))
#define I2S_SERCTRL_SLOTDIS3_Pos              _U_(19)                                              /**< (I2S_SERCTRL) Slot 3 Disabled for this Serializer Position */
#define I2S_SERCTRL_SLOTDIS3_Msk              (_U_(0x1) << I2S_SERCTRL_SLOTDIS3_Pos)               /**< (I2S_SERCTRL) Slot 3 Disabled for this Serializer Mask */
#define I2S_SERCTRL_SLOTDIS3(value)           (I2S_SERCTRL_SLOTDIS3_Msk & ((value) << I2S_SERCTRL_SLOTDIS3_Pos))
#define I2S_SERCTRL_SLOTDIS4_Pos              _U_(20)                                              /**< (I2S_SERCTRL) Slot 4 Disabled for this Serializer Position */
#define I2S_SERCTRL_SLOTDIS4_Msk              (_U_(0x1) << I2S_SERCTRL_SLOTDIS4_Pos)               /**< (I2S_SERCTRL) Slot 4 Disabled for this Serializer Mask */
#define I2S_SERCTRL_SLOTDIS4(value)           (I2S_SERCTRL_SLOTDIS4_Msk & ((value) << I2S_SERCTRL_SLOTDIS4_Pos))
#define I2S_SERCTRL_SLOTDIS5_Pos              _U_(21)                                              /**< (I2S_SERCTRL) Slot 5 Disabled for this Serializer Position */
#define I2S_SERCTRL_SLOTDIS5_Msk              (_U_(0x1) << I2S_SERCTRL_SLOTDIS5_Pos)               /**< (I2S_SERCTRL) Slot 5 Disabled for this Serializer Mask */
#define I2S_SERCTRL_SLOTDIS5(value)           (I2S_SERCTRL_SLOTDIS5_Msk & ((value) << I2S_SERCTRL_SLOTDIS5_Pos))
#define I2S_SERCTRL_SLOTDIS6_Pos              _U_(22)                                              /**< (I2S_SERCTRL) Slot 6 Disabled for this Serializer Position */
#define I2S_SERCTRL_SLOTDIS6_Msk              (_U_(0x1) << I2S_SERCTRL_SLOTDIS6_Pos)               /**< (I2S_SERCTRL) Slot 6 Disabled for this Serializer Mask */
#define I2S_SERCTRL_SLOTDIS6(value)           (I2S_SERCTRL_SLOTDIS6_Msk & ((value) << I2S_SERCTRL_SLOTDIS6_Pos))
#define I2S_SERCTRL_SLOTDIS7_Pos              _U_(23)                                              /**< (I2S_SERCTRL) Slot 7 Disabled for this Serializer Position */
#define I2S_SERCTRL_SLOTDIS7_Msk              (_U_(0x1) << I2S_SERCTRL_SLOTDIS7_Pos)               /**< (I2S_SERCTRL) Slot 7 Disabled for this Serializer Mask */
#define I2S_SERCTRL_SLOTDIS7(value)           (I2S_SERCTRL_SLOTDIS7_Msk & ((value) << I2S_SERCTRL_SLOTDIS7_Pos))
#define I2S_SERCTRL_MONO_Pos                  _U_(24)                                              /**< (I2S_SERCTRL) Mono Mode Position */
#define I2S_SERCTRL_MONO_Msk                  (_U_(0x1) << I2S_SERCTRL_MONO_Pos)                   /**< (I2S_SERCTRL) Mono Mode Mask */
#define I2S_SERCTRL_MONO(value)               (I2S_SERCTRL_MONO_Msk & ((value) << I2S_SERCTRL_MONO_Pos))
#define   I2S_SERCTRL_MONO_STEREO_Val         _U_(0x0)                                             /**< (I2S_SERCTRL) Normal mode  */
#define   I2S_SERCTRL_MONO_MONO_Val           _U_(0x1)                                             /**< (I2S_SERCTRL) Left channel data is duplicated to right channel  */
#define I2S_SERCTRL_MONO_STEREO               (I2S_SERCTRL_MONO_STEREO_Val << I2S_SERCTRL_MONO_Pos) /**< (I2S_SERCTRL) Normal mode Position  */
#define I2S_SERCTRL_MONO_MONO                 (I2S_SERCTRL_MONO_MONO_Val << I2S_SERCTRL_MONO_Pos)  /**< (I2S_SERCTRL) Left channel data is duplicated to right channel Position  */
#define I2S_SERCTRL_DMA_Pos                   _U_(25)                                              /**< (I2S_SERCTRL) Single or Multiple DMA Channels Position */
#define I2S_SERCTRL_DMA_Msk                   (_U_(0x1) << I2S_SERCTRL_DMA_Pos)                    /**< (I2S_SERCTRL) Single or Multiple DMA Channels Mask */
#define I2S_SERCTRL_DMA(value)                (I2S_SERCTRL_DMA_Msk & ((value) << I2S_SERCTRL_DMA_Pos))
#define   I2S_SERCTRL_DMA_SINGLE_Val          _U_(0x0)                                             /**< (I2S_SERCTRL) Single DMA channel  */
#define   I2S_SERCTRL_DMA_MULTIPLE_Val        _U_(0x1)                                             /**< (I2S_SERCTRL) One DMA channel per data channel  */
#define I2S_SERCTRL_DMA_SINGLE                (I2S_SERCTRL_DMA_SINGLE_Val << I2S_SERCTRL_DMA_Pos)  /**< (I2S_SERCTRL) Single DMA channel Position  */
#define I2S_SERCTRL_DMA_MULTIPLE              (I2S_SERCTRL_DMA_MULTIPLE_Val << I2S_SERCTRL_DMA_Pos) /**< (I2S_SERCTRL) One DMA channel per data channel Position  */
#define I2S_SERCTRL_RXLOOP_Pos                _U_(26)                                              /**< (I2S_SERCTRL) Loop-back Test Mode Position */
#define I2S_SERCTRL_RXLOOP_Msk                (_U_(0x1) << I2S_SERCTRL_RXLOOP_Pos)                 /**< (I2S_SERCTRL) Loop-back Test Mode Mask */
#define I2S_SERCTRL_RXLOOP(value)             (I2S_SERCTRL_RXLOOP_Msk & ((value) << I2S_SERCTRL_RXLOOP_Pos))
#define I2S_SERCTRL_Msk                       _U_(0x07FFF7BF)                                      /**< (I2S_SERCTRL) Register Mask  */

#define I2S_SERCTRL_SLOTDIS_Pos               _U_(16)                                              /**< (I2S_SERCTRL Position) Slot x Disabled for this Serializer */
#define I2S_SERCTRL_SLOTDIS_Msk               (_U_(0xFF) << I2S_SERCTRL_SLOTDIS_Pos)               /**< (I2S_SERCTRL Mask) SLOTDIS */
#define I2S_SERCTRL_SLOTDIS(value)            (I2S_SERCTRL_SLOTDIS_Msk & ((value) << I2S_SERCTRL_SLOTDIS_Pos)) 

/* -------- I2S_DATA : (I2S Offset: 0x30) (R/W 32) Data n -------- */
#define I2S_DATA_RESETVALUE                   _U_(0x00)                                            /**<  (I2S_DATA) Data n  Reset Value */

#define I2S_DATA_DATA_Pos                     _U_(0)                                               /**< (I2S_DATA) Sample Data Position */
#define I2S_DATA_DATA_Msk                     (_U_(0xFFFFFFFF) << I2S_DATA_DATA_Pos)               /**< (I2S_DATA) Sample Data Mask */
#define I2S_DATA_DATA(value)                  (I2S_DATA_DATA_Msk & ((value) << I2S_DATA_DATA_Pos))
#define I2S_DATA_Msk                          _U_(0xFFFFFFFF)                                      /**< (I2S_DATA) Register Mask  */


/** \brief I2S register offsets definitions */
#define I2S_CTRLA_REG_OFST             (0x00)              /**< (I2S_CTRLA) Control A Offset */
#define I2S_CLKCTRL_REG_OFST           (0x04)              /**< (I2S_CLKCTRL) Clock Unit n Control Offset */
#define I2S_INTENCLR_REG_OFST          (0x0C)              /**< (I2S_INTENCLR) Interrupt Enable Clear Offset */
#define I2S_INTENSET_REG_OFST          (0x10)              /**< (I2S_INTENSET) Interrupt Enable Set Offset */
#define I2S_INTFLAG_REG_OFST           (0x14)              /**< (I2S_INTFLAG) Interrupt Flag Status and Clear Offset */
#define I2S_SYNCBUSY_REG_OFST          (0x18)              /**< (I2S_SYNCBUSY) Synchronization Status Offset */
#define I2S_SERCTRL_REG_OFST           (0x20)              /**< (I2S_SERCTRL) Serializer n Control Offset */
#define I2S_DATA_REG_OFST              (0x30)              /**< (I2S_DATA) Data n Offset */

#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
/** \brief I2S register API structure */
typedef struct
{  /* Inter-IC Sound Interface */
  __IO  uint8_t                        I2S_CTRLA;          /**< Offset: 0x00 (R/W  8) Control A */
  __I   uint8_t                        Reserved1[0x03];
  __IO  uint32_t                       I2S_CLKCTRL[2];     /**< Offset: 0x04 (R/W  32) Clock Unit n Control */
  __IO  uint16_t                       I2S_INTENCLR;       /**< Offset: 0x0C (R/W  16) Interrupt Enable Clear */
  __I   uint8_t                        Reserved2[0x02];
  __IO  uint16_t                       I2S_INTENSET;       /**< Offset: 0x10 (R/W  16) Interrupt Enable Set */
  __I   uint8_t                        Reserved3[0x02];
  __IO  uint16_t                       I2S_INTFLAG;        /**< Offset: 0x14 (R/W  16) Interrupt Flag Status and Clear */
  __I   uint8_t                        Reserved4[0x02];
  __I   uint16_t                       I2S_SYNCBUSY;       /**< Offset: 0x18 (R/   16) Synchronization Status */
  __I   uint8_t                        Reserved5[0x06];
  __IO  uint32_t                       I2S_SERCTRL[2];     /**< Offset: 0x20 (R/W  32) Serializer n Control */
  __I   uint8_t                        Reserved6[0x08];
  __IO  uint32_t                       I2S_DATA[2];        /**< Offset: 0x30 (R/W  32) Data n */
} i2s_registers_t;


#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
#endif /* _SAMD21_I2S_COMPONENT_H_ */
