/**
 * \brief Component description for WDT
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
#ifndef _SAMD21_WDT_COMPONENT_H_
#define _SAMD21_WDT_COMPONENT_H_

/* ************************************************************************** */
/*   SOFTWARE API DEFINITION FOR WDT                                          */
/* ************************************************************************** */

/* -------- WDT_CTRL : (WDT Offset: 0x00) (R/W 8) Control -------- */
#define WDT_CTRL_RESETVALUE                   _U_(0x00)                                            /**<  (WDT_CTRL) Control  Reset Value */

#define WDT_CTRL_ENABLE_Pos                   _U_(1)                                               /**< (WDT_CTRL) Enable Position */
#define WDT_CTRL_ENABLE_Msk                   (_U_(0x1) << WDT_CTRL_ENABLE_Pos)                    /**< (WDT_CTRL) Enable Mask */
#define WDT_CTRL_ENABLE(value)                (WDT_CTRL_ENABLE_Msk & ((value) << WDT_CTRL_ENABLE_Pos))
#define WDT_CTRL_WEN_Pos                      _U_(2)                                               /**< (WDT_CTRL) Watchdog Timer Window Mode Enable Position */
#define WDT_CTRL_WEN_Msk                      (_U_(0x1) << WDT_CTRL_WEN_Pos)                       /**< (WDT_CTRL) Watchdog Timer Window Mode Enable Mask */
#define WDT_CTRL_WEN(value)                   (WDT_CTRL_WEN_Msk & ((value) << WDT_CTRL_WEN_Pos))  
#define WDT_CTRL_ALWAYSON_Pos                 _U_(7)                                               /**< (WDT_CTRL) Always-On Position */
#define WDT_CTRL_ALWAYSON_Msk                 (_U_(0x1) << WDT_CTRL_ALWAYSON_Pos)                  /**< (WDT_CTRL) Always-On Mask */
#define WDT_CTRL_ALWAYSON(value)              (WDT_CTRL_ALWAYSON_Msk & ((value) << WDT_CTRL_ALWAYSON_Pos))
#define WDT_CTRL_Msk                          _U_(0x86)                                            /**< (WDT_CTRL) Register Mask  */


/* -------- WDT_CONFIG : (WDT Offset: 0x01) (R/W 8) Configuration -------- */
#define WDT_CONFIG_RESETVALUE                 _U_(0xBB)                                            /**<  (WDT_CONFIG) Configuration  Reset Value */

