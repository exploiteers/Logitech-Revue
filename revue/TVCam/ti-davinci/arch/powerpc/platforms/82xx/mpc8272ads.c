/*
 * MPC8272ADS board support
 *
 * Copyright 2007 Freescale Semiconductor, Inc.
 * Author: Scott Wood <scottwood@freescale.com>
 *
 * Based on code by Vitaly Bordug <vbordug@ru.mvista.com>
 * Copyright (c) 2006 MontaVista Software, Inc.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */


#include <linux/config.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/fsl_devices.h>

#include <asm/io.h>
#include <asm/cpm2.h>
#include <asm/udbg.h>
#include <asm/of_platform.h>
#include <asm/machdep.h>
#include <asm/time.h>
 
#include <platforms/82xx/pq2.h>

#include <sysdev/fsl_soc.h>
#include <sysdev/cpm2_pic.h>

#include "pq2ads.h"
#include "pq2.h"

static void __init mpc8272ads_pic_init(void)
{
	struct device_node *np = of_find_compatible_node(NULL, NULL, "fsl,pq2-pic");
	if (!np) {
		printk(KERN_ERR "PIC init: can not find cpm-pic node\n");
		return;
	}

	cpm2_pic_init(np);
	of_node_put(np);

	/* Initialize stuff for the 82xx CPLD IC and install demux  */
	pq2ads_pci_init_irq();
}

struct cpm_pin {
	int port, pin, flags;
};
  
static struct cpm_pin mpc8272ads_pins[] = {
	/* SCC1 */
	{3, 30, CPM_PIN_OUTPUT | CPM_PIN_SECONDARY},
	{3, 31, CPM_PIN_INPUT | CPM_PIN_PRIMARY},

	/* SCC4 */
	{3, 21, CPM_PIN_OUTPUT | CPM_PIN_PRIMARY},
	{3, 22, CPM_PIN_INPUT | CPM_PIN_PRIMARY},

	/* USB */
	{2, 10, CPM_PIN_INPUT | CPM_PIN_PRIMARY},
	{2, 11, CPM_PIN_INPUT | CPM_PIN_PRIMARY},
	{2, 20, CPM_PIN_OUTPUT | CPM_PIN_PRIMARY},
	{2, 24, CPM_PIN_INPUT | CPM_PIN_PRIMARY},
	{3, 23, CPM_PIN_OUTPUT | CPM_PIN_PRIMARY},
	{3, 24, CPM_PIN_OUTPUT | CPM_PIN_PRIMARY},
	{3, 25, CPM_PIN_INPUT | CPM_PIN_PRIMARY},

	/* FCC1 */
	{0, 14, CPM_PIN_INPUT | CPM_PIN_PRIMARY},
	{0, 15, CPM_PIN_INPUT | CPM_PIN_PRIMARY},
	{0, 16, CPM_PIN_INPUT | CPM_PIN_PRIMARY},
	{0, 17, CPM_PIN_INPUT | CPM_PIN_PRIMARY},
	{0, 18, CPM_PIN_OUTPUT | CPM_PIN_PRIMARY},
	{0, 19, CPM_PIN_OUTPUT | CPM_PIN_PRIMARY},
	{0, 20, CPM_PIN_OUTPUT | CPM_PIN_PRIMARY},
	{0, 21, CPM_PIN_OUTPUT | CPM_PIN_PRIMARY},
	{0, 26, CPM_PIN_INPUT | CPM_PIN_SECONDARY},
	{0, 27, CPM_PIN_INPUT | CPM_PIN_SECONDARY},
	{0, 28, CPM_PIN_OUTPUT | CPM_PIN_SECONDARY},
	{0, 29, CPM_PIN_OUTPUT | CPM_PIN_SECONDARY},
	{0, 30, CPM_PIN_INPUT | CPM_PIN_SECONDARY},
	{0, 31, CPM_PIN_INPUT | CPM_PIN_SECONDARY},
	{2, 21, CPM_PIN_INPUT | CPM_PIN_PRIMARY},
	{2, 22, CPM_PIN_INPUT | CPM_PIN_PRIMARY},

	/* FCC2 */
	{1, 18, CPM_PIN_INPUT | CPM_PIN_PRIMARY},
	{1, 19, CPM_PIN_INPUT | CPM_PIN_PRIMARY},
	{1, 20, CPM_PIN_INPUT | CPM_PIN_PRIMARY},
	{1, 21, CPM_PIN_INPUT | CPM_PIN_PRIMARY},
	{1, 22, CPM_PIN_OUTPUT | CPM_PIN_PRIMARY},
	{1, 23, CPM_PIN_OUTPUT | CPM_PIN_PRIMARY},
	{1, 24, CPM_PIN_OUTPUT | CPM_PIN_PRIMARY},
	{1, 25, CPM_PIN_OUTPUT | CPM_PIN_PRIMARY},
	{1, 26, CPM_PIN_INPUT | CPM_PIN_PRIMARY},
	{1, 27, CPM_PIN_INPUT | CPM_PIN_PRIMARY},
	{1, 28, CPM_PIN_INPUT | CPM_PIN_PRIMARY},
	{1, 29, CPM_PIN_OUTPUT | CPM_PIN_SECONDARY},
	{1, 30, CPM_PIN_INPUT | CPM_PIN_PRIMARY},
	{1, 31, CPM_PIN_OUTPUT | CPM_PIN_PRIMARY},
	{2, 16, CPM_PIN_INPUT | CPM_PIN_PRIMARY},
	{2, 17, CPM_PIN_INPUT | CPM_PIN_PRIMARY},
};
  
