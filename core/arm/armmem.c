#include "armmem.h"

#include "armstate.h"
#include "../defines.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define FLASH_REGION_SIZE      (FLASH_SIZE      >> 4)
#define FLASH_ROW_SIZE         (FLASH_PAGE_SIZE << 2)

#define ARM_SERCOM_SLEEP       4

static uint32_t bitreverse(uint32_t x, uint8_t bits) {
#if __has_builtin(__builtin_bitreverse32)
    return __builtin_bitreverse32(x) >> (32 - bits);
#else
    uint32_t r = 0;
    while (bits--) {
        r <<= 1;
        r |= x & 1;
        x >>= 1;
    }
    return r;
#endif
}

static void arm_mem_set_pending(arm_t *arm, IRQn_Type irq, bool set) {
    if (unlikely(set)) {
        arm->cpu.nvic.ipr |= 1 << irq;
    } else {
        arm->cpu.nvic.ipr &= ~(1 << irq);
    }
}

static void arm_mem_pm_update_pending(arm_t *arm) {
    PM_Type *pm = &arm->mem.pm;
    arm_mem_set_pending(arm, PM_IRQn,
                        pm->INTFLAG.reg &
                        pm->INTEN.reg);
}

static void arm_mem_nvmctrl_update_pending(arm_t *arm) {
    NVMCTRL_Type *nvmctrl = &arm->mem.nvmctrl;
    nvmctrl->INTFLAG.bit.ERROR =
        nvmctrl->STATUS.reg & ~(NVMCTRL_STATUS_PRM |
                                NVMCTRL_STATUS_LOAD);
    arm_mem_set_pending(arm, NVMCTRL_IRQn,
                        nvmctrl->INTFLAG.reg &
                        nvmctrl->INTEN.reg);
}

static void arm_mem_sercom_update_pending(arm_t *arm, uint8_t idx) {
    SERCOM_Type *sercom = &arm->mem.sercom[idx];
    sercom->USART.INTFLAG.bit.ERROR =
        sercom->USART.STATUS.reg & ~SERCOM_USART_STATUS_CTS;
    arm_mem_set_pending(arm, SERCOM0_IRQn + idx,
                        sercom->USART.INTFLAG.reg &
                        sercom->USART.INTEN.reg);
}

void arm_mem_update_pending(arm_t *arm) {
    arm_mem_pm_update_pending(arm);
    arm_mem_nvmctrl_update_pending(arm);
    for (uint8_t idx = 0; idx != SERCOM_INST_NUM; ++idx) {
        arm_mem_sercom_update_pending(arm, idx);
    }
}

static uint16_t arm_nvm_region_lock_mask(NVMCTRL_Type *nvmctrl) {
    return 1 << (nvmctrl->ADDR.bit.ADDR << 1) / FLASH_REGION_SIZE;
}

static bool arm_nvm_region_lock_check(arm_t *arm) {
    NVMCTRL_Type *nvmctrl = &arm->mem.nvmctrl;
    if (nvmctrl->LOCK.bit.LOCK & arm_nvm_region_lock_mask(nvmctrl)) {
        return true;
    }
    nvmctrl->INTFLAG.bit.ERROR = true;
    if (likely(nvmctrl->INTEN.bit.ERROR)) {
        arm_mem_set_pending(arm, NVMCTRL_IRQn, true);
    }
    nvmctrl->STATUS.bit.LOCKE = true;
    return false;
}

static void arm_nvm_clear_page_buffer(arm_mem_t *mem) {
    memset(mem->pb, ~0, FLASH_PAGE_SIZE);
}

static void arm_nvm_erase_row(arm_t *arm, bool aux) {
    assert(!aux && "Not implemented");
    if (likely(arm_nvm_region_lock_check(arm))) {
        memset(&arm->mem.nvm[(arm->mem.nvmctrl.ADDR.bit.ADDR << 1 &
                              (FLASH_SIZE - 1) & -FLASH_ROW_SIZE) >> 2],
               ~0, FLASH_ROW_SIZE);
    }
}

static void arm_nvm_write_page(arm_t *arm, bool aux) {
    uint32_t idx = (arm->mem.nvmctrl.ADDR.bit.ADDR << 1 &
                    (FLASH_SIZE - 1) & -FLASH_PAGE_SIZE) >> 2;
    assert(!aux && "Not implemented");
    if (likely(arm_nvm_region_lock_check(arm))) {
        for (uint32_t off = 0; off != FLASH_PAGE_SIZE >> 2; ++off) {
            arm->mem.nvm[idx + off] &= arm->mem.pb[off];
        }
    }
    arm_nvm_clear_page_buffer(&arm->mem);
}

static void arm_mem_sercom_reset(SERCOM_Type *sercom) {
    memset(sercom, 0, sizeof(*sercom));
}

void arm_mem_spi_sel(arm_t *arm, uint8_t pin, bool low) {
    SERCOM_SPI_Type *spi = &arm->mem.sercom[pin].SPI;
    assert(pin < SERCOM_INST_NUM && "pin out of range");
    if (unlikely(!spi->CTRLA.bit.ENABLE ||
                 spi->CTRLA.bit.MODE !=
                 SERCOM_SPI_CTRLA_MODE_SPI_SLAVE_Val ||
                 !spi->CTRLB.bit.SSDE ||
                 spi->SS == low)) {
        return;
    }
    if ((spi->SS = low)) {
        spi->INTFLAG.bit.SSL = true;
    } else  {
        spi->INTFLAG.bit.TXC = true;
    }
    arm_mem_sercom_update_pending(arm, pin);
}

uint8_t arm_mem_spi_peek(arm_t *arm, uint8_t pin, uint32_t *res) {
    SERCOM_SPI_Type* spi = &arm->mem.sercom[pin].SPI;
    assert(pin < SERCOM_INST_NUM && "pin out of range");
    if (likely(!spi->CTRLA.bit.ENABLE ||
               spi->CTRLA.bit.MODE !=
               SERCOM_SPI_CTRLA_MODE_SPI_SLAVE_Val ||
               !spi->CTRLB.bit.RXEN)) {
        *res = 1 << 15;
        return 16;
    }
    uint8_t bits = 8 | (spi->CTRLB.bit.CHSIZE & 1);
    *res = spi->BUFFER[1].bit.DATA;
    if (unlikely(spi->CTRLA.bit.DORD)) {
        *res = bitreverse(*res, bits);
    }
    return bits;
}

void arm_mem_spi_xfer(arm_t *arm, uint8_t pin, uint32_t val) {
    SERCOM_SPI_Type *spi = &arm->mem.sercom[pin].SPI;
    assert(pin < SERCOM_INST_NUM && "pin out of range");
    if (likely(!spi->CTRLA.bit.ENABLE ||
               spi->CTRLA.bit.MODE !=
               SERCOM_SPI_CTRLA_MODE_SPI_SLAVE_Val ||
               !spi->CTRLB.bit.RXEN)) {
        return;
    }
    if (unlikely(spi->CTRLA.bit.DORD)) {
        uint8_t bits = 8 | (spi->CTRLB.bit.CHSIZE & 1);
        val = bitreverse(val, bits);
    }
    spi->BUFFER[1].bit.DATA = val;
    if (spi->BUFFER[3].bit.VLD) {
        if (spi->CTRLA.bit.IBON) {
            spi->INTFLAG.bit.ERROR = true;
            if (likely(spi->INTEN.bit.ERROR)) {
                arm_mem_set_pending(arm, SERCOM0_IRQn + pin, true);
            }
            spi->STATUS.bit.BUFOVF = true;
        } else {
            spi->BUFFER[2].bit.OVF = true;
        }
        spi->BUFFER[1].bit.VLD = true;
    } else {
        spi->INTFLAG.bit.DRE = true;
        spi->INTFLAG.bit.RXC = true;
        if (likely(spi->INTEN.bit.DRE || spi->INTEN.bit.RXC)) {
            arm_mem_set_pending(arm, SERCOM0_IRQn + pin, true);
        }
        spi->BUFFER[3].reg = spi->BUFFER[2].reg;
        spi->BUFFER[2].reg = spi->BUFFER[1].reg;
        if (likely(spi->BUFFER[1].bit.VLD = spi->BUFFER[0].bit.VLD)) {
            spi->BUFFER[1].bit.DATA = spi->BUFFER[0].bit.DATA;
            spi->BUFFER[0].reg = 0;
        } else if (unlikely(spi->CTRLA.bit.MODE ==
                            SERCOM_SPI_CTRLA_MODE_SPI_MASTER_Val)) {
            spi->INTFLAG.bit.TXC = true;
            if (likely(spi->INTEN.bit.TXC)) {
                arm_mem_set_pending(arm, SERCOM0_IRQn + pin, true);
            }
        }
    }
}

