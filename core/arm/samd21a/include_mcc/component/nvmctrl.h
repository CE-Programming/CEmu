/**
 * \brief Component description for NVMCTRL
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
#ifndef _SAMD21_NVMCTRL_COMPONENT_H_
#define _SAMD21_NVMCTRL_COMPONENT_H_

/* ************************************************************************** */
/*   SOFTWARE API DEFINITION FOR NVMCTRL                                      */
/* ************************************************************************** */

/* -------- NVMCTRL_CTRLA : (NVMCTRL Offset: 0x00) (R/W 16) Control A -------- */
#define NVMCTRL_CTRLA_RESETVALUE              _U_(0x00)                                            /**<  (NVMCTRL_CTRLA) Control A  Reset Value */

#define NVMCTRL_CTRLA_CMD_Pos                 _U_(0)                                               /**< (NVMCTRL_CTRLA) Command Position */
#define NVMCTRL_CTRLA_CMD_Msk                 (_U_(0x7F) << NVMCTRL_CTRLA_CMD_Pos)                 /**< (NVMCTRL_CTRLA) Command Mask */
#define NVMCTRL_CTRLA_CMD(value)              (NVMCTRL_CTRLA_CMD_Msk & ((value) << NVMCTRL_CTRLA_CMD_Pos))
#define   NVMCTRL_CTRLA_CMD_ER_Val            _U_(0x2)                                             /**< (NVMCTRL_CTRLA) Erase Row - Erases the row addressed by the ADDR register.  */
#define   NVMCTRL_CTRLA_CMD_WP_Val            _U_(0x4)                                             /**< (NVMCTRL_CTRLA) Write Page - Writes the contents of the page buffer to the page addressed by the ADDR register.  */
#define   NVMCTRL_CTRLA_CMD_EAR_Val           _U_(0x5)                                             /**< (NVMCTRL_CTRLA) Erase Auxiliary Row - Erases the auxiliary row addressed by the ADDR register. This command can be given only when the security bit is not set and only to the user configuration row.  */
#define   NVMCTRL_CTRLA_CMD_WAP_Val           _U_(0x6)                                             /**< (NVMCTRL_CTRLA) Write Auxiliary Page - Writes the contents of the page buffer to the page addressed by the ADDR register. This command can be given only when the security bit is not set and only to the user configuration row.  */
#define   NVMCTRL_CTRLA_CMD_SF_Val            _U_(0xA)                                             /**< (NVMCTRL_CTRLA) Security Flow Command  */
#define   NVMCTRL_CTRLA_CMD_WL_Val            _U_(0xF)                                             /**< (NVMCTRL_CTRLA) Write lockbits  */
#define   NVMCTRL_CTRLA_CMD_LR_Val            _U_(0x40)                                            /**< (NVMCTRL_CTRLA) Lock Region - Locks the region containing the address location in the ADDR register.  */
#define   NVMCTRL_CTRLA_CMD_UR_Val            _U_(0x41)                                            /**< (NVMCTRL_CTRLA) Unlock Region - Unlocks the region containing the address location in the ADDR register.  */
#define   NVMCTRL_CTRLA_CMD_SPRM_Val          _U_(0x42)                                            /**< (NVMCTRL_CTRLA) Sets the power reduction mode.  */
#define   NVMCTRL_CTRLA_CMD_CPRM_Val          _U_(0x43)                                            /**< (NVMCTRL_CTRLA) Clears the power reduction mode.  */
#define   NVMCTRL_CTRLA_CMD_PBC_Val           _U_(0x44)                                            /**< (NVMCTRL_CTRLA) Page Buffer Clear - Clears the page buffer.  */
#define   NVMCTRL_CTRLA_CMD_SSB_Val           _U_(0x45)                                            /**< (NVMCTRL_CTRLA) Set Security Bit - Sets the security bit by writing 0x00 to the first byte in the lockbit row.  */
#define   NVMCTRL_CTRLA_CMD_INVALL_Val        _U_(0x46)                                            /**< (NVMCTRL_CTRLA) Invalidates all cache lines.  */
#define NVMCTRL_CTRLA_CMD_ER                  (NVMCTRL_CTRLA_CMD_ER_Val << NVMCTRL_CTRLA_CMD_Pos)  /**< (NVMCTRL_CTRLA) Erase Row - Erases the row addressed by the ADDR register. Position  */
#define NVMCTRL_CTRLA_CMD_WP                  (NVMCTRL_CTRLA_CMD_WP_Val << NVMCTRL_CTRLA_CMD_Pos)  /**< (NVMCTRL_CTRLA) Write Page - Writes the contents of the page buffer to the page addressed by the ADDR register. Position  */
#define NVMCTRL_CTRLA_CMD_EAR                 (NVMCTRL_CTRLA_CMD_EAR_Val << NVMCTRL_CTRLA_CMD_Pos) /**< (NVMCTRL_CTRLA) Erase Auxiliary Row - Erases the auxiliary row addressed by the ADDR register. This command can be given only when the security bit is not set and only to the user configuration row. Position  */
#define NVMCTRL_CTRLA_CMD_WAP                 (NVMCTRL_CTRLA_CMD_WAP_Val << NVMCTRL_CTRLA_CMD_Pos) /**< (NVMCTRL_CTRLA) Write Auxiliary Page - Writes the contents of the page buffer to the page addressed by the ADDR register. This command can be given only when the security bit is not set and only to the user configuration row. Position  */
#define NVMCTRL_CTRLA_CMD_SF                  (NVMCTRL_CTRLA_CMD_SF_Val << NVMCTRL_CTRLA_CMD_Pos)  /**< (NVMCTRL_CTRLA) Security Flow Command Position  */
#define NVMCTRL_CTRLA_CMD_WL                  (NVMCTRL_CTRLA_CMD_WL_Val << NVMCTRL_CTRLA_CMD_Pos)  /**< (NVMCTRL_CTRLA) Write lockbits Position  */
#define NVMCTRL_CTRLA_CMD_LR                  (NVMCTRL_CTRLA_CMD_LR_Val << NVMCTRL_CTRLA_CMD_Pos)  /**< (NVMCTRL_CTRLA) Lock Region - Locks the region containing the address location in the ADDR register. Position  */
#define NVMCTRL_CTRLA_CMD_UR                  (NVMCTRL_CTRLA_CMD_UR_Val << NVMCTRL_CTRLA_CMD_Pos)  /**< (NVMCTRL_CTRLA) Unlock Region - Unlocks the region containing the address location in the ADDR register. Position  */
#define NVMCTRL_CTRLA_CMD_SPRM                (NVMCTRL_CTRLA_CMD_SPRM_Val << NVMCTRL_CTRLA_CMD_Pos) /**< (NVMCTRL_CTRLA) Sets the power reduction mode. Position  */
#define NVMCTRL_CTRLA_CMD_CPRM                (NVMCTRL_CTRLA_CMD_CPRM_Val << NVMCTRL_CTRLA_CMD_Pos) /**< (NVMCTRL_CTRLA) Clears the power reduction mode. Position  */
#define NVMCTRL_CTRLA_CMD_PBC                 (NVMCTRL_CTRLA_CMD_PBC_Val << NVMCTRL_CTRLA_CMD_Pos) /**< (NVMCTRL_CTRLA) Page Buffer Clear - Clears the page buffer. Position  */
#define NVMCTRL_CTRLA_CMD_SSB                 (NVMCTRL_CTRLA_CMD_SSB_Val << NVMCTRL_CTRLA_CMD_Pos) /**< (NVMCTRL_CTRLA) Set Security Bit - Sets the security bit by writing 0x00 to the first byte in the lockbit row. Position  */
#define NVMCTRL_CTRLA_CMD_INVALL              (NVMCTRL_CTRLA_CMD_INVALL_Val << NVMCTRL_CTRLA_CMD_Pos) /**< (NVMCTRL_CTRLA) Invalidates all cache lines. Position  */
#define NVMCTRL_CTRLA_CMDEX_Pos               _U_(8)                                               /**< (NVMCTRL_CTRLA) Command Execution Position */
#define NVMCTRL_CTRLA_CMDEX_Msk               (_U_(0xFF) << NVMCTRL_CTRLA_CMDEX_Pos)               /**< (NVMCTRL_CTRLA) Command Execution Mask */
#define NVMCTRL_CTRLA_CMDEX(value)            (NVMCTRL_CTRLA_CMDEX_Msk & ((value) << NVMCTRL_CTRLA_CMDEX_Pos))
#define   NVMCTRL_CTRLA_CMDEX_KEY_Val         _U_(0xA5)                                            /**< (NVMCTRL_CTRLA) Execution Key  */
#define NVMCTRL_CTRLA_CMDEX_KEY               (NVMCTRL_CTRLA_CMDEX_KEY_Val << NVMCTRL_CTRLA_CMDEX_Pos) /**< (NVMCTRL_CTRLA) Execution Key Position  */
#define NVMCTRL_CTRLA_Msk                     _U_(0xFF7F)                                          /**< (NVMCTRL_CTRLA) Register Mask  */


