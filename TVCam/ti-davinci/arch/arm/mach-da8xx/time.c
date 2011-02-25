/*
 * Copyright (C) 2008 MontaVista Software, Inc. <source@mvista.com>
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */

#include <linux/kernel.h>
#include <linux/init.h>

#include <asm/hardware.h>
#include <asm/mach/irq.h>
#include <asm/mach/time.h>
#include <asm/arch/time.h>

static	int da8xx_timer_irqs[NUM_TIMERS] = {
	IRQ_DA8XX_TINT12_0,
	IRQ_DA8XX_TINT34_0,
	IRQ_DA8XX_TINT12_1,
	IRQ_DA8XX_TINT34_1
};

/* Compare registers are only available to the bottom timer 0 */
static	int da8xx_cmp_irqs[NUM_TIMERS] = {
	IRQ_DA8XX_T12CMPINT0_0,
};

static u32 da8xx_bases[] = { DA8XX_TIMER64P0_BASE, DA8XX_TIMER64P1_BASE };

static void __init davinci_timer_init(void)
{
	/*
	 * Configure the 2 64-bit timer as 4 32-bit timers with
	 * following assignments.
	 *
	 * T0_BOT: Timer 0, bottom: free run counter and system clock.
	 * T0_TOP: Timer 0, top: reserved for DSP.
	 * T1_BOT: Timer 1: used as watchdog timer.
	 */
	davinci_common_timer_init(da8xx_bases, 2, da8xx_timer_irqs,
				  da8xx_cmp_irqs, T0_BOT, T0_BOT);
}

struct sys_timer davinci_timer = {
	.init	= davinci_timer_init,
};
