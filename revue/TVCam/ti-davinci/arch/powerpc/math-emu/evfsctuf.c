/*
 * arch/powerpc/math-emu/evfsctuf.c
 *
 * Copyright (C) 2006 Freescale Semiconductor, Inc. All rights reserved.
 *
 * Author: Ebony Zhu, ebony.zhu@freescale.com
 *
 * Description:
 * This file is the implementation of SPE instruction "evfsctuf"
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <linux/types.h>
#include <linux/errno.h>
#include <asm/uaccess.h>

#include "spe.h"
#include "soft-fp.h"
#include "single.h"

int
evfsctuf(u32 *rD, u32 *rB)
{
	__asm__ __volatile__ ("mtspr 512, %4\n"
			      "efsctuf %0, %2\n"
			      "efsctuf %1, %3\n"
			      : "=r" (rD[0]), "=r" (rD[1])
                              : "r" (rB[0]), "r" (rB[1]), "r" (0));

#ifdef DEBUG
	printk("%s: D %p, B %p: ", __FUNCTION__, rD, rB);
	printk("\n");
#endif

	return 0;
}
