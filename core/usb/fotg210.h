/*
 * Faraday USB 2.0 OTG Controller
 *
 * (C) Copyright 2010 Faraday Technology
 * Dante Su <dantesu@faraday-tech.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _FOTG210_H
#define _FOTG210_H

struct fotg210_regs {
	/* USB Host Controller */
	struct {
		uint32_t data[4];
	} hccr;			/* 0x00 - 0x0f: hccr */
	struct {
		uint32_t usbcmd;           /* 0x10: USB Command */
		uint32_t usbsts;           /* 0x14: USB Status */
		uint32_t usbintr;          /* 0x18: USB Interrupt Enable */
		uint32_t frindex;          /* 0x1c: USB Frame Index */
		uint32_t ctrldssegment;    /* 0x20: 4GB Segment Selector (Reserved) */
		uint32_t periodiclistbase; /* 0x24: Frame List Base Address */
		uint32_t asynclistaddr;    /* 0x28: Next Asynchronous List Address */
		uint32_t rsvd0[1];         /* 0x2c: Reserved */
		uint32_t portsc[1];        /* 0x30: Port Status/Control */
	} hcor;			/* 0x10 - 0x33: hcor */
	uint32_t rsvd1[3];
	uint32_t miscr;	/* 0x40: Miscellaneous Register */
	uint32_t rsvd2[15];
	/* USB OTG Controller */
	uint32_t otgcsr;/* 0x80: OTG Control Status Register */
	uint32_t otgisr;/* 0x84: OTG Interrupt Status Register */
	uint32_t otgier;/* 0x88: OTG Interrupt Enable Register */
	uint32_t rsvd3[13];
	uint32_t isr;	/* 0xc0: Global Interrupt Status Register */
	uint32_t imr;	/* 0xc4: Global Interrupt Mask Register */
	uint32_t rsvd4[14];
	/* USB Device Controller */
	uint32_t dev_ctrl;/* 0x100: Device Control Register */
	uint32_t dev_addr;/* 0x104: Device Address Register */
	uint32_t dev_test;/* 0x108: Device Test Register */
	uint32_t sof_fnr; /* 0x10c: SOF Frame Number Register */
	uint32_t sof_mtr; /* 0x110: SOF Mask Timer Register */
	uint32_t phy_tmsr;/* 0x114: PHY Test Mode Selector Register */
	uint32_t rsvd5[2];
	uint32_t cxfifo;/* 0x120: CX FIFO Register */
	uint32_t idle;	/* 0x124: IDLE Counter Register */
	uint32_t rsvd6[2];
	uint32_t gimr;	/* 0x130: Group Interrupt Mask Register */
	uint32_t gimr0; /* 0x134: Group Interrupt Mask Register 0 */
	uint32_t gimr1; /* 0x138: Group Interrupt Mask Register 1 */
	uint32_t gimr2; /* 0x13c: Group Interrupt Mask Register 2 */
	uint32_t gisr;	/* 0x140: Group Interrupt Status Register */
	uint32_t gisr0; /* 0x144: Group Interrupt Status Register 0 */
	uint32_t gisr1; /* 0x148: Group Interrupt Status Register 1 */
	uint32_t gisr2; /* 0x14c: Group Interrupt Status Register 2 */
	uint32_t rxzlp; /* 0x150: Receive Zero-Length-Packet Register */
	uint32_t txzlp; /* 0x154: Transfer Zero-Length-Packet Register */
	uint32_t isoeasr;/* 0x158: ISOC Error/Abort Status Register */
	uint32_t rsvd7[1];
	uint32_t iep[8]; /* 0x160 - 0x17f: IN Endpoint Register */
	uint32_t oep[8]; /* 0x180 - 0x19f: OUT Endpoint Register */
	uint8_t  epmap[8];/* 0x1a0: Endpoint Map Register (EP1 ~ 8) */
	uint8_t  fifomap[4];/* 0x1a8: FIFO Map Register */
	uint32_t fifocfg; /* 0x1ac: FIFO Configuration Register */
	uint32_t fifocsr[4];/* 0x1b0 - 0x1bf: FIFO Control Status Register */
	uint32_t dma_fifo; /* 0x1c0: DMA Target FIFO Register */
	uint32_t rsvd8[1];
	uint32_t dma_ctrl; /* 0x1c8: DMA Control Register */
	uint32_t dma_addr; /* 0x1cc: DMA Address Register */
};

