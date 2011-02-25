/*
 * TI DaVinci DM365 EVM board
 *
 * Derived from: arch/arm/mach-davinci/board-evm.c
 * Copyright (C) 2006 Texas Instruments.
 *
 * 2007 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

/**************************************************************************
 * Included Files
 **************************************************************************/

#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/irq.h>
#include <linux/major.h>
#include <linux/root_dev.h>
#include <linux/dma-mapping.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/physmap.h>
#include <linux/mtd/nand.h>
#include <linux/delay.h>
#include <linux/serial.h>
#include <linux/serial_8250.h>

#include <asm/setup.h>
#include <linux/io.h>
#include <asm/mach-types.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/flash.h>
#include <asm/arch/irqs.h>
#include <asm/arch/hardware.h>
#include <asm/arch/edma.h>
#include <asm/arch/mux.h>
#include <asm/arch/gpio.h>
#include <asm/arch/nand.h>
#include <linux/kgdb.h>
#include <asm/arch/clock.h>
#include <asm/arch/cpu.h>
#include <asm/arch/common.h>
#include <asm/arch/clock.h>
#include <asm/arch/mmc.h>
#include <asm/arch/i2c-client.h>
#include <asm/arch/dm365_keypad.h>
#include <asm/arch/cpld.h>

/**************************************************************************
 * Definitions
 **************************************************************************/
static struct plat_serial8250_port serial_platform_data[] = {
	{
		.membase = (char *) IO_ADDRESS(DAVINCI_UART0_BASE),
		.mapbase = (unsigned long) DAVINCI_UART0_BASE,
		.irq = IRQ_UARTINT0,
		.flags = UPF_BOOT_AUTOCONF | UPF_SKIP_TEST,
		.iotype = UPIO_MEM,
		.regshift = 2,
		.uartclk = 24000000,
	},
	{
		.membase = (char *) IO_ADDRESS(DM365_UART1_BASE),
		.mapbase = (unsigned long) DM365_UART1_BASE,
		.irq = IRQ_UARTINT1,
		.flags = UPF_BOOT_AUTOCONF | UPF_SKIP_TEST,
		.iotype = UPIO_MEM,
		.regshift = 2,
		.uartclk = 24000000,
	},
	{
		.flags = 0
	},
};

static struct platform_device serial_device = {
	.name = "serial8250",
	.id = 0,
	.dev = {
		.platform_data = serial_platform_data,
	},
};

/**************************************************************************
 * Public Functions
 **************************************************************************/
int cpu_type(void)
{
	return MACH_TYPE_DAVINCI_DM365_EVM;
}
EXPORT_SYMBOL(cpu_type);

static struct resource mmc0_resources[] = {
	[0] = {			/* registers */
		.start = DM365_MMC_SD0_BASE,
		.end = DM365_MMC_SD0_BASE + SZ_1K - 1,
		.flags = IORESOURCE_MEM,
		},
	[1] = {			/* interrupt */
		.start = IRQ_DM3XX_MMCINT0,
		.end = IRQ_DM3XX_MMCINT0,
		.flags = IORESOURCE_IRQ,
		},
	[2] = {			/* dma rx */
		.start = DM365_DMA_MMC0RXEVT,
		.end = DM365_DMA_MMC0RXEVT,
		.flags = IORESOURCE_DMA | IORESOURCE_DMA_RX_CHAN,
		},
	[3] = {			/* dma tx */
		.start = DM365_DMA_MMC0TXEVT,
		.end = DM365_DMA_MMC0TXEVT,
		.flags = IORESOURCE_DMA | IORESOURCE_DMA_TX_CHAN,
		},
	[4] = {			/* event queue */
		.start = EVENTQ_3,
		.end = EVENTQ_3,
		.flags	= IORESOURCE_DMA | IORESOURCE_DMA_EVENT_Q,
		},
};