/* -------- NVMCTRL_CTRLB : (NVMCTRL Offset: 0x04) (R/W 32) Control B -------- */
#define NVMCTRL_CTRLB_RESETVALUE              _U_(0x00)                                            /**<  (NVMCTRL_CTRLB) Control B  Reset Value */

#define NVMCTRL_CTRLB_RWS_Pos                 _U_(1)                                               /**< (NVMCTRL_CTRLB) NVM Read Wait States Position */
#define NVMCTRL_CTRLB_RWS_Msk                 (_U_(0xF) << NVMCTRL_CTRLB_RWS_Pos)                  /**< (NVMCTRL_CTRLB) NVM Read Wait States Mask */
#define NVMCTRL_CTRLB_RWS(value)              (NVMCTRL_CTRLB_RWS_Msk & ((value) << NVMCTRL_CTRLB_RWS_Pos))
#define   NVMCTRL_CTRLB_RWS_SINGLE_Val        _U_(0x0)                                             /**< (NVMCTRL_CTRLB) Single Auto Wait State  */
#define   NVMCTRL_CTRLB_RWS_HALF_Val          _U_(0x1)                                             /**< (NVMCTRL_CTRLB) Half Auto Wait State  */
#define   NVMCTRL_CTRLB_RWS_DUAL_Val          _U_(0x2)                                             /**< (NVMCTRL_CTRLB) Dual Auto Wait State  */
#define NVMCTRL_CTRLB_RWS_SINGLE              (NVMCTRL_CTRLB_RWS_SINGLE_Val << NVMCTRL_CTRLB_RWS_Pos) /**< (NVMCTRL_CTRLB) Single Auto Wait State Position  */
#define NVMCTRL_CTRLB_RWS_HALF                (NVMCTRL_CTRLB_RWS_HALF_Val << NVMCTRL_CTRLB_RWS_Pos) /**< (NVMCTRL_CTRLB) Half Auto Wait State Position  */
#define NVMCTRL_CTRLB_RWS_DUAL                (NVMCTRL_CTRLB_RWS_DUAL_Val << NVMCTRL_CTRLB_RWS_Pos) /**< (NVMCTRL_CTRLB) Dual Auto Wait State Position  */
#define NVMCTRL_CTRLB_MANW_Pos                _U_(7)                                               /**< (NVMCTRL_CTRLB) Manual Write Position */
#define NVMCTRL_CTRLB_MANW_Msk                (_U_(0x1) << NVMCTRL_CTRLB_MANW_Pos)                 /**< (NVMCTRL_CTRLB) Manual Write Mask */
#define NVMCTRL_CTRLB_MANW(value)             (NVMCTRL_CTRLB_MANW_Msk & ((value) << NVMCTRL_CTRLB_MANW_Pos))
#define NVMCTRL_CTRLB_SLEEPPRM_Pos            _U_(8)                                               /**< (NVMCTRL_CTRLB) Power Reduction Mode during Sleep Position */
#define NVMCTRL_CTRLB_SLEEPPRM_Msk            (_U_(0x3) << NVMCTRL_CTRLB_SLEEPPRM_Pos)             /**< (NVMCTRL_CTRLB) Power Reduction Mode during Sleep Mask */
#define NVMCTRL_CTRLB_SLEEPPRM(value)         (NVMCTRL_CTRLB_SLEEPPRM_Msk & ((value) << NVMCTRL_CTRLB_SLEEPPRM_Pos))
#define   NVMCTRL_CTRLB_SLEEPPRM_WAKEONACCESS_Val _U_(0x0)                                             /**< (NVMCTRL_CTRLB) NVM block enters low-power mode when entering sleep.NVM block exits low-power mode upon first access.  */
#define   NVMCTRL_CTRLB_SLEEPPRM_WAKEUPINSTANT_Val _U_(0x1)                                             /**< (NVMCTRL_CTRLB) NVM block enters low-power mode when entering sleep.NVM block exits low-power mode when exiting sleep.  */
#define   NVMCTRL_CTRLB_SLEEPPRM_DISABLED_Val _U_(0x3)                                             /**< (NVMCTRL_CTRLB) Auto power reduction disabled.  */
#define NVMCTRL_CTRLB_SLEEPPRM_WAKEONACCESS   (NVMCTRL_CTRLB_SLEEPPRM_WAKEONACCESS_Val << NVMCTRL_CTRLB_SLEEPPRM_Pos) /**< (NVMCTRL_CTRLB) NVM block enters low-power mode when entering sleep.NVM block exits low-power mode upon first access. Position  */
#define NVMCTRL_CTRLB_SLEEPPRM_WAKEUPINSTANT  (NVMCTRL_CTRLB_SLEEPPRM_WAKEUPINSTANT_Val << NVMCTRL_CTRLB_SLEEPPRM_Pos) /**< (NVMCTRL_CTRLB) NVM block enters low-power mode when entering sleep.NVM block exits low-power mode when exiting sleep. Position  */
#define NVMCTRL_CTRLB_SLEEPPRM_DISABLED       (NVMCTRL_CTRLB_SLEEPPRM_DISABLED_Val << NVMCTRL_CTRLB_SLEEPPRM_Pos) /**< (NVMCTRL_CTRLB) Auto power reduction disabled. Position  */
#define NVMCTRL_CTRLB_READMODE_Pos            _U_(16)                                              /**< (NVMCTRL_CTRLB) NVMCTRL Read Mode Position */
#define NVMCTRL_CTRLB_READMODE_Msk            (_U_(0x3) << NVMCTRL_CTRLB_READMODE_Pos)             /**< (NVMCTRL_CTRLB) NVMCTRL Read Mode Mask */
#define NVMCTRL_CTRLB_READMODE(value)         (NVMCTRL_CTRLB_READMODE_Msk & ((value) << NVMCTRL_CTRLB_READMODE_Pos))
#define   NVMCTRL_CTRLB_READMODE_NO_MISS_PENALTY_Val _U_(0x0)                                             /**< (NVMCTRL_CTRLB) The NVM Controller (cache system) does not insert wait states on a cache miss. Gives the best system performance.  */
#define   NVMCTRL_CTRLB_READMODE_LOW_POWER_Val _U_(0x1)                                             /**< (NVMCTRL_CTRLB) Reduces power consumption of the cache system, but inserts a wait state each time there is a cache miss. This mode may not be relevant if CPU performance is required, as the application will be stalled and may lead to increase run time.  */
#define   NVMCTRL_CTRLB_READMODE_DETERMINISTIC_Val _U_(0x2)                                             /**< (NVMCTRL_CTRLB) The cache system ensures that a cache hit or miss takes the same amount of time, determined by the number of programmed flash wait states. This mode can be used for real-time applications that require deterministic execution timings.  */
#define NVMCTRL_CTRLB_READMODE_NO_MISS_PENALTY (NVMCTRL_CTRLB_READMODE_NO_MISS_PENALTY_Val << NVMCTRL_CTRLB_READMODE_Pos) /**< (NVMCTRL_CTRLB) The NVM Controller (cache system) does not insert wait states on a cache miss. Gives the best system performance. Position  */
#define NVMCTRL_CTRLB_READMODE_LOW_POWER      (NVMCTRL_CTRLB_READMODE_LOW_POWER_Val << NVMCTRL_CTRLB_READMODE_Pos) /**< (NVMCTRL_CTRLB) Reduces power consumption of the cache system, but inserts a wait state each time there is a cache miss. This mode may not be relevant if CPU performance is required, as the application will be stalled and may lead to increase run time. Position  */
#define NVMCTRL_CTRLB_READMODE_DETERMINISTIC  (NVMCTRL_CTRLB_READMODE_DETERMINISTIC_Val << NVMCTRL_CTRLB_READMODE_Pos) /**< (NVMCTRL_CTRLB) The cache system ensures that a cache hit or miss takes the same amount of time, determined by the number of programmed flash wait states. This mode can be used for real-time applications that require deterministic execution timings. Position  */
#define NVMCTRL_CTRLB_CACHEDIS_Pos            _U_(18)                                              /**< (NVMCTRL_CTRLB) Cache Disable Position */
#define NVMCTRL_CTRLB_CACHEDIS_Msk            (_U_(0x1) << NVMCTRL_CTRLB_CACHEDIS_Pos)             /**< (NVMCTRL_CTRLB) Cache Disable Mask */
#define NVMCTRL_CTRLB_CACHEDIS(value)         (NVMCTRL_CTRLB_CACHEDIS_Msk & ((value) << NVMCTRL_CTRLB_CACHEDIS_Pos))
#define NVMCTRL_CTRLB_Msk                     _U_(0x0007039E)                                      /**< (NVMCTRL_CTRLB) Register Mask  */


