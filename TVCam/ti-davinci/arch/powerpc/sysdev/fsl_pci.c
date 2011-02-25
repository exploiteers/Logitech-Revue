/*
 * MPC85xx/86xx PCI/PCIE support routing.
 *
 * Copyright 2007 Freescale Semiconductor, Inc
 *
 * Initial author: Xianghua Xiao <x.xiao@freescale.com>
 * Recode: ZHANG WEI <wei.zhang@freescale.com>
 * Rewrite the routing for Frescale PCI and PCI Express
 * 	Roy Zang <tie-fei.zang@freescale.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/init.h>
#include <linux/bootmem.h>

#include <asm/io.h>
#include <asm/prom.h>
#include <asm/pci-bridge.h>
#include <asm/machdep.h>
#include <sysdev/fsl_soc.h>
#include <sysdev/fsl_pci.h>

/* atmu setup for fsl pci/pcie controller */
void __init setup_pci_atmu(struct pci_controller *hose, struct resource *rsrc)
{
	struct ccsr_pci __iomem *pci;
	int i;

	pr_debug("PCI memory map start 0x%016llx, size 0x%016llx\n",
		    (u64)rsrc->start, (u64)rsrc->end - (u64)rsrc->start + 1);
	pci = ioremap(rsrc->start, rsrc->end - rsrc->start + 1);

	/* Disable all windows (except powar0 since its ignored) */
	for(i = 1; i < 5; i++)
		out_be32(&pci->pow[i].powar, 0);
	for(i = 0; i < 3; i++)
		out_be32(&pci->piw[i].piwar, 0);

	/* Setup outbound MEM window */
	for(i = 0; i < 3; i++)
		if (hose->mem_resources[i].flags & IORESOURCE_MEM){
			resource_size_t pci_addr_start =
				 hose->mem_resources[i].start -
				 hose->pci_mem_offset;
			pr_debug("PCI MEM resource start 0x%016llx, size 0x%016llx.\n",
				(u64)hose->mem_resources[i].start,
				(u64)hose->mem_resources[i].end
				  - (u64)hose->mem_resources[i].start + 1);
			out_be32(&pci->pow[i+1].potar, (pci_addr_start >> 12));
			out_be32(&pci->pow[i+1].potear, 0);
			out_be32(&pci->pow[i+1].powbar,
				(hose->mem_resources[i].start >> 12));
			/* Enable, Mem R/W */
			out_be32(&pci->pow[i+1].powar, 0x80044000
				| (__ilog2(hose->mem_resources[i].end
				- hose->mem_resources[i].start + 1) - 1));
		}

	/* Setup outbound IO window */
	if (hose->io_resource.flags & IORESOURCE_IO){
		pr_debug("PCI IO resource start 0x%016llx, size 0x%016llx, "
			 "phy base 0x%016llx.\n",
			(u64)hose->io_resource.start,
			(u64)hose->io_resource.end - (u64)hose->io_resource.start + 1,
			(u64)hose->io_base_phys);
		out_be32(&pci->pow[i+1].potar, (hose->io_resource.start >> 12));
		out_be32(&pci->pow[i+1].potear, 0);
		out_be32(&pci->pow[i+1].powbar, (hose->io_base_phys >> 12));
		/* Enable, IO R/W */
		out_be32(&pci->pow[i+1].powar, 0x80088000
			| (__ilog2(hose->io_resource.end
			- hose->io_resource.start + 1) - 1));
	}

	/* Setup 2G inbound Memory Window @ 1 */
	out_be32(&pci->piw[2].pitar, 0x00000000);
	out_be32(&pci->piw[2].piwbar,0x00000000);
	out_be32(&pci->piw[2].piwar, PIWAR_2G);
}

void __init setup_pci_cmd(struct pci_controller *hose)
{
	u16 cmd;
	int cap_x;

	early_read_config_word(hose, 0, 0, PCI_COMMAND, &cmd);
	cmd |= PCI_COMMAND_SERR | PCI_COMMAND_MASTER | PCI_COMMAND_MEMORY
		| PCI_COMMAND_IO;
	early_write_config_word(hose, 0, 0, PCI_COMMAND, cmd);

	cap_x = early_find_capability(hose, 0, 0, PCI_CAP_ID_PCIX);
	if (cap_x) {
		int pci_x_cmd = cap_x + PCI_X_CMD;
		cmd = PCI_X_CMD_MAX_SPLIT | PCI_X_CMD_MAX_READ
			| PCI_X_CMD_ERO | PCI_X_CMD_DPERR_E;
		early_write_config_word(hose, 0, 0, pci_x_cmd, cmd);
	} else {
		early_write_config_byte(hose, 0, 0, PCI_LATENCY_TIMER, 0x80);
	}
}