bool arm_mem_usart_send(arm_t *arm, uint8_t pin, uint16_t *val) {
    SERCOM_USART_Type *usart = &arm->mem.sercom[pin].USART;
    assert(pin < SERCOM_INST_NUM && "pin out of range");
    uint16_t tmp;
    if (likely(!usart->CTRLA.bit.ENABLE ||
               usart->CTRLA.bit.MODE !=
               SERCOM_USART_CTRLA_MODE_USART_INT_CLK_Val ||
               !usart->CTRLB.bit.TXEN ||
               !usart->BUFFER[1].bit.VLD)) {
        return false;
    }
    usart->INTFLAG.bit.DRE = true;
    if (likely(usart->INTEN.bit.DRE)) {
        arm_mem_set_pending(arm, SERCOM0_IRQn + pin, true);
    }
    tmp = usart->BUFFER[1].bit.DATA;
    if (likely(usart->BUFFER[1].bit.VLD = usart->BUFFER[0].bit.VLD)) {
        usart->BUFFER[1].bit.DATA = usart->BUFFER[0].bit.DATA;
        usart->BUFFER[0].reg = 0;
    } else {
        usart->INTFLAG.bit.TXC = true;
        if (likely(usart->INTEN.bit.TXC)) {
            arm_mem_set_pending(arm, SERCOM0_IRQn + pin, true);
        }
    }
    if (unlikely(!usart->CTRLA.bit.DORD)) {
        tmp = bitreverse(tmp, !(usart->CTRLB.bit.CHSIZE >> 1) << 3 |
                         usart->CTRLB.bit.CHSIZE);
    }
    *val = tmp;
    return true;
}

bool arm_mem_usart_recv(arm_t *arm, uint8_t pin, uint16_t val) {
    SERCOM_USART_Type *usart = &arm->mem.sercom[pin].USART;
    assert(pin < SERCOM_INST_NUM && "pin out of range");
    if (likely(!usart->CTRLA.bit.ENABLE ||
               usart->CTRLA.bit.MODE !=
               SERCOM_USART_CTRLA_MODE_USART_INT_CLK_Val ||
               !usart->CTRLB.bit.RXEN ||
               usart->BUFFER[3].bit.VLD)) {
        return false;
    }
    if (unlikely(!usart->CTRLA.bit.DORD)) {
        val = bitreverse(val, !(usart->CTRLB.bit.CHSIZE >> 1) << 3 |
                         usart->CTRLB.bit.CHSIZE);
    }
    usart->INTFLAG.bit.RXC = true;
    if (likely(usart->INTEN.bit.RXC)) {
        arm_mem_set_pending(arm, SERCOM0_IRQn + pin, true);
    }
    usart->BUFFER[3].reg = usart->BUFFER[2].reg;
    usart->BUFFER[2].bit.DATA = val;
    usart->BUFFER[2].bit.VLD = true;
    return true;
}

bool arm_mem_init(arm_mem_t *mem) {
    memset(mem, 0, sizeof(*mem));
    mem->nvm = malloc(FLASH_SIZE);
    if (likely(mem->nvm)) {
        memset(mem->nvm, ~0, FLASH_SIZE);
        mem->ram = malloc(HMCRAMC0_SIZE);
        if (likely(mem->ram)) {
            return true;
            free(mem->ram);
        }
        free(mem->nvm);
    }
    return false;
}

void arm_mem_destroy(arm_mem_t *mem) {
    free(mem->nvm);
    free(mem->ram);
}

void arm_mem_reset(arm_mem_t *mem, uint8_t rcause) {
    memset(mem->ram, 0, HMCRAMC0_SIZE);
    arm_nvm_clear_page_buffer(mem);
    mem->pm.RCAUSE.reg = rcause;
    mem->nvmctrl.INTFLAG.bit.READY = true;
    mem->nvmctrl.LOCK.reg = NVMCTRL_LOCK_LOCK_Msk;
    for (uint8_t idx = 0; idx != SERCOM_INST_NUM; ++idx) {
        arm_mem_sercom_reset(&mem->sercom[idx]);
    }
}

bool arm_mem_load_rom(arm_mem_t *mem, FILE *file) {
    size_t read = fread(mem->nvm, 1, FLASH_SIZE, file);
    memset((uint8_t *)mem->nvm + read, ~0, FLASH_SIZE - read);
    return read;
}

