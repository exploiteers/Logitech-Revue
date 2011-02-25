/*
 * TI DaVinci DM6467 EVM board
 *
 * Derived from: arch/arm/mach-davinci/board-evm.c
 * Copyright (C) 2006 Texas Instruments.
 * Copyright (C) 2007-2008, MontaVista Software, Inc.
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 *
 */

/**************************************************************************
 * Included Files
 **************************************************************************/

#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/major.h>
#include <linux/root_dev.h>
#include <linux/dma-mapping.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/serial.h>
#include <linux/mtd/nand.h>
#include <linux/serial_8250.h>
#include <linux/kgdb.h>

#include <asm/setup.h>
#include <asm/io.h>
#include <asm/mach-types.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/flash.h>
#include <asm/arch/irqs.h>
#include <asm/arch/hardware.h>
#include <asm/arch/edma.h>
#include <asm/arch/cpu.h>
#include <asm/arch/mux.h>
#include <asm/arch/nand.h>
#include <asm/arch/i2c-client.h>
#include <asm/arch/gpio.h>
#include <asm/arch/clock.h>

/**************************************************************************
 * Definitions
 **************************************************************************/
#define DAVINCI_DM646X_UART_CLK		24000000

static struct plat_serial8250_port serial_platform_data[] = {
	{
		.membase	= (char *)IO_ADDRESS(DAVINCI_UART0_BASE),
		.mapbase	= (unsigned long)DAVINCI_UART0_BASE,
		.irq		= IRQ_UARTINT0,
		.flags		= UPF_BOOT_AUTOCONF | UPF_SKIP_TEST,
		.iotype		= UPIO_MEM32,
		.regshift	= 2,
		.uartclk	= DAVINCI_DM646X_UART_CLK,
	},
	{
		.membase	= (char *)IO_ADDRESS(DAVINCI_UART1_BASE),
		.mapbase	= (unsigned long)DAVINCI_UART1_BASE,
		.irq		= IRQ_UARTINT1,
		.flags		= UPF_BOOT_AUTOCONF | UPF_SKIP_TEST,
		.iotype		= UPIO_MEM32,
		.regshift	= 2,
		.uartclk	= DAVINCI_DM646X_UART_CLK,
	},
	{
		.membase	= (char *)IO_ADDRESS(DM644X_UART2_BASE),
		.mapbase	= (unsigned long)DM644X_UART2_BASE,
		.irq		= IRQ_UARTINT2,
		.flags		= UPF_BOOT_AUTOCONF | UPF_SKIP_TEST,
		.iotype		= UPIO_MEM32,
		.regshift	= 2,
		.uartclk	= DAVINCI_DM646X_UART_CLK,
	},
	{
		.flags	= 0,
	},
};

static struct platform_device serial_device = {
	.name	= "serial8250",
	.id	= 0,
	.dev	= {
		.platform_data	= serial_platform_data,
	},
};

/**************************************************************************
 * Public Functions
 **************************************************************************/

#if defined (CONFIG_MTD_NAND_DAVINCI) || defined(CONFIG_MTD_NAND_DAVINCI_MODULE)
static struct mtd_partition dm646x_nand_partitions[] = {
	/* bootloader (U-Boot, etc) in first sector */
	{
		.name		= "bootloader",
		.offset		= 0,
		.size		= SZ_512K,
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
		.mask_flags		= 0,
	},
	/* file system */
	{
		.name		= "filesystem",
		.offset		= MTDPART_OFS_APPEND,
		.size		= MTDPART_SIZ_FULL,
		.mask_flags		= 0,
	}
};

static struct nand_davinci_platform_data dm646x_nand_data = {
	.options	= 0,
	.ecc_mode	= NAND_ECC_HW,
	.cle_mask	= 0x80000,
	.ale_mask	= 0x40000,
	.parts		= dm646x_nand_partitions,
	.nr_parts	= ARRAY_SIZE(dm646x_nand_partitions),
};

static struct resource dm646x_nand_resources[] = {
	[0] = {		/* First memory resource is AEMIF control registers */
		.start		= DAVINCI_DM646X_ASYNC_EMIF_CNTRL_BASE,
		.end		= DAVINCI_DM646X_ASYNC_EMIF_CNTRL_BASE +
					SZ_16K - 1,
		.flags		= IORESOURCE_MEM,
	},
	[1] = {		/* Second memory resource is NAND I/O window */
		.start		= DAVINCI_DM646X_ASYNC_EMIF_DATA_CE0_BASE,
		.end		= DAVINCI_DM646X_ASYNC_EMIF_DATA_CE0_BASE +
					SZ_512K + 2 * SZ_1K - 1,
		.flags		= IORESOURCE_MEM,
	},
};

static struct platform_device nand_device = {
	.name			= "nand_davinci",
	.id			= 0,
	.dev			= {
		.platform_data	= &dm646x_nand_data,
	},