/* -------- NVMCTRL_PARAM : (NVMCTRL Offset: 0x08) (R/W 32) NVM Parameter -------- */
#define NVMCTRL_PARAM_RESETVALUE              _U_(0x00)                                            /**<  (NVMCTRL_PARAM) NVM Parameter  Reset Value */

#define NVMCTRL_PARAM_NVMP_Pos                _U_(0)                                               /**< (NVMCTRL_PARAM) NVM Pages Position */
#define NVMCTRL_PARAM_NVMP_Msk                (_U_(0xFFFF) << NVMCTRL_PARAM_NVMP_Pos)              /**< (NVMCTRL_PARAM) NVM Pages Mask */
#define NVMCTRL_PARAM_NVMP(value)             (NVMCTRL_PARAM_NVMP_Msk & ((value) << NVMCTRL_PARAM_NVMP_Pos))
#define NVMCTRL_PARAM_PSZ_Pos                 _U_(16)                                              /**< (NVMCTRL_PARAM) Page Size Position */
#define NVMCTRL_PARAM_PSZ_Msk                 (_U_(0x7) << NVMCTRL_PARAM_PSZ_Pos)                  /**< (NVMCTRL_PARAM) Page Size Mask */
#define NVMCTRL_PARAM_PSZ(value)              (NVMCTRL_PARAM_PSZ_Msk & ((value) << NVMCTRL_PARAM_PSZ_Pos))
#define   NVMCTRL_PARAM_PSZ_8_Val             _U_(0x0)                                             /**< (NVMCTRL_PARAM) 8 bytes  */
#define   NVMCTRL_PARAM_PSZ_16_Val            _U_(0x1)                                             /**< (NVMCTRL_PARAM) 16 bytes  */
#define   NVMCTRL_PARAM_PSZ_32_Val            _U_(0x2)                                             /**< (NVMCTRL_PARAM) 32 bytes  */
#define   NVMCTRL_PARAM_PSZ_64_Val            _U_(0x3)                                             /**< (NVMCTRL_PARAM) 64 bytes  */
#define   NVMCTRL_PARAM_PSZ_128_Val           _U_(0x4)                                             /**< (NVMCTRL_PARAM) 128 bytes  */
#define   NVMCTRL_PARAM_PSZ_256_Val           _U_(0x5)                                             /**< (NVMCTRL_PARAM) 256 bytes  */
#define   NVMCTRL_PARAM_PSZ_512_Val           _U_(0x6)                                             /**< (NVMCTRL_PARAM) 512 bytes  */
#define   NVMCTRL_PARAM_PSZ_1024_Val          _U_(0x7)                                             /**< (NVMCTRL_PARAM) 1024 bytes  */
#define NVMCTRL_PARAM_PSZ_8                   (NVMCTRL_PARAM_PSZ_8_Val << NVMCTRL_PARAM_PSZ_Pos)   /**< (NVMCTRL_PARAM) 8 bytes Position  */
#define NVMCTRL_PARAM_PSZ_16                  (NVMCTRL_PARAM_PSZ_16_Val << NVMCTRL_PARAM_PSZ_Pos)  /**< (NVMCTRL_PARAM) 16 bytes Position  */
#define NVMCTRL_PARAM_PSZ_32                  (NVMCTRL_PARAM_PSZ_32_Val << NVMCTRL_PARAM_PSZ_Pos)  /**< (NVMCTRL_PARAM) 32 bytes Position  */
#define NVMCTRL_PARAM_PSZ_64                  (NVMCTRL_PARAM_PSZ_64_Val << NVMCTRL_PARAM_PSZ_Pos)  /**< (NVMCTRL_PARAM) 64 bytes Position  */
#define NVMCTRL_PARAM_PSZ_128                 (NVMCTRL_PARAM_PSZ_128_Val << NVMCTRL_PARAM_PSZ_Pos) /**< (NVMCTRL_PARAM) 128 bytes Position  */
#define NVMCTRL_PARAM_PSZ_256                 (NVMCTRL_PARAM_PSZ_256_Val << NVMCTRL_PARAM_PSZ_Pos) /**< (NVMCTRL_PARAM) 256 bytes Position  */
#define NVMCTRL_PARAM_PSZ_512                 (NVMCTRL_PARAM_PSZ_512_Val << NVMCTRL_PARAM_PSZ_Pos) /**< (NVMCTRL_PARAM) 512 bytes Position  */
#define NVMCTRL_PARAM_PSZ_1024                (NVMCTRL_PARAM_PSZ_1024_Val << NVMCTRL_PARAM_PSZ_Pos) /**< (NVMCTRL_PARAM) 1024 bytes Position  */
#define NVMCTRL_PARAM_Msk                     _U_(0x0007FFFF)                                      /**< (NVMCTRL_PARAM) Register Mask  */