/* USB Command */
#define USBCMD_ASYNC_ADV_DRBL  (1 << 6)
#define USBCMD_ASYNC_SCHED     (1 << 5)
#define USBCMD_PERIOD_SCHED    (1 << 4)
#define USBCMD_FRLIST_SIZE(x)  (((x) >> 2) & 0x03)
#define USBCMD_FRLIST_ELTS(x)  (1 << (10 - USBCMD_FRLIST_SIZE(x)))
#define USBCMD_FRLIST_BYTES(x) (USBCMD_FRLIST_ELTS(x) << 3)
#define USBCMD_HCRESET         (1 << 1)
#define USBCMD_RUN             (1 << 0)

/* USB Status */
#define USBSTS_ASYNC_SCHED     (1 << 15)
#define USBSTS_PERIOD_SCHED    (1 << 14)
#define USBSTS_RECLAMATION     (1 << 13)
#define USBSTS_HCHALTED        (1 << 12)
#define USBSTS_ASYNC_ADV       (1 << 5)
#define USBSTS_HOST_SYS_ERR    (1 << 4)
#define USBSTS_FRAME_LIST_OVER (1 << 3)
#define USBSTS_PORT_CHANGE     (1 << 2)
#define USBSTS_USBERRINT       (1 << 1)
#define USBSTS_USBINT          (1 << 0)

/* Port Status and Control */
#define PORTSC_J_STATE         (2 << 10)
#define PORTSC_K_STATE         (1 << 10)
#define PORTSC_SE0_STATE       (0 << 10)
#define PORTSC_RESET           (1 << 8)
#define PORTSC_EN_CHANGE       (1 << 3)
#define PORTSC_EN_STATUS       (1 << 2)
#define PORTSC_CONN_CHANGE     (1 << 1)
#define PORTSC_CONN_STATUS     (1 << 0)

/* Miscellaneous Register */
#define MISCR_SUSPEND  (1 << 6) /* Put transceiver in suspend mode */
#define MISCR_EOF2(x)  (((x) & 0x3) << 4) /* EOF 2 Timing */
#define MISCR_EOF1(x)  (((x) & 0x3) << 2) /* EOF 1 Timing */
#define MISCR_ASST(x)  (((x) & 0x3) << 0) /* Async. Sched. Sleep Timer */

/* OTG Control Status Register */
#define OTGCSR_SPD_HIGH     (2 << 22) /* Speed of the attached device (host) */
#define OTGCSR_SPD_LOW      (1 << 22)
#define OTGCSR_SPD_FULL     (0 << 22)
#define OTGCSR_SPD_MASK     (3 << 22)
#define OTGCSR_SPD_SHIFT    22
#define OTGCSR_SPD(x)       (((x) >> 22) & 0x03)
#define OTGCSR_DEV_A        (0 << 21) /* Acts as A-device */
#define OTGCSR_DEV_B        (1 << 21) /* Acts as B-device */
#define OTGCSR_ROLE_H       (0 << 20) /* Acts as Host */
#define OTGCSR_ROLE_D       (1 << 20) /* Acts as Device */
#define OTGCSR_A_VBUS_VLD   (1 << 19) /* A-device VBUS Valid */
#define OTGCSR_A_SESS_VLD   (1 << 18) /* A-device Session Valid */
#define OTGCSR_B_SESS_VLD   (1 << 17) /* B-device Session Valid */
#define OTGCSR_B_SESS_END   (1 << 16) /* B-device Session End */
#define OTGCSR_HFT_LONG     (1 << 11) /* HDISCON noise filter = 270 us*/
#define OTGCSR_HFT          (0 << 11) /* HDISCON noise filter = 135 us*/
#define OTGCSR_VFT_LONG     (1 << 10) /* VBUS noise filter = 472 us*/
#define OTGCSR_VFT          (0 << 10) /* VBUS noise filter = 135 us*/
#define OTGCSR_IDFT_LONG    (1 << 9)  /* ID noise filter = 4 ms*/
#define OTGCSR_IDFT         (0 << 9)  /* ID noise filter = 3 ms*/
#define OTGCSR_A_SRPR_VBUS  (0 << 8)  /* A-device: SRP responds to VBUS */
#define OTGCSR_A_SRPR_DATA  (1 << 8)  /* A-device: SRP responds to DATA-LINE */
#define OTGCSR_A_SRP_EN     (1 << 7)  /* A-device SRP detection enabled */
#define OTGCSR_A_HNP        (1 << 6)  /* Set role=A-device with HNP enabled */
#define OTGCSR_A_BUSDROP    (1 << 5)  /* A-device drop bus (power-down) */
#define OTGCSR_A_BUSREQ     (1 << 4)  /* A-device request bus */
#define OTGCSR_B_VBUS_DISC  (1 << 2)  /* B-device discharges VBUS */
#define OTGCSR_B_HNP        (1 << 1)  /* B-device enable HNP */
#define OTGCSR_B_BUSREQ     (1 << 0)  /* B-device request bus */

