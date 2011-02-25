/*
 * AMCC Canyonlands SATA wrapper
 *
 * Copyright 2008 DENX Software Engineering, Stefan Roese <sr@denx.de>
 *
 * Extract the resources (MEM & IRQ) from the dts file and put them
 * into the platform-device struct for usage in the platform-device
 * SATA driver.
 *
 */

#if defined(CONFIG_SATA_DWC) || defined(CONFIG_SATA_DWC_MODULE)
#include <linux/platform_device.h>
#include <asm/of_platform.h>

#include <asm/machdep.h>

/*
 * Resource template will be filled dynamically with the values
 * extracted from the dts file
 */
static struct resource sata_resources[] = {
	[0] = {
		/* 460EX SATA registers */
		.flags  = IORESOURCE_MEM,
	},
	[1] = {
		/* 460EX AHBDMA registers */
		.flags  = IORESOURCE_MEM,
	},
	[2] = {
		/* 460EX SATA IRQ */
		.flags  = IORESOURCE_IRQ,
	},
	[3] = {
		/* 460EX AHBDMA IRQ */
		.flags  = IORESOURCE_IRQ,
	},
};

static u64 dma_mask = 0xffffffffULL;

static struct platform_device sata_device = {
	.name = "sata-dwc",
	.id = 0,
	.num_resources = ARRAY_SIZE(sata_resources),
	.resource = sata_resources,
	.dev = {
		.dma_mask = &dma_mask,
		.coherent_dma_mask = 0xffffffffULL,
	}
};

static struct platform_device *ppc460ex_devs[] __initdata = {
	&sata_device,
};

static int __devinit ppc460ex_sata_probe(struct of_device *ofdev,
					 const struct of_device_id *match)
{
	struct device_node *np = ofdev->node;
	struct resource res;
	const char *val;

	/*
	 * Check if device is enabled
	 */
	val = of_get_property(np, "status", NULL);
	if (val && !strcmp(val, "disabled")) {
		printk(KERN_INFO "SATA port disabled via device-tree\n");
		return 0;
	}

	/*
	 * Extract register address reange from device tree and put it into
	 * the platform device structure
	 */
	if (of_address_to_resource(np, 0, &res)) {
		printk(KERN_ERR "%s: Can't get SATA register address\n", __func__);
		return -ENOMEM;
	}
	sata_resources[0].start = res.start;
	sata_resources[0].end = res.end;

	if (of_address_to_resource(np, 1, &res)) {
		printk(KERN_ERR "%s: Can't get AHBDMA register address\n", __func__);
		return -ENOMEM;
	}
	sata_resources[1].start = res.start;
	sata_resources[1].end = res.end;

	/*
	 * Extract IRQ number(s) from device tree and put them into
	 * the platform device structure
	 */
	sata_resources[2].start = sata_resources[2].end =
		irq_of_parse_and_map(np, 0);
	sata_resources[3].start = sata_resources[3].end =
		irq_of_parse_and_map(np, 1);

	return platform_add_devices(ppc460ex_devs, ARRAY_SIZE(ppc460ex_devs));
}

static int __devexit ppc460ex_sata_remove(struct of_device *ofdev)
{
	/* Nothing to do here */
	return 0;
}

static const struct of_device_id ppc460ex_sata_match[] = {
	{ .compatible = "amcc,sata-460ex", },
	{}
};

static struct of_platform_driver ppc460ex_sata_driver = {
	.name = "sata-460ex",
	.match_table = ppc460ex_sata_match,
	.probe = ppc460ex_sata_probe,
	.remove = ppc460ex_sata_remove,
};

static int __init ppc460ex_sata_init(void)
{
	return of_register_platform_driver(&ppc460ex_sata_driver);
}

machine_device_initcall(canyonlands, ppc460ex_sata_init);
#endif	/* CONFIG_SATA_DWC || CONFIG_SATA_DWC_MODULE */