static uint32_t arm_mem_load_any(arm_t *arm, uint32_t addr) {
    uint8_t offset = addr >> 2 & 0xFF;
    uint32_t val = 0;
    assert(!(addr & 3) &&
           "Address should be aligned");
    if (likely(addr - FLASH_ADDR < FLASH_SIZE)) { // Internal Flash
        return arm->mem.nvm[(addr - FLASH_ADDR) >> 2];
    } else if (likely(addr - HMCRAMC0_ADDR < HMCRAMC0_SIZE)) { // Internal SRAM
        return arm->mem.ram[(addr - HMCRAMC0_ADDR) >> 2];
    } else if (unlikely(addr < HPB0_ADDR)) {
        if (addr - NVMCTRL_OTP1 < 8) {
            return 0;
        }
        if (addr - NVMCTRL_OTP2 < 8) {
            return 0;
        }
        if (addr - NVMCTRL_OTP4 < 8) {
            return 0;
        }
        if (addr == 0x80A00C) {
            return 0;
        }
        if (addr == 0x80A040) {
            return 0;
        }
        if (addr == 0x80A044) {
            return 0;
        }
        if (addr == 0x80A048) {
            return 0;
        }
    } else if (unlikely(addr < HPB1_ADDR)) { // Peripheral Bridge A
        uint32_t id = ID_PAC0 + ((addr - HPB0_ADDR) >> 10);
        switch (id) {
            case ID_PAC0:
                break;
            case ID_PM: {
                PM_Type *pm = &arm->mem.pm;
                switch (offset) {
                    case (PM_CPUSEL_OFFSET |
                          PM_APBASEL_OFFSET |
                          PM_APBBSEL_OFFSET |
                          PM_APBCSEL_OFFSET) >> 2:
                        return pm->CPUSEL.reg << 0 |
                            pm->APBASEL.reg << 8 |
                            pm->APBBSEL.reg << 16 |
                            pm->APBCSEL.reg << 24;
                    case PM_AHBMASK_OFFSET >> 2:
                        return pm->AHBMASK.reg;
                    case PM_APBAMASK_OFFSET >> 2:
                        return pm->APBAMASK.reg;
                    case PM_APBBMASK_OFFSET >> 2:
                        return pm->APBBMASK.reg;
                    case PM_APBCMASK_OFFSET >> 2:
                        return pm->APBCMASK.reg;
                    case (PM_INTENCLR_OFFSET |
                          PM_INTENSET_OFFSET |
                          PM_INTFLAG_OFFSET) >> 2:
                        return pm->INTEN.reg << 0 |
                            pm->INTEN.reg << 8 |
                            pm->INTFLAG.reg << 16;
                    case PM_RCAUSE_OFFSET >> 2:
                        return pm->RCAUSE.reg;
                }
                break;
            }
            case ID_SYSCTRL:
                switch (offset) {
                    case SYSCTRL_INTENCLR_OFFSET >> 2:
                        break;
                    case SYSCTRL_INTENSET_OFFSET >> 2:
                        break;
                    case SYSCTRL_INTFLAG_OFFSET >> 2:
                        break;
                    case SYSCTRL_PCLKSR_OFFSET >> 2:
                        return SYSCTRL_PCLKSR_XOSCRDY |
                            SYSCTRL_PCLKSR_XOSC32KRDY |
                            SYSCTRL_PCLKSR_OSC32KRDY |
                            SYSCTRL_PCLKSR_OSC8MRDY |
                            SYSCTRL_PCLKSR_DFLLRDY |
                            SYSCTRL_PCLKSR_BOD33RDY |
                            SYSCTRL_PCLKSR_B33SRDY;
                    case SYSCTRL_XOSC_OFFSET >> 2:
                        return SYSCTRL_XOSC_RESETVALUE;
                    case SYSCTRL_XOSC32K_OFFSET >> 2:
                        return SYSCTRL_XOSC32K_RESETVALUE;
                    case SYSCTRL_OSC32K_OFFSET >> 2:
                        return SYSCTRL_OSC32K_RESETVALUE;
                    case SYSCTRL_OSCULP32K_OFFSET >> 2:
                        return SYSCTRL_OSCULP32K_RESETVALUE;
                    case SYSCTRL_OSC8M_OFFSET >> 2:
                        return SYSCTRL_OSC8M_RESETVALUE;
                    case SYSCTRL_DFLLCTRL_OFFSET >> 2:
                        return SYSCTRL_DFLLCTRL_RESETVALUE;
                    case SYSCTRL_DFLLVAL_OFFSET >> 2:
                        return SYSCTRL_DFLLVAL_RESETVALUE;
                    case SYSCTRL_DFLLMUL_OFFSET >> 2:
                        return SYSCTRL_DFLLMUL_RESETVALUE;
                    case SYSCTRL_DFLLSYNC_OFFSET >> 2:
                        return SYSCTRL_DFLLSYNC_RESETVALUE;
                    case SYSCTRL_BOD33_OFFSET >> 2:
                        return SYSCTRL_BOD33_RESETVALUE;
                    case SYSCTRL_VREG_OFFSET >> 2:
                        return SYSCTRL_VREG_RESETVALUE;
                    case SYSCTRL_VREF_OFFSET >> 2:
                        return SYSCTRL_VREF_RESETVALUE;
                    case SYSCTRL_DPLLCTRLA_OFFSET >> 2:
                        return SYSCTRL_DPLLCTRLA_RESETVALUE;
                    case SYSCTRL_DPLLRATIO_OFFSET >> 2:
                        return SYSCTRL_DPLLRATIO_RESETVALUE;
                    case SYSCTRL_DPLLCTRLB_OFFSET >> 2:
                        return SYSCTRL_DPLLCTRLB_RESETVALUE;
                    case SYSCTRL_DPLLSTATUS_OFFSET >> 2:
                        return SYSCTRL_DPLLSTATUS_RESETVALUE;
                }
                break;
            case ID_GCLK: {
                GCLK_Type *gclk = &arm->mem.gclk;
                switch (offset) {
                    case (GCLK_CTRL_OFFSET |
                          GCLK_STATUS_OFFSET |
                          GCLK_CLKCTRL_OFFSET) >> 2:
                        return GCLK_CTRL_RESETVALUE << 0 |
                            GCLK_STATUS_RESETVALUE << 8 |
                            (gclk->CLKCTRL->bit.ID <= GCLK_CLKCTRL_ID_I2S_1_Val ?
                             gclk->CLKCTRL[gclk->CLKCTRL->bit.ID].reg :
                             gclk->CLKCTRL->bit.ID << GCLK_CLKCTRL_ID_Pos) << 16;
                    case GCLK_GENCTRL_OFFSET >> 2:
                        return gclk->GENCTRL->bit.ID <= GCLK_CLKCTRL_GEN_GCLK8_Val ?
                            gclk->GENCTRL[gclk->GENCTRL->bit.ID].reg :
                            gclk->GENCTRL->bit.ID << GCLK_GENCTRL_ID_Pos;
                    case GCLK_GENDIV_OFFSET >> 2:
                        return gclk->GENDIV->bit.ID <= GCLK_CLKCTRL_GEN_GCLK8_Val ?
                            gclk->GENDIV[gclk->GENDIV->bit.ID].reg :
                            gclk->GENDIV->bit.ID << GCLK_GENDIV_ID_Pos;
                }
                break;
            }
            case ID_WDT:
                break;
            case ID_RTC:
                return 0;
            case ID_EIC:
                switch (offset) {
                    case (EIC_CTRL_OFFSET |
                          EIC_STATUS_OFFSET |
                          EIC_NMICTRL_OFFSET |
                          EIC_NMIFLAG_OFFSET) >> 2:
                        return EIC_CTRL_RESETVALUE << 0 |
                            EIC_STATUS_RESETVALUE << 8 |
                            EIC_NMICTRL_RESETVALUE << 16 |
                            EIC_NMIFLAG_RESETVALUE << 24;
                }
                break;
        }
    } else if (unlikely(addr < HPB2_ADDR)) { // Peripheral Bridge B
        if (addr < (uint32_t)DSU) { // PAC1
        } else if (addr < (uint32_t)NVMCTRL) { // DSU
        } else if (addr < (uint32_t)PORT) {
            NVMCTRL_Type *nvmctrl = &arm->mem.nvmctrl;
            switch (offset) {
                case NVMCTRL_CTRLA_OFFSET >> 2:
                    return nvmctrl->CTRLA.reg;
                case NVMCTRL_CTRLB_OFFSET >> 2:
                    return nvmctrl->CTRLB.reg;
                case NVMCTRL_PARAM_OFFSET >> 2:
                    return CONCAT(NVMCTRL_PARAM_PSZ_, FLASH_PAGE_SIZE) |
                        NVMCTRL_PARAM_NVMP(FLASH_NB_OF_PAGES);
                case NVMCTRL_INTENCLR_OFFSET >> 2:
                case NVMCTRL_INTENSET_OFFSET >> 2:
                    return nvmctrl->INTEN.reg;
                case NVMCTRL_INTFLAG_OFFSET >> 2:
                    return nvmctrl->INTFLAG.reg;
                case NVMCTRL_STATUS_OFFSET >> 2:
                    return nvmctrl->STATUS.reg;
                case NVMCTRL_ADDR_OFFSET >> 2:
                    return nvmctrl->ADDR.reg;
                case NVMCTRL_LOCK_OFFSET >> 2:
                    return nvmctrl->LOCK.reg;
            }
        } else if (addr < (uint32_t)DMAC) { // PORT
            switch (offset & 0x1F) {
                case PORT_DIR_OFFSET >> 2:
                    return PORT_DIR_RESETVALUE;
                case PORT_DIRCLR_OFFSET >> 2:
                    return PORT_DIRCLR_RESETVALUE;
                case PORT_DIRSET_OFFSET >> 2:
                    return PORT_DIRSET_RESETVALUE;
                case PORT_DIRTGL_OFFSET >> 2:
                    return PORT_DIRTGL_RESETVALUE;
                case PORT_OUT_OFFSET >> 2:
                    return PORT_OUT_RESETVALUE;
                case PORT_OUTCLR_OFFSET >> 2:
                    return PORT_OUTCLR_RESETVALUE;
                case PORT_OUTSET_OFFSET >> 2:
                    return PORT_OUTSET_RESETVALUE;
                case PORT_OUTTGL_OFFSET >> 2:
                    return PORT_OUTTGL_RESETVALUE;
                case PORT_IN_OFFSET >> 2:
                    return PORT_IN_RESETVALUE;
                case PORT_CTRL_OFFSET >> 2:
                    return PORT_CTRL_RESETVALUE;
                case PORT_WRCONFIG_OFFSET >> 2:
                    return PORT_WRCONFIG_RESETVALUE;
                default:
                    if (addr - (uint32_t)&PORT->Group->PMUX < sizeof(PORT->Group->PMUX)) {
                        return PORT_PMUX_RESETVALUE;
                    } else if (addr - (uint32_t)&PORT->Group->PINCFG < sizeof(PORT->Group->PINCFG)) {
                        return PORT_PINCFG_RESETVALUE;
                    }
                    break;
            }
        } else if (addr < (uint32_t)USB) { // DMAC
            switch (offset) {
                case (DMAC_CRCSTATUS_OFFSET | DMAC_DBGCTRL_OFFSET | DMAC_QOSCTRL_OFFSET) >> 2:
                    return DMAC_CRCSTATUS_RESETVALUE << 0 |
                        DMAC_DBGCTRL_RESETVALUE << 8 |
                        DMAC_QOSCTRL_RESETVALUE << 16;
                case DMAC_SWTRIGCTRL_OFFSET >> 2:
                    return DMAC_SWTRIGCTRL_RESETVALUE;
                case DMAC_BUSYCH_OFFSET >> 2:
                    return DMAC_BUSYCH_RESETVALUE;
                case DMAC_CHCTRLA_OFFSET >> 2:
                    return DMAC_CHCTRLA_RESETVALUE;
            }
        } else if (addr < (uint32_t)MTB) { // USB
            switch (offset) {
                case (USB_CTRLA_OFFSET | USB_SYNCBUSY_OFFSET | USB_QOSCTRL_OFFSET) >> 2:
                    return USB_CTRLA_RESETVALUE << 0 |
                        USB_SYNCBUSY_RESETVALUE << 16 |
                        USB_QOSCTRL_RESETVALUE << 24;
            }
        } else if (addr < (uint32_t)SBMATRIX) { // MTB
        } else { // SBMATRIX
        }
    } else if (likely(addr < (uint32_t)PORT_IOBUS)) { // Peripheral Bridge C
        uint32_t id = ID_PAC2 + ((addr - HPB2_ADDR) >> 10);
        switch (id) {
            case ID_PAC2:
                break;
            case ID_EVSYS:
                switch (offset) {
                    case EVSYS_CTRL_OFFSET >> 2:
                        return EVSYS_CTRL_RESETVALUE;
                }
                break;
            case ID_SERCOM0:
            case ID_SERCOM1:
            case ID_SERCOM2:
            case ID_SERCOM3: {
                SERCOM_Type *sercom = &arm->mem.sercom[id - ID_SERCOM0];
                switch (sercom->USART.CTRLA.bit.MODE) {
                    case SERCOM_USART_CTRLA_MODE_USART_EXT_CLK_Val:
                    case SERCOM_USART_CTRLA_MODE_USART_INT_CLK_Val: {
                        SERCOM_USART_Type *usart = &sercom->USART;
                        switch (offset) {
                            case SERCOM_USART_CTRLA_OFFSET >> 2:
                                return usart->CTRLA.reg;
                            case SERCOM_USART_CTRLB_OFFSET >> 2:
                                return usart->CTRLB.reg;
                            case (SERCOM_USART_BAUD_OFFSET |
                                  SERCOM_USART_RXPL_OFFSET) >> 2:
                                return usart->BAUD.reg << 0 |
                                    usart->RXPL.reg << 16;
                            case (SERCOM_USART_INTENCLR_OFFSET |
                                  SERCOM_USART_INTENSET_OFFSET) >> 2:
                                return usart->INTEN.reg |
                                    usart->INTEN.reg << 16;
                            case (SERCOM_USART_INTFLAG_OFFSET |
                                  SERCOM_USART_STATUS_OFFSET) >> 2:
                                if (usart->SLEEPFLAG != usart->INTFLAG.reg) {
                                    usart->SLEEPFLAG = usart->INTFLAG.reg;
                                    usart->SLEEPCOUNT = 0;
                                } else if (++usart->SLEEPCOUNT == ARM_SERCOM_SLEEP) {
                                    usart->SLEEPCOUNT = 0;
                                    sync_sleep(&arm->sync);
                                }
                                return usart->INTFLAG.reg |
                                    usart->STATUS.reg << 16;
                            case SERCOM_USART_SYNCBUSY_OFFSET >> 2:
                                return SERCOM_USART_SYNCBUSY_RESETVALUE;
                            case SERCOM_USART_DATA_OFFSET >> 2: {
                                SERCOM_BUFFER_Type *buffer = &usart->BUFFER[3];
                                if (!buffer->bit.VLD) {
                                    usart->INTFLAG.bit.RXC = false;
                                    arm_mem_sercom_update_pending(arm, id - ID_SERCOM0);
                                    --buffer;
                                }
                                if (buffer->bit.OVF) {
                                    usart->INTFLAG.bit.ERROR = true;
                                    if (likely(usart->INTEN.bit.ERROR)) {
                                        arm_mem_set_pending(arm, SERCOM0_IRQn +
                                                            id - ID_SERCOM0, true);
                                    }
                                    usart->STATUS.bit.BUFOVF = true;
                                }
                                val = buffer->bit.DATA;
                                buffer->reg = 0;
                                return val;
                            }
                        }
                        break;
                    }
                    case SERCOM_SPI_CTRLA_MODE_SPI_SLAVE_Val:
                    case SERCOM_SPI_CTRLA_MODE_SPI_MASTER_Val: {
                        SERCOM_SPI_Type *spi = &sercom->SPI;
                        switch (offset) {
                            case SERCOM_SPI_CTRLA_OFFSET >> 2:
                                return spi->CTRLA.reg;
                            case SERCOM_SPI_CTRLB_OFFSET >> 2:
                                return spi->CTRLB.reg;
                            case SERCOM_SPI_BAUD_OFFSET >> 2:
                                return spi->BAUD.reg;
                            case (SERCOM_SPI_INTENCLR_OFFSET |
                                  SERCOM_SPI_INTENSET_OFFSET) >> 2:
                                return spi->INTEN.reg |
                                    spi->INTEN.reg << 16;
                            case (SERCOM_SPI_INTFLAG_OFFSET |
                                  SERCOM_SPI_STATUS_OFFSET) >> 2:
                                if (spi->SLEEPFLAG != spi->INTFLAG.reg) {
                                    spi->SLEEPFLAG = spi->INTFLAG.reg;
                                    spi->SLEEPCOUNT = 0;
                                } else if (++spi->SLEEPCOUNT == ARM_SERCOM_SLEEP) {
                                    spi->SLEEPCOUNT = 0;
                                    sync_sleep(&arm->sync);
                                }
                                return spi->INTFLAG.reg |
                                    spi->STATUS.reg << 16;
                            case SERCOM_SPI_SYNCBUSY_OFFSET >> 2:
                                return SERCOM_SPI_SYNCBUSY_RESETVALUE;
                            case SERCOM_SPI_ADDR_OFFSET >> 2:
                                return spi->ADDR.reg;
                            case SERCOM_SPI_DATA_OFFSET >> 2: {
                                SERCOM_BUFFER_Type *buffer = &spi->BUFFER[3];
                                if (!buffer->bit.VLD) {
                                    spi->INTFLAG.bit.RXC = false;
                                    arm_mem_sercom_update_pending(arm, id - ID_SERCOM0);
                                    --buffer;
                                }
                                if (buffer->bit.OVF) {
                                    spi->INTFLAG.bit.ERROR = true;
                                    if (likely(spi->INTEN.bit.ERROR)) {
                                        arm_mem_set_pending(arm, SERCOM0_IRQn +
                                                            id - ID_SERCOM0, true);
                                    }
                                    spi->STATUS.bit.BUFOVF = true;
                                }
                                val = buffer->bit.DATA;
                                buffer->reg = 0;
                                return val;
                            }
                        }
                        break;
                    }
                }
                break;
            }
            case ID_TCC0:
            case ID_TCC1:
            case ID_TCC2:
                break;
            case ID_TC3:
            case ID_TC4:
            case ID_TC5:
                break;
            case ID_ADC:
                break;
            case ID_DAC:
                break;
            case ID_PTC:
                break;
            case ID_I2S:
                break;
        }
    } else if (unlikely(addr - (uint32_t)PORT_IOBUS <= 1u << 10)) { // IOBUS
        switch (offset & 0x1F) {
            case PORT_DIR_OFFSET >> 2:
                return PORT_DIR_RESETVALUE;
            case PORT_DIRCLR_OFFSET >> 2:
                return PORT_DIRCLR_RESETVALUE;
            case PORT_DIRSET_OFFSET >> 2:
                return PORT_DIRSET_RESETVALUE;
            case PORT_DIRTGL_OFFSET >> 2:
                return PORT_DIRTGL_RESETVALUE;
            case PORT_OUT_OFFSET >> 2:
                return PORT_OUT_RESETVALUE;
            case PORT_OUTCLR_OFFSET >> 2:
                return PORT_OUTCLR_RESETVALUE;
            case PORT_OUTSET_OFFSET >> 2:
                return PORT_OUTSET_RESETVALUE;
            case PORT_OUTTGL_OFFSET >> 2:
                return PORT_OUTTGL_RESETVALUE;
            case PORT_IN_OFFSET >> 2:
                return PORT_IN_RESETVALUE;
            case PORT_CTRL_OFFSET >> 2:
                return PORT_CTRL_RESETVALUE;
            case PORT_WRCONFIG_OFFSET >> 2:
                return PORT_WRCONFIG_RESETVALUE;
            default:
                if (addr - (uint32_t)&PORT->Group->PMUX < sizeof(PORT->Group->PMUX)) {
                    return PORT_PMUX_RESETVALUE;
                } else if (addr - (uint32_t)&PORT->Group->PINCFG < sizeof(PORT->Group->PINCFG)) {
                    return PORT_PINCFG_RESETVALUE;
                }
                break;
        }
    }
    if (arm->debug) {
        printf("%08X: load %08X %08X\n", arm->cpu.pc - 4, val, addr);
    }
    arm_cpu_exception(arm, ARM_Exception_HardFault);
    return val;
}

