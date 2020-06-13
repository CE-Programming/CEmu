/**
 * \brief Component description for MTB
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
#ifndef _SAMD21_MTB_COMPONENT_H_
#define _SAMD21_MTB_COMPONENT_H_

/* ************************************************************************** */
/*   SOFTWARE API DEFINITION FOR MTB                                          */
/* ************************************************************************** */

/* -------- MTB_POSITION : (MTB Offset: 0x00) (R/W 32) MTB Position -------- */
#define MTB_POSITION_WRAP_Pos                 _U_(2)                                               /**< (MTB_POSITION) Pointer Value Wraps Position */
#define MTB_POSITION_WRAP_Msk                 (_U_(0x1) << MTB_POSITION_WRAP_Pos)                  /**< (MTB_POSITION) Pointer Value Wraps Mask */
#define MTB_POSITION_WRAP(value)              (MTB_POSITION_WRAP_Msk & ((value) << MTB_POSITION_WRAP_Pos))
#define MTB_POSITION_POINTER_Pos              _U_(3)                                               /**< (MTB_POSITION) Trace Packet Location Pointer Position */
#define MTB_POSITION_POINTER_Msk              (_U_(0x1FFFFFFF) << MTB_POSITION_POINTER_Pos)        /**< (MTB_POSITION) Trace Packet Location Pointer Mask */
#define MTB_POSITION_POINTER(value)           (MTB_POSITION_POINTER_Msk & ((value) << MTB_POSITION_POINTER_Pos))
#define MTB_POSITION_Msk                      _U_(0xFFFFFFFC)                                      /**< (MTB_POSITION) Register Mask  */


/* -------- MTB_MASTER : (MTB Offset: 0x04) (R/W 32) MTB Master -------- */
#define MTB_MASTER_RESETVALUE                 _U_(0x00)                                            /**<  (MTB_MASTER) MTB Master  Reset Value */

#define MTB_MASTER_MASK_Pos                   _U_(0)                                               /**< (MTB_MASTER) Maximum Value of the Trace Buffer in SRAM Position */
#define MTB_MASTER_MASK_Msk                   (_U_(0x1F) << MTB_MASTER_MASK_Pos)                   /**< (MTB_MASTER) Maximum Value of the Trace Buffer in SRAM Mask */
#define MTB_MASTER_MASK(value)                (MTB_MASTER_MASK_Msk & ((value) << MTB_MASTER_MASK_Pos))
#define MTB_MASTER_TSTARTEN_Pos               _U_(5)                                               /**< (MTB_MASTER) Trace Start Input Enable Position */
#define MTB_MASTER_TSTARTEN_Msk               (_U_(0x1) << MTB_MASTER_TSTARTEN_Pos)                /**< (MTB_MASTER) Trace Start Input Enable Mask */
#define MTB_MASTER_TSTARTEN(value)            (MTB_MASTER_TSTARTEN_Msk & ((value) << MTB_MASTER_TSTARTEN_Pos))
#define MTB_MASTER_TSTOPEN_Pos                _U_(6)                                               /**< (MTB_MASTER) Trace Stop Input Enable Position */
#define MTB_MASTER_TSTOPEN_Msk                (_U_(0x1) << MTB_MASTER_TSTOPEN_Pos)                 /**< (MTB_MASTER) Trace Stop Input Enable Mask */
#define MTB_MASTER_TSTOPEN(value)             (MTB_MASTER_TSTOPEN_Msk & ((value) << MTB_MASTER_TSTOPEN_Pos))
#define MTB_MASTER_SFRWPRIV_Pos               _U_(7)                                               /**< (MTB_MASTER) Special Function Register Write Privilege Position */
#define MTB_MASTER_SFRWPRIV_Msk               (_U_(0x1) << MTB_MASTER_SFRWPRIV_Pos)                /**< (MTB_MASTER) Special Function Register Write Privilege Mask */
#define MTB_MASTER_SFRWPRIV(value)            (MTB_MASTER_SFRWPRIV_Msk & ((value) << MTB_MASTER_SFRWPRIV_Pos))
#define MTB_MASTER_RAMPRIV_Pos                _U_(8)                                               /**< (MTB_MASTER) SRAM Privilege Position */
#define MTB_MASTER_RAMPRIV_Msk                (_U_(0x1) << MTB_MASTER_RAMPRIV_Pos)                 /**< (MTB_MASTER) SRAM Privilege Mask */
#define MTB_MASTER_RAMPRIV(value)             (MTB_MASTER_RAMPRIV_Msk & ((value) << MTB_MASTER_RAMPRIV_Pos))
#define MTB_MASTER_HALTREQ_Pos                _U_(9)                                               /**< (MTB_MASTER) Halt Request Position */
#define MTB_MASTER_HALTREQ_Msk                (_U_(0x1) << MTB_MASTER_HALTREQ_Pos)                 /**< (MTB_MASTER) Halt Request Mask */
#define MTB_MASTER_HALTREQ(value)             (MTB_MASTER_HALTREQ_Msk & ((value) << MTB_MASTER_HALTREQ_Pos))
#define MTB_MASTER_EN_Pos                     _U_(31)                                              /**< (MTB_MASTER) Main Trace Enable Position */
#define MTB_MASTER_EN_Msk                     (_U_(0x1) << MTB_MASTER_EN_Pos)                      /**< (MTB_MASTER) Main Trace Enable Mask */
#define MTB_MASTER_EN(value)                  (MTB_MASTER_EN_Msk & ((value) << MTB_MASTER_EN_Pos))
#define MTB_MASTER_Msk                        _U_(0x800003FF)                                      /**< (MTB_MASTER) Register Mask  */


