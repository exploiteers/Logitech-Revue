/*
 * arch/powerpc/math-emu/evfscmplt.c
 *
 * Copyright (C) 2006 Freescale Semiconductor, Inc. All rights reserved.
 *
 * Author: Ebony Zhu, ebony.zhu@freescale.com
 *
 * Description:
 * This file is the implementation of SPE instruction "evfscmplt"
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
evfscmplt(u32 *ccr, int crD, u32 *rA, u32 *rB)
{
	FP_DECL_S(A0);
	FP_DECL_S(A1);
	FP_DECL_S(B0);
	FP_DECL_S(B1);
	long cmp, cmp0, cmp1, ch, cl;
	int ret = 0;

#ifdef DEBUG
	printk("%s: %p (%08x) %d %p %p\n", __FUNCTION__, ccr, *ccr, crD, rA, rB);
#endif

	__FP_UNPACK_S(A0, rA);
	rA[0] = rA[1];
	__FP_UNPACK_S(A1, rA);
	__FP_UNPACK_S(B0, rB);
	rB[0] = rB[1];
	__FP_UNPACK_S(B1, rB);

#ifdef DEBUG
	printk("A0: %ld %lu %ld (%ld)\n", A0_s, A0_f, A0_e, A0_c);
	printk("A1: %ld %lu %ld (%ld)\n", A1_s, A1_f, A1_e, A1_c);
	printk("B0: %ld %lu %ld (%ld)\n", B0_s, B0_f, B0_e, B0_c);
	printk("B1: %ld %lu %ld (%ld)\n", B1_s, B1_f, B1_e, B1_c);
#endif

	FP_CMP_S(cmp0, A0, B0, 2);
	FP_CMP_S(cmp1, A1, B1, 2);

	ch = (cmp0 == -1) ? 1 : 0;
	cl = (cmp1 == -1) ? 1 : 0;
	cmp = 0;
	cmp = (ch << 3) | (cl << 2) | ((ch | cl) << 1) | ((ch & cl) << 0);

	*ccr &= ~(15 << ((7 - crD) << 2));
	*ccr |= (cmp << ((7 - crD) << 2));

#ifdef DEBUG
	printk("CR: %08x\n", *ccr);
#endif

	return ret;
}
