/*
 * arch/powerpc/platforms/83xx/mpc834x_itx.c
 *
 * MPC834x ITX board specific routines
 *
 * Maintainer: Kumar Gala <galak@kernel.crashing.org>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/config.h>
#include <linux/stddef.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/reboot.h>
#include <linux/pci.h>
#include <linux/kdev_t.h>
#include <linux/major.h>
#include <linux/console.h>
#include <linux/delay.h>
#include <linux/seq_file.h>
#include <linux/root_dev.h>
#include <linux/pata_platform.h>

#include <asm/system.h>
#include <asm/atomic.h>
#include <asm/time.h>
#include <asm/io.h>
#include <asm/machdep.h>
#include <asm/of_platform.h>
#include <asm/ipic.h>
#include <asm/bootinfo.h>
#include <asm/irq.h>
#include <asm/prom.h>
#include <asm/udbg.h>
#include <sysdev/fsl_soc.h>

#include "mpc83xx.h"

#include <platforms/83xx/mpc834x_sys.h>

void early_printk(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	udbg_printf (fmt, args);
	va_end(args);
}

static int __init ide_of_init(void)
{
	struct device_node *np;
	unsigned int i;

	for (np = NULL, i = 0;
	     (np = of_find_node_by_name(np, "ide")) != NULL;
	     i++) {
		int ret;
		struct resource res[3];
		struct platform_device *pdev;
		static struct pata_platform_info pdata;

		memset(res, 0, sizeof(res));

		ret = of_address_to_resource(np, 0, &res[0]);
		if (ret) {
			printk(KERN_ERR "ide.%d: unable to get "
			       "resource from OF\n", i);
			goto err0;
		}

		ret = of_address_to_resource(np, 1, &res[1]);
		if (ret) {
			printk(KERN_ERR "ide.%d: unable to get "
			       "resource from OF\n", i);
			goto err0;
		}

		res[2].start = res[2].end = irq_of_parse_and_map(np, 0);
		if (res[2].start == NO_IRQ) {
			printk(KERN_ERR "ide.%d: no IRQ\n", i);
			goto err0;
		}
		res[2].name = "pata_platform";
		res[2].flags = IORESOURCE_IRQ;

		pdata.ioport_shift = *((u32 *)get_property(np,
					"ioport-shift", NULL));

		pdev = platform_device_alloc("pata_platform", i);
		if (!pdev)
			goto err0;

		ret = platform_device_add_data(pdev, &pdata, sizeof(pdata));
		if (ret)
			goto err1;

		ret = platform_device_add_resources(pdev, res, ARRAY_SIZE(res));
		if (ret)
			goto err1;

		ret = platform_device_register(pdev);
		if (ret)
			goto err1;

		continue;
err1:
		printk(KERN_ERR "ide.%d: registration failed\n", i);
		platform_device_del(pdev); /* it will free everything */
err0:
		/* Even if some device failed, try others */
		continue;
	}

	return 0;
}
device_initcall(ide_of_init);

static struct of_device_id mpc834x_itx_ids[] __initdata = {
	{ .compatible = "soc", },
	{ .compatible = "simple-bus", },
	{},
};

static int __init mpc834x_itx_declare_of_platform_devices(void)
{
	return of_platform_bus_probe(NULL, mpc834x_itx_ids, NULL);
}
machine_device_initcall(mpc834x_itx, mpc834x_itx_declare_of_platform_devices);

/* ************************************************************************
 *
 * Setup the architecture
 *
 */
static void __init mpc834x_itx_setup_arch(void)
{
	struct device_node *np;

	if (ppc_md.progress)
		ppc_md.progress("mpc834x_itx_setup_arch()", 0);

	np = of_find_node_by_type(NULL, "cpu");
	if (np != 0) {
		const unsigned int *fp =
			get_property(np, "clock-frequency", NULL);
		if (fp != 0)
			loops_per_jiffy = *fp / HZ;
		else
			loops_per_jiffy = 50000000 / HZ;
		of_node_put(np);
	}
#ifdef CONFIG_PCI
	for_each_compatible_node(np, "pci", "fsl,mpc8349-pci")
		mpc83xx_add_bridge(np);

	ppc_md.pci_exclude_device = mpc83xx_exclude_device;
#endif

#ifdef  CONFIG_ROOT_NFS
	ROOT_DEV = Root_NFS;
#else
	ROOT_DEV = Root_HDA1;
#endif
}

void __init mpc834x_itx_init_IRQ(void)
{
	struct device_node *np;

	np = of_find_node_by_type(NULL, "ipic");
	if (!np)
		return;

	ipic_init(np, 0);

	/* Initialize the default interrupt mapping priorities,
	 * in case the boot rom changed something on us.
	 */
	ipic_set_default_priority();
}

/*
 * Called very early, MMU is off, device-tree isn't unflattened
 */
static int __init mpc834x_itx_probe(void)
{
	/* We always match for now, eventually we should look at the flat
	   dev tree to ensure this is the board we are suppose to run on
	*/
	return 1;
}

define_machine(mpc834x_itx) {
	.name			= "MPC834x ITX",
	.probe			= mpc834x_itx_probe,
	.setup_arch		= mpc834x_itx_setup_arch,
	.init_IRQ		= mpc834x_itx_init_IRQ,
	.get_irq		= ipic_get_irq,
	.restart		= mpc83xx_restart,
	.time_init		= mpc83xx_time_init,
	.calibrate_decr		= generic_calibrate_decr,
	.progress		= udbg_progress,
};
