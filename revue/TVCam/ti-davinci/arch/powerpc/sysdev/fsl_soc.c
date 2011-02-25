/*
 * FSL SoC setup code
 *
 * Maintained by Kumar Gala (see MAINTAINERS for contact information)
 *
 * 2006 (c) MontaVista Software, Inc.
 * Vitaly Bordug <vbordug@ru.mvista.com>
 *
 * Copyright (C) Freescale Semiconductor, Inc. 2006. All rights reserved.
 * 2006: Lo Wilson (r43300@freescale.com)
 *	     Added support for Enhanced Local Bus Controller
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/stddef.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/major.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/phy.h>
#include <linux/spi/spi.h>
#include <linux/fsl_devices.h>
#include <linux/fs_enet_pd.h>
#include <linux/fs_uart_pd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/fsl_elbc.h>

#include <asm/system.h>
#include <asm/atomic.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/time.h>
#include <asm/prom.h>
#include <asm/of_platform.h>
#include <sysdev/fsl_soc.h>
#include <mm/mmu_decl.h>
#include <asm/cpm2.h>

extern void init_fcc_ioports(struct fs_platform_info*);
extern void init_fec_ioports(struct fs_platform_info*);
extern void init_smc_ioports(struct fs_uart_platform_info*);
#ifdef CONFIG_FSL_BOOKE
extern void abort(void);
#endif
phys_addr_t immrbase = -1;

phys_addr_t get_immrbase(void)
{
	struct device_node *soc;

	if (immrbase != -1)
		return immrbase;

	soc = of_find_node_by_type(NULL, "soc");
	if (soc) {
		int size;
		const void *prop = of_get_property(soc, "reg", &size);

		if (prop)
			immrbase = of_translate_address(soc, prop);
		of_node_put(soc);
	}

	return immrbase;
}

EXPORT_SYMBOL(get_immrbase);

static u32 sysfreq = -1;

u32 fsl_get_sys_freq(void)
{
	struct device_node *soc;
	const u32 *prop;
	int size;

	if (sysfreq != -1)
		return sysfreq;

	soc = of_find_node_by_type(NULL, "soc");
	if (!soc)
		return -1;

	prop = of_get_property(soc, "clock-frequency", &size);
	if (!prop || size != sizeof(*prop) || *prop == 0)
		prop = of_get_property(soc, "bus-frequency", &size);

	if (prop && size == sizeof(*prop))
		sysfreq = *prop;

	of_node_put(soc);
	return sysfreq;
}
EXPORT_SYMBOL(fsl_get_sys_freq);

#if defined(CONFIG_CPM2) || defined(CONFIG_QUICC_ENGINE) || defined(CONFIG_8xx)

u32 brgfreq = -1;

u32 get_brgfreq(void)
{
	struct device_node *node;
	const unsigned int *prop;
	int size;

	if (brgfreq != -1)
		return brgfreq;

	node = of_find_compatible_node(NULL, NULL, "fsl,cpm-brg");
	if (node) {
		prop = of_get_property(node, "clock-frequency", &size);
		if (prop && size == 4)
			brgfreq = *prop;

		of_node_put(node);
		return brgfreq;
	}

	/* Legacy device binding -- will go away when no users are left. */
	node = of_find_node_by_type(NULL, "cpm");
	if (!node)
		node = of_find_compatible_node(NULL, NULL, "fsl,qe");
	if (!node)
		node = of_find_node_by_type(NULL, "qe");

	if (node) {
		prop = of_get_property(node, "brg-frequency", &size);
		if (prop && size == 4)
			brgfreq = *prop;

		if (brgfreq == -1 || brgfreq == 0) {
			prop = of_get_property(node, "bus-frequency", &size);
			if (prop && size == 4)
				brgfreq = *prop / 2;
		}
		of_node_put(node);
	}

	return brgfreq;
}

EXPORT_SYMBOL(get_brgfreq);

u32 fs_baudrate = -1;

u32 get_baudrate(void)
{
	struct device_node *node;

	if (fs_baudrate != -1)
		return fs_baudrate;

	node = of_find_node_by_type(NULL, "serial");
	if (node) {
		int size;
		const unsigned int *prop = of_get_property(node,
				"current-speed", &size);

		if (prop)
			fs_baudrate = *prop;
		of_node_put(node);
	}

	return fs_baudrate;
}

EXPORT_SYMBOL(get_baudrate);
#endif /* CONFIG_CPM2 */

static int __init gfar_mdio_of_init(void)
{
	struct device_node *np = NULL;
	struct platform_device *mdio_dev;
	struct resource res;
	int ret;

	np = of_find_compatible_node(np, NULL, "fsl,gianfar-mdio");

	/* try the deprecated version */
	if (!np)
		np = of_find_compatible_node(np, "mdio", "gianfar");

	if (np) {
		int k;
		struct device_node *child = NULL;
		struct gianfar_mdio_data mdio_data;

		memset(&res, 0, sizeof(res));
		memset(&mdio_data, 0, sizeof(mdio_data));

		ret = of_address_to_resource(np, 0, &res);
		if (ret)
			goto err;

		mdio_dev =
		    platform_device_register_simple("fsl-gianfar_mdio",
						    res.start, &res, 1);
		if (IS_ERR(mdio_dev)) {
			ret = PTR_ERR(mdio_dev);
			goto err;
		}

		for (k = 0; k < 32; k++)
			mdio_data.irq[k] = PHY_POLL;

		while ((child = of_get_next_child(np, child)) != NULL) {
			int irq = irq_of_parse_and_map(child, 0);
			if (irq != NO_IRQ) {
				const u32 *id = of_get_property(child,
							"reg", NULL);
				mdio_data.irq[*id] = irq;
			}
		}

		ret =
		    platform_device_add_data(mdio_dev, &mdio_data,
					     sizeof(struct gianfar_mdio_data));
		if (ret)
			goto unreg;
	}

	of_node_put(np);
	return 0;

unreg:
	platform_device_unregister(mdio_dev);
err:
	of_node_put(np);
	return ret;
}

