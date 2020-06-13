/**
 * \brief Component description for PM
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
#ifndef _SAMD21_PM_COMPONENT_H_
#define _SAMD21_PM_COMPONENT_H_

/* ************************************************************************** */
/*   SOFTWARE API DEFINITION FOR PM                                           */
/* ************************************************************************** */

/* -------- PM_CTRL : (PM Offset: 0x00) (R/W 8) Control -------- */
#define PM_CTRL_RESETVALUE                    _U_(0x00)                                            /**<  (PM_CTRL) Control  Reset Value */

#define PM_CTRL_Msk                           _U_(0x00)                                            /**< (PM_CTRL) Register Mask  */


/* -------- PM_SLEEP : (PM Offset: 0x01) (R/W 8) Sleep Mode -------- */
#define PM_SLEEP_RESETVALUE                   _U_(0x00)                                            /**<  (PM_SLEEP) Sleep Mode  Reset Value */

#define PM_SLEEP_IDLE_Pos                     _U_(0)                                               /**< (PM_SLEEP) Idle Mode Configuration Position */
#define PM_SLEEP_IDLE_Msk                     (_U_(0x3) << PM_SLEEP_IDLE_Pos)                      /**< (PM_SLEEP) Idle Mode Configuration Mask */
#define PM_SLEEP_IDLE(value)                  (PM_SLEEP_IDLE_Msk & ((value) << PM_SLEEP_IDLE_Pos))
#define   PM_SLEEP_IDLE_CPU_Val               _U_(0x0)                                             /**< (PM_SLEEP) The CPU clock domain is stopped  */
#define   PM_SLEEP_IDLE_AHB_Val               _U_(0x1)                                             /**< (PM_SLEEP) The CPU and AHB clock domains are stopped  */
#define   PM_SLEEP_IDLE_APB_Val               _U_(0x2)                                             /**< (PM_SLEEP) The CPU, AHB and APB clock domains are stopped  */
#define PM_SLEEP_IDLE_CPU                     (PM_SLEEP_IDLE_CPU_Val << PM_SLEEP_IDLE_Pos)         /**< (PM_SLEEP) The CPU clock domain is stopped Position  */
#define PM_SLEEP_IDLE_AHB                     (PM_SLEEP_IDLE_AHB_Val << PM_SLEEP_IDLE_Pos)         /**< (PM_SLEEP) The CPU and AHB clock domains are stopped Position  */
#define PM_SLEEP_IDLE_APB                     (PM_SLEEP_IDLE_APB_Val << PM_SLEEP_IDLE_Pos)         /**< (PM_SLEEP) The CPU, AHB and APB clock domains are stopped Position  */
#define PM_SLEEP_Msk                          _U_(0x03)                                            /**< (PM_SLEEP) Register Mask  */


/* -------- PM_CPUSEL : (PM Offset: 0x08) (R/W 8) CPU Clock Select -------- */
#define PM_CPUSEL_RESETVALUE                  _U_(0x00)                                            /**<  (PM_CPUSEL) CPU Clock Select  Reset Value */

#define PM_CPUSEL_CPUDIV_Pos                  _U_(0)                                               /**< (PM_CPUSEL) CPU Prescaler Selection Position */
#define PM_CPUSEL_CPUDIV_Msk                  (_U_(0x7) << PM_CPUSEL_CPUDIV_Pos)                   /**< (PM_CPUSEL) CPU Prescaler Selection Mask */
#define PM_CPUSEL_CPUDIV(value)               (PM_CPUSEL_CPUDIV_Msk & ((value) << PM_CPUSEL_CPUDIV_Pos))
#define   PM_CPUSEL_CPUDIV_DIV1_Val           _U_(0x0)                                             /**< (PM_CPUSEL) Divide by 1  */
#define   PM_CPUSEL_CPUDIV_DIV2_Val           _U_(0x1)                                             /**< (PM_CPUSEL) Divide by 2  */
#define   PM_CPUSEL_CPUDIV_DIV4_Val           _U_(0x2)                                             /**< (PM_CPUSEL) Divide by 4  */
#define   PM_CPUSEL_CPUDIV_DIV8_Val           _U_(0x3)                                             /**< (PM_CPUSEL) Divide by 8  */
#define   PM_CPUSEL_CPUDIV_DIV16_Val          _U_(0x4)                                             /**< (PM_CPUSEL) Divide by 16  */
#define   PM_CPUSEL_CPUDIV_DIV32_Val          _U_(0x5)                                             /**< (PM_CPUSEL) Divide by 32  */
#define   PM_CPUSEL_CPUDIV_DIV64_Val          _U_(0x6)                                             /**< (PM_CPUSEL) Divide by 64  */
#define   PM_CPUSEL_CPUDIV_DIV128_Val         _U_(0x7)                                             /**< (PM_CPUSEL) Divide by 128  */
#define PM_CPUSEL_CPUDIV_DIV1                 (PM_CPUSEL_CPUDIV_DIV1_Val << PM_CPUSEL_CPUDIV_Pos)  /**< (PM_CPUSEL) Divide by 1 Position  */
#define PM_CPUSEL_CPUDIV_DIV2                 (PM_CPUSEL_CPUDIV_DIV2_Val << PM_CPUSEL_CPUDIV_Pos)  /**< (PM_CPUSEL) Divide by 2 Position  */
#define PM_CPUSEL_CPUDIV_DIV4                 (PM_CPUSEL_CPUDIV_DIV4_Val << PM_CPUSEL_CPUDIV_Pos)  /**< (PM_CPUSEL) Divide by 4 Position  */
#define PM_CPUSEL_CPUDIV_DIV8                 (PM_CPUSEL_CPUDIV_DIV8_Val << PM_CPUSEL_CPUDIV_Pos)  /**< (PM_CPUSEL) Divide by 8 Position  */
#define PM_CPUSEL_CPUDIV_DIV16                (PM_CPUSEL_CPUDIV_DIV16_Val << PM_CPUSEL_CPUDIV_Pos) /**< (PM_CPUSEL) Divide by 16 Position  */
#define PM_CPUSEL_CPUDIV_DIV32                (PM_CPUSEL_CPUDIV_DIV32_Val << PM_CPUSEL_CPUDIV_Pos) /**< (PM_CPUSEL) Divide by 32 Position  */
#define PM_CPUSEL_CPUDIV_DIV64                (PM_CPUSEL_CPUDIV_DIV64_Val << PM_CPUSEL_CPUDIV_Pos) /**< (PM_CPUSEL) Divide by 64 Position  */
#define PM_CPUSEL_CPUDIV_DIV128               (PM_CPUSEL_CPUDIV_DIV128_Val << PM_CPUSEL_CPUDIV_Pos) /**< (PM_CPUSEL) Divide by 128 Position  */
#define PM_CPUSEL_Msk                         _U_(0x07)                                            /**< (PM_CPUSEL) Register Mask  */


/* -------- PM_APBASEL : (PM Offset: 0x09) (R/W 8) APBA Clock Select -------- */
#define PM_APBASEL_RESETVALUE                 _U_(0x00)                                            /**<  (PM_APBASEL) APBA Clock Select  Reset Value */