/* OTG Interrupt Status Register */
#define OTGISR_APRM         (1 << 12) /* Mini-A plug removed */
#define OTGISR_BPRM         (1 << 11) /* Mini-B plug removed */
#define OTGISR_OVD          (1 << 10) /* over-current detected */
#define OTGISR_IDCHG        (1 << 9)  /* ID(A/B) changed */
#define OTGISR_RLCHG        (1 << 8)  /* Role(Host/Device) changed */
#define OTGISR_BSESSEND     (1 << 6)  /* B-device Session End */
#define OTGISR_AVBUSERR     (1 << 5)  /* A-device VBUS Error */
#define OTGISR_ASRP         (1 << 4)  /* A-device SRP detected */
#define OTGISR_BSRP         (1 << 0)  /* B-device SRP complete */
#define OTGISR_MASK         0x1F71

/* OTG Interrupt Enable Register */
#define OTGIER_APRM         (1 << 12) /* Mini-A plug removed */
#define OTGIER_BPRM         (1 << 11) /* Mini-B plug removed */
#define OTGIER_OVD          (1 << 10) /* over-current detected */
#define OTGIER_IDCHG        (1 << 9)  /* ID(A/B) changed */
#define OTGIER_RLCHG        (1 << 8)  /* Role(Host/Device) changed */
#define OTGIER_BSESSEND     (1 << 6)  /* B-device Session End */
#define OTGIER_AVBUSERR     (1 << 5)  /* A-device VBUS Error */
#define OTGIER_ASRP         (1 << 4)  /* A-device SRP detected */
#define OTGIER_BSRP         (1 << 0)  /* B-device SRP complete */

/* Global Interrupt Status Register (W1C) */
#define ISR_HOST            (1 << 2)  /* USB Host interrupt */
#define ISR_OTG             (1 << 1)  /* USB OTG interrupt */
#define ISR_DEV             (1 << 0)  /* USB Device interrupt */
#define ISR_MASK            0x07

/* Global Interrupt Mask Register */
#define IMR_IRQLH           (1 << 3)  /* Interrupt triggered at level-high */
#define IMR_IRQLL           (0 << 3)  /* Interrupt triggered at level-low */
#define IMR_HOST            (1 << 2)  /* USB Host interrupt */
#define IMR_OTG             (1 << 1)  /* USB OTG interrupt */
#define IMR_DEV             (1 << 0)  /* USB Device interrupt */
#define IMR_MASK            0x0f

/* Device Control Register */
#define DEVCTRL_FS_FORCED   (1 << 9)  /* Forced to be Full-Speed Mode */
#define DEVCTRL_HS          (1 << 6)  /* High Speed Mode */
#define DEVCTRL_FS          (0 << 6)  /* Full Speed Mode */
#define DEVCTRL_EN          (1 << 5)  /* Chip Enable */
#define DEVCTRL_RESET       (1 << 4)  /* Chip Software Reset */
#define DEVCTRL_SUSPEND     (1 << 3)  /* Enter Suspend Mode */
#define DEVCTRL_GIRQ_EN     (1 << 2)  /* Global Interrupt Enabled */
#define DEVCTRL_HALFSPD     (1 << 1)  /* Half speed mode for FPGA test */
#define DEVCTRL_RWAKEUP     (1 << 0)  /* Enable remote wake-up */

/* Device Address Register */
#define DEVADDR_CONF        (1 << 7)  /* SET_CONFIGURATION has been executed */
#define DEVADDR_ADDR(x)     ((x) & 0x7f)
#define DEVADDR_ADDR_MASK   0x7f