uint8_t arm_mem_load_byte(arm_t *arm, uint32_t addr) {
    return arm_mem_load_any(arm, addr & ~3) >> ((addr & 3) << 3);
}

uint16_t arm_mem_load_half(arm_t *arm, uint32_t addr) {
    uint16_t val = 0;
    if (unlikely(addr & 1)) {
        arm_cpu_exception(arm, ARM_Exception_HardFault);
    } else {
        val = arm_mem_load_any(arm, addr & ~2) >> ((addr & 2) << 3);
    }
    return val;
}

uint32_t arm_mem_load_word(arm_t *arm, uint32_t addr) {
    uint32_t val = 0;
    if (unlikely(addr & 3)) {
        arm_cpu_exception(arm, ARM_Exception_HardFault);
        return val;
    }
    if (likely(addr < PPB_ADDR)) {
        return arm_mem_load_any(arm, addr);
    } else if (unlikely(addr < SCS_BASE)) {
    } else if (unlikely(addr < SysTick_BASE)) {
    } else if (unlikely(addr < NVIC_BASE)) {
        arm_systick_t *systick = &arm->cpu.systick;
        if (addr == (uint32_t)&SysTick->CTRL) {
            val = systick->ctrl;
            systick->ctrl &= ~SysTick_CTRL_COUNTFLAG_Msk;
            return val;
        } else if (addr == (uint32_t)&SysTick->LOAD) {
            return systick->load;
        } else if (addr == (uint32_t)&SysTick->VAL) {
            return systick->val;
        } else if (addr == (uint32_t)&SysTick->CALIB) {
        }
    } else if (unlikely(addr < SCB_BASE)) {
        arm_nvic_t *nvic = &arm->cpu.nvic;
        if (addr == (uint32_t)&NVIC->ISER[0] ||
            addr == (uint32_t)&NVIC->ICER[0]) {
            return nvic->ier;
        } else if (addr == (uint32_t)&NVIC->ISPR[0] ||
                   addr == (uint32_t)&NVIC->ICPR[0]) {
            return nvic->ipr;
        } else if (addr >= (uint32_t)&NVIC->IP[0] &&
                   addr <= (uint32_t)&NVIC->IP[7]) {
            return nvic->ip[addr >> 2 & 7];
        }
    } else {
        arm_scb_t *scb = &arm->cpu.scb;
        if (addr == (uint32_t)&SCB->CPUID) {
            return 'A' << SCB_CPUID_IMPLEMENTER_Pos |
                0 << SCB_CPUID_VARIANT_Pos |
                12 << SCB_CPUID_ARCHITECTURE_Pos |
                28 << SCB_CPUID_PARTNO_Pos |
                6 << SCB_CPUID_REVISION_Pos;
        } else if (addr == (uint32_t)&SCB->ICSR) {
            return scb->icsr;
        } else if (addr == (uint32_t)&SCB->VTOR) {
            return scb->vtor;
        } else if (addr == (uint32_t)&SCB->AIRCR) {
            return 0;
        } else if (addr == (uint32_t)&SCB->CCR) {
            return SCB_CCR_STKALIGN_Msk |
                SCB_CCR_UNALIGN_TRP_Msk;
        } else if (addr == (uint32_t)&SCB->SHP[0]) {
            return scb->shp[0];
        } else if (addr == (uint32_t)&SCB->SHP[1]) {
            return scb->shp[1];
        } else if (addr == (uint32_t)&SCB->SHCSR) {
            return (scb->icsr & SCB_ICSR_PENDSVSET_Msk) >>
                SCB_ICSR_PENDSVSET_Pos << SCB_SHCSR_SVCALLPENDED_Pos;
        }
    }
    if (arm->debug) {
        printf("%08X: load %08X %08X\n", arm->cpu.pc - 4, val, addr);
    }
    arm_cpu_exception(arm, ARM_Exception_HardFault);
    return val;
}

