/*
 * Copyright 2004-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*!
 * @file mm.c
 *
 * @brief This file creates static virtual to physical mappings, common to all
 * MX2 boards.
 *
 * @ingroup Memory
 */

#include <linux/mm.h>
#include <linux/init.h>

#include <asm/hardware.h>
#include <asm/pgtable.h>
#include <asm/mach/map.h>
#include <asm/arch/common.h>

static struct map_desc io_desc[] __initdata = {
	{
		.virtual = AIPI_BASE_ADDR_VIRT,
		.pfn     = __phys_to_pfn(AIPI_BASE_ADDR),
		.length  = AIPI_SIZE,
		.type    = MT_DEVICE,
	}, {
		.virtual = SAHB1_BASE_ADDR_VIRT,
		.pfn     = __phys_to_pfn(SAHB1_BASE_ADDR),
		.length  = SAHB1_SIZE,
		.type    = MT_DEVICE,
	}, {
		.virtual = X_MEMC_BASE_ADDR_VIRT,
		.pfn     = __phys_to_pfn(X_MEMC_BASE_ADDR),
		.length  = X_MEMC_SIZE,
		.type    = MT_DEVICE,
	},
};

void __init mxc_map_io(void)
{
	iotable_init(io_desc, ARRAY_SIZE(io_desc));
}
