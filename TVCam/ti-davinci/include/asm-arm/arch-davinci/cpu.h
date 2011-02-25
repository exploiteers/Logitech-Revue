/*
 * include/asm-arm/arch-davinci/cpu.h
 *
 * DaVinci/DA8xx CPU type detection
 *
 * Author: Steve Chen <schen@mvista.com>
 * Copyright (c) 2007-2008 MontaVista Software, Inc. <source@mvista.com>
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */

#ifndef _ASM_ARCH_CPU_H
#define _ASM_ARCH_CPU_H

extern unsigned int system_rev;
extern unsigned int davinci_cpu_index;	/* defined and init in io.c */

#define DM644X_CPU_IDX	0
#define DM6467_CPU_IDX	1
#define DM355_CPU_IDX	2
#define DM365_CPU_IDX	3

#define GET_DAVINCI_CPU_TYPE(msk)	((system_rev >> 12) & msk)

#define IS_DAVINCI_CPU(type, id, msk)					\
static inline int cpu_is_davinci_dm ##type(void)			\
{									\
	return (GET_DAVINCI_CPU_TYPE(msk) == (id & msk)) ? 1 : 0;	\
}

#define IS_DA8XX_CPU(type, id, msk)			\
static inline int cpu_is_da ##type(void)		\
{							\
	return GET_DAVINCI_CPU_TYPE(msk) == (id & msk); \
}

/* following generates the cpu_is_davinci_dmxxx */
IS_DAVINCI_CPU(644x, 0x64430, 0xffff0)       /* cpu_is_davinci_dm644x() */
IS_DAVINCI_CPU(644x_pg1x, 0x64430, 0xfffff)  /* cpu_is_davinci_dm644x_pg1x() */
IS_DAVINCI_CPU(6467, 0x64670, 0xffff0)       /* cpu_is_davinci_dm6467() */
IS_DAVINCI_CPU(355, 0x3500, 0xffff0)         /* cpu_is_davinci_dm355() */
IS_DAVINCI_CPU(357, 0x3570, 0xffff0)         /* cpu_is_davinci_dm357() */
IS_DAVINCI_CPU(365, 0x3650, 0xffff0)	     /* cpu_is_davinci_dm365() */

/* The following generates the cpu_is_daxxx */
IS_DA8XX_CPU(830, 0x08300, 0xffff0)	    /* cpu_is_davinci_da830() */

#endif