/* -------- MTB_FLOW : (MTB Offset: 0x08) (R/W 32) MTB Flow -------- */
#define MTB_FLOW_RESETVALUE                   _U_(0x00)                                            /**<  (MTB_FLOW) MTB Flow  Reset Value */

#define MTB_FLOW_AUTOSTOP_Pos                 _U_(0)                                               /**< (MTB_FLOW) Auto Stop Tracing Position */
#define MTB_FLOW_AUTOSTOP_Msk                 (_U_(0x1) << MTB_FLOW_AUTOSTOP_Pos)                  /**< (MTB_FLOW) Auto Stop Tracing Mask */
#define MTB_FLOW_AUTOSTOP(value)              (MTB_FLOW_AUTOSTOP_Msk & ((value) << MTB_FLOW_AUTOSTOP_Pos))
#define MTB_FLOW_AUTOHALT_Pos                 _U_(1)                                               /**< (MTB_FLOW) Auto Halt Request Position */
#define MTB_FLOW_AUTOHALT_Msk                 (_U_(0x1) << MTB_FLOW_AUTOHALT_Pos)                  /**< (MTB_FLOW) Auto Halt Request Mask */
#define MTB_FLOW_AUTOHALT(value)              (MTB_FLOW_AUTOHALT_Msk & ((value) << MTB_FLOW_AUTOHALT_Pos))
#define MTB_FLOW_WATERMARK_Pos                _U_(3)                                               /**< (MTB_FLOW) Watermark value Position */
#define MTB_FLOW_WATERMARK_Msk                (_U_(0x1FFFFFFF) << MTB_FLOW_WATERMARK_Pos)          /**< (MTB_FLOW) Watermark value Mask */
#define MTB_FLOW_WATERMARK(value)             (MTB_FLOW_WATERMARK_Msk & ((value) << MTB_FLOW_WATERMARK_Pos))
#define MTB_FLOW_Msk                          _U_(0xFFFFFFFB)                                      /**< (MTB_FLOW) Register Mask  */


/* -------- MTB_BASE : (MTB Offset: 0x0C) ( R/ 32) MTB Base -------- */
#define MTB_BASE_Msk                          _U_(0x00000000)                                      /**< (MTB_BASE) Register Mask  */


/* -------- MTB_ITCTRL : (MTB Offset: 0xF00) (R/W 32) MTB Integration Mode Control -------- */
#define MTB_ITCTRL_Msk                        _U_(0x00000000)                                      /**< (MTB_ITCTRL) Register Mask  */


