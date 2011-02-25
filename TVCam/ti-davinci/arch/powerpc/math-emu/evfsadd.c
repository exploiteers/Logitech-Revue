/*
 * arch/powerpc/math-emu/evfsadd.c
 *
 * Copyright (C) 2006 Freescale Semiconductor, Inc. All rights reserved.
 *
 * Author: Ebony Zhu, ebony.zhu@freescale.com
 *
 * Description:
 * This file is the implementation of SPE instruction "evfsadd"
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
evfsadd(u64 *rD, u32 *rA, u32 *rB)
{
	FP_DECL_S(A0);
	FP_DECL_S(A1);
	FP_DECL_S(B0);
	FP_DECL_S(B1);
	FP_DECL_S(R0);
	FP_DECL_S(R1);
	int ret = 0;

#ifdef DEBUG
	printk("%s: %p %p %p\n", __FUNCTION__, rD, rA, rB);
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
	
	FP_ADD_S(R0, A0, B0);
	FP_ADD_S(R1, A1, B1);

#ifdef DEBUG
	printk("D0: %ld %lu %ld (%ld)\n", R0_s, R0_f, R0_e, R0_c);
	printk("D1: %ld %lu %ld (%ld)\n", R1_s, R1_f, R1_e, R1_c);
#endif
	
	return (ret | __FP_PACK_S(rD, R0) | __FP_PACK_S(rD+1, R1));
}