arch_initcall(gfar_mdio_of_init);

static const char *gfar_tx_intr = "tx";
static const char *gfar_rx_intr = "rx";
static const char *gfar_err_intr = "error";


static int __init gfar_of_init(void)
{
	struct device_node *np;
	unsigned int i;
	struct platform_device *gfar_dev;
	struct resource res;
	int ret;

	for (np = NULL, i = 0;
	     (np = of_find_compatible_node(np, "network", "gianfar")) != NULL;
	     i++) {
		struct resource r[4];
		struct device_node *phy, *mdio;
		struct gianfar_platform_data gfar_data;
		const unsigned int *id;
		const char *model;
		const void *mac_addr;
		const phandle *ph;
		int n_res = 2;

		memset(r, 0, sizeof(r));
		memset(&gfar_data, 0, sizeof(gfar_data));

		ret = of_address_to_resource(np, 0, &r[0]);
		if (ret)
			goto err;

		of_irq_to_resource(np, 0, &r[1]);

		model = of_get_property(np, "model", NULL);

		/* If we aren't the FEC we have multiple interrupts */
		if (model && strcasecmp(model, "FEC")) {
			r[1].name = gfar_tx_intr;

			r[2].name = gfar_rx_intr;
			of_irq_to_resource(np, 1, &r[2]);

			r[3].name = gfar_err_intr;
			of_irq_to_resource(np, 2, &r[3]);

			n_res += 2;
		}

		gfar_dev =
		    platform_device_register_simple("fsl-gianfar", i, &r[0],
						    n_res);

		if (IS_ERR(gfar_dev)) {
			ret = PTR_ERR(gfar_dev);
			goto err;
		}

		mac_addr = of_get_mac_address(np);
		if (mac_addr)
			memcpy(gfar_data.mac_addr, mac_addr, 6);

		if (model && !strcasecmp(model, "TSEC"))
			gfar_data.device_flags =
			    FSL_GIANFAR_DEV_HAS_GIGABIT |
			    FSL_GIANFAR_DEV_HAS_COALESCE |
			    FSL_GIANFAR_DEV_HAS_RMON |
			    FSL_GIANFAR_DEV_HAS_MULTI_INTR;
		if (model && !strcasecmp(model, "eTSEC"))
			gfar_data.device_flags =
			    FSL_GIANFAR_DEV_HAS_GIGABIT |
			    FSL_GIANFAR_DEV_HAS_COALESCE |
			    FSL_GIANFAR_DEV_HAS_RMON |
			    FSL_GIANFAR_DEV_HAS_MULTI_INTR |
			    FSL_GIANFAR_DEV_HAS_CSUM |
			    FSL_GIANFAR_DEV_HAS_VLAN |
			    FSL_GIANFAR_DEV_HAS_EXTENDED_HASH;

		ph = of_get_property(np, "phy-handle", NULL);
		if (ph == NULL) {
			unsigned int *bus_id;

			bus_id = of_get_property(np, "fixed_speed",NULL);
			gfar_data.bus_id = (bus_id[0]<<16) | bus_id[1];
			gfar_data.phy_id = -1;
		} else {
			phy = of_find_node_by_phandle(*ph);

			if (phy == NULL) {
				ret = -ENODEV;
				goto unreg;
			}

			mdio = of_get_parent(phy);

			id = of_get_property(phy, "reg", NULL);
			ret = of_address_to_resource(mdio, 0, &res);
			if (ret) {
				of_node_put(phy);
				of_node_put(mdio);
				goto unreg;
			}

			gfar_data.phy_id = *id;
			gfar_data.bus_id = res.start;

			of_node_put(phy);
			of_node_put(mdio);
		}

		ret =
		    platform_device_add_data(gfar_dev, &gfar_data,
					     sizeof(struct
						    gianfar_platform_data));
		if (ret)
			goto unreg;
	}

	return 0;

unreg:
	platform_device_unregister(gfar_dev);
err:
	return ret;
}

arch_initcall(gfar_of_init);

static int __init fsl_i2c_of_init(void)
{
	struct device_node *np;
	unsigned int i = 0;
	struct platform_device *i2c_dev;
	int ret;

	for_each_compatible_node(np, NULL, "fsl-i2c") {
		struct resource r[2];
		struct fsl_i2c_platform_data i2c_data;
		const unsigned char *flags = NULL;

		memset(&r, 0, sizeof(r));
		memset(&i2c_data, 0, sizeof(i2c_data));

		ret = of_address_to_resource(np, 0, &r[0]);
		if (ret)
			goto err;

		of_irq_to_resource(np, 0, &r[1]);

		i2c_dev = platform_device_register_simple("fsl-i2c", i++,
							  r, 2);
		if (IS_ERR(i2c_dev)) {
			ret = PTR_ERR(i2c_dev);
			goto err;
		}

		i2c_data.device_flags = 0;
		flags = of_get_property(np, "dfsrr", NULL);
		if (flags)
			i2c_data.device_flags |= FSL_I2C_DEV_SEPARATE_DFSRR;

		flags = of_get_property(np, "fsl5200-clocking", NULL);
		if (flags)
			i2c_data.device_flags |= FSL_I2C_DEV_CLOCK_5200;

		ret =
		    platform_device_add_data(i2c_dev, &i2c_data,
					     sizeof(struct
						    fsl_i2c_platform_data));
		if (ret)
			goto unreg;
	}

	return 0;

unreg:
	platform_device_unregister(i2c_dev);
err:
	return ret;
}