static struct davinci_mmc_platform_data mmc0_platform_data = {
	.mmc_clk = "MMCSDCLK0",
	.rw_threshold = 64,
	.use_4bit_mode = 1,
	.use_8bit_mode		= 0,
	.max_frq		= 50000000,
	.pio_set_dmatrig	= 1,
};

static struct platform_device mmc0_device = {
	.name = "davinci-mmc",
	.id = 0,
	.dev = {
		.platform_data = &mmc0_platform_data,
		},
	.num_resources = ARRAY_SIZE(mmc0_resources),
	.resource = mmc0_resources,
};

#ifdef CONFIG_DAVINCI_MMC1
static struct resource mmc1_resources[] = {
	[0] = {			/* registers */
		.start = DM365_MMC_SD1_BASE,
		.end = DM365_MMC_SD1_BASE + SZ_1K - 1,
		.flags = IORESOURCE_MEM,
		},
	[1] = {			/* interrupt */
		.start = IRQ_DM3XX_MMCINT1,
		.end = IRQ_DM3XX_MMCINT1,
		.flags = IORESOURCE_IRQ,
		},
	[2] = {			/* dma rx */
		.start = DM365_DMA_MMC1RXEVT,
		.end = DM365_DMA_MMC1RXEVT,
		.flags = IORESOURCE_DMA | IORESOURCE_DMA_RX_CHAN,
		},
	[3] = {			/* dma tx */
		.start = DM365_DMA_MMC1TXEVT,
		.end = DM365_DMA_MMC1TXEVT,
		.flags = IORESOURCE_DMA | IORESOURCE_DMA_TX_CHAN,
		},
	[4] = {			/* event queue */
		.start = EVENTQ_3,
		.end = EVENTQ_3,
		.flags	= IORESOURCE_DMA | IORESOURCE_DMA_EVENT_Q,
		},
};

static struct davinci_mmc_platform_data mmc1_platform_data = {
	.mmc_clk = "MMCSDCLK1",
	.rw_threshold = 64,
	.use_4bit_mode = 1,
	.use_8bit_mode		= 0,
	.max_frq		= 50000000,
	.pio_set_dmatrig	= 1,
};

static struct platform_device mmc1_device = {
	.name = "davinci-mmc",
	.id = 1,
	.dev = {
		.platform_data = &mmc1_platform_data,
		},
	.num_resources = ARRAY_SIZE(mmc1_resources),
	.resource = mmc1_resources,
};
#endif // CONFIG_DAVINCI_MMC1

/*
 * The NAND partition map used by UBL/U-Boot is a function of the NAND block
 * size.  We support NAND components with either a 128KB or 256KB block size.
*/
#ifdef CONFIG_DAVINCI_NAND_256KB_BLOCKS
#define NAND_BLOCK_SIZE	(SZ_256K)
#else
#define NAND_BLOCK_SIZE	(SZ_128K)
#endif

static struct mtd_partition nand_partitions[] = {
	/* bootloader (UBL, U-Boot, BBT) in sectors: 0 - 14 */
	{
		.name = "bootloader",
		.offset = 0,
		.size = 30 * NAND_BLOCK_SIZE,
		//.mask_flags = MTD_WRITEABLE,	/* force read-only */
		.mask_flags = 0,	// make UBL/u-boot writeable, for firmware upgrade
	},
	/* bootloader params in the next sector 15 */
	{
		.name = "params",
		.offset = MTDPART_OFS_APPEND,
		.size = 2 * NAND_BLOCK_SIZE,
		//.mask_flags = MTD_WRITEABLE,	/* force read-only */
		.mask_flags = 0,	// make uboot params writeable, for firmware upgrade
	},
	/* kernel in sectors: 16 */
	{
		.name = "kernel",
		.offset = MTDPART_OFS_APPEND,
		.size = SZ_4M,
		.mask_flags = 0
	},
	{
		.name = "cramfs",
		.offset = MTDPART_OFS_APPEND,
		//.size = (SZ_16M|SZ_4M|SZ_2M|SZ_1M),
		.size = (SZ_16M|SZ_4M|SZ_2M),
		.mask_flags = 0
	},
	{
		.name = "livefs",
		.offset = MTDPART_OFS_APPEND,
		//.size = SZ_1M,
		.size = SZ_2M,
		.mask_flags = 0
	}
};

