/*
 * DA8xx common platform reset code
 *
 * Author: David Griego <dgriego@mvista.com>
 *
 * Copyright (C) 2008 MontaVista Software, Inc. <source@mvista.com>
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */
#include <linux/kernel.h>

#include <asm/hardware.h>

#include <asm/arch/time.h>

/*
 * This should reset the system in most, if not all, DA8xx platforms.
 * Compile this file if you don't have a better way to reset your board.
 */
void arch_reset(char mode)
{
	davinci_watchdog_reset(DA8XX_WDOG_BASE);
}
