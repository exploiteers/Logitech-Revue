/*
 *  linux/include/asm-arm/arch-realview/hardware.h
 *
 *  This file contains the hardware definitions of the RealView boards.
 *
 *  Copyright (C) 2003 ARM Limited.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef __ASM_ARCH_HARDWARE_H
#define __ASM_ARCH_HARDWARE_H

#include <asm/sizes.h>
#include <asm/arch/platform.h>

#define REALVIEW_GIC1_CPU_VBASE		0xE9000000
#define REALVIEW_GIC1_DIST_VBASE	0xE9001000

#define PCIX_UNIT_BASE			0xE8000000
#define REALVIEW_PB11MPC_PCI_IO_VBASE	0xEA000000

#if defined(CONFIG_MACH_REALVIEW_PB1176)
#define PCIBIOS_MIN_IO			0x43000000
#define PCIBIOS_MIN_MEM			0x44000000
#endif
#define pcibios_assign_all_busses()     1
/* macro to get at IO space when running virtually */
#define IO_ADDRESS(x)          ((((x) & 0x0effffff) | (((x) >> 4) & 0x0f000000)) + 0xf0000000)
#define __io_address(n)		(void __iomem *)IO_ADDRESS(n)

#endif
