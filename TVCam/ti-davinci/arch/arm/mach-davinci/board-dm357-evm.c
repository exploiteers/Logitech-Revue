/*
 * TI DaVinci EVM board support
 *
 * Author: Kevin Hilman, MontaVista Software, Inc. <source@mvista.com>
 *
 * (C) 2007-2008 MontaVista Software, Inc.
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 *
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/physmap.h>
#include <linux/serial.h>
#include <linux/serial_8250.h>
#include <linux/io.h>

#include <asm/setup.h>
#include <asm/mach-types.h>
#include <asm/hardware.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/flash.h>

#include <asm/arch/common.h>
#include <asm/arch/gpio.h>
#include <asm/arch/mux.h>
#include <asm/arch/clock.h>
#include <asm/arch/nand.h>
#include <asm/arch/mmc.h>
#include <asm/arch/edma.h>
#include <asm/arch/i2c-client.h>

/* other misc. init functions */
void __init davinci_irq_init(void);
void __init davinci_map_common_io(void);
void __init davinci_init_common_hw(void);

/*
 * Serial
 */

static struct plat_serial8250_port serial_platform_data[] = {
	{
		.membase	= (char *)IO_ADDRESS(DAVINCI_UART0_BASE),
		.mapbase	= (unsigned long)DAVINCI_UART0_BASE,
		.irq		= IRQ_UARTINT0,
		.flags		= UPF_BOOT_AUTOCONF | UPF_SKIP_TEST,
		.iotype		= UPIO_MEM,
		.regshift	= 2,
		.uartclk	= 27000000,
	},
	{
		.flags		= 0
	},
};

static struct platform_device serial_device = {
	.name			= "serial8250",
	.id			= PLAT8250_DEV_PLATFORM,
	.dev			= {
		.platform_data	= serial_platform_data,
	},
};

#ifdef CONFIG_DM357_STORAGE_NAND

/*
 * Storage NAND
 */

static struct mtd_partition davinci_nand_partitions_storage[] = {
	{
		.name		= "filesystem1",
		.offset		= MTDPART_OFS_APPEND,
		.size		= SZ_512M,
		.mask_flags	= 0
	},

	{
		.name		= "filesystem2",
		.offset		= MTDPART_OFS_APPEND,
		.size		= MTDPART_SIZ_FULL,
		.mask_flags	= 0
	}
};

/* flash bbt decriptors */
static uint8_t nand_davinci_bbt_pattern[] = { 'B', 'b', 't', '0' };
static uint8_t nand_davinci_mirror_pattern[] = { '1', 't', 'b', 'B' };

static struct nand_bbt_descr nand_davinci_bbt_main_descr = {
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
	| NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_PERCHIP,
	.offs = 2,
	.len = 4,
	.veroffs = 16,
	.maxblocks = 4,
	.pattern = nand_davinci_bbt_pattern
};

static struct nand_bbt_descr nand_davinci_bbt_mirror_descr = {
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
	| NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_PERCHIP,
	.offs = 2,
	.len = 4,
	.veroffs = 16,
	.maxblocks = 4,
	.pattern = nand_davinci_mirror_pattern
};

static struct nand_davinci_platform_data davinci_nand_data_storage = {
	.options	= 0,
	.ecc_mode	= NAND_ECC_SOFT,
	.cle_mask	= 0x10,
	.ale_mask	= 0x08,
	.bbt_td         = &nand_davinci_bbt_main_descr,
	.bbt_md         = &nand_davinci_bbt_mirror_descr,
	.parts		= davinci_nand_partitions_storage,
	.nr_parts	= ARRAY_SIZE(davinci_nand_partitions_storage),
};

static struct resource davinci_nand_resources_storage[] = {
	[0] = {		/* First memory resource is AEMIF control registers */
		.start	= DM644X_ASYNC_EMIF_CNTRL_BASE,
		.end	= DM644X_ASYNC_EMIF_CNTRL_BASE + SZ_4K - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {         /* Second memory resource is NAND I/O window */
		.start  = DAVINCI_ASYNC_EMIF_DATA_CE2_BASE,
		.end    = DAVINCI_ASYNC_EMIF_DATA_CE2_BASE + SZ_16K - 1,
		.flags  = IORESOURCE_MEM,
	},
	[2] = {		/*
			* Third (optional) memory resource is NAND I/O window
			* for second NAND chip select
			*/
		.start  = DAVINCI_ASYNC_EMIF_DATA_CE2_BASE + SZ_16K,
		.end    = DAVINCI_ASYNC_EMIF_DATA_CE2_BASE + SZ_16K +
				SZ_16K - 1,
		.flags  = IORESOURCE_MEM,
	},
};

static struct platform_device davinci_nand_device_storage = {
	.name			= "nand_davinci",
	.id			= 1,
	.dev			= {
		.platform_data	= &davinci_nand_data_storage,
	},

