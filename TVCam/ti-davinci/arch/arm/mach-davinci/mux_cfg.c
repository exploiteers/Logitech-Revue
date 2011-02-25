/*
 * DAVINCI pin multiplexing configurations
 *
 * Author: Vladimir Barinov, MontaVista Software, Inc.
 * Copyright (C) 2007-2008 MontaVista Software, Inc. <source@mvista.com>
 *
 * Based on linux/arch/arm/mach-omap1/mux.c:
 * Copyright (C) 2003 - 2005 Nokia Corporation
 * Written by Tony Lindgren <tony.lindgren@nokia.com>
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */

#include <linux/module.h>
#include <linux/init.h>

#include <asm/hardware.h>

#include <asm/arch/clock.h>
#include <asm/arch/cpu.h>
#include <asm/arch/mux.h>

/*
 * DM644x and DM6467 have 2 PinMux registers, DM355 has 5 PinMux registers.
 * Allocate the maximum.
 */
static unsigned long pinmux_in_use[5];

const struct pin_config __initdata_or_module davinci_dm644x_pinmux[] = {
/*
 *	 description		mux  mode   mode  mux	 dbg
 *				reg  offset mask  mode
 */
MUX_CFG("HDIREN",		 0,   16,    1,	  1,	 1)
MUX_CFG("ATAEN",		 0,   17,    1,	  1,	 1)

MUX_CFG("MSTK",			 1,   9,     1,	  0,	 0)

MUX_CFG("I2C",			 1,   7,     1,	  1,	 0)

MUX_CFG("MCBSP",		 1,   10,    1,	  1,	 0)

MUX_CFG("PWM0",			 1,   4,     1,	  1,	 0)

MUX_CFG("PWM1",			 1,   5,     1,	  1,	 0)

MUX_CFG("PWM2",			 1,   6,     1,	  1,	 0)

MUX_CFG("VLINQEN",		 0,   15,    1,	  1,	 0)
MUX_CFG("VLINQWD",		 0,   12,    3,	  3,	 0)

MUX_CFG("EMACEN",		 0,   31,    1,	  1,	 1)

MUX_CFG("GPIO3V",		 0,   31,    1,	  0,	 1)

MUX_CFG("GPIO0",		 0,   24,    1,	  0,	 1)
MUX_CFG("GPIO3",		 0,   25,    1,	  0,	 0)
MUX_CFG("GPIO43_44",		 1,   7,     1,	  0,	 0)
MUX_CFG("GPIO46_47",		 0,   22,    1,	  0,	 1)

MUX_CFG("RGB666",		 0,   22,    1,	  1,	 1)

MUX_CFG("LOEEN",		 0,   24,    1,	  1,	 1)
MUX_CFG("LFLDEN",		 0,   25,    1,	  1,	 0)
};

const struct pin_config __initdata_or_module davinci_dm357_pinmux[] = {
/*
 *      description            mux  mode   mode  mux    dbg
 *                             reg  offset mask  mode
 */
MUX_CFG("I2C",                  1,   7,     1,   1,     0)

MUX_CFG("MCBSP",                1,   10,    1,   1,     0)

MUX_CFG("PWM0",                         1,   4,     1,   1,     0)

MUX_CFG("PWM1",                         1,   5,     1,   1,     0)

MUX_CFG("PWM2",                         1,   6,     1,   1,     0)

MUX_CFG("EMACEN",               0,   31,    1,   1,     1)

MUX_CFG("GPIO3V",               0,   31,    1,   0,     1)

MUX_CFG("GPIO0",                0,   24,    1,   0,     1)
MUX_CFG("GPIO3",                0,   25,    1,   0,     0)
MUX_CFG("GPIO43_44",            1,   7,     1,   0,     0)
MUX_CFG("GPIO46_47",            0,   22,    1,   0,     1)

MUX_CFG("RGB666",               0,   22,    1,   1,     1)

MUX_CFG("LOEEN",                0,   24,    1,   1,     1)
MUX_CFG("LFLDEN",               0,   25,    1,   1,     0)

MUX_CFG("AECS4",                0,   10,    1,   1,     0)
MUX_CFG("AECS5",                0,   11,    1,   1,     0)
};