/* Device Test Register */
#define DEVTEST_NOSOF       (1 << 6)  /* Do not generate SOF */
#define DEVTEST_TST_MODE    (1 << 5)  /* Enter Test Mode */
#define DEVTEST_TST_NOTS    (1 << 4)  /* Do not toggle sequence */
#define DEVTEST_TST_NOCRC   (1 << 3)  /* Do not append CRC */
#define DEVTEST_TST_CLREA   (1 << 2)  /* Clear External Side Address */
#define DEVTEST_TST_CXLP    (1 << 1)  /* EP0 loopback test */
#define DEVTEST_TST_CLRFF   (1 << 0)  /* Clear FIFO */

/* SOF Frame Number Register */
#define SOFFNR_UFN(x)       (((x) >> 11) & 0x7) /* SOF Micro-Frame Number */
#define SOFFNR_FNR(x)       ((x) & 0x7ff) /* SOF Frame Number */

/* SOF Mask Timer Register */
#define SOFMTR_TMR(x)       ((x) & 0xffff)

/* PHY Test Mode Selector Register */
#define PHYTMSR_TST_PKT     (1 << 4) /* Packet send test */
#define PHYTMSR_TST_SE0NAK  (1 << 3) /* High-Speed quiescent state */
#define PHYTMSR_TST_KSTA    (1 << 2) /* High-Speed K state */
#define PHYTMSR_TST_JSTA    (1 << 1) /* High-Speed J state */
#define PHYTMSR_UNPLUG      (1 << 0) /* Enable soft-detachment */

/* CX FIFO Register */
#define CXFIFO_GET_BYTES(x) (((x) >> 24) & 0x7f) /* CX/EP0 FIFO byte count */
#define CXFIFO_SET_BYTES(x, bytes) (((x) & 0xffffff) | (((bytes) & 0x7f) << 24))
#define CXFIFO_FIFOE(x)     (1 << (((x) & 0x03) + 8)) /* EPx FIFO empty */
#define CXFIFO_FIFOE_FIFO0  (1 << 8)
#define CXFIFO_FIFOE_FIFO1  (1 << 9)
#define CXFIFO_FIFOE_FIFO2  (1 << 10)
#define CXFIFO_FIFOE_FIFO3  (1 << 11)
#define CXFIFO_FIFOE_MASK   (0x0f << 8)
#define CXFIFO_CXFIFOE      (1 << 5) /* CX FIFO empty */
#define CXFIFO_CXFIFOF      (1 << 4) /* CX FIFO full */
#define CXFIFO_CXFIFOCLR    (1 << 3) /* CX FIFO clear */
#define CXFIFO_CXSTALL      (1 << 2) /* CX Stall */
#define CXFIFO_TSTPKTFIN    (1 << 1) /* Test packet data transfer finished */
#define CXFIFO_CXFIN        (1 << 0) /* CX data transfer finished */

/* IDLE Counter Register */
#define IDLE_MS(x)          ((x) & 0x07) /* PHY suspend delay = x ms */

/* Group Interrupt Mask(Disable) Register */
#define GIMR_GRP2           (1 << 2) /* Disable interrupt group 2 */
#define GIMR_GRP1           (1 << 1) /* Disable interrupt group 1 */
#define GIMR_GRP0           (1 << 0) /* Disable interrupt group 0 */
#define GIMR_MASK           0x07

/* Group Interrupt Mask(Disable) Register 0 (CX) */
#define GIMR0_CXABORT       (1 << 5) /* CX command abort interrupt */
#define GIMR0_CXERR         (1 << 4) /* CX command error interrupt */
#define GIMR0_CXEND         (1 << 3) /* CX command end interrupt */
#define GIMR0_CXOUT         (1 << 2) /* EP0-OUT packet interrupt */
#define GIMR0_CXIN          (1 << 1) /* EP0-IN packet interrupt */
#define GIMR0_CXSETUP       (1 << 0) /* EP0-SETUP packet interrupt */
#define GIMR0_MASK          0x3f