/* flash bbt decriptors */
static uint8_t nand_davinci_bbt_pattern[] = { 'B', 'b', 't', '0' };
static uint8_t nand_davinci_mirror_pattern[] = { '1', 't', 'b', 'B' };

static struct nand_bbt_descr nand_davinci_bbt_main_descr = {
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE |
		   NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_PERCHIP,
	.offs = 2,
	.len = 4,
	.veroffs = 16,
	.maxblocks = 4,
	.pattern = nand_davinci_bbt_pattern
};

static struct nand_bbt_descr nand_davinci_bbt_mirror_descr = {
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE |
		   NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_PERCHIP,
	.offs = 2,
	.len = 4,
	.veroffs = 16,
	.maxblocks = 4,
	.pattern = nand_davinci_mirror_pattern
};

static struct nand_davinci_platform_data nand_data = {
	.options = NAND_SKIP_BBTSCAN,
	.ecc_mode = NAND_ECC_HW_SYNDROME,
	.cle_mask = 0x10,
	.ale_mask = 0x08,
	.bbt_td = &nand_davinci_bbt_main_descr,
	.bbt_md = &nand_davinci_bbt_mirror_descr,
	.parts = nand_partitions,
	.nr_parts = ARRAY_SIZE(nand_partitions),
};

static struct resource nand_resources[] = {
	[0] = {
		/* First memory resource is AEMIF control registers */
		.start = DM365_ASYNC_EMIF_CNTRL_BASE,
		.end = DM365_ASYNC_EMIF_CNTRL_BASE + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
		},
	[1] = {
		/* Second memory resource is NAND I/O window */
		.start = DAVINCI_ASYNC_EMIF_DATA_CE0_BASE,
		.end = DAVINCI_ASYNC_EMIF_DATA_CE0_BASE + SZ_16K - 1,
		.flags = IORESOURCE_MEM,
		},
	[2] = {
		/* Third (optional) memory resource is NAND I/O window */
		/* for second NAND chip select */
		.start = DAVINCI_ASYNC_EMIF_DATA_CE0_BASE + SZ_16K,
		.end = DAVINCI_ASYNC_EMIF_DATA_CE0_BASE + 2 * SZ_16K - 1,
		.flags = IORESOURCE_MEM,
		},
};

static struct platform_device nand_device = {
	.name = "nand_davinci",
	.id = 0,
	.dev = {
		.platform_data = &nand_data
		},
	.num_resources = ARRAY_SIZE(nand_resources),
	.resource = nand_resources,
};