arch_initcall(fsl_i2c_of_init);

static enum fsl_usb2_phy_modes determine_usb_phy(const char *phy_type)
{
	if (!phy_type)
		return FSL_USB2_PHY_NONE;
	if (!strcasecmp(phy_type, "ulpi"))
		return FSL_USB2_PHY_ULPI;
	if (!strcasecmp(phy_type, "utmi"))
		return FSL_USB2_PHY_UTMI;
	if (!strcasecmp(phy_type, "utmi_wide"))
		return FSL_USB2_PHY_UTMI_WIDE;
	if (!strcasecmp(phy_type, "serial"))
		return FSL_USB2_PHY_SERIAL;

	return FSL_USB2_PHY_NONE;
}

static int __init fsl_usb_of_init(void)
{
	struct device_node *np;
	unsigned int i = 0;
	struct platform_device *usb_dev_mph = NULL, *usb_dev_dr_host = NULL,
		*usb_dev_dr_client = NULL;
	int ret;

	for_each_compatible_node(np, NULL, "fsl-usb2-mph") {
		struct resource r[2];
		struct fsl_usb2_platform_data usb_data;
		const unsigned char *prop = NULL;

		memset(&r, 0, sizeof(r));
		memset(&usb_data, 0, sizeof(usb_data));

		ret = of_address_to_resource(np, 0, &r[0]);
		if (ret)
			goto err;

		of_irq_to_resource(np, 0, &r[1]);

		usb_dev_mph =
		    platform_device_register_simple("fsl-ehci", i, r, 2);
		if (IS_ERR(usb_dev_mph)) {
			ret = PTR_ERR(usb_dev_mph);
			goto err;
		}

		usb_dev_mph->dev.coherent_dma_mask = 0xffffffffUL;
		usb_dev_mph->dev.dma_mask = &usb_dev_mph->dev.coherent_dma_mask;

		usb_data.operating_mode = FSL_USB2_MPH_HOST;

		prop = of_get_property(np, "port0", NULL);
		if (prop)
			usb_data.port_enables |= FSL_USB2_PORT0_ENABLED;

		prop = of_get_property(np, "port1", NULL);
		if (prop)
			usb_data.port_enables |= FSL_USB2_PORT1_ENABLED;

		prop = of_get_property(np, "phy_type", NULL);
		usb_data.phy_mode = determine_usb_phy(prop);

		ret =
		    platform_device_add_data(usb_dev_mph, &usb_data,
					     sizeof(struct
						    fsl_usb2_platform_data));
		if (ret)
			goto unreg_mph;
		i++;
	}

	for_each_compatible_node(np, NULL, "fsl-usb2-dr") {
		struct resource r[2];
		struct fsl_usb2_platform_data usb_data;
		const unsigned char *prop = NULL;

		memset(&r, 0, sizeof(r));
		memset(&usb_data, 0, sizeof(usb_data));

		ret = of_address_to_resource(np, 0, &r[0]);
		if (ret)
			goto unreg_mph;

		of_irq_to_resource(np, 0, &r[1]);

		prop = of_get_property(np, "dr_mode", NULL);

		if (!prop || !strcmp(prop, "host")) {
			usb_data.operating_mode = FSL_USB2_DR_HOST;
			usb_dev_dr_host = platform_device_register_simple(
					"fsl-ehci", i, r, 2);
			if (IS_ERR(usb_dev_dr_host)) {
				ret = PTR_ERR(usb_dev_dr_host);
				goto err;
			}
		} else if (prop && !strcmp(prop, "peripheral")) {
			usb_data.operating_mode = FSL_USB2_DR_DEVICE;
			usb_dev_dr_client = platform_device_register_simple(
					"fsl-usb2-udc", i, r, 2);
			if (IS_ERR(usb_dev_dr_client)) {
				ret = PTR_ERR(usb_dev_dr_client);
				goto err;
			}
		} else if (prop && !strcmp(prop, "otg")) {
			usb_data.operating_mode = FSL_USB2_DR_OTG;
			usb_dev_dr_host = platform_device_register_simple(
					"fsl-ehci", i, r, 2);
			if (IS_ERR(usb_dev_dr_host)) {
				ret = PTR_ERR(usb_dev_dr_host);
				goto err;
			}
			usb_dev_dr_client = platform_device_register_simple(
					"fsl-usb2-udc", i, r, 2);
			if (IS_ERR(usb_dev_dr_client)) {
				ret = PTR_ERR(usb_dev_dr_client);
				goto err;
			}
		} else {
			ret = -EINVAL;
			goto err;
		}

		prop = of_get_property(np, "phy_type", NULL);
		usb_data.phy_mode = determine_usb_phy(prop);

		if (usb_dev_dr_host) {
			usb_dev_dr_host->dev.coherent_dma_mask = 0xffffffffUL;
			usb_dev_dr_host->dev.dma_mask = &usb_dev_dr_host->
				dev.coherent_dma_mask;
			if ((ret = platform_device_add_data(usb_dev_dr_host,
						&usb_data, sizeof(struct
						fsl_usb2_platform_data))))
				goto unreg_dr;
		}
		if (usb_dev_dr_client) {
			usb_dev_dr_client->dev.coherent_dma_mask = 0xffffffffUL;
			usb_dev_dr_client->dev.dma_mask = &usb_dev_dr_client->
				dev.coherent_dma_mask;
			if ((ret = platform_device_add_data(usb_dev_dr_client,
						&usb_data, sizeof(struct
						fsl_usb2_platform_data))))
				goto unreg_dr;
		}
		i++;
	}
	return 0;

unreg_dr:
	if (usb_dev_dr_host)
		platform_device_unregister(usb_dev_dr_host);
	if (usb_dev_dr_client)
		platform_device_unregister(usb_dev_dr_client);
unreg_mph:
	if (usb_dev_mph)
		platform_device_unregister(usb_dev_mph);
err:
	return ret;
}