/* -------- NVMCTRL_INTENCLR : (NVMCTRL Offset: 0x0C) (R/W 8) Interrupt Enable Clear -------- */
#define NVMCTRL_INTENCLR_RESETVALUE           _U_(0x00)                                            /**<  (NVMCTRL_INTENCLR) Interrupt Enable Clear  Reset Value */

#define NVMCTRL_INTENCLR_READY_Pos            _U_(0)                                               /**< (NVMCTRL_INTENCLR) NVM Ready Interrupt Enable Position */
#define NVMCTRL_INTENCLR_READY_Msk            (_U_(0x1) << NVMCTRL_INTENCLR_READY_Pos)             /**< (NVMCTRL_INTENCLR) NVM Ready Interrupt Enable Mask */
#define NVMCTRL_INTENCLR_READY(value)         (NVMCTRL_INTENCLR_READY_Msk & ((value) << NVMCTRL_INTENCLR_READY_Pos))
#define NVMCTRL_INTENCLR_ERROR_Pos            _U_(1)                                               /**< (NVMCTRL_INTENCLR) Error Interrupt Enable Position */
#define NVMCTRL_INTENCLR_ERROR_Msk            (_U_(0x1) << NVMCTRL_INTENCLR_ERROR_Pos)             /**< (NVMCTRL_INTENCLR) Error Interrupt Enable Mask */
#define NVMCTRL_INTENCLR_ERROR(value)         (NVMCTRL_INTENCLR_ERROR_Msk & ((value) << NVMCTRL_INTENCLR_ERROR_Pos))
#define NVMCTRL_INTENCLR_Msk                  _U_(0x03)                                            /**< (NVMCTRL_INTENCLR) Register Mask  */