const struct pin_config __initdata_or_module davinci_dm646x_pinmux[] = {
/*
 *	 description		mux  mode   mode  mux	 dbg
 *				reg  offset mask  mode
 */
MUX_CFG("ATAEN",		 0,   0,     1,	  1,	 1)

MUX_CFG("VBUSDIS",		 0,   31,    1,	  0,	 0)

MUX_CFG("VBUSDIS_GPIO22",	 0,   31,    1,	  1,	 0)

MUX_CFG("STCCK",		 0,   30,    1,	  1,	 0)

MUX_CFG("AUDCK1",		 0,   29,    1,	  0,	 0)

MUX_CFG("AUDCK0",		 0,   28,    1,	  0,	 0)

MUX_CFG("CRGMUX",		 0,   24,    7,	  5,	 0)

MUX_CFG("STSOMUX_DISABLE",	 0,   22,    3,	  0,	 0)

MUX_CFG("STSIMUX_DISABLE",	 0,   20,    3,	  0,	 0)

MUX_CFG("PTSOMUX_DISABLE",	 0,   18,    3,	  0,	 0)

MUX_CFG("PTSIMUX_DISABLE",	 0,   16,    3,	  0,	 0)

MUX_CFG("STSOMUX",		 0,   22,    3,	  2,	 0)

MUX_CFG("STSIMUX",		 0,   20,    3,	  2,	 0)

MUX_CFG("PTSOMUX_PARALLEL",	 0,   18,    3,	  2,	 0)

MUX_CFG("PTSIMUX_PARALLEL",	 0,   16,    3,	  2,	 0)

MUX_CFG("PTSOMUX_SERIAL",	 0,   18,    3,	  3,	 0)

MUX_CFG("PTSIMUX_SERIAL",	 0,   16,    3,	  3,	 0)
};

const struct pin_config __initdata_or_module davinci_dm355_pinmux[] = {
/*
 *	 description		mux  mode   mode  mux	 dbg
 *				reg  offset mask  mode
 */
MUX_CFG("MMCSD0",		 4,   2,     1,	  0,	 0)

MUX_CFG("SD1_CLK",		 3,   6,     1,	  1,	 0)
MUX_CFG("SD1_CMD",		 3,   7,     1,	  1,	 0)
MUX_CFG("SD1_DATA3",		 3,   8,     3,	  1,	 0)
MUX_CFG("SD1_DATA2",		 3,   10,    3,	  1,	 0)
MUX_CFG("SD1_DATA1",		 3,   12,    3,	  1,	 0)
MUX_CFG("SD1_DATA0",		 3,   14,    3,	  1,	 0)

MUX_CFG("I2C_SDA",		 3,   19,    1,	  1,	 0)
MUX_CFG("I2C_SCL",		 3,   20,    1,	  1,	 0)

MUX_CFG("MCBSP0_BDX",		 3,   0,     1,	  1,	 0)
MUX_CFG("MCBSP0_X",		 3,   1,     1,	  1,	 0)
MUX_CFG("MCBSP0_BFSX",		 3,   2,     1,	  1,	 0)
MUX_CFG("MCBSP0_BDR",		 3,   3,     1,	  1,	 0)
MUX_CFG("MCBSP0_R",		 3,   4,     1,	  1,	 0)
MUX_CFG("MCBSP0_BFSR",		 3,   5,     1,	  1,	 0)

MUX_CFG("PWM0",			 1,   0,     3,	  2,	 1)

MUX_CFG("PWM1",			 1,   2,     3,	  2,	 1)

MUX_CFG("PWM2_G76",		 1,   10,    3,	  2,	 1)
MUX_CFG("PWM2_G77",		 1,   8,     3,	  2,	 1)
MUX_CFG("PWM2_G78",		 1,   6,     3,	  2,	 1)
MUX_CFG("PWM2_G79",		 1,   4,     3,	  2,	 1)

MUX_CFG("PWM3_G69",		 1,   20,    3,	  3,	 0)
MUX_CFG("PWM3_G70",		 1,   18,    3,	  3,	 0)
MUX_CFG("PWM3_G74",		 1,   14,    3,	  2,	 1)
MUX_CFG("PWM3_G75",		 1,   12,    3,	  2,	 1)

MUX_CFG("SPI0_SDI",		 4,   1,     1,	  0,	 0)
MUX_CFG("SPI0_SDENA0",		 4,   0,     1,	  0,	 0)
MUX_CFG("SPI0_SDENA1",		 3,   28,    1,	  1,	 0)

MUX_CFG("SPI1_SCLK",		 3,   24,    1,	  1,	 0)
MUX_CFG("SPI1_SDO",		 3,   27,    1,	  1,	 0)
MUX_CFG("SPI1_SDENA0",		 3,   23,    1,	  1,	 0)
MUX_CFG("SPI1_SDENA1",		 3,   25,    3,	  2,	 0)

MUX_CFG("SPI2_SCLK",		 0,   0,     3,	  2,	 0)
MUX_CFG("SPI2_SDO",		 0,   2,     3,	  2,	 0)
MUX_CFG("SPI2_SDENA0",		 0,   4,     3,	  2,	 0)
MUX_CFG("SPI2_SDENA1",		 0,   6,     3,	  3,	 0)

MUX_CFG("GPIO14",		 3,   20,    1,	  0,	 0)
MUX_CFG("GPIO15",		 3,   19,    1,	  0,	 0)
MUX_CFG("GPIO71",		 1,   17,    1,	  1,	 0)

MUX_CFG("VOUT_FIELD",		 1,   18,    3,	  1,	 1)
MUX_CFG("VOUT_FIELD_G70",	 1,   18,    3,	  0,	 1)
MUX_CFG("VOUT_HVSYNC",		 1,   16,    1,	  0,	 0)
MUX_CFG("VOUT_COUTL_EN",	 1,   0,     0xff, 0x55,  1)
MUX_CFG("VOUT_COUTH_EN",	 1,   8,     0xff, 0x55,  1)

MUX_CFG("VIN_PCLK",		 0,   14,    1,	  1,	 0)
MUX_CFG("VIN_CAM_WEN",		 0,   13,    1,	  1,	 0)
MUX_CFG("VIN_CAM_VD",		 0,   12,    1,	  1,	 0)
MUX_CFG("VIN_CAM_HD",		 0,   11,    1,	  1,	 0)
MUX_CFG("VIN_YIN_EN",		 0,   10,    1,	  1,	 0)
MUX_CFG("VIN_CINL_EN",		 0,   0,     0xff, 0x55,  0)
MUX_CFG("VIN_CINH_EN",		 0,   8,     3,	  3,	 0)
};

