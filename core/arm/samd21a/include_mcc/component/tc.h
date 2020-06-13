/**
 * \brief Component description for TC
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
#ifndef _SAMD21_TC_COMPONENT_H_
#define _SAMD21_TC_COMPONENT_H_

/* ************************************************************************** */
/*   SOFTWARE API DEFINITION FOR TC                                           */
/* ************************************************************************** */

/* -------- TC_CTRLA : (TC Offset: 0x00) (R/W 16) Control A -------- */
#define TC_CTRLA_RESETVALUE                   _U_(0x00)                                            /**<  (TC_CTRLA) Control A  Reset Value */

#define TC_CTRLA_SWRST_Pos                    _U_(0)                                               /**< (TC_CTRLA) Software Reset Position */
#define TC_CTRLA_SWRST_Msk                    (_U_(0x1) << TC_CTRLA_SWRST_Pos)                     /**< (TC_CTRLA) Software Reset Mask */
#define TC_CTRLA_SWRST(value)                 (TC_CTRLA_SWRST_Msk & ((value) << TC_CTRLA_SWRST_Pos))
#define TC_CTRLA_ENABLE_Pos                   _U_(1)                                               /**< (TC_CTRLA) Enable Position */
#define TC_CTRLA_ENABLE_Msk                   (_U_(0x1) << TC_CTRLA_ENABLE_Pos)                    /**< (TC_CTRLA) Enable Mask */
#define TC_CTRLA_ENABLE(value)                (TC_CTRLA_ENABLE_Msk & ((value) << TC_CTRLA_ENABLE_Pos))
#define TC_CTRLA_MODE_Pos                     _U_(2)                                               /**< (TC_CTRLA) TC Mode Position */
#define TC_CTRLA_MODE_Msk                     (_U_(0x3) << TC_CTRLA_MODE_Pos)                      /**< (TC_CTRLA) TC Mode Mask */
#define TC_CTRLA_MODE(value)                  (TC_CTRLA_MODE_Msk & ((value) << TC_CTRLA_MODE_Pos))
#define   TC_CTRLA_MODE_COUNT16_Val           _U_(0x0)                                             /**< (TC_CTRLA) Counter in 16-bit mode  */
#define   TC_CTRLA_MODE_COUNT8_Val            _U_(0x1)                                             /**< (TC_CTRLA) Counter in 8-bit mode  */
#define   TC_CTRLA_MODE_COUNT32_Val           _U_(0x2)                                             /**< (TC_CTRLA) Counter in 32-bit mode  */
#define TC_CTRLA_MODE_COUNT16                 (TC_CTRLA_MODE_COUNT16_Val << TC_CTRLA_MODE_Pos)     /**< (TC_CTRLA) Counter in 16-bit mode Position  */
#define TC_CTRLA_MODE_COUNT8                  (TC_CTRLA_MODE_COUNT8_Val << TC_CTRLA_MODE_Pos)      /**< (TC_CTRLA) Counter in 8-bit mode Position  */
#define TC_CTRLA_MODE_COUNT32                 (TC_CTRLA_MODE_COUNT32_Val << TC_CTRLA_MODE_Pos)     /**< (TC_CTRLA) Counter in 32-bit mode Position  */
#define TC_CTRLA_WAVEGEN_Pos                  _U_(5)                                               /**< (TC_CTRLA) Waveform Generation Operation Position */
#define TC_CTRLA_WAVEGEN_Msk                  (_U_(0x3) << TC_CTRLA_WAVEGEN_Pos)                   /**< (TC_CTRLA) Waveform Generation Operation Mask */
#define TC_CTRLA_WAVEGEN(value)               (TC_CTRLA_WAVEGEN_Msk & ((value) << TC_CTRLA_WAVEGEN_Pos))
#define   TC_CTRLA_WAVEGEN_NFRQ_Val           _U_(0x0)                                             /**< (TC_CTRLA)   */
#define   TC_CTRLA_WAVEGEN_MFRQ_Val           _U_(0x1)                                             /**< (TC_CTRLA)   */
#define   TC_CTRLA_WAVEGEN_NPWM_Val           _U_(0x2)                                             /**< (TC_CTRLA)   */
#define   TC_CTRLA_WAVEGEN_MPWM_Val           _U_(0x3)                                             /**< (TC_CTRLA)   */
#define TC_CTRLA_WAVEGEN_NFRQ                 (TC_CTRLA_WAVEGEN_NFRQ_Val << TC_CTRLA_WAVEGEN_Pos)  /**< (TC_CTRLA)  Position  */
#define TC_CTRLA_WAVEGEN_MFRQ                 (TC_CTRLA_WAVEGEN_MFRQ_Val << TC_CTRLA_WAVEGEN_Pos)  /**< (TC_CTRLA)  Position  */
#define TC_CTRLA_WAVEGEN_NPWM                 (TC_CTRLA_WAVEGEN_NPWM_Val << TC_CTRLA_WAVEGEN_Pos)  /**< (TC_CTRLA)  Position  */
#define TC_CTRLA_WAVEGEN_MPWM                 (TC_CTRLA_WAVEGEN_MPWM_Val << TC_CTRLA_WAVEGEN_Pos)  /**< (TC_CTRLA)  Position  */
#define TC_CTRLA_PRESCALER_Pos                _U_(8)                                               /**< (TC_CTRLA) Prescaler Position */
#define TC_CTRLA_PRESCALER_Msk                (_U_(0x7) << TC_CTRLA_PRESCALER_Pos)                 /**< (TC_CTRLA) Prescaler Mask */
#define TC_CTRLA_PRESCALER(value)             (TC_CTRLA_PRESCALER_Msk & ((value) << TC_CTRLA_PRESCALER_Pos))
#define   TC_CTRLA_PRESCALER_DIV1_Val         _U_(0x0)                                             /**< (TC_CTRLA) Prescaler: GCLK_TC  */
#define   TC_CTRLA_PRESCALER_DIV2_Val         _U_(0x1)                                             /**< (TC_CTRLA) Prescaler: GCLK_TC/2  */
#define   TC_CTRLA_PRESCALER_DIV4_Val         _U_(0x2)                                             /**< (TC_CTRLA) Prescaler: GCLK_TC/4  */
#define   TC_CTRLA_PRESCALER_DIV8_Val         _U_(0x3)                                             /**< (TC_CTRLA) Prescaler: GCLK_TC/8  */
#define   TC_CTRLA_PRESCALER_DIV16_Val        _U_(0x4)                                             /**< (TC_CTRLA) Prescaler: GCLK_TC/16  */
#define   TC_CTRLA_PRESCALER_DIV64_Val        _U_(0x5)                                             /**< (TC_CTRLA) Prescaler: GCLK_TC/64  */
#define   TC_CTRLA_PRESCALER_DIV256_Val       _U_(0x6)                                             /**< (TC_CTRLA) Prescaler: GCLK_TC/256  */
#define   TC_CTRLA_PRESCALER_DIV1024_Val      _U_(0x7)                                             /**< (TC_CTRLA) Prescaler: GCLK_TC/1024  */
#define TC_CTRLA_PRESCALER_DIV1               (TC_CTRLA_PRESCALER_DIV1_Val << TC_CTRLA_PRESCALER_Pos) /**< (TC_CTRLA) Prescaler: GCLK_TC Position  */
#define TC_CTRLA_PRESCALER_DIV2               (TC_CTRLA_PRESCALER_DIV2_Val << TC_CTRLA_PRESCALER_Pos) /**< (TC_CTRLA) Prescaler: GCLK_TC/2 Position  */
#define TC_CTRLA_PRESCALER_DIV4               (TC_CTRLA_PRESCALER_DIV4_Val << TC_CTRLA_PRESCALER_Pos) /**< (TC_CTRLA) Prescaler: GCLK_TC/4 Position  */
#define TC_CTRLA_PRESCALER_DIV8               (TC_CTRLA_PRESCALER_DIV8_Val << TC_CTRLA_PRESCALER_Pos) /**< (TC_CTRLA) Prescaler: GCLK_TC/8 Position  */
#define TC_CTRLA_PRESCALER_DIV16              (TC_CTRLA_PRESCALER_DIV16_Val << TC_CTRLA_PRESCALER_Pos) /**< (TC_CTRLA) Prescaler: GCLK_TC/16 Position  */
#define TC_CTRLA_PRESCALER_DIV64              (TC_CTRLA_PRESCALER_DIV64_Val << TC_CTRLA_PRESCALER_Pos) /**< (TC_CTRLA) Prescaler: GCLK_TC/64 Position  */
#define TC_CTRLA_PRESCALER_DIV256             (TC_CTRLA_PRESCALER_DIV256_Val << TC_CTRLA_PRESCALER_Pos) /**< (TC_CTRLA) Prescaler: GCLK_TC/256 Position  */
#define TC_CTRLA_PRESCALER_DIV1024            (TC_CTRLA_PRESCALER_DIV1024_Val << TC_CTRLA_PRESCALER_Pos) /**< (TC_CTRLA) Prescaler: GCLK_TC/1024 Position  */
#define TC_CTRLA_RUNSTDBY_Pos                 _U_(11)                                              /**< (TC_CTRLA) Run in Standby Position */
#define TC_CTRLA_RUNSTDBY_Msk                 (_U_(0x1) << TC_CTRLA_RUNSTDBY_Pos)                  /**< (TC_CTRLA) Run in Standby Mask */
#define TC_CTRLA_RUNSTDBY(value)              (TC_CTRLA_RUNSTDBY_Msk & ((value) << TC_CTRLA_RUNSTDBY_Pos))
#define TC_CTRLA_PRESCSYNC_Pos                _U_(12)                                              /**< (TC_CTRLA) Prescaler and Counter Synchronization Position */
#define TC_CTRLA_PRESCSYNC_Msk                (_U_(0x3) << TC_CTRLA_PRESCSYNC_Pos)                 /**< (TC_CTRLA) Prescaler and Counter Synchronization Mask */
#define TC_CTRLA_PRESCSYNC(value)             (TC_CTRLA_PRESCSYNC_Msk & ((value) << TC_CTRLA_PRESCSYNC_Pos))
#define   TC_CTRLA_PRESCSYNC_GCLK_Val         _U_(0x0)                                             /**< (TC_CTRLA) Reload or reset the counter on next generic clock  */
#define   TC_CTRLA_PRESCSYNC_PRESC_Val        _U_(0x1)                                             /**< (TC_CTRLA) Reload or reset the counter on next prescaler clock  */
#define   TC_CTRLA_PRESCSYNC_RESYNC_Val       _U_(0x2)                                             /**< (TC_CTRLA) Reload or reset the counter on next generic clock. Reset the prescaler counter  */
#define TC_CTRLA_PRESCSYNC_GCLK               (TC_CTRLA_PRESCSYNC_GCLK_Val << TC_CTRLA_PRESCSYNC_Pos) /**< (TC_CTRLA) Reload or reset the counter on next generic clock Position  */
#define TC_CTRLA_PRESCSYNC_PRESC              (TC_CTRLA_PRESCSYNC_PRESC_Val << TC_CTRLA_PRESCSYNC_Pos) /**< (TC_CTRLA) Reload or reset the counter on next prescaler clock Position  */
#define TC_CTRLA_PRESCSYNC_RESYNC             (TC_CTRLA_PRESCSYNC_RESYNC_Val << TC_CTRLA_PRESCSYNC_Pos) /**< (TC_CTRLA) Reload or reset the counter on next generic clock. Reset the prescaler counter Position  */
#define TC_CTRLA_Msk                          _U_(0x3F6F)                                          /**< (TC_CTRLA) Register Mask  */