static int fsl_pcie_bus_fixup;
  
static void __init quirk_fsl_pcie_header(struct pci_dev *dev)
{

	/* if we aren't a PCIe don't bother */
	if (!pci_find_capability(dev, PCI_CAP_ID_EXP))
		return ;

	dev->class = PCI_CLASS_BRIDGE_PCI << 8;
	fsl_pcie_bus_fixup = 1;
	return ;
}

int __init fsl_pcie_check_link(struct pci_controller *hose)
{
	u32 val;
	early_read_config_dword(hose, 0, 0, PCIE_LTSSM, &val);
	if (val < PCIE_LTSSM_L0)
		return 1;
	return 0;
}

void fsl_pcibios_fixup_bus(struct pci_bus *bus)
{
	struct pci_controller *hose = (struct pci_controller *) bus->sysdata;
	int i;

	if ((bus->parent == hose->bus) &&
	    ((fsl_pcie_bus_fixup &&
	      early_find_capability(hose, 0, 0, PCI_CAP_ID_EXP)) ||
	     (hose->indirect_type & PPC_INDIRECT_TYPE_NO_PCIE_LINK)))
	{
		for (i = 0; i < 4; ++i) {
			struct resource *res = bus->resource[i];
			struct resource *par = bus->parent->resource[i];
			if (res) {
				res->start = 0;
				res->end   = 0;
				res->flags = 0;
			}
			if (res && par) {
				res->start = par->start;
				res->end   = par->end;
				res->flags = par->flags;
			}
		}
	}
}

int __init fsl_add_bridge(struct device_node *dev, int is_primary)
{
	int len;
	struct pci_controller *hose;
	struct resource rsrc;
	const int *bus_range;

	pr_debug("Adding PCI host bridge %s\n", dev->full_name);

	/* Fetch host bridge registers address */
	if (of_address_to_resource(dev, 0, &rsrc)) {
		printk(KERN_WARNING "Can't get pci register base!");
		return -ENOMEM;
	}

	/* Get bus range if any */
	bus_range = of_get_property(dev, "bus-range", &len);
	if (bus_range == NULL || len < 2 * sizeof(int))
		printk(KERN_WARNING "Can't get bus-range for %s, assume"
			" bus 0\n", dev->full_name);

	ppc_pci_flags |= PPC_PCI_REASSIGN_ALL_BUS;
	hose = pcibios_alloc_controller(dev);
	if (!hose)
		return -ENOMEM;

	hose->first_busno = bus_range ? bus_range[0] : 0x0;
	hose->last_busno = bus_range ? bus_range[1] : 0xff;

	setup_indirect_pci(hose, rsrc.start, rsrc.start + 0x4,
		PPC_INDIRECT_TYPE_BIG_ENDIAN);
	setup_pci_cmd(hose);

	/* check PCI express link status */
	if (early_find_capability(hose, 0, 0, PCI_CAP_ID_EXP)) {
		hose->indirect_type |= PPC_INDIRECT_TYPE_EXT_REG |
			PPC_INDIRECT_TYPE_SURPRESS_PRIMARY_BUS;
		if (fsl_pcie_check_link(hose))
			hose->indirect_type |= PPC_INDIRECT_TYPE_NO_PCIE_LINK;
	}

	printk(KERN_INFO "Found FSL PCI host bridge at 0x%016llx."
		"Firmware bus number: %d->%d\n",
		(unsigned long long)rsrc.start, hose->first_busno,
		hose->last_busno);

	pr_debug(" ->Hose at 0x%p, cfg_addr=0x%p,cfg_data=0x%p\n",
		hose, hose->cfg_addr, hose->cfg_data);

	/* Interpret the "ranges" property */
	/* This also maps the I/O region and sets isa_io/mem_base */
	pci_process_bridge_OF_ranges(hose, dev, is_primary);

	/* Setup PEX window registers */
	setup_pci_atmu(hose, &rsrc);

	return 0;
}

/* MPC83xx PCIE routines*/
/* PCIE Registers */
#define PEX_LTSSM_STAT          0x404
#define PEX_LTSSM_STAT_L0       0x16
#define PEX_GCLK_RATIO          0x440

/* With the convention of u-boot, the PCIE outbound window 0 serves
 * as configuration transactions outbound */
#define PEX_OUTWIN0_TAL		0xCA8
#define PEX_OUTWIN0_TAH		0xCAC

