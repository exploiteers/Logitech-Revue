/*
 * DA8xx I/O mapping code
 *
 * Copyright (C) 2008 MontaVista Software, Inc. <source@mvista.com>
 *
 * Based on arch/arm/mach-davinci/io.c
 * Copyright (C) 2005-2006 Texas Instruments
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/memory.h>

#include <asm/tlb.h>
#include <asm/mach/map.h>
#include <asm/arch/clock.h>
#include <asm/arch/mux.h>
#include <asm/arch/hardware.h>
#include <asm/arch/common.h>
#include <asm/arch/cpu.h>

#include "da8xx.h"

/*
 * The machine specific code may provide the extra mapping besides the
 * default mapping provided here.
 */
static struct map_desc da8xx_io_desc[] __initdata = {
	{
		.virtual	= IO_VIRT,
		.pfn		= __phys_to_pfn(IO_PHYS),
		.length		= IO_SIZE,
		.type		= MT_DEVICE
	},
	{
		.virtual	= CP_INTC_VIRT,
		.pfn		= __phys_to_pfn(DA8XX_ARM_INTC_BASE),
		.length		= CP_INTC_SIZE,
		.type		= MT_DEVICE
	},
};

void __init da8xx_map_common_io(void)
{
	iotable_init(da8xx_io_desc, ARRAY_SIZE(da8xx_io_desc));

	/*
	 * Normally devicemaps_init() would flush caches and tlb after
	 * mdesc->map_io(), but we must also do it here because of the CPU
	 * revision check below.
	 */
	local_flush_tlb_all();
	flush_cache_all();

	/*
	 * We want to check CPU revision early for cpu_is_daxxx() macros.
	 * I/O space mapping must be initialized before we can do that.
	 */
	da8xx_check_revision();
}

void __init da8xx_init_common_hw(void)
{
	da8xx_mux_init();
	da8xx_clk_init();
}
