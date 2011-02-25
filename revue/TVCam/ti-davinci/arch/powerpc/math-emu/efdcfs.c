/*
 * arch/powerpc/math-emu/efdcfs.c
 *
 * Copyright (C) 2006 Freescale Semiconductor, Inc. All rights reserved.
 *
 * Author: Ebony Zhu, ebony.zhu@freescale.com
 *
 * Description:
 * This file is the implementation of SPE instruction "efdcfs"
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
#include "single.h"

int
efdcfs(void *rD, u32 *rB)
{
	FP_DECL_S(B);
	FP_DECL_D(R);
	int ret;
	rB[0]=rB[1];

	__FP_UNPACK_S(B, rB);

#ifdef DEBUG
	printk("B: %ld %lu %ld (%ld)\n", B_s, B_f, B_e, B_c);
#endif

	FP_CONV(D, S, 2, 1, R, B);

#ifdef DEBUG
	printk("R: %ld %lu %lu %ld (%ld)\n", R_s, R_f1, R_f0, R_e, R_c);
#endif
	
	return (ret | __FP_PACK_D(rD, R));
}