#define WDT_CONFIG_PER_Pos                    _U_(0)                                               /**< (WDT_CONFIG) Time-Out Period Position */
#define WDT_CONFIG_PER_Msk                    (_U_(0xF) << WDT_CONFIG_PER_Pos)                     /**< (WDT_CONFIG) Time-Out Period Mask */
#define WDT_CONFIG_PER(value)                 (WDT_CONFIG_PER_Msk & ((value) << WDT_CONFIG_PER_Pos))
#define   WDT_CONFIG_PER_8_Val                _U_(0x0)                                             /**< (WDT_CONFIG) 8 clock cycles  */
#define   WDT_CONFIG_PER_16_Val               _U_(0x1)                                             /**< (WDT_CONFIG) 16 clock cycles  */
#define   WDT_CONFIG_PER_32_Val               _U_(0x2)                                             /**< (WDT_CONFIG) 32 clock cycles  */
#define   WDT_CONFIG_PER_64_Val               _U_(0x3)                                             /**< (WDT_CONFIG) 64 clock cycles  */
#define   WDT_CONFIG_PER_128_Val              _U_(0x4)                                             /**< (WDT_CONFIG) 128 clock cycles  */
#define   WDT_CONFIG_PER_256_Val              _U_(0x5)                                             /**< (WDT_CONFIG) 256 clock cycles  */
#define   WDT_CONFIG_PER_512_Val              _U_(0x6)                                             /**< (WDT_CONFIG) 512 clock cycles  */
#define   WDT_CONFIG_PER_1K_Val               _U_(0x7)                                             /**< (WDT_CONFIG) 1024 clock cycles  */
#define   WDT_CONFIG_PER_2K_Val               _U_(0x8)                                             /**< (WDT_CONFIG) 2048 clock cycles  */
#define   WDT_CONFIG_PER_4K_Val               _U_(0x9)                                             /**< (WDT_CONFIG) 4096 clock cycles  */
#define   WDT_CONFIG_PER_8K_Val               _U_(0xA)                                             /**< (WDT_CONFIG) 8192 clock cycles  */
#define   WDT_CONFIG_PER_16K_Val              _U_(0xB)                                             /**< (WDT_CONFIG) 16384 clock cycles  */
#define WDT_CONFIG_PER_8                      (WDT_CONFIG_PER_8_Val << WDT_CONFIG_PER_Pos)         /**< (WDT_CONFIG) 8 clock cycles Position  */
#define WDT_CONFIG_PER_16                     (WDT_CONFIG_PER_16_Val << WDT_CONFIG_PER_Pos)        /**< (WDT_CONFIG) 16 clock cycles Position  */
#define WDT_CONFIG_PER_32                     (WDT_CONFIG_PER_32_Val << WDT_CONFIG_PER_Pos)        /**< (WDT_CONFIG) 32 clock cycles Position  */
#define WDT_CONFIG_PER_64                     (WDT_CONFIG_PER_64_Val << WDT_CONFIG_PER_Pos)        /**< (WDT_CONFIG) 64 clock cycles Position  */
#define WDT_CONFIG_PER_128                    (WDT_CONFIG_PER_128_Val << WDT_CONFIG_PER_Pos)       /**< (WDT_CONFIG) 128 clock cycles Position  */
#define WDT_CONFIG_PER_256                    (WDT_CONFIG_PER_256_Val << WDT_CONFIG_PER_Pos)       /**< (WDT_CONFIG) 256 clock cycles Position  */
#define WDT_CONFIG_PER_512                    (WDT_CONFIG_PER_512_Val << WDT_CONFIG_PER_Pos)       /**< (WDT_CONFIG) 512 clock cycles Position  */
#define WDT_CONFIG_PER_1K                     (WDT_CONFIG_PER_1K_Val << WDT_CONFIG_PER_Pos)        /**< (WDT_CONFIG) 1024 clock cycles Position  */
#define WDT_CONFIG_PER_2K                     (WDT_CONFIG_PER_2K_Val << WDT_CONFIG_PER_Pos)        /**< (WDT_CONFIG) 2048 clock cycles Position  */
#define WDT_CONFIG_PER_4K                     (WDT_CONFIG_PER_4K_Val << WDT_CONFIG_PER_Pos)        /**< (WDT_CONFIG) 4096 clock cycles Position  */
#define WDT_CONFIG_PER_8K                     (WDT_CONFIG_PER_8K_Val << WDT_CONFIG_PER_Pos)        /**< (WDT_CONFIG) 8192 clock cycles Position  */
#define WDT_CONFIG_PER_16K                    (WDT_CONFIG_PER_16K_Val << WDT_CONFIG_PER_Pos)       /**< (WDT_CONFIG) 16384 clock cycles Position  */
#define WDT_CONFIG_WINDOW_Pos                 _U_(4)                                               /**< (WDT_CONFIG) Window Mode Time-Out Period Position */
#define WDT_CONFIG_WINDOW_Msk                 (_U_(0xF) << WDT_CONFIG_WINDOW_Pos)                  /**< (WDT_CONFIG) Window Mode Time-Out Period Mask */
#define WDT_CONFIG_WINDOW(value)              (WDT_CONFIG_WINDOW_Msk & ((value) << WDT_CONFIG_WINDOW_Pos))
#define   WDT_CONFIG_WINDOW_8_Val             _U_(0x0)                                             /**< (WDT_CONFIG) 8 clock cycles  */
#define   WDT_CONFIG_WINDOW_16_Val            _U_(0x1)                                             /**< (WDT_CONFIG) 16 clock cycles  */
#define   WDT_CONFIG_WINDOW_32_Val            _U_(0x2)                                             /**< (WDT_CONFIG) 32 clock cycles  */
#define   WDT_CONFIG_WINDOW_64_Val            _U_(0x3)                                             /**< (WDT_CONFIG) 64 clock cycles  */
#define   WDT_CONFIG_WINDOW_128_Val           _U_(0x4)                                             /**< (WDT_CONFIG) 128 clock cycles  */
#define   WDT_CONFIG_WINDOW_256_Val           _U_(0x5)                                             /**< (WDT_CONFIG) 256 clock cycles  */
#define   WDT_CONFIG_WINDOW_512_Val           _U_(0x6)                                             /**< (WDT_CONFIG) 512 clock cycles  */
#define   WDT_CONFIG_WINDOW_1K_Val            _U_(0x7)                                             /**< (WDT_CONFIG) 1024 clock cycles  */
#define   WDT_CONFIG_WINDOW_2K_Val            _U_(0x8)                                             /**< (WDT_CONFIG) 2048 clock cycles  */
#define   WDT_CONFIG_WINDOW_4K_Val            _U_(0x9)                                             /**< (WDT_CONFIG) 4096 clock cycles  */
#define   WDT_CONFIG_WINDOW_8K_Val            _U_(0xA)                                             /**< (WDT_CONFIG) 8192 clock cycles  */
#define   WDT_CONFIG_WINDOW_16K_Val           _U_(0xB)                                             /**< (WDT_CONFIG) 16384 clock cycles  */
#define WDT_CONFIG_WINDOW_8                   (WDT_CONFIG_WINDOW_8_Val << WDT_CONFIG_WINDOW_Pos)   /**< (WDT_CONFIG) 8 clock cycles Position  */
#define WDT_CONFIG_WINDOW_16                  (WDT_CONFIG_WINDOW_16_Val << WDT_CONFIG_WINDOW_Pos)  /**< (WDT_CONFIG) 16 clock cycles Position  */
#define WDT_CONFIG_WINDOW_32                  (WDT_CONFIG_WINDOW_32_Val << WDT_CONFIG_WINDOW_Pos)  /**< (WDT_CONFIG) 32 clock cycles Position  */
#define WDT_CONFIG_WINDOW_64                  (WDT_CONFIG_WINDOW_64_Val << WDT_CONFIG_WINDOW_Pos)  /**< (WDT_CONFIG) 64 clock cycles Position  */
#define WDT_CONFIG_WINDOW_128                 (WDT_CONFIG_WINDOW_128_Val << WDT_CONFIG_WINDOW_Pos) /**< (WDT_CONFIG) 128 clock cycles Position  */
#define WDT_CONFIG_WINDOW_256                 (WDT_CONFIG_WINDOW_256_Val << WDT_CONFIG_WINDOW_Pos) /**< (WDT_CONFIG) 256 clock cycles Position  */
#define WDT_CONFIG_WINDOW_512                 (WDT_CONFIG_WINDOW_512_Val << WDT_CONFIG_WINDOW_Pos) /**< (WDT_CONFIG) 512 clock cycles Position  */
#define WDT_CONFIG_WINDOW_1K                  (WDT_CONFIG_WINDOW_1K_Val << WDT_CONFIG_WINDOW_Pos)  /**< (WDT_CONFIG) 1024 clock cycles Position  */
#define WDT_CONFIG_WINDOW_2K                  (WDT_CONFIG_WINDOW_2K_Val << WDT_CONFIG_WINDOW_Pos)  /**< (WDT_CONFIG) 2048 clock cycles Position  */
#define WDT_CONFIG_WINDOW_4K                  (WDT_CONFIG_WINDOW_4K_Val << WDT_CONFIG_WINDOW_Pos)  /**< (WDT_CONFIG) 4096 clock cycles Position  */
#define WDT_CONFIG_WINDOW_8K                  (WDT_CONFIG_WINDOW_8K_Val << WDT_CONFIG_WINDOW_Pos)  /**< (WDT_CONFIG) 8192 clock cycles Position  */
#define WDT_CONFIG_WINDOW_16K                 (WDT_CONFIG_WINDOW_16K_Val << WDT_CONFIG_WINDOW_Pos) /**< (WDT_CONFIG) 16384 clock cycles Position  */
#define WDT_CONFIG_Msk                        _U_(0xFF)                                            /**< (WDT_CONFIG) Register Mask  */


