/*
 * Table of the DAVINCI register configurations for the PINMUX combinations
 *
 * Author: Vladimir Barinov, MontaVista Software, Inc.
 * Copyright (C) 2007-2008 MontaVista Software, Inc. <source@mvista.com>
 *
 * Based on linux/include/asm-arm/arch-omap/mux.h:
 * Copyright (C) 2003 - 2005 Nokia Corporation
 * Written by Tony Lindgren <tony.lindgren@nokia.com>
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */

#ifndef __ASM_ARCH_MUX_H
#define __ASM_ARCH_MUX_H

#define PINMUX_UNUSED	0
#define PINMUX_FREE	1
#define PINMUX_RESV	2

#define MUX_CFG(desc, reg, offset, mode_mask, mux_mode, dbg)	\
{ \
	.name =	 desc, \
		.mux_reg	= PINMUX##reg,			\
		.reg_index	= reg,				\
		.mask_offset	= offset,			\
		.mask		= mode_mask,			\
		.mode		= mux_mode,			\
	.debug = dbg, \
},

/* DA8xx uses 4 bits per mode, so there's no need to parametrize mask */
#define DA8XX_MUX_CFG(desc, reg, offset, mux_mode, dbg) 	\
{ \
	.name =	 desc, \
		.mux_reg	= DA8XX_PINMUX(reg),		\
		.reg_index	= reg,				\
		.mask_offset	= offset,			\
		.mask		= 0xf,				\
		.mode		= mux_mode,			\
	.debug = dbg, \
},

struct pin_config {
	const char *name;
	unsigned long mux_reg;
	unsigned char reg_index;
	unsigned char mask_offset;
	unsigned char mask;
	unsigned char mode;
	unsigned char debug;
};

enum davinci_dm644x_index {
	/* ATA and HDDIR functions */
	DM644X_HDIREN,
	DM644X_ATAEN,

	/* Memory Stick */
	DM644X_MSTK,

	/* I2C */
	DM644X_I2C,

	/* ASP function */
	DM644X_MCBSP,

	/* PWM0 */
	DM644X_PWM0,

	/* PWM1 */
	DM644X_PWM1,

	/* PWM2 */
	DM644X_PWM2,

	/* VLINQ function */
	DM644X_VLINQEN,
	DM644X_VLINQWD,

	/* EMAC and MDIO function */
	DM644X_EMACEN,

	/* GPIO3V[0:16] pins */
	DM644X_GPIO3V,

	/* GPIO pins */
	DM644X_GPIO0,
	DM644X_GPIO3,
	DM644X_GPIO43_44,
	DM644X_GPIO46_47,

	/* VPBE */
	DM644X_RGB666,

	/* LCD */
	DM644X_LOEEN,
	DM644X_LFLDEN,
};

enum davinci_dm357_index {

	/* I2C */
	DM357_I2C,

	/* ASP function */
	DM357_MCBSP,

	/* PWM0 */
	DM357_PWM0,

	/* PWM1 */
	DM357_PWM1,

	/* PWM2 */
	DM357_PWM2,

	/* EMAC and MDIO function */
	DM357_EMACEN,

	/* GPIO3V[0:16] pins */
	DM357_GPIO3V,

	/* GPIO pins */
	DM357_GPIO0,
	DM357_GPIO3,
	DM357_GPIO43_44,
	DM357_GPIO46_47,

	/* VPBE */
	DM357_RGB666,

	/* LCD */
	DM357_LOEEN,
	DM357_LFLDEN,

	/* Chip enable */
	DM357_AECS4,
	DM357_AECS5,
};

enum davinci_dm646x_index {
        /* ATA function */
        DM646X_ATAEN,

        /* USB */
        DM646X_VBUSDIS,
        DM646X_VBUSDIS_GPIO22,

        /* STC source clock input */
        DM646X_STCCK,

        /* AUDIO Clock */
        DM646X_AUDCK1,
        DM646X_AUDCK0,

        /* CRGEN Control */
        DM646X_CRGMUX,

        /* VPIF Control */
        DM646X_STSOMUX_DISABLE,
        DM646X_STSIMUX_DISABLE,
        DM646X_PTSOMUX_DISABLE,
        DM646X_PTSIMUX_DISABLE,