const struct pin_config __initdata_or_module davinci_dm365_pinmux[] = {
/*
 *	 description		mux  mode   mode  mux	 dbg
 *				reg  offset mask  mode
 */
MUX_CFG("MMCSD0",		 0,   24,    1,   0,	 0)

MUX_CFG("SD1_CLK",		 0,   16,    3,   1,	 0)
MUX_CFG("SD1_CMD",		 4,   30,    3,   1,	 0)
MUX_CFG("SD1_DATA3",		 4,   28,    3,   1,	 0)
MUX_CFG("SD1_DATA2",		 4,   26,    3,   1,	 0)
MUX_CFG("SD1_DATA1",		 4,   24,    3,   1,	 0)
MUX_CFG("SD1_DATA0",		 4,   22,    3,   1,	 0)

MUX_CFG("I2C_SDA",		 3,   23,    3,   2,	 0)
MUX_CFG("I2C_SCL",		 3,   21,    3,   2,	 0)

MUX_CFG("EM_AR",		 2,   0,     3,	  1,	0)
MUX_CFG("EM_A3",		 2,   2,     3,	  1,	0)
MUX_CFG("EM_A7",		 2,   4,     3,	  1,	0)
MUX_CFG("EM_D15_8",		 2,   6,     1,	  1,	0)
MUX_CFG("EM_CE0",		 2,   7,     1,	  0,	0)

MUX_CFG("MCBSP_BDX",		 0,   23,    1,   1,	 0)
MUX_CFG("MCBSP_X",		 0,   22,    1,   1,	 0)
MUX_CFG("MCBSP_BFSX",		 0,   21,    1,   1,	 0)
MUX_CFG("MCBSP_BDR",		 0,   20,    1,   1,	 0)
MUX_CFG("MCBSP_R",		 0,   19,    1,   1,	 0)
MUX_CFG("MCBSP_BFSR",		 0,   18,    1,   1,	 0)

MUX_CFG("PWM0",			 1,   0,     3,   2,	 1)
MUX_CFG("PWM0_G23",		 3,   26,    3,   3,	 1)

MUX_CFG("PWM1",			 1,   2,     3,   2,	 1)
MUX_CFG("PWM1_G25",		 3,   29,    3,   2,	 1)

MUX_CFG("PWM2_G87",		 1,   10,    3,	  2,	 1)
MUX_CFG("PWM2_G88",		 1,   8,     3,	  2,	 1)
MUX_CFG("PWM2_G89",		 1,   6,     3,	  2,	 1)
MUX_CFG("PWM2_G90",		 1,   4,     3,	  2,	 1)

MUX_CFG("PWM3_G80",		 1,   20,    3,	  3,	 0)
MUX_CFG("PWM3_G81",		 1,   18,    3,	  3,	 0)
MUX_CFG("PWM3_G85",		 1,   14,    3,	  2,	 1)
MUX_CFG("PWM3_G86",		 1,   12,    3,	  2,	 1)

MUX_CFG("SPI0_SCLK",		 3,   28,    1,	  1,	 0)
MUX_CFG("SPI0_SDI",		 3,   26,    3,   1,	 0)
MUX_CFG("SPI0_SDO)",		 3,   25,    1,   1,	 0)
MUX_CFG("SPI0_SDENA0",		 3,   29,    3,   1,	 0)
MUX_CFG("SPI0_SDENA1",		 3,   26,    3,	  2,	 0)

MUX_CFG("UART0_RXD",		 3,   20,    1,   1,	 0)
MUX_CFG("UART0_TXD",		 3,   19,    1,   1,	 0)
MUX_CFG("UART1_RXD",		 3,   17,    3,   2,	 0)
MUX_CFG("UART1_TXD",		 3,   15,    3,   2,	 0)
MUX_CFG("UART1_RTS",		 3,   23,    3,   1,	 0)
MUX_CFG("UART1_CTS",		 3,   21,    3,   1,	 0)

MUX_CFG("EMAC_TX_EN",		 3,   17,    3,	  1,	 0)
MUX_CFG("EMAC_TX_CLK",		 3,   15,    3,   1,	 0)
MUX_CFG("EMAC_COL",		 3,   14,    1,   1,	 0) 
MUX_CFG("EMAC_TXD3",		 3,   13,    1,   1,	 0)
MUX_CFG("EMAC_TXD2",		 3,   12,    1,   1,	 0)
MUX_CFG("EMAC_TXD1",		 3,   11,    1,   1,	 0)
MUX_CFG("EMAC_TXD0",		 3,   10,    1,   1,	 0)
MUX_CFG("EMAC_RXD3",		 3,   9,     1,   1,	 0)
MUX_CFG("EMAC_RXD2",		 3,   8,     1,   1,	 0)
MUX_CFG("EMAC_RXD1",		 3,   7,     1,   1,	 0)
MUX_CFG("EMAC_RXD0",		 3,   6,     1,   1,	 0)
MUX_CFG("EMAC_RX_CLK",		 3,   5,     1,   1,	 0)
MUX_CFG("EMAC_RX_DV",		 3,   4,     1,   1,	 0)
MUX_CFG("EMAC_RX_ER",		 3,   3,     1,   1,	 0)
MUX_CFG("EMAC_CRS",		 3,   2,     1,   1,	 0)
MUX_CFG("EMAC_MDIO",		 3,   1,     1,   1,	 0)
MUX_CFG("EMAC_MDCLK",		 3,   0,     1,   1,	 0)

MUX_CFG("SPI1_SCLK",		 4,   2,     3,	  1,	 0)
MUX_CFG("SPI1_SDO",		 3,   31,    1,	  1,	 0)
MUX_CFG("SPI1_SDI",		 4,   0,     3,	  1,	 0)
MUX_CFG("SPI1_SDENA0",		 4,   4,     3,	  1,	 0)
MUX_CFG("SPI1_SDENA1",		 4,   0,     3,	  2,	 0)

MUX_CFG("SPI2_SCLK",		 4,   10,    3,	  1,	 0)
MUX_CFG("SPI2_SDO",		 4,   6,     3,	  1,	 0)
MUX_CFG("SPI2_SDI",		 4,   8,     3,	  1,	 0)
MUX_CFG("SPI2_SDENA0",		 4,   12,    3,	  1,	 0)
MUX_CFG("SPI2_SDENA1",		 4,   8,     3,	  2,	 0)

MUX_CFG("SPI3_SCLK",		 0,   0,     3,   2,     0)
MUX_CFG("SPI3_SDO",		 0,   2,     3,   2,     0)
MUX_CFG("SPI3_SDI",		 0,   6,     3,   2,     0)
MUX_CFG("SPI3_SDENA0",		 0,   4,     3,   2,     0)
MUX_CFG("SPI3_SDENA1",		 0,   6,     3,   3,     0)

MUX_CFG("SPI4_SCLK",		 4,   18,    3,   1,     0)
MUX_CFG("SPI4_SDO",		 4,   14,    3,   1,     0)
MUX_CFG("SPI4_SDI",		 4,   16,    3,   1,     0)
MUX_CFG("SPI4_SDENA0",		 4,   20,    3,   1,     0)
MUX_CFG("SPI4_SDENA1",		 4,   16,    3,   2,     0)

MUX_CFG("GPIO20",		 3,   21,    3,   0,	 0)
MUX_CFG("GPIO33",		 4,   12,    3,	  0,	 0)
MUX_CFG("GPIO40",		 4,   26,    3,	  0,	 0)

MUX_CFG("VOUT_FIELD",		 1,   18,    3,	  1,	 1)
MUX_CFG("VOUT_FIELD_G81",	 1,   18,    3,	  0,	 1)
MUX_CFG("VOUT_HVSYNC",		 1,   16,    1,	  0,	 0)
MUX_CFG("VOUT_COUTL_EN",	 1,   0,     0xff, 0x55,  1)
MUX_CFG("VOUT_COUTH_EN",	 1,   8,     0xff, 0x55,  1)

MUX_CFG("VIN_CAM_WEN",		 0,   14,    3,	  0,	 0)
MUX_CFG("VIN_CAM_VD",		 0,   13,    1,	  0,	 0)
MUX_CFG("VIN_CAM_HD",		 0,   12,    1,	  0,	 0)
MUX_CFG("VIN_YINL_EN",		 0,   0,     0xff,  0,	 0)
MUX_CFG("VIN_YINH_EN",		 0,   8,     0xf,  0,	 0)

MUX_CFG("KEYPAD",		 2,   0,     0x3f, 0x3f, 0)

/* adv:cdc added more GPIO's on overloaded COUT pins */
MUX_CFG("GPIO79",		 1,   22,    1,	  1,	 0) // GPIO79 VCLK              
MUX_CFG("GPIO80",		 1,   20,    3,	  0,	 0) // GPIO80 EXTCLK B2         
MUX_CFG("GPIO81",		 1,   28,    3,	  0,	 0) // GPIO81 FIELD R2          
MUX_CFG("GPIO82",		 1,   17,    1,	  1,	 0) // GPIO82 LCD_OE            
MUX_CFG("GPIO84_83",	 1,   16,    1,	  1,	 0) // GPIO84_83 HSYNC & VSYNC  
MUX_CFG("GPIO85",		 1,   14,    3,	  0,	 0) // GPIO85 COUT0 PWM3        
MUX_CFG("GPIO86",		 1,   12,    3,	  0,	 0) // GPIO86 COUT1 PWM3        
MUX_CFG("GPIO87",		 1,   10,    3,	  0,	 0) // GPIO87 COUT2 PWM2        
MUX_CFG("GPIO88",		 1,    8,    3,	  0,	 0) // GPIO88 COUT3 PWM2        
MUX_CFG("GPIO89",		 1,    6,    3,	  0,	 0) // GPIO89 COUT4 PWM2        
MUX_CFG("GPIO90",		 1,    4,    3,	  0,	 0) // GPIO90 COUT5 PWM2        
MUX_CFG("GPIO91",		 1,    2,    3,	  0,	 0) // GPIO91 COUT6 PWM1 (Optionally LCD_OE instead of COUT6)
MUX_CFG("GPIO92",		 1,    0,    3,	  0,	 0) // GPIO92 COUT7 PWM0        
MUX_CFG("GPIO92_85L",	 1,    0,    0xFF,	  0,	 0) // GPIO92-85 COUT7-0 PWM0,2,3        
MUX_CFG("GPIO92_85H",	 1,    8,    0xFF,	  0,	 0) // GPIO92-85 COUT7-0 PWM0,2,3        

/* adv:cdc added alternate functions for GPIO's on overloaded COUT pins */
MUX_CFG("VCLK",		     1,   22,    1,	  0,	 0) // GPIO79 VCLK
MUX_CFG("EXTCLK",		 1,   20,    3,	  1,	 0) // GPIO80 EXTCLK B2
MUX_CFG("FIELD",		 1,   28,    3,	  1,	 0) // GPIO81 FIELD R2
MUX_CFG("LCD_OE_1",		 1,   17,    1,	  0,	 0) // GPIO82 LCD_OE
MUX_CFG("HVSYNC",	     1,   16,    1,	  0,	 0) // GPIO84_83 HSYNC & VSYNC
MUX_CFG("COUT0",		 1,   14,    3,	  1,	 0) // GPIO85 COUT0 PWM3
MUX_CFG("COUT1",		 1,   12,    3,	  1,	 0) // GPIO86 COUT1 PWM3
MUX_CFG("COUT2",		 1,   10,    3,	  1,	 0) // GPIO87 COUT2 PWM2
MUX_CFG("COUT3",		 1,    8,    3,	  1,	 0) // GPIO88 COUT3 PWM2
MUX_CFG("COUT4",		 1,    6,    3,	  1,	 0) // GPIO89 COUT4 PWM2 
MUX_CFG("COUT5",		 1,    4,    3,	  1,	 0) // GPIO90 COUT5 PWM2 
MUX_CFG("COUT6",		 1,    2,    3,	  1,	 0) // GPIO91 COUT6 PWM1 (Optionally LCD_OE instead of COUT6)
MUX_CFG("COUT7",		 1,    0,    3,	  1,	 0) // GPIO92 COUT7 PWM0 
MUX_CFG("COUT7_0L",	     1,    0,    0xFF,	  0x55,	 0) // GPIO92-85 COUT7-0 PWM0,2,3        
MUX_CFG("COUT7_0H",	     1,    8,    0xFF,	  0x55,	 0) // GPIO92-85 COUT7-0 PWM0,2,3        

/* adv:cdc added setups for CLKOUT0 pinmux */
MUX_CFG("C_WE_FIELD",    0,   14,    3,   0,     0) // C_WE_FIELD GPIO93 CLKOUT0 USBDRVVBUS
MUX_CFG("GPIO93",        0,   14,    3,   1,     0) // C_WE_FIELD GPIO93 CLKOUT0 USBDRVVBUS
MUX_CFG("CLKOUT0",       0,   14,    3,   2,     0) // C_WE_FIELD GPIO93 CLKOUT0 USBDRVVBUS
MUX_CFG("USBDRVVBUS",    0,   14,    3,   3,     0) // C_WE_FIELD GPIO93 CLKOUT0 USBDRVVBUS

/* adv:cdc added setups for GIO30 */
MUX_CFG("GPIO30",        4,    6,    3,   0,     0) // GIO30 as GIO30
MUX_CFG("SPI2_SIMO",     4,    6,    3,   1,     0) // GIO30 as GIO30
MUX_CFG("G1",            4,    6,    3,   3,     0) // GIO30 as GIO30

};