/* -------- WDT_EWCTRL : (WDT Offset: 0x02) (R/W 8) Early Warning Interrupt Control -------- */
#define WDT_EWCTRL_RESETVALUE                 _U_(0x0B)                                            /**<  (WDT_EWCTRL) Early Warning Interrupt Control  Reset Value */

#define WDT_EWCTRL_EWOFFSET_Pos               _U_(0)                                               /**< (WDT_EWCTRL) Early Warning Interrupt Time Offset Position */
#define WDT_EWCTRL_EWOFFSET_Msk               (_U_(0xF) << WDT_EWCTRL_EWOFFSET_Pos)                /**< (WDT_EWCTRL) Early Warning Interrupt Time Offset Mask */
#define WDT_EWCTRL_EWOFFSET(value)            (WDT_EWCTRL_EWOFFSET_Msk & ((value) << WDT_EWCTRL_EWOFFSET_Pos))
#define   WDT_EWCTRL_EWOFFSET_8_Val           _U_(0x0)                                             /**< (WDT_EWCTRL) 8 clock cycles  */
#define   WDT_EWCTRL_EWOFFSET_16_Val          _U_(0x1)                                             /**< (WDT_EWCTRL) 16 clock cycles  */
#define   WDT_EWCTRL_EWOFFSET_32_Val          _U_(0x2)                                             /**< (WDT_EWCTRL) 32 clock cycles  */
#define   WDT_EWCTRL_EWOFFSET_64_Val          _U_(0x3)                                             /**< (WDT_EWCTRL) 64 clock cycles  */
#define   WDT_EWCTRL_EWOFFSET_128_Val         _U_(0x4)                                             /**< (WDT_EWCTRL) 128 clock cycles  */
#define   WDT_EWCTRL_EWOFFSET_256_Val         _U_(0x5)                                             /**< (WDT_EWCTRL) 256 clock cycles  */
#define   WDT_EWCTRL_EWOFFSET_512_Val         _U_(0x6)                                             /**< (WDT_EWCTRL) 512 clock cycles  */
#define   WDT_EWCTRL_EWOFFSET_1K_Val          _U_(0x7)                                             /**< (WDT_EWCTRL) 1024 clock cycles  */
#define   WDT_EWCTRL_EWOFFSET_2K_Val          _U_(0x8)                                             /**< (WDT_EWCTRL) 2048 clock cycles  */
#define   WDT_EWCTRL_EWOFFSET_4K_Val          _U_(0x9)                                             /**< (WDT_EWCTRL) 4096 clock cycles  */
#define   WDT_EWCTRL_EWOFFSET_8K_Val          _U_(0xA)                                             /**< (WDT_EWCTRL) 8192 clock cycles  */
#define   WDT_EWCTRL_EWOFFSET_16K_Val         _U_(0xB)                                             /**< (WDT_EWCTRL) 16384 clock cycles  */
#define WDT_EWCTRL_EWOFFSET_8                 (WDT_EWCTRL_EWOFFSET_8_Val << WDT_EWCTRL_EWOFFSET_Pos) /**< (WDT_EWCTRL) 8 clock cycles Position  */
#define WDT_EWCTRL_EWOFFSET_16                (WDT_EWCTRL_EWOFFSET_16_Val << WDT_EWCTRL_EWOFFSET_Pos) /**< (WDT_EWCTRL) 16 clock cycles Position  */
#define WDT_EWCTRL_EWOFFSET_32                (WDT_EWCTRL_EWOFFSET_32_Val << WDT_EWCTRL_EWOFFSET_Pos) /**< (WDT_EWCTRL) 32 clock cycles Position  */
#define WDT_EWCTRL_EWOFFSET_64                (WDT_EWCTRL_EWOFFSET_64_Val << WDT_EWCTRL_EWOFFSET_Pos) /**< (WDT_EWCTRL) 64 clock cycles Position  */
#define WDT_EWCTRL_EWOFFSET_128               (WDT_EWCTRL_EWOFFSET_128_Val << WDT_EWCTRL_EWOFFSET_Pos) /**< (WDT_EWCTRL) 128 clock cycles Position  */
#define WDT_EWCTRL_EWOFFSET_256               (WDT_EWCTRL_EWOFFSET_256_Val << WDT_EWCTRL_EWOFFSET_Pos) /**< (WDT_EWCTRL) 256 clock cycles Position  */
#define WDT_EWCTRL_EWOFFSET_512               (WDT_EWCTRL_EWOFFSET_512_Val << WDT_EWCTRL_EWOFFSET_Pos) /**< (WDT_EWCTRL) 512 clock cycles Position  */
#define WDT_EWCTRL_EWOFFSET_1K                (WDT_EWCTRL_EWOFFSET_1K_Val << WDT_EWCTRL_EWOFFSET_Pos) /**< (WDT_EWCTRL) 1024 clock cycles Position  */
#define WDT_EWCTRL_EWOFFSET_2K                (WDT_EWCTRL_EWOFFSET_2K_Val << WDT_EWCTRL_EWOFFSET_Pos) /**< (WDT_EWCTRL) 2048 clock cycles Position  */
#define WDT_EWCTRL_EWOFFSET_4K                (WDT_EWCTRL_EWOFFSET_4K_Val << WDT_EWCTRL_EWOFFSET_Pos) /**< (WDT_EWCTRL) 4096 clock cycles Position  */
#define WDT_EWCTRL_EWOFFSET_8K                (WDT_EWCTRL_EWOFFSET_8K_Val << WDT_EWCTRL_EWOFFSET_Pos) /**< (WDT_EWCTRL) 8192 clock cycles Position  */
#define WDT_EWCTRL_EWOFFSET_16K               (WDT_EWCTRL_EWOFFSET_16K_Val << WDT_EWCTRL_EWOFFSET_Pos) /**< (WDT_EWCTRL) 16384 clock cycles Position  */
#define WDT_EWCTRL_Msk                        _U_(0x0F)                                            /**< (WDT_EWCTRL) Register Mask  */