	.num_resources		= ARRAY_SIZE(davinci_nand_resources_storage),
	.resource		= davinci_nand_resources_storage,
};

#else /* CONFIG_DM357_STORAGE_NAND */

/*
 * Boot NAND
 */

static struct mtd_partition davinci_nand_partitions[] = {
	/* bootloader (U-Boot, etc) in first sector */
	{
		.name		= "bootloader",
		.offset		= 0,
		.size		= SZ_256K,
		.mask_flags	= MTD_WRITEABLE,	/* force read-only */
	},
	/* bootloader params in the next sector */
	{
		.name		= "params",
		.offset		= MTDPART_OFS_APPEND,
		.size		= SZ_128K,
		.mask_flags	= MTD_WRITEABLE,	/* force read-only */
	},
	/* kernel */
	{
		.name		= "kernel",
		.offset		= MTDPART_OFS_APPEND,
		.size		= SZ_4M,
		.mask_flags	= 0,
	},
	/* file system */
	{
		.name		= "filesystem",
		.offset		= MTDPART_OFS_APPEND,
		.size		= MTDPART_SIZ_FULL,
		.mask_flags	= 0,
	}
};

static struct nand_davinci_platform_data davinci_nand_data = {
	.options	= 0,
	.ecc_mode	= NAND_ECC_HW,
	.cle_mask	= 0x10,
	.ale_mask	= 0x08,
	.bbt_td		= NULL,
	.bbt_md		= NULL,
	.parts		= davinci_nand_partitions,
	.nr_parts	= ARRAY_SIZE(davinci_nand_partitions),
};

static struct resource davinci_nand_resources[] = {
	[0] = {		/* First memory resource is AEMIF control registers */
		.start		= DM644X_ASYNC_EMIF_CNTRL_BASE,
		.end		= DM644X_ASYNC_EMIF_CNTRL_BASE + SZ_4K - 1,
		.flags		= IORESOURCE_MEM,
	},
	[1] = {		/* Second memory resource is NAND I/O window */
		.start		= DAVINCI_ASYNC_EMIF_DATA_CE0_BASE,
		.end		= DAVINCI_ASYNC_EMIF_DATA_CE0_BASE + SZ_16K - 1,
		.flags		= IORESOURCE_MEM,
	},
};

static struct platform_device davinci_nand_device = {
	.name			= "nand_davinci",
	.id			= 0,
	.dev			= {
		.platform_data	= &davinci_nand_data,
	},

	.num_resources		= ARRAY_SIZE(davinci_nand_resources),
	.resource		= davinci_nand_resources,
};

#endif /* CONFIG_DM357_STORAGE_NAND */

/*
 * Real Time Clock
 */

static struct platform_device rtc_dev = {
	.name		= "rtc_davinci_evm",
	.id		= -1,
};

/*
 * Framebuffer
 */

static u64 davinci_fb_dma_mask = DMA_32BIT_MASK;

static struct platform_device davinci_fb_device = {
	.name		= "davincifb",
	.id		= -1,
	.dev = {
		.dma_mask		= &davinci_fb_dma_mask,
		.coherent_dma_mask      = DMA_32BIT_MASK,
	},
	.num_resources = 0,
};

/*
 * SD/MMC
 */

static struct resource mmc0_resources[] = {
	[0] = {			/* registers */
		.start		= DAVINCI_MMC_SD_BASE,
		.end		= DAVINCI_MMC_SD_BASE + SZ_1K - 1,
		.flags		= IORESOURCE_MEM,
	},
	[1] = {			/* interrupt */
		.start		= IRQ_MMCINT,
		.end		= IRQ_MMCINT,
		.flags		= IORESOURCE_IRQ,
	},
	[2] = {			/* dma rx */
		.start		= DM644X_DMACH_MMCRXEVT,
		.end		= DM644X_DMACH_MMCRXEVT,
		.flags		= IORESOURCE_DMA | IORESOURCE_DMA_RX_CHAN,
	},
	[3] = {			/* dma tx */
		.start		= DM644X_DMACH_MMCTXEVT,
		.end		= DM644X_DMACH_MMCTXEVT,
		.flags		= IORESOURCE_DMA | IORESOURCE_DMA_TX_CHAN,
	},
	[4] = {			/* event queue */
		.start		= EVENTQ_0,
		.end		= EVENTQ_0,
		.flags		= IORESOURCE_DMA | IORESOURCE_DMA_EVENT_Q,
	},
};

static struct davinci_mmc_platform_data mmc0_platform_data = {
	.mmc_clk		= "MMCSDCLK0",
	.rw_threshold		= 32,
	.use_4bit_mode		= 1,
};

static struct platform_device mmc0_device = {
	.name			= "davinci-mmc",
	.id			= 0,
	.dev			= {
		.platform_data	= &mmc0_platform_data,
	},