/* -------- NVMCTRL_INTENSET : (NVMCTRL Offset: 0x10) (R/W 8) Interrupt Enable Set -------- */
#define NVMCTRL_INTENSET_RESETVALUE           _U_(0x00)                                            /**<  (NVMCTRL_INTENSET) Interrupt Enable Set  Reset Value */

#define NVMCTRL_INTENSET_READY_Pos            _U_(0)                                               /**< (NVMCTRL_INTENSET) NVM Ready Interrupt Enable Position */
#define NVMCTRL_INTENSET_READY_Msk            (_U_(0x1) << NVMCTRL_INTENSET_READY_Pos)             /**< (NVMCTRL_INTENSET) NVM Ready Interrupt Enable Mask */
#define NVMCTRL_INTENSET_READY(value)         (NVMCTRL_INTENSET_READY_Msk & ((value) << NVMCTRL_INTENSET_READY_Pos))
#define NVMCTRL_INTENSET_ERROR_Pos            _U_(1)                                               /**< (NVMCTRL_INTENSET) Error Interrupt Enable Position */
#define NVMCTRL_INTENSET_ERROR_Msk            (_U_(0x1) << NVMCTRL_INTENSET_ERROR_Pos)             /**< (NVMCTRL_INTENSET) Error Interrupt Enable Mask */
#define NVMCTRL_INTENSET_ERROR(value)         (NVMCTRL_INTENSET_ERROR_Msk & ((value) << NVMCTRL_INTENSET_ERROR_Pos))
#define NVMCTRL_INTENSET_Msk                  _U_(0x03)                                            /**< (NVMCTRL_INTENSET) Register Mask  */


