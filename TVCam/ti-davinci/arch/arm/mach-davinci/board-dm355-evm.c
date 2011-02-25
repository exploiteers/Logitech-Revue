/*
 * TI DaVinci DM355 EVM board
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
#include <linux/mtd/nand.h>
#include <linux/delay.h>
#include <linux/serial.h>
#include <linux/serial_8250.h>

#include <asm/setup.h>
#include <asm/io.h>
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


#if (defined(CONFIG_DAVINCI_LOGICPD_ENCODER) || \
		                defined(CONFIG_DAVINCI_LOGICPD_ENCODER_MODULE)) && \
        (defined(CONFIG_DAVINCI_PWM) || defined(CONFIG_DAVINCI_PWM_MODULE))
#error There is a PINMUX dependency between DAVINCI_LOGICPD_ENCODER and \
	DAVINCI_PWM options for DM355 EVM. Please deselect DAVINCI_PWM \
for DM355 to enable DAVINCI_LOGICPD_ENCODER or vice versa.
#endif

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
		.membase = (char *) IO_ADDRESS(DAVINCI_UART1_BASE),
		.mapbase = (unsigned long) DAVINCI_UART1_BASE,
		.irq = IRQ_UARTINT1,
		.flags = UPF_BOOT_AUTOCONF | UPF_SKIP_TEST,
		.iotype = UPIO_MEM,
		.regshift = 2,
		.uartclk = 24000000,
	},
	{
		.membase = (char *) IO_ADDRESS(DM355_UART2_BASE),
		.mapbase = (unsigned long) DM355_UART2_BASE,
		.irq = IRQ_DM355_UARTINT2,
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
	return MACH_TYPE_DAVINCI_DM355_EVM;
}
EXPORT_SYMBOL(cpu_type);

#if defined(CONFIG_MTD_NAND_DAVINCI) ||  defined(CONFIG_MTD_NAND_DAVINCI_MODULE)


/*
 * The NAND partition map used by UBL/U-Boot is a function of the NAND block
 * size.  We support NAND components with either a 128KB or 256KB block size.
*/
#ifdef CONFIG_DM355_NAND_256KB_BLOCKS
	#define NAND_BLOCK_SIZE	(SZ_256K)
#else
	#define NAND_BLOCK_SIZE	(SZ_128K)
#endif

static struct mtd_partition nand_partitions[] = {
	/* bootloader (UBL, U-Boot, BBT) in sectors: 0 - 14 */
	{
		.name		= "bootloader",
		.offset		= 0,
		.size		= 30*NAND_BLOCK_SIZE,
		.mask_flags	= MTD_WRITEABLE, /* force read-only */
	},
	/* bootloader params in the next sector 15 */
	{
		.name		= "params",
		.offset		= MTDPART_OFS_APPEND,
		.size		= 2*NAND_BLOCK_SIZE,
		.mask_flags	= MTD_WRITEABLE, /* force read-only */
	},
	/* kernel in sectors: 16 */
	{
		.name		= "kernel",
		.offset		= MTDPART_OFS_APPEND,
		.size		= SZ_4M,
		.mask_flags	= 0
	},
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

static struct nand_davinci_platform_data nand_data = {
	.options	= 0,
	.ecc_mode	= NAND_ECC_HW_SYNDROME,
	.cle_mask	= 0x10,
	.ale_mask	= 0x08,
	.bbt_td         = &nand_davinci_bbt_main_descr,
	.bbt_md         = &nand_davinci_bbt_mirror_descr,
	.parts		= nand_partitions,
	.nr_parts	= ARRAY_SIZE(nand_partitions),
  };

static struct resource nand_resources[] = {
	[0] = {		/* First memory resource is AEMIF control registers */
		.start  = DM355_ASYNC_EMIF_CNTRL_BASE,
		.end    = DM355_ASYNC_EMIF_CNTRL_BASE + SZ_4K - 1,
		.flags  = IORESOURCE_MEM,
	},
	[1] = {         /* Second memory resource is NAND I/O window */
		.start  = DAVINCI_ASYNC_EMIF_DATA_CE0_BASE,
		.end    = DAVINCI_ASYNC_EMIF_DATA_CE0_BASE + SZ_16K - 1,
		.flags  = IORESOURCE_MEM,
	},
	[2] = {		/*
			* Third (optional) memory resource is NAND I/O window
			* for second NAND chip select
			*/
		.start  = DAVINCI_ASYNC_EMIF_DATA_CE0_BASE + SZ_16K,
		.end    = DAVINCI_ASYNC_EMIF_DATA_CE0_BASE + SZ_16K +
				SZ_16K - 1,
		.flags  = IORESOURCE_MEM,
	},
  };


static struct platform_device nand_device = {
	.name		= "nand_davinci",
	.id		= 0,
	.dev		= {
		.platform_data	= &nand_data
	},