/* -------- TC_READREQ : (TC Offset: 0x02) (R/W 16) Read Request -------- */
#define TC_READREQ_RESETVALUE                 _U_(0x00)                                            /**<  (TC_READREQ) Read Request  Reset Value */

#define TC_READREQ_ADDR_Pos                   _U_(0)                                               /**< (TC_READREQ) Address Position */
#define TC_READREQ_ADDR_Msk                   (_U_(0x1F) << TC_READREQ_ADDR_Pos)                   /**< (TC_READREQ) Address Mask */
#define TC_READREQ_ADDR(value)                (TC_READREQ_ADDR_Msk & ((value) << TC_READREQ_ADDR_Pos))
#define TC_READREQ_RCONT_Pos                  _U_(14)                                              /**< (TC_READREQ) Read Continuously Position */
#define TC_READREQ_RCONT_Msk                  (_U_(0x1) << TC_READREQ_RCONT_Pos)                   /**< (TC_READREQ) Read Continuously Mask */
#define TC_READREQ_RCONT(value)               (TC_READREQ_RCONT_Msk & ((value) << TC_READREQ_RCONT_Pos))
#define TC_READREQ_RREQ_Pos                   _U_(15)                                              /**< (TC_READREQ) Read Request Position */
#define TC_READREQ_RREQ_Msk                   (_U_(0x1) << TC_READREQ_RREQ_Pos)                    /**< (TC_READREQ) Read Request Mask */
#define TC_READREQ_RREQ(value)                (TC_READREQ_RREQ_Msk & ((value) << TC_READREQ_RREQ_Pos))
#define TC_READREQ_Msk                        _U_(0xC01F)                                          /**< (TC_READREQ) Register Mask  */


/* -------- TC_CTRLBCLR : (TC Offset: 0x04) (R/W 8) Control B Clear -------- */
#define TC_CTRLBCLR_RESETVALUE                _U_(0x02)                                            /**<  (TC_CTRLBCLR) Control B Clear  Reset Value */