/* Group Interrupt Mask(Disable) Register 1 (FIFO) */
#define GIMR1_FIFO_IN(x)    (1 << (((x) & 3) + 16))    /* FIFOx IN */
#define GIMR1_FIFO_TX(x)    GIMR1_FIFO_IN(x)
#define GIMR1_FIFO_OUT(x)   (1 << (((x) & 3) * 2))     /* FIFOx OUT */
#define GIMR1_FIFO_SPK(x)   (1 << (((x) & 3) * 2 + 1)) /* FIFOx SHORT PACKET */
#define GIMR1_FIFO_RX(x)    (GIMR1_FIFO_OUT(x) | GIMR1_FIFO_SPK(x))
#define GIMR1_MASK          0xf00ff

/* Group Interrupt Mask(Disable) Register 2 (Device) */
#define GIMR2_WAKEUP        (1 << 10) /* Device waked up */
#define GIMR2_IDLE          (1 << 9)  /* Device idle */
#define GIMR2_DMAERR        (1 << 8)  /* DMA error */
#define GIMR2_DMAFIN        (1 << 7)  /* DMA finished */
#define GIMR2_ZLPRX         (1 << 6)  /* Zero-Length-Packet Rx Interrupt */
#define GIMR2_ZLPTX         (1 << 5)  /* Zero-Length-Packet Tx Interrupt */
#define GIMR2_ISOCABT       (1 << 4)  /* ISOC Abort Interrupt */
#define GIMR2_ISOCERR       (1 << 3)  /* ISOC Error Interrupt */
#define GIMR2_RESUME        (1 << 2)  /* Resume state change Interrupt */
#define GIMR2_SUSPEND       (1 << 1)  /* Suspend state change Interrupt */
#define GIMR2_RESET         (1 << 0)  /* Reset Interrupt */
#define GIMR2_MASK          0x7ff

/* Group Interrupt Status Register */
#define GISR_GRP2           (1 << 2) /* Interrupt group 2 */
#define GISR_GRP1           (1 << 1) /* Interrupt group 1 */
#define GISR_GRP0           (1 << 0) /* Interrupt group 0 */

/* Group Interrupt Status Register 0 (CX) */
#define GISR0_CXABORT       (1 << 5) /* CX command abort interrupt */
#define GISR0_CXERR         (1 << 4) /* CX command error interrupt */
#define GISR0_CXEND         (1 << 3) /* CX command end interrupt */
#define GISR0_CXOUT         (1 << 2) /* EP0-OUT packet interrupt */
#define GISR0_CXIN          (1 << 1) /* EP0-IN packet interrupt */
#define GISR0_CXSETUP       (1 << 0) /* EP0-SETUP packet interrupt */

/* Group Interrupt Status Register 1 (FIFO) */
#define GISR1_IN_FIFO(x)    (1 << (((x) & 0x03) + 16))    /* FIFOx IN */
#define GISR1_OUT_FIFO(x)   (1 << (((x) & 0x03) * 2))     /* FIFOx OUT */
#define GISR1_SPK_FIFO(x)   (1 << (((x) & 0x03) * 2 + 1)) /* FIFOx SPK */
#define GISR1_RX_FIFO(x)    (3 << (((x) & 0x03) * 2))     /* FIFOx OUT/SPK */

/* Group Interrupt Status Register 2 (Device) */
#define GISR2_WAKEUP        (1 << 10) /* Device waked up */
#define GISR2_IDLE          (1 << 9)  /* Device idle */
#define GISR2_DMAERR        (1 << 8)  /* DMA error */
#define GISR2_DMAFIN        (1 << 7)  /* DMA finished */
#define GISR2_ZLPRX         (1 << 6)  /* Zero-Length-Packet Rx Interrupt */
#define GISR2_ZLPTX         (1 << 5)  /* Zero-Length-Packet Tx Interrupt */
#define GISR2_ISOCABT       (1 << 4)  /* ISOC Abort Interrupt */
#define GISR2_ISOCERR       (1 << 3)  /* ISOC Error Interrupt */
#define GISR2_RESUME        (1 << 2)  /* Resume state change Interrupt */
#define GISR2_SUSPEND       (1 << 1)  /* Suspend state change Interrupt */
#define GISR2_RESET         (1 << 0)  /* Reset Interrupt */

/* Receive Zero-Length-Packet Register */
#define RXZLP_EP(x)         (1 << ((x) - 1)) /* EPx ZLP rx interrupt */

/* Transfer Zero-Length-Packet Register */
#define TXZLP_EP(x)         (1 << ((x) - 1)) /* EPx ZLP tx interrupt */