static void arm_mem_store_any(arm_t *arm, uint32_t val, uint32_t mask, uint32_t addr) {
    uint8_t offset = addr >> 2 & 0xFF;
    assert(!((addr & 3) | (val & mask)) &&
           "Address should be aligned and mask should be valid");
    if (unlikely(addr - FLASH_ADDR < FLASH_SIZE)) { // Page Buffer
        if (likely(!(mask & mask >> 16))) { // no 8-bit writes
            uint32_t *ptr = &arm->mem.pb[((addr - FLASH_ADDR) & (FLASH_PAGE_SIZE - 1)) >> 2];
            *ptr = (*ptr & mask) | val;
            arm->mem.nvmctrl.ADDR.bit.ADDR = (addr - FLASH_ADDR) >> 1 | (mask & 1);
            if (unlikely(!(mask >> 16) && !~(addr | -FLASH_PAGE_SIZE | 3) &&
                         !arm->mem.nvmctrl.CTRLB.bit.MANW)) {
                arm_nvm_write_page(arm, false);
            }
            return;
        }
    } else if (likely(addr - HMCRAMC0_ADDR < HMCRAMC0_SIZE)) { // Internal SRAM
        uint32_t *ptr = &arm->mem.ram[(addr - HMCRAMC0_ADDR) >> 2];
        *ptr = (*ptr & mask) | val;
        return;
    } else if (unlikely(addr < HPB0_ADDR)) {
    } else if (unlikely(addr < HPB1_ADDR)) { // Peripheral Bridge A
        uint32_t id = ID_PAC0 + ((addr - HPB0_ADDR) >> 10);
        switch (id) {
            case ID_PAC0:
                break;
            case ID_PM: {
                PM_Type *pm = &arm->mem.pm;
                switch (offset) {
                    case (PM_CPUSEL_OFFSET |
                          PM_APBASEL_OFFSET |
                          PM_APBBSEL_OFFSET |
                          PM_APBCSEL_OFFSET) >> 2:
                        pm->CPUSEL.reg =
                            ((pm->CPUSEL.reg & mask >> 0) | val >> 0) & PM_CPUSEL_MASK;
                        pm->APBASEL.reg =
                            ((pm->APBASEL.reg & mask >> 8) | val >> 8) & PM_APBASEL_MASK;
                        pm->APBBSEL.reg =
                            ((pm->APBBSEL.reg & mask >> 16) | val >> 16) & PM_APBBSEL_MASK;
                        pm->APBCSEL.reg =
                            ((pm->APBCSEL.reg & mask >> 24) | val >> 24) & PM_APBCSEL_MASK;
                        return;
                    case PM_AHBMASK_OFFSET >> 2:
                        pm->AHBMASK.reg = ((pm->AHBMASK.reg & mask) | val) & PM_AHBMASK_MASK;
                        return;
                    case PM_APBAMASK_OFFSET >> 2:
                        pm->APBAMASK.reg = ((pm->APBAMASK.reg & mask) | val) & PM_APBAMASK_MASK;
                        return;
                    case PM_APBBMASK_OFFSET >> 2:
                        pm->APBBMASK.reg = ((pm->APBBMASK.reg & mask) | val) & PM_APBBMASK_MASK;
                        return;
                    case PM_APBCMASK_OFFSET >> 2:
                        pm->APBCMASK.reg = ((pm->APBCMASK.reg & mask) | val) & PM_APBCMASK_MASK;
                        return;
                    case (PM_INTENCLR_OFFSET | PM_INTENSET_OFFSET | PM_INTFLAG_OFFSET) >> 2:
                        pm->INTEN.reg = (pm->INTEN.reg & ~(val >> 0 & PM_INTENCLR_MASK)) |
                            (val >> 8 & PM_INTENSET_MASK);
                        pm->INTFLAG.reg &= ~(val >> 16 & PM_INTFLAG_MASK);
                        arm_mem_pm_update_pending(arm);
                        return;
                }
                break;
            }
            case ID_SYSCTRL:
                switch (offset) {
                    case SYSCTRL_INTENCLR_OFFSET >> 2:
                        break;
                    case SYSCTRL_INTENSET_OFFSET >> 2:
                        break;
                    case SYSCTRL_INTFLAG_OFFSET >> 2:
                        break;
                    case SYSCTRL_PCLKSR_OFFSET >> 2:
                        return;
                    case SYSCTRL_XOSC_OFFSET >> 2:
                        break;
                    case SYSCTRL_XOSC32K_OFFSET >> 2:
                        break;
                    case SYSCTRL_OSC32K_OFFSET >> 2:
                        return;
                    case SYSCTRL_OSCULP32K_OFFSET >> 2:
                        break;
                    case SYSCTRL_OSC8M_OFFSET >> 2:
                        return;
                    case SYSCTRL_DFLLCTRL_OFFSET >> 2:
                        return;
                    case SYSCTRL_DFLLVAL_OFFSET >> 2:
                        return;
                    case SYSCTRL_DFLLMUL_OFFSET >> 2:
                        return;
                    case SYSCTRL_DFLLSYNC_OFFSET >> 2:
                        break;
                    case SYSCTRL_BOD33_OFFSET >> 2:
                        return;
                    case SYSCTRL_VREG_OFFSET >> 2:
                        break;
                    case SYSCTRL_VREF_OFFSET >> 2:
                        break;
                    case SYSCTRL_DPLLCTRLA_OFFSET >> 2:
                        break;
                    case SYSCTRL_DPLLRATIO_OFFSET >> 2:
                        break;
                    case SYSCTRL_DPLLCTRLB_OFFSET >> 2:
                        break;
                    case SYSCTRL_DPLLSTATUS_OFFSET >> 2:
                        break;
                }
                break;
            case ID_GCLK: {
                GCLK_Type *gclk = &arm->mem.gclk;
                switch (offset) {
                    case (GCLK_CTRL_OFFSET | GCLK_STATUS_OFFSET | GCLK_CLKCTRL_OFFSET) >> 2:
                        if (val >> 0 & GCLK_STATUS_SYNCBUSY) {
                            memset(gclk, 0, sizeof(*gclk));
                        } else {
                            gclk->CLKCTRL->bit.ID =
                                (gclk->CLKCTRL->bit.ID & mask >> (16 + GCLK_CLKCTRL_ID_Pos)) |
                                val >> (16 + GCLK_CLKCTRL_ID_Pos);
                            if (gclk->CLKCTRL->bit.ID <= GCLK_CLKCTRL_ID_I2S_1_Val) {
                                gclk->CLKCTRL[gclk->CLKCTRL->bit.ID].reg =
                                    ((gclk->CLKCTRL[gclk->CLKCTRL->bit.ID].reg & mask >> 16) |
                                     val >> 16) & GCLK_CLKCTRL_MASK;
                            }
                        }
                        return;
                    case GCLK_GENCTRL_OFFSET >> 2:
                        gclk->GENCTRL->bit.ID =
                            (gclk->GENCTRL->bit.ID & mask >> GCLK_GENCTRL_ID_Pos) |
                            val >> GCLK_GENCTRL_ID_Pos;
                        if (gclk->GENCTRL->bit.ID <= GCLK_CLKCTRL_GEN_GCLK8_Val) {
                            gclk->GENCTRL[gclk->GENCTRL->bit.ID].reg =
                                ((gclk->GENCTRL[gclk->GENCTRL->bit.ID].reg & mask) | val) &
                                GCLK_GENCTRL_MASK;
                        }
                        return;
                    case GCLK_GENDIV_OFFSET >> 2:
                        gclk->GENDIV->bit.ID =
                            (gclk->GENDIV->bit.ID & mask >> GCLK_GENDIV_ID_Pos) |
                            val >> GCLK_GENDIV_ID_Pos;
                        if (gclk->GENDIV->bit.ID <= GCLK_CLKCTRL_GEN_GCLK8_Val) {
                            gclk->GENDIV[gclk->GENDIV->bit.ID].reg =
                                ((gclk->GENDIV[gclk->GENDIV->bit.ID].reg & mask) | val) &
                                GCLK_GENDIV_MASK;
                        }
                        return;
                }
                break;
            }
            case ID_WDT:
                break;
            case ID_RTC:
                return;
            case ID_EIC:
                switch (offset) {
                    case (EIC_CTRL_OFFSET |
                          EIC_STATUS_OFFSET |
                          EIC_NMICTRL_OFFSET |
                          EIC_NMIFLAG_OFFSET) >> 2:
                        return;
                }
                break;
        }
    } else if (unlikely(addr < HPB2_ADDR)) { // Peripheral Bridge B
        if (addr < (uint32_t)DSU) { // PAC1
        } else if (addr < (uint32_t)NVMCTRL) { // DSU
        } else if (addr < (uint32_t)PORT) {
            NVMCTRL_Type *nvmctrl = &arm->mem.nvmctrl;
            switch (offset) {
                case NVMCTRL_CTRLA_OFFSET >> 2: {
                    nvmctrl->CTRLA.reg = ((nvmctrl->CTRLA.reg & mask) | val) & NVMCTRL_CTRLA_MASK &
                        ~NVMCTRL_CTRLA_CMDEX(1);
                    if ((val & NVMCTRL_CTRLA_CMDEX_Msk) == NVMCTRL_CTRLA_CMDEX_KEY) {
                        switch (val & NVMCTRL_CTRLA_CMD_Msk) {
                            case NVMCTRL_CTRLA_CMD_ER:
                                arm_nvm_erase_row(arm, false);
                                return;
                            case NVMCTRL_CTRLA_CMD_WP:
                                arm_nvm_write_page(arm, false);
                                return;
                            case NVMCTRL_CTRLA_CMD_EAR:
                                arm_nvm_erase_row(arm, true);
                                return;
                            case NVMCTRL_CTRLA_CMD_WAP:
                                arm_nvm_write_page(arm, true);
                                return;
                            case NVMCTRL_CTRLA_CMD_SF:
                                break;
                            case NVMCTRL_CTRLA_CMD_WL:
                                break;
                            case NVMCTRL_CTRLA_CMD_LR:
                                nvmctrl->LOCK.bit.LOCK &= ~arm_nvm_region_lock_mask(nvmctrl);
                                return;
                            case NVMCTRL_CTRLA_CMD_UR:
                                nvmctrl->LOCK.bit.LOCK |= arm_nvm_region_lock_mask(nvmctrl);
                                return;
                            case NVMCTRL_CTRLA_CMD_SPRM:
                                break;
                            case NVMCTRL_CTRLA_CMD_CPRM:
                                break;
                            case NVMCTRL_CTRLA_CMD_PBC:
                                arm_nvm_clear_page_buffer(&arm->mem);
                                return;
                            case NVMCTRL_CTRLA_CMD_SSB:
                                break;
                            case NVMCTRL_CTRLA_CMD_INVALL:
                                return;
                        }
                    }
                    nvmctrl->INTFLAG.bit.ERROR = true;
                    if (likely(nvmctrl->INTEN.bit.ERROR)) {
                        arm_mem_set_pending(arm, NVMCTRL_IRQn, true);
                    }
                    nvmctrl->STATUS.bit.PROGE = true;
                    return;
                }
                case NVMCTRL_CTRLB_OFFSET >> 2:
                    nvmctrl->CTRLB.reg = ((nvmctrl->CTRLB.reg & mask) | val) & NVMCTRL_CTRLB_MASK;
                    return;
                case NVMCTRL_PARAM_OFFSET >> 2:
                    return;
                case NVMCTRL_INTENCLR_OFFSET >> 2:
                    nvmctrl->INTEN.reg &= ~(val & NVMCTRL_INTENCLR_MASK);
                    arm_mem_nvmctrl_update_pending(arm);
                    return;
                case NVMCTRL_INTENSET_OFFSET >> 2:
                    nvmctrl->INTEN.reg |= val & NVMCTRL_INTENCLR_MASK;
                    arm_mem_nvmctrl_update_pending(arm);
                    return;
                case NVMCTRL_INTFLAG_OFFSET >> 2:
                    nvmctrl->INTFLAG.reg &= ~(val & NVMCTRL_INTFLAG_ERROR);
                    arm_mem_nvmctrl_update_pending(arm);
                    return;
                case NVMCTRL_STATUS_OFFSET >> 2:
                    nvmctrl->STATUS.reg &= ~(val & (NVMCTRL_STATUS_NVME  |
                                                    NVMCTRL_STATUS_LOCKE |
                                                    NVMCTRL_STATUS_PROGE |
                                                    NVMCTRL_STATUS_LOAD));
                    arm_mem_nvmctrl_update_pending(arm);
                    return;
                case NVMCTRL_ADDR_OFFSET >> 2:
                    nvmctrl->ADDR.reg = ((nvmctrl->ADDR.reg & mask) | val) & NVMCTRL_ADDR_MASK;
                    return;
                case NVMCTRL_LOCK_OFFSET >> 2:
                    return;
            }
        } else if (addr < (uint32_t)DMAC) { // PORT
            switch (offset & 0x1F) {
                case PORT_DIR_OFFSET >> 2:
                    return;
                case PORT_DIRCLR_OFFSET >> 2:
                    return;
                case PORT_DIRSET_OFFSET >> 2:
                    return;
                case PORT_DIRTGL_OFFSET >> 2:
                    return;
                case PORT_OUT_OFFSET >> 2:
                    return;
                case PORT_OUTCLR_OFFSET >> 2:
                    return;
                case PORT_OUTSET_OFFSET >> 2:
                    return;
                case PORT_OUTTGL_OFFSET >> 2:
                    return;
                case PORT_IN_OFFSET >> 2:
                    return;
                case PORT_CTRL_OFFSET >> 2:
                    return;
                case PORT_WRCONFIG_OFFSET >> 2:
                    return;
                default:
                    if (addr - (uint32_t)&PORT->Group->PMUX < sizeof(PORT->Group->PMUX)) {
                        return;
                    } else if (addr - (uint32_t)&PORT->Group->PINCFG < sizeof(PORT->Group->PINCFG)) {
                        return;
                    }
                    break;
            }
        } else if (addr < (uint32_t)USB) { // DMAC
            switch (offset) {
                case (DMAC_CTRL_OFFSET | DMAC_CRCCTRL_OFFSET) >> 2:
                    return;
                case (DMAC_CRCSTATUS_OFFSET | DMAC_DBGCTRL_OFFSET | DMAC_QOSCTRL_OFFSET) >> 2:
                    return;
                case DMAC_SWTRIGCTRL_OFFSET >> 2:
                    return;
                case DMAC_BASEADDR_OFFSET >> 2:
                    return;
                case DMAC_WRBADDR_OFFSET >> 2:
                    return;
                case DMAC_CHID_OFFSET >> 2:
                    return;
                case DMAC_CHCTRLA_OFFSET >> 2:
                    return;
                case DMAC_CHCTRLB_OFFSET >> 2:
                    return;
            }
        } else if (addr < (uint32_t)MTB) { // USB
            switch (offset) {
                case (USB_CTRLA_OFFSET | USB_SYNCBUSY_OFFSET | USB_QOSCTRL_OFFSET) >> 2:
                    return;
            }
        } else if (addr < (uint32_t)SBMATRIX) { // MTB
            switch (offset) {
                case MTB_MASTER_OFFSET >> 2:
                    return;
            }
        } else { // SBMATRIX
            switch (addr) {
                case (uint32_t)&REG_SBMATRIX_SFR4:
                    return;
            }
        }
    } else if (likely(addr < (uint32_t)PORT_IOBUS)) { // Peripheral Bridge C
        uint32_t id = ID_PAC2 + ((addr - HPB2_ADDR) >> 10);
        switch (id) {
            case ID_PAC2:
                break;
            case ID_EVSYS:
                switch (offset) {
                    case EVSYS_CTRL_OFFSET >> 2:
                        return;
                }
                break;
            case ID_SERCOM0:
            case ID_SERCOM1:
            case ID_SERCOM2:
            case ID_SERCOM3: {
                SERCOM_Type *sercom = &arm->mem.sercom[id - ID_SERCOM0];
                if (unlikely(offset == (SERCOM_USART_CTRLA_OFFSET |
                                        SERCOM_SPI_CTRLA_OFFSET) >> 2)) {
                    val = (sercom->USART.CTRLA.reg & mask) | val;
                    if (unlikely(val & (SERCOM_USART_CTRLA_SWRST |
                                        SERCOM_SPI_CTRLA_SWRST))) {
                        memset(sercom, 0, sizeof(*sercom));
                        return;
                    } else if (unlikely(val & (SERCOM_USART_CTRLA_ENABLE |
                                               SERCOM_SPI_CTRLA_ENABLE))) {
                        sercom->USART.CTRLA.bit.ENABLE = true;
                        sercom->USART.INTFLAG.bit.DRE =
                            !sercom->USART.BUFFER[0].bit.VLD;
                        sercom->USART.INTFLAG.bit.RXC =
                            sercom->USART.BUFFER[2].bit.VLD;
                        if (likely((sercom->USART.INTFLAG.bit.DRE &&
                                    sercom->USART.INTEN.bit.DRE) ||
                                   (sercom->USART.INTFLAG.bit.RXC &&
                                    sercom->USART.INTEN.bit.RXC))) {
                            arm_mem_set_pending(arm, SERCOM0_IRQn +
                                                id - ID_SERCOM0, true);
                        }
                        return;
                    } else {
                        switch ((val & (SERCOM_USART_CTRLA_MODE_Msk |
                                        SERCOM_SPI_CTRLA_MODE_Msk)) >>
                                (SERCOM_USART_CTRLA_MODE_Pos |
                                 SERCOM_SPI_CTRLA_MODE_Pos)) {
                            case SERCOM_USART_CTRLA_MODE_USART_EXT_CLK_Val:
                            case SERCOM_USART_CTRLA_MODE_USART_INT_CLK_Val:
                                sercom->USART.CTRLA.reg = val & SERCOM_USART_CTRLA_MASK;
                                return;
                            case SERCOM_SPI_CTRLA_MODE_SPI_SLAVE_Val:
                            case SERCOM_SPI_CTRLA_MODE_SPI_MASTER_Val:
                                sercom->SPI.CTRLA.reg = val & SERCOM_SPI_CTRLA_MASK;
                                return;
                        }
                    }
                } else {
                    switch (sercom->USART.CTRLA.bit.MODE) {
                        case SERCOM_USART_CTRLA_MODE_USART_EXT_CLK_Val:
                        case SERCOM_USART_CTRLA_MODE_USART_INT_CLK_Val: {
                            SERCOM_USART_Type *usart = &sercom->USART;
                            switch (offset) {
                                case SERCOM_USART_CTRLB_OFFSET >> 2:
                                    if (unlikely((usart->CTRLB.reg ^ val) &
                                                 ~mask & SERCOM_USART_CTRLB_RXEN &&
                                                 !(usart->CTRLB.bit.RXEN ^= true))) {
                                        usart->INTFLAG.bit.RXC = false;
                                        usart->STATUS.bit.COLL = false;
                                        usart->STATUS.bit.ISF = false;
                                        usart->STATUS.bit.BUFOVF = false;
                                        usart->STATUS.bit.FERR = false;
                                        usart->STATUS.bit.PERR = false;
                                        arm_mem_sercom_update_pending(arm, id - ID_SERCOM0);
                                        usart->BUFFER[2].reg = 0;
                                        usart->BUFFER[3].reg = 0;
                                    }
                                    if (unlikely((usart->CTRLB.reg ^ val) &
                                                 ~mask & SERCOM_USART_CTRLB_TXEN)) {
                                        usart->CTRLB.bit.TXEN ^= true;
                                    }
                                    if (likely(!usart->CTRLA.bit.ENABLE)) {
                                        usart->CTRLB.reg = ((usart->CTRLB.reg & mask) | val) &
                                            SERCOM_USART_CTRLB_MASK;
                                    }
                                    return;
                                case (SERCOM_USART_BAUD_OFFSET |
                                      SERCOM_USART_RXPL_OFFSET) >> 2:
                                    if (likely(!usart->CTRLA.bit.ENABLE)) {
                                        usart->BAUD.reg = ((usart->BAUD.reg & mask >> 0) |
                                                           val >> 0) & SERCOM_USART_BAUD_MASK;
                                    }
                                    usart->RXPL.reg = ((usart->RXPL.reg & mask >> 16) |
                                                       val >> 16) & SERCOM_USART_RXPL_MASK;
                                    return;
                                case (SERCOM_USART_INTENCLR_OFFSET |
                                      SERCOM_USART_INTENSET_OFFSET) >> 2:
                                    usart->INTEN.reg = (usart->INTEN.reg &
                                                        ~(val >> 0 & SERCOM_USART_INTENCLR_MASK)) |
                                        (val >> 16 & SERCOM_USART_INTENSET_MASK);
                                    arm_mem_sercom_update_pending(arm, id - ID_SERCOM0);
                                    return;
                                case (SERCOM_USART_INTFLAG_OFFSET |
                                      SERCOM_USART_STATUS_OFFSET) >> 2:
                                    usart->INTFLAG.reg &= ~(val >> 0 & SERCOM_USART_INTFLAG_MASK &
                                                            ~(SERCOM_USART_INTFLAG_RXC |
                                                              SERCOM_USART_INTFLAG_DRE));
                                    usart->STATUS.reg &= ~(val >> 16 & SERCOM_USART_STATUS_MASK &
                                                           ~SERCOM_USART_STATUS_CTS);
                                    arm_mem_sercom_update_pending(arm, id - ID_SERCOM0);
                                    return;
                                case SERCOM_USART_DATA_OFFSET >> 2: {
                                    SERCOM_BUFFER_Type *buffer = &usart->BUFFER[1];
                                    if (likely(buffer->bit.VLD)) {
                                        usart->INTFLAG.bit.DRE = false;
                                        --buffer;
                                    } else {
                                        usart->INTFLAG.bit.TXC = false;
                                    }
                                    arm_mem_sercom_update_pending(arm, id - ID_SERCOM0);
                                    buffer->bit.DATA = val;
                                    buffer->bit.VLD = true;
                                    return;
                                }
                                case SERCOM_USART_DBGCTRL_OFFSET >> 2:
                                    return;
                            }
                            break;
                        }
                        case SERCOM_SPI_CTRLA_MODE_SPI_SLAVE_Val:
                        case SERCOM_SPI_CTRLA_MODE_SPI_MASTER_Val: {
                            SERCOM_SPI_Type *spi = &sercom->SPI;
                            switch (offset) {
                                case SERCOM_SPI_CTRLB_OFFSET >> 2:
                                    if (unlikely((spi->CTRLB.reg ^ val) &
                                                 ~mask & SERCOM_SPI_CTRLB_RXEN &&
                                                 !(spi->CTRLB.bit.RXEN ^= true))) {
                                        spi->INTFLAG.bit.RXC = false;
                                        spi->STATUS.bit.BUFOVF = false;
                                        arm_mem_sercom_update_pending(arm, id - ID_SERCOM0);
                                        spi->BUFFER[2].reg = 0;
                                        spi->BUFFER[3].reg = 0;
                                        spi->SS = false;
                                    }
                                    if (likely(!spi->CTRLA.bit.ENABLE)) {
                                        spi->CTRLB.reg = ((spi->CTRLB.reg & mask) |
                                                          val) & SERCOM_SPI_CTRLB_MASK;
                                    }
                                    return;
                                case SERCOM_SPI_BAUD_OFFSET >> 2:
                                    if (likely(!spi->CTRLA.bit.ENABLE)) {
                                        spi->BAUD.reg = ((spi->BAUD.reg & mask) |
                                                         val) & SERCOM_SPI_BAUD_MASK;
                                    }
                                    return;
                                case (SERCOM_SPI_INTENCLR_OFFSET |
                                      SERCOM_SPI_INTENSET_OFFSET) >> 2:
                                    spi->INTEN.reg = (spi->INTEN.reg &
                                                      ~(val >> 0 & SERCOM_SPI_INTENCLR_MASK)) |
                                        (val >> 16 & SERCOM_SPI_INTENSET_MASK);
                                    arm_mem_sercom_update_pending(arm, id - ID_SERCOM0);
                                    return;
                                case (SERCOM_SPI_INTFLAG_OFFSET |
                                      SERCOM_SPI_STATUS_OFFSET) >> 2:
                                    spi->INTFLAG.reg &= ~(val & SERCOM_SPI_INTFLAG_MASK &
                                                          ~(SERCOM_SPI_INTFLAG_RXC |
                                                            SERCOM_SPI_INTFLAG_DRE));
                                    spi->STATUS.reg &= ~(val >> 16 & SERCOM_SPI_STATUS_MASK);
                                    arm_mem_sercom_update_pending(arm, id - ID_SERCOM0);
                                    return;
                                case SERCOM_SPI_ADDR_OFFSET >> 2:
                                    if (likely(!spi->CTRLA.bit.ENABLE)) {
                                        spi->ADDR.reg = ((spi->ADDR.reg & mask) |
                                                         val) & SERCOM_SPI_ADDR_MASK;
                                    }
                                    return;
                                case SERCOM_SPI_DATA_OFFSET >> 2: {
                                    SERCOM_BUFFER_Type *buffer = &spi->BUFFER[1];
                                    if (likely(!spi->CTRLB.bit.PLOADEN || //spi->SS ||
                                               buffer->bit.VLD)) {
                                        spi->INTFLAG.bit.DRE = false;
                                        --buffer;
                                    } else {
                                        spi->INTFLAG.bit.TXC = false;
                                    }
                                    arm_mem_sercom_update_pending(arm, id - ID_SERCOM0);
                                    buffer->bit.DATA = val;
                                    buffer->bit.VLD = true;
                                    return;
                                }
                                case SERCOM_SPI_DBGCTRL_OFFSET >> 2:
                                    return;
                            }
                            break;
                        }
                    }
                }
                break;
            }
            case ID_TCC0:
            case ID_TCC1:
            case ID_TCC2:
                break;
            case ID_TC3:
            case ID_TC4:
            case ID_TC5:
                break;
            case ID_ADC:
                break;
            case ID_DAC:
                break;
            case ID_PTC:
                break;
            case ID_I2S:
                break;
        }
    } else if (unlikely(addr - (uint32_t)PORT_IOBUS <= 1u << 10)) { // IOBUS
        switch (offset & 0x1F) {
            case PORT_DIR_OFFSET >> 2:
                return;
            case PORT_DIRCLR_OFFSET >> 2:
                return;
            case PORT_DIRSET_OFFSET >> 2:
                return;
            case PORT_DIRTGL_OFFSET >> 2:
                return;
            case PORT_OUT_OFFSET >> 2:
                return;
            case PORT_OUTCLR_OFFSET >> 2:
                return;
            case PORT_OUTSET_OFFSET >> 2:
                return;
            case PORT_OUTTGL_OFFSET >> 2:
                return;
            case PORT_IN_OFFSET >> 2:
                return;
            case PORT_CTRL_OFFSET >> 2:
                return;
            case PORT_WRCONFIG_OFFSET >> 2:
                return;
            default:
                if (addr - (uint32_t)&PORT->Group->PMUX < sizeof(PORT->Group->PMUX)) {
                    return;
                } else if (addr - (uint32_t)&PORT->Group->PINCFG < sizeof(PORT->Group->PINCFG)) {
                    return;
                }
                break;
        }
    }
    if (arm->debug) {
        printf("%08X: store %08X %08X\n", arm->cpu.pc - 4, val, addr);
    }
    arm_cpu_exception(arm, ARM_Exception_HardFault);
}

