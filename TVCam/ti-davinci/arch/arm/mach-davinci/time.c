/*
 * DaVinci timer subsystem
 *
 * Author: Kevin Hilman, MontaVista Software, Inc.
 * Copyright (C) 2007-2008 MontaVista Software, Inc. <source@mvista.com>
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
#include <asm/arch/cpu.h>
#include <asm/arch/time.h>

static int timer_irqs[NUM_TIMERS] = {
	IRQ_TINT0_TINT12,
	IRQ_TINT0_TINT34,
	IRQ_TINT1_TINT12,
	IRQ_TINT1_TINT34,
};

static u32 davinci_bases[] = { DAVINCI_TIMER0_BASE, DAVINCI_TIMER1_BASE };

static void __init davinci_timer_init(void)
{
	int system;

	if (cpu_is_davinci_dm6467()) {
		/*
		 * Configure the 2 64-bit timer as 4 32-bit timers with
		 * following assignments.
		 *
		 * T0_BOT: Timer 0, bottom:  AV Sync
		 * T0_TOP: Timer 0, top:  free-running counter,
		 *                        used for cycle counter
		 * T1_BOT: Timer 1, bottom:  reserved for DSP
		 * T1_TOP: Timer 1, top   :  Linux system tick
		 */
		system = T1_TOP;
	} else {
		/*
		 * Configure the 2 64-bit timer as 4 32-bit timers with
		 * following assignments.
		 *
		 * T0_BOT: Timer 0, bottom:  clockevent source for hrtimers
		 * T0_TOP: Timer 0, top   :  clocksource for generic timekeeping
		 * T1_BOT: Timer 1, bottom:  (used by DSP in TI DSPLink code)
		 * T1_TOP: Timer 1, top   :  <unused>
		 */
		system = T0_BOT;
	}
	davinci_common_timer_init(davinci_bases, 2, timer_irqs, NULL,
				  system, T0_TOP);
}

struct sys_timer davinci_timer = {
	.init   = davinci_timer_init,
};
