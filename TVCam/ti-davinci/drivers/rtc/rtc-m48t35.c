/*
 * ST M48T35 RTC driver
 *
 * Copyright (c) 2008 MontaVista Software, Inc.
 * Copyright (c) 2007 Wind River Systems, Inc.
 *
 * Author: Randy Vinson <rvinson@mvista.com>
 *
 * Derived from m48t59.c by Mark Zhan <rongkai.zhan@windriver.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/rtc.h>
#include <linux/m48t35.h>
#include <linux/bcd.h>

#define M48T35_READ(reg)	pdata->read_byte(dev, reg)
#define M48T35_WRITE(val, reg)	pdata->write_byte(dev, reg, val)

#define M48T35_SET_BITS(mask, reg)	\
	M48T35_WRITE((M48T35_READ(reg) | (mask)), (reg))
#define M48T35_CLEAR_BITS(mask, reg)	\
	M48T35_WRITE((M48T35_READ(reg) & ~(mask)), (reg))

struct m48t35_private {
	void __iomem *ioaddr;
	unsigned int size; /* iomem size */
	struct rtc_device *rtc;
	spinlock_t lock; /* serialize the NVRAM and RTC access */
};

/*
 * This is the generic access method when the chip is memory-mapped
 */
static void
m48t35_mem_writeb(struct device *dev, u32 ofs, u8 val)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct m48t35_private *m48t35 = platform_get_drvdata(pdev);

	writeb(val, m48t35->ioaddr+ofs);
}

static u8
m48t35_mem_readb(struct device *dev, u32 ofs)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct m48t35_private *m48t35 = platform_get_drvdata(pdev);

	return readb(m48t35->ioaddr+ofs);
}

/*
 * NOTE: M48T35 only uses BCD mode
 */
static int m48t35_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct m48t35_plat_data *pdata = pdev->dev.platform_data;
	struct m48t35_private *m48t35 = platform_get_drvdata(pdev);
	u8 val;

	spin_lock(&m48t35->lock);
	/* Issue the READ command */
	M48T35_SET_BITS(M48T35_CNTL_READ, M48T35_CNTL);

	tm->tm_year	= BCD2BIN(M48T35_READ(M48T35_YEAR));
	/* tm_mon is 0-11 */
	tm->tm_mon	= BCD2BIN(M48T35_READ(M48T35_MONTH)) - 1;
	tm->tm_mday	= BCD2BIN(M48T35_READ(M48T35_MDAY));

	val = M48T35_READ(M48T35_WDAY);
	if ((val & M48T35_WDAY_CEB) && (val & M48T35_WDAY_CB)) {
		dev_dbg(dev, "Century bit is enabled\n");
		tm->tm_year += 100;	/* one century */
	}

	tm->tm_wday	= BCD2BIN(val & 0x07);
	tm->tm_hour	= BCD2BIN(M48T35_READ(M48T35_HOUR) & 0x3F);
	tm->tm_min	= BCD2BIN(M48T35_READ(M48T35_MIN) & 0x7F);
	tm->tm_sec	= BCD2BIN(M48T35_READ(M48T35_SEC) & 0x7F);

	/* Clear the READ bit */
	M48T35_CLEAR_BITS(M48T35_CNTL_READ, M48T35_CNTL);
	spin_unlock(&m48t35->lock);

	dev_dbg(dev, "RTC read time %04d-%02d-%02d %02d/%02d/%02d\n",
		tm->tm_year + 1900, tm->tm_mon, tm->tm_mday,
		tm->tm_hour, tm->tm_min, tm->tm_sec);
	return 0;
}

static int m48t35_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct m48t35_plat_data *pdata = pdev->dev.platform_data;
	struct m48t35_private *m48t35 = platform_get_drvdata(pdev);
	u8 val = 0;

	dev_dbg(dev, "RTC set time %04d-%02d-%02d %02d/%02d/%02d\n",
		tm->tm_year + 1900, tm->tm_mon, tm->tm_mday,
		tm->tm_hour, tm->tm_min, tm->tm_sec);

	spin_lock(&m48t35->lock);
	/* Issue the WRITE command */
	M48T35_SET_BITS(M48T35_CNTL_WRITE, M48T35_CNTL);

	M48T35_WRITE((BIN2BCD(tm->tm_sec) & 0x7F), M48T35_SEC);
	M48T35_WRITE((BIN2BCD(tm->tm_min) & 0x7F), M48T35_MIN);
	M48T35_WRITE((BIN2BCD(tm->tm_hour) & 0x3F), M48T35_HOUR);
	M48T35_WRITE((BIN2BCD(tm->tm_mday) & 0x3F), M48T35_MDAY);
	/* tm_mon is 0-11 */
	M48T35_WRITE((BIN2BCD(tm->tm_mon + 1) & 0x1F), M48T35_MONTH);
	M48T35_WRITE(BIN2BCD(tm->tm_year % 100), M48T35_YEAR);

	if (tm->tm_year/100)
		val = (M48T35_WDAY_CEB | M48T35_WDAY_CB);
	val |= (BIN2BCD(tm->tm_wday) & 0x07);
	M48T35_WRITE(val, M48T35_WDAY);

	/* Clear the WRITE bit */
	M48T35_CLEAR_BITS(M48T35_CNTL_WRITE, M48T35_CNTL);
	spin_unlock(&m48t35->lock);
	return 0;
}

static struct rtc_class_ops m48t35_rtc_ops = {
	.read_time	= m48t35_rtc_read_time,
	.set_time	= m48t35_rtc_set_time,
};