static const short dm644x_ata_pins[] = {
	DM644X_HDIREN, DM644X_ATAEN,
	-1
};

static const short dm644x_mmcsd_pins[] = {
	DM644X_MSTK,
	-1
};

static const short dm644x_i2c_pins[] = {
	DM644X_I2C,
	-1
};

static const short dm644x_mcbsp_pins[] = {
	DM644X_MCBSP,
	-1
};

static const short dm644x_pwm0_pins[] = {
	DM644X_PWM0,
	-1
};

static const short dm644x_pwm1_pins[] = {
	DM644X_PWM1,
	-1
};

static const short dm644x_pwm2_pins[] = {
	DM644X_PWM2,
	-1
};

static const short dm644x_vlynq_pins[] = {
	DM644X_VLINQEN, DM644X_VLINQWD,
	-1
};

static const short *dm644x_pins[DAVINCI_LPSC_IMCOP + 1] = {
	[DAVINCI_LPSC_ATA]	= dm644x_ata_pins,
	[DAVINCI_LPSC_MMC_SD]	= dm644x_mmcsd_pins,
	[DAVINCI_LPSC_I2C]	= dm644x_i2c_pins,
	[DAVINCI_LPSC_McBSP]	= dm644x_mcbsp_pins,
	[DAVINCI_LPSC_PWM0]	= dm644x_pwm0_pins,
	[DAVINCI_LPSC_PWM1]	= dm644x_pwm1_pins,
	[DAVINCI_LPSC_PWM2]	= dm644x_pwm2_pins,
	[DAVINCI_LPSC_VLYNQ]	= dm644x_vlynq_pins,
};

