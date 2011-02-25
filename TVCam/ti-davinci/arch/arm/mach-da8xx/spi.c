/*
 * TI DA8xx EVM SPI platform
 *
 * Copyright (C) 2008 Monta Vista Software Inc. <source@mvista.com>
 *
 * Based on: arch/arm/mach-davinci/davinci_spi_platform.c
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * ----------------------------------------------------------------------------
 *
 * This file came directly from spi_platform_init.c.  This file has been
 * generalized to all DaVinci variants.  This file should replace
 * spi_platform_init.c
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

static struct davinci_spi_platform_data da8xx_spi_pdata0 = {
	.version = DAVINCI_SPI_VERSION_2,
	.num_chipselect = 1,
	.clk_name = "SPI0CLK",
};

static struct resource da8xx_spi_resources0[] = {
	[0] = {
		.start = DA8XX_SPI0_BASE,
		.end = DA8XX_SPI0_BASE + 0xfff,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_DA8XX_SPINT0,
		.end = IRQ_DA8XX_SPINT0,
		.flags = IORESOURCE_IRQ,
	},
	[2] = {
		.start = DA8XX_DMACH_SPI0_RX,
		.end = DA8XX_DMACH_SPI0_RX,
		.flags = IORESOURCE_DMA | IORESOURCE_DMA_RX_CHAN,
	},
	[3] = {
		.start = DA8XX_DMACH_SPI0_TX,
		.end = DA8XX_DMACH_SPI0_TX,
		.flags = IORESOURCE_DMA | IORESOURCE_DMA_TX_CHAN,
	},
	[4] = {
		.start = EVENTQ_1,
		.end = EVENTQ_1,
		.flags = IORESOURCE_DMA | IORESOURCE_DMA_EVENT_Q,
	},
};

static struct platform_device da8xx_spi_pdev0 = {
	.name = "dm_spi",
	.id = 0,
	.resource = da8xx_spi_resources0,
	.num_resources = ARRAY_SIZE(da8xx_spi_resources0),
	.dev = {
		.platform_data = &da8xx_spi_pdata0,
	},
};

static u8 da8xx_spi1_chip_sel[] = {
	DAVINCI_SPI_INTERN_CS, DAVINCI_SPI_INTERN_CS, 58,
};

static struct davinci_spi_platform_data da8xx_spi_pdata1 = {
	.version = DAVINCI_SPI_VERSION_2,
	.num_chipselect = 3,
	.clk_name = "SPI1CLK",
	.chip_sel = da8xx_spi1_chip_sel,
};

static struct resource da8xx_spi_resources1[] = {
	[0] = {
		.start = DA8XX_SPI1_BASE,
		.end = DA8XX_SPI1_BASE + 0xfff,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_DA8XX_SPINT1,
		.end = IRQ_DA8XX_SPINT1,
		.flags = IORESOURCE_IRQ,
	},
	[2] = {
		.start = DA8XX_DMACH_SPI1_RX,
		.end = DA8XX_DMACH_SPI1_RX,
		.flags = IORESOURCE_DMA | IORESOURCE_DMA_RX_CHAN,
	},
	[3] = {
		.start = DA8XX_DMACH_SPI1_TX,
		.end = DA8XX_DMACH_SPI1_TX,
		.flags = IORESOURCE_DMA | IORESOURCE_DMA_TX_CHAN,
	},
	[4] = {
		.start = EVENTQ_1,
		.end = EVENTQ_1,
		.flags = IORESOURCE_DMA | IORESOURCE_DMA_EVENT_Q,
	},
};

static struct platform_device da8xx_spi_pdev1 = {
	.name = "dm_spi",
	.id = 1,
	.resource = da8xx_spi_resources1,
	.num_resources = ARRAY_SIZE(da8xx_spi_resources1),
	.dev = {
		.platform_data = &da8xx_spi_pdata1,
	},
};

#if defined(CONFIG_MTD_SPI_FLASH) || defined(CONFIG_MTD_SPI_FLASH_MODULE)
static struct mtd_partition flash_partitions[] = {
	[0] = {
		.name = "U-Boot",
		.offset = 0,
		.size = SZ_256K,
		.mask_flags = MTD_WRITEABLE,
	},
	[1] = {
		.name = "U-Boot Environment",
		.offset = MTDPART_OFS_APPEND,
		.size = SZ_16K,
		.mask_flags = MTD_WRITEABLE,
	},
	[2] = {
		.name = "Linux",
		.offset = MTDPART_OFS_NXTBLK,
		.size = MTDPART_SIZ_FULL,
		.mask_flags = 0,
	},
};

struct davinci_spi_config_t w25x64_spi_cfg = {
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

struct mtd_spi_flash_info w25x64_spi_flash = {
	.name = "Windbond spi nand flash",
	.chip_sel = SCS0_SELECT,
	.parts = flash_partitions,
	.nr_parts = ARRAY_SIZE(flash_partitions),
};
#endif

static struct spi_board_info da8xx_spi_board_info0[] = {
#if defined(CONFIG_MTD_SPI_FLASH) || defined(CONFIG_MTD_SPI_FLASH_MODULE)
	[0] = {
		.modalias = MTD_SPI_FLASH_NAME,
		.platform_data = &w25x64_spi_flash,
		.controller_data = &w25x64_spi_cfg,
		.mode = SPI_MODE_0,
		.max_speed_hz = 30000000,	/* max sample rate at 3V */
		.bus_num = 0,
		.chip_select = 0,
	},
#endif
};

static int ak4588_place_holder;

struct davinci_spi_config_t ak4588_spi_cfg = {
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

static struct spi_board_info da8xx_spi_board_info1[] = {
	[0] = {
		.modalias = "AK4588 audio codec",
		.platform_data = &ak4588_place_holder,
		.controller_data = &ak4588_spi_cfg,
		.mode = SPI_MODE_0,
		.max_speed_hz = 2000000,	/* max sample rate at 3V */
		.bus_num = 1,
		.chip_select = 2,
	},
};

/*
 * This function registers the SPI master platform device and the SPI slave
 * devices with the SPI bus.
 */
static int __init da8xx_spi_register(struct platform_device *pdev,
				     struct spi_board_info *bi,
				     unsigned int bi_size)
{
	int ret = platform_device_register(pdev);

	if (ret)
		return ret;

	return spi_register_board_info(bi, bi_size);

}

static int __init da8xx_spi_board_init(void)
{
	int ret = da8xx_spi_register(&da8xx_spi_pdev0, da8xx_spi_board_info0,
				     ARRAY_SIZE(da8xx_spi_board_info0));

	if (ret)
		return ret;

	return da8xx_spi_register(&da8xx_spi_pdev1, da8xx_spi_board_info1,
				  ARRAY_SIZE(da8xx_spi_board_info1));
}

static void __exit da8xx_spi_board_exit(void)
{
	/* nothing to be done */
}

module_init(da8xx_spi_board_init);
module_exit(da8xx_spi_board_exit);