arch_initcall(fsl_usb_of_init);

static int __init fsl_elbc_of_init(void)
{
	struct device_node *np;
	unsigned int i;
	struct platform_device *elbc_dev = NULL;
	struct platform_device *nand_dev = NULL;
	int ret;

	/* find and register the enhanced local bus controller */
	for (np = NULL, i = 0;
	     (np = of_find_compatible_node(np, "elbc-fcm", "fsl-fcm")) != NULL;
	     i++) {
		struct resource r[2];

		memset(&r, 0, sizeof(r));

		ret = of_address_to_resource(np, 0, &r[0]);
		if (ret)
			goto err;

		r[1].start = r[1].end = irq_of_parse_and_map(np, 0);
		r[1].flags = IORESOURCE_IRQ;

		elbc_dev =
		    platform_device_register_simple("fsl-fcm", i, r, 2);
		if (IS_ERR(elbc_dev)) {
			ret = PTR_ERR(elbc_dev);
			goto err;
		}
	}

	/* find and register NAND memories if the eLBC was found */
	for (np = NULL, i = 0;
	     elbc_dev &&
	     (np = of_find_compatible_node(np, "rom", "fsl-nand")) != NULL;
	     i++) {
		struct resource r;
		struct platform_fsl_nand_chip chip_data;

		memset(&r, 0, sizeof(r));
		memset(&chip_data, 0, sizeof(chip_data));

		ret = of_address_to_resource(np, 0, &r);
		if (ret)
			goto unreg_elbc;

		nand_dev =
		    platform_device_register_simple("fsl-nand", i, &r, 1);
		if (IS_ERR(nand_dev)) {
			ret = PTR_ERR(nand_dev);
			goto unreg_elbc;
		}

		chip_data.name = of_get_property(np, "name", NULL);
		chip_data.partitions_str = of_get_property(np, "partitions", NULL);

		ret = platform_device_add_data(nand_dev, &chip_data,
					sizeof(struct platform_fsl_nand_chip));
		if (ret)
			goto unreg_nand;
	}
	return 0;

unreg_nand:
	platform_device_unregister(elbc_dev);
unreg_elbc:
	platform_device_unregister(nand_dev);
err:
	return ret;
}

arch_initcall(fsl_elbc_of_init);
 
#ifdef CONFIG_CPM2

static const char fcc_regs[] = "fcc_regs";
static const char fcc_regs_c[] = "fcc_regs_c";
static const char fcc_pram[] = "fcc_pram";
static char bus_id[9][BUS_ID_SIZE];

static int __init fs_enet_mdio_of_init(void)
{
	struct device_node *np;
	unsigned int i;
	struct platform_device *mdio_dev;
	struct resource res;
	int ret;

	for (np = NULL, i = 0;
	     (np = of_find_compatible_node(np, "mdio", "fs_enet")) != NULL;
	     i++) {
		int k;
		struct fs_mii_bb_platform_info mdio_data;
		struct device_node *child = NULL;

		memset(&res, 0, sizeof(res));
		memset(&mdio_data, 0, sizeof(mdio_data));

		ret = of_address_to_resource(np, 0, &res);
		if (ret)
			goto err;

		mdio_dev =
		    platform_device_register_simple("fsl-bb-mdio",
						    res.start, &res, 1);
		if (IS_ERR(mdio_dev)) {
			ret = PTR_ERR(mdio_dev);
			goto err;
		}

		for (k = 0; k < 32; k++)
			mdio_data.irq[k] = PHY_POLL;

		while ((child = of_get_next_child(np, child)) != NULL) {
			const unsigned char *mdio_bb_prop;
			int irq = irq_of_parse_and_map(child, 0);
			if (irq != NO_IRQ) {
				const u32 *id = of_get_property(child, "reg",
								NULL);
				mdio_data.irq[*id] = irq;
			}
			if (!(mdio_bb_prop = of_get_property(child, "bitbang",
							     NULL)))
				continue;
			mdio_data.mdio_dat.bit = mdio_bb_prop[0];
                	mdio_data.mdio_dir.bit = mdio_bb_prop[1];
                	mdio_data.mdc_dat.bit = mdio_bb_prop[2];
                	mdio_data.mdio_port = mdio_bb_prop[3];
                	mdio_data.mdc_port = mdio_bb_prop[4];
                	mdio_data.delay = mdio_bb_prop[5];
		}

		mdio_data.mdio_dat.offset =
			(u32)&cpm2_immr->im_ioport.iop_pdatc;
		mdio_data.mdio_dir.offset =
			(u32)&cpm2_immr->im_ioport.iop_pdirc;
                mdio_data.mdc_dat.offset = (u32)&cpm2_immr->im_ioport.iop_pdatc;


		ret =
		    platform_device_add_data(mdio_dev, &mdio_data,
					     sizeof(struct fs_mii_fec_platform_info));
		if (ret)
			goto unreg;
	}
	return 0;

unreg:
	platform_device_unregister(mdio_dev);
err:
	return ret;
}

arch_initcall(fs_enet_mdio_of_init);

