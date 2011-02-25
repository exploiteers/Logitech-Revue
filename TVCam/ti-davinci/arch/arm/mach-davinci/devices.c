/*
 * linux/arch/arm/mach-davinci/devices.c
 *
 * DaVinci platform device setup/initialization
 *
 * Copyright (C) 2006 Komal Shah <komal_shah802003@yahoo.com>
 * Copyright (c) 2007, MontaVista Software, Inc. <source@mvista.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/usb/musb.h>
#include <linux/davinci_emac.h>

#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/mach-types.h>
#include <asm/mach/map.h>

#include <asm/arch/i2c.h>
#include <asm/arch/cpu.h>

static struct resource i2c_resources[] = {
	{
		.start		= DAVINCI_I2C_BASE,
		.end		= DAVINCI_I2C_BASE + 0x40,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= IRQ_I2C,
		.flags		= IORESOURCE_IRQ,
	},
};

static struct davinci_i2c_platform_data dm644x_i2c_data = {
	.bus_freq	= 20,
	.bus_delay	= 100,
};

static struct davinci_i2c_platform_data dm355_i2c_data = {
	.bus_freq	= 20,
	.bus_delay	= 100,
};

static struct davinci_i2c_platform_data dm646x_i2c_data = {
	.bus_freq	= 100,
	.bus_delay	= 0,
};

static struct platform_device i2c_device = {
	.name           = "i2c_davinci",
	.id             = 1,
	.dev		= {
		.platform_data = &dm355_i2c_data,
	},
	.num_resources	= ARRAY_SIZE(i2c_resources),
	.resource	= i2c_resources,
};

struct resource watchdog_resources[] = {
	{
		.start = DAVINCI_WDOG_BASE,
		.end = DAVINCI_WDOG_BASE + SZ_1K - 1,
		.flags = IORESOURCE_MEM,
	},
};

static struct platform_device watchdog_device = {
	.name = "watchdog",
	.id = -1,
	.num_resources = ARRAY_SIZE(watchdog_resources),
	.resource = watchdog_resources,
};


static struct musb_hdrc_platform_data usb_data = {
#if     defined(CONFIG_USB_MUSB_OTG)
	/* OTG requires a Mini-AB connector */
	.mode		= MUSB_OTG,
#elif   defined(CONFIG_USB_MUSB_PERIPHERAL)
	.mode		= MUSB_PERIPHERAL,
#elif   defined(CONFIG_USB_MUSB_HOST)
	.mode		= MUSB_HOST,
#endif
	/* irlml6401 switches 5V */
	.power		= 250,		/* sustains 3.0+ Amps (!) */
	.potpgt		= 4,		/* ~8 msec */

	/* REVISIT multipoint is a _chip_ capability; not board specific */
	.multipoint	= 1,
};

static struct resource usb_resources [] = {
	{
		.start	= DAVINCI_USB_OTG_BASE,
		.end	= DAVINCI_USB_OTG_BASE + 0x5ff,
		.flags	= IORESOURCE_MEM,
	},
	{
		.start	= IRQ_USBINT,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.start  = 0, /*by default no dedicated dma irq*/
		.flags  = IORESOURCE_IRQ,
	}
};

static u64 usb_dmamask = DMA_32BIT_MASK;

static struct platform_device usb_device = {
        .name		= "musb_hdrc",
        .id		= -1,
        .dev = {
                .platform_data		= &usb_data,
                .dma_mask		= &usb_dmamask,
                .coherent_dma_mask	= DMA_32BIT_MASK,
        },
        .resource	= usb_resources,
        .num_resources	= ARRAY_SIZE(usb_resources),
};

static struct platform_device *devices[] __initdata = {
	&i2c_device,
	&watchdog_device,
	&usb_device,
};

static void __init davinci_init_cpu_i2c(void)
{
	if (cpu_is_davinci_dm644x() || cpu_is_davinci_dm357())
		i2c_device.dev.platform_data = &dm644x_i2c_data;
	else if (cpu_is_davinci_dm6467())
		i2c_device.dev.platform_data = &dm646x_i2c_data;

	/* all others default to use dm355 because dm355 uses the max speed */
}

static void __init davinci_init_cpu_usb(void)
{
	if (cpu_is_davinci_dm6467()) {
		/*
		 * overwrite default settings
		 * as DM6467 uses different irq lines
		 */
		usb_device.resource[1].start = IRQ_DM646X_USBINT;
		usb_device.resource[2].start = IRQ_DM646X_USBDMAINT;
	}
}

/*
 * MAC stuff
 */

#define RESOURCE_IRQ 4