static const short dm357_i2c_pins[] = {
       DM357_I2C,
       -1
};

static const short dm357_mcbsp_pins[] = {
       DM357_MCBSP,
       -1
};

static const short dm357_pwm0_pins[] = {
       DM357_PWM0,
       -1
};

static const short dm357_pwm1_pins[] = {
       DM357_PWM1,
       -1
};

static const short dm357_pwm2_pins[] = {
       DM357_PWM2,
       -1
};

static const short *dm357_pins[DAVINCI_LPSC_IMCOP + 1] = {
       [DAVINCI_LPSC_I2C]      = dm357_i2c_pins,
       [DAVINCI_LPSC_McBSP]    = dm357_mcbsp_pins,
       [DAVINCI_LPSC_PWM0]     = dm357_pwm0_pins,
       [DAVINCI_LPSC_PWM1]     = dm357_pwm1_pins,
       [DAVINCI_LPSC_PWM2]     = dm357_pwm2_pins,
};

static const short dm355_pwm3_pins[] = {
	DM355_PWM3_G69, DM355_PWM3_G70, DM355_PWM3_G74, DM355_PWM3_G75,
	-1

};
static const short dm355_mmcsd0_pins[] = {
	DM355_MMCSD0,
	-1
};

static const short dm355_mmcsd1_pins[] = {
	DM355_SD1_CLK, DM355_SD1_CMD, DM355_SD1_DATA3, DM355_SD1_DATA2,
	DM355_SD1_DATA1, DM355_SD1_DATA0,
	-1
};