#define PM_APBASEL_APBADIV_Pos                _U_(0)                                               /**< (PM_APBASEL) APBA Prescaler Selection Position */
#define PM_APBASEL_APBADIV_Msk                (_U_(0x7) << PM_APBASEL_APBADIV_Pos)                 /**< (PM_APBASEL) APBA Prescaler Selection Mask */
#define PM_APBASEL_APBADIV(value)             (PM_APBASEL_APBADIV_Msk & ((value) << PM_APBASEL_APBADIV_Pos))
#define   PM_APBASEL_APBADIV_DIV1_Val         _U_(0x0)                                             /**< (PM_APBASEL) Divide by 1  */
#define   PM_APBASEL_APBADIV_DIV2_Val         _U_(0x1)                                             /**< (PM_APBASEL) Divide by 2  */
#define   PM_APBASEL_APBADIV_DIV4_Val         _U_(0x2)                                             /**< (PM_APBASEL) Divide by 4  */
#define   PM_APBASEL_APBADIV_DIV8_Val         _U_(0x3)                                             /**< (PM_APBASEL) Divide by 8  */
#define   PM_APBASEL_APBADIV_DIV16_Val        _U_(0x4)                                             /**< (PM_APBASEL) Divide by 16  */
#define   PM_APBASEL_APBADIV_DIV32_Val        _U_(0x5)                                             /**< (PM_APBASEL) Divide by 32  */
#define   PM_APBASEL_APBADIV_DIV64_Val        _U_(0x6)                                             /**< (PM_APBASEL) Divide by 64  */
#define   PM_APBASEL_APBADIV_DIV128_Val       _U_(0x7)                                             /**< (PM_APBASEL) Divide by 128  */
#define PM_APBASEL_APBADIV_DIV1               (PM_APBASEL_APBADIV_DIV1_Val << PM_APBASEL_APBADIV_Pos) /**< (PM_APBASEL) Divide by 1 Position  */
#define PM_APBASEL_APBADIV_DIV2               (PM_APBASEL_APBADIV_DIV2_Val << PM_APBASEL_APBADIV_Pos) /**< (PM_APBASEL) Divide by 2 Position  */
#define PM_APBASEL_APBADIV_DIV4               (PM_APBASEL_APBADIV_DIV4_Val << PM_APBASEL_APBADIV_Pos) /**< (PM_APBASEL) Divide by 4 Position  */
#define PM_APBASEL_APBADIV_DIV8               (PM_APBASEL_APBADIV_DIV8_Val << PM_APBASEL_APBADIV_Pos) /**< (PM_APBASEL) Divide by 8 Position  */
#define PM_APBASEL_APBADIV_DIV16              (PM_APBASEL_APBADIV_DIV16_Val << PM_APBASEL_APBADIV_Pos) /**< (PM_APBASEL) Divide by 16 Position  */
#define PM_APBASEL_APBADIV_DIV32              (PM_APBASEL_APBADIV_DIV32_Val << PM_APBASEL_APBADIV_Pos) /**< (PM_APBASEL) Divide by 32 Position  */
#define PM_APBASEL_APBADIV_DIV64              (PM_APBASEL_APBADIV_DIV64_Val << PM_APBASEL_APBADIV_Pos) /**< (PM_APBASEL) Divide by 64 Position  */
#define PM_APBASEL_APBADIV_DIV128             (PM_APBASEL_APBADIV_DIV128_Val << PM_APBASEL_APBADIV_Pos) /**< (PM_APBASEL) Divide by 128 Position  */
#define PM_APBASEL_Msk                        _U_(0x07)                                            /**< (PM_APBASEL) Register Mask  */


/* -------- PM_APBBSEL : (PM Offset: 0x0A) (R/W 8) APBB Clock Select -------- */
#define PM_APBBSEL_RESETVALUE                 _U_(0x00)                                            /**<  (PM_APBBSEL) APBB Clock Select  Reset Value */

#define PM_APBBSEL_APBBDIV_Pos                _U_(0)                                               /**< (PM_APBBSEL) APBB Prescaler Selection Position */
#define PM_APBBSEL_APBBDIV_Msk                (_U_(0x7) << PM_APBBSEL_APBBDIV_Pos)                 /**< (PM_APBBSEL) APBB Prescaler Selection Mask */
#define PM_APBBSEL_APBBDIV(value)             (PM_APBBSEL_APBBDIV_Msk & ((value) << PM_APBBSEL_APBBDIV_Pos))
#define   PM_APBBSEL_APBBDIV_DIV1_Val         _U_(0x0)                                             /**< (PM_APBBSEL) Divide by 1  */
#define   PM_APBBSEL_APBBDIV_DIV2_Val         _U_(0x1)                                             /**< (PM_APBBSEL) Divide by 2  */
#define   PM_APBBSEL_APBBDIV_DIV4_Val         _U_(0x2)                                             /**< (PM_APBBSEL) Divide by 4  */
#define   PM_APBBSEL_APBBDIV_DIV8_Val         _U_(0x3)                                             /**< (PM_APBBSEL) Divide by 8  */
#define   PM_APBBSEL_APBBDIV_DIV16_Val        _U_(0x4)                                             /**< (PM_APBBSEL) Divide by 16  */
#define   PM_APBBSEL_APBBDIV_DIV32_Val        _U_(0x5)                                             /**< (PM_APBBSEL) Divide by 32  */
#define   PM_APBBSEL_APBBDIV_DIV64_Val        _U_(0x6)                                             /**< (PM_APBBSEL) Divide by 64  */
#define   PM_APBBSEL_APBBDIV_DIV128_Val       _U_(0x7)                                             /**< (PM_APBBSEL) Divide by 128  */
#define PM_APBBSEL_APBBDIV_DIV1               (PM_APBBSEL_APBBDIV_DIV1_Val << PM_APBBSEL_APBBDIV_Pos) /**< (PM_APBBSEL) Divide by 1 Position  */
#define PM_APBBSEL_APBBDIV_DIV2               (PM_APBBSEL_APBBDIV_DIV2_Val << PM_APBBSEL_APBBDIV_Pos) /**< (PM_APBBSEL) Divide by 2 Position  */
#define PM_APBBSEL_APBBDIV_DIV4               (PM_APBBSEL_APBBDIV_DIV4_Val << PM_APBBSEL_APBBDIV_Pos) /**< (PM_APBBSEL) Divide by 4 Position  */
#define PM_APBBSEL_APBBDIV_DIV8               (PM_APBBSEL_APBBDIV_DIV8_Val << PM_APBBSEL_APBBDIV_Pos) /**< (PM_APBBSEL) Divide by 8 Position  */
#define PM_APBBSEL_APBBDIV_DIV16              (PM_APBBSEL_APBBDIV_DIV16_Val << PM_APBBSEL_APBBDIV_Pos) /**< (PM_APBBSEL) Divide by 16 Position  */
#define PM_APBBSEL_APBBDIV_DIV32              (PM_APBBSEL_APBBDIV_DIV32_Val << PM_APBBSEL_APBBDIV_Pos) /**< (PM_APBBSEL) Divide by 32 Position  */
#define PM_APBBSEL_APBBDIV_DIV64              (PM_APBBSEL_APBBDIV_DIV64_Val << PM_APBBSEL_APBBDIV_Pos) /**< (PM_APBBSEL) Divide by 64 Position  */
#define PM_APBBSEL_APBBDIV_DIV128             (PM_APBBSEL_APBBDIV_DIV128_Val << PM_APBBSEL_APBBDIV_Pos) /**< (PM_APBBSEL) Divide by 128 Position  */
#define PM_APBBSEL_Msk                        _U_(0x07)                                            /**< (PM_APBBSEL) Register Mask  */