	.num_resources		= ARRAY_SIZE(dm646x_nand_resources),
	.resource		= dm646x_nand_resources,
};
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
/*
	&usb_dev,
*/
#if defined (CONFIG_MTD_NAND_DAVINCI) || defined(CONFIG_MTD_NAND_DAVINCI_MODULE)
	&nand_device,
#endif
#if defined(CONFIG_BLK_DEV_PALMCHIP_BK3710) || \
    defined(CONFIG_BLK_DEV_PALMCHIP_BK3710_MODULE)
	&davinci_ide_device,
#endif
};

/* FIQ are pri 0-1; otherwise 2-7, with 7 lowest priority */
static const u8 dm646x_default_priorities[DAVINCI_N_AINTC_IRQ] = {
	[IRQ_DM646X_VP_VERTINT0]	= 7,
	[IRQ_DM646X_VP_VERTINT1]	= 7,
	[IRQ_DM646X_VP_VERTINT2]	= 7,
	[IRQ_DM646X_VP_VERTINT3]	= 7,
	[IRQ_DM646X_VP_ERRINT]		= 7,
	[IRQ_DM646X_RESERVED_1]		= 7,
	[IRQ_DM646X_RESERVED_2]		= 7,
	[IRQ_DM646X_WDINT]		= 7,
	[IRQ_DM646X_CRGENINT0]		= 7,
	[IRQ_DM646X_CRGENINT1]		= 7,
	[IRQ_DM646X_TSIFINT0]		= 7,
	[IRQ_DM646X_TSIFINT1]		= 7,
	[IRQ_DM646X_VDCEINT]		= 7,
	[IRQ_DM646X_USBINT]		= 7,
	[IRQ_DM646X_USBDMAINT]		= 7,
	[IRQ_DM646X_PCIINT]		= 7,
	[IRQ_CCINT0]			= 7,	/* dma */
	[IRQ_CCERRINT]			= 7,	/* dma */
	[IRQ_TCERRINT0]			= 7,	/* dma */
	[IRQ_TCERRINT]			= 7,	/* dma */
	[IRQ_DM646X_TCERRINT2]		= 7,
	[IRQ_DM646X_TCERRINT3]		= 7,
	[IRQ_DM646X_IDE]		= 7,
	[IRQ_DM646X_HPIINT]		= 7,
	[IRQ_DM646X_EMACRXTHINT]	= 7,
	[IRQ_DM646X_EMACRXINT]		= 7,
	[IRQ_DM646X_EMACTXINT]		= 7,
	[IRQ_DM646X_EMACMISCINT]	= 7,
	[IRQ_DM646X_MCASP0TXINT]	= 7,
	[IRQ_DM646X_MCASP0RXINT]	= 7,
	[IRQ_AEMIFINT]			= 7,
	[IRQ_DM646X_RESERVED_3]		= 7,
	[IRQ_DM646X_MCASP1TXINT]	= 7,	/* clockevent */
	[IRQ_TINT0_TINT34]		= 7,	/* clocksource */
	[IRQ_TINT1_TINT12]		= 7,	/* DSP timer */
	[IRQ_TINT1_TINT34]		= 7,	/* system tick */
	[IRQ_PWMINT0]			= 7,
	[IRQ_PWMINT1]			= 7,
	[IRQ_DM646X_VLQINT]		= 7,
	[IRQ_I2C]			= 7,
	[IRQ_UARTINT0]			= 7,
	[IRQ_UARTINT1]			= 7,
	[IRQ_DM646X_UARTINT2]		= 7,
	[IRQ_DM646X_SPINT0]		= 7,
	[IRQ_DM646X_SPINT1]		= 7,
	[IRQ_DM646X_DSP2ARMINT]		= 7,
	[IRQ_DM646X_RESERVED_4]		= 7,
	[IRQ_DM646X_PSCINT]		= 7,
	[IRQ_DM646X_GPIO0]		= 7,
	[IRQ_DM646X_GPIO1]		= 7,
	[IRQ_DM646X_GPIO2]		= 7,
	[IRQ_DM646X_GPIO3]		= 7,
	[IRQ_DM646X_GPIO4]		= 7,
	[IRQ_DM646X_GPIO5]		= 7,
	[IRQ_DM646X_GPIO6]		= 7,
	[IRQ_DM646X_GPIO7]		= 7,
	[IRQ_DM646X_GPIOBNK0]		= 7,
	[IRQ_DM646X_GPIOBNK1]		= 7,
	[IRQ_DM646X_GPIOBNK2]		= 7,
	[IRQ_DM646X_DDRINT]		= 7,
	[IRQ_DM646X_AEMIFINT]		= 7,
	[IRQ_COMMTX]			= 7,
	[IRQ_COMMRX]			= 7,
	[IRQ_EMUINT]			= 7,
};
static void board_init(void)
{
	davinci_psc_config(DAVINCI_GPSC_ARMDOMAIN, 0, DAVINCI_LPSC_VLYNQ, 1);
	davinci_psc_config(DAVINCI_GPSC_ARMDOMAIN, 0,
			   DAVINCI_DM646X_LPSC_HDVICP0, 1);
	davinci_psc_config(DAVINCI_GPSC_ARMDOMAIN, 0,
			   DAVINCI_DM646X_LPSC_HDVICP1, 1);
	davinci_psc_config(DAVINCI_GPSC_ARMDOMAIN, 0,
			   DAVINCI_DM646X_LPSC_SPI, 1);
	davinci_psc_config(DAVINCI_GPSC_ARMDOMAIN, 0,
			   DAVINCI_DM646X_LPSC_TPCC, 1);
	davinci_psc_config(DAVINCI_GPSC_ARMDOMAIN, 0,
			   DAVINCI_DM646X_LPSC_TPTC0, 1);
	davinci_psc_config(DAVINCI_GPSC_ARMDOMAIN, 0,
			   DAVINCI_DM646X_LPSC_TPTC1, 1);
	davinci_psc_config(DAVINCI_GPSC_ARMDOMAIN, 0,
			   DAVINCI_DM646X_LPSC_TPTC2, 1);
	davinci_psc_config(DAVINCI_GPSC_ARMDOMAIN, 0,
			   DAVINCI_DM646X_LPSC_TPTC3, 1);
	davinci_psc_config(DAVINCI_GPSC_ARMDOMAIN, 0,
			   DAVINCI_DM646X_LPSC_AEMIF, 1);
	davinci_psc_config(DAVINCI_GPSC_ARMDOMAIN, 0,
			   DAVINCI_DM646X_LPSC_GPIO, 1);
/*
	davinci_psc_config(DAVINCI_GPSC_ARMDOMAIN, 0,
			   DAVINCI_DM646X_LPSC_TSIF0, 1);
	davinci_psc_config(DAVINCI_GPSC_ARMDOMAIN, 0,
			   DAVINCI_DM646X_LPSC_TSIF1, 1);
*/
}

