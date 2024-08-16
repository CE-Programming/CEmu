#ifndef ARMMEM_H
#define ARMMEM_H

#include "arm.h"

#define NO_VOLATILE_CONST_IO
#include "samd21a/include/samd21e18a.h"

#include <assert.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "threading.h"

typedef struct {
    __IO PM_CTRL_Type               CTRL;        /**< \brief Offset: 0x00 (R/W  8) Control */
    __IO PM_SLEEP_Type              SLEEP;       /**< \brief Offset: 0x01 (R/W  8) Sleep Mode */
    __IO PM_CPUSEL_Type             CPUSEL;      /**< \brief Offset: 0x08 (R/W  8) CPU Clock Select */
    __IO PM_APBASEL_Type            APBASEL;     /**< \brief Offset: 0x09 (R/W  8) APBA Clock Select */
    __IO PM_APBBSEL_Type            APBBSEL;     /**< \brief Offset: 0x0A (R/W  8) APBB Clock Select */
    __IO PM_APBCSEL_Type            APBCSEL;     /**< \brief Offset: 0x0B (R/W  8) APBC Clock Select */
    __IO PM_AHBMASK_Type            AHBMASK;     /**< \brief Offset: 0x14 (R/W 32) AHB Mask */
    __IO PM_APBAMASK_Type           APBAMASK;    /**< \brief Offset: 0x18 (R/W 32) APBA Mask */
    __IO PM_APBBMASK_Type           APBBMASK;    /**< \brief Offset: 0x1C (R/W 32) APBB Mask */
    __IO PM_APBCMASK_Type           APBCMASK;    /**< \brief Offset: 0x20 (R/W 32) APBC Mask */
    __IO PM_INTENCLR_Type           INTEN;       /**< \brief Offset: 0x34 (R/W  8) Interrupt Enable Clear */
                                                 /**< \brief Offset: 0x35 (R/W  8) Interrupt Enable Set */
    __IO PM_INTFLAG_Type            INTFLAG;     /**< \brief Offset: 0x36 (R/W  8) Interrupt Flag Status and Clear */
    __I  PM_RCAUSE_Type             RCAUSE;      /**< \brief Offset: 0x38 (R/   8) Reset Cause */
} PM_Type;

typedef struct {
                                              /**< \brief Offset: 0x0 (R/W  8) Control */
                                              /**< \brief Offset: 0x1 (R/   8) Status */
    __IO GCLK_CLKCTRL_Type          CLKCTRL   /**< \brief Offset: 0x2 (R/W 16) Generic Clock Control */
                                    [GCLK_CLKCTRL_ID_I2S_1_Val + 1];
    __IO GCLK_GENCTRL_Type          GENCTRL   /**< \brief Offset: 0x4 (R/W 32) Generic Clock Generator Control */
                                    [GCLK_CLKCTRL_GEN_GCLK8_Val + 1];
    __IO GCLK_GENDIV_Type           GENDIV    /**< \brief Offset: 0x8 (R/W 32) Generic Clock Generator Division */
                                    [GCLK_CLKCTRL_GEN_GCLK8_Val + 1];
} GCLK_Type;

typedef struct {
    __IO NVMCTRL_CTRLA_Type         CTRLA;    /**< \brief Offset: 0x00 (R/W 16) Control A */
    __IO NVMCTRL_CTRLB_Type         CTRLB;    /**< \brief Offset: 0x04 (R/W 32) Control B */
                                              /**< \brief Offset: 0x08 (R/W 32) NVM Parameter */
    __IO NVMCTRL_INTENCLR_Type      INTEN;    /**< \brief Offset: 0x0C (R/W  8) Interrupt Enable Clear */
                                              /**< \brief Offset: 0x10 (R/W  8) Interrupt Enable Set */
    __IO NVMCTRL_INTFLAG_Type       INTFLAG;  /**< \brief Offset: 0x14 (R/W  8) Interrupt Flag Status and Clear */
    __IO NVMCTRL_STATUS_Type        STATUS;   /**< \brief Offset: 0x18 (R/W 16) Status */
    __IO NVMCTRL_ADDR_Type          ADDR;     /**< \brief Offset: 0x1C (R/W 32) Address */
    __IO NVMCTRL_LOCK_Type          LOCK;     /**< \brief Offset: 0x20 (R/W 16) Lock Section */
} NVMCTRL_Type;

typedef union {
    struct {
        uint16_t DATA:9; /*!< bit:  0.. 8  Data Value                         */
        bool VLD:1;      /*!< bit:      9  Valid Flag                         */
        bool OVF:1;      /*!< bit:     10  Overflow Flag                      */
    } bit;               /*!< Structure used for bit  access                  */
    uint16_t reg;        /*!< Type      used for register access              */
} SERCOM_BUFFER_Type;

typedef struct { /* SPI Mode */
    __IO SERCOM_SPI_CTRLA_Type      CTRLA;    /**< \brief Offset: 0x00 (R/W 32) SPI Control A */
    __IO SERCOM_SPI_CTRLB_Type      CTRLB;    /**< \brief Offset: 0x04 (R/W 32) SPI Control B */
    __IO SERCOM_SPI_BAUD_Type       BAUD;     /**< \brief Offset: 0x0C (R/W  8) SPI Baud Rate */
         RoReg8                     Reserved[2];
    __IO SERCOM_SPI_INTENCLR_Type   INTEN;    /**< \brief Offset: 0x14 (R/W  8) SPI Interrupt Enable Clear */
                                              /**< \brief Offset: 0x16 (R/W  8) SPI Interrupt Enable Set */
    __IO SERCOM_SPI_INTFLAG_Type    INTFLAG;  /**< \brief Offset: 0x18 (R/W  8) SPI Interrupt Flag Status and Clear */
    __IO SERCOM_SPI_STATUS_Type     STATUS;   /**< \brief Offset: 0x1A (R/W 16) SPI Status */
                                              /**< \brief Offset: 0x1C (R/  32) SPI Syncbusy */
    __IO SERCOM_SPI_ADDR_Type       ADDR;     /**< \brief Offset: 0x24 (R/W 32) SPI Address */
                                              /**< \brief Offset: 0x28 (R/W 32) SPI Data */
                                              /**< \brief Offset: 0x30 (R/W  8) SPI Debug Control */
         SERCOM_BUFFER_Type         BUFFER[0x4];
         uint8_t                    SLEEPFLAG, SLEEPCOUNT;
         bool                       SS;
} SERCOM_SPI_Type;

