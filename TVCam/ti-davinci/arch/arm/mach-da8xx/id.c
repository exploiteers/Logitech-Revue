/*
 * DA8xx CPU identification code
 *
 * Copyright (C) 2008 MontaVista Software, Inc. <source@mvista.com>
 *
 * Based on arch/arm/mach-davinci/id.c
 * Copyright (C) 2006 Komal Shah <komal_shah802003@yahoo.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/io.h>

#define JTAG_ID_REG		0x01c14018

struct	da8xx_id {
	u8	variant;	/* JTAG ID bits 31:28 */
	u16	part_no;	/* JTAG ID bits 27:12 */
	u32	manufacturer;	/* JTAG ID bits 11:1 */
	u32	type;		/* Cpu id bits [31:8], cpu class bits [7:0] */
};

/* Register values to detect the DaVinci version */
static struct da8xx_id da8xx_ids[] __initdata = {
	{
		/* DA8xx */
		.part_no	= 0xb7df,
		.variant	= 0x0,
		.manufacturer	= 0x017,
		.type		= 0x08300000
	},
};

void __init da8xx_check_revision(void)
{
	u32 jtag_id	= davinci_readl(JTAG_ID_REG);
	u16 part_no	= (jtag_id >> 12) & 0xffff;
	u8  variant	= (jtag_id >> 28) & 0xf;
	int i;

	/* First check only the major version in a safe way */
	for (i = 0; i < ARRAY_SIZE(da8xx_ids); i++)
		if (part_no == da8xx_ids[i].part_no) {
			system_rev = da8xx_ids[i].type;
			break;
		}

	/* Check if we can find the dev revision */
	for (i = 0; i < ARRAY_SIZE(da8xx_ids); i++)
		if (part_no == da8xx_ids[i].part_no &&
		    variant == da8xx_ids[i].variant) {
			system_rev = da8xx_ids[i].type;
			break;
		}

	printk(KERN_INFO "DA%x variant %#x\n", system_rev >> 16, variant);
}
