/*
 * Copyright 2007 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

/*!
 * @file cpu.c
 *
 * @brief This file contains the CPU initialization code.
 *
 * @ingroup System
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <asm/setup.h>
#include <asm/arch/hardware.h>
#include <asm/io.h>

#define SYS_CHIP_ID		(IO_ADDRESS(SYSCTRL_BASE_ADDR) + 0x0)
#define SYS_CHIP_VERSION_OFFSET 28
#define SYS_CHIP_VERSION_MASK   (0xF<<28)

static const u32 system_rev_tbl[SYSTEM_REV_NUM][2] = {
	/* SREV, own defined ver */
	{0x00, CHIP_REV_1_0},	/* MX27 TO1 */
	{0x01, CHIP_REV_2_0},	/* MX27 TO2 */
};

static int system_rev_updated;	/* = 0 */

static void __init system_rev_setup(char **p)
{
	system_rev = simple_strtoul(*p, NULL, 16);
	system_rev_updated = 1;
}

/* system_rev=0x00 for TO1; 0x01 for TO2; etc */
__early_param("system_rev=", system_rev_setup);

/*
 * FIXME: i.MX27 TO2 has the private rule to indicate the version of chip
 * This functions reads the CHIP ID register and returns the system revision
 * number.
 */
static u32 read_system_rev(void)
{
	u32 val;
	u32 i;

	val = __raw_readl(SYS_CHIP_ID);
	val = (val & SYS_CHIP_VERSION_MASK) >> SYS_CHIP_VERSION_OFFSET;
	for (i = 0; i < SYSTEM_REV_NUM; i++) {
		if (val == system_rev_tbl[i][0]) {
			return system_rev_tbl[i][1];
		}
	}
	if (i == SYSTEM_REV_NUM) {
		val = system_rev_tbl[SYSTEM_REV_NUM - 1][1];
		printk(KERN_ALERT "WARNING: Can't find valid system rev\n");
		printk(KERN_ALERT "Assuming last known system_rev=0x%x\n", val);
		return val;
	}
	return 0;
}

/*
 * Update the system_rev value.
 * If no system_rev is passed in through the command line, it gets the value
 * from the IIM module. Otherwise, it uses the pass-in value.
 */
static void system_rev_update(void)
{
	int i;

	if (!system_rev_updated) {
		/* means NO value passed-in through command line */
		system_rev = read_system_rev();
		pr_info("system_rev is: 0x%x\n", system_rev);
	} else {
		pr_info("Command line passed system_rev: 0x%x\n", system_rev);
		for (i = 0; i < SYSTEM_REV_NUM; i++) {
			if (system_rev == system_rev_tbl[i][1]) {
				break;
			}
		}
		/* Reach here only the SREV value unknown.
		 * Maybe due to new tapeout? */
		if (i == SYSTEM_REV_NUM) {
			panic("Command system_rev is unknown\n");
		}
	}
}

void mxc_cpu_common_init(void)
{
	system_rev_update();
}