static const short dm355_i2c_pins[] = {
	DM355_I2C_SDA, DM355_I2C_SCL,
	-1
};

static const short dm355_mcbsp0_pins[] = {
	DM355_MCBSP0_BDX, DM355_MCBSP0_X, DM355_MCBSP0_BFSX, DM355_MCBSP0_BDR,
	DM355_MCBSP0_R, DM355_MCBSP0_BFSR,
	-1
};

static const short dm355_pwm0_pins[] = {
	DM355_PWM0,
	-1
};

static const short dm355_pwm1_pins[] = {
	DM355_PWM1,
	-1
};

static const short dm355_pwm2_pins[] = {
	DM355_PWM2_G76, DM355_PWM2_G77, DM355_PWM2_G78, DM355_PWM2_G79,
	-1
};

static const short *dm355_pins[DAVINCI_LPSC_IMCOP + 1] = {
	[DAVINCI_LPSC_PWM3]	= dm355_pwm3_pins,
	[DAVINCI_LPSC_MMC_SD0]	= dm355_mmcsd0_pins,
	[DAVINCI_LPSC_MMC_SD1]	= dm355_mmcsd1_pins,
	[DAVINCI_LPSC_I2C]	= dm355_i2c_pins,
	[DAVINCI_LPSC_McBSP0]	= dm355_mcbsp0_pins,
	[DAVINCI_LPSC_PWM0]	= dm355_pwm0_pins,
	[DAVINCI_LPSC_PWM1]	= dm355_pwm1_pins,
	[DAVINCI_LPSC_PWM2]	= dm355_pwm2_pins,
};