        /* TSIF Control */
        DM646X_STSOMUX,
        DM646X_STSIMUX,
        DM646X_PTSOMUX_PARALLEL,
        DM646X_PTSIMUX_PARALLEL,
        DM646X_PTSOMUX_SERIAL,
        DM646X_PTSIMUX_SERIAL,
};

enum davinci_dm355_index {
	/* MMC/SD 0 */
	DM355_MMCSD0,

	/* MMC/SD 1 */
	DM355_SD1_CLK,
	DM355_SD1_CMD,
	DM355_SD1_DATA3,
	DM355_SD1_DATA2,
	DM355_SD1_DATA1,
	DM355_SD1_DATA0,

	/* I2C */
	DM355_I2C_SDA,
	DM355_I2C_SCL,

	/* ASP function */
	DM355_MCBSP0_BDX,
	DM355_MCBSP0_X,
	DM355_MCBSP0_BFSX,
	DM355_MCBSP0_BDR,
	DM355_MCBSP0_R,
	DM355_MCBSP0_BFSR,

	/* PWM0 */
	DM355_PWM0,

	/* PWM1 */
	DM355_PWM1,

	/* PWM2 */
	DM355_PWM2_G76,
	DM355_PWM2_G77,
	DM355_PWM2_G78,
	DM355_PWM2_G79,

	/* PWM3 */
	DM355_PWM3_G69,
	DM355_PWM3_G70,
	DM355_PWM3_G74,
	DM355_PWM3_G75,

	/* SPI0 */
	DM355_SPI0_SDI,
	DM355_SPI0_SDENA0,
	DM355_SPI0_SDENA1,

	/* SPI1 */
	DM355_SPI1_SCLK,
	DM355_SPI1_SDO,
	DM355_SPI1_SDENA0,
	DM355_SPI1_SDENA1,

	/* SPI2 */
	DM355_SPI2_SCLK,
	DM355_SPI2_SDO,
	DM355_SPI2_SDENA0,
	DM355_SPI2_SDENA1,

	/* GPIO */
	DM355_GPIO14,
	DM355_GPIO15,
	DM355_GPIO71,

	/* Video Out */
	DM355_VOUT_FIELD,
	DM355_VOUT_FIELD_G70,
	DM355_VOUT_HVSYNC,
	DM355_VOUT_COUTL_EN,
	DM355_VOUT_COUTH_EN,

	/* Video In */
	DM355_VIN_PCLK,
	DM355_VIN_CAM_WEN,
	DM355_VIN_CAM_VD,
	DM355_VIN_CAM_HD,
	DM355_VIN_YIN_EN,
	DM355_VIN_CINL_EN,
	DM355_VIN_CINH_EN,
};