static int __init fs_enet_of_init(void)
{
	struct device_node *np;
	unsigned int i;
	struct platform_device *fs_enet_dev;
	struct resource res;
	int ret;

	for (np = NULL, i = 0;
	     (np = of_find_compatible_node(np, "network", "fs_enet")) != NULL;
	     i++) {
		struct resource r[4];
		struct device_node *phy, *mdio;
		struct fs_platform_info fs_enet_data;
		const unsigned int *id, *phy_addr, *phy_irq;
		const void *mac_addr;
		const phandle *ph;
		const char *model;

		memset(r, 0, sizeof(r));
		memset(&fs_enet_data, 0, sizeof(fs_enet_data));

		ret = of_address_to_resource(np, 0, &r[0]);
		if (ret)
			goto err;
		r[0].name = fcc_regs;

		ret = of_address_to_resource(np, 1, &r[1]);
		if (ret)
			goto err;
		r[1].name = fcc_pram;

		ret = of_address_to_resource(np, 2, &r[2]);
		if (ret)
			goto err;
		r[2].name = fcc_regs_c;
		fs_enet_data.fcc_regs_c = r[2].start;

		of_irq_to_resource(np, 0, &r[3]);

		fs_enet_dev =
		    platform_device_register_simple("fsl-cpm-fcc", i, &r[0], 4);

		if (IS_ERR(fs_enet_dev)) {
			ret = PTR_ERR(fs_enet_dev);
			goto err;
		}

		model = of_get_property(np, "model", NULL);
		if (model == NULL) {
			ret = -ENODEV;
			goto unreg;
		}

		mac_addr = of_get_mac_address(np);
		if (mac_addr)
			memcpy(fs_enet_data.macaddr, mac_addr, 6);

		ph = of_get_property(np, "phy-handle", NULL);
		phy = of_find_node_by_phandle(*ph);

		if (phy == NULL) {
			ret = -ENODEV;
			goto unreg;
		}

		phy_addr = of_get_property(phy, "reg", NULL);
		fs_enet_data.phy_addr = *phy_addr;

		phy_irq = of_get_property(phy, "interrupts", NULL);

		id = of_get_property(np, "device-id", NULL);
		fs_enet_data.fs_no = *id;
		strcpy(fs_enet_data.fs_type, model);

		mdio = of_get_parent(phy);
                ret = of_address_to_resource(mdio, 0, &res);
                if (ret) {
                        of_node_put(phy);
                        of_node_put(mdio);
                        goto unreg;
                }

		fs_enet_data.clk_rx = *((u32 *)of_get_property(np,
						"rx-clock", NULL));
		fs_enet_data.clk_tx = *((u32 *)of_get_property(np,
						"tx-clock", NULL));

		if (strstr(model, "FCC")) {
			int fcc_index = *id - 1;

			fs_enet_data.dpram_offset = (u32)cpm_dpram_addr(0);
			fs_enet_data.rx_ring = 32;
			fs_enet_data.tx_ring = 32;
			fs_enet_data.rx_copybreak = 240;
			fs_enet_data.use_napi = 0;
			fs_enet_data.napi_weight = 17;
			fs_enet_data.mem_offset = FCC_MEM_OFFSET(fcc_index);
			fs_enet_data.cp_page = CPM_CR_FCC_PAGE(fcc_index);
			fs_enet_data.cp_block = CPM_CR_FCC_SBLOCK(fcc_index);

			snprintf((char*)&bus_id[(*id)], BUS_ID_SIZE, "%x:%02x",
							(u32)res.start, fs_enet_data.phy_addr);
			fs_enet_data.bus_id = (char*)&bus_id[(*id)];

			ret = platform_device_add_data(fs_enet_dev, &fs_enet_data,
						     sizeof(struct
							    fs_platform_info));
			if (ret)
				goto unreg;
		}
		of_node_put(phy);
		of_node_put(mdio);

	}
	return 0;

unreg:
	platform_device_unregister(fs_enet_dev);
err:
	return ret;
}

arch_initcall(fs_enet_of_init);

static const char scc_regs[] = "regs";
static const char scc_pram[] = "pram";

static int __init cpm_uart_of_init(void)
{
	struct device_node *np;
	unsigned int i;
	struct platform_device *cpm_uart_dev;
	int ret;

	for (np = NULL, i = 0;
	     (np = of_find_compatible_node(np, "serial", "cpm_uart")) != NULL;
	     i++) {
		struct resource r[3];
		struct fs_uart_platform_info cpm_uart_data;
		const int *id;
		const char *model;

		memset(r, 0, sizeof(r));
		memset(&cpm_uart_data, 0, sizeof(cpm_uart_data));

		ret = of_address_to_resource(np, 0, &r[0]);
		if (ret)
			goto err;

		r[0].name = scc_regs;

		ret = of_address_to_resource(np, 1, &r[1]);
		if (ret)
			goto err;
		r[1].name = scc_pram;

		of_irq_to_resource(np, 0, &r[2]);

		cpm_uart_dev =
		    platform_device_register_simple("fsl-cpm-scc:uart", i, &r[0], 3);

		if (IS_ERR(cpm_uart_dev)) {
			ret = PTR_ERR(cpm_uart_dev);
			goto err;
		}

		id = of_get_property(np, "device-id", NULL);
		cpm_uart_data.fs_no = *id;

		model = of_get_property(np, "model", NULL);
		strcpy(cpm_uart_data.fs_type, model);

		cpm_uart_data.uart_clk = ppc_proc_freq;

		cpm_uart_data.tx_num_fifo = 4;
		cpm_uart_data.tx_buf_size = 32;
		cpm_uart_data.rx_num_fifo = 4;
		cpm_uart_data.rx_buf_size = 32;
		cpm_uart_data.brg = cpm_uart_data.clk_rx =
			*((u32 *)of_get_property(np, "rx-clock", NULL));
		cpm_uart_data.clk_tx = *((u32 *)of_get_property(np,
						"tx-clock", NULL));

		ret =
		    platform_device_add_data(cpm_uart_dev, &cpm_uart_data,
					     sizeof(struct
						    fs_uart_platform_info));
		if (ret)
			goto unreg;
	}

	return 0;

unreg:
	platform_device_unregister(cpm_uart_dev);
err:
	return ret;
}

