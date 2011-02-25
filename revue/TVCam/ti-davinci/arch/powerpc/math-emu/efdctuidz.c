/*
 * arch/powerpc/math-emu/efdctuidz.c
 *
 * Copyright (C) 2006 Freescale Semiconductor, Inc. All rights reserved.
 *
 * Author: Ebony Zhu, ebony.zhu@freescale.com
 *
 * Description:
 * This file is the implementation of SPE instruction "efdctuidz"
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
#include "double.h"

int
efdctuidz(u32 *rD, void *rB)
{
	FP_DECL_D(B);
	u64 r;

	__FP_UNPACK_D(B, rB);
	_FP_ROUND_ZERO(2, B);
	FP_TO_INT_D(r, B, 64, 0);
	rD[1] = r;
	rD[0] = r >> 32;

#ifdef DEBUG
	printk("%s: D %p, B %p: ", __FUNCTION__, rD, rB);
	dump_double(rD);
	printk("\n");
#endif

	return 0;
}