typedef struct { /* USART Mode */
    __IO SERCOM_USART_CTRLA_Type    CTRLA;    /**< \brief Offset: 0x00 (R/W 32) USART Control A */
    __IO SERCOM_USART_CTRLB_Type    CTRLB;    /**< \brief Offset: 0x04 (R/W 32) USART Control B */
    __IO SERCOM_USART_BAUD_Type     BAUD;     /**< \brief Offset: 0x0C (R/W 16) USART Baud Rate */
    __IO SERCOM_USART_RXPL_Type     RXPL;     /**< \brief Offset: 0x0E (R/W  8) USART Receive Pulse Length */
    __IO SERCOM_USART_INTENCLR_Type INTEN;    /**< \brief Offset: 0x14 (R/W  8) USART Interrupt Enable Clear */
                                              /**< \brief Offset: 0x16 (R/W  8) USART Interrupt Enable Set */
    __IO SERCOM_USART_INTFLAG_Type  INTFLAG;  /**< \brief Offset: 0x18 (R/W  8) USART Interrupt Flag Status and Clear */
    __IO SERCOM_USART_STATUS_Type   STATUS;   /**< \brief Offset: 0x1A (R/W 16) USART Status */
                                              /**< \brief Offset: 0x1C (R/  32) USART Syncbusy */
         RoReg                      Reserved[1];
                                              /**< \brief Offset: 0x28 (R/W 16) USART Data */
                                              /**< \brief Offset: 0x30 (R/W  8) USART Debug Control */
         SERCOM_BUFFER_Type         BUFFER[0x4];
         uint8_t                    SLEEPFLAG, SLEEPCOUNT;
} SERCOM_USART_Type;

static_assert(offsetof(SERCOM_SPI_Type, CTRLA) == offsetof(SERCOM_USART_Type, CTRLA) &&
              offsetof(SERCOM_SPI_Type, CTRLB) == offsetof(SERCOM_USART_Type, CTRLB) &&
              offsetof(SERCOM_SPI_Type, BAUD) == offsetof(SERCOM_USART_Type, BAUD) &&
              offsetof(SERCOM_SPI_Type, INTEN) == offsetof(SERCOM_USART_Type, INTEN) &&
              offsetof(SERCOM_SPI_Type, INTFLAG) == offsetof(SERCOM_USART_Type, INTFLAG) &&
              offsetof(SERCOM_SPI_Type, STATUS) == offsetof(SERCOM_USART_Type, STATUS) &&
              offsetof(SERCOM_SPI_Type, BUFFER) == offsetof(SERCOM_USART_Type, BUFFER) &&
              offsetof(SERCOM_SPI_Type, SLEEPFLAG) == offsetof(SERCOM_USART_Type, SLEEPFLAG) &&
              offsetof(SERCOM_SPI_Type, SLEEPCOUNT) == offsetof(SERCOM_USART_Type, SLEEPCOUNT),
              "SERCOM_SPI_Type and SERCOM_USART_Type are not compatible!");

typedef union {
         SERCOM_SPI_Type            SPI;      /**< \brief Offset: 0x00 SPI Mode */
         SERCOM_USART_Type          USART;    /**< \brief Offset: 0x00 USART Mode */
} SERCOM_Type;

typedef struct arm_mem {
    uint32_t *ram, *nvm, pb[FLASH_PAGE_SIZE >> 2];
    PM_Type pm;
    GCLK_Type gclk;
    NVMCTRL_Type nvmctrl;
    SERCOM_Type sercom[SERCOM_INST_NUM];
} arm_mem_t;

#ifdef __cplusplus
extern "C" {
#endif

bool arm_mem_init(arm_mem_t *mem);
void arm_mem_destroy(arm_mem_t *mem);
void arm_mem_reset(arm_mem_t *mem, uint8_t rcause);
bool arm_mem_load_rom(arm_mem_t *mem, FILE *file);
void arm_mem_update_pending(arm_t *arm);
uint8_t arm_mem_load_byte(arm_t *arm, uint32_t addr);
uint16_t arm_mem_load_half(arm_t *arm, uint32_t addr);
uint32_t arm_mem_load_word(arm_t *arm, uint32_t addr);
void arm_mem_store_byte(arm_t *arm, uint8_t val, uint32_t addr);
void arm_mem_store_half(arm_t *arm, uint16_t val, uint32_t addr);
void arm_mem_store_word(arm_t *arm, uint32_t val, uint32_t addr);

void arm_mem_spi_sel(arm_t *arm, uint8_t pin, bool low);
uint8_t arm_mem_spi_peek(arm_t *arm, uint8_t pin, uint32_t *res);
void arm_mem_spi_xfer(arm_t *arm, uint8_t pin, uint32_t val);
bool arm_mem_usart_send(arm_t *arm, uint8_t pin, uint16_t *val);
bool arm_mem_usart_recv(arm_t *arm, uint8_t pin, uint16_t val);

#ifdef __cplusplus
}
#endif

#endif