/* -------- MTB_CLAIMSET : (MTB Offset: 0xFA0) (R/W 32) MTB Claim Set -------- */
#define MTB_CLAIMSET_Msk                      _U_(0x00000000)                                      /**< (MTB_CLAIMSET) Register Mask  */


/* -------- MTB_CLAIMCLR : (MTB Offset: 0xFA4) (R/W 32) MTB Claim Clear -------- */
#define MTB_CLAIMCLR_Msk                      _U_(0x00000000)                                      /**< (MTB_CLAIMCLR) Register Mask  */


/* -------- MTB_LOCKACCESS : (MTB Offset: 0xFB0) (R/W 32) MTB Lock Access -------- */
#define MTB_LOCKACCESS_Msk                    _U_(0x00000000)                                      /**< (MTB_LOCKACCESS) Register Mask  */


/* -------- MTB_LOCKSTATUS : (MTB Offset: 0xFB4) ( R/ 32) MTB Lock Status -------- */
#define MTB_LOCKSTATUS_Msk                    _U_(0x00000000)                                      /**< (MTB_LOCKSTATUS) Register Mask  */


/* -------- MTB_AUTHSTATUS : (MTB Offset: 0xFB8) ( R/ 32) MTB Authentication Status -------- */
#define MTB_AUTHSTATUS_Msk                    _U_(0x00000000)                                      /**< (MTB_AUTHSTATUS) Register Mask  */


/* -------- MTB_DEVARCH : (MTB Offset: 0xFBC) ( R/ 32) MTB Device Architecture -------- */
#define MTB_DEVARCH_Msk                       _U_(0x00000000)                                      /**< (MTB_DEVARCH) Register Mask  */


/* -------- MTB_DEVID : (MTB Offset: 0xFC8) ( R/ 32) MTB Device Configuration -------- */
#define MTB_DEVID_Msk                         _U_(0x00000000)                                      /**< (MTB_DEVID) Register Mask  */


/* -------- MTB_DEVTYPE : (MTB Offset: 0xFCC) ( R/ 32) MTB Device Type -------- */
#define MTB_DEVTYPE_Msk                       _U_(0x00000000)                                      /**< (MTB_DEVTYPE) Register Mask  */


/* -------- MTB_PID4 : (MTB Offset: 0xFD0) ( R/ 32) CoreSight -------- */
#define MTB_PID4_Msk                          _U_(0x00000000)                                      /**< (MTB_PID4) Register Mask  */


/* -------- MTB_PID5 : (MTB Offset: 0xFD4) ( R/ 32) CoreSight -------- */
#define MTB_PID5_Msk                          _U_(0x00000000)                                      /**< (MTB_PID5) Register Mask  */


/* -------- MTB_PID6 : (MTB Offset: 0xFD8) ( R/ 32) CoreSight -------- */
#define MTB_PID6_Msk                          _U_(0x00000000)                                      /**< (MTB_PID6) Register Mask  */


/* -------- MTB_PID7 : (MTB Offset: 0xFDC) ( R/ 32) CoreSight -------- */
#define MTB_PID7_Msk                          _U_(0x00000000)                                      /**< (MTB_PID7) Register Mask  */


/* -------- MTB_PID0 : (MTB Offset: 0xFE0) ( R/ 32) CoreSight -------- */
#define MTB_PID0_Msk                          _U_(0x00000000)                                      /**< (MTB_PID0) Register Mask  */


/* -------- MTB_PID1 : (MTB Offset: 0xFE4) ( R/ 32) CoreSight -------- */
#define MTB_PID1_Msk                          _U_(0x00000000)                                      /**< (MTB_PID1) Register Mask  */


/* -------- MTB_PID2 : (MTB Offset: 0xFE8) ( R/ 32) CoreSight -------- */
#define MTB_PID2_Msk                          _U_(0x00000000)                                      /**< (MTB_PID2) Register Mask  */


