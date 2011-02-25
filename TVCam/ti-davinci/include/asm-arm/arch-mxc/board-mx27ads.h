/*
 * Copyright 2005-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ASM_ARCH_MXC_BOARD_MX27ADS_H__
#define __ASM_ARCH_MXC_BOARD_MX27ADS_H__

#ifndef __ASM_ARCH_MXC_HARDWARE_H__
#error "Do not include directly."
#endif

/* Size of contiguous memory for DMA and other h/w blocks */
#define CONSISTENT_DMA_SIZE	SZ_8M

/*!
 * @name MXC UART EVB board level configurations
 */
/*!
 * Specify the max baudrate for the MXC UARTs for your board, do not specify
 * a max baudrate greater than 1500000. This is used while specifying the UART
 * Power management constraints.
 */
#define MAX_UART_BAUDRATE       1500000
/*!
 * Specifies if the Irda transmit path is inverting
 */
#define MXC_IRDA_TX_INV         0
/*!
 * Specifies if the Irda receive path is inverting
 */
#define MXC_IRDA_RX_INV         0

/* UART 1 configuration */
/*!
 * This define specifies if the UART port is configured to be in DTE or
 * DCE mode. There exists a define like this for each UART port. Valid
 * values that can be used are \b MODE_DTE or \b MODE_DCE.
 */
#define UART1_MODE              MODE_DCE
/*!
 * This define specifies if the UART is to be used for IRDA. There exists a
 * define like this for each UART port. Valid values that can be used are
 * \b IRDA or \b NO_IRDA.
 */
#define UART1_IR                NO_IRDA
/*!
 * This define is used to enable or disable a particular UART port. If
 * disabled, the UART will not be registered in the file system and the user
 * will not be able to access it. There exists a define like this for each UART
 * port. Specify a value of 1 to enable the UART and 0 to disable it.
 */
#define UART1_ENABLED           1

/* UART 2 configuration */
#define UART2_MODE              MODE_DCE
#define UART2_IR                NO_IRDA
#define UART2_ENABLED           1

/* UART 3 configuration */
#define UART3_MODE              MODE_DCE
#define UART3_IR                IRDA
#define UART3_ENABLED           1

/* UART 4 configuration */
#define UART4_MODE              MODE_DTE
#define UART4_IR                NO_IRDA

/* Disable UART 4 as its pins are shared with ATA */
#define UART4_ENABLED           0

/* UART 5 configuration */
#define UART5_MODE              MODE_DTE
#define UART5_IR                NO_IRDA
#define UART5_ENABLED           1

/* UART 6 configuration */
#define UART6_MODE              MODE_DTE
#define UART6_IR                NO_IRDA
#define UART6_ENABLED           1

#define MXC_LL_EXTUART_PADDR	(CS4_BASE_ADDR + 0x20000)
#define MXC_LL_EXTUART_VADDR	(CS4_BASE_ADDR_VIRT + 0x20000)
#define MXC_LL_EXTUART_16BIT_BUS

#define MXC_LL_UART_PADDR       UART1_BASE_ADDR
#define MXC_LL_UART_VADDR       AIPI_IO_ADDRESS(UART1_BASE_ADDR)

/*!
 * @name Memory Size parameters
 */
#define SDRAM_MEM_SIZE          SZ_128M

/*!
 * @name Keypad Configurations
 */
/* Maximum number of rows (0 to 7) */
#define MXC_KBD_MAXROW          6

/* Maximum number of columns (0 to 7) */
#define MXC_KBD_MAXCOL          6

/*!
 * @name PBC Controller parameters
 */
#define PBC_BASE_ADDRESS        IO_ADDRESS(CS4_BASE_ADDR)
#define PBC_REG_ADDR(offset)    (PBC_BASE_ADDRESS + (offset))

/*!
 * PBC Interupt name definitions
 */
#define PBC_GPIO1_0  0
#define PBC_GPIO1_1  1
#define PBC_GPIO1_2  2
#define PBC_GPIO1_3  3
#define PBC_GPIO1_4  4
#define PBC_GPIO1_5  5

#define PBC_INTR_MAX_NUM 6
#define PBC_INTR_SHARED_MAX_NUM 8

/* When the PBC address connection is fixed in h/w, defined as 1 */
#define PBC_ADDR_SH             0

