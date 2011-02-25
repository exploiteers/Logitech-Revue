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
#if defined(CONFIG_MTD) || defined(CONFIG_MTD_MODULE)
#include <linux/mtd/physmap.h>
#endif
#include <linux/serial.h>
#include <linux/serial_8250.h>
#include <linux/kgdb.h>
#include <linux/delay.h>

#include <asm/setup.h>
#include <asm/io.h>
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
	.name	= "serial8250",
	.id	= PLAT8250_DEV_PLATFORM,
	.dev	= {
		.platform_data	= serial_platform_data,
	},
};

#if defined(CONFIG_MTD) || defined(CONFIG_MTD_MODULE)
/* NOR Flash base address set to CS0 by default */
#define NOR_FLASH_PHYS 0x02000000

static struct mtd_partition davinci_evm_partitions[] = {
	/* bootloader (U-Boot, etc) in first 4 sectors */
	{
		.name		= "bootloader",
		.offset		= 0,
		.size		= SZ_128K,
		.mask_flags	= MTD_WRITEABLE, /* force read-only */
	},
	/* bootloader params in the next 1 sectors */
	{
		.name		= "params",
		.offset		= MTDPART_OFS_APPEND,
		.size		= SZ_128K,
		.mask_flags	= MTD_WRITEABLE,
	},
	/* kernel */
	{
		.name		= "kernel",
		.offset		= MTDPART_OFS_APPEND,
		.size		= SZ_4M,
		.mask_flags	= 0
	},
	/* file system */
	{
		.name		= "filesystem",
		.offset		= MTDPART_OFS_APPEND,
		.size		= MTDPART_SIZ_FULL,
		.mask_flags	= 0
	}
};

static struct physmap_flash_data davinci_evm_flash_data = {
	.width		= 2,
	.parts		= davinci_evm_partitions,
	.nr_parts	= ARRAY_SIZE(davinci_evm_partitions),
};

/* NOTE: CFI probe will correctly detect flash part as 32M, but EMIF
 * limits addresses to 16M, so using addresses past 16M will wrap */
static struct resource davinci_evm_flash_resource = {
	.start		= NOR_FLASH_PHYS,
	.end		= NOR_FLASH_PHYS + SZ_16M - 1,
	.flags		= IORESOURCE_MEM,
};

static struct platform_device davinci_evm_flash_device = {
	.name		= "physmap-flash",
	.id		= 0,
	.dev		= {
		.platform_data	= &davinci_evm_flash_data,
	},
	.num_resources	= 1,
	.resource	= &davinci_evm_flash_resource,
};
#endif

#if defined(CONFIG_MTD_NAND_DAVINCI) || defined(CONFIG_MTD_NAND_DAVINCI_MODULE)
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
#endif

static struct platform_device rtc_dev = {
	.name		= "rtc_davinci_evm",
	.id		= -1,
};

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

#if defined(CONFIG_MMC_DAVINCI) || defined(CONFIG_MMC_DAVINCI_MODULE)
int dm644x_mmc_get_ro(int index)
{
	char input_state[4] = { 2, 4, 0, 0 };

	davinci_i2c_write(2, input_state, 0x23);
	udelay(1000);
	davinci_i2c_read(4,  input_state, 0x23);
	udelay(1000);

	return input_state[3] & 0x40;
}

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
	.get_ro			= dm644x_mmc_get_ro,
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

static void setup_mmc(void)
{
	davinci_psc_config(DAVINCI_GPSC_ARMDOMAIN, 0, DAVINCI_LPSC_MMC_SD0, 1);
}
#else
#define setup_mmc()
#endif

#if defined(CONFIG_BLK_DEV_PALMCHIP_BK3710) || \
    defined(CONFIG_BLK_DEV_PALMCHIP_BK3710_MODULE)
static struct resource ide_resources[] = {
	{
		.start		= DAVINCI_CFC_ATA_BASE,
		.end		= DAVINCI_CFC_ATA_BASE + 0x7ff,
		.flags		= IORESOURCE_MEM,
	},
	{
		.start		= IRQ_IDE,
		.end		= IRQ_IDE,
		.flags		= IORESOURCE_IRQ,
	},
};

static struct platform_device davinci_ide_device = {
	.name		= "palm_bk3710",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(ide_resources),
	.resource	= ide_resources,
};
#endif