#ifdef CONFIG_DAVINCI_NOR
static struct mtd_partition wolverine_nor_partitions[] = {
	/* bootloader (U-Boot, etc) in first 4 sectors */
	{
		.name		= "bootloader",
		.offset		= 0,
		.size		= SZ_512K,
		.mask_flags	= MTD_WRITEABLE, /* force read-only */
	},
	/* bootloader params in the next 1 sectors */
	{
		.name		= "params",
		.offset		= MTDPART_OFS_APPEND,
		.size		= SZ_512K,
		.mask_flags	= 0,
	},
	/* kernel */
	{
		.name		= "kernel",
		.offset		= MTDPART_OFS_APPEND,
		.size		= SZ_2M,
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

static struct physmap_flash_data wolverine_evm_nor_data = {
	.width		= 2,
	.parts		= wolverine_nor_partitions,
	.nr_parts	= ARRAY_SIZE(wolverine_nor_partitions),
};

static struct resource wolverine_nor_resources = {
	.start		= DAVINCI_ASYNC_EMIF_DATA_CE0_BASE,
	.end		= DAVINCI_ASYNC_EMIF_DATA_CE0_BASE + SZ_8M - 1,
	.flags		= IORESOURCE_MEM,
};

static struct platform_device nor_device = {
	.name		= "physmap-flash",
	.id		= 0,
	.dev		= {
		.platform_data = &wolverine_evm_nor_data,
	},
	.num_resources	= 1,
	.resource	= &wolverine_nor_resources,
};
#endif // CONFIG_DAVINCI_NOR

static struct resource rtc_resources[] = {
	[0] = {
		/* registers */
		.start = DM365_RTC_BASE,
		.end = DM365_RTC_BASE + SZ_1K - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		/* interrupt */
		.start = IRQ_DM365_RTCINT,
		.end = IRQ_DM365_RTCINT,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device rtc_device = {
	.name = "rtc_davinci_dm365",
	.id = 0,
	.num_resources = ARRAY_SIZE(rtc_resources),
	.resource = rtc_resources,
};

static int dm365evm_keymap[] = {
	KEY_DM365_KEY2,
	KEY_DM365_LEFT,
	KEY_DM365_EXIT,
	KEY_DM365_DOWN,
	KEY_DM365_ENTER,
	KEY_DM365_UP,
	KEY_DM365_KEY1,
	KEY_DM365_RIGHT,
	KEY_DM365_MENU,
	KEY_DM365_REC,
	KEY_DM365_REW,
	KEY_DM365_SKIPMINUS,
	KEY_DM365_STOP,
	KEY_DM365_FF,
	KEY_DM365_SKIPPLUL,
	KEY_DM365_PLAYPAUSE,
	0
};

static struct resource keypad_resources[] = {
	[0] = {
		/* registers */
		.start = DM365_KEYSCAN_BASE,
		.end = DM365_KEYSCAN_BASE + SZ_1K - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		/* interrupt */
		.start = IRQ_DM365_KEYINT,
		.end = IRQ_DM365_KEYINT,
		.flags = IORESOURCE_IRQ,
	},
};

static struct dm365_kp_platform_data dm365evm_kp_data = {
	.keymap 	= dm365evm_keymap,
	.keymapsize	= ARRAY_SIZE(dm365evm_keymap),
	.rep		= 1,
};

static struct platform_device dm365_kp_device = {
	.name		= "dm365_keypad",
	.id		= -1,
	.dev		= {
				.platform_data = &dm365evm_kp_data,
			},
	.num_resources = ARRAY_SIZE(keypad_resources),
	.resource = keypad_resources,
};

static struct platform_device *dm365_evm_devices[] __initdata = {
	&serial_device,
	&mmc0_device,
#ifdef CONFIG_DAVINCI_MMC1
	&mmc1_device,
#endif
#ifdef CONFIG_DAVINCI_NOR
	&nor_device,
#else // !CONFIG_DAVINCI_NOR
	&nand_device,
#endif // CONFIG_DAVINCI_NOR
	&rtc_device,
	&dm365_kp_device,
};

/* FIQ are pri 0-1; otherwise 2-7, with 7 lowest priority */
static const u8 dm365_default_priorities[DAVINCI_N_AINTC_IRQ] = {
	[IRQ_DM3XX_VPSSINT0]	= 2,
	[IRQ_DM3XX_VPSSINT1]	= 6,
	[IRQ_DM3XX_VPSSINT2]	= 6,
	[IRQ_DM3XX_VPSSINT3]	= 6,
	[IRQ_DM3XX_VPSSINT4]	= 6,
	[IRQ_DM3XX_VPSSINT5]	= 6,
	[IRQ_DM3XX_VPSSINT6]	= 6,
	[IRQ_DM3XX_VPSSINT7]	= 7,
	[IRQ_DM3XX_VPSSINT8]	= 6,
	[IRQ_ASQINT]		= 6,
	[IRQ_DM365_IMXINT0]	= 6,
	[IRQ_DM3XX_IMCOPINT]	= 6,
	[IRQ_USBINT]		= 4,
	[IRQ_DM3XX_RTOINT]	= 4,
	[IRQ_DM3XX_TINT5]	= 7,
	[IRQ_DM3XX_TINT6]	= 7,
	[IRQ_CCINT0]		= 5,	/* dma */
	[IRQ_DM3XX_SPINT1_0]	= 5,	/* dma */
	[IRQ_DM3XX_SPINT1_1]	= 5,	/* dma */
	[IRQ_DM3XX_SPINT2_0]	= 5,	/* dma */
	[IRQ_DM365_PSCINT]	= 7,
	[IRQ_DM3XX_SPINT2_1]	= 7,
	[IRQ_DM3XX_TINT7]	= 4,
	[IRQ_DM3XX_SDIOINT0]	= 7,
	[IRQ_DM365_MBXINT]	= 7,
	[IRQ_DM365_MBRINT]	= 7,
	[IRQ_DM3XX_MMCINT0]	= 7,
	[IRQ_DM3XX_MMCINT1]	= 7,
	[IRQ_DM3XX_PWMINT3]	= 7,
	[IRQ_DM365_DDRINT]	= 7,
	[IRQ_DM365_AEMIFINT]	= 7,
	[IRQ_DM3XX_SDIOINT1]	= 4,
	[IRQ_DM365_TINT0]	= 2,	/* clockevent */
	[IRQ_DM365_TINT1]	= 2,	/* clocksource */
	[IRQ_DM365_TINT2]	= 7,	/* DSP timer */
	[IRQ_DM365_TINT3]	= 7,	/* system tick */
	[IRQ_PWMINT0]		= 7,
	[IRQ_PWMINT1]		= 7,
	[IRQ_DM365_PWMINT2]	= 7,
	[IRQ_DM365_IICINT]	= 3,
	[IRQ_UARTINT0]		= 3,
	[IRQ_UARTINT1]		= 3,
	[IRQ_DM3XX_SPINT0_0]	= 3,
	[IRQ_DM3XX_SPINT0_1]	= 3,
	[IRQ_DM3XX_GPIO0]	= 3,
	[IRQ_DM3XX_GPIO1]	= 7,
	[IRQ_DM3XX_GPIO2]	= 4,
	[IRQ_DM3XX_GPIO3]	= 4,
	[IRQ_DM3XX_GPIO4]	= 7,
	[IRQ_DM3XX_GPIO5]	= 7,
	[IRQ_DM3XX_GPIO6]	= 7,
	[IRQ_DM3XX_GPIO7]	= 7,
	[IRQ_DM3XX_GPIO8]	= 7,
	[IRQ_DM3XX_GPIO9]	= 7,
	[IRQ_DM365_GPIO10]	= 7,
	[IRQ_DM365_GPIO11]	= 7,
	[IRQ_DM365_GPIO12]	= 7,
	[IRQ_DM365_GPIO13]	= 7,
	[IRQ_DM365_GPIO14]	= 7,
	[IRQ_DM365_GPIO15]	= 7,
	[IRQ_DM365_KEYINT]	= 7,
	[IRQ_DM365_COMMTX]	= 7,
	[IRQ_DM365_COMMRX]	= 7,
	[IRQ_EMUINT]		= 7,
};

static void board_init(void)
{
	unsigned int val;

	davinci_psc_config(DAVINCI_GPSC_ARMDOMAIN, 0,
			DAVINCI_DM365_LPSC_VPSSMSTR, 1);
	davinci_psc_config(DAVINCI_GPSC_ARMDOMAIN, 0,
			DAVINCI_DM365_LPSC_TPCC, 1);
	davinci_psc_config(DAVINCI_GPSC_ARMDOMAIN, 0,
			DAVINCI_DM365_LPSC_TPTC0, 1);
	davinci_psc_config(DAVINCI_GPSC_ARMDOMAIN, 0,
			DAVINCI_DM365_LPSC_TPTC1, 1);
	davinci_psc_config(DAVINCI_GPSC_ARMDOMAIN, 0,
			DAVINCI_DM365_LPSC_TPTC2, 1);
	davinci_psc_config(DAVINCI_GPSC_ARMDOMAIN, 0,
			DAVINCI_DM365_LPSC_TPTC3, 1);
	davinci_psc_config(DAVINCI_GPSC_ARMDOMAIN, 0,
			DAVINCI_DM365_LPSC_GPIO, 1);
	davinci_psc_config(DAVINCI_GPSC_ARMDOMAIN, 0,
			DAVINCI_DM365_LPSC_McBSP, 1);
	davinci_psc_config(DAVINCI_GPSC_ARMDOMAIN, 0,
			DAVINCI_DM365_LPSC_SPI0, 1);

	/* Turn on WatchDog timer LPSC.  Needed for RESET to work */
	davinci_psc_config(DAVINCI_GPSC_ARMDOMAIN, 0,
			DAVINCI_DM365_LPSC_TIMER2, 1);
	/* Configure interrupt mux */
	val = davinci_readl(DM365_ARM_INTMUX);
	val |= 0x3c400; /* For EMAC Rx/Tx and RTC */
	davinci_writel(val, DM365_ARM_INTMUX);

	/* Configure edma event mux */
	val = davinci_readl(DM365_EDMA_EVTMUX);
#if defined(CONFIG_SND_DM365_INTERNAL_CODEC) || \
    defined(CONFIG_SND_DM365_INTERNAL_CODEC_MODULE)
	val |= 0x3c003;
#else
	val |= 0x3c000;
#endif
	davinci_writel(val, DM365_EDMA_EVTMUX);
}

static void __init davinci_map_io(void)
{
	davinci_irq_priorities = dm365_default_priorities;
	davinci_map_common_io();
	davinci_init_common_hw();

#ifdef CONFIG_KGDB_8250
	kgdb8250_add_platform_port(CONFIG_KGDB_PORT_NUM,
				 &serial_platform_data[CONFIG_KGDB_PORT_NUM]);
#endif
}

static void setup_mmc(void)
{
	unsigned int val;

	/* Configure pull down control */
	val = davinci_readl(DAVINCI_PUPDCTL1);
	val &= ~0x00000400;
	davinci_writel(val, DAVINCI_PUPDCTL1);
}

static void setup_i2c(void)
{
	int i;

	/* GPIO20 */
	davinci_cfg_reg(DM365_GPIO20, PINMUX_RESV);

	/* Configure GPIO20 as an output */
	gpio_direction_output(20, 0);

	for (i = 0; i < 4; i++) {
		gpio_set_value(20, 0);
		mdelay(1);
		gpio_set_value(20, 1);
	}

	/* Free GPIO20, for I2C */
	davinci_cfg_reg(DM365_GPIO20, PINMUX_FREE);

	/* I2C */
	davinci_cfg_reg(DM365_I2C_SCL, PINMUX_RESV);
}

extern void davinci_serial_reset(struct plat_serial8250_port *p);

static __init void dm365_wolverine_init(void)
{
	struct plat_serial8250_port *pspp;

	board_init();

	for (pspp = serial_platform_data; pspp->flags; pspp++)
		davinci_serial_reset(pspp);

	davinci_gpio_init();

	setup_i2c();
	setup_mmc();

	platform_notify = davinci_serial_init;
	platform_add_devices(dm365_evm_devices, ARRAY_SIZE(dm365_evm_devices));
}

static __init void davinci_dm365_evm_irq_init(void)
{
	davinci_irq_init();
}

MACHINE_START(DAVINCI_DM365_EVM, "Logitech Wolverine")
	/* Maintainer: MontaVista Software <source@mvista.com> */
	.phys_io	= IO_PHYS,
	.io_pg_offst	= (io_p2v(IO_PHYS) >> 18) & 0xfffc,
	.boot_params	= (DAVINCI_DDR_BASE + 0x100),
	.map_io		= davinci_map_io,
	.init_irq	= davinci_dm365_evm_irq_init,
	.timer		= &davinci_timer,
	.init_machine	= dm365_wolverine_init,
MACHINE_END