#define TC_CTRLBCLR_DIR_Pos                   _U_(0)                                               /**< (TC_CTRLBCLR) Counter Direction Position */
#define TC_CTRLBCLR_DIR_Msk                   (_U_(0x1) << TC_CTRLBCLR_DIR_Pos)                    /**< (TC_CTRLBCLR) Counter Direction Mask */
#define TC_CTRLBCLR_DIR(value)                (TC_CTRLBCLR_DIR_Msk & ((value) << TC_CTRLBCLR_DIR_Pos))
#define TC_CTRLBCLR_ONESHOT_Pos               _U_(2)                                               /**< (TC_CTRLBCLR) One-Shot Position */
#define TC_CTRLBCLR_ONESHOT_Msk               (_U_(0x1) << TC_CTRLBCLR_ONESHOT_Pos)                /**< (TC_CTRLBCLR) One-Shot Mask */
#define TC_CTRLBCLR_ONESHOT(value)            (TC_CTRLBCLR_ONESHOT_Msk & ((value) << TC_CTRLBCLR_ONESHOT_Pos))
#define TC_CTRLBCLR_CMD_Pos                   _U_(6)                                               /**< (TC_CTRLBCLR) Command Position */
#define TC_CTRLBCLR_CMD_Msk                   (_U_(0x3) << TC_CTRLBCLR_CMD_Pos)                    /**< (TC_CTRLBCLR) Command Mask */
#define TC_CTRLBCLR_CMD(value)                (TC_CTRLBCLR_CMD_Msk & ((value) << TC_CTRLBCLR_CMD_Pos))
#define   TC_CTRLBCLR_CMD_NONE_Val            _U_(0x0)                                             /**< (TC_CTRLBCLR) No action  */
#define   TC_CTRLBCLR_CMD_RETRIGGER_Val       _U_(0x1)                                             /**< (TC_CTRLBCLR) Force a start, restart or retrigger  */
#define   TC_CTRLBCLR_CMD_STOP_Val            _U_(0x2)                                             /**< (TC_CTRLBCLR) Force a stop  */
#define TC_CTRLBCLR_CMD_NONE                  (TC_CTRLBCLR_CMD_NONE_Val << TC_CTRLBCLR_CMD_Pos)    /**< (TC_CTRLBCLR) No action Position  */
#define TC_CTRLBCLR_CMD_RETRIGGER             (TC_CTRLBCLR_CMD_RETRIGGER_Val << TC_CTRLBCLR_CMD_Pos) /**< (TC_CTRLBCLR) Force a start, restart or retrigger Position  */
#define TC_CTRLBCLR_CMD_STOP                  (TC_CTRLBCLR_CMD_STOP_Val << TC_CTRLBCLR_CMD_Pos)    /**< (TC_CTRLBCLR) Force a stop Position  */
#define TC_CTRLBCLR_Msk                       _U_(0xC5)                                            /**< (TC_CTRLBCLR) Register Mask  */


/* -------- TC_CTRLBSET : (TC Offset: 0x05) (R/W 8) Control B Set -------- */
#define TC_CTRLBSET_RESETVALUE                _U_(0x00)                                            /**<  (TC_CTRLBSET) Control B Set  Reset Value */

#define TC_CTRLBSET_DIR_Pos                   _U_(0)                                               /**< (TC_CTRLBSET) Counter Direction Position */
#define TC_CTRLBSET_DIR_Msk                   (_U_(0x1) << TC_CTRLBSET_DIR_Pos)                    /**< (TC_CTRLBSET) Counter Direction Mask */
#define TC_CTRLBSET_DIR(value)                (TC_CTRLBSET_DIR_Msk & ((value) << TC_CTRLBSET_DIR_Pos))
#define TC_CTRLBSET_ONESHOT_Pos               _U_(2)                                               /**< (TC_CTRLBSET) One-Shot Position */
#define TC_CTRLBSET_ONESHOT_Msk               (_U_(0x1) << TC_CTRLBSET_ONESHOT_Pos)                /**< (TC_CTRLBSET) One-Shot Mask */
#define TC_CTRLBSET_ONESHOT(value)            (TC_CTRLBSET_ONESHOT_Msk & ((value) << TC_CTRLBSET_ONESHOT_Pos))
#define TC_CTRLBSET_CMD_Pos                   _U_(6)                                               /**< (TC_CTRLBSET) Command Position */
#define TC_CTRLBSET_CMD_Msk                   (_U_(0x3) << TC_CTRLBSET_CMD_Pos)                    /**< (TC_CTRLBSET) Command Mask */
#define TC_CTRLBSET_CMD(value)                (TC_CTRLBSET_CMD_Msk & ((value) << TC_CTRLBSET_CMD_Pos))
#define   TC_CTRLBSET_CMD_NONE_Val            _U_(0x0)                                             /**< (TC_CTRLBSET) No action  */
#define   TC_CTRLBSET_CMD_RETRIGGER_Val       _U_(0x1)                                             /**< (TC_CTRLBSET) Force a start, restart or retrigger  */
#define   TC_CTRLBSET_CMD_STOP_Val            _U_(0x2)                                             /**< (TC_CTRLBSET) Force a stop  */
#define TC_CTRLBSET_CMD_NONE                  (TC_CTRLBSET_CMD_NONE_Val << TC_CTRLBSET_CMD_Pos)    /**< (TC_CTRLBSET) No action Position  */
#define TC_CTRLBSET_CMD_RETRIGGER             (TC_CTRLBSET_CMD_RETRIGGER_Val << TC_CTRLBSET_CMD_Pos) /**< (TC_CTRLBSET) Force a start, restart or retrigger Position  */
#define TC_CTRLBSET_CMD_STOP                  (TC_CTRLBSET_CMD_STOP_Val << TC_CTRLBSET_CMD_Pos)    /**< (TC_CTRLBSET) Force a stop Position  */
#define TC_CTRLBSET_Msk                       _U_(0xC5)                                            /**< (TC_CTRLBSET) Register Mask  */


/* -------- TC_CTRLC : (TC Offset: 0x06) (R/W 8) Control C -------- */
#define TC_CTRLC_RESETVALUE                   _U_(0x00)                                            /**<  (TC_CTRLC) Control C  Reset Value */

#define TC_CTRLC_INVEN0_Pos                   _U_(0)                                               /**< (TC_CTRLC) Output Waveform 0 Invert Enable Position */
#define TC_CTRLC_INVEN0_Msk                   (_U_(0x1) << TC_CTRLC_INVEN0_Pos)                    /**< (TC_CTRLC) Output Waveform 0 Invert Enable Mask */
#define TC_CTRLC_INVEN0(value)                (TC_CTRLC_INVEN0_Msk & ((value) << TC_CTRLC_INVEN0_Pos))
#define TC_CTRLC_INVEN1_Pos                   _U_(1)                                               /**< (TC_CTRLC) Output Waveform 1 Invert Enable Position */
#define TC_CTRLC_INVEN1_Msk                   (_U_(0x1) << TC_CTRLC_INVEN1_Pos)                    /**< (TC_CTRLC) Output Waveform 1 Invert Enable Mask */
#define TC_CTRLC_INVEN1(value)                (TC_CTRLC_INVEN1_Msk & ((value) << TC_CTRLC_INVEN1_Pos))
#define TC_CTRLC_CPTEN0_Pos                   _U_(4)                                               /**< (TC_CTRLC) Capture Channel 0 Enable Position */
#define TC_CTRLC_CPTEN0_Msk                   (_U_(0x1) << TC_CTRLC_CPTEN0_Pos)                    /**< (TC_CTRLC) Capture Channel 0 Enable Mask */
#define TC_CTRLC_CPTEN0(value)                (TC_CTRLC_CPTEN0_Msk & ((value) << TC_CTRLC_CPTEN0_Pos))
#define TC_CTRLC_CPTEN1_Pos                   _U_(5)                                               /**< (TC_CTRLC) Capture Channel 1 Enable Position */
#define TC_CTRLC_CPTEN1_Msk                   (_U_(0x1) << TC_CTRLC_CPTEN1_Pos)                    /**< (TC_CTRLC) Capture Channel 1 Enable Mask */
#define TC_CTRLC_CPTEN1(value)                (TC_CTRLC_CPTEN1_Msk & ((value) << TC_CTRLC_CPTEN1_Pos))
#define TC_CTRLC_Msk                          _U_(0x33)                                            /**< (TC_CTRLC) Register Mask  */