static struct platform_device *davinci_evm_devices[] __initdata = {
	&serial_device,
#if defined(CONFIG_MTD) || defined(CONFIG_MTD_MODULE)
	&davinci_evm_flash_device,
#endif
#if defined(CONFIG_MTD_NAND_DAVINCI) || defined(CONFIG_MTD_NAND_DAVINCI_MODULE)
	&davinci_nand_device,
#endif
	&rtc_dev,
	&davinci_fb_device,
#if defined(CONFIG_BLK_DEV_PALMCHIP_BK3710) || \
    defined(CONFIG_BLK_DEV_PALMCHIP_BK3710_MODULE)
	&davinci_ide_device,
#endif
#if defined(CONFIG_MMC_DAVINCI) || defined(CONFIG_MMC_DAVINCI_MODULE)
	&mmc0_device,
#endif
};

static void __init
davinci_evm_map_io(void)
{
	davinci_map_common_io();
#ifdef CONFIG_KGDB_8250
	kgdb8250_add_platform_port(CONFIG_KGDB_PORT_NUM,
			&serial_platform_data[CONFIG_KGDB_PORT_NUM]);
#endif
}

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
}

static __init void dm644x_evm_expander_setup(void)
{
#if defined(CONFIG_BLK_DEV_PALMCHIP_BK3710) || \
    defined(CONFIG_BLK_DEV_PALMCHIP_BK3710_MODULE)
	/*
	 * ATA_SEL is 1 -> disable, 0 -> enable.
	 * CF_SEL  is 1 -> disable, 0 -> enable.
	 *
	 * Ensure both are not enabled at once.
	 */
#ifdef CONFIG_DAVINCI_EVM_CF_SUPPORT
	davinci_i2c_expander_op(ATA_SEL, 1);
	davinci_i2c_expander_op(CF_RESET, 1);
	davinci_i2c_expander_op(CF_SEL,	0);
#else
	davinci_i2c_expander_op(CF_SEL, 1);
	davinci_i2c_expander_op(ATA_SEL, 0);
#endif
#endif
}

static __init int dm644x_evm_expander_validate(enum i2c_expander_pins pin)
{
	switch (pin) {
	case USB_DRVVBUS:
	case VDDIMX_EN:
	case VLYNQ_ON:
	case CF_RESET:
	case WLAN_RESET:
	case ATA_SEL:
	case CF_SEL:
		return 0;
	default:
		return -EINVAL;
	}
}

static __init int dm644x_evm_expander_special(enum i2c_expander_pins pin,
					      unsigned val)
{
	static u8 cmd[4] = { 4, 6, 0x00, 0x09 };

	if (pin == CF_SEL) {
		int err = davinci_i2c_write(4, cmd, 0x23);

		if (err)
			return err;
	}

	return 0;
}

static __init void davinci_evm_init(void)
{
	davinci_gpio_init();
	davinci_psc_init();

	/* There are actually 3 expanders on the board, one is LEDs only... */
	davinci_i2c_expander.address = 0x3A;
	davinci_i2c_expander.init_data = 0xF6;
	davinci_i2c_expander.name = "davinci_evm_expander1";
	davinci_i2c_expander.setup = dm644x_evm_expander_setup;
	davinci_i2c_expander.validate = dm644x_evm_expander_validate;
	davinci_i2c_expander.special = dm644x_evm_expander_special;

#if defined(CONFIG_BLK_DEV_PALMCHIP_BK3710) || \
    defined(CONFIG_BLK_DEV_PALMCHIP_BK3710_MODULE)
	printk(KERN_WARNING "WARNING: both IDE and NOR flash are enabled, "
	       "but share pins.\n\tDisable IDE for NOR support.\n");
#endif
#if defined(CONFIG_MTD_NAND_DAVINCI) || defined(CONFIG_MTD_NAND_DAVINCI_MODULE)
	printk(KERN_WARNING "WARNING: both NAND and NOR flash are enabled, "
	       "but share pins.\n\tDisable NAND for NOR support.\n");
#endif

	setup_mmc();
	platform_notify = davinci_serial_init;
	platform_add_devices(davinci_evm_devices,
			     ARRAY_SIZE(davinci_evm_devices));
}

static __init void davinci_evm_irq_init(void)
{
	davinci_init_common_hw();
	davinci_irq_init();
}

MACHINE_START(DAVINCI_EVM, "DaVinci EVM")
	/* Maintainer: MontaVista Software <source@mvista.com> */
	.phys_io      = IO_PHYS,
	.io_pg_offst  = (io_p2v(IO_PHYS) >> 18) & 0xfffc,
	.boot_params  = (DAVINCI_DDR_BASE + 0x100),
	.map_io	      = davinci_evm_map_io,
	.init_irq     = davinci_evm_irq_init,
	.timer	      = &davinci_timer,
	.init_machine = davinci_evm_init,
MACHINE_END
