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

#ifndef __ASM_POWERPC_FSL_UPM
#define __ASM_POWERPC_FSL_UPM

#include <linux/spinlock.h>
#include <asm/io.h>

#define LBC_MAR		0x68
#define LBC_MAMR	0x70
#define LBC_MBMR	0x74
#define LBC_MCMR	0x78

#define LBC_MXMR_RUNP	0x30000000

struct fsl_upm {
	void __iomem *lbc_base;
	void __iomem *mxmr;
	void __iomem *mar;
};

extern spinlock_t upm_lock;
extern unsigned long upm_lock_flags;

extern int fsl_upm_get_for(struct device_node *node, const char *name,
			   struct fsl_upm *upm);

static inline void fsl_upm_free(struct fsl_upm *upm)
{
	iounmap(upm->lbc_base);
	upm->lbc_base = NULL;
}

static inline int fsl_upm_got(struct fsl_upm *upm)
{
	return !!upm->lbc_base;
}

static inline void fsl_upm_start_pattern(struct fsl_upm *upm, u32 pat_offset)
{
	out_be32(upm->mxmr, LBC_MXMR_RUNP | pat_offset);
}

static inline void fsl_upm_end_pattern(struct fsl_upm *upm)
{
	out_be32(upm->mxmr, 0x0);

	while (in_be32(upm->mxmr) != 0x0)
		cpu_relax();
}

static inline int fsl_upm_run_pattern(struct fsl_upm *upm,
				      void __iomem *io_base,
				      int width, u32 cmd)
{
	out_be32(upm->mar, cmd << (32 - width));

	switch (width) {
	case 8:
		out_8(io_base, 0x0);
		break;
	case 16:
		out_be16(io_base, 0x0);
		break;
	case 32:
		out_be32(io_base, 0x0);
		break;
	default:
		return -EINVAL;
		break;
	}

	return 0;
}

#endif /* __ASM_POWERPC_FSL_UPM */