void remap_cfg_outbound(void * __iomem reg_base, u32 tal, u32 tah)
{
	out_le32(reg_base + PEX_OUTWIN0_TAL, tal);
	out_le32(reg_base + PEX_OUTWIN0_TAH, tah);
}

static int mpc83xx_read_config_pcie(struct pci_bus *bus,
			uint devfn, int offset, int len, u32 *val)
{
	struct pci_controller *hose = bus->sysdata;
	void __iomem *cfg_addr;
	static u32 orig_busno = 0;
	u32 bus_no;

	if (hose->indirect_type & PPC_INDIRECT_TYPE_NO_PCIE_LINK)
		return PCIBIOS_DEVICE_NOT_FOUND;

	if (ppc_md.pci_exclude_device)
		if (ppc_md.pci_exclude_device(hose, bus->number, devfn))
			return PCIBIOS_DEVICE_NOT_FOUND;

	switch (len) {
	case 2:
		if (offset & 1)
			return -EINVAL;
		break;
	case 4:
	if (offset & 3)
		return -EINVAL;
		break;
	}

	if ((bus->number == hose->first_busno) &&
		(hose->indirect_type & PPC_INDIRECT_TYPE_MPC83XX_PCIE))
		cfg_addr = (void __iomem *)((ulong) hose->cfg_data + (offset & 0xfff));
	else {
		bus_no = bus->number - hose->first_busno;
		if (bus_no != orig_busno) {
			remap_cfg_outbound((void __iomem *)hose->cfg_data, bus_no, 0);
			orig_busno = bus_no;
		}
		cfg_addr = (void __iomem *)((ulong) hose->cfg_addr +
			((devfn << 16) | (offset & 0xfff)));
	}

	switch (len) {
	case 1:
		*val = in_8(cfg_addr);
		break;
	case 2:
		*val = in_le16(cfg_addr);
		break;
	default:
		*val = in_le32(cfg_addr);
		break;
	}

	return PCIBIOS_SUCCESSFUL;
}

static int mpc83xx_write_config_pcie(struct pci_bus *bus,
			uint devfn, int offset, int len, u32 val)
{
	struct pci_controller *hose = bus->sysdata;
	void __iomem *cfg_addr;
	static u32 orig_busno = 0;
	u32 bus_no;

	if (hose->indirect_type & PPC_INDIRECT_TYPE_NO_PCIE_LINK)
		return PCIBIOS_DEVICE_NOT_FOUND;

	if (ppc_md.pci_exclude_device)
		if (ppc_md.pci_exclude_device(hose, bus->number, devfn))
			return PCIBIOS_DEVICE_NOT_FOUND;

	switch (len) {
	case 2:
		if (offset & 1)
			return -EINVAL;
		break;
	case 4:
		if (offset & 3)
			return -EINVAL;
		break;
	}


	if ((bus->number == hose->first_busno) &&
		(hose->indirect_type & PPC_INDIRECT_TYPE_MPC83XX_PCIE))
		cfg_addr = (void __iomem *)((ulong) hose->cfg_data + (offset & 0xfff));
	else {
		bus_no = bus->number - hose->first_busno;
		if (bus_no != orig_busno) {
			remap_cfg_outbound((void __iomem *)hose->cfg_data, bus_no, 0);
			orig_busno = bus_no;
		}
		cfg_addr = (void __iomem *)((ulong) hose->cfg_addr +
			((devfn << 16) | (offset & 0xfff)));
	}

	switch (len) {
	case 1:
		out_8(cfg_addr, val);
		break;
	case 2:
		out_le16(cfg_addr, val);
		break;
	default:
		out_le32(cfg_addr, val);
		break;
	}

	return PCIBIOS_SUCCESSFUL;
}

static struct pci_ops mpc83xx_pcie_ops = {
	mpc83xx_read_config_pcie,
	mpc83xx_write_config_pcie
};

void __init mpc83xx_setup_pcie(struct pci_controller *hose,
			struct resource *reg, struct resource *cfg_space)
{
	void __iomem *hose_cfg_header, *mbase;
	u32 val;

	hose_cfg_header = ioremap(reg->start, reg->end - reg->start + 1);

	val = in_le32(hose_cfg_header + PEX_LTSSM_STAT);
	if (val < PEX_LTSSM_STAT_L0)
		hose->indirect_type |= PPC_INDIRECT_TYPE_NO_PCIE_LINK;
	hose->indirect_type |= PPC_INDIRECT_TYPE_MPC83XX_PCIE;