#define TC_CTRLC_INVEN_Pos                    _U_(0)                                               /**< (TC_CTRLC Position) Output Waveform x Invert Enable */
#define TC_CTRLC_INVEN_Msk                    (_U_(0x3) << TC_CTRLC_INVEN_Pos)                     /**< (TC_CTRLC Mask) INVEN */
#define TC_CTRLC_INVEN(value)                 (TC_CTRLC_INVEN_Msk & ((value) << TC_CTRLC_INVEN_Pos)) 
#define TC_CTRLC_CPTEN_Pos                    _U_(4)                                               /**< (TC_CTRLC Position) Capture Channel x Enable */
#define TC_CTRLC_CPTEN_Msk                    (_U_(0x3) << TC_CTRLC_CPTEN_Pos)                     /**< (TC_CTRLC Mask) CPTEN */
#define TC_CTRLC_CPTEN(value)                 (TC_CTRLC_CPTEN_Msk & ((value) << TC_CTRLC_CPTEN_Pos)) 

/* -------- TC_DBGCTRL : (TC Offset: 0x08) (R/W 8) Debug Control -------- */
#define TC_DBGCTRL_RESETVALUE                 _U_(0x00)                                            /**<  (TC_DBGCTRL) Debug Control  Reset Value */

#define TC_DBGCTRL_DBGRUN_Pos                 _U_(0)                                               /**< (TC_DBGCTRL) Debug Run Mode Position */
#define TC_DBGCTRL_DBGRUN_Msk                 (_U_(0x1) << TC_DBGCTRL_DBGRUN_Pos)                  /**< (TC_DBGCTRL) Debug Run Mode Mask */
#define TC_DBGCTRL_DBGRUN(value)              (TC_DBGCTRL_DBGRUN_Msk & ((value) << TC_DBGCTRL_DBGRUN_Pos))
#define TC_DBGCTRL_Msk                        _U_(0x01)                                            /**< (TC_DBGCTRL) Register Mask  */


/* -------- TC_EVCTRL : (TC Offset: 0x0A) (R/W 16) Event Control -------- */
#define TC_EVCTRL_RESETVALUE                  _U_(0x00)                                            /**<  (TC_EVCTRL) Event Control  Reset Value */

#define TC_EVCTRL_EVACT_Pos                   _U_(0)                                               /**< (TC_EVCTRL) Event Action Position */
#define TC_EVCTRL_EVACT_Msk                   (_U_(0x7) << TC_EVCTRL_EVACT_Pos)                    /**< (TC_EVCTRL) Event Action Mask */
#define TC_EVCTRL_EVACT(value)                (TC_EVCTRL_EVACT_Msk & ((value) << TC_EVCTRL_EVACT_Pos))
#define   TC_EVCTRL_EVACT_OFF_Val             _U_(0x0)                                             /**< (TC_EVCTRL) Event action disabled  */
#define   TC_EVCTRL_EVACT_RETRIGGER_Val       _U_(0x1)                                             /**< (TC_EVCTRL) Start, restart or retrigger TC on event  */
#define   TC_EVCTRL_EVACT_COUNT_Val           _U_(0x2)                                             /**< (TC_EVCTRL) Count on event  */
#define   TC_EVCTRL_EVACT_START_Val           _U_(0x3)                                             /**< (TC_EVCTRL) Start TC on event  */
#define   TC_EVCTRL_EVACT_PPW_Val             _U_(0x5)                                             /**< (TC_EVCTRL) Period captured in CC0, pulse width in CC1  */
#define   TC_EVCTRL_EVACT_PWP_Val             _U_(0x6)                                             /**< (TC_EVCTRL) Period captured in CC1, pulse width in CC0  */
#define TC_EVCTRL_EVACT_OFF                   (TC_EVCTRL_EVACT_OFF_Val << TC_EVCTRL_EVACT_Pos)     /**< (TC_EVCTRL) Event action disabled Position  */
#define TC_EVCTRL_EVACT_RETRIGGER             (TC_EVCTRL_EVACT_RETRIGGER_Val << TC_EVCTRL_EVACT_Pos) /**< (TC_EVCTRL) Start, restart or retrigger TC on event Position  */
#define TC_EVCTRL_EVACT_COUNT                 (TC_EVCTRL_EVACT_COUNT_Val << TC_EVCTRL_EVACT_Pos)   /**< (TC_EVCTRL) Count on event Position  */
#define TC_EVCTRL_EVACT_START                 (TC_EVCTRL_EVACT_START_Val << TC_EVCTRL_EVACT_Pos)   /**< (TC_EVCTRL) Start TC on event Position  */
#define TC_EVCTRL_EVACT_PPW                   (TC_EVCTRL_EVACT_PPW_Val << TC_EVCTRL_EVACT_Pos)     /**< (TC_EVCTRL) Period captured in CC0, pulse width in CC1 Position  */
#define TC_EVCTRL_EVACT_PWP                   (TC_EVCTRL_EVACT_PWP_Val << TC_EVCTRL_EVACT_Pos)     /**< (TC_EVCTRL) Period captured in CC1, pulse width in CC0 Position  */
#define TC_EVCTRL_TCINV_Pos                   _U_(4)                                               /**< (TC_EVCTRL) TC Inverted Event Input Position */
#define TC_EVCTRL_TCINV_Msk                   (_U_(0x1) << TC_EVCTRL_TCINV_Pos)                    /**< (TC_EVCTRL) TC Inverted Event Input Mask */
#define TC_EVCTRL_TCINV(value)                (TC_EVCTRL_TCINV_Msk & ((value) << TC_EVCTRL_TCINV_Pos))
#define TC_EVCTRL_TCEI_Pos                    _U_(5)                                               /**< (TC_EVCTRL) TC Event Input Position */
#define TC_EVCTRL_TCEI_Msk                    (_U_(0x1) << TC_EVCTRL_TCEI_Pos)                     /**< (TC_EVCTRL) TC Event Input Mask */
#define TC_EVCTRL_TCEI(value)                 (TC_EVCTRL_TCEI_Msk & ((value) << TC_EVCTRL_TCEI_Pos))
#define TC_EVCTRL_OVFEO_Pos                   _U_(8)                                               /**< (TC_EVCTRL) Overflow/Underflow Event Output Enable Position */
#define TC_EVCTRL_OVFEO_Msk                   (_U_(0x1) << TC_EVCTRL_OVFEO_Pos)                    /**< (TC_EVCTRL) Overflow/Underflow Event Output Enable Mask */
#define TC_EVCTRL_OVFEO(value)                (TC_EVCTRL_OVFEO_Msk & ((value) << TC_EVCTRL_OVFEO_Pos))
#define TC_EVCTRL_MCEO0_Pos                   _U_(12)                                              /**< (TC_EVCTRL) Match or Capture Channel 0 Event Output Enable Position */
#define TC_EVCTRL_MCEO0_Msk                   (_U_(0x1) << TC_EVCTRL_MCEO0_Pos)                    /**< (TC_EVCTRL) Match or Capture Channel 0 Event Output Enable Mask */
#define TC_EVCTRL_MCEO0(value)                (TC_EVCTRL_MCEO0_Msk & ((value) << TC_EVCTRL_MCEO0_Pos))
#define TC_EVCTRL_MCEO1_Pos                   _U_(13)                                              /**< (TC_EVCTRL) Match or Capture Channel 1 Event Output Enable Position */
#define TC_EVCTRL_MCEO1_Msk                   (_U_(0x1) << TC_EVCTRL_MCEO1_Pos)                    /**< (TC_EVCTRL) Match or Capture Channel 1 Event Output Enable Mask */
#define TC_EVCTRL_MCEO1(value)                (TC_EVCTRL_MCEO1_Msk & ((value) << TC_EVCTRL_MCEO1_Pos))
#define TC_EVCTRL_Msk                         _U_(0x3137)                                          /**< (TC_EVCTRL) Register Mask  */