arch_initcall(cpm_uart_of_init);
#endif /* CONFIG_CPM2 */

#ifdef CONFIG_8xx

extern void init_scc_ioports(struct fs_platform_info*);
extern int platform_device_skip(const char *model, int id);

static int __init fs_enet_mdio_of_init(void)
{
	struct device_node *np;
	unsigned int i;
	struct platform_device *mdio_dev;
	struct resource res;
	int ret;

	for (np = NULL, i = 0;
	     (np = of_find_compatible_node(np, "mdio", "fs_enet")) != NULL;
	     i++) {
		struct fs_mii_fec_platform_info mdio_data;

		memset(&res, 0, sizeof(res));
		memset(&mdio_data, 0, sizeof(mdio_data));

		ret = of_address_to_resource(np, 0, &res);
		if (ret)
			goto err;

		mdio_dev =
		    platform_device_register_simple("fsl-cpm-fec-mdio",
						    res.start, &res, 1);
		if (IS_ERR(mdio_dev)) {
			ret = PTR_ERR(mdio_dev);
			goto err;
		}

		mdio_data.mii_speed = ((((ppc_proc_freq + 4999999) / 2500000) / 2) & 0x3F) << 1;

		ret =
		    platform_device_add_data(mdio_dev, &mdio_data,
					     sizeof(struct fs_mii_fec_platform_info));
		if (ret)
			goto unreg;
	}
	return 0;

unreg:
	platform_device_unregister(mdio_dev);
err:
	return ret;
}

arch_initcall(fs_enet_mdio_of_init);

static const char *enet_regs = "regs";
static const char *enet_pram = "pram";
static const char *enet_irq = "interrupt";
static char bus_id[9][BUS_ID_SIZE];

static int __init fs_enet_of_init(void)
{
	struct device_node *np;
	unsigned int i;
	struct platform_device *fs_enet_dev = NULL;
	struct resource res;
	int ret;

	for (np = NULL, i = 0;
	     (np = of_find_compatible_node(np, "network", "fs_enet")) != NULL;
	     i++) {
		struct resource r[4];
		struct device_node *phy = NULL, *mdio = NULL;
		struct fs_platform_info fs_enet_data;
		const unsigned int *id;
		const unsigned int *phy_addr;
		const void *mac_addr;
		const phandle *ph;
		const char *model;

		memset(r, 0, sizeof(r));
		memset(&fs_enet_data, 0, sizeof(fs_enet_data));

		model = of_get_property(np, "model", NULL);
		if (model == NULL) {
			ret = -ENODEV;
			goto unreg;
		}

		id = of_get_property(np, "device-id", NULL);
		fs_enet_data.fs_no = *id;

		if (platform_device_skip(model, *id))
			continue;

		ret = of_address_to_resource(np, 0, &r[0]);
		if (ret)
			goto err;
		r[0].name = enet_regs;

		mac_addr = of_get_mac_address(np);
		if (mac_addr)
			memcpy(fs_enet_data.macaddr, mac_addr, 6);

		ph = of_get_property(np, "phy-handle", NULL);
		if (ph != NULL)
			phy = of_find_node_by_phandle(*ph);

		if (phy != NULL) {
			phy_addr = of_get_property(phy, "reg", NULL);
			fs_enet_data.phy_addr = *phy_addr;
			fs_enet_data.has_phy = 1;

			mdio = of_get_parent(phy);
			ret = of_address_to_resource(mdio, 0, &res);
			if (ret) {
				of_node_put(phy);
				of_node_put(mdio);
                                goto unreg;
			}
		}

		model = of_get_property(np, "model", NULL);
		strcpy(fs_enet_data.fs_type, model);

		if (strstr(model, "FEC")) {
			r[1].start = r[1].end = irq_of_parse_and_map(np, 0);
			r[1].flags = IORESOURCE_IRQ;
			r[1].name = enet_irq;

			fs_enet_dev =
				    platform_device_register_simple("fsl-cpm-fec", i, &r[0], 2);

			if (IS_ERR(fs_enet_dev)) {
				ret = PTR_ERR(fs_enet_dev);
				goto err;
			}

			fs_enet_data.rx_ring = 128;
			fs_enet_data.tx_ring = 16;
			fs_enet_data.rx_copybreak = 240;
			fs_enet_data.use_napi = 1;
			fs_enet_data.napi_weight = 17;

			snprintf((char*)&bus_id[i], BUS_ID_SIZE, "%x:%02x",
							(u32)res.start, fs_enet_data.phy_addr);
			fs_enet_data.bus_id = (char*)&bus_id[i];
			fs_enet_data.init_ioports = init_fec_ioports;
		}
		if (strstr(model, "SCC")) {
			ret = of_address_to_resource(np, 1, &r[1]);
			if (ret)
				goto err;
			r[1].name = enet_pram;

			r[2].start = r[2].end = irq_of_parse_and_map(np, 0);
			r[2].flags = IORESOURCE_IRQ;
			r[2].name = enet_irq;

			fs_enet_dev =
				    platform_device_register_simple("fsl-cpm-scc", i, &r[0], 3);

			if (IS_ERR(fs_enet_dev)) {
				ret = PTR_ERR(fs_enet_dev);
				goto err;
			}

			fs_enet_data.rx_ring = 64;
			fs_enet_data.tx_ring = 8;
			fs_enet_data.rx_copybreak = 240;
			fs_enet_data.use_napi = 1;
			fs_enet_data.napi_weight = 17;

			snprintf((char*)&bus_id[i], BUS_ID_SIZE, "%s", "fixed@10:1");
                        fs_enet_data.bus_id = (char*)&bus_id[i];
			fs_enet_data.init_ioports = init_scc_ioports;
		}

		of_node_put(phy);
		of_node_put(mdio);

		ret = platform_device_add_data(fs_enet_dev, &fs_enet_data,
					     sizeof(struct
						    fs_platform_info));
		if (ret)
			goto unreg;
	}
	return 0;

unreg:
	platform_device_unregister(fs_enet_dev);
err:
	return ret;
}

