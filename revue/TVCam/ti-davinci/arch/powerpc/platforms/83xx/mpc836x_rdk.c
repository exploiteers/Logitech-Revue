/*
 * MPC8360E-RDK board file.
 *
 * Copyright (c) 2006  Freescale Semicondutor, Inc.
 * Copyright (c) 2007  MontaVista Software, Inc.
 * 		       Anton Vorontsov <avorontsov@ru.mvista.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/spi/spi.h>
#include <linux/spi/ads7846.h>
#include <asm/of_platform.h>
#include <asm/prom.h>
#include <asm/time.h>
#include <asm/ipic.h>
#include <asm/udbg.h>
#include <asm/io.h>
#include <asm/qe.h>
#include <asm/qe_ic.h>
#include <asm/gpio.h>
#include <sysdev/fsl_soc.h>

#include "mpc83xx.h"

int ad7843_cs_gpio;
int ad7843_ps_gpio;

static void mpc83xx_spi_activate_cs(u8 cs, u8 polarity)
{
	gpio_set_value(ad7843_cs_gpio, polarity);
}

static void mpc83xx_spi_deactivate_cs(u8 cs, u8 polarity)
{
	gpio_set_value(ad7843_cs_gpio, !polarity);
}

static int ads7846_get_pen_state(void)
{
	return gpio_get_value(ad7843_ps_gpio);
}

static struct ads7846_platform_data ads7846_pdata = {
	.model = 7843,
	.get_pendown_state = ads7846_get_pen_state,
};

static struct spi_board_info mpc836x_spi_boardinfo = {
	.bus_num = 0x4c0,
	.chip_select = 0,
	.max_speed_hz = 100000 * 26,
	.modalias = "ads7846",
	.platform_data = &ads7846_pdata,
};

static int __init mpc836x_spi_init(void)
{
	struct device_node *np;
	struct device_node *spi;
	struct resource irq;
	const u32 *vref;
	int size;
	int ret = -EINVAL;

	np = of_find_compatible_node(NULL, NULL, "ad,AD7843");
	if (!np)
		goto err;

	spi = of_get_parent(np);
	if (!spi)
		goto err1;

	ad7843_cs_gpio = of_get_gpio(spi, 0);
	if (ad7843_cs_gpio < 0)
		goto err2;

	ad7843_ps_gpio = of_get_gpio(np, 0);
	if (ad7843_cs_gpio < 0)
		goto err2;

	ret = gpio_request(ad7843_cs_gpio, "ad7843_cs");
	if (ret)
		goto err2;

	ret = gpio_request(ad7843_ps_gpio, "ad7843_ps");
	if (ret)
		goto err3;

	ret = of_irq_to_resource(np, 0, &irq);
	if (ret == NO_IRQ) {
		ret = -EINVAL;
		goto err4;
	} else {
		mpc836x_spi_boardinfo.irq = irq.start;
	}

	vref = of_get_property(np, "vref", &size);
	if (vref && size == sizeof(*vref))
		ads7846_pdata.vref_mv = *vref;

	gpio_direction_output(ad7843_cs_gpio, 1);
	gpio_direction_input(ad7843_ps_gpio);

	ret = fsl_spi_init(&mpc836x_spi_boardinfo, 1,
			   mpc83xx_spi_activate_cs,
			   mpc83xx_spi_deactivate_cs);

	if (ret)
		goto err4;
	else
		goto err2;

err4:
	gpio_free(ad7843_ps_gpio);
err3:
	gpio_free(ad7843_cs_gpio);
err2:
	of_node_put(spi);
err1:
	of_node_put(np);
err:
	return ret;
}
machine_device_initcall(mpc836x_rdk, mpc836x_spi_init);

static struct of_device_id __initdata mpc836x_rdk_ids[] = {
	{ .compatible = "simple-bus", },
	{},
};

static int __init mpc836x_rdk_declare_of_platform_devices(void)
{
	return of_platform_bus_probe(NULL, mpc836x_rdk_ids, NULL);
}
machine_device_initcall(mpc836x_rdk, mpc836x_rdk_declare_of_platform_devices);

static void __init mpc836x_rdk_setup_arch(void)
{
#ifdef CONFIG_PCI
	struct device_node *np;
#endif

	if (ppc_md.progress)
		ppc_md.progress("mpc836x_rdk_setup_arch()", 0);

#ifdef CONFIG_PCI
	for_each_compatible_node(np, "pci", "fsl,mpc8349-pci")
		mpc83xx_add_bridge(np);
#endif

#ifdef CONFIG_QUICC_ENGINE
	qe_reset();
#endif
}

static void __init mpc836x_rdk_init_IRQ(void)
{
	struct device_node *np;

	np = of_find_compatible_node(NULL, NULL, "fsl,ipic");
	if (!np)
		return;

	ipic_init(np, 0);

	/*
	 * Initialize the default interrupt mapping priorities,
	 * in case the boot rom changed something on us.
	 */
	ipic_set_default_priority();
	of_node_put(np);

#ifdef CONFIG_QUICC_ENGINE
	np = of_find_compatible_node(NULL, NULL, "fsl,qe-ic");
	if (!np)
		return;

	qe_ic_init(np, 0, qe_ic_cascade_low_ipic, qe_ic_cascade_high_ipic);
	of_node_put(np);
#endif
}

/*
 * Called very early, MMU is off, device-tree isn't unflattened.
 */
static int __init mpc836x_rdk_probe(void)
{
	unsigned long root = of_get_flat_dt_root();

	return of_flat_dt_is_compatible(root, "fsl,mpc8360erdk");
}

define_machine(mpc836x_rdk) {
	.name		= "MPC836x RDK",
	.probe		= mpc836x_rdk_probe,
	.setup_arch	= mpc836x_rdk_setup_arch,
	.init_IRQ	= mpc836x_rdk_init_IRQ,
	.get_irq	= ipic_get_irq,
	.restart	= mpc83xx_restart,
	.time_init	= mpc83xx_time_init,
	.calibrate_decr	= generic_calibrate_decr,
	.progress	= udbg_progress,
};