#define TC_EVCTRL_MCEO_Pos                    _U_(12)                                              /**< (TC_EVCTRL Position) Match or Capture Channel x Event Output Enable */
#define TC_EVCTRL_MCEO_Msk                    (_U_(0x3) << TC_EVCTRL_MCEO_Pos)                     /**< (TC_EVCTRL Mask) MCEO */
#define TC_EVCTRL_MCEO(value)                 (TC_EVCTRL_MCEO_Msk & ((value) << TC_EVCTRL_MCEO_Pos)) 

/* -------- TC_INTENCLR : (TC Offset: 0x0C) (R/W 8) Interrupt Enable Clear -------- */
#define TC_INTENCLR_RESETVALUE                _U_(0x00)                                            /**<  (TC_INTENCLR) Interrupt Enable Clear  Reset Value */

#define TC_INTENCLR_OVF_Pos                   _U_(0)                                               /**< (TC_INTENCLR) Overflow Interrupt Enable Position */
#define TC_INTENCLR_OVF_Msk                   (_U_(0x1) << TC_INTENCLR_OVF_Pos)                    /**< (TC_INTENCLR) Overflow Interrupt Enable Mask */
#define TC_INTENCLR_OVF(value)                (TC_INTENCLR_OVF_Msk & ((value) << TC_INTENCLR_OVF_Pos))
#define TC_INTENCLR_ERR_Pos                   _U_(1)                                               /**< (TC_INTENCLR) Error Interrupt Enable Position */
#define TC_INTENCLR_ERR_Msk                   (_U_(0x1) << TC_INTENCLR_ERR_Pos)                    /**< (TC_INTENCLR) Error Interrupt Enable Mask */
#define TC_INTENCLR_ERR(value)                (TC_INTENCLR_ERR_Msk & ((value) << TC_INTENCLR_ERR_Pos))
#define TC_INTENCLR_SYNCRDY_Pos               _U_(3)                                               /**< (TC_INTENCLR) Synchronization Ready Interrupt Enable Position */
#define TC_INTENCLR_SYNCRDY_Msk               (_U_(0x1) << TC_INTENCLR_SYNCRDY_Pos)                /**< (TC_INTENCLR) Synchronization Ready Interrupt Enable Mask */
#define TC_INTENCLR_SYNCRDY(value)            (TC_INTENCLR_SYNCRDY_Msk & ((value) << TC_INTENCLR_SYNCRDY_Pos))
#define TC_INTENCLR_MC0_Pos                   _U_(4)                                               /**< (TC_INTENCLR) Match or Capture Channel 0 Interrupt Enable Position */
#define TC_INTENCLR_MC0_Msk                   (_U_(0x1) << TC_INTENCLR_MC0_Pos)                    /**< (TC_INTENCLR) Match or Capture Channel 0 Interrupt Enable Mask */
#define TC_INTENCLR_MC0(value)                (TC_INTENCLR_MC0_Msk & ((value) << TC_INTENCLR_MC0_Pos))
#define TC_INTENCLR_MC1_Pos                   _U_(5)                                               /**< (TC_INTENCLR) Match or Capture Channel 1 Interrupt Enable Position */
#define TC_INTENCLR_MC1_Msk                   (_U_(0x1) << TC_INTENCLR_MC1_Pos)                    /**< (TC_INTENCLR) Match or Capture Channel 1 Interrupt Enable Mask */
#define TC_INTENCLR_MC1(value)                (TC_INTENCLR_MC1_Msk & ((value) << TC_INTENCLR_MC1_Pos))
#define TC_INTENCLR_Msk                       _U_(0x3B)                                            /**< (TC_INTENCLR) Register Mask  */

#define TC_INTENCLR_MC_Pos                    _U_(4)                                               /**< (TC_INTENCLR Position) Match or Capture Channel x Interrupt Enable */
#define TC_INTENCLR_MC_Msk                    (_U_(0x3) << TC_INTENCLR_MC_Pos)                     /**< (TC_INTENCLR Mask) MC */
#define TC_INTENCLR_MC(value)                 (TC_INTENCLR_MC_Msk & ((value) << TC_INTENCLR_MC_Pos)) 

/* -------- TC_INTENSET : (TC Offset: 0x0D) (R/W 8) Interrupt Enable Set -------- */
#define TC_INTENSET_RESETVALUE                _U_(0x00)                                            /**<  (TC_INTENSET) Interrupt Enable Set  Reset Value */

#define TC_INTENSET_OVF_Pos                   _U_(0)                                               /**< (TC_INTENSET) Overflow Interrupt Enable Position */
#define TC_INTENSET_OVF_Msk                   (_U_(0x1) << TC_INTENSET_OVF_Pos)                    /**< (TC_INTENSET) Overflow Interrupt Enable Mask */
#define TC_INTENSET_OVF(value)                (TC_INTENSET_OVF_Msk & ((value) << TC_INTENSET_OVF_Pos))
#define TC_INTENSET_ERR_Pos                   _U_(1)                                               /**< (TC_INTENSET) Error Interrupt Enable Position */
#define TC_INTENSET_ERR_Msk                   (_U_(0x1) << TC_INTENSET_ERR_Pos)                    /**< (TC_INTENSET) Error Interrupt Enable Mask */
#define TC_INTENSET_ERR(value)                (TC_INTENSET_ERR_Msk & ((value) << TC_INTENSET_ERR_Pos))
#define TC_INTENSET_SYNCRDY_Pos               _U_(3)                                               /**< (TC_INTENSET) Synchronization Ready Interrupt Enable Position */
#define TC_INTENSET_SYNCRDY_Msk               (_U_(0x1) << TC_INTENSET_SYNCRDY_Pos)                /**< (TC_INTENSET) Synchronization Ready Interrupt Enable Mask */
#define TC_INTENSET_SYNCRDY(value)            (TC_INTENSET_SYNCRDY_Msk & ((value) << TC_INTENSET_SYNCRDY_Pos))
#define TC_INTENSET_MC0_Pos                   _U_(4)                                               /**< (TC_INTENSET) Match or Capture Channel 0 Interrupt Enable Position */
#define TC_INTENSET_MC0_Msk                   (_U_(0x1) << TC_INTENSET_MC0_Pos)                    /**< (TC_INTENSET) Match or Capture Channel 0 Interrupt Enable Mask */
#define TC_INTENSET_MC0(value)                (TC_INTENSET_MC0_Msk & ((value) << TC_INTENSET_MC0_Pos))
#define TC_INTENSET_MC1_Pos                   _U_(5)                                               /**< (TC_INTENSET) Match or Capture Channel 1 Interrupt Enable Position */
#define TC_INTENSET_MC1_Msk                   (_U_(0x1) << TC_INTENSET_MC1_Pos)                    /**< (TC_INTENSET) Match or Capture Channel 1 Interrupt Enable Mask */
#define TC_INTENSET_MC1(value)                (TC_INTENSET_MC1_Msk & ((value) << TC_INTENSET_MC1_Pos))
#define TC_INTENSET_Msk                       _U_(0x3B)                                            /**< (TC_INTENSET) Register Mask  */