	.num_resources		= ARRAY_SIZE(mmc0_resources),
	.resource		= mmc0_resources,
};

/*
 * Initialization
 */

static struct platform_device *davinci_evm_devices[] __initdata = {
	&serial_device,
#ifdef CONFIG_DM357_STORAGE_NAND
	&davinci_nand_device_storage,
#else
	&davinci_nand_device,
#endif
	&rtc_dev,
	&davinci_fb_device,
	&mmc0_device,
};

static void __init davinci_psc_init(void)
{
	davinci_psc_config(DAVINCI_GPSC_ARMDOMAIN, 0, DAVINCI_LPSC_VPSSMSTR, 1);
	davinci_psc_config(DAVINCI_GPSC_ARMDOMAIN, 0, DAVINCI_LPSC_VPSSSLV, 1);
	davinci_psc_config(DAVINCI_GPSC_ARMDOMAIN, 0, DAVINCI_LPSC_TPCC, 1);
	davinci_psc_config(DAVINCI_GPSC_ARMDOMAIN, 0, DAVINCI_LPSC_TPTC0, 1);
	davinci_psc_config(DAVINCI_GPSC_ARMDOMAIN, 0, DAVINCI_LPSC_TPTC1, 1);
	davinci_psc_config(DAVINCI_GPSC_ARMDOMAIN, 0, DAVINCI_LPSC_GPIO, 1);
#if defined(CONFIG_USB_MUSB_HDRC) || defined(CONFIG_USB_MUSB_HDRC_MODULE)
	davinci_psc_config(DAVINCI_GPSC_ARMDOMAIN, 0, DAVINCI_LPSC_USB, 1);
#endif

	/* Turn on WatchDog timer LPSC.	 Needed for RESET to work */
	davinci_psc_config(DAVINCI_GPSC_ARMDOMAIN, 0, DAVINCI_LPSC_TIMER2, 1);

#if defined(CONFIG_MMC_DAVINCI) || defined(CONFIG_MMC_DAVINCI_MODULE)
	davinci_psc_config(DAVINCI_GPSC_ARMDOMAIN, 0, DAVINCI_LPSC_MMC_SD0, 1);
#endif
}

static __init void dm357_evm_expander_setup(void)
{
}

static __init int dm357_evm_expander_validate(enum i2c_expander_pins pin)
{
	return 0;
}

static __init void davinci_evm_init(void)
{
	davinci_gpio_init();
	davinci_psc_init();

#ifdef CONFIG_DM357_STORAGE_NAND
	davinci_cfg_reg(DM357_AECS4, PINMUX_RESV);
#endif

	davinci_i2c_expander.address = 0x3A;
	davinci_i2c_expander.init_data = 0xF6;
	davinci_i2c_expander.name = "dm357_evm_expander1";
	davinci_i2c_expander.setup = dm357_evm_expander_setup;
	davinci_i2c_expander.validate = dm357_evm_expander_validate;

	platform_notify = davinci_serial_init;
	platform_add_devices(davinci_evm_devices,
			     ARRAY_SIZE(davinci_evm_devices));
}

static __init void davinci_evm_irq_init(void)
{
	davinci_init_common_hw();
	davinci_irq_init();
}

MACHINE_START(DAVINCI_DM357_EVM, "DM357 EVM")
	/* Maintainer: MontaVista Software <source@mvista.com> */
	.phys_io      = IO_PHYS,
	.io_pg_offst  = (io_p2v(IO_PHYS) >> 18) & 0xfffc,
	.boot_params  = (DAVINCI_DDR_BASE + 0x100),
	.map_io	      = davinci_map_common_io,
	.init_irq     = davinci_evm_irq_init,
	.timer	      = &davinci_timer,
	.init_machine = davinci_evm_init,
MACHINE_END