/* -------- WDT_INTENCLR : (WDT Offset: 0x04) (R/W 8) Interrupt Enable Clear -------- */
#define WDT_INTENCLR_RESETVALUE               _U_(0x00)                                            /**<  (WDT_INTENCLR) Interrupt Enable Clear  Reset Value */

#define WDT_INTENCLR_EW_Pos                   _U_(0)                                               /**< (WDT_INTENCLR) Early Warning Interrupt Enable Position */
#define WDT_INTENCLR_EW_Msk                   (_U_(0x1) << WDT_INTENCLR_EW_Pos)                    /**< (WDT_INTENCLR) Early Warning Interrupt Enable Mask */
#define WDT_INTENCLR_EW(value)                (WDT_INTENCLR_EW_Msk & ((value) << WDT_INTENCLR_EW_Pos))
#define WDT_INTENCLR_Msk                      _U_(0x01)                                            /**< (WDT_INTENCLR) Register Mask  */


/* -------- WDT_INTENSET : (WDT Offset: 0x05) (R/W 8) Interrupt Enable Set -------- */
#define WDT_INTENSET_RESETVALUE               _U_(0x00)                                            /**<  (WDT_INTENSET) Interrupt Enable Set  Reset Value */

#define WDT_INTENSET_EW_Pos                   _U_(0)                                               /**< (WDT_INTENSET) Early Warning Interrupt Enable Position */
#define WDT_INTENSET_EW_Msk                   (_U_(0x1) << WDT_INTENSET_EW_Pos)                    /**< (WDT_INTENSET) Early Warning Interrupt Enable Mask */
#define WDT_INTENSET_EW(value)                (WDT_INTENSET_EW_Msk & ((value) << WDT_INTENSET_EW_Pos))
#define WDT_INTENSET_Msk                      _U_(0x01)                                            /**< (WDT_INTENSET) Register Mask  */