/* -------- NVMCTRL_INTFLAG : (NVMCTRL Offset: 0x14) (R/W 8) Interrupt Flag Status and Clear -------- */
#define NVMCTRL_INTFLAG_RESETVALUE            _U_(0x00)                                            /**<  (NVMCTRL_INTFLAG) Interrupt Flag Status and Clear  Reset Value */

#define NVMCTRL_INTFLAG_READY_Pos             _U_(0)                                               /**< (NVMCTRL_INTFLAG) NVM Ready Position */
#define NVMCTRL_INTFLAG_READY_Msk             (_U_(0x1) << NVMCTRL_INTFLAG_READY_Pos)              /**< (NVMCTRL_INTFLAG) NVM Ready Mask */
#define NVMCTRL_INTFLAG_READY(value)          (NVMCTRL_INTFLAG_READY_Msk & ((value) << NVMCTRL_INTFLAG_READY_Pos))
#define NVMCTRL_INTFLAG_ERROR_Pos             _U_(1)                                               /**< (NVMCTRL_INTFLAG) Error Position */
#define NVMCTRL_INTFLAG_ERROR_Msk             (_U_(0x1) << NVMCTRL_INTFLAG_ERROR_Pos)              /**< (NVMCTRL_INTFLAG) Error Mask */
#define NVMCTRL_INTFLAG_ERROR(value)          (NVMCTRL_INTFLAG_ERROR_Msk & ((value) << NVMCTRL_INTFLAG_ERROR_Pos))
#define NVMCTRL_INTFLAG_Msk                   _U_(0x03)                                            /**< (NVMCTRL_INTFLAG) Register Mask  */