static struct resource mac_dm365_resources[] = {
	{
		.start	= DM365_EMAC_CNTRL_BASE,
		.end	= DM365_EMAC_CNTRL_BASE + 0xfff,
		.flags	= IORESOURCE_MEM,
		.name	= "ctrl_regs",
	},
	{
		.start	= DM365_EMAC_WRAP_CNTRL_BASE,
		.end	= DM365_EMAC_WRAP_CNTRL_BASE + 0xfff,
		.flags	= IORESOURCE_MEM,
		.name	= "wrapper_ctrl_regs",
	},
	{
		.start	= DM365_EMAC_WRAP_RAM_BASE,
		.end	= DM365_EMAC_WRAP_RAM_BASE + 0x1fff,
		.flags	= IORESOURCE_MEM,
		.name	= "wrapper_ram",
	},
	{
		.start	= DM365_EMAC_MDIO_BASE,
		.end	= DM365_EMAC_MDIO_BASE + 0x7ff,
		.flags	= IORESOURCE_MEM,
		.name	= "mdio_regs",
	},
	[RESOURCE_IRQ] = {
		.start	= IRQ_DM365_EMAC_RXTHRESH,
		.flags	= IORESOURCE_IRQ,
		.name	= "mac_rx_threshold",
	},
	{
		.start	= IRQ_DM365_EMAC_RXPULSE,
		.flags	= IORESOURCE_IRQ,
		.name	= "mac_rx",
	},
	{
		.start	= IRQ_DM365_EMAC_TXPULSE,
		.flags	= IORESOURCE_IRQ,
		.name	= "mac_tx",
	},
	{
		.start	= IRQ_DM365_EMAC_MISCPULSE,
		.flags	= IORESOURCE_IRQ,
		.name	= "mac_misc",
	},
};

static struct resource mac_resources[] = {
	{
		.start	= DAVINCI_EMAC_CNTRL_REGS_BASE,
		.end	= DAVINCI_EMAC_CNTRL_REGS_BASE + 0xfff,
		.flags	= IORESOURCE_MEM,
		.name	= "ctrl_regs",
	},
	{
		.start	= DAVINCI_EMAC_WRAPPER_CNTRL_REGS_BASE,
		.end	= DAVINCI_EMAC_WRAPPER_CNTRL_REGS_BASE + 0xfff,
		.flags	= IORESOURCE_MEM,
		.name	= "wrapper_ctrl_regs",
	},
	{
		.start	= DAVINCI_EMAC_WRAPPER_RAM_BASE,
		.end	= DAVINCI_EMAC_WRAPPER_RAM_BASE + 0x1fff,
		.flags	= IORESOURCE_MEM,
		.name	= "wrapper_ram",
	},
	{
		.start	= DAVINCI_MDIO_CNTRL_REGS_BASE,
		.end	= DAVINCI_MDIO_CNTRL_REGS_BASE + 0x7ff,
		.flags	= IORESOURCE_MEM,
		.name	= "mdio_regs",
	},
	[RESOURCE_IRQ] = {
		.start	= IRQ_DM646X_EMACRXTHINT,
		.flags	= IORESOURCE_IRQ,
		.name	= "mac_rx_threshold",
	},
	{
		.start	= IRQ_DM646X_EMACRXINT,
		.flags	= IORESOURCE_IRQ,
		.name	= "mac_rx",
	},
	{
		.start	= IRQ_DM646X_EMACTXINT,
		.flags	= IORESOURCE_IRQ,
		.name	= "mac_tx",
	},
	{
		.start	= IRQ_DM646X_EMACMISCINT,
		.flags	= IORESOURCE_IRQ,
		.name	= "mac_misc",
	},
};

static struct emac_init_config dm365_emac_data = {
	.reset_line = 0,
	.emac_bus_frequency = 121500000,
	.mdio_reset_line = 0,
	.mdio_bus_frequency = 121500000,
	.mdio_clock_frequency = 2200000,
	.registers_old = 0,
	.gigabit = 0,
};

static struct emac_init_config dm646x_emac_data = {
	.reset_line = 0,
	.emac_bus_frequency = 148500000,
	.mdio_reset_line = 0,
	.mdio_bus_frequency = 148500000,
	.mdio_clock_frequency = 2200000,
	.registers_old = 0,
	.gigabit = 1,
};

static struct emac_init_config dm644x_emac_data = {
	.reset_line = 0,
	.emac_bus_frequency = 76500000,
	.mdio_reset_line = 0,
	.mdio_bus_frequency = 76500000,
	.mdio_clock_frequency = 2200000,
	.registers_old = 1,
	.gigabit = 0,
};

static struct platform_device emac_device = {
	.name           = "emac_davinci",
	.id             = 1,
	.dev		= {
		.platform_data = &dm646x_emac_data,
	},
	.num_resources	= ARRAY_SIZE(mac_resources),
	.resource	= mac_resources,
};

static void __init davinci_init_cpu_emac_register(void)
{
	if (cpu_is_davinci_dm644x() || cpu_is_davinci_dm357()) {
		emac_device.dev.platform_data = &dm644x_emac_data;
		mac_resources[RESOURCE_IRQ] = (struct resource) {
			.start	= IRQ_DM644X_EMACINT,
			.flags	= IORESOURCE_IRQ,
			.name	= "mac",
		};
		emac_device.num_resources = RESOURCE_IRQ + 1;
	} else if (cpu_is_davinci_dm365()) {
		emac_device.dev.platform_data = &dm365_emac_data;
		emac_device.resource = mac_dm365_resources;
		emac_device.num_resources = ARRAY_SIZE(mac_dm365_resources);
	} else if (cpu_is_davinci_dm6467()) {
		/* do nothing */
	}
	else
		return;

	platform_device_register(&emac_device);
}

static int __init davinci_init_devices(void)
{
	davinci_init_cpu_i2c();
	davinci_init_cpu_usb();
	platform_add_devices(devices, ARRAY_SIZE(devices));
	davinci_init_cpu_emac_register();
	return 0;
}
arch_initcall(davinci_init_devices);