/* -------- MTB_PID3 : (MTB Offset: 0xFEC) ( R/ 32) CoreSight -------- */
#define MTB_PID3_Msk                          _U_(0x00000000)                                      /**< (MTB_PID3) Register Mask  */


/* -------- MTB_CID0 : (MTB Offset: 0xFF0) ( R/ 32) CoreSight -------- */
#define MTB_CID0_Msk                          _U_(0x00000000)                                      /**< (MTB_CID0) Register Mask  */


/* -------- MTB_CID1 : (MTB Offset: 0xFF4) ( R/ 32) CoreSight -------- */
#define MTB_CID1_Msk                          _U_(0x00000000)                                      /**< (MTB_CID1) Register Mask  */


/* -------- MTB_CID2 : (MTB Offset: 0xFF8) ( R/ 32) CoreSight -------- */
#define MTB_CID2_Msk                          _U_(0x00000000)                                      /**< (MTB_CID2) Register Mask  */


/* -------- MTB_CID3 : (MTB Offset: 0xFFC) ( R/ 32) CoreSight -------- */
#define MTB_CID3_Msk                          _U_(0x00000000)                                      /**< (MTB_CID3) Register Mask  */


/** \brief MTB register offsets definitions */
#define MTB_POSITION_REG_OFST          (0x00)              /**< (MTB_POSITION) MTB Position Offset */
#define MTB_MASTER_REG_OFST            (0x04)              /**< (MTB_MASTER) MTB Master Offset */
#define MTB_FLOW_REG_OFST              (0x08)              /**< (MTB_FLOW) MTB Flow Offset */
#define MTB_BASE_REG_OFST              (0x0C)              /**< (MTB_BASE) MTB Base Offset */
#define MTB_ITCTRL_REG_OFST            (0xF00)             /**< (MTB_ITCTRL) MTB Integration Mode Control Offset */
#define MTB_CLAIMSET_REG_OFST          (0xFA0)             /**< (MTB_CLAIMSET) MTB Claim Set Offset */
#define MTB_CLAIMCLR_REG_OFST          (0xFA4)             /**< (MTB_CLAIMCLR) MTB Claim Clear Offset */
#define MTB_LOCKACCESS_REG_OFST        (0xFB0)             /**< (MTB_LOCKACCESS) MTB Lock Access Offset */
#define MTB_LOCKSTATUS_REG_OFST        (0xFB4)             /**< (MTB_LOCKSTATUS) MTB Lock Status Offset */
#define MTB_AUTHSTATUS_REG_OFST        (0xFB8)             /**< (MTB_AUTHSTATUS) MTB Authentication Status Offset */
#define MTB_DEVARCH_REG_OFST           (0xFBC)             /**< (MTB_DEVARCH) MTB Device Architecture Offset */
#define MTB_DEVID_REG_OFST             (0xFC8)             /**< (MTB_DEVID) MTB Device Configuration Offset */
#define MTB_DEVTYPE_REG_OFST           (0xFCC)             /**< (MTB_DEVTYPE) MTB Device Type Offset */
#define MTB_PID4_REG_OFST              (0xFD0)             /**< (MTB_PID4) CoreSight Offset */
#define MTB_PID5_REG_OFST              (0xFD4)             /**< (MTB_PID5) CoreSight Offset */
#define MTB_PID6_REG_OFST              (0xFD8)             /**< (MTB_PID6) CoreSight Offset */
#define MTB_PID7_REG_OFST              (0xFDC)             /**< (MTB_PID7) CoreSight Offset */
#define MTB_PID0_REG_OFST              (0xFE0)             /**< (MTB_PID0) CoreSight Offset */
#define MTB_PID1_REG_OFST              (0xFE4)             /**< (MTB_PID1) CoreSight Offset */
#define MTB_PID2_REG_OFST              (0xFE8)             /**< (MTB_PID2) CoreSight Offset */
#define MTB_PID3_REG_OFST              (0xFEC)             /**< (MTB_PID3) CoreSight Offset */
#define MTB_CID0_REG_OFST              (0xFF0)             /**< (MTB_CID0) CoreSight Offset */
#define MTB_CID1_REG_OFST              (0xFF4)             /**< (MTB_CID1) CoreSight Offset */
#define MTB_CID2_REG_OFST              (0xFF8)             /**< (MTB_CID2) CoreSight Offset */
#define MTB_CID3_REG_OFST              (0xFFC)             /**< (MTB_CID3) CoreSight Offset */

