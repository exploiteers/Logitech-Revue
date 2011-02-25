/*
 * PPC44x PCI host support
 *
 * Vitaly Bordug <vitb@kernel.crashing.org>
 * Stefan Roese <sr@denx.de>
 *
 * Based on arch/ppc sequoia pci bits, that are
 * Copyright 2006-2007 DENX Software Engineering, Stefan Roese <sr@denx.de>
 *
 * Based on bamboo.c from Wade Farnsworth <wfarnsworth@mvista.com>
 *      Copyright 2004 MontaVista Software Inc.
 *      Copyright 2006 AMCC
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/stddef.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/module.h>
#include <linux/io.h>

#include <mm/mmu_decl.h>

#include <asm/system.h>
#include <asm/atomic.h>
#include <asm/pci-bridge.h>
#include <asm/prom.h>

#include "44x.h"

#undef DEBUG

#ifdef DEBUG
#define DBG(x...) printk(x)
#else
#define DBG(x...)
#endif

#ifdef CONFIG_PCI

int ppc4xx_exclude_device(struct pci_controller *hose, u_char bus,
			   u_char devfn)
{
	if ((bus == hose->first_busno) && PCI_SLOT(devfn) == 0)
		return PCIBIOS_DEVICE_NOT_FOUND;
	return PCIBIOS_SUCCESSFUL;
}

inline void pci_writel(void *pci_reg_base, u32 offset, unsigned int val)
{
	writel(val, pci_reg_base + offset);
}

static void __init ppc4xx_setup_pci(struct pci_controller *hose,
				     void *pci_reg_base)
{
	unsigned long memory_size, pci_size = 0;
	phys_addr_t pci_phys_base = 0;
	phys_addr_t pci_pci_base = 0;
	int i;
	u16 cmd;

	memory_size = total_memory;

	/*
	 * 440EPx has single memory region, we'll use it to configure phb
	 */
	for (i = 0; i < 3; i++)
		if (hose->mem_resources[i].start) {
			pci_phys_base = hose->mem_resources[i].start;
			pci_pci_base = pci_phys_base - hose->pci_mem_offset;
			pci_size =
			    hose->mem_resources[i].end -
			    hose->mem_resources[i].start;
		}

	if (pci_phys_base == 0) {
		printk(KERN_ERR "PHB bridge memory region is not defined!\n");
		return;
	}

	early_read_config_word(hose, 0, 0, PCI_COMMAND, &cmd);
	cmd |= PCI_COMMAND_MASTER | PCI_COMMAND_MEMORY;
	early_write_config_word(hose, 0, 0, PCI_COMMAND, cmd);

	/* Disable region first */
	pci_writel(pci_reg_base, PPC4xx_PCIL0_PMM0MA, 0);

	/* PLB starting addr: 0x0000000180000000
	 * We need just lower word to get the things work
	 */
	pci_writel(pci_reg_base, PPC4xx_PCIL0_PMM0LA,
		   pci_phys_base & 0xFFFFFFFF);

	/* PCI start addr, 0x80000000 (PCI Address) */
	pci_writel(pci_reg_base, PPC4xx_PCIL0_PMM0PCILA,
		   pci_pci_base & 0xFFFFFFFF);
	pci_writel(pci_reg_base, PPC4xx_PCIL0_PMM0PCIHA, 0);

	/* Enable no pre-fetch, enable region */
	pci_writel(pci_reg_base, PPC4xx_PCIL0_PMM0MA,
		   ((0xffffffff - pci_size) | 0x01));

	/* Disable region one */
	pci_writel(pci_reg_base, PPC4xx_PCIL0_PMM1MA, 0);
	pci_writel(pci_reg_base, PPC4xx_PCIL0_PMM1LA, 0);
	pci_writel(pci_reg_base, PPC4xx_PCIL0_PMM1PCILA, 0);
	pci_writel(pci_reg_base, PPC4xx_PCIL0_PMM1PCIHA, 0);

	/* Disable region two */
	pci_writel(pci_reg_base, PPC4xx_PCIL0_PMM1MA, 0);
	pci_writel(pci_reg_base, PPC4xx_PCIL0_PMM1LA, 0);
	pci_writel(pci_reg_base, PPC4xx_PCIL0_PMM1PCILA, 0);
	pci_writel(pci_reg_base, PPC4xx_PCIL0_PMM1PCIHA, 0);

	/* Now configure the PCI->PLB windows, we only use PTM1
	 *
	 * For Inbound flow, set the window size to all available memory
	 * This is required because if size is smaller,
	 * then Eth/PCI DD would fail as PCI card not able to access
	 * the memory allocated by DD.
	 */

	pci_writel(pci_reg_base, PPC4xx_PCIL0_PTM1MS, 0);
	pci_writel(pci_reg_base, PPC4xx_PCIL0_PTM1LA, 0);

	memory_size = 1 << fls(memory_size - 1);

	/* Size low + Enabled */
	pci_writel(pci_reg_base, PPC4xx_PCIL0_PTM1MS,
		   (0xffffffff - (memory_size - 1)) | 0x1);
	eieio();
}

int __init ppc4xx_add_bridge(struct device_node *dev)
{
	int len;
	struct pci_controller *hose;
	const int *bus_range;
	int primary = 1;
	struct resource cfg_res;
	void __iomem *reg;

	/* Fetch host bridge registers address */
	reg = of_iomap(dev, 1);

	if (of_address_to_resource(dev, 0, &cfg_res)) {
		printk(KERN_ERR "Failed to obtain cfg resource\n");
		return -EINVAL;
	}

	DBG("Adding PCI host bridge %s\n reg %p\n", dev->full_name, reg);

	/* Get bus range if any */
	bus_range = of_get_property(dev, "bus-range", &len);
	if (bus_range == NULL || len < 2 * sizeof(int))
		printk(KERN_WARNING "Can't get bus-range for %s, assume"
		       " bus 0\n", dev->full_name);

	pci_assign_all_buses = 1;
	hose = pcibios_alloc_controller(dev);
	if (!hose)
		return -ENOMEM;

	hose->first_busno = bus_range ? bus_range[0] : 0;
	hose->last_busno = bus_range ? bus_range[1] : 0xff;

	setup_indirect_pci(hose, cfg_res.start,
				cfg_res.start + PPC4xx_PCI_CFGD_OFFSET, 0);

	/* Interpret the "ranges" property */
	/* This also maps the I/O region and sets isa_io/mem_base */
	pci_process_bridge_OF_ranges(hose, dev, primary);
	ppc4xx_setup_pci(hose, reg);

	return 0;
}


void ppc4xx_fixup_irq(struct pci_dev *dev)
{
	struct of_irq oirq;

	if (of_irq_map_pci(dev, &oirq)) {
		printk(KERN_ERR "PCI: can't map irq\n");
		return;
	}
	dev->irq = irq_create_of_mapping(oirq.controller, oirq.specifier,
				     oirq.size);
	pci_write_config_byte(dev, PCI_INTERRUPT_LINE, dev->irq);

	DBG("PCI: dev->irq = 0x%x\n", dev->irq);
}
#endif
