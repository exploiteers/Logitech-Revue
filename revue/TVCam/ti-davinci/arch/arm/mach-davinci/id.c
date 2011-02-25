/*
 * Davinci CPU identification code
 *
 * Copyright (C) 2006 Komal Shah <komal_shah802003@yahoo.com>
 *
 * Derived from OMAP1 CPU identification code.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

#include <asm/io.h>

#define JTAG_ID_BASE		0x01c40028

/*
 * On dm357 JTAG_ID_REGISTER returns the same value as it does
 * on dm6446 pg2.1. To differentiate between them TI engineers
 * suggested to use this register:
 *
 * "I believe, dm357 identification needs to be done using a
 *  Vdd enable register.  The Vdd enable/adjust registers are
 *  located at: 0x01C4 0018 and 001C with value = 0x0000 000F
 *  (shown reserved in the datasheet).
 *
 *  If you have a value of 0xF, you can consider this as DM357."
 */
#define VDD_STATUS_BASE		0x01c4001c

struct davinci_id {
	u8	variant;	/* JTAG ID bits 31:28 */
	u16	part_no;	/* JTAG ID bits 27:12 */
	u32	manufacturer;	/* JTAG ID bits 11:1 */
	u32	type;		/* Cpu id bits [31:8], cpu class bits [7:0] */
};

/* Register values to detect the DaVinci version */
static struct davinci_id davinci_ids[] __initdata = {
	{
		/* DM6446 PG 1.3 and earlier */
		.part_no      = 0xb700,
		.variant      = 0x0,
		.manufacturer = 0x017,
		.type	      = 0x64430000,
	},
	{
		/* DM6446 PG 2.1 */
		.part_no      = 0xb700,
		.variant      = 0x1,
		.manufacturer = 0x017,
		.type	      = 0x6443A000,
	},
        {
                /* DM6467 */
                .part_no      = 0xb770,
                .variant      = 0x0,
                .manufacturer = 0x017,
                .type         = 0x64670000,
        },
	{
		/* DM355 */
		.part_no      = 0xb73b,
		.variant      = 0x0,
		.manufacturer = 0x00f,
		.type	      = 0x03500000,
	},
	{
		/* DM365 */
		.part_no      = 0xb83e,
		.variant      = 0x0,
		.manufacturer = 0x017,
		.type	      = 0x03650000,
	},
};

/*
 * Get Device Part No. from JTAG ID register
 */
static u16 __init davinci_get_part_no(void)
{
	u32 dev_id, part_no;

	dev_id = davinci_readl(JTAG_ID_BASE);

	part_no = ((dev_id >> 12) & 0xffff);

	return part_no;
}

/*
 * Get Device Revision from JTAG ID register
 */
static u8 __init davinci_get_variant(void)
{
	u32 variant;

	variant = davinci_readl(JTAG_ID_BASE);

	variant = (variant >> 28) & 0xf;

	return variant;
}

void __init davinci_check_revision(void)
{
	int i;
	u16 part_no;
	u8 variant;

	part_no = davinci_get_part_no();
	variant = davinci_get_variant();

	/* First check only the major version in a safe way */
	for (i = 0; i < ARRAY_SIZE(davinci_ids); i++) {
		if (part_no == (davinci_ids[i].part_no)) {
			system_rev = davinci_ids[i].type;
			break;
		}
	}

	/* Check if we can find the dev revision */
	for (i = 0; i < ARRAY_SIZE(davinci_ids); i++) {
		if (part_no == davinci_ids[i].part_no &&
		    variant == davinci_ids[i].variant) {
			system_rev = davinci_ids[i].type;
			break;
		}
	}

	if (system_rev == 0x6443A000 &&
			(davinci_readl(VDD_STATUS_BASE) == 0xf)) {
			system_rev = 0x03570000;
			variant = 0;
	}

	printk(KERN_INFO "DaVinci DM%04x variant 0x%x\n",
			system_rev >> 16, variant);
}