#define TC_INTENSET_MC_Pos                    _U_(4)                                               /**< (TC_INTENSET Position) Match or Capture Channel x Interrupt Enable */
#define TC_INTENSET_MC_Msk                    (_U_(0x3) << TC_INTENSET_MC_Pos)                     /**< (TC_INTENSET Mask) MC */
#define TC_INTENSET_MC(value)                 (TC_INTENSET_MC_Msk & ((value) << TC_INTENSET_MC_Pos)) 

/* -------- TC_INTFLAG : (TC Offset: 0x0E) (R/W 8) Interrupt Flag Status and Clear -------- */
#define TC_INTFLAG_RESETVALUE                 _U_(0x00)                                            /**<  (TC_INTFLAG) Interrupt Flag Status and Clear  Reset Value */

#define TC_INTFLAG_OVF_Pos                    _U_(0)                                               /**< (TC_INTFLAG) Overflow Position */
#define TC_INTFLAG_OVF_Msk                    (_U_(0x1) << TC_INTFLAG_OVF_Pos)                     /**< (TC_INTFLAG) Overflow Mask */
#define TC_INTFLAG_OVF(value)                 (TC_INTFLAG_OVF_Msk & ((value) << TC_INTFLAG_OVF_Pos))
#define TC_INTFLAG_ERR_Pos                    _U_(1)                                               /**< (TC_INTFLAG) Error Position */
#define TC_INTFLAG_ERR_Msk                    (_U_(0x1) << TC_INTFLAG_ERR_Pos)                     /**< (TC_INTFLAG) Error Mask */
#define TC_INTFLAG_ERR(value)                 (TC_INTFLAG_ERR_Msk & ((value) << TC_INTFLAG_ERR_Pos))
#define TC_INTFLAG_SYNCRDY_Pos                _U_(3)                                               /**< (TC_INTFLAG) Synchronization Ready Position */
#define TC_INTFLAG_SYNCRDY_Msk                (_U_(0x1) << TC_INTFLAG_SYNCRDY_Pos)                 /**< (TC_INTFLAG) Synchronization Ready Mask */
#define TC_INTFLAG_SYNCRDY(value)             (TC_INTFLAG_SYNCRDY_Msk & ((value) << TC_INTFLAG_SYNCRDY_Pos))
#define TC_INTFLAG_MC0_Pos                    _U_(4)                                               /**< (TC_INTFLAG) Match or Capture Channel 0 Position */
#define TC_INTFLAG_MC0_Msk                    (_U_(0x1) << TC_INTFLAG_MC0_Pos)                     /**< (TC_INTFLAG) Match or Capture Channel 0 Mask */
#define TC_INTFLAG_MC0(value)                 (TC_INTFLAG_MC0_Msk & ((value) << TC_INTFLAG_MC0_Pos))
#define TC_INTFLAG_MC1_Pos                    _U_(5)                                               /**< (TC_INTFLAG) Match or Capture Channel 1 Position */
#define TC_INTFLAG_MC1_Msk                    (_U_(0x1) << TC_INTFLAG_MC1_Pos)                     /**< (TC_INTFLAG) Match or Capture Channel 1 Mask */
#define TC_INTFLAG_MC1(value)                 (TC_INTFLAG_MC1_Msk & ((value) << TC_INTFLAG_MC1_Pos))
#define TC_INTFLAG_Msk                        _U_(0x3B)                                            /**< (TC_INTFLAG) Register Mask  */

#define TC_INTFLAG_MC_Pos                     _U_(4)                                               /**< (TC_INTFLAG Position) Match or Capture Channel x */
#define TC_INTFLAG_MC_Msk                     (_U_(0x3) << TC_INTFLAG_MC_Pos)                      /**< (TC_INTFLAG Mask) MC */
#define TC_INTFLAG_MC(value)                  (TC_INTFLAG_MC_Msk & ((value) << TC_INTFLAG_MC_Pos)) 

/* -------- TC_STATUS : (TC Offset: 0x0F) ( R/ 8) Status -------- */
#define TC_STATUS_RESETVALUE                  _U_(0x08)                                            /**<  (TC_STATUS) Status  Reset Value */

#define TC_STATUS_STOP_Pos                    _U_(3)                                               /**< (TC_STATUS) Stop Position */
#define TC_STATUS_STOP_Msk                    (_U_(0x1) << TC_STATUS_STOP_Pos)                     /**< (TC_STATUS) Stop Mask */
#define TC_STATUS_STOP(value)                 (TC_STATUS_STOP_Msk & ((value) << TC_STATUS_STOP_Pos))
#define TC_STATUS_SLAVE_Pos                   _U_(4)                                               /**< (TC_STATUS) Slave Position */
#define TC_STATUS_SLAVE_Msk                   (_U_(0x1) << TC_STATUS_SLAVE_Pos)                    /**< (TC_STATUS) Slave Mask */
#define TC_STATUS_SLAVE(value)                (TC_STATUS_SLAVE_Msk & ((value) << TC_STATUS_SLAVE_Pos))
#define TC_STATUS_SYNCBUSY_Pos                _U_(7)                                               /**< (TC_STATUS) Synchronization Busy Position */
#define TC_STATUS_SYNCBUSY_Msk                (_U_(0x1) << TC_STATUS_SYNCBUSY_Pos)                 /**< (TC_STATUS) Synchronization Busy Mask */
#define TC_STATUS_SYNCBUSY(value)             (TC_STATUS_SYNCBUSY_Msk & ((value) << TC_STATUS_SYNCBUSY_Pos))
#define TC_STATUS_Msk                         _U_(0x98)                                            /**< (TC_STATUS) Register Mask  */


/* -------- TC_COUNT8_COUNT : (TC Offset: 0x10) (R/W 8) COUNT8 Counter Value -------- */
#define TC_COUNT8_COUNT_RESETVALUE            _U_(0x00)                                            /**<  (TC_COUNT8_COUNT) COUNT8 Counter Value  Reset Value */

#define TC_COUNT8_COUNT_COUNT_Pos             _U_(0)                                               /**< (TC_COUNT8_COUNT) Counter Value Position */
#define TC_COUNT8_COUNT_COUNT_Msk             (_U_(0xFF) << TC_COUNT8_COUNT_COUNT_Pos)             /**< (TC_COUNT8_COUNT) Counter Value Mask */
#define TC_COUNT8_COUNT_COUNT(value)          (TC_COUNT8_COUNT_COUNT_Msk & ((value) << TC_COUNT8_COUNT_COUNT_Pos))
#define TC_COUNT8_COUNT_Msk                   _U_(0xFF)                                            /**< (TC_COUNT8_COUNT) Register Mask  */


