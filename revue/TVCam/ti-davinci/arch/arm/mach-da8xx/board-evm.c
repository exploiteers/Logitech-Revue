/*
 * TI DA8xx EVM board.
 *
 * Copyright (C) 2008-2009 MontaVista Software, Inc. <source@mvista.com>
 *
 * Derived from: arch/arm/mach-davinci/board-evm.c
 * Copyright (C) 2006 Texas Instruments.
 * Copyright (C) 2007-2008 MontaVista Software, Inc.
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 *
 */
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/serial.h>
#include <linux/serial_8250.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/console.h>
#include <linux/davinci_emac.h>
#include <linux/usb/musb.h>
#include <linux/mtd/latch-addr-flash.h>
#include <linux/mtd/partitions.h>

#include <asm/hardware.h>
#include <asm/mach-types.h>
#include <asm/arch/i2c.h>
#include <asm/mach/arch.h>
#include <asm/arch/edma.h>
#include <asm/arch/mux.h>
#include <asm/arch/gpio.h>
#include <asm/arch/clock.h>
#include <asm/arch/nand.h>
#include <asm/arch/common.h>
#include <asm/arch/mmc.h>
#include <asm/arch/usb.h>
#include <asm/arch/da8xx_lcdc.h>
#include <asm/arch/i2c-client.h>

#include "da8xx.h"
#include "devices.h"

#define DAVINCI_DA8XX_UART_CLK		150000000

#if defined(CONFIG_DA8XX_UI_NAND)

static struct mtd_partition da8xx_evm_nand_partitions[] = {
	/* bootloader (U-Boot, etc) in first sector */
	[0] = {
		.name		= "bootloader",
		.offset		= 0,
		.size		= SZ_128K,
		.mask_flags	= MTD_WRITEABLE,	/* force read-only */
	},
	/* bootloader params in the next sector */
	[1] = {
		.name		= "params",
		.offset		= MTDPART_OFS_APPEND,
		.size		= SZ_128K,
		.mask_flags	= MTD_WRITEABLE,	/* force read-only */
	},
	/* kernel */
	[2] = {
		.name		= "kernel",
		.offset		= MTDPART_OFS_APPEND,
		.size		= SZ_2M,
		.mask_flags	= 0,
	},
	/* file system */
	[3] = {
		.name		= "filesystem",
		.offset		= MTDPART_OFS_APPEND,
		.size		= MTDPART_SIZ_FULL,
		.mask_flags	= 0,
	}
};

/* flash bbt decriptors */
static uint8_t da8xx_evm_nand_bbt_pattern[] = { 'B', 'b', 't', '0' };
static uint8_t da8xx_evm_nand_mirror_pattern[] = { '1', 't', 'b', 'B' };

static struct nand_bbt_descr da8xx_evm_nand_bbt_main_descr = {
	.options	= NAND_BBT_LASTBLOCK | NAND_BBT_CREATE |
			  NAND_BBT_WRITE | NAND_BBT_2BIT |
			  NAND_BBT_VERSION | NAND_BBT_PERCHIP,
	.offs		= 2,
	.len		= 4,
	.veroffs	= 16,
	.maxblocks	= 4,
	.pattern	= da8xx_evm_nand_bbt_pattern
};

static struct nand_bbt_descr da8xx_evm_nand_bbt_mirror_descr = {
	.options	= NAND_BBT_LASTBLOCK | NAND_BBT_CREATE |
			  NAND_BBT_WRITE | NAND_BBT_2BIT |
			  NAND_BBT_VERSION | NAND_BBT_PERCHIP,
	.offs		= 2,
	.len		= 4,
	.veroffs	= 16,
	.maxblocks	= 4,
	.pattern	= da8xx_evm_nand_mirror_pattern
};

static struct nand_davinci_platform_data da8xx_evm_nand_pdata = {
	.options	= 0,
	.ecc_mode	= NAND_ECC_HW_SYNDROME,
	.cle_mask	= 0x10,
	.ale_mask	= 0x08,
	.bbt_td		= &da8xx_evm_nand_bbt_main_descr,
	.bbt_md		= &da8xx_evm_nand_bbt_mirror_descr,
	.parts		= da8xx_evm_nand_partitions,
	.nr_parts	= ARRAY_SIZE(da8xx_evm_nand_partitions),
};

#define	SZ_32K	(32 * 1024)