/* ISOC Error/Abort Status Register */
#define ISOEASR_EP(x)       (0x10001 << ((x) - 1)) /* EPx ISOC Error/Abort */

/* IN Endpoint Register */
#define IEP_SENDZLP         (1 << 15)     /* Send Zero-Length-Packet */
#define IEP_TNRHB(x)        (((x) & 0x03) << 13) \
	/* Transaction Number for High-Bandwidth EP(ISOC) */
#define IEP_RESET           (1 << 12)     /* Reset Toggle Sequence */
#define IEP_STALL           (1 << 11)     /* Stall */

/* OUT Endpoint Register */
#define OEP_RESET           (1 << 12)     /* Reset Toggle Sequence */
#define OEP_STALL           (1 << 11)     /* Stall */
#define EP_MAXPS(x)         ((x) & 0x7ff) /* Max. packet size */

/* Endpoint Map Register (EP1 ~ EP8) */
#define EPMAP_SET_IN(fifo)  (((fifo) & 3) << 0)
#define EPMAP_SET_OUT(fifo) (((fifo) & 3) << 4)
#define EPMAP_GET_IN(x)     ((x) >> 0 & 3)
#define EPMAP_GET_OUT(x)    ((x) >> 4 & 3)

/* FIFO Map Register */
#define FIFOMAP_BIDIR       (2 << 4)
#define FIFOMAP_IN          (1 << 4)
#define FIFOMAP_OUT         (0 << 4)
#define FIFOMAP_DIR_MASK    0x30
#define FIFOMAP_EP_MASK     0x0f
#define FIFOMAP_EP(x)       ((x) & FIFOMAP_EP_MASK)
#define FIFOMAP_CFG_MASK    0x3f
#define FIFOMAP_DEFAULT     0x04030201 /* FIFO0->EP1, FIFO1->EP2... */
#define FIFOMAP(fifo, cfg)  (((cfg) & 0x3f) << (((fifo) & 3) << 3))

/* FIFO Configuration Register */
#define FIFOCFG_EN          (1 << 5)
#define FIFOCFG_BLKSZ_1024  (1 << 4)
#define FIFOCFG_BLKSZ_512   (0 << 4)
#define FIFOCFG_3BLK        (2 << 2)
#define FIFOCFG_2BLK        (1 << 2)
#define FIFOCFG_1BLK        (0 << 2)
#define FIFOCFG_NBLK_MASK   3
#define FIFOCFG_NBLK_SHIFT  2
#define FIFOCFG_INTR        (3 << 0)
#define FIFOCFG_BULK        (2 << 0)
#define FIFOCFG_ISOC        (1 << 0)
#define FIFOCFG_RSVD        (0 << 0)  /* Reserved */
#define FIFOCFG_TYPE_MASK   3
#define FIFOCFG_TYPE_SHIFT  0
#define FIFOCFG_CFG_MASK    0x3f
#define FIFOCFG(fifo, cfg)  (((cfg) & 0x3f) << (((fifo) & 3) << 3))

/* FIFO Control Status Register */
#define FIFOCSR_RESET       (1 << 12) /* FIFO Reset */
#define FIFOCSR_BYTES(x)    ((x) & 0x7ff) /* Length(bytes) for OUT-EP/FIFO */

/* DMA Target FIFO Register */
#define DMAFIFO_CX          (1 << 4) /* DMA FIFO = CX FIFO */
#define DMAFIFO_FIFO(x)     (1 << ((x) & 0x3)) /* DMA FIFO = FIFOx */

/* DMA Control Register */
#define DMACTRL_LEN_SHIFT   8
#define DMACTRL_LEN_MASK    0x1ffff
#define DMACTRL_LEN(x)      (((x) >> DMACTRL_LEN_SHIFT) & DMACTRL_LEN_MASK) /* DMA length (Bytes) */
#define DMACTRL_CLRFF       (1 << 4) /* Clear FIFO upon DMA abort */
#define DMACTRL_ABORT       (1 << 3) /* DMA abort */
#define DMACTRL_IO2IO       (1 << 2) /* IO to IO */
#define DMACTRL_FIFO2MEM    (0 << 1) /* FIFO to Memory */
#define DMACTRL_MEM2FIFO    (1 << 1) /* Memory to FIFO */
#define DMACTRL_START       (1 << 0) /* DMA start */

#endif
