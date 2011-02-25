/*
 * TI DA8xx GPIO Support
 *
 * Copyright (c) 2008 MontaVista Software, Inc. <source@mvista.com>
 *
 * Based on: arch/arm/mach-davinci/gpio.c
 * Copyright (c) 2006	David Brownell
 * Copyright (c) 2007, MontaVista Software, Inc. <source@mvista.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/kernel.h>

#include <asm/arch/irqs.h>
#include <asm/arch/hardware.h>
#include <asm/arch/cpu.h>
#include <asm/arch/gpio.h>

/*
 * If a new chip is added with number of GPIO greater than 128, please
 * update DA8XX_MAX_N_GPIO in include/asm-arm/arch-davinci/irqs.h
 */
#define DA8xx_N_GPIO	128

static DECLARE_BITMAP(da8xx_gpio_in_use, DA8xx_N_GPIO);
static struct gpio_bank gpio_bank_da8xx = {
	.base		= DA8XX_GPIO_BASE,
	.num_gpio	= DA8xx_N_GPIO,
	.irq_num	= IRQ_DA8XX_GPIO0,
	.irq_mask	= 0x1ff,
	.in_use		= da8xx_gpio_in_use,
};

void davinci_gpio_init(void)
{
	davinci_gpio_irq_setup(&gpio_bank_da8xx);
}