static struct resource da8xx_evm_nand_resources[] = {
	[0] = {		/* First memory resource is AEMIF control registers */
		.start	= DA8XX_EMIF25_CONTROL_BASE,
		.end	= DA8XX_EMIF25_CONTROL_BASE + SZ_32K - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {		/* Second memory resource is NAND I/O window */
		.start	= DA8XX_EMIF25_ASYNC_DATA_CE3_BASE,
		.end	= DA8XX_EMIF25_ASYNC_DATA_CE3_BASE + PAGE_SIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
};

static struct platform_device da8xx_evm_nand_device = {
	.name		= "nand_davinci",
	.id		= 0,
	.dev		= {
		.platform_data	= &da8xx_evm_nand_pdata,
	},
	.num_resources	= ARRAY_SIZE(da8xx_evm_nand_resources),
	.resource	= da8xx_evm_nand_resources,
};

#elif defined(CONFIG_DA8XX_UI_NOR)

#define	SZ_32K	(32 * 1024)

#define NOR_WINDOW_SIZE_LOG	15
#define NOR_WINDOW_SIZE 	(1 << NOR_WINDOW_SIZE_LOG)

static struct {
	struct clk *clk;
	struct {
		struct resource *res;
		void __iomem *addr;
	} latch, aemif;
} da8xx_evm_nor;

static void da8xx_evm_nor_set_window(unsigned long offset, void *data)
{
	writeb(0, da8xx_evm_nor.latch.addr +
	       (~3UL & (offset >> (NOR_WINDOW_SIZE_LOG - 2))));
}

static void da8xx_evm_nor_done(void *data)
{
	if (da8xx_evm_nor.clk != NULL) {
		clk_disable(da8xx_evm_nor.clk);
		clk_put(da8xx_evm_nor.clk);
		da8xx_evm_nor.clk  = NULL;
	}
	if (da8xx_evm_nor.latch.addr != NULL) {
		iounmap(da8xx_evm_nor.latch.addr);
		da8xx_evm_nor.latch.addr = NULL;
	}
	if (da8xx_evm_nor.latch.res != NULL) {
		release_mem_region(DA8XX_EMIF25_ASYNC_DATA_CE3_BASE, PAGE_SIZE);
		da8xx_evm_nor.latch.res = NULL;
	}
	if (da8xx_evm_nor.aemif.addr != NULL) {
		iounmap(da8xx_evm_nor.aemif.addr);
		da8xx_evm_nor.aemif.addr = NULL;
	}
	if (da8xx_evm_nor.aemif.res != NULL) {
		release_mem_region(DA8XX_EMIF25_CONTROL_BASE, SZ_32K);
		da8xx_evm_nor.aemif.res = NULL;
	}
}

static int da8xx_evm_nor_init(void *data)
{
	/*
	 * Turn on AEMIF clocks
	 */
	da8xx_evm_nor.clk = clk_get(NULL, "AEMIFCLK");
	if (IS_ERR(da8xx_evm_nor.clk)) {
		printk(KERN_ERR "%s: could not get AEMIF clock\n", __func__);
		da8xx_evm_nor.clk = NULL;
		return -ENODEV;
	}
	clk_enable(da8xx_evm_nor.clk);

	da8xx_evm_nor.aemif.res = request_mem_region(DA8XX_EMIF25_CONTROL_BASE,
						     SZ_32K, "AEMIF control");
	if (da8xx_evm_nor.aemif.res == NULL) {
		printk(KERN_ERR	"%s: could not request AEMIF control region\n",
		       __func__);
		da8xx_evm_nor_done(data);
		return -EBUSY;
	}

	da8xx_evm_nor.aemif.addr = ioremap_nocache(DA8XX_EMIF25_CONTROL_BASE,
						   SZ_32K);
	if (da8xx_evm_nor.aemif.addr == NULL) {
		printk(KERN_ERR	"%s: could not remap AEMIF control region\n",
		       __func__);
		da8xx_evm_nor_done(data);
		return -ENOMEM;
	}

	/*
	 * Setup AEMIF -- timings, etc.
	 */

	/* Set maximum wait cycles */
	writel(0xff, da8xx_evm_nor.aemif.addr + ASYNC_EMIF_AWCCR);

	/* Set the async memory config register for NOR flash */
	writel(0x00300608, da8xx_evm_nor.aemif.addr + ASYNC_EMIF_A1CR);

	/* Set the async memory config register for the latch */
	writel(0x00300388, da8xx_evm_nor.aemif.addr + ASYNC_EMIF_A2CR);

	/*
	 * Setup the window to access the latch
	 */
	da8xx_evm_nor.latch.res =
		request_mem_region(DA8XX_EMIF25_ASYNC_DATA_CE3_BASE, PAGE_SIZE,
				   "DA8xx UI NOR address latch");
	if (da8xx_evm_nor.latch.res == NULL) {
		printk(KERN_ERR	"%s: could not request address latch region\n",
		       __func__);
		da8xx_evm_nor_done(data);
		return -EBUSY;
	}

	da8xx_evm_nor.latch.addr =
		ioremap_nocache(DA8XX_EMIF25_ASYNC_DATA_CE3_BASE, PAGE_SIZE);
	if (da8xx_evm_nor.latch.addr == NULL) {
		printk(KERN_ERR	"%s: could not remap address latch region\n",
		       __func__);
		da8xx_evm_nor_done(data);
		return -ENOMEM;
	}

	return 0;
}

static struct mtd_partition da8xx_evm_nor_partitions[] = {
       /* bootloader (U-Boot, etc) in first 2 sectors */
       [0] = {
	       .name           = "bootloader",
	       .offset         = 0,
	       .size           = SZ_128K,
	       .mask_flags     = MTD_WRITEABLE, /* force read-only */
       },
       /* bootloader parameters in the next 1 sector */
       [1] = {
	       .name           = "params",
	       .offset         = MTDPART_OFS_APPEND,
	       .size           = SZ_64K,
	       .mask_flags     = 0,
       },
       /* kernel */
       [2] = {
	       .name           = "kernel",
	       .offset         = MTDPART_OFS_APPEND,
	       .size           = SZ_2M,
	       .mask_flags     = 0
       },
       /* file system */
       [3] = {
	       .name           = "filesystem",
	       .offset         = MTDPART_OFS_APPEND,
	       .size           = MTDPART_SIZ_FULL,
	       .mask_flags     = 0
       }
};

static struct latch_addr_flash_data da8xx_evm_nor_pdata = {
	.width		= 1,
	.size		= SZ_4M,
	.init		= da8xx_evm_nor_init,
	.done		= da8xx_evm_nor_done,
	.set_window	= da8xx_evm_nor_set_window,
	.nr_parts	= ARRAY_SIZE(da8xx_evm_nor_partitions),
	.parts		= da8xx_evm_nor_partitions,
};

static struct resource da8xx_evm_nor_resource[] = {
	[0] = {
		.start	= DA8XX_EMIF25_ASYNC_DATA_CE2_BASE,
		.end	= DA8XX_EMIF25_ASYNC_DATA_CE2_BASE +
			  NOR_WINDOW_SIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= DA8XX_EMIF25_ASYNC_DATA_CE3_BASE,
		.end	= DA8XX_EMIF25_ASYNC_DATA_CE3_BASE + PAGE_SIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
	[2] = {
		.start	= DA8XX_EMIF25_CONTROL_BASE,
		.end	= DA8XX_EMIF25_CONTROL_BASE + SZ_32K - 1,
		.flags	= IORESOURCE_MEM,
	}
};

static struct platform_device da8xx_evm_nor_device = {
	.name		= "latch-addr-flash",
	.id		= 0,
	.dev		= {
		.platform_data	= &da8xx_evm_nor_pdata,
	},
	.num_resources	= ARRAY_SIZE(da8xx_evm_nor_resource),
	.resource	= da8xx_evm_nor_resource,
};

#endif	/* CONFIG_DA8XX_UI_NOR */

static struct platform_device *da8xx_evm_devices[] __initdata = {
#if defined(CONFIG_DA8XX_UI_NAND)
	&da8xx_evm_nand_device,
#elif defined(CONFIG_DA8XX_UI_NOR)
	&da8xx_evm_nor_device,
#endif
};

static struct davinci_i2c_platform_data da8xx_evm_i2c_pdata0 = {
	.bus_freq	= 100,
	.bus_delay	= 0,
};

static struct musb_hdrc_platform_data da8xx_evm_usb20_pdata = {
#if defined(CONFIG_USB_MUSB_OTG)
	/* OTG requires a Mini-AB connector */
	.mode		= MUSB_OTG,
#elif defined(CONFIG_USB_MUSB_PERIPHERAL)
	.mode		= MUSB_PERIPHERAL,
#elif defined(CONFIG_USB_MUSB_HOST)
	.mode		= MUSB_HOST,
#endif
	/* TPS2065 switch @ 5V */
	.power		= 500 / 2,	/* actually 1 A (sustains 1.5 A) */
	.potpgt		= (3 + 1) / 2,	/* 3 ms max */

	/* REVISIT multipoint is a _chip_ capability; not board specific */
	.multipoint	= 1,
};

#if defined(CONFIG_USB_OHCI_HCD) || defined(CONFIG_USB_OHCI_HCD_MODULE)

#define ON_BD_USB_DRV	GPIO(31)	/* GPIO1[15]	*/
#define ON_BD_USB_OVC	GPIO(36)	/* GPIO2[4]	*/

static da8xx_ocic_handler_t da8xx_evm_usb_ocic_handler;

static int da8xx_evm_usb_set_power(unsigned port, int on)
{
	gpio_set_value(ON_BD_USB_DRV, on);
	return 0;
}

static int da8xx_evm_usb_get_power(unsigned port)
{
	return gpio_get_value(ON_BD_USB_DRV);
}

static int da8xx_evm_usb_get_oci(unsigned port)
{
	return !gpio_get_value(ON_BD_USB_OVC);
}

static irqreturn_t da8xx_evm_usb_ocic_irq(int, void *, struct pt_regs *);

static int da8xx_evm_usb_ocic_notify(da8xx_ocic_handler_t handler)
{
	int irq 	= gpio_to_irq(ON_BD_USB_OVC);
	int error	= 0;

	if (handler != NULL) {
		da8xx_evm_usb_ocic_handler = handler;

		error = request_irq(irq, da8xx_evm_usb_ocic_irq, IRQF_DISABLED |
				    IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
				    "OHCI over-current indicator", NULL);
		if (error)
			printk(KERN_ERR "%s: could not request IRQ to watch "
			       "over-current indicator changes\n", __func__);
	} else
		free_irq(irq, NULL);

	return error;
}

static struct da8xx_ohci_root_hub da8xx_evm_usb11_pdata = {
	.set_power	= da8xx_evm_usb_set_power,
	.get_power	= da8xx_evm_usb_get_power,
	.get_oci	= da8xx_evm_usb_get_oci,
	.ocic_notify	= da8xx_evm_usb_ocic_notify,

	/* TPS2065 switch @ 5V */
	.potpgt		= (3 + 1) / 2,	/* 3 ms max */
};

static irqreturn_t da8xx_evm_usb_ocic_irq(int irq, void *dev_id,
					  struct pt_regs *regs)
{
	da8xx_evm_usb_ocic_handler(&da8xx_evm_usb11_pdata, 1);
	return IRQ_HANDLED;
}
#endif /* defined(CONFIG_USB_OHCI_HCD) || defined(CONFIG_USB_OHCI_HCD_MODULE) */
static struct emac_init_config da8xx_evm_emac_pdata = {
	.reset_line		= 0,
	.emac_bus_frequency	= 148500000,
	.mdio_reset_line	= 0,
	.mdio_bus_frequency	= 148500000,
	.mdio_clock_frequency	= 2200000,
	.registers_old		= 0,
	.gigabit		= 0,
	.phy_mode		= -1,
};

static int da8xx_evm_mmc_get_ro(int index)
{
	int val, status, gpio_num = 33;

	status = gpio_request(gpio_num, "MMC WP\n");
	if (status < 0) {
		printk(KERN_WARNING "%s can not open GPIO %d\n", __func__,
				gpio_num);
		return 0;
	}
	gpio_direction_input(gpio_num);
	val = gpio_get_value(gpio_num);
	gpio_free(gpio_num);
	return val;
}

static struct davinci_mmc_platform_data da8xx_evm_mmc_pdata = {
	.mmc_clk		= "MMC_SD_CLK",
	.rw_threshold		= 64,
	.use_4bit_mode		= 1,
	.use_8bit_mode		= 1,
	.max_frq		= 50000000,
	.pio_set_dmatrig	= 1,
	.get_ro			= da8xx_evm_mmc_get_ro,
};

static struct da8xx_lcdc_platform_data da8xx_evm_lcdc_pdata = {
	.lcdc_clk_name	= "LCDCTRLCLK",
};

static void *da8xx_evm_soc_pdata[DA8XX_PDEV_COUNT] __initdata = {
	[DA8XX_PDEV_I2C_0]	= &da8xx_evm_i2c_pdata0,
	[DA8XX_PDEV_USB_20]	= &da8xx_evm_usb20_pdata,
	[DA8XX_PDEV_EMAC]	= &da8xx_evm_emac_pdata,
	[DA8XX_PDEV_MMC]	= &da8xx_evm_mmc_pdata,
	[DA8XX_PDEV_LCDC]	= &da8xx_evm_lcdc_pdata,
};

static __init void da8xx_evm_usb_init(void)
{
	u32 cfgchip2;

	/*
	 * Set up USB clock/mode in the CFGCHIP2 register.
	 * FYI:  CFGCHIP2 is 0x0000ef00 initially.
	 */
	cfgchip2 = davinci_cfg_readl(DA8XX_CFGCHIP(2));

	/* USB2.0 PHY reference clock is 24 MHz */
	cfgchip2 &= ~CFGCHIP2_REFFREQ;
	cfgchip2 |=  CFGCHIP2_REFFREQ_24MHZ;

	/*
	 * Select internal reference clock for USB 2.0 PHY
	 * and use it as a clock source for USB 1.1 PHY
	 * (this is the default setting anyway).
	 */
	cfgchip2 &= ~CFGCHIP2_USB1PHYCLKMUX;
	cfgchip2 |=  CFGCHIP2_USB2PHYCLKMUX;

	/*
	 * We have to override VBUS/ID signals when MUSB is configured into the
	 * host-only mode -- ID pin will float if no cable is connected, so the
	 * controller won't be able to drive VBUS thinking that it's a B-device.
	 * Otherwise, we want to use the OTG mode and enable VBUS comparators.
	 */
	cfgchip2 &= ~CFGCHIP2_OTGMODE;
#ifdef	CONFIG_USB_MUSB_HOST
	cfgchip2 |=  CFGCHIP2_FORCE_HOST;
#else
	cfgchip2 |=  CFGCHIP2_SESENDEN | CFGCHIP2_VBDTCTEN;
#endif

	davinci_cfg_writel(cfgchip2, DA8XX_CFGCHIP(2));

#if defined(CONFIG_USB_OHCI_HCD) || defined(CONFIG_USB_OHCI_HCD_MODULE)

	if (gpio_request(ON_BD_USB_DRV, "ON_BD_USB_DRV") ||
	    gpio_request(ON_BD_USB_OVC, "ON_BD_USB_OVC")) {
		printk(KERN_ERR "%s: could not request GPIOs for USB1 port "
		       "power control and over-current indicator\n", __func__);

		return;
	}
	gpio_direction_output(ON_BD_USB_DRV, 0);
	gpio_direction_input(ON_BD_USB_OVC);

	da8xx_evm_soc_pdata[DA8XX_PDEV_USB_11] = &da8xx_evm_usb11_pdata;

#endif /* defined(CONFIG_USB_OHCI_HCD) || defined(CONFIG_USB_OHCI_HCD_MODULE) */
}

#ifdef	CONFIG_DA8XX_UI

/*
 * Setup the MUX_MODE pin of the GPIO expander located on the UI board:
 * 0 switches AEMIF lines to LCD, 1 (default) to NAND/NOR flashes.
 */
static __init void da8xx_evm_expander_setup(void)
{
#ifdef	CONFIG_DA8XX_UI_LCD
	davinci_i2c_expander_op(MUX_MODE_DA8XX, 0);
#endif
}

static __init int da8xx_evm_expander_validate(enum i2c_expander_pins pin)
{
	switch (pin) {
	case MUX_MODE_DA8XX:
	case SPI_MODE_DA8XX:
		return 0;
	default:
		return  -EINVAL;
	}
}

#endif	/* CONFIG_DA8XX_UI */

static __init void da8xx_evm_init(void)
{
	int ret;

	davinci_gpio_init();

#ifdef	CONFIG_DA8XX_UI
	davinci_i2c_expander.address = 0x3F;
	davinci_i2c_expander.init_data = 0xFF;
	davinci_i2c_expander.name = "da8xx_evm_ui_expander";
	davinci_i2c_expander.setup = da8xx_evm_expander_setup;
	davinci_i2c_expander.validate = da8xx_evm_expander_validate;
#endif

	da8xx_evm_usb_init();

	platform_notify = davinci_serial_init;

	/* Add da8xx EVM specific devices */
	ret = platform_add_devices(da8xx_evm_devices,
				   ARRAY_SIZE(da8xx_evm_devices));
	if (ret)
		printk(KERN_WARNING "DA8xx: DA8xx EVM devices not added.\n");

	/* Add generic da8xx SoC devices */
	ret = da8xx_add_devices(da8xx_evm_soc_pdata);
	if (ret)
		printk(KERN_WARNING "DA8xx: SoC devices not added.\n");
}

/* nFIQ are priorities 0-1, nIRQ are priorities 2-31 (with 31 being lowest) */
static u8 da8xx_evm_default_priorities[DA8XX_N_CP_INTC_IRQ] = {
	[IRQ_DA8XX_COMMTX]		= 7,
	[IRQ_DA8XX_COMMRX]		= 7,
	[IRQ_DA8XX_NINT]		= 7,
	[IRQ_DA8XX_EVTOUT0]		= 7,
	[IRQ_DA8XX_EVTOUT1]		= 7,
	[IRQ_DA8XX_EVTOUT2]		= 7,
	[IRQ_DA8XX_EVTOUT3]		= 7,
	[IRQ_DA8XX_EVTOUT4]		= 7,
	[IRQ_DA8XX_EVTOUT5]		= 7,
	[IRQ_DA8XX_EVTOUT6]		= 7,
	[IRQ_DA8XX_EVTOUT6]		= 7,
	[IRQ_DA8XX_EVTOUT7]		= 7,
	[IRQ_DA8XX_CCINT0]		= 7,
	[IRQ_DA8XX_CCERRINT]		= 7,
	[IRQ_DA8XX_TCERRINT0]		= 7,
	[IRQ_DA8XX_AEMIFINT]		= 7,
	[IRQ_DA8XX_I2CINT0]		= 7,
	[IRQ_DA8XX_MMCSDINT0]		= 7,
	[IRQ_DA8XX_MMCSDINT1]		= 7,
	[IRQ_DA8XX_ALLINT0]		= 7,
	[IRQ_DA8XX_RTC]			= 7,
	[IRQ_DA8XX_SPINT0]		= 7,
	[IRQ_DA8XX_TINT12_0]		= 7,
	[IRQ_DA8XX_TINT34_0]		= 7,
	[IRQ_DA8XX_TINT12_1]		= 7,
	[IRQ_DA8XX_TINT34_1]		= 7,
	[IRQ_DA8XX_UARTINT0]		= 7,
	[IRQ_DA8XX_KEYMGRINT]		= 7,
	[IRQ_DA8XX_SECINT]		= 7,
	[IRQ_DA8XX_SECKEYERR]		= 7,
	[IRQ_DA8XX_MPUERR]		= 7,
	[IRQ_DA8XX_IOPUERR]		= 7,
	[IRQ_DA8XX_BOOTCFGERR]		= 7,
	[IRQ_DA8XX_CHIPINT0]		= 7,
	[IRQ_DA8XX_CHIPINT1]		= 7,
	[IRQ_DA8XX_CHIPINT2]		= 7,
	[IRQ_DA8XX_CHIPINT3]		= 7,
	[IRQ_DA8XX_TCERRINT1]		= 7,
	[IRQ_DA8XX_C0_RX_THRESH_PULSE]	= 7,
	[IRQ_DA8XX_C0_RX_PULSE]		= 7,
	[IRQ_DA8XX_C0_TX_PULSE]		= 7,
	[IRQ_DA8XX_C0_MISC_PULSE]	= 7,
	[IRQ_DA8XX_C1_RX_THRESH_PULSE]	= 7,
	[IRQ_DA8XX_C1_RX_PULSE]		= 7,
	[IRQ_DA8XX_C1_TX_PULSE]		= 7,
	[IRQ_DA8XX_C1_MISC_PULSE]	= 7,
	[IRQ_DA8XX_MEMERR]		= 7,
	[IRQ_DA8XX_GPIO0]		= 7,
	[IRQ_DA8XX_GPIO1]		= 7,
	[IRQ_DA8XX_GPIO2]		= 7,
	[IRQ_DA8XX_GPIO3]		= 7,
	[IRQ_DA8XX_GPIO4]		= 7,
	[IRQ_DA8XX_GPIO5]		= 7,
	[IRQ_DA8XX_GPIO6]		= 7,
	[IRQ_DA8XX_GPIO7]		= 7,
	[IRQ_DA8XX_GPIO8]		= 7,
	[IRQ_DA8XX_I2CINT1]		= 7,
	[IRQ_DA8XX_LCDINT]		= 7,
	[IRQ_DA8XX_UARTINT1]		= 7,
	[IRQ_DA8XX_MCASPINT]		= 7,
	[IRQ_DA8XX_ALLINT1]		= 7,
	[IRQ_DA8XX_SPINT1]		= 7,
	[IRQ_DA8XX_UHPI_INT1]		= 7,
	[IRQ_DA8XX_USB_INT]		= 7,
	[IRQ_DA8XX_IRQN]		= 7,
	[IRQ_DA8XX_RWAKEUP]		= 7,
	[IRQ_DA8XX_UARTINT2]		= 7,
	[IRQ_DA8XX_DFTSSINT]		= 7,
	[IRQ_DA8XX_EHRPWM0]		= 7,
	[IRQ_DA8XX_EHRPWM0TZ]		= 7,
	[IRQ_DA8XX_EHRPWM1]		= 7,
	[IRQ_DA8XX_EHRPWM1TZ]		= 7,
	[IRQ_DA8XX_EHRPWM2]		= 7,
	[IRQ_DA8XX_EHRPWM2TZ]		= 7,
	[IRQ_DA8XX_ECAP0]		= 7,
	[IRQ_DA8XX_ECAP1]		= 7,
	[IRQ_DA8XX_ECAP2]		= 7,
	[IRQ_DA8XX_EQEP0]		= 7,
	[IRQ_DA8XX_EQEP1]		= 7,
	[IRQ_DA8XX_T12CMPINT0_0]	= 7,
	[IRQ_DA8XX_T12CMPINT1_0]	= 7,
	[IRQ_DA8XX_T12CMPINT2_0]	= 7,
	[IRQ_DA8XX_T12CMPINT3_0]	= 7,
	[IRQ_DA8XX_T12CMPINT4_0]	= 7,
	[IRQ_DA8XX_T12CMPINT5_0]	= 7,
	[IRQ_DA8XX_T12CMPINT6_0]	= 7,
	[IRQ_DA8XX_T12CMPINT7_0]	= 7,
	[IRQ_DA8XX_T12CMPINT0_1]	= 7,
	[IRQ_DA8XX_T12CMPINT1_1]	= 7,
	[IRQ_DA8XX_T12CMPINT2_1]	= 7,
	[IRQ_DA8XX_T12CMPINT3_1]	= 7,
	[IRQ_DA8XX_T12CMPINT4_1]	= 7,
	[IRQ_DA8XX_T12CMPINT5_1]	= 7,
	[IRQ_DA8XX_T12CMPINT6_1]	= 7,
	[IRQ_DA8XX_T12CMPINT7_1]	= 7,
	[IRQ_DA8XX_ARMCLKSTOPREQ]	= 7,
};

static __init void da8xx_evm_irq_init(void)
{
	da8xx_irq_init(da8xx_evm_default_priorities);
}

#ifdef CONFIG_DAVINCI_MUX

/*
 * UI board NAND/NOR flashes only use 8-bit data bus.
 */
static const short evm_emif25_pins[] = {
	DA8XX_EMA_D_0, DA8XX_EMA_D_1, DA8XX_EMA_D_2, DA8XX_EMA_D_3,
	DA8XX_EMA_D_4, DA8XX_EMA_D_5, DA8XX_EMA_D_6, DA8XX_EMA_D_7,
	DA8XX_EMA_A_0, DA8XX_EMA_A_1, DA8XX_EMA_A_2, DA8XX_EMA_A_3,
	DA8XX_EMA_A_4, DA8XX_EMA_A_5, DA8XX_EMA_A_6, DA8XX_EMA_A_7,
	DA8XX_EMA_A_8, DA8XX_EMA_A_9, DA8XX_EMA_A_10, DA8XX_EMA_A_11,
	DA8XX_EMA_A_12, DA8XX_EMA_BA_0, DA8XX_EMA_BA_1, DA8XX_NEMA_WE,
	DA8XX_NEMA_CS_2, DA8XX_NEMA_CS_3, DA8XX_NEMA_OE, DA8XX_EMA_WAIT_0,
	-1
};

/*
 * GPIO2[1] is used as MMC_SD_WP and GPIO2[2] as MMC_SD_INS.
 */
static const short evm_mmc_sd_pins[] = {
	DA8XX_MMCSD_DAT_0, DA8XX_MMCSD_DAT_1, DA8XX_MMCSD_DAT_2,
	DA8XX_MMCSD_DAT_3, DA8XX_MMCSD_DAT_4, DA8XX_MMCSD_DAT_5,
	DA8XX_MMCSD_DAT_6, DA8XX_MMCSD_DAT_7, DA8XX_MMCSD_CLK,
	DA8XX_MMCSD_CMD,   DA8XX_GPIO2_1,     DA8XX_GPIO2_2,
	-1
};

/*
 * UART0 is not used on the EVM board.
 * Due to the pin conflicts, evm_uart0_pins[] is empty.
 */
static const short evm_uart0_pins[] = {
	-1
};

/*
 * USB_REFCLKIN is not used on the EVM board.
 */
static const short evm_usb20_pins[] = {
	DA8XX_USB0_DRVVBUS,
	-1
};

/*
 * USB1 VBUS is controlled by GPIO1[15], over-current is reported on GPIO2[4].
 */
static const short evm_usb11_pins[] = {
	DA8XX_GPIO1_15, DA8XX_GPIO2_4,
	-1
};

static const short evm_mcasp0_pins[] = {
	DA8XX_AXR0_9, DA8XX_AFSR0, DA8XX_ACLKR0,
	-1
};

static const short evm_mcasp1_pins[] = {
	DA8XX_AHCLKX1, DA8XX_ACLKX1, DA8XX_AFSX1, DA8XX_AHCLKR1, DA8XX_AFSR1,
	DA8XX_AMUTE1, DA8XX_AXR1_0, DA8XX_AXR1_1, DA8XX_AXR1_2, DA8XX_AXR1_5,
	DA8XX_ACLKR1, DA8XX_AXR1_6, DA8XX_AXR1_7, DA8XX_AXR1_8, DA8XX_AXR1_10,
	DA8XX_AXR1_11,
	-1
};

static const short evm_mcasp2_pins[] = {
	DA8XX_AXR2_0,
	-1
};

/*
 * GPIO3[10] is used for SPI1 chip select instead of NSPI1_ENA due to the pin
 * conflicts with UART2.
 */
static const short evm_spi1_pins[] = {
	DA8XX_SPI1_SOMI_0, DA8XX_SPI1_SIMO_0, DA8XX_SPI1_CLK, DA8XX_GPIO3_10,
	-1
};

/*
 * I2C1_SCL and I2C1_SDA are not used on the EVM board.  Both are removed due
 * to SPI1_SOMI_0 vs I2C1_SCL pin conflict.
 */
static const short evm_i2c_pins[] = {
	DA8XX_I2C0_SDA, DA8XX_I2C0_SCL,
	-1
};

/*
 * UART1 is not used on the EVM board.
 * Due to the pin conflicts, evm_uart1_pins[] is empty.
 */
static const short evm_uart1_pins[] = {
	-1
};

static void da8xx_evm_pinmux_override(void)
{
	da8xx_psc0_pins[DA8XX_LPSC0_EMIF25] = evm_emif25_pins;
	da8xx_psc0_pins[DA8XX_LPSC0_MMC_SD] = evm_mmc_sd_pins;

	da8xx_psc0_pins[DA8XX_LPSC0_UART0] = evm_uart0_pins;
	da8xx_psc1_pins[DA8XX_LPSC1_UART1] = evm_uart1_pins;

	da8xx_psc1_pins[DA8XX_LPSC1_USB20] = evm_usb20_pins;
	da8xx_psc1_pins[DA8XX_LPSC1_USB11] = evm_usb11_pins;

	da8xx_psc1_pins[DA8XX_LPSC1_McASP0] = evm_mcasp0_pins;
	da8xx_psc1_pins[DA8XX_LPSC1_McASP1] = evm_mcasp1_pins;
	da8xx_psc1_pins[DA8XX_LPSC1_McASP2] = evm_mcasp2_pins;

	da8xx_psc1_pins[DA8XX_LPSC1_SPI1] = evm_spi1_pins;

	da8xx_psc1_pins[DA8XX_LPSC1_I2C] = evm_i2c_pins;
}
#else
static inline void da8xx_evm_pinmux_override(void) {}
#endif /* CONFIG_DAVINCI_MUX */

static __init void da8xx_evm_psc_init(void)
{
	davinci_psc_config(DAVINCI_GPSC_ARMDOMAIN, 0, DA8XX_LPSC0_TPCC, 1);
	davinci_psc_config(DAVINCI_GPSC_ARMDOMAIN, 0, DA8XX_LPSC0_TPTC0, 1);
	davinci_psc_config(DAVINCI_GPSC_ARMDOMAIN, 0, DA8XX_LPSC0_TPTC1, 1);
	davinci_psc_config(DAVINCI_GPSC_ARMDOMAIN, 0, DA8XX_LPSC0_AINTC, 1);
	davinci_psc_config(DAVINCI_GPSC_ARMDOMAIN, 0, DA8XX_LPSC0_ARM_RAM_ROM,
			   1);
	davinci_psc_config(DAVINCI_GPSC_ARMDOMAIN, 0, DA8XX_LPSC0_SECU_MGR, 1);
	davinci_psc_config(DAVINCI_GPSC_ARMDOMAIN, 0, DA8XX_LPSC0_SCR0_SS, 1);
	davinci_psc_config(DAVINCI_GPSC_ARMDOMAIN, 0, DA8XX_LPSC0_SCR1_SS, 1);
	davinci_psc_config(DAVINCI_GPSC_ARMDOMAIN, 0, DA8XX_LPSC0_SCR2_SS, 1);
	davinci_psc_config(DAVINCI_GPSC_ARMDOMAIN, 0, DA8XX_LPSC0_DMAX, 1);
#if defined(CONFIG_TI_CPPI41) || defined(CONFIG_TI_CPPI41_MODULE)
	davinci_psc_config(DAVINCI_GPSC_ARMDOMAIN, 1, DA8XX_LPSC1_USB20, 1);
#endif
#ifdef	CONFIG_INPUT_EQEP
	davinci_psc_config(DAVINCI_GPSC_ARMDOMAIN, 1, DA8XX_LPSC1_EQEP, 1);
#endif
}

static void __init da8xx_evm_map_io(void)
{
	da8xx_evm_pinmux_override();
	da8xx_map_common_io();

	da8xx_unlock_cfg_regs();

	/* UART clock needs to be ready otherwise kgdbwait won't work */
	da8xx_uart_clk_init(DAVINCI_DA8XX_UART_CLK);
	da8xx_kgdb_init();

	/* Initialize the DA8XX EVM board settigs */
	da8xx_init_common_hw();
	da8xx_evm_psc_init();
}

#ifdef CONFIG_SERIAL_8250_CONSOLE
static int __init da8xx_evm_console_init(void)
{
	return add_preferred_console("ttyS", 2, "115200");
}
console_initcall(da8xx_evm_console_init);
#endif

MACHINE_START(DA8XX_EVM, "DaVinci DA8XX EVM")
	.phys_io	=  IO_PHYS,
	.io_pg_offst	= (IO_VIRT >> 18) & 0xfffc,
	.boot_params	= DA8XX_DDR_BASE + 0x100,
	.map_io		= da8xx_evm_map_io,
	.init_irq	= da8xx_evm_irq_init,
	.timer		= &davinci_timer,
	.init_machine	= da8xx_evm_init,
MACHINE_END