enum davinci_da8xx_index {
	DA8XX_GPIO7_14,
	DA8XX_RTCK,
	DA8XX_GPIO7_15,
	DA8XX_EMU_0,
	DA8XX_EMB_SDCKE,
	DA8XX_EMB_CLK_GLUE,
	DA8XX_EMB_CLK,
	DA8XX_NEMB_CS_0,
	DA8XX_NEMB_CAS,
	DA8XX_NEMB_RAS,
	DA8XX_NEMB_WE,
	DA8XX_EMB_BA_1,
	DA8XX_EMB_BA_0,
	DA8XX_EMB_A_0,
	DA8XX_EMB_A_1,
	DA8XX_EMB_A_2,
	DA8XX_EMB_A_3,
	DA8XX_EMB_A_4,
	DA8XX_EMB_A_5,
	DA8XX_GPIO7_0,
	DA8XX_GPIO7_1,
	DA8XX_GPIO7_2,
	DA8XX_GPIO7_3,
	DA8XX_GPIO7_4,
	DA8XX_GPIO7_5,
	DA8XX_GPIO7_6,
	DA8XX_GPIO7_7,
	DA8XX_EMB_A_6,
	DA8XX_EMB_A_7,
	DA8XX_EMB_A_8,
	DA8XX_EMB_A_9,
	DA8XX_EMB_A_10,
	DA8XX_EMB_A_11,
	DA8XX_EMB_A_12,
	DA8XX_EMB_D_31,
	DA8XX_GPIO7_8,
	DA8XX_GPIO7_9,
	DA8XX_GPIO7_10,
	DA8XX_GPIO7_11,
	DA8XX_GPIO7_12,
	DA8XX_GPIO7_13,
	DA8XX_GPIO3_13,
	DA8XX_EMB_D_30,
	DA8XX_EMB_D_29,
	DA8XX_EMB_D_28,
	DA8XX_EMB_D_27,
	DA8XX_EMB_D_26,
	DA8XX_EMB_D_25,
	DA8XX_EMB_D_24,
	DA8XX_EMB_D_23,
	DA8XX_EMB_D_22,
	DA8XX_EMB_D_21,
	DA8XX_EMB_D_20,
	DA8XX_EMB_D_19,
	DA8XX_EMB_D_18,
	DA8XX_EMB_D_17,
	DA8XX_EMB_D_16,
	DA8XX_NEMB_WE_DQM_3,
	DA8XX_NEMB_WE_DQM_2,
	DA8XX_EMB_D_0,
	DA8XX_EMB_D_1,
	DA8XX_EMB_D_2,
	DA8XX_EMB_D_3,
	DA8XX_EMB_D_4,
	DA8XX_EMB_D_5,
	DA8XX_EMB_D_6,
	DA8XX_GPIO6_0,
	DA8XX_GPIO6_1,
	DA8XX_GPIO6_2,
	DA8XX_GPIO6_3,
	DA8XX_GPIO6_4,
	DA8XX_GPIO6_5,
	DA8XX_GPIO6_6,
	DA8XX_EMB_D_7,
	DA8XX_EMB_D_8,
	DA8XX_EMB_D_9,
	DA8XX_EMB_D_10,
	DA8XX_EMB_D_11,
	DA8XX_EMB_D_12,
	DA8XX_EMB_D_13,
	DA8XX_EMB_D_14,
	DA8XX_GPIO6_7,
	DA8XX_GPIO6_8,
	DA8XX_GPIO6_9,
	DA8XX_GPIO6_10,
	DA8XX_GPIO6_11,
	DA8XX_GPIO6_12,
	DA8XX_GPIO6_13,
	DA8XX_GPIO6_14,
	DA8XX_EMB_D_15,
	DA8XX_NEMB_WE_DQM_1,
	DA8XX_NEMB_WE_DQM_0,
	DA8XX_SPI0_SOMI_0,
	DA8XX_SPI0_SIMO_0,
	DA8XX_SPI0_CLK,
	DA8XX_NSPI0_ENA,
	DA8XX_NSPI0_SCS_0,
	DA8XX_EQEP0I,
	DA8XX_EQEP0S,
	DA8XX_EQEP1I,
	DA8XX_NUART0_CTS,
	DA8XX_NUART0_RTS,
	DA8XX_EQEP0A,
	DA8XX_EQEP0B,
	DA8XX_GPIO6_15,
	DA8XX_GPIO5_14,
	DA8XX_GPIO5_15,
	DA8XX_GPIO5_0,
	DA8XX_GPIO5_1,
	DA8XX_GPIO5_2,
	DA8XX_GPIO5_3,
	DA8XX_GPIO5_4,
	DA8XX_SPI1_SOMI_0,
	DA8XX_SPI1_SIMO_0,
	DA8XX_SPI1_CLK,
	DA8XX_UART0_RXD,
	DA8XX_UART0_TXD,
	DA8XX_AXR1_10,
	DA8XX_AXR1_11,
	DA8XX_NSPI1_ENA,
	DA8XX_I2C1_SCL,
	DA8XX_I2C1_SDA,
	DA8XX_EQEP1S,
	DA8XX_I2C0_SDA,
	DA8XX_I2C0_SCL,
	DA8XX_UART2_RXD,
	DA8XX_TM64P0_IN12,
	DA8XX_TM64P0_OUT12,
	DA8XX_GPIO5_5,
	DA8XX_GPIO5_6,
	DA8XX_GPIO5_7,
	DA8XX_GPIO5_8,
	DA8XX_GPIO5_9,
	DA8XX_GPIO5_10,
	DA8XX_GPIO5_11,
	DA8XX_GPIO5_12,
	DA8XX_NSPI1_SCS_0,
	DA8XX_USB0_DRVVBUS,
	DA8XX_AHCLKX0,
	DA8XX_ACLKX0,
	DA8XX_AFSX0,
	DA8XX_AHCLKR0,
	DA8XX_ACLKR0,
	DA8XX_AFSR0,
	DA8XX_UART2_TXD,
	DA8XX_AHCLKX2,
	DA8XX_ECAP0_APWM0,
	DA8XX_RMII_MHZ_50_CLK,
	DA8XX_ECAP1_APWM1,
	DA8XX_USB_REFCLKIN,
	DA8XX_GPIO5_13,
	DA8XX_GPIO4_15,
	DA8XX_GPIO2_11,
	DA8XX_GPIO2_12,
	DA8XX_GPIO2_13,
	DA8XX_GPIO2_14,
	DA8XX_GPIO2_15,
	DA8XX_GPIO3_12,
	DA8XX_AMUTE0,
	DA8XX_AXR0_0,
	DA8XX_AXR0_1,
	DA8XX_AXR0_2,
	DA8XX_AXR0_3,
	DA8XX_AXR0_4,
	DA8XX_AXR0_5,
	DA8XX_AXR0_6,
	DA8XX_RMII_TXD_0,
	DA8XX_RMII_TXD_1,
	DA8XX_RMII_TXEN,
	DA8XX_RMII_CRS_DV,
	DA8XX_RMII_RXD_0,
	DA8XX_RMII_RXD_1,
	DA8XX_RMII_RXER,
	DA8XX_AFSR2,
	DA8XX_ACLKX2,
	DA8XX_AXR2_3,
	DA8XX_AXR2_2,
	DA8XX_AXR2_1,
	DA8XX_AFSX2,
	DA8XX_ACLKR2,
	DA8XX_NRESETOUT,
	DA8XX_GPIO3_0,
	DA8XX_GPIO3_1,
	DA8XX_GPIO3_2,
	DA8XX_GPIO3_3,
	DA8XX_GPIO3_4,
	DA8XX_GPIO3_5,
	DA8XX_GPIO3_6,
	DA8XX_AXR0_7,
	DA8XX_AXR0_8,
	DA8XX_UART1_RXD,
	DA8XX_UART1_TXD,
	DA8XX_AXR0_11,
	DA8XX_AHCLKX1,
	DA8XX_ACLKX1,
	DA8XX_AFSX1,
	DA8XX_MDIO_CLK,
	DA8XX_MDIO_D,
	DA8XX_AXR0_9,
	DA8XX_AXR0_10,
	DA8XX_EPWM0B,
	DA8XX_EPWM0A,
	DA8XX_EPWMSYNCI,
	DA8XX_AXR2_0,
	DA8XX_EPWMSYNC0,
	DA8XX_GPIO3_7,
	DA8XX_GPIO3_8,
	DA8XX_GPIO3_9,
	DA8XX_GPIO3_10,
	DA8XX_GPIO3_11,
	DA8XX_GPIO3_14,
	DA8XX_GPIO3_15,
	DA8XX_GPIO4_10,
	DA8XX_AHCLKR1,
	DA8XX_ACLKR1,
	DA8XX_AFSR1,
	DA8XX_AMUTE1,
	DA8XX_AXR1_0,
	DA8XX_AXR1_1,
	DA8XX_AXR1_2,
	DA8XX_AXR1_3,
	DA8XX_ECAP2_APWM2,
	DA8XX_EHRPWMGLUETZ,
	DA8XX_EQEP1A,
	DA8XX_GPIO4_11,
	DA8XX_GPIO4_12,
	DA8XX_GPIO4_13,
	DA8XX_GPIO4_14,
	DA8XX_GPIO4_0,
	DA8XX_GPIO4_1,
	DA8XX_GPIO4_2,
	DA8XX_GPIO4_3,
	DA8XX_AXR1_4,
	DA8XX_AXR1_5,
	DA8XX_AXR1_6,
	DA8XX_AXR1_7,
	DA8XX_AXR1_8,
	DA8XX_AXR1_9,
	DA8XX_EMA_D_0,
	DA8XX_EMA_D_1,
	DA8XX_EQEP1B,
	DA8XX_EPWM2B,
	DA8XX_EPWM2A,
	DA8XX_EPWM1B,
	DA8XX_EPWM1A,
	DA8XX_MMCSD_DAT_0,
	DA8XX_MMCSD_DAT_1,
	DA8XX_UHPI_HD_0,
	DA8XX_UHPI_HD_1,
	DA8XX_GPIO4_4,
	DA8XX_GPIO4_5,
	DA8XX_GPIO4_6,
	DA8XX_GPIO4_7,
	DA8XX_GPIO4_8,
	DA8XX_GPIO4_9,
	DA8XX_GPIO0_0,
	DA8XX_GPIO0_1,
	DA8XX_EMA_D_2,
	DA8XX_EMA_D_3,
	DA8XX_EMA_D_4,
	DA8XX_EMA_D_5,
	DA8XX_EMA_D_6,
	DA8XX_EMA_D_7,
	DA8XX_EMA_D_8,
	DA8XX_EMA_D_9,
	DA8XX_MMCSD_DAT_2,
	DA8XX_MMCSD_DAT_3,
	DA8XX_MMCSD_DAT_4,
	DA8XX_MMCSD_DAT_5,
	DA8XX_MMCSD_DAT_6,
	DA8XX_MMCSD_DAT_7,
	DA8XX_UHPI_HD_8,
	DA8XX_UHPI_HD_9,
	DA8XX_UHPI_HD_2,
	DA8XX_UHPI_HD_3,
	DA8XX_UHPI_HD_4,
	DA8XX_UHPI_HD_5,
	DA8XX_UHPI_HD_6,
	DA8XX_UHPI_HD_7,
	DA8XX_LCD_D_8,
	DA8XX_LCD_D_9,
	DA8XX_GPIO0_2,
	DA8XX_GPIO0_3,
	DA8XX_GPIO0_4,
	DA8XX_GPIO0_5,
	DA8XX_GPIO0_6,
	DA8XX_GPIO0_7,
	DA8XX_GPIO0_8,
	DA8XX_GPIO0_9,
	DA8XX_EMA_D_10,
	DA8XX_EMA_D_11,
	DA8XX_EMA_D_12,
	DA8XX_EMA_D_13,
	DA8XX_EMA_D_14,
	DA8XX_EMA_D_15,
	DA8XX_EMA_A_0,
	DA8XX_EMA_A_1,
	DA8XX_UHPI_HD_10,
	DA8XX_UHPI_HD_11,
	DA8XX_UHPI_HD_12,
	DA8XX_UHPI_HD_13,
	DA8XX_UHPI_HD_14,
	DA8XX_UHPI_HD_15,
	DA8XX_LCD_D_7,
	DA8XX_MMCSD_CLK,
	DA8XX_LCD_D_10,
	DA8XX_LCD_D_11,
	DA8XX_LCD_D_12,
	DA8XX_LCD_D_13,
	DA8XX_LCD_D_14,
	DA8XX_LCD_D_15,
	DA8XX_UHPI_HCNTL0,
	DA8XX_GPIO0_10,
	DA8XX_GPIO0_11,
	DA8XX_GPIO0_12,
	DA8XX_GPIO0_13,
	DA8XX_GPIO0_14,
	DA8XX_GPIO0_15,
	DA8XX_GPIO1_0,
	DA8XX_GPIO1_1,
	DA8XX_EMA_A_2,
	DA8XX_EMA_A_3,
	DA8XX_EMA_A_4,
	DA8XX_EMA_A_5,
	DA8XX_EMA_A_6,
	DA8XX_EMA_A_7,
	DA8XX_EMA_A_8,
	DA8XX_EMA_A_9,
	DA8XX_MMCSD_CMD,
	DA8XX_LCD_D_6,
	DA8XX_LCD_D_3,
	DA8XX_LCD_D_2,
	DA8XX_LCD_D_1,
	DA8XX_LCD_D_0,
	DA8XX_LCD_PCLK,
	DA8XX_LCD_HSYNC,
	DA8XX_UHPI_HCNTL1,
	DA8XX_GPIO1_2,
	DA8XX_GPIO1_3,
	DA8XX_GPIO1_4,
	DA8XX_GPIO1_5,
	DA8XX_GPIO1_6,
	DA8XX_GPIO1_7,
	DA8XX_GPIO1_8,
	DA8XX_GPIO1_9,
	DA8XX_EMA_A_10,
	DA8XX_EMA_A_11,
	DA8XX_EMA_A_12,
	DA8XX_EMA_BA_1,
	DA8XX_EMA_BA_0,
	DA8XX_EMA_CLK,
	DA8XX_EMA_SDCKE,
	DA8XX_NEMA_CAS,
	DA8XX_LCD_VSYNC,
	DA8XX_NLCD_AC_ENB_CS,
	DA8XX_LCD_MCLK,
	DA8XX_LCD_D_5,
	DA8XX_LCD_D_4,
	DA8XX_OBSCLK,
	DA8XX_NEMA_CS_4,
	DA8XX_UHPI_HHWIL,
	DA8XX_AHCLKR2,
	DA8XX_GPIO1_10,
	DA8XX_GPIO1_11,
	DA8XX_GPIO1_12,
	DA8XX_GPIO1_13,
	DA8XX_GPIO1_14,
	DA8XX_GPIO1_15,
	DA8XX_GPIO2_0,
	DA8XX_GPIO2_1,
	DA8XX_NEMA_RAS,
	DA8XX_NEMA_WE,
	DA8XX_NEMA_CS_0,
	DA8XX_NEMA_CS_2,
	DA8XX_NEMA_CS_3,
	DA8XX_NEMA_OE,
	DA8XX_NEMA_WE_DQM_1,
	DA8XX_NEMA_WE_DQM_0,
	DA8XX_NEMA_CS_5,
	DA8XX_UHPI_HRNW,
	DA8XX_NUHPI_HAS,
	DA8XX_NUHPI_HCS,
	DA8XX_NUHPI_HDS1,
	DA8XX_NUHPI_HDS2,
	DA8XX_NUHPI_HINT,
	DA8XX_AXR0_12,
	DA8XX_AMUTE2,
	DA8XX_AXR0_13,
	DA8XX_AXR0_14,
	DA8XX_AXR0_15,
	DA8XX_GPIO2_2,
	DA8XX_GPIO2_3,
	DA8XX_GPIO2_4,
	DA8XX_GPIO2_5,
	DA8XX_GPIO2_6,
	DA8XX_GPIO2_7,
	DA8XX_GPIO2_8,
	DA8XX_GPIO2_9,
	DA8XX_EMA_WAIT_0,
	DA8XX_NUHPI_HRDY,
	DA8XX_GPIO2_10,
};