/* -------- NVMCTRL_STATUS : (NVMCTRL Offset: 0x18) (R/W 16) Status -------- */
#define NVMCTRL_STATUS_RESETVALUE             _U_(0x00)                                            /**<  (NVMCTRL_STATUS) Status  Reset Value */

#define NVMCTRL_STATUS_PRM_Pos                _U_(0)                                               /**< (NVMCTRL_STATUS) Power Reduction Mode Position */
#define NVMCTRL_STATUS_PRM_Msk                (_U_(0x1) << NVMCTRL_STATUS_PRM_Pos)                 /**< (NVMCTRL_STATUS) Power Reduction Mode Mask */
#define NVMCTRL_STATUS_PRM(value)             (NVMCTRL_STATUS_PRM_Msk & ((value) << NVMCTRL_STATUS_PRM_Pos))
#define NVMCTRL_STATUS_LOAD_Pos               _U_(1)                                               /**< (NVMCTRL_STATUS) NVM Page Buffer Active Loading Position */
#define NVMCTRL_STATUS_LOAD_Msk               (_U_(0x1) << NVMCTRL_STATUS_LOAD_Pos)                /**< (NVMCTRL_STATUS) NVM Page Buffer Active Loading Mask */
#define NVMCTRL_STATUS_LOAD(value)            (NVMCTRL_STATUS_LOAD_Msk & ((value) << NVMCTRL_STATUS_LOAD_Pos))
#define NVMCTRL_STATUS_PROGE_Pos              _U_(2)                                               /**< (NVMCTRL_STATUS) Programming Error Status Position */
#define NVMCTRL_STATUS_PROGE_Msk              (_U_(0x1) << NVMCTRL_STATUS_PROGE_Pos)               /**< (NVMCTRL_STATUS) Programming Error Status Mask */
#define NVMCTRL_STATUS_PROGE(value)           (NVMCTRL_STATUS_PROGE_Msk & ((value) << NVMCTRL_STATUS_PROGE_Pos))
#define NVMCTRL_STATUS_LOCKE_Pos              _U_(3)                                               /**< (NVMCTRL_STATUS) Lock Error Status Position */
#define NVMCTRL_STATUS_LOCKE_Msk              (_U_(0x1) << NVMCTRL_STATUS_LOCKE_Pos)               /**< (NVMCTRL_STATUS) Lock Error Status Mask */
#define NVMCTRL_STATUS_LOCKE(value)           (NVMCTRL_STATUS_LOCKE_Msk & ((value) << NVMCTRL_STATUS_LOCKE_Pos))
#define NVMCTRL_STATUS_NVME_Pos               _U_(4)                                               /**< (NVMCTRL_STATUS) NVM Error Position */
#define NVMCTRL_STATUS_NVME_Msk               (_U_(0x1) << NVMCTRL_STATUS_NVME_Pos)                /**< (NVMCTRL_STATUS) NVM Error Mask */
#define NVMCTRL_STATUS_NVME(value)            (NVMCTRL_STATUS_NVME_Msk & ((value) << NVMCTRL_STATUS_NVME_Pos))
#define NVMCTRL_STATUS_SB_Pos                 _U_(8)                                               /**< (NVMCTRL_STATUS) Security Bit Status Position */
#define NVMCTRL_STATUS_SB_Msk                 (_U_(0x1) << NVMCTRL_STATUS_SB_Pos)                  /**< (NVMCTRL_STATUS) Security Bit Status Mask */
#define NVMCTRL_STATUS_SB(value)              (NVMCTRL_STATUS_SB_Msk & ((value) << NVMCTRL_STATUS_SB_Pos))
#define NVMCTRL_STATUS_Msk                    _U_(0x011F)                                          /**< (NVMCTRL_STATUS) Register Mask  */


/* -------- NVMCTRL_ADDR : (NVMCTRL Offset: 0x1C) (R/W 32) Address -------- */
#define NVMCTRL_ADDR_RESETVALUE               _U_(0x00)                                            /**<  (NVMCTRL_ADDR) Address  Reset Value */

