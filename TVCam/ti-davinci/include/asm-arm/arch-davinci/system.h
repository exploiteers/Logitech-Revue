/*
 * DaVinci system definitions
 *
 * Author: Kevin Hilman, MontaVista Software, Inc.
 * (C) 2007-2008 MontaVista Software, Inc. <source@mvista.com>
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */
#ifndef __ASM_ARCH_SYSTEM_H
#define __ASM_ARCH_SYSTEM_H

#include <asm/io.h>
#include <asm/hardware.h>

static void arch_idle(void)
{
	cpu_do_idle();
}

void arch_reset(char mode);

#endif /* __ASM_ARCH_SYSTEM_H */