enum davinci_dm365_index {
	/* MMC/SD 0 */
	DM365_MMCSD0,

	/* MMC/SD 1 */
	DM365_SD1_CLK,
	DM365_SD1_CMD,
	DM365_SD1_DATA3,
	DM365_SD1_DATA2,
	DM365_SD1_DATA1,
	DM365_SD1_DATA0,

	/* I2C */
	DM365_I2C_SDA,
	DM365_I2C_SCL,

	/* AEMIF */
	DM365_EM_AR,
	DM365_EM_A3,
	DM365_EM_A7,
	DM365_EM_D15_8,
	DM365_EM_CE0,

	/* ASP function */
	DM365_MCBSP_BDX,
	DM365_MCBSP_X,
	DM365_MCBSP_BFSX,
	DM365_MCBSP_BDR,
	DM365_MCBSP_R,
	DM365_MCBSP_BFSR,

	/* PWM0 */
	DM365_PWM0,
	DM365_PWM0_G23,

	/* PWM1 */
	DM365_PWM1,
	DM365_PWM1_G25,

	/* PWM2 */
	DM365_PWM2_G87,
	DM365_PWM2_G88,
	DM365_PWM2_G89,
	DM365_PWM2_G90,

	/* PWM3 */
	DM365_PWM3_G80,
	DM365_PWM3_G81,
	DM365_PWM3_G85,
	DM365_PWM3_G86,