#define UART_DM6467_SCR		__REG(DAVINCI_UART0_BASE + 0x40)
/*
 * Internal UARTs need to be initialized for the 8250 autoconfig to work
 * properly. Note that the TX watermark initialization may not be needed
 * once the 8250.c watermark handling code is merged.
 */
static int __init dm646x_serial_reset(void)
{
	UART_DM6467_SCR = 0x08;
	return 0;
}
late_initcall(dm646x_serial_reset);

extern const u8 *davinci_def_priorities;

void __init davinci_map_common_io(void);

static void __init davinci_map_io(void)
{
	davinci_def_priorities = dm646x_default_priorities;
	davinci_map_common_io();

#ifdef CONFIG_KGDB_8250
	kgdb8250_add_platform_port(CONFIG_KGDB_PORT_NUM,
				   &serial_platform_data[CONFIG_KGDB_PORT_NUM]);
#endif

}
static __init void dm6467_evm_expander_setup(void)
{
#if defined(CONFIG_BLK_DEV_PALMCHIP_BK3710) || \
    defined(CONFIG_BLK_DEV_PALMCHIP_BK3710_MODULE)

	davinci_i2c_expander_op(ATA_PWD_DM646X, 0);
	davinci_i2c_expander_op(ATA_SEL_DM646X, 0);
#endif
}

static __init int dm646x_evm_expander_validate(enum i2c_expander_pins pin)
{
	switch (pin) {
	case ATA_SEL_DM646X:
	case ATA_PWD_DM646X:
	case VSCALE_ON_DM646X:
	case VLYNQ_RESET_DM646X:
	case I2C_INT_DM646X:
	case USB_FB_DM646X:
	case CIR_MOD_DM646X:
	case CIR_DEMOD_DM646X:
		return 0;
	default:
		return -EINVAL;
	}
}

static __init void dm6467_evm_init(void)
{
	davinci_gpio_init();

	/* Initialize the DaVinci EVM board settigs */
	board_init();

	davinci_i2c_expander.address = 0x3A;
	davinci_i2c_expander.init_data = 0xF6;
	davinci_i2c_expander.name = "dm6467_evm_expander1";
	davinci_i2c_expander.setup = dm6467_evm_expander_setup;
	davinci_i2c_expander.validate = dm646x_evm_expander_validate;

	platform_add_devices(davinci_evm_devices,
		ARRAY_SIZE(davinci_evm_devices));
}

extern void davinci_irq_init(void);
extern struct sys_timer davinci_timer;
extern void __init davinci_init_common_hw(void);

static __init void davinci_dm6467_evm_irq_init(void)
{
        davinci_init_common_hw();
        davinci_irq_init();
}

MACHINE_START(DAVINCI_DM6467_EVM, "DaVinci DM6467 EVM")
        .phys_io      = IO_PHYS,
        .io_pg_offst  = (io_p2v(IO_PHYS) >> 18) & 0xfffc,
        .boot_params  = (0x80000100),
        .map_io       = davinci_map_io,
        .init_irq     = davinci_dm6467_evm_irq_init,
        .timer        = &davinci_timer,
	.init_machine = dm6467_evm_init,
MACHINE_END
