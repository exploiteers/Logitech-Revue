/*
 * arch/powerpc/math-emu/efscmplt.c
 *
 * Copyright (C) 2006 Freescale Semiconductor, Inc. All rights reserved.
 *
 * Author: Ebony Zhu, ebony.zhu@freescale.com
 *
 * Description:
 * This file is the implementation of SPE instruction "efscmplt"
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
efscmplt(u32 *ccr, int crD, void *rA, void *rB)
{
	FP_DECL_S(A);
	FP_DECL_S(B);
	long cmp;
	int ret = 0;

#ifdef DEBUG
	printk("%s: %p (%08x) %d %p %p\n", __FUNCTION__, ccr, *ccr, crD, rA, rB);
#endif

	__FP_UNPACK_S(A, rA);
	__FP_UNPACK_S(B, rB);

#ifdef DEBUG
	printk("A: %ld %lu %ld (%ld)\n", A_s, A_f, A_e, A_c);
	printk("B: %ld %lu %ld (%ld)\n", B_s, B_f, B_e, B_c);
#endif

	FP_CMP_S(cmp, A, B, 2);
	
	if (cmp == -1) {
		cmp = 0x4;
	} else {
		cmp = 0;
	}
	
	*ccr &= ~(15 << ((7 - crD) << 2));
	*ccr |= (cmp << ((7 - crD) << 2));

#ifdef DEBUG
	printk("CR: %08x\n", *ccr);
#endif

	return ret;
}
