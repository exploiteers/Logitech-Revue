/*
 * Freescale UPM NAND driver.
 *
 * Copyright (c) 2007  MontaVista Software, Inc.
 * Copyright (c) 2007  Anton Vorontsov <avorontsov@ru.mvista.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/nand_ecc.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/mtd.h>
#include <linux/io.h>
#include <asm/of_platform.h>
#include <asm/gpio.h>
#include <asm/fsl_upm.h>

struct upm_data {
	struct device *dev;
	struct mtd_info mtd;
	struct nand_chip chip;
	int last_ctrl;
#ifdef CONFIG_MTD_PARTITIONS
	struct mtd_partition *parts;
#endif

	struct fsl_upm upm;

	int width;
	int upm_addr_offset;
	int upm_cmd_offset;
	void __iomem *io_base;
	int rnb_gpio;
	const u32 *wait_pattern;
	const u32 *wait_write;
	int chip_delay;
};

#define to_upm_data(mtd) container_of(mtd, struct upm_data, mtd)

static int upm_chip_ready(struct mtd_info *mtd)
{
	struct upm_data *ud = to_upm_data(mtd);

	if (gpio_get_value(ud->rnb_gpio))
		return 1;

	return 0;
}

static void upm_wait_rnb(struct upm_data *ud)
{
	int cnt = 1000000;

	if (ud->rnb_gpio >= 0) {
		while (--cnt && !upm_chip_ready(&ud->mtd))
			cpu_relax();
	}

	if (!cnt)
		dev_err(ud->dev, "tired waiting for RNB\n");
}

static void upm_cmd_ctrl(struct mtd_info *mtd, int cmd, unsigned int ctrl)
{
	struct upm_data *ud = to_upm_data(mtd);

	if (!(ctrl & ud->last_ctrl)) {
		fsl_upm_end_pattern(&ud->upm);

		if (cmd == NAND_CMD_NONE)
			return;

		ud->last_ctrl = ctrl & (NAND_ALE | NAND_CLE);
	}

	if (ctrl & NAND_CTRL_CHANGE) {
		if (ctrl & NAND_ALE)
			fsl_upm_start_pattern(&ud->upm, ud->upm_addr_offset);
		else if (ctrl & NAND_CLE)
			fsl_upm_start_pattern(&ud->upm, ud->upm_cmd_offset);
	}

	fsl_upm_run_pattern(&ud->upm, ud->io_base, ud->width, cmd);

	if (ud->wait_pattern)
		upm_wait_rnb(ud);
}

static uint8_t upm_read_byte(struct mtd_info *mtd)
{
	struct upm_data *ud = to_upm_data(mtd);

	return in_8(ud->chip.IO_ADDR_R);
}

static void upm_read_buf(struct mtd_info *mtd, uint8_t *buf, int len)
{
	struct upm_data *ud = to_upm_data(mtd);
	int i;

	for (i = 0; i < len; i++)
		buf[i] = in_8(ud->chip.IO_ADDR_R);
}

static void upm_write_buf(struct mtd_info *mtd, const uint8_t *buf, int len)
{
	struct upm_data *ud = to_upm_data(mtd);
	int i;

	for (i = 0; i < len; i++) {
		out_8(ud->chip.IO_ADDR_W, buf[i]);
		if (ud->wait_write)
			upm_wait_rnb(ud);
	}
}

static int __devinit upm_chip_init(struct upm_data *ud)
{
	int ret;
#ifdef CONFIG_MTD_PARTITIONS
	static const char *part_types[] = { "cmdlinepart", NULL, };
#endif

	ud->chip.IO_ADDR_R = ud->io_base;
	ud->chip.IO_ADDR_W = ud->io_base;
	ud->chip.cmd_ctrl = upm_cmd_ctrl;
	ud->chip.chip_delay = ud->chip_delay;
	ud->chip.read_byte = upm_read_byte;
	ud->chip.read_buf = upm_read_buf;
	ud->chip.write_buf = upm_write_buf;
	ud->chip.ecc.mode = NAND_ECC_SOFT;

	if (ud->rnb_gpio >= 0)
		ud->chip.dev_ready = upm_chip_ready;

	ud->mtd.priv = &ud->chip;
	ud->mtd.owner = THIS_MODULE;

	ret = nand_scan(&ud->mtd, 1);
	if (ret)
		return ret;

	ud->mtd.name = ud->dev->bus_id;

#ifdef CONFIG_MTD_PARTITIONS
	ret = parse_mtd_partitions(&ud->mtd, part_types, &ud->parts, 0);
	if (ret > 0)
		return add_mtd_partitions(&ud->mtd, ud->parts, ret);
#endif
	return add_mtd_device(&ud->mtd);
}

static int __devinit upm_chip_probe(struct of_device *ofdev,
				    const struct of_device_id *ofid)
{
	struct upm_data *ud;
	struct resource io_res;
	const u32 *prop;
	int ret;
	int size;

	ud = kzalloc(sizeof(*ud), GFP_KERNEL);
	if (!ud)
		return -ENOMEM;

	ret = of_address_to_resource(ofdev->node, 0, &io_res);
	if (ret) {
		dev_err(&ofdev->dev, "can't get IO base\n");
		goto err;
	}

	prop = of_get_property(ofdev->node, "width", &size);
	if (!prop || size != sizeof(u32)) {
		dev_err(&ofdev->dev, "can't get chip width\n");
		goto err;
	}
	ud->width = *prop * 8;

	prop = of_get_property(ofdev->node, "upm", &size);
	if (!prop || size < 1) {
		dev_err(&ofdev->dev, "can't get UPM to use\n");
		ret = -EINVAL;
		goto err;
	}

	ret = fsl_upm_get_for(ofdev->node, (const char *)prop, &ud->upm);
	if (ret) {
		dev_err(&ofdev->dev, "can't get FSL UPM\n");
		goto err;
	}

	prop = of_get_property(ofdev->node, "upm-addr-offset", &size);
	if (!prop || size != sizeof(u32)) {
		dev_err(&ofdev->dev, "can't get UPM address offset\n");
		ret = -EINVAL;
		goto err;
	}
	ud->upm_addr_offset = *prop;

	prop = of_get_property(ofdev->node, "upm-cmd-offset", &size);
	if (!prop || size != sizeof(u32)) {
		dev_err(&ofdev->dev, "can't get UPM command offset\n");
		ret = -EINVAL;
		goto err;
	}
	ud->upm_cmd_offset = *prop;

	ud->rnb_gpio = of_get_gpio(ofdev->node, 0);
	if (ud->rnb_gpio >= 0) {
		ret = gpio_request(ud->rnb_gpio, ofdev->dev.bus_id);
		if (ret) {
			dev_err(&ofdev->dev, "can't request RNB gpio\n");
			goto err;
		}
		gpio_direction_input(ud->rnb_gpio);
	} else if (ud->rnb_gpio == -EINVAL) {
		dev_err(&ofdev->dev, "specified RNB gpio is invalid\n");
		goto err;
	}

	ud->io_base = ioremap(io_res.start, io_res.end - io_res.start + 1);
	if (!ud->io_base) {
		ret = -ENOMEM;
		goto err;
	}

	ud->dev = &ofdev->dev;
	ud->last_ctrl = NAND_CLE;
	ud->wait_pattern = of_get_property(ofdev->node, "wait-pattern", NULL);
	ud->wait_write = of_get_property(ofdev->node, "wait-write", NULL);

	prop = of_get_property(ofdev->node, "chip-delay", NULL);
	if (prop)
		ud->chip_delay = *prop;
	else
		ud->chip_delay = 50;

	ret = upm_chip_init(ud);
	if (ret)
		goto err;

	dev_set_drvdata(&ofdev->dev, ud);

	return 0;

err:
	if (fsl_upm_got(&ud->upm))
		fsl_upm_free(&ud->upm);

	if (ud->rnb_gpio >= 0)
		gpio_free(ud->rnb_gpio);

	if (ud->io_base)
		iounmap(ud->io_base);

	kfree(ud);

	return ret;
}

static int __devexit upm_chip_remove(struct of_device *ofdev)
{
	struct upm_data *ud = dev_get_drvdata(&ofdev->dev);

	nand_release(&ud->mtd);

	fsl_upm_free(&ud->upm);

	iounmap(ud->io_base);

	if (ud->rnb_gpio >= 0)
		gpio_free(ud->rnb_gpio);

	kfree(ud);

	return 0;
}

static struct of_device_id of_upm_nand_match[] = {
	{ .compatible = "fsl,upm-nand" },
	{},
};
MODULE_DEVICE_TABLE(of, of_upm_nand_match);

static struct of_platform_driver of_upm_chip_driver = {
	.name		= "fsl_upm_nand",
	.match_table	= of_upm_nand_match,
	.probe		= upm_chip_probe,
	.remove		= __devexit_p(upm_chip_remove),
};

static int __init upm_nand_init(void)
{
	return of_register_platform_driver(&of_upm_chip_driver);
}

static void __exit upm_nand_exit(void)
{
	of_unregister_platform_driver(&of_upm_chip_driver);
}

module_init(upm_nand_init);
module_exit(upm_nand_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Anton Vorontsov <avorontsov@ru.mvista.com>");
MODULE_DESCRIPTION("Driver for NAND chips working through Freescale "
		   "LocalBus User-Programmable Machine");
