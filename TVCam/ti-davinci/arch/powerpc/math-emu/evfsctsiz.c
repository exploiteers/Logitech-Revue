/*
 * arch/powerpc/math-emu/evfsctsiz.c
 *
 * Copyright (C) 2006 Freescale Semiconductor, Inc. All rights reserved.
 *
 * Author: Ebony Zhu, ebony.zhu@freescale.com
 *
 * Description:
 * This file is the implementation of SPE instruction "evfsctsiz"
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
evfsctsiz(u32 *rD, u32 *rB)
{
	FP_DECL_S(B0);
	FP_DECL_S(B1);
	unsigned int r0, r1;

	__FP_UNPACK_S(B0, rB);
	__FP_UNPACK_S(B1, rB+1);
	_FP_ROUND_ZERO(1, B0);
	_FP_ROUND_ZERO(1, B1);
	FP_TO_INT_S(r0, B0, 32, 1);
	rD[0] = r0;
	FP_TO_INT_S(r1, B1, 32, 1);
	rD[1] = r1;

#ifdef DEBUG
	printk("%s: D %p, B %p: ", __FUNCTION__, rD, rB);
	printk("\n");
#endif

	return 0;
}
