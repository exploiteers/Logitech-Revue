/*
 * arch/powerpc/math-emu/efscfd.c
 *
 * Copyright (C) 2006 Freescale Semiconductor, Inc. All rights reserved.
 *
 * Author: Ebony Zhu, ebony.zhu@freescale.com
 *
 * Description:
 * This file is the implementation of SPE instruction "efscfd"
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
efscfd(void *rD, u64 *rB)
{
	FP_DECL_D(B);
	FP_DECL_S(R);
	int ret;
	u32 tmp;
	tmp = rB[0] >> 32;
	rB[0] = rB[0] <<32 | tmp;

#ifdef DEBUG
	printk("%s: S %p, ea %p\n", __FUNCTION__, rD, rB);
#endif

	__FP_UNPACK_D(B, rB);

#ifdef DEBUG
	printk("B: %ld %lu %lu %ld (%ld)\n", B_s, B_f1, B_f0, B_e, B_c);
#endif

	FP_CONV(S, D, 1, 2, R, B);

#ifdef DEBUG
	printk("R: %ld %lu %ld (%ld)\n", R_s, R_f, R_e, R_c);
#endif

	return (ret | __FP_PACK_S(rD, R));
}
