/*
 * TI DaVinci EVM board
 *
 * Copyright (C) 2007 Texas Instruments.
 * Copyright (C) 2007 Monta Vista Software Inc.
 *
 * ----------------------------------------------------------------------------
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * ----------------------------------------------------------------------------
 *
 *  This file came directly from spi_platform_init.c.  This file has been
 *  generalized to all DaVinci variants.  This file should replace
 *  spi_platform_init.c
 *
 */

/*
 * Platform device support for TI SoCs.
 *
 */
#include <linux/config.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/resource.h>
#include <linux/spi/spi.h>
#include <linux/spi/davinci_spi.h>
#include <linux/spi/flash.h>
#include <linux/device.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/platform_device.h>
#include <linux/spi/davinci_spi_master.h>
#include <linux/spi/at25xxA_eeprom.h>
#include <linux/spi/mtd_spi_flash.h>

#include <asm/arch/hardware.h>
#include <asm/arch/cpu.h>

static struct davinci_spi_platform_data dm646x_spi_pdata = {
	.version = DAVINCI_SPI_VERSION_1,
	.num_chipselect = 2,
	.clk_name = "SPICLK",
};

static struct resource dm646x_spi_resources[] = {
	[0] = {
		.start = DAVINCI_SPI_BASE, /* Need 646x defines XXX */
		.end = DAVINCI_SPI_BASE + (SZ_4K/2) - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_DM646X_SPINT0,
		.end = IRQ_DM646X_SPINT0,
		.flags = IORESOURCE_IRQ,
	},
	[2] = {
		.start = DM644X_DMACH_SPI_SPIR, /* Need 646x defines XXX */
		.end = DM644X_DMACH_SPI_SPIR,
		.flags = IORESOURCE_DMA | IORESOURCE_DMA_RX_CHAN,
	},
	[3] = {
		.start = DM644X_DMACH_SPI_SPIX,
		.end = DM644X_DMACH_SPI_SPIX,
		.flags = IORESOURCE_DMA | IORESOURCE_DMA_TX_CHAN,
	},
	[4] = {
		.start = EVENTQ_3,
		.end = EVENTQ_3,
		.flags = IORESOURCE_DMA | IORESOURCE_DMA_EVENT_Q,
	},
};

static struct platform_device dm646x_spi_pdev = {
	.name = "dm_spi",
	.id = 0,
	.resource = dm646x_spi_resources,
	.num_resources = ARRAY_SIZE(dm646x_spi_resources),
	.dev = {
		.platform_data = &dm646x_spi_pdata,
	},
};


static struct davinci_spi_platform_data dm355_spi_pdata = {
	.version = DAVINCI_SPI_VERSION_1,
	.num_chipselect = 2,
	.clk_name = "SPICLK",
};

static struct davinci_spi_platform_data dm365_spi_pdata = {
	.version = DAVINCI_SPI_VERSION_1,
	.num_chipselect = 2,
	.clk_name = "SPICLK",
};

static struct resource dm3xx_spi_resources[] = {
	[0] = {
		.start = DM3XX_SPI0_BASE,
		.end = DM3XX_SPI0_BASE + (SZ_4K/2) - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_DM3XX_SPINT0_0,
		.end = IRQ_DM3XX_SPINT0_0,
		.flags = IORESOURCE_IRQ,
	},
	/* No DMA for SPI on DM355 */
};

static struct platform_device dm355_spi_pdev = {
	.name = "dm_spi",
	.id = 0,
	.resource = dm3xx_spi_resources,
	.num_resources = ARRAY_SIZE(dm3xx_spi_resources),
	.dev = {
		.platform_data = &dm355_spi_pdata,
	},
};

static struct platform_device dm365_spi_pdev = {
	.name = "dm_spi",
	.id = 0,
	.resource = dm3xx_spi_resources,
	.num_resources = ARRAY_SIZE(dm3xx_spi_resources),
	.dev = {
		.platform_data = &dm365_spi_pdata,
	},
};

#if defined(CONFIG_DAVINCI_SPI_EEPROM) || \
    defined(CONFIG_DAVINCI_SPI_EEPROM_MODULE)
static struct mtd_partition spi_partitions[] = {
	/* UBL in first sector */
	[0] = {
		.name = "UBL",
		.offset = 0,
		.size = SZ_16K,
		.mask_flags = MTD_WRITEABLE,
	},
	/* User data in the next sector */
	[1] = {
		.name = "data",
		.offset = MTDPART_OFS_APPEND,
		.size = MTDPART_SIZ_FULL,
		.mask_flags = 0,
	}
};