/* -------- WDT_INTFLAG : (WDT Offset: 0x06) (R/W 8) Interrupt Flag Status and Clear -------- */
#define WDT_INTFLAG_RESETVALUE                _U_(0x00)                                            /**<  (WDT_INTFLAG) Interrupt Flag Status and Clear  Reset Value */

#define WDT_INTFLAG_EW_Pos                    _U_(0)                                               /**< (WDT_INTFLAG) Early Warning Position */
#define WDT_INTFLAG_EW_Msk                    (_U_(0x1) << WDT_INTFLAG_EW_Pos)                     /**< (WDT_INTFLAG) Early Warning Mask */
#define WDT_INTFLAG_EW(value)                 (WDT_INTFLAG_EW_Msk & ((value) << WDT_INTFLAG_EW_Pos))
#define WDT_INTFLAG_Msk                       _U_(0x01)                                            /**< (WDT_INTFLAG) Register Mask  */


/* -------- WDT_STATUS : (WDT Offset: 0x07) ( R/ 8) Status -------- */
#define WDT_STATUS_RESETVALUE                 _U_(0x00)                                            /**<  (WDT_STATUS) Status  Reset Value */

#define WDT_STATUS_SYNCBUSY_Pos               _U_(7)                                               /**< (WDT_STATUS) Synchronization Busy Position */
#define WDT_STATUS_SYNCBUSY_Msk               (_U_(0x1) << WDT_STATUS_SYNCBUSY_Pos)                /**< (WDT_STATUS) Synchronization Busy Mask */
#define WDT_STATUS_SYNCBUSY(value)            (WDT_STATUS_SYNCBUSY_Msk & ((value) << WDT_STATUS_SYNCBUSY_Pos))
#define WDT_STATUS_Msk                        _U_(0x80)                                            /**< (WDT_STATUS) Register Mask  */