#define NVMCTRL_ADDR_ADDR_Pos                 _U_(0)                                               /**< (NVMCTRL_ADDR) NVM Address Position */
#define NVMCTRL_ADDR_ADDR_Msk                 (_U_(0x3FFFFF) << NVMCTRL_ADDR_ADDR_Pos)             /**< (NVMCTRL_ADDR) NVM Address Mask */
#define NVMCTRL_ADDR_ADDR(value)              (NVMCTRL_ADDR_ADDR_Msk & ((value) << NVMCTRL_ADDR_ADDR_Pos))
#define NVMCTRL_ADDR_Msk                      _U_(0x003FFFFF)                                      /**< (NVMCTRL_ADDR) Register Mask  */


/* -------- NVMCTRL_LOCK : (NVMCTRL Offset: 0x20) (R/W 16) Lock Section -------- */
#define NVMCTRL_LOCK_LOCK_Pos                 _U_(0)                                               /**< (NVMCTRL_LOCK) Region Lock Bits Position */
#define NVMCTRL_LOCK_LOCK_Msk                 (_U_(0xFFFF) << NVMCTRL_LOCK_LOCK_Pos)               /**< (NVMCTRL_LOCK) Region Lock Bits Mask */
#define NVMCTRL_LOCK_LOCK(value)              (NVMCTRL_LOCK_LOCK_Msk & ((value) << NVMCTRL_LOCK_LOCK_Pos))
#define NVMCTRL_LOCK_Msk                      _U_(0xFFFF)                                          /**< (NVMCTRL_LOCK) Register Mask  */


/** \brief NVMCTRL register offsets definitions */
#define NVMCTRL_CTRLA_REG_OFST         (0x00)              /**< (NVMCTRL_CTRLA) Control A Offset */
#define NVMCTRL_CTRLB_REG_OFST         (0x04)              /**< (NVMCTRL_CTRLB) Control B Offset */
#define NVMCTRL_PARAM_REG_OFST         (0x08)              /**< (NVMCTRL_PARAM) NVM Parameter Offset */
#define NVMCTRL_INTENCLR_REG_OFST      (0x0C)              /**< (NVMCTRL_INTENCLR) Interrupt Enable Clear Offset */
#define NVMCTRL_INTENSET_REG_OFST      (0x10)              /**< (NVMCTRL_INTENSET) Interrupt Enable Set Offset */
#define NVMCTRL_INTFLAG_REG_OFST       (0x14)              /**< (NVMCTRL_INTFLAG) Interrupt Flag Status and Clear Offset */
#define NVMCTRL_STATUS_REG_OFST        (0x18)              /**< (NVMCTRL_STATUS) Status Offset */
#define NVMCTRL_ADDR_REG_OFST          (0x1C)              /**< (NVMCTRL_ADDR) Address Offset */
#define NVMCTRL_LOCK_REG_OFST          (0x20)              /**< (NVMCTRL_LOCK) Lock Section Offset */

#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
/** \brief NVMCTRL register API structure */
typedef struct
{  /* Non-Volatile Memory Controller */
  __IO  uint16_t                       NVMCTRL_CTRLA;      /**< Offset: 0x00 (R/W  16) Control A */
  __I   uint8_t                        Reserved1[0x02];
  __IO  uint32_t                       NVMCTRL_CTRLB;      /**< Offset: 0x04 (R/W  32) Control B */
  __IO  uint32_t                       NVMCTRL_PARAM;      /**< Offset: 0x08 (R/W  32) NVM Parameter */
  __IO  uint8_t                        NVMCTRL_INTENCLR;   /**< Offset: 0x0C (R/W  8) Interrupt Enable Clear */
  __I   uint8_t                        Reserved2[0x03];
  __IO  uint8_t                        NVMCTRL_INTENSET;   /**< Offset: 0x10 (R/W  8) Interrupt Enable Set */
  __I   uint8_t                        Reserved3[0x03];
  __IO  uint8_t                        NVMCTRL_INTFLAG;    /**< Offset: 0x14 (R/W  8) Interrupt Flag Status and Clear */
  __I   uint8_t                        Reserved4[0x03];
  __IO  uint16_t                       NVMCTRL_STATUS;     /**< Offset: 0x18 (R/W  16) Status */
  __I   uint8_t                        Reserved5[0x02];
  __IO  uint32_t                       NVMCTRL_ADDR;       /**< Offset: 0x1C (R/W  32) Address */
  __IO  uint16_t                       NVMCTRL_LOCK;       /**< Offset: 0x20 (R/W  16) Lock Section */
} nvmctrl_registers_t;


#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
#endif /* _SAMD21_NVMCTRL_COMPONENT_H_ */