/* -------- PM_APBCSEL : (PM Offset: 0x0B) (R/W 8) APBC Clock Select -------- */
#define PM_APBCSEL_RESETVALUE                 _U_(0x00)                                            /**<  (PM_APBCSEL) APBC Clock Select  Reset Value */

#define PM_APBCSEL_APBCDIV_Pos                _U_(0)                                               /**< (PM_APBCSEL) APBC Prescaler Selection Position */
#define PM_APBCSEL_APBCDIV_Msk                (_U_(0x7) << PM_APBCSEL_APBCDIV_Pos)                 /**< (PM_APBCSEL) APBC Prescaler Selection Mask */
#define PM_APBCSEL_APBCDIV(value)             (PM_APBCSEL_APBCDIV_Msk & ((value) << PM_APBCSEL_APBCDIV_Pos))
#define   PM_APBCSEL_APBCDIV_DIV1_Val         _U_(0x0)                                             /**< (PM_APBCSEL) Divide by 1  */
#define   PM_APBCSEL_APBCDIV_DIV2_Val         _U_(0x1)                                             /**< (PM_APBCSEL) Divide by 2  */
#define   PM_APBCSEL_APBCDIV_DIV4_Val         _U_(0x2)                                             /**< (PM_APBCSEL) Divide by 4  */
#define   PM_APBCSEL_APBCDIV_DIV8_Val         _U_(0x3)                                             /**< (PM_APBCSEL) Divide by 8  */
#define   PM_APBCSEL_APBCDIV_DIV16_Val        _U_(0x4)                                             /**< (PM_APBCSEL) Divide by 16  */
#define   PM_APBCSEL_APBCDIV_DIV32_Val        _U_(0x5)                                             /**< (PM_APBCSEL) Divide by 32  */
#define   PM_APBCSEL_APBCDIV_DIV64_Val        _U_(0x6)                                             /**< (PM_APBCSEL) Divide by 64  */
#define   PM_APBCSEL_APBCDIV_DIV128_Val       _U_(0x7)                                             /**< (PM_APBCSEL) Divide by 128  */
#define PM_APBCSEL_APBCDIV_DIV1               (PM_APBCSEL_APBCDIV_DIV1_Val << PM_APBCSEL_APBCDIV_Pos) /**< (PM_APBCSEL) Divide by 1 Position  */
#define PM_APBCSEL_APBCDIV_DIV2               (PM_APBCSEL_APBCDIV_DIV2_Val << PM_APBCSEL_APBCDIV_Pos) /**< (PM_APBCSEL) Divide by 2 Position  */
#define PM_APBCSEL_APBCDIV_DIV4               (PM_APBCSEL_APBCDIV_DIV4_Val << PM_APBCSEL_APBCDIV_Pos) /**< (PM_APBCSEL) Divide by 4 Position  */
#define PM_APBCSEL_APBCDIV_DIV8               (PM_APBCSEL_APBCDIV_DIV8_Val << PM_APBCSEL_APBCDIV_Pos) /**< (PM_APBCSEL) Divide by 8 Position  */
#define PM_APBCSEL_APBCDIV_DIV16              (PM_APBCSEL_APBCDIV_DIV16_Val << PM_APBCSEL_APBCDIV_Pos) /**< (PM_APBCSEL) Divide by 16 Position  */
#define PM_APBCSEL_APBCDIV_DIV32              (PM_APBCSEL_APBCDIV_DIV32_Val << PM_APBCSEL_APBCDIV_Pos) /**< (PM_APBCSEL) Divide by 32 Position  */
#define PM_APBCSEL_APBCDIV_DIV64              (PM_APBCSEL_APBCDIV_DIV64_Val << PM_APBCSEL_APBCDIV_Pos) /**< (PM_APBCSEL) Divide by 64 Position  */
#define PM_APBCSEL_APBCDIV_DIV128             (PM_APBCSEL_APBCDIV_DIV128_Val << PM_APBCSEL_APBCDIV_Pos) /**< (PM_APBCSEL) Divide by 128 Position  */
#define PM_APBCSEL_Msk                        _U_(0x07)                                            /**< (PM_APBCSEL) Register Mask  */


/* -------- PM_AHBMASK : (PM Offset: 0x14) (R/W 32) AHB Mask -------- */
#define PM_AHBMASK_RESETVALUE                 _U_(0x7F)                                            /**<  (PM_AHBMASK) AHB Mask  Reset Value */

#define PM_AHBMASK_HPB0_Pos                   _U_(0)                                               /**< (PM_AHBMASK) HPB0 AHB Clock Mask Position */
#define PM_AHBMASK_HPB0_Msk                   (_U_(0x1) << PM_AHBMASK_HPB0_Pos)                    /**< (PM_AHBMASK) HPB0 AHB Clock Mask Mask */
#define PM_AHBMASK_HPB0(value)                (PM_AHBMASK_HPB0_Msk & ((value) << PM_AHBMASK_HPB0_Pos))
#define PM_AHBMASK_HPB1_Pos                   _U_(1)                                               /**< (PM_AHBMASK) HPB1 AHB Clock Mask Position */
#define PM_AHBMASK_HPB1_Msk                   (_U_(0x1) << PM_AHBMASK_HPB1_Pos)                    /**< (PM_AHBMASK) HPB1 AHB Clock Mask Mask */
#define PM_AHBMASK_HPB1(value)                (PM_AHBMASK_HPB1_Msk & ((value) << PM_AHBMASK_HPB1_Pos))
#define PM_AHBMASK_HPB2_Pos                   _U_(2)                                               /**< (PM_AHBMASK) HPB2 AHB Clock Mask Position */
#define PM_AHBMASK_HPB2_Msk                   (_U_(0x1) << PM_AHBMASK_HPB2_Pos)                    /**< (PM_AHBMASK) HPB2 AHB Clock Mask Mask */
#define PM_AHBMASK_HPB2(value)                (PM_AHBMASK_HPB2_Msk & ((value) << PM_AHBMASK_HPB2_Pos))
#define PM_AHBMASK_DSU_Pos                    _U_(3)                                               /**< (PM_AHBMASK) DSU AHB Clock Mask Position */
#define PM_AHBMASK_DSU_Msk                    (_U_(0x1) << PM_AHBMASK_DSU_Pos)                     /**< (PM_AHBMASK) DSU AHB Clock Mask Mask */
#define PM_AHBMASK_DSU(value)                 (PM_AHBMASK_DSU_Msk & ((value) << PM_AHBMASK_DSU_Pos))
#define PM_AHBMASK_NVMCTRL_Pos                _U_(4)                                               /**< (PM_AHBMASK) NVMCTRL AHB Clock Mask Position */
#define PM_AHBMASK_NVMCTRL_Msk                (_U_(0x1) << PM_AHBMASK_NVMCTRL_Pos)                 /**< (PM_AHBMASK) NVMCTRL AHB Clock Mask Mask */
#define PM_AHBMASK_NVMCTRL(value)             (PM_AHBMASK_NVMCTRL_Msk & ((value) << PM_AHBMASK_NVMCTRL_Pos))
#define PM_AHBMASK_DMAC_Pos                   _U_(5)                                               /**< (PM_AHBMASK) DMAC AHB Clock Mask Position */
#define PM_AHBMASK_DMAC_Msk                   (_U_(0x1) << PM_AHBMASK_DMAC_Pos)                    /**< (PM_AHBMASK) DMAC AHB Clock Mask Mask */
#define PM_AHBMASK_DMAC(value)                (PM_AHBMASK_DMAC_Msk & ((value) << PM_AHBMASK_DMAC_Pos))
#define PM_AHBMASK_USB_Pos                    _U_(6)                                               /**< (PM_AHBMASK) USB AHB Clock Mask Position */
#define PM_AHBMASK_USB_Msk                    (_U_(0x1) << PM_AHBMASK_USB_Pos)                     /**< (PM_AHBMASK) USB AHB Clock Mask Mask */
#define PM_AHBMASK_USB(value)                 (PM_AHBMASK_USB_Msk & ((value) << PM_AHBMASK_USB_Pos))
#define PM_AHBMASK_Msk                        _U_(0x0000007F)                                      /**< (PM_AHBMASK) Register Mask  */

