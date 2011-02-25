/*
 * DaVinci memory space definitions
 *
 * Author: Kevin Hilman, MontaVista Software, Inc.
 * Copyright (C) 2007-2008 MontaVista Software, Inc. <source@mvista.com>
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */
#ifndef __ASM_ARCH_MEMORY_H
#define __ASM_ARCH_MEMORY_H

/**************************************************************************
 * Included Files
 **************************************************************************/
#include <asm/page.h>
#include <asm/sizes.h>

/**************************************************************************
 * Definitions
 **************************************************************************/
#define DAVINCI_DDR_BASE	(0x80000000 \
		+ CONFIG_DAVINCI_KERNEL_SDRAM_OFFSET)
#define DAVINCI_IRAM_BASE	0x00008000 /* ARM Internal RAM */

#define DA8XX_DDR_BASE		0xc0000000
#define DA8XX_IRAM_BASE 	0x80000000 /* ARM Internal RAM */

#ifdef	CONFIG_ARCH_DA8XX
#define PHYS_OFFSET	DA8XX_DDR_BASE
#else
#define PHYS_OFFSET	DAVINCI_DDR_BASE
#endif

/*
 * Increase size of DMA-consistent memory region
 */
#define CONSISTENT_DMA_SIZE (40<<20)

#ifndef __ASSEMBLY__
/*
 * Restrict DMA-able region to workaround silicon bug.  The bug
 * restricts buffers available for DMA to video hardware to be
 * below 128M
 */
static inline void
__arch_adjust_zones(int node, unsigned long *size, unsigned long *holes)
{
	unsigned int sz = (128<<20) >> PAGE_SHIFT;

	if (node != 0)
		sz = 0;

	size[1] = size[0] - sz;
	size[0] = sz;
}

#define arch_adjust_zones(node, zone_size, holes) \
        if ((meminfo.bank[0].size >> 20) > 128) __arch_adjust_zones(node, zone_size, holes)

#define ISA_DMA_THRESHOLD	(PHYS_OFFSET + (128<<20) - 1)

#endif

/*
 * Bus address is physical address
 */
#define __virt_to_bus(x)	__virt_to_phys(x)
#define __bus_to_virt(x)	__phys_to_virt(x)

#endif /* __ASM_ARCH_MEMORY_H */
