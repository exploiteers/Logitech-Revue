/*
 * AMCC Sequoia/Rainier NAND flash specific routines
 *
 * Based on the warp-nand.c which is:
 *   Copyright (c) 2008 PIKA Technologies
 *     Sean MacLennan <smaclennan@pikatech.com>
 */

#if defined(CONFIG_MTD_NAND_NDFC) || defined(CONFIG_MTD_NAND_NDFC_MODULE)
#include <linux/platform_device.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/ndfc.h>

#include <asm/machdep.h>


#define CS_NAND_0	3	/* use chip select 3 for NAND device 0 */

#define SEQUOIA_NAND_FLASH_REG_ADDR	0x1D0000000ULL
#define SEQUOIA_NAND_FLASH_REG_SIZE	0x2000

static struct resource sequoia_ndfc = {
	.start = SEQUOIA_NAND_FLASH_REG_ADDR,
	.end   = SEQUOIA_NAND_FLASH_REG_ADDR + SEQUOIA_NAND_FLASH_REG_SIZE - 1,
	.flags = IORESOURCE_MEM,
};

static struct mtd_partition nand_parts[] = {
	{
		.name   = "u-boot-nand",
		.offset = 0,
		.size   = 0x0080000,
	},
	{
		.name   = "kernel-nand",
		.offset = 0x0080000,
		.size   = 0x0180000,
	},
	{
	 	.name   = "filesystem",
	 	.offset = 0x0200000,
	 	.size   = 0x1e00000,
	 },
};

struct ndfc_controller_settings sequoia_ndfc_settings = {
	.ccr_settings = (NDFC_CCR_BS(CS_NAND_0) | NDFC_CCR_ARAC1),
	.ndfc_erpn = 0,
};

static struct ndfc_chip_settings sequoia_chip0_settings = {
	.bank_settings = 0x80002222,
};

struct platform_nand_ctrl sequoia_nand_ctrl = {
	.priv = &sequoia_ndfc_settings,
};

static struct platform_device sequoia_ndfc_device = {
	.name = "ndfc-nand",
	.id = 0,
	.dev = {
		.platform_data = &sequoia_nand_ctrl,
	},
	.num_resources = 1,
	.resource = &sequoia_ndfc,
};

static struct platform_nand_chip sequoia_nand_chip0 = {
	.nr_chips = 1,
	.chip_offset = CS_NAND_0,
	.nr_partitions = ARRAY_SIZE(nand_parts),
	.partitions = nand_parts,
	.chip_delay = 50,
	.priv = &sequoia_chip0_settings,
};

static struct platform_device sequoia_nand_device = {
	.name = "ndfc-chip",
	.id = 0,
	.num_resources = 0,
	.dev = {
		.platform_data = &sequoia_nand_chip0,
		.parent = &sequoia_ndfc_device.dev,
	}
};

static int sequoia_setup_nand_flash(void)
{
	platform_device_register(&sequoia_ndfc_device);
	platform_device_register(&sequoia_nand_device);

	return 0;
}

machine_device_initcall(sequoia, sequoia_setup_nand_flash);
machine_device_initcall(rainier, sequoia_setup_nand_flash);

#endif /* CONFIG_MTD_NAND_NDFC || CONFIG_MTD_NAND_NDFC_MODULE */