#define PM_AHBMASK_HPB_Pos                    _U_(0)                                               /**< (PM_AHBMASK Position) HPBx AHB Clock Mask */
#define PM_AHBMASK_HPB_Msk                    (_U_(0x7) << PM_AHBMASK_HPB_Pos)                     /**< (PM_AHBMASK Mask) HPB */
#define PM_AHBMASK_HPB(value)                 (PM_AHBMASK_HPB_Msk & ((value) << PM_AHBMASK_HPB_Pos)) 

/* -------- PM_APBAMASK : (PM Offset: 0x18) (R/W 32) APBA Mask -------- */
#define PM_APBAMASK_RESETVALUE                _U_(0x7F)                                            /**<  (PM_APBAMASK) APBA Mask  Reset Value */

#define PM_APBAMASK_PAC0_Pos                  _U_(0)                                               /**< (PM_APBAMASK) PAC0 APB Clock Enable Position */
#define PM_APBAMASK_PAC0_Msk                  (_U_(0x1) << PM_APBAMASK_PAC0_Pos)                   /**< (PM_APBAMASK) PAC0 APB Clock Enable Mask */
#define PM_APBAMASK_PAC0(value)               (PM_APBAMASK_PAC0_Msk & ((value) << PM_APBAMASK_PAC0_Pos))
#define PM_APBAMASK_PM_Pos                    _U_(1)                                               /**< (PM_APBAMASK) PM APB Clock Enable Position */
#define PM_APBAMASK_PM_Msk                    (_U_(0x1) << PM_APBAMASK_PM_Pos)                     /**< (PM_APBAMASK) PM APB Clock Enable Mask */
#define PM_APBAMASK_PM(value)                 (PM_APBAMASK_PM_Msk & ((value) << PM_APBAMASK_PM_Pos))
#define PM_APBAMASK_SYSCTRL_Pos               _U_(2)                                               /**< (PM_APBAMASK) SYSCTRL APB Clock Enable Position */
#define PM_APBAMASK_SYSCTRL_Msk               (_U_(0x1) << PM_APBAMASK_SYSCTRL_Pos)                /**< (PM_APBAMASK) SYSCTRL APB Clock Enable Mask */
#define PM_APBAMASK_SYSCTRL(value)            (PM_APBAMASK_SYSCTRL_Msk & ((value) << PM_APBAMASK_SYSCTRL_Pos))
#define PM_APBAMASK_GCLK_Pos                  _U_(3)                                               /**< (PM_APBAMASK) GCLK APB Clock Enable Position */
#define PM_APBAMASK_GCLK_Msk                  (_U_(0x1) << PM_APBAMASK_GCLK_Pos)                   /**< (PM_APBAMASK) GCLK APB Clock Enable Mask */
#define PM_APBAMASK_GCLK(value)               (PM_APBAMASK_GCLK_Msk & ((value) << PM_APBAMASK_GCLK_Pos))
#define PM_APBAMASK_WDT_Pos                   _U_(4)                                               /**< (PM_APBAMASK) WDT APB Clock Enable Position */
#define PM_APBAMASK_WDT_Msk                   (_U_(0x1) << PM_APBAMASK_WDT_Pos)                    /**< (PM_APBAMASK) WDT APB Clock Enable Mask */
#define PM_APBAMASK_WDT(value)                (PM_APBAMASK_WDT_Msk & ((value) << PM_APBAMASK_WDT_Pos))
#define PM_APBAMASK_RTC_Pos                   _U_(5)                                               /**< (PM_APBAMASK) RTC APB Clock Enable Position */
#define PM_APBAMASK_RTC_Msk                   (_U_(0x1) << PM_APBAMASK_RTC_Pos)                    /**< (PM_APBAMASK) RTC APB Clock Enable Mask */
#define PM_APBAMASK_RTC(value)                (PM_APBAMASK_RTC_Msk & ((value) << PM_APBAMASK_RTC_Pos))
#define PM_APBAMASK_EIC_Pos                   _U_(6)                                               /**< (PM_APBAMASK) EIC APB Clock Enable Position */
#define PM_APBAMASK_EIC_Msk                   (_U_(0x1) << PM_APBAMASK_EIC_Pos)                    /**< (PM_APBAMASK) EIC APB Clock Enable Mask */
#define PM_APBAMASK_EIC(value)                (PM_APBAMASK_EIC_Msk & ((value) << PM_APBAMASK_EIC_Pos))
#define PM_APBAMASK_Msk                       _U_(0x0000007F)                                      /**< (PM_APBAMASK) Register Mask  */

#define PM_APBAMASK_PAC_Pos                   _U_(0)                                               /**< (PM_APBAMASK Position) PACx APB Clock Enable */
#define PM_APBAMASK_PAC_Msk                   (_U_(0x1) << PM_APBAMASK_PAC_Pos)                    /**< (PM_APBAMASK Mask) PAC */
#define PM_APBAMASK_PAC(value)                (PM_APBAMASK_PAC_Msk & ((value) << PM_APBAMASK_PAC_Pos)) 

/* -------- PM_APBBMASK : (PM Offset: 0x1C) (R/W 32) APBB Mask -------- */
#define PM_APBBMASK_RESETVALUE                _U_(0x7F)                                            /**<  (PM_APBBMASK) APBB Mask  Reset Value */