	mbase = ioremap(cfg_space->start & PAGE_MASK, cfg_space->end - cfg_space->start + 1);
	hose->ops = &mpc83xx_pcie_ops;
	hose->cfg_addr = mbase + (cfg_space->start & ~PAGE_MASK);

	/* The MPC83xx PCIE implements direct access configure space
	 * routines instead of indirect ones. So, the cfg_data field is free.
	 * The MPC83xx PCIE RC configure header is memory-mapped,
	 * we use cfg_data as this header pointer */
	hose->cfg_data = hose_cfg_header;
}

DECLARE_PCI_FIXUP_HEADER(0x1957, PCI_DEVICE_ID_MPC8315E, quirk_fsl_pcie_header);
DECLARE_PCI_FIXUP_HEADER(0x1957, PCI_DEVICE_ID_MPC8377E, quirk_fsl_pcie_header);
DECLARE_PCI_FIXUP_HEADER(0x1957, PCI_DEVICE_ID_MPC8377, quirk_fsl_pcie_header);
DECLARE_PCI_FIXUP_HEADER(0x1957, PCI_DEVICE_ID_MPC8378E, quirk_fsl_pcie_header);
DECLARE_PCI_FIXUP_HEADER(0x1957, PCI_DEVICE_ID_MPC8378, quirk_fsl_pcie_header);
DECLARE_PCI_FIXUP_HEADER(0x1957, PCI_DEVICE_ID_MPC8548E, quirk_fsl_pcie_header);
DECLARE_PCI_FIXUP_HEADER(0x1957, PCI_DEVICE_ID_MPC8548, quirk_fsl_pcie_header);
DECLARE_PCI_FIXUP_HEADER(0x1957, PCI_DEVICE_ID_MPC8543E, quirk_fsl_pcie_header);
DECLARE_PCI_FIXUP_HEADER(0x1957, PCI_DEVICE_ID_MPC8543, quirk_fsl_pcie_header);
DECLARE_PCI_FIXUP_HEADER(0x1957, PCI_DEVICE_ID_MPC8547E, quirk_fsl_pcie_header);
DECLARE_PCI_FIXUP_HEADER(0x1957, PCI_DEVICE_ID_MPC8545E, quirk_fsl_pcie_header);
DECLARE_PCI_FIXUP_HEADER(0x1957, PCI_DEVICE_ID_MPC8545, quirk_fsl_pcie_header);
DECLARE_PCI_FIXUP_HEADER(0x1957, PCI_DEVICE_ID_MPC8568E, quirk_fsl_pcie_header);
DECLARE_PCI_FIXUP_HEADER(0x1957, PCI_DEVICE_ID_MPC8568, quirk_fsl_pcie_header);
DECLARE_PCI_FIXUP_HEADER(0x1957, PCI_DEVICE_ID_MPC8567E, quirk_fsl_pcie_header);
DECLARE_PCI_FIXUP_HEADER(0x1957, PCI_DEVICE_ID_MPC8567, quirk_fsl_pcie_header);
DECLARE_PCI_FIXUP_HEADER(0x1957, PCI_DEVICE_ID_MPC8533E, quirk_fsl_pcie_header);
DECLARE_PCI_FIXUP_HEADER(0x1957, PCI_DEVICE_ID_MPC8533, quirk_fsl_pcie_header);
DECLARE_PCI_FIXUP_HEADER(0x1957, PCI_DEVICE_ID_MPC8544E, quirk_fsl_pcie_header);
DECLARE_PCI_FIXUP_HEADER(0x1957, PCI_DEVICE_ID_MPC8544, quirk_fsl_pcie_header);
DECLARE_PCI_FIXUP_HEADER(0x1957, PCI_DEVICE_ID_MPC8572E, quirk_fsl_pcie_header);
DECLARE_PCI_FIXUP_HEADER(0x1957, PCI_DEVICE_ID_MPC8572, quirk_fsl_pcie_header);
DECLARE_PCI_FIXUP_HEADER(0x1957, PCI_DEVICE_ID_MPC8536E, quirk_fsl_pcie_header);
DECLARE_PCI_FIXUP_HEADER(0x1957, PCI_DEVICE_ID_MPC8536, quirk_fsl_pcie_header);
DECLARE_PCI_FIXUP_HEADER(0x1957, PCI_DEVICE_ID_MPC8641, quirk_fsl_pcie_header);
DECLARE_PCI_FIXUP_HEADER(0x1957, PCI_DEVICE_ID_MPC8641D, quirk_fsl_pcie_header);
DECLARE_PCI_FIXUP_HEADER(0x1957, PCI_DEVICE_ID_MPC8610, quirk_fsl_pcie_header);