/* -------- TC_COUNT16_COUNT : (TC Offset: 0x10) (R/W 16) COUNT16 Counter Value -------- */
#define TC_COUNT16_COUNT_RESETVALUE           _U_(0x00)                                            /**<  (TC_COUNT16_COUNT) COUNT16 Counter Value  Reset Value */

#define TC_COUNT16_COUNT_COUNT_Pos            _U_(0)                                               /**< (TC_COUNT16_COUNT) Count Value Position */
#define TC_COUNT16_COUNT_COUNT_Msk            (_U_(0xFFFF) << TC_COUNT16_COUNT_COUNT_Pos)          /**< (TC_COUNT16_COUNT) Count Value Mask */
#define TC_COUNT16_COUNT_COUNT(value)         (TC_COUNT16_COUNT_COUNT_Msk & ((value) << TC_COUNT16_COUNT_COUNT_Pos))
#define TC_COUNT16_COUNT_Msk                  _U_(0xFFFF)                                          /**< (TC_COUNT16_COUNT) Register Mask  */


/* -------- TC_COUNT32_COUNT : (TC Offset: 0x10) (R/W 32) COUNT32 Counter Value -------- */
#define TC_COUNT32_COUNT_RESETVALUE           _U_(0x00)                                            /**<  (TC_COUNT32_COUNT) COUNT32 Counter Value  Reset Value */

#define TC_COUNT32_COUNT_COUNT_Pos            _U_(0)                                               /**< (TC_COUNT32_COUNT) Count Value Position */
#define TC_COUNT32_COUNT_COUNT_Msk            (_U_(0xFFFFFFFF) << TC_COUNT32_COUNT_COUNT_Pos)      /**< (TC_COUNT32_COUNT) Count Value Mask */
#define TC_COUNT32_COUNT_COUNT(value)         (TC_COUNT32_COUNT_COUNT_Msk & ((value) << TC_COUNT32_COUNT_COUNT_Pos))
#define TC_COUNT32_COUNT_Msk                  _U_(0xFFFFFFFF)                                      /**< (TC_COUNT32_COUNT) Register Mask  */


/* -------- TC_COUNT8_PER : (TC Offset: 0x14) (R/W 8) COUNT8 Period Value -------- */
#define TC_COUNT8_PER_RESETVALUE              _U_(0xFF)                                            /**<  (TC_COUNT8_PER) COUNT8 Period Value  Reset Value */

#define TC_COUNT8_PER_PER_Pos                 _U_(0)                                               /**< (TC_COUNT8_PER) Period Value Position */
#define TC_COUNT8_PER_PER_Msk                 (_U_(0xFF) << TC_COUNT8_PER_PER_Pos)                 /**< (TC_COUNT8_PER) Period Value Mask */
#define TC_COUNT8_PER_PER(value)              (TC_COUNT8_PER_PER_Msk & ((value) << TC_COUNT8_PER_PER_Pos))
#define TC_COUNT8_PER_Msk                     _U_(0xFF)                                            /**< (TC_COUNT8_PER) Register Mask  */


/* -------- TC_COUNT8_CC : (TC Offset: 0x18) (R/W 8) COUNT8 Compare/Capture -------- */
#define TC_COUNT8_CC_RESETVALUE               _U_(0x00)                                            /**<  (TC_COUNT8_CC) COUNT8 Compare/Capture  Reset Value */

#define TC_COUNT8_CC_CC_Pos                   _U_(0)                                               /**< (TC_COUNT8_CC) Compare/Capture Value Position */
#define TC_COUNT8_CC_CC_Msk                   (_U_(0xFF) << TC_COUNT8_CC_CC_Pos)                   /**< (TC_COUNT8_CC) Compare/Capture Value Mask */
#define TC_COUNT8_CC_CC(value)                (TC_COUNT8_CC_CC_Msk & ((value) << TC_COUNT8_CC_CC_Pos))
#define TC_COUNT8_CC_Msk                      _U_(0xFF)                                            /**< (TC_COUNT8_CC) Register Mask  */


/* -------- TC_COUNT16_CC : (TC Offset: 0x18) (R/W 16) COUNT16 Compare/Capture -------- */
#define TC_COUNT16_CC_RESETVALUE              _U_(0x00)                                            /**<  (TC_COUNT16_CC) COUNT16 Compare/Capture  Reset Value */

#define TC_COUNT16_CC_CC_Pos                  _U_(0)                                               /**< (TC_COUNT16_CC) Compare/Capture Value Position */
#define TC_COUNT16_CC_CC_Msk                  (_U_(0xFFFF) << TC_COUNT16_CC_CC_Pos)                /**< (TC_COUNT16_CC) Compare/Capture Value Mask */
#define TC_COUNT16_CC_CC(value)               (TC_COUNT16_CC_CC_Msk & ((value) << TC_COUNT16_CC_CC_Pos))
#define TC_COUNT16_CC_Msk                     _U_(0xFFFF)                                          /**< (TC_COUNT16_CC) Register Mask  */


/* -------- TC_COUNT32_CC : (TC Offset: 0x18) (R/W 32) COUNT32 Compare/Capture -------- */
#define TC_COUNT32_CC_RESETVALUE              _U_(0x00)                                            /**<  (TC_COUNT32_CC) COUNT32 Compare/Capture  Reset Value */

#define TC_COUNT32_CC_CC_Pos                  _U_(0)                                               /**< (TC_COUNT32_CC) Compare/Capture Value Position */
#define TC_COUNT32_CC_CC_Msk                  (_U_(0xFFFFFFFF) << TC_COUNT32_CC_CC_Pos)            /**< (TC_COUNT32_CC) Compare/Capture Value Mask */
#define TC_COUNT32_CC_CC(value)               (TC_COUNT32_CC_CC_Msk & ((value) << TC_COUNT32_CC_CC_Pos))
#define TC_COUNT32_CC_Msk                     _U_(0xFFFFFFFF)                                      /**< (TC_COUNT32_CC) Register Mask  */


/** \brief TC register offsets definitions */
#define TC_CTRLA_REG_OFST              (0x00)              /**< (TC_CTRLA) Control A Offset */
#define TC_READREQ_REG_OFST            (0x02)              /**< (TC_READREQ) Read Request Offset */
#define TC_CTRLBCLR_REG_OFST           (0x04)              /**< (TC_CTRLBCLR) Control B Clear Offset */
#define TC_CTRLBSET_REG_OFST           (0x05)              /**< (TC_CTRLBSET) Control B Set Offset */
#define TC_CTRLC_REG_OFST              (0x06)              /**< (TC_CTRLC) Control C Offset */
#define TC_DBGCTRL_REG_OFST            (0x08)              /**< (TC_DBGCTRL) Debug Control Offset */
#define TC_EVCTRL_REG_OFST             (0x0A)              /**< (TC_EVCTRL) Event Control Offset */
#define TC_INTENCLR_REG_OFST           (0x0C)              /**< (TC_INTENCLR) Interrupt Enable Clear Offset */
#define TC_INTENSET_REG_OFST           (0x0D)              /**< (TC_INTENSET) Interrupt Enable Set Offset */
#define TC_INTFLAG_REG_OFST            (0x0E)              /**< (TC_INTFLAG) Interrupt Flag Status and Clear Offset */
#define TC_STATUS_REG_OFST             (0x0F)              /**< (TC_STATUS) Status Offset */
#define TC_COUNT8_COUNT_REG_OFST       (0x10)              /**< (TC_COUNT8_COUNT) COUNT8 Counter Value Offset */
#define TC_COUNT16_COUNT_REG_OFST      (0x10)              /**< (TC_COUNT16_COUNT) COUNT16 Counter Value Offset */
#define TC_COUNT32_COUNT_REG_OFST      (0x10)              /**< (TC_COUNT32_COUNT) COUNT32 Counter Value Offset */
#define TC_COUNT8_PER_REG_OFST         (0x14)              /**< (TC_COUNT8_PER) COUNT8 Period Value Offset */
#define TC_COUNT8_CC_REG_OFST          (0x18)              /**< (TC_COUNT8_CC) COUNT8 Compare/Capture Offset */
#define TC_COUNT16_CC_REG_OFST         (0x18)              /**< (TC_COUNT16_CC) COUNT16 Compare/Capture Offset */
#define TC_COUNT32_CC_REG_OFST         (0x18)              /**< (TC_COUNT32_CC) COUNT32 Compare/Capture Offset */