#define PM_APBBMASK_PAC1_Pos                  _U_(0)                                               /**< (PM_APBBMASK) PAC1 APB Clock Enable Position */
#define PM_APBBMASK_PAC1_Msk                  (_U_(0x1) << PM_APBBMASK_PAC1_Pos)                   /**< (PM_APBBMASK) PAC1 APB Clock Enable Mask */
#define PM_APBBMASK_PAC1(value)               (PM_APBBMASK_PAC1_Msk & ((value) << PM_APBBMASK_PAC1_Pos))
#define PM_APBBMASK_DSU_Pos                   _U_(1)                                               /**< (PM_APBBMASK) DSU APB Clock Enable Position */
#define PM_APBBMASK_DSU_Msk                   (_U_(0x1) << PM_APBBMASK_DSU_Pos)                    /**< (PM_APBBMASK) DSU APB Clock Enable Mask */
#define PM_APBBMASK_DSU(value)                (PM_APBBMASK_DSU_Msk & ((value) << PM_APBBMASK_DSU_Pos))
#define PM_APBBMASK_NVMCTRL_Pos               _U_(2)                                               /**< (PM_APBBMASK) NVMCTRL APB Clock Enable Position */
#define PM_APBBMASK_NVMCTRL_Msk               (_U_(0x1) << PM_APBBMASK_NVMCTRL_Pos)                /**< (PM_APBBMASK) NVMCTRL APB Clock Enable Mask */
#define PM_APBBMASK_NVMCTRL(value)            (PM_APBBMASK_NVMCTRL_Msk & ((value) << PM_APBBMASK_NVMCTRL_Pos))
#define PM_APBBMASK_PORT_Pos                  _U_(3)                                               /**< (PM_APBBMASK) PORT APB Clock Enable Position */
#define PM_APBBMASK_PORT_Msk                  (_U_(0x1) << PM_APBBMASK_PORT_Pos)                   /**< (PM_APBBMASK) PORT APB Clock Enable Mask */
#define PM_APBBMASK_PORT(value)               (PM_APBBMASK_PORT_Msk & ((value) << PM_APBBMASK_PORT_Pos))
#define PM_APBBMASK_DMAC_Pos                  _U_(4)                                               /**< (PM_APBBMASK) DMAC APB Clock Enable Position */
#define PM_APBBMASK_DMAC_Msk                  (_U_(0x1) << PM_APBBMASK_DMAC_Pos)                   /**< (PM_APBBMASK) DMAC APB Clock Enable Mask */
#define PM_APBBMASK_DMAC(value)               (PM_APBBMASK_DMAC_Msk & ((value) << PM_APBBMASK_DMAC_Pos))
#define PM_APBBMASK_USB_Pos                   _U_(5)                                               /**< (PM_APBBMASK) USB APB Clock Enable Position */
#define PM_APBBMASK_USB_Msk                   (_U_(0x1) << PM_APBBMASK_USB_Pos)                    /**< (PM_APBBMASK) USB APB Clock Enable Mask */
#define PM_APBBMASK_USB(value)                (PM_APBBMASK_USB_Msk & ((value) << PM_APBBMASK_USB_Pos))
#define PM_APBBMASK_HMATRIX_Pos               _U_(6)                                               /**< (PM_APBBMASK) HMATRIX APB Clock Enable Position */
#define PM_APBBMASK_HMATRIX_Msk               (_U_(0x1) << PM_APBBMASK_HMATRIX_Pos)                /**< (PM_APBBMASK) HMATRIX APB Clock Enable Mask */
#define PM_APBBMASK_HMATRIX(value)            (PM_APBBMASK_HMATRIX_Msk & ((value) << PM_APBBMASK_HMATRIX_Pos))
#define PM_APBBMASK_Msk                       _U_(0x0000007F)                                      /**< (PM_APBBMASK) Register Mask  */

#define PM_APBBMASK_PAC_Pos                   _U_(0)                                               /**< (PM_APBBMASK Position) PACx APB Clock Enable */
#define PM_APBBMASK_PAC_Msk                   (_U_(0x1) << PM_APBBMASK_PAC_Pos)                    /**< (PM_APBBMASK Mask) PAC */
#define PM_APBBMASK_PAC(value)                (PM_APBBMASK_PAC_Msk & ((value) << PM_APBBMASK_PAC_Pos)) 

/* -------- PM_APBCMASK : (PM Offset: 0x20) (R/W 32) APBC Mask -------- */
#define PM_APBCMASK_RESETVALUE                _U_(0x10000)                                         /**<  (PM_APBCMASK) APBC Mask  Reset Value */