	.num_resources	= ARRAY_SIZE(nand_resources),
	.resource	= nand_resources,
};

static void setup_nand(void)
{
	void __iomem *pinmux2 =
		(void __iomem *) IO_ADDRESS(DAVINCI_SYSTEM_MODULE_BASE + 0x8);

	/* Configure the pin multiplexing to enable all of the EMIF signals */
	__raw_writel(0x00000004, pinmux2);
}

#else
#define setup_nand()
#endif

static struct platform_device rtc_dev = {
	.name		= "rtc_davinci_dm355",
	.id		= -1,
};

#if defined(CONFIG_DM9000) || defined(CONFIG_DM9000_MODULE)
#define ETH_PHYS	(DAVINCI_ASYNC_EMIF_DATA_CE1_BASE + 0x00014000)
static struct resource dm9000_resources[] = {
	[0] = {
		.start	= ETH_PHYS,
		.end	= ETH_PHYS + 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= ETH_PHYS + 2,
		.end	= ETH_PHYS + 3,
		.flags	= IORESOURCE_MEM,
	},
	[2] = {
		.start	= IRQ_DM3XX_GPIO1,
		.end	= IRQ_DM3XX_GPIO1,
		.flags	= (IORESOURCE_IRQ | IRQT_RISING),
	},
};

static struct platform_device dm9000_device = {
	.name		= "dm9000",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(dm9000_resources),
	.resource	= dm9000_resources,
};

static void setup_ethernet(void)
{
	/* GPIO1 is Ethernet interrupt GPIO */
	u32 gpio = 1;

	/* Configure GPIO1 as an input */
	gpio_direction_input(gpio);

	/* Configure GPIO1 to generate an interrupt on rising edge only */
	set_irq_type(gpio_to_irq(gpio), IRQT_RISING);
}
#else
#define setup_ethernet()
#endif

#if defined(CONFIG_MMC_DAVINCI) || defined(CONFIG_MMC_DAVINCI_MODULE)
#define MSP430_I2C_CLIENT_ADDRESS	0x25
int dm355_mmc_get_ro(int index)
{
	char i2cdata[2] = { 0x06, 0 };

	davinci_i2c_write(1, i2cdata, MSP430_I2C_CLIENT_ADDRESS);
	udelay(100);
	davinci_i2c_read(1,  i2cdata, MSP430_I2C_CLIENT_ADDRESS);

	return index ? i2cdata[0] & 0x10 : i2cdata[0] & 0x04;
}

static struct resource mmc0_resources[] = {
	[0] = {			/* registers */
		.start	= DM355_MMC0_BASE,
		.end	= DM355_MMC0_BASE + SZ_1K - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {			/* interrupt */
		.start	= IRQ_DM3XX_MMCINT0,
		.end	= IRQ_DM3XX_MMCINT0,
		.flags	= IORESOURCE_IRQ,
	},
	[2] = {			/* dma rx */
		.start	= DM355_DMA_MMC0RXEVT,
		.end	= DM355_DMA_MMC0RXEVT,
		.flags	= IORESOURCE_DMA | IORESOURCE_DMA_RX_CHAN,
	},
	[3] = {			/* dma tx */
		.start	= DM355_DMA_MMC0TXEVT,
		.end	= DM355_DMA_MMC0TXEVT,
		.flags	= IORESOURCE_DMA | IORESOURCE_DMA_TX_CHAN,
	},
	[4] = {			/* event queue */
		.start	= EVENTQ_0,
		.end	= EVENTQ_0,
		.flags	= IORESOURCE_DMA | IORESOURCE_DMA_EVENT_Q,
	},
};

static struct davinci_mmc_platform_data mmc0_platform_data = {
	.mmc_clk	= "MMCSDCLK0",
	.rw_threshold	= 32,
	.use_4bit_mode	= 1,
	.get_ro		= dm355_mmc_get_ro,
};

static struct platform_device mmc0_device = {
	.name	= "davinci-mmc",
	.id	= 0,
	.dev	= {
		.platform_data = &mmc0_platform_data,
	},
	.num_resources	= ARRAY_SIZE(mmc0_resources),
	.resource	= mmc0_resources,
};

static struct resource mmc1_resources[] = {
	[0] = {			/* registers */
		.start	= DM355_MMC1_BASE,
		.end	= DM355_MMC1_BASE + SZ_1K - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {			/* interrupt */
		.start	= IRQ_DM3XX_MMCINT1,
		.end	= IRQ_DM3XX_MMCINT1,
		.flags	= IORESOURCE_IRQ,
	},
	[2] = {			/* dma rx */
		.start	= DM355_DMA_MMC1RXEVT,
		.end	= DM355_DMA_MMC1RXEVT,
		.flags	= IORESOURCE_DMA | IORESOURCE_DMA_RX_CHAN,
	},
	[3] = {			/* dma tx */
		.start	= DM355_DMA_MMC1TXEVT,
		.end	= DM355_DMA_MMC1TXEVT,
		.flags	= IORESOURCE_DMA | IORESOURCE_DMA_TX_CHAN,
	},
	[4] = {			/* event queue */
		.start	= EVENTQ_0,
		.end	= EVENTQ_0,
		.flags	= IORESOURCE_DMA | IORESOURCE_DMA_EVENT_Q,
	},
};

static struct davinci_mmc_platform_data mmc1_platform_data = {
	.mmc_clk	= "MMCSDCLK1",
	.rw_threshold	= 32,
	.use_4bit_mode	= 1,
	.get_ro		= dm355_mmc_get_ro,
};

static struct platform_device mmc1_device = {
	.name	= "davinci-mmc",
	.id	= 1,
	.dev	= {
		.platform_data = &mmc1_platform_data,
	},
	.num_resources	= ARRAY_SIZE(mmc1_resources),
	.resource	= mmc1_resources,
};

static void setup_mmc(void)
{
	davinci_psc_config(DAVINCI_GPSC_ARMDOMAIN, 0, DAVINCI_LPSC_MMC_SD0, 1);
	davinci_psc_config(DAVINCI_GPSC_ARMDOMAIN, 0, DAVINCI_LPSC_MMC_SD1, 1);
}
#else
#define setup_mmc()
#endif

static struct platform_device *dm355_evm_devices[] __initdata =
{
	&serial_device,
	&rtc_dev,
#if defined(CONFIG_MTD_NAND_DAVINCI) || defined(CONFIG_MTD_NAND_DAVINCI_MODULE)
	&nand_device,
#endif
#if defined(CONFIG_DM9000) || defined(CONFIG_DM9000_MODULE)
	&dm9000_device,
#endif
#if defined(CONFIG_MMC_DAVINCI) || defined(CONFIG_MMC_DAVINCI_MODULE)
	&mmc0_device,
	&mmc1_device,
#endif
};

/* FIQ are pri 0-1; otherwise 2-7, with 7 lowest priority */
static const u8 dm355_default_priorities[DAVINCI_N_AINTC_IRQ] = {
	[IRQ_DM3XX_VPSSINT0]		= 2,
	[IRQ_DM3XX_VPSSINT1]		= 6,
	[IRQ_DM3XX_VPSSINT2]		= 6,
	[IRQ_DM3XX_VPSSINT3]		= 6,
	[IRQ_DM3XX_VPSSINT4]		= 6,
	[IRQ_DM3XX_VPSSINT5]		= 6,
	[IRQ_DM3XX_VPSSINT6]		= 6,
	[IRQ_DM3XX_VPSSINT7]		= 7,
	[IRQ_DM3XX_VPSSINT8]		= 6,
	[IRQ_ASQINT]			= 6,
	[IRQ_IMXINT]			= 6,
	[IRQ_DM644X_VLCDINT]		= 6,
	[IRQ_USBINT]			= 4,
	[IRQ_DM3XX_RTOINT]		= 4,
	[IRQ_DM355_UARTINT2]		= 7,
	[IRQ_DM3XX_TINT6]		= 7,
	[IRQ_CCINT0]			= 5,	/* dma */
	[IRQ_CCERRINT]			= 5,	/* dma */
	[IRQ_TCERRINT0]			= 5,	/* dma */
	[IRQ_TCERRINT]			= 5,	/* dma */
	[IRQ_PSCIN]			= 7,
	[IRQ_DM3XX_SPINT2_1]		= 7,
	[IRQ_DM3XX_TINT7]		= 4,
	[IRQ_DM3XX_SDIOINT0]		= 7,
	[IRQ_MBXINT]			= 7,
	[IRQ_MBRINT]			= 7,
	[IRQ_MMCINT]			= 7,
	[IRQ_DM3XX_MMCINT1]		= 7,
	[IRQ_DM3XX_PWMINT3]		= 7,
	[IRQ_DDRINT]			= 7,
	[IRQ_AEMIFINT]			= 7,
	[IRQ_DM3XX_SDIOINT1]		= 4,
	[IRQ_TINT0_TINT12]		= 2,	/* clockevent */
	[IRQ_TINT0_TINT34]		= 2,	/* clocksource */
	[IRQ_TINT1_TINT12]		= 7,	/* DSP timer */
	[IRQ_TINT1_TINT34]		= 7,	/* system tick */
	[IRQ_PWMINT0]			= 7,
	[IRQ_PWMINT1]			= 7,
	[IRQ_PWMINT2]			= 7,
	[IRQ_I2C]			= 3,
	[IRQ_UARTINT0]			= 3,
	[IRQ_UARTINT1]			= 3,
	[IRQ_DM3XX_SPINT0_0]		= 3,
	[IRQ_DM3XX_SPINT0_1]		= 3,
	[IRQ_DM3XX_GPIO0]		= 3,
	[IRQ_DM3XX_GPIO1]		= 7,
	[IRQ_DM3XX_GPIO2]		= 4,
	[IRQ_DM3XX_GPIO3]		= 4,
	[IRQ_DM3XX_GPIO4]		= 7,
	[IRQ_DM3XX_GPIO5]		= 7,
	[IRQ_DM3XX_GPIO6]		= 7,
	[IRQ_DM3XX_GPIO7]		= 7,
	[IRQ_DM3XX_GPIO8]		= 7,
	[IRQ_DM3XX_GPIO9]		= 7,
	[IRQ_DM355_GPIOBNK0]		= 7,
	[IRQ_DM355_GPIOBNK1]		= 7,
	[IRQ_DM355_GPIOBNK2]		= 7,
	[IRQ_DM355_GPIOBNK3]		= 7,
	[IRQ_DM355_GPIOBNK4]		= 7,
	[IRQ_DM355_GPIOBNK5]		= 7,
	[IRQ_DM355_GPIOBNK6]		= 7,
	[IRQ_COMMTX]			= 7,
	[IRQ_COMMRX]			= 7,
	[IRQ_EMUINT]			= 7,
};

static void board_init(void)
{
	davinci_psc_config(DAVINCI_GPSC_ARMDOMAIN, 0, DAVINCI_LPSC_VPSSMSTR, 1);
	davinci_psc_config(DAVINCI_GPSC_ARMDOMAIN, 0, DAVINCI_LPSC_VPSSSLV, 1);
	davinci_psc_config(DAVINCI_GPSC_ARMDOMAIN, 0, DAVINCI_LPSC_TPCC, 1);
	davinci_psc_config(DAVINCI_GPSC_ARMDOMAIN, 0, DAVINCI_LPSC_TPTC0, 1);
	davinci_psc_config(DAVINCI_GPSC_ARMDOMAIN, 0, DAVINCI_LPSC_TPTC1, 1);
	davinci_psc_config(DAVINCI_GPSC_ARMDOMAIN, 0, DAVINCI_LPSC_GPIO, 1);
	davinci_psc_config(DAVINCI_GPSC_ARMDOMAIN, 0, DAVINCI_LPSC_McBSP1, 1);
	davinci_psc_config(DAVINCI_GPSC_ARMDOMAIN, 0, DAVINCI_LPSC_SPI, 1);

	/* Turn on WatchDog timer LPSC.  Needed for RESET to work */
	davinci_psc_config(DAVINCI_GPSC_ARMDOMAIN, 0, DAVINCI_LPSC_TIMER2, 1);
}

static void __init davinci_map_io(void)
{
	davinci_irq_priorities = dm355_default_priorities;
	davinci_map_common_io();
	davinci_init_common_hw();

#ifdef CONFIG_KGDB_8250
	kgdb8250_add_platform_port(CONFIG_KGDB_PORT_NUM,
				 &serial_platform_data[CONFIG_KGDB_PORT_NUM]);
#endif
}

static __init void dm355_evm_init(void)
{
	struct plat_serial8250_port *pspp;

	/* Board-specific initialization */
	board_init();

	for (pspp = serial_platform_data; pspp->flags; pspp++)
		davinci_serial_reset(pspp);

	davinci_gpio_init();
	setup_ethernet();
	setup_mmc();
	setup_nand();

	platform_notify = davinci_serial_init;
	platform_add_devices(dm355_evm_devices, ARRAY_SIZE(dm355_evm_devices));
}

static __init void davinci_dm355_evm_irq_init(void)
{
	davinci_irq_init();
}

MACHINE_START(DAVINCI_DM355_EVM, "DaVinci DM355 EVM")
	/* Maintainer: MontaVista Software <source@mvista.com> */
	.phys_io      = IO_PHYS,
	.io_pg_offst  = (io_p2v(IO_PHYS) >> 18) & 0xfffc,
	.boot_params  = (DAVINCI_DDR_BASE + 0x100),
	.map_io	      = davinci_map_io,
	.init_irq     = davinci_dm355_evm_irq_init,
	.timer	      = &davinci_timer,
	.init_machine = dm355_evm_init,
MACHINE_END