static ssize_t m48t35_nvram_read(struct kobject *kobj,
				char *buf, loff_t pos, size_t size)
{
	struct device *dev = container_of(kobj, struct device, kobj);
	struct platform_device *pdev = to_platform_device(dev);
	struct m48t35_plat_data *pdata = pdev->dev.platform_data;
	struct m48t35_private *m48t35 = platform_get_drvdata(pdev);
	ssize_t cnt = 0;

	for (; size > 0 && pos < M48T35_NVRAM_SIZE; cnt++, size--) {
		spin_lock(&m48t35->lock);
		*buf++ = M48T35_READ(cnt);
		spin_unlock(&m48t35->lock);
	}

	return cnt;
}

static ssize_t m48t35_nvram_write(struct kobject *kobj,
				char *buf, loff_t pos, size_t size)
{
	struct device *dev = container_of(kobj, struct device, kobj);
	struct platform_device *pdev = to_platform_device(dev);
	struct m48t35_plat_data *pdata = pdev->dev.platform_data;
	struct m48t35_private *m48t35 = platform_get_drvdata(pdev);
	ssize_t cnt = 0;

	for (; size > 0 && pos < M48T35_NVRAM_SIZE; cnt++, size--) {
		spin_lock(&m48t35->lock);
		M48T35_WRITE(*buf++, cnt);
		spin_unlock(&m48t35->lock);
	}

	return cnt;
}

static struct bin_attribute m48t35_nvram_attr = {
	.attr = {
		.name = "nvram",
		.mode = S_IRUGO | S_IWUSR,
		.owner = THIS_MODULE,
	},
	.read = m48t35_nvram_read,
	.write = m48t35_nvram_write,
	.size = M48T35_NVRAM_SIZE,
};

static int __devinit m48t35_rtc_probe(struct platform_device *pdev)
{
	struct m48t35_plat_data *pdata = pdev->dev.platform_data;
	struct m48t35_private *m48t35 = NULL;
	struct resource *res;
	int ret = -ENOMEM;

	/* This chip could be memory-mapped or I/O-mapped */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		res = platform_get_resource(pdev, IORESOURCE_IO, 0);
		if (!res)
			return -EINVAL;
	}

	if (res->flags & IORESOURCE_IO) {
		/* If we are I/O-mapped, the platform should provide
		 * the operations accessing chip registers.
		 */
		if (!pdata || !pdata->write_byte || !pdata->read_byte)
			return -EINVAL;
	} else if (res->flags & IORESOURCE_MEM) {
		/* we are memory-mapped */
		if (!pdata) {
			pdata = kzalloc(sizeof(*pdata), GFP_KERNEL);
			if (!pdata)
				return -ENOMEM;
			/* Ensure we only kmalloc platform data once */
			pdev->dev.platform_data = pdata;
		}

		/* Try to use the generic memory read/write ops */
		if (!pdata->write_byte)
			pdata->write_byte = m48t35_mem_writeb;
		if (!pdata->read_byte)
			pdata->read_byte = m48t35_mem_readb;
	}

	m48t35 = kzalloc(sizeof(*m48t35), GFP_KERNEL);
	if (!m48t35)
		return -ENOMEM;

	m48t35->size = res->end - res->start + 1;
	m48t35->ioaddr = ioremap(res->start, m48t35->size);
	if (!m48t35->ioaddr)
		goto out;

	m48t35->rtc = rtc_device_register("m48t35", &pdev->dev,
				&m48t35_rtc_ops, THIS_MODULE);
	if (IS_ERR(m48t35->rtc)) {
		ret = PTR_ERR(m48t35->rtc);
		goto out;
	}

	ret = sysfs_create_bin_file(&pdev->dev.kobj, &m48t35_nvram_attr);
	if (ret)
		goto out;

	spin_lock_init(&m48t35->lock);
	platform_set_drvdata(pdev, m48t35);

	return 0;

out:
	if (!IS_ERR(m48t35->rtc))
		rtc_device_unregister(m48t35->rtc);
	if (m48t35->ioaddr)
		iounmap(m48t35->ioaddr);
	if (m48t35)
		kfree(m48t35);
	return ret;
}

static int __devexit m48t35_rtc_remove(struct platform_device *pdev)
{
	struct m48t35_private *m48t35 = platform_get_drvdata(pdev);

	sysfs_remove_bin_file(&pdev->dev.kobj, &m48t35_nvram_attr);
	if (!IS_ERR(m48t35->rtc))
		rtc_device_unregister(m48t35->rtc);
	if (m48t35->ioaddr)
		iounmap(m48t35->ioaddr);
	platform_set_drvdata(pdev, NULL);
	kfree(m48t35);
	return 0;
}

/* work with hotplug and coldplug */
MODULE_ALIAS("platform:rtc-m48t35");

static struct platform_driver m48t35_rtc_driver = {
	.driver		= {
		.name	= "rtc-m48t35",
		.owner	= THIS_MODULE,
	},
	.probe		= m48t35_rtc_probe,
	.remove		= __devexit_p(m48t35_rtc_remove),
};

static int __init m48t35_rtc_init(void)
{
	return platform_driver_register(&m48t35_rtc_driver);
}

static void __exit m48t35_rtc_exit(void)
{
	platform_driver_unregister(&m48t35_rtc_driver);
}

module_init(m48t35_rtc_init);
module_exit(m48t35_rtc_exit);

MODULE_AUTHOR("Randy Vinson <rvinson@mvista.com>");
MODULE_DESCRIPTION("M48T35 RTC driver");
MODULE_LICENSE("GPL");