	/* SPI0 */
	DM365_SPI0_SCLK,
	DM365_SPI0_SDI,
	DM365_SPI0_SDO,
	DM365_SPI0_SDENA0,
	DM365_SPI0_SDENA1,

	/* UART */
	DM365_UART0_RXD,
	DM365_UART0_TXD,
	DM365_UART1_RXD,
	DM365_UART1_TXD,
	DM365_UART1_RTS,
	DM365_UART1_CTS,

	/* EMAC */
	DM365_EMAC_TX_EN,
	DM365_EMAC_TX_CLK,
	DM365_EMAC_COL,
	DM365_EMAC_TXD3,
	DM365_EMAC_TXD2,
	DM365_EMAC_TXD1,
	DM365_EMAC_TXD0,
	DM365_EMAC_RXD3,
	DM365_EMAC_RXD2,
	DM365_EMAC_RXD1,
	DM365_EMAC_RXD0,
	DM365_EMAC_RX_CLK,
	DM365_EMAC_RX_DV,
	DM365_EMAC_RX_ER,
	DM365_EMAC_CRS,
	DM365_EMAC_MDIO,
	DM365_EMAC_MDCLK,

	/* SPI1 */
	DM365_SPI1_SCLK,
	DM365_SPI1_SDI,
	DM365_SPI1_SDO,
	DM365_SPI1_SDENA0,
	DM365_SPI1_SDENA1,