struct davinci_spi_config_t davinci_spi_eeprom_spi_cfg = {
	.wdelay		= 0,
	.odd_parity	= 0,
	.parity_enable	= 0,
	.wait_enable	= 0,
	.lsb_first	= 0,
	.timer_disable	= 0,
	.clk_high	= 0,
	.phase_in	= 1,
	.clk_internal	= 1,
	.loop_back	= 0,
	.cs_hold	= 1,
	.intr_level	= 0,
	.pin_op_modes	= SPI_OPMODE_SPISCS_4PIN,
#ifndef CONFIG_SPI_INTERRUPT
	.poll_mode	= 1,
#endif
};

struct davinci_eeprom_info davinci_8k_spi_eeprom_info = {
	.eeprom_size = 8192,
	.page_size = 32,
	.page_mask = 0x001F,
	.chip_sel = SCS0_SELECT,
	.parts = NULL,
	.nr_parts = 0,
	.commit_delay = 3,
};

struct davinci_eeprom_info davinci_32k_spi_eeprom_info = {
	.eeprom_size = 32768,
	.page_size = 64,
	.page_mask = 0x003F,
	.chip_sel = SCS0_SELECT,
	.parts = spi_partitions,
	.nr_parts = ARRAY_SIZE(spi_partitions),
	.commit_delay = 5,
};
#endif

/*Put slave specific information in this array.*/
/*For more information refer the table at the end of file tnetd84xx_spi_cs.c*/
static struct spi_board_info dm646x_spi_board_info[] = {
#if defined(CONFIG_DAVINCI_SPI_EEPROM) || \
    defined(CONFIG_DAVINCI_SPI_EEPROM_MODULE)
	[0] = {
		.modalias = DAVINCI_SPI_EEPROM_NAME,
		.platform_data = &davinci_32k_spi_eeprom_info,
		.controller_data = &davinci_spi_eeprom_spi_cfg,
		.mode = SPI_MODE_0,
		.irq = 0,
		.max_speed_hz = 2 * 1000 * 1000 /* max sample rate at 3V */ ,
		.bus_num = 0,
		.chip_select = 0,
	},
#endif
};

static struct spi_board_info dm3xx_spi_board_info[] = {
#if defined(CONFIG_DAVINCI_SPI_EEPROM) || \
    defined(CONFIG_DAVINCI_SPI_EEPROM_MODULE)
	[0] = {
		.modalias = DAVINCI_SPI_EEPROM_NAME,
		.platform_data = &davinci_8k_spi_eeprom_info,
		.controller_data = &davinci_spi_eeprom_spi_cfg,
		.mode = SPI_MODE_0,
		.irq = 0,
		.max_speed_hz = 2 * 1000 * 1000 /* max sample rate at 3V */ ,
		.bus_num = 0,
		.chip_select = 0,
	},
#endif
};

static int davinci_spi_register(struct platform_device *pdev,
		struct spi_board_info *bi, unsigned int bi_size)
{
	int ret;

	ret = platform_device_register(pdev);
	if (ret == 0)
		ret = spi_register_board_info(bi, bi_size);

	return ret;
}

/*
 * This function initializes the GPIOs used by the SPI module
 * and it also registers the spi mastere device with the platform
 * and the spi slave devices with the spi bus
 */
static int __init davinci_spi_board_init(void)
{
	int ret = 0;

	if (cpu_is_davinci_dm6467())
		ret = davinci_spi_register(&dm646x_spi_pdev,
				dm646x_spi_board_info,
				ARRAY_SIZE(dm646x_spi_board_info));
	else if (cpu_is_davinci_dm355())
		ret = davinci_spi_register(&dm355_spi_pdev,
				dm3xx_spi_board_info,
				ARRAY_SIZE(dm3xx_spi_board_info));
	else if (cpu_is_davinci_dm365())
		ret = davinci_spi_register(&dm365_spi_pdev,
				dm3xx_spi_board_info,
				ARRAY_SIZE(dm3xx_spi_board_info));
	else
		pr_info("davinci_spi_board_init: NO spi support\n");

	return ret;
}

static void __exit davinci_spi_board_exit(void)
{
	/* nothing to be done */
}

module_init(davinci_spi_board_init);
module_exit(davinci_spi_board_exit);
