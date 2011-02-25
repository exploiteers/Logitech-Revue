/*
 *  Copyright 2004-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*!
 * @file hardware.h
 * @brief This file contains the hardware definitions of the board.
 *
 * @ingroup System
 */
#ifndef __ASM_ARCH_MXC_HARDWARE_H__
#define __ASM_ARCH_MXC_HARDWARE_H__

#include <asm/sizes.h>

#ifdef CONFIG_ARCH_MX2
#include <asm/arch/mx27.h>
#endif

#include <asm/arch/mxc.h>

#define MXC_MAX_GPIO_LINES      (GPIO_NUM_PIN * GPIO_PORT_NUM)

/*
 * Processor specific defines
 * ---------------------------------------------------------------------------
 */
#define CHIP_REV_1_0		0x10
#define CHIP_REV_1_1		0x11
#define CHIP_REV_1_2		0x12
#define CHIP_REV_1_3		0x13
#define CHIP_REV_2_0		0x20
#define CHIP_REV_2_1		0x21
#define CHIP_REV_2_2		0x22
#define CHIP_REV_2_3		0x23
#define CHIP_REV_3_0		0x30
#define CHIP_REV_3_1		0x31
#define CHIP_REV_3_2		0x32

#ifndef cpu_is_mx27
#define cpu_is_mx27()		(0)
#endif

#if !defined(__ASSEMBLY__) && !defined(__MXC_BOOT_UNCOMPRESS)

/*
 * Create inline functions to test for cpu revision
 * Function name is cpu_is_<cpu name>_rev(rev)
 *
 * Returns:
 *	 0 - not the cpu queried
 *	 1 - cpu and revision match
 *	 2 - cpu matches, but cpu revision is greater than queried rev
 *	-1 - cpu matches, but cpu revision is less than queried rev
 */

extern unsigned int system_rev;
#define _is_rev(rev) ((system_rev == rev) ? 1 : ((system_rev < rev) ? -1 : 2))

#define MXC_REV(type)				\
static inline int type## _rev (int rev)		\
{						\
	return (type() ? _is_rev(rev) : 0);	\
}

MXC_REV(cpu_is_mx27);

#endif

/*
 * ---------------------------------------------------------------------------
 * ---------------------------------------------------------------------------
 * Board specific defines
 * ---------------------------------------------------------------------------
 */
#define MXC_EXP_IO_BASE         (MXC_GPIO_INT_BASE + MXC_MAX_GPIO_LINES)

#include <asm/arch/board.h>

#ifndef MXC_MAX_EXP_IO_LINES
#define MXC_MAX_EXP_IO_LINES 0
#endif

#define MXC_MAX_VIRTUAL_INTS	16
#define MXC_VIRTUAL_INTS_BASE	(MXC_EXP_IO_BASE + MXC_MAX_EXP_IO_LINES)
#define MXC_SDIO1_CARD_IRQ	MXC_VIRTUAL_INTS_BASE
#define MXC_SDIO2_CARD_IRQ	(MXC_VIRTUAL_INTS_BASE + 1)
#define MXC_SDIO3_CARD_IRQ	(MXC_VIRTUAL_INTS_BASE + 2)

#define MXC_MAX_INTS		(MXC_MAX_INT_LINES + \
				MXC_MAX_GPIO_LINES + \
				MXC_MAX_EXP_IO_LINES + \
				MXC_MAX_VIRTUAL_INTS)

#endif				/* __ASM_ARCH_MXC_HARDWARE_H__ */