	/* SPI2 */
	DM365_SPI2_SCLK,
	DM365_SPI2_SDI,
	DM365_SPI2_SDO,
	DM365_SPI2_SDENA0,
	DM365_SPI2_SDENA1,

	/* SPI3 */
	DM365_SPI3_SCLK,
	DM365_SPI3_SDI,
	DM365_SPI3_SDO,
	DM365_SPI3_SDENA0,
	DM365_SPI3_SDENA1,

	/* SPI4 */
	DM365_SPI4_SCLK,
	DM365_SPI4_SDI,
	DM365_SPI4_SDO,
	DM365_SPI4_SDENA0,
	DM365_SPI4_SDENA1,

	/* GPIO */
	DM365_GPIO20,
	DM365_GPIO33,
	DM365_GPIO40,

	/* Video Out */
	DM365_VOUT_FIELD,
	DM365_VOUT_FIELD_G81,
	DM365_VOUT_HVSYNC,
	DM365_VOUT_COUTL_EN,
	DM365_VOUT_COUTH_EN,

	/* Video In */
	DM365_VIN_CAM_WEN,
	DM365_VIN_CAM_VD,
	DM365_VIN_CAM_HD,
	DM365_VIN_YINL_EN,
    DM365_VIN_YINH_EN,

	/* Keypad */
	DM365_KEYPAD,

