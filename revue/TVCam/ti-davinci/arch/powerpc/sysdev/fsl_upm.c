/*
 * Freescale UPM routines.
 *
 * Copyright (c) 2007  MontaVista Software, Inc.
 * Copyright (c) 2007  Anton Vorontsov <avorontsov@ru.mvista.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/kernel.h>
#include <asm/of_platform.h>
#include <asm/fsl_upm.h>

spinlock_t upm_lock = __SPIN_LOCK_UNLOCKED(upm_lock);
unsigned long upm_lock_flags;

int fsl_upm_get_for(struct device_node *node, const char *name,
		    struct fsl_upm *upm)
{
	int ret;
	struct device_node *lbus;
	struct resource lbc_res;
	ptrdiff_t mxmr_offs;

	lbus = of_get_parent(node);
	if (!lbus) {
		pr_err("FSL UPM: can't get parent local bus node\n");
		return -ENOENT;
	}

	ret = of_address_to_resource(lbus, 0, &lbc_res);
	if (ret) {
		pr_err("FSL UPM: can't get parent local bus base\n");
		return -ENOMEM;
	}

	switch (name[0]) {
	case 'A':
		mxmr_offs = LBC_MAMR;
		break;
	case 'B':
		mxmr_offs = LBC_MBMR;
		break;
	case 'C':
		mxmr_offs = LBC_MCMR;
		break;
	default:
		pr_err("FSL UPM: unknown UPM requested\n");
		return -EINVAL;
		break;
	}

	upm->lbc_base = ioremap_nocache(lbc_res.start,
					lbc_res.end - lbc_res.start + 1);
	if (!upm->lbc_base)
		return -ENOMEM;

	upm->mxmr = upm->lbc_base + mxmr_offs;
	upm->mar = upm->lbc_base + LBC_MAR;

	return 0;
}
