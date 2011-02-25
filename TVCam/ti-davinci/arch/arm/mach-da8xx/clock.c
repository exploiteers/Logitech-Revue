/*
 * TI DA8xx clock config file
 *
 * Copyright (C) 2008 MontaVista Software, Inc. <source@mvista.com>
 *
 * Based on arch/arm/mach-davinci/clock.c
 * Copyright (C) 2006 Texas Instruments.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/kernel.h>

#include <asm/hardware.h>

#include <asm/arch/clock.h>

static unsigned long da8xx_psc_bases[] = { DA8XX_PSC0_BASE, DA8XX_PSC1_BASE };

static unsigned int commonrate;
static unsigned int div_by_four;
static unsigned int armrate;
static unsigned int fixedrate = DA8XX_CLOCK_TICK_RATE; /* AUXCLK */

static struct clk da8xx_clks[] = {
	{
		.name = "ARMCLK",
		.rate = &armrate,
		.lpsc = DA8XX_LPSC0_ARM,
		.flags = ALWAYS_ENABLED,
		.ctlr = 0,
	},
	{
		.name = "UART0",
		.rate = &commonrate,
		.lpsc = DA8XX_LPSC0_UART0,
		.ctlr = 0,
	},
	{
		.name = "SPI0CLK",
		.rate = &commonrate,
		.lpsc = DA8XX_LPSC0_SPI0,
		.ctlr = 0,
	},
	{
		.name = "MMC_SD_CLK",
		.rate = &commonrate,
		.lpsc = DA8XX_LPSC0_MMC_SD,
		.ctlr = 0,
	},
	{
		.name = "UART1",
		.rate = &commonrate,
		.lpsc = DA8XX_LPSC1_UART1,
		.ctlr = 1,
	},
	{
		.name = "UART2",
		.rate = &commonrate,
		.lpsc = DA8XX_LPSC1_UART2,
		.ctlr = 1,
	},
	{
		.name = "EMACCLK",
		.rate = &div_by_four,
		.lpsc = DA8XX_LPSC1_CPGMAC,
		.ctlr = 1,
	},
	{
		.name = "I2CCLK",
		.rate = &fixedrate,
		.id   = 1,
		.lpsc = DA8XX_LPSC1_I2C,
		.ctlr = 1,
	},
	{
		.name = "I2CCLK",
		.rate = &div_by_four,
		.id   = 2,
		.lpsc = DA8XX_LPSC1_I2C,
		.ctlr = 1,
	},
	{
		.name = "McASPCLK0",
		.rate = &commonrate,
		.lpsc = DA8XX_LPSC1_McASP0,
		.ctlr = 1,
	},
	{
		.name = "McASPCLK1",
		.rate = &commonrate,
		.lpsc = DA8XX_LPSC1_McASP1,
		.ctlr = 1,
	},
	{
		.name = "McASPCLK2",
		.rate = &commonrate,
		.lpsc = DA8XX_LPSC1_McASP2,
		.ctlr = 1,
	},
	{
		.name = "gpio",
		.rate = &div_by_four,
		.lpsc = DA8XX_LPSC1_GPIO,
		.ctlr = 1,
	},
	{
		.name = "ECAPCLK",
		.rate = &commonrate,
		.lpsc = DA8XX_LPSC1_ECAP,
		.ctlr = 1,
	},
	{
		.name = "EMIF3CLK",
		.rate = &commonrate,
		.usecount = 1,
		.lpsc = DA8XX_LPSC1_EMIF3C,
		.ctlr = 1,
	},
	{
		.name = "AEMIFCLK",
		.rate = &commonrate,
		.lpsc = DA8XX_LPSC0_EMIF25,
		.ctlr = 0,
	},
	{
		.name = "PWM_CLK",
		.rate = &commonrate,
		.lpsc = DA8XX_LPSC1_PWM,
		.ctlr = 1,
	},
	{
		.name = "IOPUCLK",
		.rate = &div_by_four,
		.lpsc = -1,
	},
	{
		.name = "LCDCTRLCLK",
		.rate = &commonrate,
		.lpsc = DA8XX_LPSC1_LCDC,
		.ctlr = 1,
	},
	{
		.name = "SPI1CLK",
		.rate = &commonrate,
		.lpsc = DA8XX_LPSC1_SPI1,
		.ctlr = 1,
	},
	{
		.name = "USB11CLK",
		.rate = &div_by_four,
		.lpsc = DA8XX_LPSC1_USB11,
		.ctlr = 1,
	},
	{
		.name = "USB20CLK",
		.rate = &commonrate,
		.lpsc = DA8XX_LPSC1_USB20,
		.ctlr = 1,
	},
};

int __init da8xx_clk_init(void)
{
	u32 pll_mult;

	davinci_psc_register(da8xx_psc_bases, 2);

	pll_mult = davinci_readl(DA8XX_PLL_CNTRL0_BASE + PLLM);
	armrate = ((pll_mult + 1) * DA8XX_CLOCK_TICK_RATE) / 2;
	commonrate = armrate / 2;
	div_by_four = armrate / 4;

	return davinci_enable_clks(da8xx_clks, ARRAY_SIZE(da8xx_clks));
}