arch_initcall(fs_enet_of_init);

static int __init fsl_pcmcia_of_init(void)
{
	struct device_node *np = NULL;
	/*
	 * Register all the devices which type is "pcmcia"
	 */
	while ((np = of_find_compatible_node(np,
			"pcmcia", "fsl,pq-pcmcia")) != NULL)
			    of_platform_device_create(np, "m8xx-pcmcia", NULL);
	return 0;
}

arch_initcall(fsl_pcmcia_of_init);

static const char *smc_regs = "regs";
static const char *smc_pram = "pram";

static int __init cpm_smc_uart_of_init(void)
{
	struct device_node *np;
	unsigned int i;
	struct platform_device *cpm_uart_dev;
	int ret;

	for (np = NULL, i = 0;
	     (np = of_find_compatible_node(np, "serial", "cpm_uart")) != NULL;
	     i++) {
		struct resource r[3];
		struct fs_uart_platform_info cpm_uart_data;
		const u32 *brg;
		const int *id;
		const char *model;

		memset(r, 0, sizeof(r));
		memset(&cpm_uart_data, 0, sizeof(cpm_uart_data));

		ret = of_address_to_resource(np, 0, &r[0]);
		if (ret)
			goto err;

		r[0].name = smc_regs;

		ret = of_address_to_resource(np, 1, &r[1]);
		if (ret)
			goto err;
		r[1].name = smc_pram;

		r[2].start = r[2].end = irq_of_parse_and_map(np, 0);
		r[2].flags = IORESOURCE_IRQ;

		cpm_uart_dev =
		    platform_device_register_simple("fsl-cpm-smc:uart", i, &r[0], 3);

		if (IS_ERR(cpm_uart_dev)) {
			ret = PTR_ERR(cpm_uart_dev);
			goto err;
		}

		model = of_get_property(np, "model", NULL);
		strcpy(cpm_uart_data.fs_type, model);

		id = of_get_property(np, "device-id", NULL);
		cpm_uart_data.fs_no = *id;
		cpm_uart_data.uart_clk = ppc_proc_freq;

		cpm_uart_data.tx_num_fifo = 4;
		cpm_uart_data.tx_buf_size = 32;
		cpm_uart_data.rx_num_fifo = 4;
		cpm_uart_data.rx_buf_size = 32;
		brg = of_get_property(np, "fsl,cpm-brg", NULL);
		if (brg)
			cpm_uart_data.brg = *brg;

		ret =
		    platform_device_add_data(cpm_uart_dev, &cpm_uart_data,
					     sizeof(struct
						    fs_uart_platform_info));
		if (ret)
			goto unreg;
	}

	return 0;

unreg:
	platform_device_unregister(cpm_uart_dev);
err:
	return ret;
}

arch_initcall(cpm_smc_uart_of_init);

static int __init fsl_i2c_cpm_of_init(void)
{
	struct device_node *np = NULL;

	/* Register all the devices which type is "i2c-cpm" */
	while ((np = of_find_compatible_node(np, "i2c", "fsl,i2c-cpm")) != NULL)
		of_platform_device_create(np, "fsl-i2c-cpm", NULL);

	return 0;
}

arch_initcall(fsl_i2c_cpm_of_init);

static const char *usbc_regs = "regs";
static const char *usbc_pram = "pram";
static const char *usbc_irq = "interrupt";

static int __init fsl_usbc_of_init(void)
{
	struct device_node *np;
	unsigned int i;
	struct platform_device *usbc_dev;
	int ret;

	for (np = NULL, i = 0;
	     (np = of_find_compatible_node(np, "usbc", "fsl,usbc-cpm")) != NULL;
	     i++) {
		struct resource r[3];

		memset(&r, 0, sizeof(r));

		ret = of_address_to_resource(np, 0, &r[0]);
		if (ret)
			goto err;
		r[0].name = usbc_regs;

		ret = of_address_to_resource(np, 1, &r[1]);
		if (ret)
			goto err;
		r[1].name = usbc_pram;

		r[2].start = r[2].end = irq_of_parse_and_map(np, 0);
		r[2].flags = IORESOURCE_IRQ;
		r[2].name = usbc_irq;

		usbc_dev = platform_device_register_simple("fsl-cpm-usbc", i, &r[0], 3);
		if (IS_ERR(usbc_dev)) {
			ret = PTR_ERR(usbc_dev);
			goto err;
		}
		ret =
		    platform_device_add_data(usbc_dev, NULL, 0);

		if (ret)
			goto unreg;
	}

	return 0;

unreg:
	platform_device_unregister(usbc_dev);
err:
	return ret;
}

arch_initcall(fsl_usbc_of_init);