#define PM_APBCMASK_PAC2_Pos                  _U_(0)                                               /**< (PM_APBCMASK) PAC2 APB Clock Enable Position */
#define PM_APBCMASK_PAC2_Msk                  (_U_(0x1) << PM_APBCMASK_PAC2_Pos)                   /**< (PM_APBCMASK) PAC2 APB Clock Enable Mask */
#define PM_APBCMASK_PAC2(value)               (PM_APBCMASK_PAC2_Msk & ((value) << PM_APBCMASK_PAC2_Pos))
#define PM_APBCMASK_EVSYS_Pos                 _U_(1)                                               /**< (PM_APBCMASK) EVSYS APB Clock Enable Position */
#define PM_APBCMASK_EVSYS_Msk                 (_U_(0x1) << PM_APBCMASK_EVSYS_Pos)                  /**< (PM_APBCMASK) EVSYS APB Clock Enable Mask */
#define PM_APBCMASK_EVSYS(value)              (PM_APBCMASK_EVSYS_Msk & ((value) << PM_APBCMASK_EVSYS_Pos))
#define PM_APBCMASK_SERCOM0_Pos               _U_(2)                                               /**< (PM_APBCMASK) SERCOM0 APB Clock Enable Position */
#define PM_APBCMASK_SERCOM0_Msk               (_U_(0x1) << PM_APBCMASK_SERCOM0_Pos)                /**< (PM_APBCMASK) SERCOM0 APB Clock Enable Mask */
#define PM_APBCMASK_SERCOM0(value)            (PM_APBCMASK_SERCOM0_Msk & ((value) << PM_APBCMASK_SERCOM0_Pos))
#define PM_APBCMASK_SERCOM1_Pos               _U_(3)                                               /**< (PM_APBCMASK) SERCOM1 APB Clock Enable Position */
#define PM_APBCMASK_SERCOM1_Msk               (_U_(0x1) << PM_APBCMASK_SERCOM1_Pos)                /**< (PM_APBCMASK) SERCOM1 APB Clock Enable Mask */
#define PM_APBCMASK_SERCOM1(value)            (PM_APBCMASK_SERCOM1_Msk & ((value) << PM_APBCMASK_SERCOM1_Pos))
#define PM_APBCMASK_SERCOM2_Pos               _U_(4)                                               /**< (PM_APBCMASK) SERCOM2 APB Clock Enable Position */
#define PM_APBCMASK_SERCOM2_Msk               (_U_(0x1) << PM_APBCMASK_SERCOM2_Pos)                /**< (PM_APBCMASK) SERCOM2 APB Clock Enable Mask */
#define PM_APBCMASK_SERCOM2(value)            (PM_APBCMASK_SERCOM2_Msk & ((value) << PM_APBCMASK_SERCOM2_Pos))
#define PM_APBCMASK_SERCOM3_Pos               _U_(5)                                               /**< (PM_APBCMASK) SERCOM3 APB Clock Enable Position */
#define PM_APBCMASK_SERCOM3_Msk               (_U_(0x1) << PM_APBCMASK_SERCOM3_Pos)                /**< (PM_APBCMASK) SERCOM3 APB Clock Enable Mask */
#define PM_APBCMASK_SERCOM3(value)            (PM_APBCMASK_SERCOM3_Msk & ((value) << PM_APBCMASK_SERCOM3_Pos))
#define PM_APBCMASK_SERCOM4_Pos               _U_(6)                                               /**< (PM_APBCMASK) SERCOM4 APB Clock Enable Position */
#define PM_APBCMASK_SERCOM4_Msk               (_U_(0x1) << PM_APBCMASK_SERCOM4_Pos)                /**< (PM_APBCMASK) SERCOM4 APB Clock Enable Mask */
#define PM_APBCMASK_SERCOM4(value)            (PM_APBCMASK_SERCOM4_Msk & ((value) << PM_APBCMASK_SERCOM4_Pos))
#define PM_APBCMASK_SERCOM5_Pos               _U_(7)                                               /**< (PM_APBCMASK) SERCOM5 APB Clock Enable Position */
#define PM_APBCMASK_SERCOM5_Msk               (_U_(0x1) << PM_APBCMASK_SERCOM5_Pos)                /**< (PM_APBCMASK) SERCOM5 APB Clock Enable Mask */
#define PM_APBCMASK_SERCOM5(value)            (PM_APBCMASK_SERCOM5_Msk & ((value) << PM_APBCMASK_SERCOM5_Pos))
#define PM_APBCMASK_TCC0_Pos                  _U_(8)                                               /**< (PM_APBCMASK) TCC0 APB Clock Enable Position */
#define PM_APBCMASK_TCC0_Msk                  (_U_(0x1) << PM_APBCMASK_TCC0_Pos)                   /**< (PM_APBCMASK) TCC0 APB Clock Enable Mask */
#define PM_APBCMASK_TCC0(value)               (PM_APBCMASK_TCC0_Msk & ((value) << PM_APBCMASK_TCC0_Pos))
#define PM_APBCMASK_TCC1_Pos                  _U_(9)                                               /**< (PM_APBCMASK) TCC1 APB Clock Enable Position */
#define PM_APBCMASK_TCC1_Msk                  (_U_(0x1) << PM_APBCMASK_TCC1_Pos)                   /**< (PM_APBCMASK) TCC1 APB Clock Enable Mask */
#define PM_APBCMASK_TCC1(value)               (PM_APBCMASK_TCC1_Msk & ((value) << PM_APBCMASK_TCC1_Pos))
#define PM_APBCMASK_TCC2_Pos                  _U_(10)                                              /**< (PM_APBCMASK) TCC2 APB Clock Enable Position */
#define PM_APBCMASK_TCC2_Msk                  (_U_(0x1) << PM_APBCMASK_TCC2_Pos)                   /**< (PM_APBCMASK) TCC2 APB Clock Enable Mask */
#define PM_APBCMASK_TCC2(value)               (PM_APBCMASK_TCC2_Msk & ((value) << PM_APBCMASK_TCC2_Pos))
#define PM_APBCMASK_TC3_Pos                   _U_(11)                                              /**< (PM_APBCMASK) TC3 APB Clock Enable Position */
#define PM_APBCMASK_TC3_Msk                   (_U_(0x1) << PM_APBCMASK_TC3_Pos)                    /**< (PM_APBCMASK) TC3 APB Clock Enable Mask */
#define PM_APBCMASK_TC3(value)                (PM_APBCMASK_TC3_Msk & ((value) << PM_APBCMASK_TC3_Pos))
#define PM_APBCMASK_TC4_Pos                   _U_(12)                                              /**< (PM_APBCMASK) TC4 APB Clock Enable Position */
#define PM_APBCMASK_TC4_Msk                   (_U_(0x1) << PM_APBCMASK_TC4_Pos)                    /**< (PM_APBCMASK) TC4 APB Clock Enable Mask */
#define PM_APBCMASK_TC4(value)                (PM_APBCMASK_TC4_Msk & ((value) << PM_APBCMASK_TC4_Pos))
#define PM_APBCMASK_TC5_Pos                   _U_(13)                                              /**< (PM_APBCMASK) TC5 APB Clock Enable Position */
#define PM_APBCMASK_TC5_Msk                   (_U_(0x1) << PM_APBCMASK_TC5_Pos)                    /**< (PM_APBCMASK) TC5 APB Clock Enable Mask */
#define PM_APBCMASK_TC5(value)                (PM_APBCMASK_TC5_Msk & ((value) << PM_APBCMASK_TC5_Pos))
#define PM_APBCMASK_TC6_Pos                   _U_(14)                                              /**< (PM_APBCMASK) TC6 APB Clock Enable Position */
#define PM_APBCMASK_TC6_Msk                   (_U_(0x1) << PM_APBCMASK_TC6_Pos)                    /**< (PM_APBCMASK) TC6 APB Clock Enable Mask */
#define PM_APBCMASK_TC6(value)                (PM_APBCMASK_TC6_Msk & ((value) << PM_APBCMASK_TC6_Pos))
#define PM_APBCMASK_TC7_Pos                   _U_(15)                                              /**< (PM_APBCMASK) TC7 APB Clock Enable Position */
#define PM_APBCMASK_TC7_Msk                   (_U_(0x1) << PM_APBCMASK_TC7_Pos)                    /**< (PM_APBCMASK) TC7 APB Clock Enable Mask */
#define PM_APBCMASK_TC7(value)                (PM_APBCMASK_TC7_Msk & ((value) << PM_APBCMASK_TC7_Pos))
#define PM_APBCMASK_ADC_Pos                   _U_(16)                                              /**< (PM_APBCMASK) ADC APB Clock Enable Position */
#define PM_APBCMASK_ADC_Msk                   (_U_(0x1) << PM_APBCMASK_ADC_Pos)                    /**< (PM_APBCMASK) ADC APB Clock Enable Mask */
#define PM_APBCMASK_ADC(value)                (PM_APBCMASK_ADC_Msk & ((value) << PM_APBCMASK_ADC_Pos))
#define PM_APBCMASK_AC_Pos                    _U_(17)                                              /**< (PM_APBCMASK) AC APB Clock Enable Position */
#define PM_APBCMASK_AC_Msk                    (_U_(0x1) << PM_APBCMASK_AC_Pos)                     /**< (PM_APBCMASK) AC APB Clock Enable Mask */
#define PM_APBCMASK_AC(value)                 (PM_APBCMASK_AC_Msk & ((value) << PM_APBCMASK_AC_Pos))
#define PM_APBCMASK_DAC_Pos                   _U_(18)                                              /**< (PM_APBCMASK) DAC APB Clock Enable Position */
#define PM_APBCMASK_DAC_Msk                   (_U_(0x1) << PM_APBCMASK_DAC_Pos)                    /**< (PM_APBCMASK) DAC APB Clock Enable Mask */
#define PM_APBCMASK_DAC(value)                (PM_APBCMASK_DAC_Msk & ((value) << PM_APBCMASK_DAC_Pos))
#define PM_APBCMASK_PTC_Pos                   _U_(19)                                              /**< (PM_APBCMASK) PTC APB Clock Enable Position */
#define PM_APBCMASK_PTC_Msk                   (_U_(0x1) << PM_APBCMASK_PTC_Pos)                    /**< (PM_APBCMASK) PTC APB Clock Enable Mask */
#define PM_APBCMASK_PTC(value)                (PM_APBCMASK_PTC_Msk & ((value) << PM_APBCMASK_PTC_Pos))
#define PM_APBCMASK_I2S_Pos                   _U_(20)                                              /**< (PM_APBCMASK) I2S APB Clock Enable Position */
#define PM_APBCMASK_I2S_Msk                   (_U_(0x1) << PM_APBCMASK_I2S_Pos)                    /**< (PM_APBCMASK) I2S APB Clock Enable Mask */
#define PM_APBCMASK_I2S(value)                (PM_APBCMASK_I2S_Msk & ((value) << PM_APBCMASK_I2S_Pos))
#define PM_APBCMASK_Msk                       _U_(0x001FFFFF)                                      /**< (PM_APBCMASK) Register Mask  */