static const short dm365_pwm3_pins[] = {
	DM365_PWM3_G80, DM365_PWM3_G81, DM365_PWM3_G85, DM365_PWM3_G86,
	-1
};
static const short dm365_mmcsd0_pins[] = {
	DM365_MMCSD0,
	-1
};

static const short dm365_mmcsd1_pins[] = {
	DM365_SD1_CLK, DM365_SD1_CMD, DM365_SD1_DATA3, DM365_SD1_DATA2,
	DM365_SD1_DATA1, DM365_SD1_DATA0,
	-1
};

static const short dm365_i2c_pins[] = {
	DM365_I2C_SDA, DM365_I2C_SCL,
	-1
};

static const short dm365_aemif_pins[] = {
	DM365_EM_AR, DM365_EM_A3, DM365_EM_A7, DM365_EM_D15_8, DM365_EM_CE0,
	-1
};

static const short dm365_mcbsp0_pins[] = {
	DM365_MCBSP_BDX, DM365_MCBSP_X, DM365_MCBSP_BFSX, DM365_MCBSP_BDR,
	DM365_MCBSP_R, DM365_MCBSP_BFSR,
	-1
};

static const short dm365_pwm0_pins[] = {
	DM365_PWM0,
	-1
};

static const short dm365_pwm1_pins[] = {
	DM365_PWM1,
	-1
};

static const short dm365_pwm2_pins[] = {
	DM365_PWM2_G87, DM365_PWM2_G88, DM365_PWM2_G89, DM365_PWM2_G90,
	-1
};

static const short dm365_uart0_pins[] = {
	DM365_UART0_RXD, DM365_UART0_TXD,
	-1
};

static const short dm365_cpmac_pins[] = {
	DM365_EMAC_TX_EN, DM365_EMAC_TX_CLK, DM365_EMAC_COL, DM365_EMAC_TXD3,
	DM365_EMAC_TXD2, DM365_EMAC_TXD1, DM365_EMAC_TXD0, DM365_EMAC_RXD3,
	DM365_EMAC_RXD2, DM365_EMAC_RXD1, DM365_EMAC_RXD0, DM365_EMAC_RX_CLK,
	DM365_EMAC_RX_DV, DM365_EMAC_RX_ER, DM365_EMAC_CRS, DM365_EMAC_MDIO,
	DM365_EMAC_MDCLK,
	-1
};

