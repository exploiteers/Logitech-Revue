/*
 * RapidIO PowerPC support
 *
 * Copyright (C) 2007 Freescale Semiconductor, Inc. All rights reserved.
 * Zhang Wei <wei.zhang@freescale.com>, Jun 2007
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 * New RapidIO peer-to-peer network initialize with of-device supoort.
 *
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/rio.h>

#include <asm/rio.h>
#include <asm/of_device.h>
#include <asm/of_platform.h>

#include <sysdev/fsl_rio.h>


/* The probe function for RapidIO peer-to-peer network.
 */
static int __devinit of_rio_rpn_probe(struct of_device *dev,
				     const struct of_device_id *match)
{
	int rc;
	printk(KERN_INFO "Setting up RapidIO peer-to-peer network %s\n",
			dev->node->full_name);

	rc = fsl_rio_setup(dev);
	if (rc)
		goto out;

	/* Enumerate all registered ports */
	rc = rio_init_mports();
out:
	return rc;
};

static struct of_device_id of_rio_rpn_ids[] = {
	{
		.compatible = "fsl,rapidio-delta",
	},
	{},
};

static struct of_platform_driver of_rio_rpn_driver = {
	.name = "of-rio",
	.match_table = of_rio_rpn_ids,
	.probe = of_rio_rpn_probe,
};

static __init int of_rio_rpn_init(void)
{
	return of_register_platform_driver(&of_rio_rpn_driver);
}

subsys_initcall(of_rio_rpn_init);