#define PM_APBCMASK_PAC_Pos                   _U_(0)                                               /**< (PM_APBCMASK Position) PAC2 APB Clock Enable */
#define PM_APBCMASK_PAC_Msk                   (_U_(0x1) << PM_APBCMASK_PAC_Pos)                    /**< (PM_APBCMASK Mask) PAC */
#define PM_APBCMASK_PAC(value)                (PM_APBCMASK_PAC_Msk & ((value) << PM_APBCMASK_PAC_Pos)) 
#define PM_APBCMASK_SERCOM_Pos                _U_(2)                                               /**< (PM_APBCMASK Position) SERCOMx APB Clock Enable */
#define PM_APBCMASK_SERCOM_Msk                (_U_(0x3F) << PM_APBCMASK_SERCOM_Pos)                /**< (PM_APBCMASK Mask) SERCOM */
#define PM_APBCMASK_SERCOM(value)             (PM_APBCMASK_SERCOM_Msk & ((value) << PM_APBCMASK_SERCOM_Pos)) 
#define PM_APBCMASK_TCC_Pos                   _U_(8)                                               /**< (PM_APBCMASK Position) TCCx APB Clock Enable */
#define PM_APBCMASK_TCC_Msk                   (_U_(0x7) << PM_APBCMASK_TCC_Pos)                    /**< (PM_APBCMASK Mask) TCC */
#define PM_APBCMASK_TCC(value)                (PM_APBCMASK_TCC_Msk & ((value) << PM_APBCMASK_TCC_Pos)) 
#define PM_APBCMASK_TC_Pos                    _U_(11)                                              /**< (PM_APBCMASK Position) TC3 APB Clock Enable */
#define PM_APBCMASK_TC_Msk                    (_U_(0x1F) << PM_APBCMASK_TC_Pos)                    /**< (PM_APBCMASK Mask) TC */
#define PM_APBCMASK_TC(value)                 (PM_APBCMASK_TC_Msk & ((value) << PM_APBCMASK_TC_Pos)) 

/* -------- PM_INTENCLR : (PM Offset: 0x34) (R/W 8) Interrupt Enable Clear -------- */
#define PM_INTENCLR_RESETVALUE                _U_(0x00)                                            /**<  (PM_INTENCLR) Interrupt Enable Clear  Reset Value */

#define PM_INTENCLR_CKRDY_Pos                 _U_(0)                                               /**< (PM_INTENCLR) Clock Ready Interrupt Enable Position */
#define PM_INTENCLR_CKRDY_Msk                 (_U_(0x1) << PM_INTENCLR_CKRDY_Pos)                  /**< (PM_INTENCLR) Clock Ready Interrupt Enable Mask */
#define PM_INTENCLR_CKRDY(value)              (PM_INTENCLR_CKRDY_Msk & ((value) << PM_INTENCLR_CKRDY_Pos))
#define PM_INTENCLR_Msk                       _U_(0x01)                                            /**< (PM_INTENCLR) Register Mask  */


/* -------- PM_INTENSET : (PM Offset: 0x35) (R/W 8) Interrupt Enable Set -------- */
#define PM_INTENSET_RESETVALUE                _U_(0x00)                                            /**<  (PM_INTENSET) Interrupt Enable Set  Reset Value */

#define PM_INTENSET_CKRDY_Pos                 _U_(0)                                               /**< (PM_INTENSET) Clock Ready Interrupt Enable Position */
#define PM_INTENSET_CKRDY_Msk                 (_U_(0x1) << PM_INTENSET_CKRDY_Pos)                  /**< (PM_INTENSET) Clock Ready Interrupt Enable Mask */
#define PM_INTENSET_CKRDY(value)              (PM_INTENSET_CKRDY_Msk & ((value) << PM_INTENSET_CKRDY_Pos))
#define PM_INTENSET_Msk                       _U_(0x01)                                            /**< (PM_INTENSET) Register Mask  */


/* -------- PM_INTFLAG : (PM Offset: 0x36) (R/W 8) Interrupt Flag Status and Clear -------- */
#define PM_INTFLAG_RESETVALUE                 _U_(0x00)                                            /**<  (PM_INTFLAG) Interrupt Flag Status and Clear  Reset Value */

#define PM_INTFLAG_CKRDY_Pos                  _U_(0)                                               /**< (PM_INTFLAG) Clock Ready Position */
#define PM_INTFLAG_CKRDY_Msk                  (_U_(0x1) << PM_INTFLAG_CKRDY_Pos)                   /**< (PM_INTFLAG) Clock Ready Mask */
#define PM_INTFLAG_CKRDY(value)               (PM_INTFLAG_CKRDY_Msk & ((value) << PM_INTFLAG_CKRDY_Pos))
#define PM_INTFLAG_Msk                        _U_(0x01)                                            /**< (PM_INTFLAG) Register Mask  */


/* -------- PM_RCAUSE : (PM Offset: 0x38) ( R/ 8) Reset Cause -------- */
#define PM_RCAUSE_RESETVALUE                  _U_(0x01)                                            /**<  (PM_RCAUSE) Reset Cause  Reset Value */