void __init init_ioports(void)
{
	int i;
  
	for (i = 0; i < ARRAY_SIZE(mpc8272ads_pins); i++) {
		struct cpm_pin *pin = &mpc8272ads_pins[i];
		cpm2_set_pin(pin->port, pin->pin, pin->flags);
	}
  
	cpm2_clk_setup(CPM_CLK_SCC1, CPM_BRG1, CPM_CLK_RX);
	cpm2_clk_setup(CPM_CLK_SCC1, CPM_BRG1, CPM_CLK_TX);
	cpm2_clk_setup(CPM_CLK_SCC4, CPM_BRG4, CPM_CLK_RX);
	cpm2_clk_setup(CPM_CLK_SCC4, CPM_BRG4, CPM_CLK_TX);
	cpm2_clk_setup(CPM_CLK_SCC3, CPM_CLK8, CPM_CLK_RX);
	cpm2_clk_setup(CPM_CLK_SCC3, CPM_CLK8, CPM_CLK_TX);
	cpm2_clk_setup(CPM_CLK_FCC1, CPM_CLK11, CPM_CLK_RX);
	cpm2_clk_setup(CPM_CLK_FCC1, CPM_CLK10, CPM_CLK_TX);
	cpm2_clk_setup(CPM_CLK_FCC2, CPM_CLK15, CPM_CLK_RX);
	cpm2_clk_setup(CPM_CLK_FCC2, CPM_CLK16, CPM_CLK_TX);
  }
  
static void __init mpc8272ads_setup_arch(void)
  {
  	struct device_node *np;
  	u32 *bcsr;
  
	if (ppc_md.progress)
		ppc_md.progress("mpc8272ads_setup_arch()", 0);
  
	cpm2_reset();
  
	np = of_find_compatible_node(NULL, NULL, "fsl,mpc8272ads-bcsr");
	if (!np) {
		printk(KERN_ERR "No bcsr in device tree\n");
  		return;
  	}
  
	bcsr = of_iomap(np, 0);
	if (!bcsr) {
		printk(KERN_INFO "Cannot map BCSR registers\n");
  		return;
  	}
  
  	of_node_put(np);
  
	clrbits32(&bcsr[1], BCSR1_RS232_EN1 | BCSR1_RS232_EN2 | BCSR1_FETHIEN);
	setbits32(&bcsr[1], BCSR1_FETH_RST);
  
	clrbits32(&bcsr[3], BCSR3_FETHIEN2 | BCSR3_USB_EN |
			    BCSR3_USB_HI_SPEED | BCSR3_USBVCC0);
	setbits32(&bcsr[3], BCSR3_FETH2_RST);
  
  	iounmap(bcsr);
  
	init_ioports();

 	pq2_init_pci();

	if (ppc_md.progress)
		ppc_md.progress("mpc8272ads_setup_arch(), finish", 0);
}

static struct of_device_id __initdata of_bus_ids[] = {
	{ .name = "soc", },
	{ .name = "cpm", },
	{ .name = "localbus", },
	{},
};

static int __init declare_of_platform_devices(void)
{
	if (!machine_is(mpc8272ads))
		return 0;

	/* Publish the QE devices */
	of_platform_bus_probe(NULL, of_bus_ids, NULL);
	return 0;
}
device_initcall(declare_of_platform_devices);

/*
 * Called very early, device-tree isn't unflattened
 */
static int __init mpc8272ads_probe(void)
{
	unsigned long root = of_get_flat_dt_root();
	return of_flat_dt_is_compatible(root, "fsl,mpc8272ads");
}

define_machine(mpc8272ads)
{
	.name = "Freescale MPC8272ADS",
	.probe = mpc8272ads_probe,
	.setup_arch = mpc8272ads_setup_arch,
	.init_IRQ = mpc8272ads_pic_init,
	.show_cpuinfo = m82xx_show_cpuinfo,
	.get_irq = cpm2_get_irq,
	.calibrate_decr = generic_calibrate_decr,
	.restart = pq2_restart,
	.halt = pq2_halt,
	.progress = udbg_progress,
};

#ifdef CONFIG_PPC_EARLY_DEBUG_CPM
u32 __iomem *cpm_udbg_txdesc = (u32 __iomem *)0xf0000808;
#endif