static int __init fsl_irda_of_init(void)
{
	struct device_node *np = NULL;

	while ((np = of_find_compatible_node(np, "network", "fsl,irda")) != NULL)
		of_platform_device_create(np, "fsl-cpm-scc:irda", NULL);
	return 0;
}

arch_initcall(fsl_irda_of_init);


#endif /* CONFIG_8xx */

int __init fsl_spi_init(struct spi_board_info *board_infos,
			unsigned int num_board_infos,
			void (*activate_cs)(u8 cs, u8 polarity),
			void (*deactivate_cs)(u8 cs, u8 polarity))
{
	struct device_node *np;
	unsigned int i;
	u32 sysclk = -1;

	/* SPI controller is either clocked from QE or SoC clock */
#ifdef CONFIG_QUICC_ENGINE
	sysclk = get_brgfreq();
#endif
	if (sysclk == -1) {
		sysclk = fsl_get_sys_freq();
		if (sysclk == -1);
			return -ENODEV;
	}

	for (np = NULL, i = 1;
	     (np = of_find_compatible_node(np, "spi", "fsl_spi")) != NULL;
	     i++) {
		int ret = 0;
		unsigned int j;
		const void *prop;
		struct resource res[2];
		struct platform_device *pdev;
		struct fsl_spi_platform_data pdata = {
			.activate_cs = activate_cs,
			.deactivate_cs = deactivate_cs,
		};

		memset(res, 0, sizeof(res));

		pdata.sysclk = sysclk;

		prop = of_get_property(np, "reg", NULL);
		if (!prop)
			goto err;
		pdata.bus_num = *(u32 *)prop;

		prop = of_get_property(np, "mode", NULL);
		if (prop && !strcmp(prop, "cpu-qe"))
			pdata.qe_mode = 1;

		for (j = 0; j < num_board_infos; j++) {
			if (board_infos[j].bus_num == pdata.bus_num)
				pdata.max_chipselect++;
		}

		if (!pdata.max_chipselect)
			goto err;

		ret = of_address_to_resource(np, 0, &res[0]);
		if (ret)
			goto err;

		res[1].start = res[2].end = irq_of_parse_and_map(np, 0);
		if (res[1].start == NO_IRQ)
			goto err;

		res[1].name = "mpc83xx_spi";
		res[1].flags = IORESOURCE_IRQ;

		pdev = platform_device_alloc("mpc83xx_spi", i);
		if (!pdev)
			goto err;

		ret = platform_device_add_data(pdev, &pdata, sizeof(pdata));
		if (ret)
			goto unreg;

		ret = platform_device_add_resources(pdev, res,
						    ARRAY_SIZE(res));
		if (ret)
			goto unreg;

		ret = platform_device_register(pdev);
		if (ret)
			goto unreg;

		continue;
unreg:
		platform_device_del(pdev);
err:
		continue;
	}

	return spi_register_board_info(board_infos, num_board_infos);
}

#if defined(CONFIG_PPC_85xx) || defined(CONFIG_PPC_86xx)
static __be32 __iomem *rstcr;

static int __init setup_rstcr(void)
{
	struct device_node *np;
	np = of_find_node_by_name(NULL, "global-utilities");
	if ((np && of_get_property(np, "fsl,has-rstcr", NULL))) {
		const u32 *prop = of_get_property(np, "reg", NULL);
		if (prop) {
			/* map reset control register
			 * 0xE00B0 is offset of reset control register
			 */
			rstcr = ioremap(get_immrbase() + *prop + 0xB0, 0xff);
			if (!rstcr)
				printk (KERN_EMERG "Error: reset control "
						"register not mapped!\n");
		}
	} else
		printk (KERN_INFO "rstcr compatible register does not exist!\n");
	if (np)
		of_node_put(np);
	return 0;
}

arch_initcall(setup_rstcr);

void fsl_rstcr_restart(char *cmd)
{
	local_irq_disable();
	if (rstcr)
		/* set reset control register */
		out_be32(rstcr, 0x2);	/* HRESET_REQ */
#ifdef CONFIG_FSL_BOOKE
	else
		/* some CPUs does not have rstcr, use dbcr0 */
		abort();
#endif

	while (1) ;
}
#endif

#if defined(CONFIG_FB_FSL_DIU) || defined(CONFIG_FB_FSL_DIU_MODULE)
struct platform_diu_data_ops diu_ops = {
	.diu_size = 1280 * 1024 * 4,	/* default one 1280x1024 buffer */
};
EXPORT_SYMBOL(diu_ops);

int __init preallocate_diu_videomemory(void)
{
	pr_debug("diu_size=%lu\n", diu_ops.diu_size);

	diu_ops.diu_mem = __alloc_bootmem(diu_ops.diu_size, 8, 0);
	if (!diu_ops.diu_mem) {
		printk(KERN_ERR "fsl-diu: cannot allocate %lu bytes\n",
			diu_ops.diu_size);
		return -ENOMEM;
	}

	pr_debug("diu_mem=%p\n", diu_ops.diu_mem);

	rh_init(&diu_ops.diu_rh_info, 4096, ARRAY_SIZE(diu_ops.diu_rh_block),
		diu_ops.diu_rh_block);
	return rh_attach_region(&diu_ops.diu_rh_info,
				(unsigned long) diu_ops.diu_mem,
				diu_ops.diu_size);
}

static int __init early_parse_diufb(char *p)
{
	if (!p)
		return 1;

	diu_ops.diu_size = _ALIGN_UP(memparse(p, &p), 8);

	pr_debug("diu_size=%lu\n", diu_ops.diu_size);

	return 0;
}
early_param("diufb", early_parse_diufb);

#endif