#define PM_RCAUSE_POR_Pos                     _U_(0)                                               /**< (PM_RCAUSE) Power On Reset Position */
#define PM_RCAUSE_POR_Msk                     (_U_(0x1) << PM_RCAUSE_POR_Pos)                      /**< (PM_RCAUSE) Power On Reset Mask */
#define PM_RCAUSE_POR(value)                  (PM_RCAUSE_POR_Msk & ((value) << PM_RCAUSE_POR_Pos))
#define PM_RCAUSE_BOD12_Pos                   _U_(1)                                               /**< (PM_RCAUSE) Brown Out 12 Detector Reset Position */
#define PM_RCAUSE_BOD12_Msk                   (_U_(0x1) << PM_RCAUSE_BOD12_Pos)                    /**< (PM_RCAUSE) Brown Out 12 Detector Reset Mask */
#define PM_RCAUSE_BOD12(value)                (PM_RCAUSE_BOD12_Msk & ((value) << PM_RCAUSE_BOD12_Pos))
#define PM_RCAUSE_BOD33_Pos                   _U_(2)                                               /**< (PM_RCAUSE) Brown Out 33 Detector Reset Position */
#define PM_RCAUSE_BOD33_Msk                   (_U_(0x1) << PM_RCAUSE_BOD33_Pos)                    /**< (PM_RCAUSE) Brown Out 33 Detector Reset Mask */
#define PM_RCAUSE_BOD33(value)                (PM_RCAUSE_BOD33_Msk & ((value) << PM_RCAUSE_BOD33_Pos))
#define PM_RCAUSE_EXT_Pos                     _U_(4)                                               /**< (PM_RCAUSE) External Reset Position */
#define PM_RCAUSE_EXT_Msk                     (_U_(0x1) << PM_RCAUSE_EXT_Pos)                      /**< (PM_RCAUSE) External Reset Mask */
#define PM_RCAUSE_EXT(value)                  (PM_RCAUSE_EXT_Msk & ((value) << PM_RCAUSE_EXT_Pos))
#define PM_RCAUSE_WDT_Pos                     _U_(5)                                               /**< (PM_RCAUSE) Watchdog Reset Position */
#define PM_RCAUSE_WDT_Msk                     (_U_(0x1) << PM_RCAUSE_WDT_Pos)                      /**< (PM_RCAUSE) Watchdog Reset Mask */
#define PM_RCAUSE_WDT(value)                  (PM_RCAUSE_WDT_Msk & ((value) << PM_RCAUSE_WDT_Pos))
#define PM_RCAUSE_SYST_Pos                    _U_(6)                                               /**< (PM_RCAUSE) System Reset Request Position */
#define PM_RCAUSE_SYST_Msk                    (_U_(0x1) << PM_RCAUSE_SYST_Pos)                     /**< (PM_RCAUSE) System Reset Request Mask */
#define PM_RCAUSE_SYST(value)                 (PM_RCAUSE_SYST_Msk & ((value) << PM_RCAUSE_SYST_Pos))
#define PM_RCAUSE_Msk                         _U_(0x77)                                            /**< (PM_RCAUSE) Register Mask  */

#define PM_RCAUSE_BOD_Pos                     _U_(1)                                               /**< (PM_RCAUSE Position) Brown Out x2 Detector Reset */
#define PM_RCAUSE_BOD_Msk                     (_U_(0x3) << PM_RCAUSE_BOD_Pos)                      /**< (PM_RCAUSE Mask) BOD */
#define PM_RCAUSE_BOD(value)                  (PM_RCAUSE_BOD_Msk & ((value) << PM_RCAUSE_BOD_Pos)) 

/** \brief PM register offsets definitions */
#define PM_CTRL_REG_OFST               (0x00)              /**< (PM_CTRL) Control Offset */
#define PM_SLEEP_REG_OFST              (0x01)              /**< (PM_SLEEP) Sleep Mode Offset */
#define PM_CPUSEL_REG_OFST             (0x08)              /**< (PM_CPUSEL) CPU Clock Select Offset */
#define PM_APBASEL_REG_OFST            (0x09)              /**< (PM_APBASEL) APBA Clock Select Offset */
#define PM_APBBSEL_REG_OFST            (0x0A)              /**< (PM_APBBSEL) APBB Clock Select Offset */
#define PM_APBCSEL_REG_OFST            (0x0B)              /**< (PM_APBCSEL) APBC Clock Select Offset */
#define PM_AHBMASK_REG_OFST            (0x14)              /**< (PM_AHBMASK) AHB Mask Offset */
#define PM_APBAMASK_REG_OFST           (0x18)              /**< (PM_APBAMASK) APBA Mask Offset */
#define PM_APBBMASK_REG_OFST           (0x1C)              /**< (PM_APBBMASK) APBB Mask Offset */
#define PM_APBCMASK_REG_OFST           (0x20)              /**< (PM_APBCMASK) APBC Mask Offset */
#define PM_INTENCLR_REG_OFST           (0x34)              /**< (PM_INTENCLR) Interrupt Enable Clear Offset */
#define PM_INTENSET_REG_OFST           (0x35)              /**< (PM_INTENSET) Interrupt Enable Set Offset */
#define PM_INTFLAG_REG_OFST            (0x36)              /**< (PM_INTFLAG) Interrupt Flag Status and Clear Offset */
#define PM_RCAUSE_REG_OFST             (0x38)              /**< (PM_RCAUSE) Reset Cause Offset */

#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
/** \brief PM register API structure */
typedef struct
{  /* Power Manager */
  __IO  uint8_t                        PM_CTRL;            /**< Offset: 0x00 (R/W  8) Control */
  __IO  uint8_t                        PM_SLEEP;           /**< Offset: 0x01 (R/W  8) Sleep Mode */
  __I   uint8_t                        Reserved1[0x06];
  __IO  uint8_t                        PM_CPUSEL;          /**< Offset: 0x08 (R/W  8) CPU Clock Select */
  __IO  uint8_t                        PM_APBASEL;         /**< Offset: 0x09 (R/W  8) APBA Clock Select */
  __IO  uint8_t                        PM_APBBSEL;         /**< Offset: 0x0A (R/W  8) APBB Clock Select */
  __IO  uint8_t                        PM_APBCSEL;         /**< Offset: 0x0B (R/W  8) APBC Clock Select */
  __I   uint8_t                        Reserved2[0x08];
  __IO  uint32_t                       PM_AHBMASK;         /**< Offset: 0x14 (R/W  32) AHB Mask */
  __IO  uint32_t                       PM_APBAMASK;        /**< Offset: 0x18 (R/W  32) APBA Mask */
  __IO  uint32_t                       PM_APBBMASK;        /**< Offset: 0x1C (R/W  32) APBB Mask */
  __IO  uint32_t                       PM_APBCMASK;        /**< Offset: 0x20 (R/W  32) APBC Mask */
  __I   uint8_t                        Reserved3[0x10];
  __IO  uint8_t                        PM_INTENCLR;        /**< Offset: 0x34 (R/W  8) Interrupt Enable Clear */
  __IO  uint8_t                        PM_INTENSET;        /**< Offset: 0x35 (R/W  8) Interrupt Enable Set */
  __IO  uint8_t                        PM_INTFLAG;         /**< Offset: 0x36 (R/W  8) Interrupt Flag Status and Clear */
  __I   uint8_t                        Reserved4[0x01];
  __I   uint8_t                        PM_RCAUSE;          /**< Offset: 0x38 (R/   8) Reset Cause */
} pm_registers_t;


#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
#endif /* _SAMD21_PM_COMPONENT_H_ */
