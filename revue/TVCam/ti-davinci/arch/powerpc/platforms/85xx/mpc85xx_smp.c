/*
 * Copyright (C) 2006,2007 Freescale Semiconductor, Inc. All rights reserved.
 *
 * Author: Xianghua Xiao, x.xiao@freescale.com
 *         Zhang Wei, wei.zhang@freescale.com
 *         Andy Fleming, afleming@freescale.com
 *
 * Description:
 * MPC85xx SMP setup file
 *
 * This file is part of the Linux kernel
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/stddef.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>

#include <asm/pgtable.h>
#include <asm/page.h>
#include <asm/pci-bridge.h>
#include <asm-powerpc/mpic.h>
#include <asm/cacheflush.h>

#include <sysdev/fsl_soc.h>

extern void __secondary_start_page(void);
extern volatile unsigned long __secondary_hold_acknowledge;

#define EEBPCR_CPU1_EN	0x02000000
#define EEBPCR_CPU0_EN	0x01000000
#define MPC85xx_ECM_OFFSET 0x1000
#define MPC85xx_ECM_SIZE 0xf00
#define ECM_PORT_CONFIG_OFFSET 0x0010
#define MPC85xx_BPTR_OFFSET 0x20
#define BPTR_EN	0x80000000


static void __init
smp_85xx_release_core(int nr)
{
	__iomem u32 *ecm_vaddr;
	unsigned long pcr;

	/*
	 * Startup Core #nr.
	 */
	ecm_vaddr = ioremap(get_immrbase() + MPC85xx_ECM_OFFSET,
			    MPC85xx_ECM_SIZE);
	pcr = in_be32(ecm_vaddr + (ECM_PORT_CONFIG_OFFSET >> 2));
	pcr |= EEBPCR_CPU1_EN;
	out_be32(ecm_vaddr + (ECM_PORT_CONFIG_OFFSET >> 2), pcr);
}


static void __init
smp_85xx_kick_cpu(int nr)
{
	unsigned long flags;
	u32 bptr, oldbptr;
	__iomem u32 *bptr_vaddr;
	int n = 0;

	WARN_ON (nr < 0 || nr >= NR_CPUS);

	pr_debug("smp_85xx_kick_cpu: kick CPU #%d\n", nr);

	local_irq_save(flags);

	/* Get the BPTR */
	bptr_vaddr = ioremap(get_immrbase() + MPC85xx_BPTR_OFFSET, 4);

	/* Set the BPTR to the secondary boot page */
	oldbptr = in_be32(bptr_vaddr);
	bptr = (BPTR_EN | (__pa((unsigned)__secondary_start_page) >> 12));
	out_be32(bptr_vaddr, bptr);

	/* Kick that CPU */
	smp_85xx_release_core(nr);

	/* Wait a bit for the CPU to take the exception. */
	while ((__secondary_hold_acknowledge != nr) && (++n < 1000))
		mdelay(1);

	/* Restore the BPTR */
	out_be32(bptr_vaddr, oldbptr);

	local_irq_restore(flags);

	pr_debug("waited %d msecs for CPU #%d.\n", nr, n);
}


static void __init
smp_85xx_setup_cpu(int cpu_nr)
{
	mpic_setup_this_cpu();
}


struct smp_ops_t smp_85xx_ops = {
	.message_pass = smp_mpic_message_pass,
	.probe = smp_mpic_probe,
	.kick_cpu = smp_85xx_kick_cpu,
	.setup_cpu = smp_85xx_setup_cpu,
	.take_timebase = smp_generic_take_timebase,
	.give_timebase = smp_generic_give_timebase,
};


void __init
mpc85xx_smp_init(void)
{
	smp_ops = &smp_85xx_ops;
}
