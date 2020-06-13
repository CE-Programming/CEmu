/**
 * \brief Component description for PAC
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
#ifndef _SAMD21_PAC_COMPONENT_H_
#define _SAMD21_PAC_COMPONENT_H_

/* ************************************************************************** */
/*   SOFTWARE API DEFINITION FOR PAC                                          */
/* ************************************************************************** */

/* -------- PAC_WPCLR : (PAC Offset: 0x00) (R/W 32) Write Protection Clear -------- */
#define PAC_WPCLR_RESETVALUE                  _U_(0x00)                                            /**<  (PAC_WPCLR) Write Protection Clear  Reset Value */

#define PAC_WPCLR_WP_Pos                      _U_(1)                                               /**< (PAC_WPCLR) Write Protection Clear Position */
#define PAC_WPCLR_WP_Msk                      (_U_(0x7FFFFFFF) << PAC_WPCLR_WP_Pos)                /**< (PAC_WPCLR) Write Protection Clear Mask */
#define PAC_WPCLR_WP(value)                   (PAC_WPCLR_WP_Msk & ((value) << PAC_WPCLR_WP_Pos))  
#define PAC_WPCLR_Msk                         _U_(0xFFFFFFFE)                                      /**< (PAC_WPCLR) Register Mask  */


/* -------- PAC_WPSET : (PAC Offset: 0x04) (R/W 32) Write Protection Set -------- */
#define PAC_WPSET_RESETVALUE                  _U_(0x00)                                            /**<  (PAC_WPSET) Write Protection Set  Reset Value */

#define PAC_WPSET_WP_Pos                      _U_(1)                                               /**< (PAC_WPSET) Write Protection Set Position */
#define PAC_WPSET_WP_Msk                      (_U_(0x7FFFFFFF) << PAC_WPSET_WP_Pos)                /**< (PAC_WPSET) Write Protection Set Mask */
#define PAC_WPSET_WP(value)                   (PAC_WPSET_WP_Msk & ((value) << PAC_WPSET_WP_Pos))  
#define PAC_WPSET_Msk                         _U_(0xFFFFFFFE)                                      /**< (PAC_WPSET) Register Mask  */


/** \brief PAC register offsets definitions */
#define PAC_WPCLR_REG_OFST             (0x00)              /**< (PAC_WPCLR) Write Protection Clear Offset */
#define PAC_WPSET_REG_OFST             (0x04)              /**< (PAC_WPSET) Write Protection Set Offset */

#if !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__))
/** \brief PAC register API structure */
typedef struct
{  /* Peripheral Access Controller */
  __IO  uint32_t                       PAC_WPCLR;          /**< Offset: 0x00 (R/W  32) Write Protection Clear */
  __IO  uint32_t                       PAC_WPSET;          /**< Offset: 0x04 (R/W  32) Write Protection Set */
} pac_registers_t;


#endif /* !(defined(__ASSEMBLER__) || defined(__IAR_SYSTEMS_ASM__)) */
#endif /* _SAMD21_PAC_COMPONENT_H_ */