#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
/** \brief TC register API structure */
typedef struct
{  /* Basic Timer Counter */
  __IO  uint16_t                       TC_CTRLA;           /**< Offset: 0x00 (R/W  16) Control A */
  __IO  uint16_t                       TC_READREQ;         /**< Offset: 0x02 (R/W  16) Read Request */
  __IO  uint8_t                        TC_CTRLBCLR;        /**< Offset: 0x04 (R/W  8) Control B Clear */
  __IO  uint8_t                        TC_CTRLBSET;        /**< Offset: 0x05 (R/W  8) Control B Set */
  __IO  uint8_t                        TC_CTRLC;           /**< Offset: 0x06 (R/W  8) Control C */
  __I   uint8_t                        Reserved1[0x01];
  __IO  uint8_t                        TC_DBGCTRL;         /**< Offset: 0x08 (R/W  8) Debug Control */
  __I   uint8_t                        Reserved2[0x01];
  __IO  uint16_t                       TC_EVCTRL;          /**< Offset: 0x0A (R/W  16) Event Control */
  __IO  uint8_t                        TC_INTENCLR;        /**< Offset: 0x0C (R/W  8) Interrupt Enable Clear */
  __IO  uint8_t                        TC_INTENSET;        /**< Offset: 0x0D (R/W  8) Interrupt Enable Set */
  __IO  uint8_t                        TC_INTFLAG;         /**< Offset: 0x0E (R/W  8) Interrupt Flag Status and Clear */
  __I   uint8_t                        TC_STATUS;          /**< Offset: 0x0F (R/   8) Status */
  __IO  uint8_t                        TC_COUNT;           /**< Offset: 0x10 (R/W  8) COUNT8 Counter Value */
  __I   uint8_t                        Reserved3[0x03];
  __IO  uint8_t                        TC_PER;             /**< Offset: 0x14 (R/W  8) COUNT8 Period Value */
  __I   uint8_t                        Reserved4[0x03];
  __IO  uint8_t                        TC_CC[2];           /**< Offset: 0x18 (R/W  8) COUNT8 Compare/Capture */
} tc_count8_registers_t;

/** \brief TC register API structure */
typedef struct
{  /* Basic Timer Counter */
  __IO  uint16_t                       TC_CTRLA;           /**< Offset: 0x00 (R/W  16) Control A */
  __IO  uint16_t                       TC_READREQ;         /**< Offset: 0x02 (R/W  16) Read Request */
  __IO  uint8_t                        TC_CTRLBCLR;        /**< Offset: 0x04 (R/W  8) Control B Clear */
  __IO  uint8_t                        TC_CTRLBSET;        /**< Offset: 0x05 (R/W  8) Control B Set */
  __IO  uint8_t                        TC_CTRLC;           /**< Offset: 0x06 (R/W  8) Control C */
  __I   uint8_t                        Reserved1[0x01];
  __IO  uint8_t                        TC_DBGCTRL;         /**< Offset: 0x08 (R/W  8) Debug Control */
  __I   uint8_t                        Reserved2[0x01];
  __IO  uint16_t                       TC_EVCTRL;          /**< Offset: 0x0A (R/W  16) Event Control */
  __IO  uint8_t                        TC_INTENCLR;        /**< Offset: 0x0C (R/W  8) Interrupt Enable Clear */
  __IO  uint8_t                        TC_INTENSET;        /**< Offset: 0x0D (R/W  8) Interrupt Enable Set */
  __IO  uint8_t                        TC_INTFLAG;         /**< Offset: 0x0E (R/W  8) Interrupt Flag Status and Clear */
  __I   uint8_t                        TC_STATUS;          /**< Offset: 0x0F (R/   8) Status */
  __IO  uint16_t                       TC_COUNT;           /**< Offset: 0x10 (R/W  16) COUNT16 Counter Value */
  __I   uint8_t                        Reserved3[0x06];
  __IO  uint16_t                       TC_CC[2];           /**< Offset: 0x18 (R/W  16) COUNT16 Compare/Capture */
} tc_count16_registers_t;

/** \brief TC register API structure */
typedef struct
{  /* Basic Timer Counter */
  __IO  uint16_t                       TC_CTRLA;           /**< Offset: 0x00 (R/W  16) Control A */
  __IO  uint16_t                       TC_READREQ;         /**< Offset: 0x02 (R/W  16) Read Request */
  __IO  uint8_t                        TC_CTRLBCLR;        /**< Offset: 0x04 (R/W  8) Control B Clear */
  __IO  uint8_t                        TC_CTRLBSET;        /**< Offset: 0x05 (R/W  8) Control B Set */
  __IO  uint8_t                        TC_CTRLC;           /**< Offset: 0x06 (R/W  8) Control C */
  __I   uint8_t                        Reserved1[0x01];
  __IO  uint8_t                        TC_DBGCTRL;         /**< Offset: 0x08 (R/W  8) Debug Control */
  __I   uint8_t                        Reserved2[0x01];
  __IO  uint16_t                       TC_EVCTRL;          /**< Offset: 0x0A (R/W  16) Event Control */
  __IO  uint8_t                        TC_INTENCLR;        /**< Offset: 0x0C (R/W  8) Interrupt Enable Clear */
  __IO  uint8_t                        TC_INTENSET;        /**< Offset: 0x0D (R/W  8) Interrupt Enable Set */
  __IO  uint8_t                        TC_INTFLAG;         /**< Offset: 0x0E (R/W  8) Interrupt Flag Status and Clear */
  __I   uint8_t                        TC_STATUS;          /**< Offset: 0x0F (R/   8) Status */
  __IO  uint32_t                       TC_COUNT;           /**< Offset: 0x10 (R/W  32) COUNT32 Counter Value */
  __I   uint8_t                        Reserved3[0x04];
  __IO  uint32_t                       TC_CC[2];           /**< Offset: 0x18 (R/W  32) COUNT32 Compare/Capture */
} tc_count32_registers_t;

/** \brief TC hardware registers */
typedef union
{  /* Basic Timer Counter */
       tc_count8_registers_t          COUNT8;         /**< 8-bit Counter Mode */
       tc_count16_registers_t         COUNT16;        /**< 16-bit Counter Mode */
       tc_count32_registers_t         COUNT32;        /**< 32-bit Counter Mode */
} tc_registers_t;


#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
#endif /* _SAMD21_TC_COMPONENT_H_ */
