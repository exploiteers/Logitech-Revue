/*
 * arch/arm/mach-da8xx/devices.c
 *
 * DA8xx platform device setup/initialization
 *
 * Copyright (C) 2008 MontaVista Software, Inc. <source@mvista.com>
 *
 * Based on arch/arm/mach-davinci/devices.c
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
#include <linux/serial.h>
#include <linux/serial_8250.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/usb/musb.h>
#include <linux/davinci_emac.h>
#include <linux/io.h>
#include <linux/kgdb.h>

#include <asm/hardware.h>
#include <asm/mach-types.h>
#include <asm/mach/map.h>

#include <asm/arch/i2c.h>
#include <asm/arch/cpu.h>
#include <asm/arch/edma.h>
#include <asm/hardware/cp_intc.h>

#include "devices.h"

static struct plat_serial8250_port da8xx_serial_pdata[] = {
	[0] = {
		.membase	= (char *)IO_ADDRESS(DA8XX_UART0_BASE),
		.mapbase	= (unsigned long)DA8XX_UART0_BASE,
		.irq		= IRQ_DA8XX_UARTINT0,
		.flags		= UPF_BOOT_AUTOCONF | UPF_SKIP_TEST,
		.iotype		= UPIO_MEM,
		.regshift	= 2,
		.uartclk	= 0, /* Filled in later */
	},
	[1] = {
		.membase	= (char *)IO_ADDRESS(DA8XX_UART1_BASE),
		.mapbase	= (unsigned long)DAVINCI_UART1_BASE,
		.irq		= IRQ_DA8XX_UARTINT1,
		.flags		= UPF_BOOT_AUTOCONF | UPF_SKIP_TEST,
		.iotype		= UPIO_MEM,
		.regshift	= 2,
		.uartclk	= 0, /* Filled in later */
	},
	[2] = {
		.membase	= (char *)IO_ADDRESS(DA8XX_UART2_BASE),
		.mapbase	= (unsigned long)DA8XX_UART2_BASE,
		.irq		= IRQ_DA8XX_UARTINT2,
		.flags		= UPF_BOOT_AUTOCONF | UPF_SKIP_TEST,
		.iotype		= UPIO_MEM,
		.regshift	= 2,
		.uartclk	= 0, /* Filled in later */
	},
	[3] = {
		.flags	= 0,
	},
};

static struct platform_device da8xx_serial_device = {
	.name	= "serial8250",
	.id	= 0,
	.dev	= {
		.platform_data	= da8xx_serial_pdata,
	},
};