/* Offsets for the PBC Controller register */
#define PBC_VERSION_REG         PBC_REG_ADDR(0x00000 >> PBC_ADDR_SH)
#define PBC_BCTRL1_SET_REG      PBC_REG_ADDR(0x00008 >> PBC_ADDR_SH)
#define PBC_BCTRL1_CLEAR_REG    PBC_REG_ADDR(0x0000C >> PBC_ADDR_SH)
#define PBC_BCTRL2_SET_REG      PBC_REG_ADDR(0x00010 >> PBC_ADDR_SH)
#define PBC_BCTRL2_CLEAR_REG    PBC_REG_ADDR(0x00014 >> PBC_ADDR_SH)
#define PBC_BCTRL3_SET_REG      PBC_REG_ADDR(0x00018 >> PBC_ADDR_SH)
#define PBC_BCTRL3_CLEAR_REG    PBC_REG_ADDR(0x0001C >> PBC_ADDR_SH)
#define PBC_BCTRL4_SET_REG      PBC_REG_ADDR(0x00020 >> PBC_ADDR_SH)
#define PBC_BCTRL4_CLEAR_REG    PBC_REG_ADDR(0x00024 >> PBC_ADDR_SH)
#define PBC_BSTAT1_REG          PBC_REG_ADDR(0x00028 >> PBC_ADDR_SH)
#define PBC_INTSTATUS_REG       PBC_REG_ADDR(0x0002C >> PBC_ADDR_SH)
#define PBC_INTCURR_STATUS_REG  PBC_REG_ADDR(0x00034 >> PBC_ADDR_SH)
#define PBC_INTMASK_SET_REG     PBC_REG_ADDR(0x00038 >> PBC_ADDR_SH)
#define PBC_INTMASK_CLEAR_REG   PBC_REG_ADDR(0x0003C >> PBC_ADDR_SH)
#define PBC_CS8900A_IOBASE_REG  PBC_REG_ADDR(0x40000 >> PBC_ADDR_SH)
#define PBC_CS8900A_MEMBASE_REG PBC_REG_ADDR(0x42000 >> PBC_ADDR_SH)
#define PBC_CS8900A_DMABASE_REG PBC_REG_ADDR(0x44000 >> PBC_ADDR_SH)

/* PBC Board Version Register bit definitions */
#define PBC_VERSION_ADS         0x8000	/* Bit15=1 means version for ads */
#define PBC_VERSION_EVB_REVB    0x4000	/* BIT14=1 means version for evb revb */

/* PBC Board Control Register 1 bit definitions */
#define PBC_BCTRL1_ERST         0x0001	/* Ethernet Reset */
#define PBC_BCTRL1_URST         0x0002	/* Reset External UART controller */
#define PBC_BCTRL1_FRST         0x0004	/* FEC Reset */
#define PBC_BCTRL1_ESLEEP       0x0010	/* Enable ethernet Sleep */
#define PBC_BCTRL1_LCDON        0x0800	/* Enable the LCD */

/* PBC Board Control Register 2 bit definitions */
#define PBC_BCTRL2_VCC_EN       0x0004	/*   Enable VCC */
#define PBC_BCTRL2_VPP_EN       0x0008	/*   Enable Vpp */
#define PBC_BCTRL2_ATAFEC_EN    0x0010
#define PBC_BCTRL2_ATAFEC_SEL   0x0020
#define PBC_BCTRL2_ATA_EN       0x0040
#define PBC_BCTRL2_IRDA_SD      0x0080
#define PBC_BCTRL2_IRDA_EN      0x0100
#define PBC_BCTRL2_CCTL10       0x0200
#define PBC_BCTRL2_CCTL11       0x0400

/* PBC Board Control Register 3 bit definitions */
#define PBC_BCTRL3_HSH_EN       0x0020
#define PBC_BCTRL3_FSH_MOD      0x0040
#define PBC_BCTRL3_OTG_HS_EN    0x0080
#define PBC_BCTRL3_OTG_VBUS_EN  0x0100
#define PBC_BCTRL3_FSH_VBUS_EN  0x0200
#define PBC_BCTRL3_USB_OTG_ON   0x0800
#define PBC_BCTRL3_USB_FSH_ON   0x1000

/* PBC Board Control Register 4 bit definitions */
#define PBC_BCTRL4_REGEN_SEL    0x0001
#define PBC_BCTRL4_USER_OFF     0x0002
#define PBC_BCTRL4_VIB_EN       0x0004
#define PBC_BCTRL4_PWRGT1_EN    0x0008
#define PBC_BCTRL4_PWRGT2_EN    0x0010
#define PBC_BCTRL4_STDBY_PRI    0x0020

#ifndef __ASSEMBLY__
/*!
 * Enumerations for SD cards and memory stick card. This corresponds to
 * the card EN bits in the IMR: SD1_EN | MS_EN | SD3_EN | SD2_EN.
 */
enum mxc_card_no {
	MXC_CARD_SD2 = 0,
	MXC_CARD_SD3,
	MXC_CARD_MS,
	MXC_CARD_SD1,
	MXC_CARD_MIN = MXC_CARD_SD2,
	MXC_CARD_MAX = MXC_CARD_SD1,
};
#endif

#define MXC_CPLD_VER_1_50       0x01

/*!
 * PBC BSTAT Register bit definitions
 */