    /* adv:cdc added more GPIO's see mux_cfg.c for details */
    DM365_GPIO79,  
    DM365_GPIO80,  
    DM365_GPIO81,  
    DM365_GPIO82,  
    DM365_GPIO84_83,
    DM365_GPIO85,  
    DM365_GPIO86,  
    DM365_GPIO87,  
    DM365_GPIO88,  
    DM365_GPIO89,  
    DM365_GPIO90,  
    DM365_GPIO91,  
    DM365_GPIO92,  
    DM365_GPIO92_85L,
    DM365_GPIO92_85H,

    /* adv:cdc added alternate functions for GPIO's see mux_cfg.c for details */
    DM365_VCLK,    
    DM365_EXTCLK,  
    DM365_FIELD,   
    DM365_LCD_OE_1,
    DM365_HVSYNC,  
    DM365_COUT0,   
    DM365_COUT1,   
    DM365_COUT2,   
    DM365_COUT3,   
    DM365_COUT4,   
    DM365_COUT5,   
    DM365_COUT6,   
    DM365_COUT7,   
    DM365_COUT7_0L, 
    DM365_COUT7_0H, 

    /* adv:cdc added setups for CLKOUT0 pinmux */
    DM365_C_WE_FIELD,
    DM365_GPIO93,
    DM365_CLKOUT0,
    DM365_USBDRVVBUS,

    /* adv:cdc added setups for GIO30 */
    DM365_GPIO30,
    DM365_SPI2_SIMO,
    DM365_G1,
};

#ifdef	CONFIG_DAVINCI_MUX
extern const short *da8xx_psc0_pins[];
extern const short *da8xx_psc1_pins[];

/* setup pin muxing in Linux */
extern void davinci_mux_init(void);
extern void da8xx_mux_init(void);
extern void davinci_mux_register(const struct pin_config *pins, unsigned size,
				 const short *(*get_pins)(unsigned, unsigned),
				 unsigned long *in_use);
extern int  davinci_cfg_reg(unsigned index, int action);
extern int  davinci_pinmux_setup(unsigned ctlr, unsigned id, int action);
#else
/* boot loader does it all (no warnings from CONFIG_DAVINCI_MUX_WARNINGS) */
static inline void davinci_mux_init(void) { }
static inline void da8xx_mux_init(void) { }
static inline int  davinci_cfg_reg(unsigned index, int action) { return 0; }
static inline int  davinci_pinmux_setup(unsigned ctlr, unsigned id, int action)
{
	return 0;
}
#endif

#endif