static const short dm365_spi0_pins[] = {
	DM365_SPI0_SCLK, DM365_SPI0_SDO, DM365_SPI0_SDI, DM365_SPI0_SDENA0,
	-1
};

static const short *dm365_pins[DAVINCI_LPSC_IMCOP + 1] = {
	[DAVINCI_LPSC_PWM3]		= dm365_pwm3_pins,
	[DAVINCI_LPSC_MMC_SD0]		= dm365_mmcsd0_pins,
	[DAVINCI_LPSC_MMC_SD1]		= dm365_mmcsd1_pins,
	[DAVINCI_LPSC_I2C]		= dm365_i2c_pins,
	[DAVINCI_LPSC_AEMIF]		= dm365_aemif_pins,
	[DAVINCI_DM365_LPSC_McBSP]	= dm365_mcbsp0_pins,
	[DAVINCI_LPSC_PWM0]		= dm365_pwm0_pins,
	[DAVINCI_LPSC_PWM1]		= dm365_pwm1_pins,
	[DAVINCI_LPSC_PWM2]		= dm365_pwm2_pins,
	[DAVINCI_LPSC_UART0]		= dm365_uart0_pins,
	[DAVINCI_DM365_LPSC_CPGMAC]	= dm365_cpmac_pins,
	[DAVINCI_LPSC_SPI]		= dm365_spi0_pins,
};

static const short dm466x_ata_pins[] = {
	DM646X_ATAEN,
	-1
};

static const short dm466x_usb_pins[] = {
	DM646X_VBUSDIS,
	-1
};

static const short dm466x_video_pins[] = {
	DM646X_STSOMUX_DISABLE, DM646X_STSIMUX_DISABLE,
	DM646X_PTSOMUX_DISABLE, DM646X_PTSIMUX_DISABLE,
	-1
};

static const short *dm646x_pins[DAVINCI_DM646X_LPSC_ARM_INTC + 1] = {
	[DAVINCI_LPSC_ATA]		= dm466x_ata_pins,
	[DAVINCI_LPSC_USB]		= dm466x_usb_pins,
	[DAVINCI_DM646X_LPSC_HDVICP0]	= dm466x_video_pins,
	[DAVINCI_DM646X_LPSC_HDVICP1]	= dm466x_video_pins,
};

static const short **davinci_pins;
static int davinci_num_pins;

static const short *davinci_get_pins(unsigned ctlr, unsigned id)
{
	if (ctlr == 0 && id <= davinci_num_pins)
		return davinci_pins[id];

	return NULL;
}

void __init davinci_mux_init(void)
{
	const struct pin_config *table;
	unsigned size;

	if (cpu_is_davinci_dm365()) {
		davinci_pins = dm365_pins;
		davinci_num_pins = ARRAY_SIZE(dm365_pins);
		table = davinci_dm365_pinmux;
		size  = ARRAY_SIZE(davinci_dm365_pinmux);
	} else if (cpu_is_davinci_dm355()) {
		davinci_pins = dm355_pins;
		davinci_num_pins = ARRAY_SIZE(dm355_pins);
		table = davinci_dm355_pinmux;
		size  = ARRAY_SIZE(davinci_dm355_pinmux);
	} else if (cpu_is_davinci_dm6467()) {
		davinci_pins = dm646x_pins;
		davinci_num_pins = ARRAY_SIZE(dm646x_pins);
		table = davinci_dm646x_pinmux;
		size  = ARRAY_SIZE(davinci_dm646x_pinmux);
	} else if (cpu_is_davinci_dm644x()) {
		davinci_pins = dm644x_pins;
		davinci_num_pins = ARRAY_SIZE(dm644x_pins);
		table = davinci_dm644x_pinmux;
		size  = ARRAY_SIZE(davinci_dm644x_pinmux);
	} else {
		if (!cpu_is_davinci_dm357())
			BUG();
		davinci_pins = dm357_pins;
		davinci_num_pins = ARRAY_SIZE(dm357_pins);
		table = davinci_dm357_pinmux;
		size  = ARRAY_SIZE(davinci_dm357_pinmux);
	}
	davinci_mux_register(table, size, davinci_get_pins, pinmux_in_use);
}