#define PBC_BSTAT_PRI_INT       0x0001
#define PBC_BSTAT_USB_BYP       0x0002
#define PBC_BSTAT_ATA_IOCS16    0x0004
#define PBC_BSTAT_ATA_CBLID     0x0008
#define PBC_BSTAT_ATA_DASP      0x0010
#define PBC_BSTAT_PWR_RDY       0x0020
#define PBC_BSTAT_SD3_WP        0x0100
#define PBC_BSTAT_SD2_WP        0x0200
#define PBC_BSTAT_SD1_WP        0x0400
#define PBC_BSTAT_SD3_DET       0x0800
#define PBC_BSTAT_SD2_DET       0x1000
#define PBC_BSTAT_SD1_DET       0x2000
#define PBC_BSTAT_MS_DET        0x4000
#define PBC_BSTAT_SD3_DET_BIT   11
#define PBC_BSTAT_SD2_DET_BIT   12
#define PBC_BSTAT_SD1_DET_BIT   13
#define PBC_BSTAT_MS_DET_BIT    14
#define MXC_BSTAT_BIT(n)        ((n == MXC_CARD_SD2) ? PBC_BSTAT_SD2_DET : \
				 ((n == MXC_CARD_SD3) ? PBC_BSTAT_SD3_DET : \
				 ((n == MXC_CARD_SD1) ? PBC_BSTAT_SD1_DET : \
				 ((n == MXC_CARD_MS) ? PBC_BSTAT_MS_DET : 0))))

/*!
 * PBC UART Control Register bit definitions
 */
#define PBC_UCTRL_DCE_DCD       0x0001
#define PBC_UCTRL_DCE_DSR       0x0002
#define PBC_UCTRL_DCE_RI        0x0004
#define PBC_UCTRL_DTE_DTR       0x0100

/*!
 * PBC UART Status Register bit definitions
 */
#define PBC_USTAT_DTE_DCD       0x0001
#define PBC_USTAT_DTE_DSR       0x0002
#define PBC_USTAT_DTE_RI        0x0004
#define PBC_USTAT_DCE_DTR       0x0100

/*!
 * PBC Interupt mask register bit definitions
 */
#define PBC_INTR_SD3_R_EN_BIT   4
#define PBC_INTR_SD2_R_EN_BIT   0
#define PBC_INTR_SD1_R_EN_BIT   6
#define PBC_INTR_MS_R_EN_BIT    5
#define PBC_INTR_SD3_EN_BIT     13
#define PBC_INTR_SD2_EN_BIT     12
#define PBC_INTR_MS_EN_BIT      14
#define PBC_INTR_SD1_EN_BIT     15

#define PBC_INTR_SD2_R_EN       0x0001
#define PBC_INTR_LOW_BAT        0x0002
#define PBC_INTR_OTG_FSOVER     0x0004
#define PBC_INTR_FSH_OVER       0x0008
#define PBC_INTR_SD3_R_EN       0x0010
#define PBC_INTR_MS_R_EN        0x0020
#define PBC_INTR_SD1_R_EN       0x0040
#define PBC_INTR_FEC_INT        0x0080
#define PBC_INTR_ENET_INT       0x0100
#define PBC_INTR_OTGFS_INT      0x0200
#define PBC_INTR_XUART_INT      0x0400
#define PBC_INTR_CCTL12         0x0800
#define PBC_INTR_SD2_EN         0x1000
#define PBC_INTR_SD3_EN         0x2000
#define PBC_INTR_MS_EN          0x4000
#define PBC_INTR_SD1_EN         0x8000

/* For interrupts like xuart, enet etc */
#define EXPIO_PARENT_INT	IOMUX_TO_IRQ(MX27_PIN_TIN)
#define MXC_MAX_EXP_IO_LINES	16

/*
 * This corresponds to PBC_INTMASK_SET_REG at offset 0x38.
 */
#define EXPIO_INT_LOW_BAT       (MXC_EXP_IO_BASE + 1)
#define EXPIO_INT_OTG_FS_OVR    (MXC_EXP_IO_BASE + 2)
#define EXPIO_INT_FSH_OVR       (MXC_EXP_IO_BASE + 3)
#define EXPIO_INT_RES4          (MXC_EXP_IO_BASE + 4)
#define EXPIO_INT_RES5          (MXC_EXP_IO_BASE + 5)
#define EXPIO_INT_RES6          (MXC_EXP_IO_BASE + 6)
#define EXPIO_INT_FEC           (MXC_EXP_IO_BASE + 7)
#define EXPIO_INT_ENET_INT      (MXC_EXP_IO_BASE + 8)
#define EXPIO_INT_OTG_FS_INT    (MXC_EXP_IO_BASE + 9)
#define EXPIO_INT_XUART_INTA    (MXC_EXP_IO_BASE + 10)
#define EXPIO_INT_CCTL12_INT    (MXC_EXP_IO_BASE + 11)
#define EXPIO_INT_SD2_EN        (MXC_EXP_IO_BASE + 12)
#define EXPIO_INT_SD3_EN        (MXC_EXP_IO_BASE + 13)
#define EXPIO_INT_MS_EN         (MXC_EXP_IO_BASE + 14)
#define EXPIO_INT_SD1_EN        (MXC_EXP_IO_BASE + 15)

/*! This is System IRQ used by CS8900A, taken from platform.h */
#define CS8900AIRQ              EXPIO_INT_ENET_INT

/*! This is I/O Base address used to access registers of CS8900A on MXC ADS */
#define CS8900A_BASE_ADDRESS    (PBC_CS8900A_IOBASE_REG + 0x300)

#endif	/* __ASM_ARCH_MXC_BOARD_MX27ADS_H__ */