void arm_mem_store_byte(arm_t *arm, uint8_t val, uint32_t addr) {
    uint32_t shift = (addr & 3) << 3;
    arm_mem_store_any(arm, val << shift, ~(UINT32_C(0xFF) << shift), addr & ~3);
}

void arm_mem_store_half(arm_t *arm, uint16_t val, uint32_t addr) {
    uint32_t shift = (addr & 2) << 3;
    if (unlikely(addr & 1)) {
        arm_cpu_exception(arm, ARM_Exception_HardFault);
        return;
    }
    arm_mem_store_any(arm, val << shift, ~(UINT32_C(0xFFFF) << shift), addr & ~2);
}

void arm_mem_store_word(arm_t *arm, uint32_t val, uint32_t addr) {
    if (unlikely(addr & 3)) {
        arm_cpu_exception(arm, ARM_Exception_HardFault);
        return;
    }
    if (likely(addr < PPB_ADDR)) {
        arm_mem_store_any(arm, val, 0, addr);
        return;
    } else if (unlikely(addr < SCS_BASE)) {
    } else if (unlikely(addr < SysTick_BASE)) {
    } else if (unlikely(addr < NVIC_BASE)) {
        arm_systick_t *systick = &arm->cpu.systick;
        if (addr == (uint32_t)&SysTick->CTRL) {
            systick->ctrl = (systick->ctrl & SysTick_CTRL_COUNTFLAG_Msk) |
                (val & (SysTick_CTRL_CLKSOURCE_Msk |
                        SysTick_CTRL_TICKINT_Msk |
                        SysTick_CTRL_ENABLE_Msk));
            return;
        } else if (addr == (uint32_t)&SysTick->LOAD) {
            systick->load = val & SysTick_LOAD_RELOAD_Msk;
            return;
        } else if (addr == (uint32_t)&SysTick->VAL) {
            systick->ctrl &= ~SysTick_CTRL_COUNTFLAG_Msk;
            systick->val = 0;
            return;
        } else if (addr == (uint32_t)&SysTick->CALIB) {
        }
    } else if (unlikely(addr < SCB_BASE)) {
        arm_nvic_t *nvic = &arm->cpu.nvic;
        if (addr == (uint32_t)&NVIC->ISER[0]) {
            nvic->ier |= val;
            return;
        } else if (addr == (uint32_t)&NVIC->ICER[0]) {
            nvic->ier &= ~val;
            return;
        } else if (addr == (uint32_t)&NVIC->ISPR[0]) {
            nvic->ipr |= val;
            return;
        } else if (addr == (uint32_t)&NVIC->ICPR[0]) {
            nvic->ipr &= ~val;
            return;
        } else if (addr >= (uint32_t)&NVIC->IP[0] && addr <= (uint32_t)&NVIC->IP[7]) {
            nvic->ip[addr >> 2 & 7] = val & UINT32_C(0xC0C0C0C0);
            return;
        }
    } else {
        arm_scb_t *scb = &arm->cpu.scb;
        if (addr == (uint32_t)&SCB->ICSR) {
            if (val & SCB_ICSR_NMIPENDSET_Msk) {
                scb->icsr |= SCB_ICSR_NMIPENDSET_Msk;
            }
            if (val & SCB_ICSR_PENDSVSET_Msk) {
                scb->icsr |= SCB_ICSR_PENDSVSET_Msk;
            } else if (val & SCB_ICSR_PENDSVCLR_Msk) {
                scb->icsr &= ~SCB_ICSR_PENDSVSET_Msk;
            }
            if (val & SCB_ICSR_PENDSTSET_Msk) {
                scb->icsr |= SCB_ICSR_PENDSTSET_Msk;
            } else if (val & SCB_ICSR_PENDSTCLR_Msk) {
                scb->icsr &= ~SCB_ICSR_PENDSTSET_Msk;
            }
            return;
        } else if (addr == (uint32_t)&SCB->VTOR) {
            scb->vtor = val & SCB_VTOR_TBLOFF_Msk;
            return;
        } else if (addr == (uint32_t)&SCB->SHP[0]) {
            scb->shp[0] = val & UINT32_C(0xC0000000);
            return;
        } else if (addr == (uint32_t)&SCB->SHP[1]) {
            scb->shp[1] = val & UINT32_C(0xC0C00000);
            return;
        } else if (addr == (uint32_t)&SCB->SHCSR) {
            if (val & SCB_SHCSR_SVCALLPENDED_Msk) {
                scb->icsr |= SCB_ICSR_PENDSVSET_Msk;
            } else {
                scb->icsr &= ~SCB_ICSR_PENDSVSET_Msk;
            }
        }
    }
    if (arm->debug) {
        printf("%08X: store %08X %08X\n", arm->cpu.pc - 4, val, addr);
    }
    arm_cpu_exception(arm, ARM_Exception_HardFault);
}
