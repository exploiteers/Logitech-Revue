/*
 * arch/powerpc/math-emu/efsctuf.c
 *
 * Copyright (C) 2006 Freescale Semiconductor, Inc. All rights reserved.
 *
 * Author: Ebony Zhu, ebony.zhu@freescale.com
 *
 * Description:
 * This file is the implementation of SPE instruction "efsctuf"
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
efsctuf(u32 *rD, u32 *rB)
{
        if (!((rB[0] >> 23) == 0xff && ((rB[0] & 0x7fffff) > 0 )) /* Not an NaN */
	    && (rB[0] >> 31) == 0) { /* rB is positive */
		if (((rB[0] >> 23) & 0xff) == 0 ) { /* rB is Denorm */
			rD[0] = 0x0;
		} else { /* rB is normal */
			rD[0] = 0xffffffff;
		}
        } else { /* rB < 0 or rB is NaN */
		rD[0] = 0x0;
	}
#ifdef DEBUG
	printk("%s: D %p, B %p: ", __FUNCTION__, rD, rB);
	printk("\n");
#endif

	return 0;
}