static struct resource da8xx_i2c_resources0[] = {
	[0] = {
		.start	= DA8XX_I2C0_BASE,
		.end	= DA8XX_I2C0_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_DA8XX_I2CINT0,
		.end	= IRQ_DA8XX_I2CINT0,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device da8xx_i2c_device0 = {
	.name		= "i2c_davinci",
	.id		= 1,
	.num_resources	= ARRAY_SIZE(da8xx_i2c_resources0),
	.resource	= da8xx_i2c_resources0,
};

static struct resource da8xx_i2c_resources1[] = {
	[0] = {
		.start	= DA8XX_I2C1_BASE,
		.end	= DA8XX_I2C1_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_DA8XX_I2CINT1,
		.end	= IRQ_DA8XX_I2CINT1,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device da8xx_i2c_device1 = {
	.name		= "i2c_davinci",
	.id		= 2,
	.num_resources	= ARRAY_SIZE(da8xx_i2c_resources1),
	.resource	= da8xx_i2c_resources1,
};

static struct resource da8xx_watchdog_resources[] = {
	[0] = {
		.start	= DA8XX_WDOG_BASE,
		.end	= DA8XX_WDOG_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	},
};

static struct platform_device da8xx_watchdog_device = {
	.name		= "watchdog",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(da8xx_watchdog_resources),
	.resource	= da8xx_watchdog_resources,
};

static struct resource da8xx_usb20_resources[] = {
	[0] = {
		.start	= DA8XX_USB0_CFG_BASE,
		.end	= DA8XX_USB0_CFG_BASE + SZ_64K - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_DA8XX_USB_INT,
		.end	= IRQ_DA8XX_USB_INT,
		.flags	= IORESOURCE_IRQ,
	}
};

static u64 da8xx_usb20_dma_mask = DMA_32BIT_MASK;

static struct platform_device da8xx_usb20_device = {
	.name		= "musb_hdrc",
	.id		= -1,
	.dev = {
		.dma_mask		= &da8xx_usb20_dma_mask,
		.coherent_dma_mask	= DMA_32BIT_MASK,
	},
	.num_resources	= ARRAY_SIZE(da8xx_usb20_resources),
	.resource	= da8xx_usb20_resources,
};

static struct resource da8xx_usb11_resources[] = {
	[0] = {
		.start	= DA8XX_USB1_BASE,
		.end	= DA8XX_USB1_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_DA8XX_IRQN,
		.end	= IRQ_DA8XX_IRQN,
		.flags	= IORESOURCE_IRQ,
	},
};

static u64 da8xx_usb11_dma_mask = ~(u32)0;

static struct platform_device da8xx_usb11_device = {
	.name		= "ohci",
	.id		= 0,
	.dev = {
		.dma_mask		= &da8xx_usb11_dma_mask,
		.coherent_dma_mask	= 0xffffffff,
	},
	.num_resources	= ARRAY_SIZE(da8xx_usb11_resources),
	.resource	= da8xx_usb11_resources,
};

static struct resource da8xx_emac_resources[] = {
	[0] = {
		.start	= DA8XX_EMAC_CPGMAC_BASE,
		.end	= DA8XX_EMAC_CPGMAC_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
		.name	= "ctrl_regs",
	},
	[1] = {
		.start	= DA8XX_EMAC_CPGMACSS_BASE,
		.end	= DA8XX_EMAC_CPGMACSS_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
		.name	= "wrapper_ctrl_regs",
	},
	[2] = {
		.start	= DA8XX_EMAC_CPPI_PORT_BASE,
		.end	= DA8XX_EMAC_CPPI_PORT_BASE + SZ_8K - 1,
		.flags	= IORESOURCE_MEM,
		.name	= "wrapper_ram",
	},
	[3] = {
		.start	= DA8XX_EMAC_MDIO_BASE,
		.end	= DA8XX_EMAC_MDIO_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
		.name	= "mdio_regs",
	},
	[4] = { /* Must be entry 4 */
		.start	= IRQ_DA8XX_C0_RX_THRESH_PULSE,
		.flags	= IORESOURCE_IRQ,
		.name	= "mac_rx_threshold",
	},
	[5] = {
		.start	= IRQ_DA8XX_C0_RX_PULSE,
		.flags	= IORESOURCE_IRQ,
		.name	= "mac_rx",
	},
	[6] = {
		.start	= IRQ_DA8XX_C0_TX_PULSE,
		.flags	= IORESOURCE_IRQ,
		.name	= "mac_tx",
	},
	[7] = {
		.start	= IRQ_DA8XX_C0_MISC_PULSE,
		.flags	= IORESOURCE_IRQ,
		.name	= "mac_misc",
	},
};

static struct platform_device da8xx_emac_device = {
	.name		= "emac_davinci",
	.id		= 1,
	.num_resources	= ARRAY_SIZE(da8xx_emac_resources),
	.resource	= da8xx_emac_resources,
};

static struct resource da8xx_mmc_resources[] = {
	[0] = {		 /* registers */
		.start	= DA8XX_MMC_SD0_BASE,
		.end	= DA8XX_MMC_SD0_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM
	},
	[1] = {		 /* interrupt */
		.start	= IRQ_DA8XX_MMCSDINT0,
		.end	= IRQ_DA8XX_MMCSDINT0,
		.flags	= IORESOURCE_IRQ
	},
	[2] = {		 /* DMA RX */
		.start	= DA8XX_DMACH_MMCSD_RX,
		.end	= DA8XX_DMACH_MMCSD_RX,
		.flags	= IORESOURCE_DMA
	},
	[3] = {		 /* DMA TX */
		.start	= DA8XX_DMACH_MMCSD_TX,
		.end	= DA8XX_DMACH_MMCSD_TX,
		.flags	= IORESOURCE_DMA
	}
};

static struct platform_device da8xx_mmc_device = {
	.name		= "davinci-mmc",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(da8xx_mmc_resources),
	.resource	= da8xx_mmc_resources,
};

static struct resource da8xx_rtc_resources[] = {
	[0] = {		 /* registers */
		.start	= DA8XX_RTC_BASE,
		.end	= DA8XX_RTC_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {		 /* interrupt */
		.start	= IRQ_DA8XX_RTC,
		.end	= IRQ_DA8XX_RTC,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device da8xx_rtc_device = {
	.name		= "rtc-da8xx",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(da8xx_rtc_resources),
	.resource	= da8xx_rtc_resources,
};

static struct resource da8xx_eqep_resources0[] = {
	[0] = {		 /* registers */
		.start	= DA8XX_EQEP0_BASE,
		.end	= DA8XX_EQEP0_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {		 /* interrupt */
		.start	= IRQ_DA8XX_EQEP0,
		.end	= IRQ_DA8XX_EQEP0,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device da8xx_eqep_device0 = {
	.name		= "eqep",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(da8xx_eqep_resources0),
	.resource	= da8xx_eqep_resources0,
};

static struct resource da8xx_eqep_resources1[] = {
	[0] = {		 /* registers */
		.start	= DA8XX_EQEP1_BASE,
		.end	= DA8XX_EQEP1_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {		 /* interrupt */
		.start	= IRQ_DA8XX_EQEP1,
		.end	= IRQ_DA8XX_EQEP1,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device da8xx_eqep_device1 = {
	.name		= "eqep",
	.id		= 1,
	.num_resources	= ARRAY_SIZE(da8xx_eqep_resources1),
	.resource	= da8xx_eqep_resources1,
};

static struct resource da8xx_lcdc_resources[] = {
	[0] = {		/* registers */
		.start	= DA8XX_LCD_CNTRL_BASE,
		.end	= DA8XX_LCD_CNTRL_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1]= {		/* interrupt */
		.start	= IRQ_DA8XX_LCDINT,
		.end	= IRQ_DA8XX_LCDINT,
		.flags	= IORESOURCE_IRQ,
	},
};

static struct platform_device da8xx_lcdc_device = {
	.name		= "da8xx_lcdc",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(da8xx_lcdc_resources),
	.resource	= da8xx_lcdc_resources,
};

static struct platform_device *da8xx_devices[DA8XX_PDEV_COUNT] __initdata = {
	[DA8XX_PDEV_SERIAL]	= &da8xx_serial_device,
	[DA8XX_PDEV_I2C_0]	= &da8xx_i2c_device0,
	[DA8XX_PDEV_I2C_1]	= &da8xx_i2c_device1,
	[DA8XX_PDEV_WATCHDOG]	= &da8xx_watchdog_device,
	[DA8XX_PDEV_USB_20]	= &da8xx_usb20_device,
	[DA8XX_PDEV_USB_11]	= &da8xx_usb11_device,
	[DA8XX_PDEV_EMAC]	= &da8xx_emac_device,
	[DA8XX_PDEV_MMC]	= &da8xx_mmc_device,
	[DA8XX_PDEV_RTC]	= &da8xx_rtc_device,
	[DA8XX_PDEV_EQEP_0]	= &da8xx_eqep_device0,
	[DA8XX_PDEV_EQEP_1]	= &da8xx_eqep_device1,
	[DA8XX_PDEV_LCDC]	= &da8xx_lcdc_device,
};

int __init da8xx_add_devices(void *pdata[DA8XX_PDEV_COUNT])
{
	int i;

	for (i = 0; i < DA8XX_PDEV_COUNT; i++)
		if (pdata[i] != NULL)
			da8xx_devices[i]->dev.platform_data = pdata[i];

	return platform_add_devices(da8xx_devices, ARRAY_SIZE(da8xx_devices));
}

void __init da8xx_irq_init(u8 *irq_prio)
{
	cp_intc_init((void __iomem *)CP_INTC_VIRT, DA8XX_N_CP_INTC_IRQ,
		     irq_prio);
}

void __init da8xx_uart_clk_init(u32 uartclk)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(da8xx_serial_pdata) - 1; i++)
		da8xx_serial_pdata[i].uartclk = uartclk;
}

#ifdef CONFIG_KGDB_8250
void __init da8xx_kgdb_init(void)
{
	kgdb8250_add_platform_port(CONFIG_KGDB_PORT_NUM,
		&da8xx_serial_pdata[CONFIG_KGDB_PORT_NUM]);
}
#endif