#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
/** \brief MTB register API structure */
typedef struct
{  /* Cortex-M0+ Micro-Trace Buffer */
  __IO  uint32_t                       MTB_POSITION;       /**< Offset: 0x00 (R/W  32) MTB Position */
  __IO  uint32_t                       MTB_MASTER;         /**< Offset: 0x04 (R/W  32) MTB Master */
  __IO  uint32_t                       MTB_FLOW;           /**< Offset: 0x08 (R/W  32) MTB Flow */
  __I   uint32_t                       MTB_BASE;           /**< Offset: 0x0C (R/   32) MTB Base */
  __I   uint8_t                        Reserved1[0xEF0];
  __IO  uint32_t                       MTB_ITCTRL;         /**< Offset: 0xF00 (R/W  32) MTB Integration Mode Control */
  __I   uint8_t                        Reserved2[0x9C];
  __IO  uint32_t                       MTB_CLAIMSET;       /**< Offset: 0xFA0 (R/W  32) MTB Claim Set */
  __IO  uint32_t                       MTB_CLAIMCLR;       /**< Offset: 0xFA4 (R/W  32) MTB Claim Clear */
  __I   uint8_t                        Reserved3[0x08];
  __IO  uint32_t                       MTB_LOCKACCESS;     /**< Offset: 0xFB0 (R/W  32) MTB Lock Access */
  __I   uint32_t                       MTB_LOCKSTATUS;     /**< Offset: 0xFB4 (R/   32) MTB Lock Status */
  __I   uint32_t                       MTB_AUTHSTATUS;     /**< Offset: 0xFB8 (R/   32) MTB Authentication Status */
  __I   uint32_t                       MTB_DEVARCH;        /**< Offset: 0xFBC (R/   32) MTB Device Architecture */
  __I   uint8_t                        Reserved4[0x08];
  __I   uint32_t                       MTB_DEVID;          /**< Offset: 0xFC8 (R/   32) MTB Device Configuration */
  __I   uint32_t                       MTB_DEVTYPE;        /**< Offset: 0xFCC (R/   32) MTB Device Type */
  __I   uint32_t                       MTB_PID4;           /**< Offset: 0xFD0 (R/   32) CoreSight */
  __I   uint32_t                       MTB_PID5;           /**< Offset: 0xFD4 (R/   32) CoreSight */
  __I   uint32_t                       MTB_PID6;           /**< Offset: 0xFD8 (R/   32) CoreSight */
  __I   uint32_t                       MTB_PID7;           /**< Offset: 0xFDC (R/   32) CoreSight */
  __I   uint32_t                       MTB_PID0;           /**< Offset: 0xFE0 (R/   32) CoreSight */
  __I   uint32_t                       MTB_PID1;           /**< Offset: 0xFE4 (R/   32) CoreSight */
  __I   uint32_t                       MTB_PID2;           /**< Offset: 0xFE8 (R/   32) CoreSight */
  __I   uint32_t                       MTB_PID3;           /**< Offset: 0xFEC (R/   32) CoreSight */
  __I   uint32_t                       MTB_CID0;           /**< Offset: 0xFF0 (R/   32) CoreSight */
  __I   uint32_t                       MTB_CID1;           /**< Offset: 0xFF4 (R/   32) CoreSight */
  __I   uint32_t                       MTB_CID2;           /**< Offset: 0xFF8 (R/   32) CoreSight */
  __I   uint32_t                       MTB_CID3;           /**< Offset: 0xFFC (R/   32) CoreSight */
} mtb_registers_t;


#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
#endif /* _SAMD21_MTB_COMPONENT_H_ */
