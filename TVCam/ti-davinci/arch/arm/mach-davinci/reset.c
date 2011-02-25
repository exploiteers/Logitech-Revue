/*
 * TI DaVinci board reset code
 *
 * Author: Kevin Hilman, MontaVista Software, Inc.
 * Copyright (C) 2007-2008 MontaVista Software, Inc. <source@mvista.com>
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 *
 */
#include <linux/kernel.h>

#include <asm/hardware.h>
#include <asm/arch/time.h>

void arch_reset(char mode)
{
	davinci_watchdog_reset(DAVINCI_WDOG_BASE);
}
