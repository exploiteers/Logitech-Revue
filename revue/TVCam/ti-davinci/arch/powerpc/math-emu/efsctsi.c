/*
 * arch/powerpc/math-emu/efsctsi.c
 *
 * Copyright (C) 2006 Freescale Semiconductor, Inc. All rights reserved.
 *
 * Author: Ebony Zhu, ebony.zhu@freescale.com
 *
 * Description:
 * This file is the implementation of SPE instruction "efsctsi"
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
efsctsi(u32 *rD, void *rB)
{
	FP_DECL_S(B);
	unsigned int r;

	__FP_UNPACK_S(B, rB);
	_FP_ROUND(1, B);
	FP_TO_INT_S(r, B, 32, 1);
	rD[0] = r;

#ifdef DEBUG
	printk("%s: D %p, B %p: ", __FUNCTION__, rD, rB);
	printk("\n");
#endif

	return 0;
}