/* -------- WDT_CLEAR : (WDT Offset: 0x08) ( /W 8) Clear -------- */
#define WDT_CLEAR_RESETVALUE                  _U_(0x00)                                            /**<  (WDT_CLEAR) Clear  Reset Value */

#define WDT_CLEAR_CLEAR_Pos                   _U_(0)                                               /**< (WDT_CLEAR) Watchdog Clear Position */
#define WDT_CLEAR_CLEAR_Msk                   (_U_(0xFF) << WDT_CLEAR_CLEAR_Pos)                   /**< (WDT_CLEAR) Watchdog Clear Mask */
#define WDT_CLEAR_CLEAR(value)                (WDT_CLEAR_CLEAR_Msk & ((value) << WDT_CLEAR_CLEAR_Pos))
#define   WDT_CLEAR_CLEAR_KEY_Val             _U_(0xA5)                                            /**< (WDT_CLEAR) Clear Key  */
#define WDT_CLEAR_CLEAR_KEY                   (WDT_CLEAR_CLEAR_KEY_Val << WDT_CLEAR_CLEAR_Pos)     /**< (WDT_CLEAR) Clear Key Position  */
#define WDT_CLEAR_Msk                         _U_(0xFF)                                            /**< (WDT_CLEAR) Register Mask  */


/** \brief WDT register offsets definitions */
#define WDT_CTRL_REG_OFST              (0x00)              /**< (WDT_CTRL) Control Offset */
#define WDT_CONFIG_REG_OFST            (0x01)              /**< (WDT_CONFIG) Configuration Offset */
#define WDT_EWCTRL_REG_OFST            (0x02)              /**< (WDT_EWCTRL) Early Warning Interrupt Control Offset */
#define WDT_INTENCLR_REG_OFST          (0x04)              /**< (WDT_INTENCLR) Interrupt Enable Clear Offset */
#define WDT_INTENSET_REG_OFST          (0x05)              /**< (WDT_INTENSET) Interrupt Enable Set Offset */
#define WDT_INTFLAG_REG_OFST           (0x06)              /**< (WDT_INTFLAG) Interrupt Flag Status and Clear Offset */
#define WDT_STATUS_REG_OFST            (0x07)              /**< (WDT_STATUS) Status Offset */
#define WDT_CLEAR_REG_OFST             (0x08)              /**< (WDT_CLEAR) Clear Offset */

#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
/** \brief WDT register API structure */
typedef struct
{  /* Watchdog Timer */
  __IO  uint8_t                        WDT_CTRL;           /**< Offset: 0x00 (R/W  8) Control */
  __IO  uint8_t                        WDT_CONFIG;         /**< Offset: 0x01 (R/W  8) Configuration */
  __IO  uint8_t                        WDT_EWCTRL;         /**< Offset: 0x02 (R/W  8) Early Warning Interrupt Control */
  __I   uint8_t                        Reserved1[0x01];
  __IO  uint8_t                        WDT_INTENCLR;       /**< Offset: 0x04 (R/W  8) Interrupt Enable Clear */
  __IO  uint8_t                        WDT_INTENSET;       /**< Offset: 0x05 (R/W  8) Interrupt Enable Set */
  __IO  uint8_t                        WDT_INTFLAG;        /**< Offset: 0x06 (R/W  8) Interrupt Flag Status and Clear */
  __I   uint8_t                        WDT_STATUS;         /**< Offset: 0x07 (R/   8) Status */
  __O   uint8_t                        WDT_CLEAR;          /**< Offset: 0x08 ( /W  8) Clear */
} wdt_registers_t;


#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
#endif /* _SAMD21_WDT_COMPONENT_H_ */
