/*
 * rtc driver for the MSP430 chip on the TI DM355 development board
 * Author: Aleksey Makarov, MontaVista Software, Inc. <source@mvista.com>
 *
 * 2008 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/rtc.h>
#include <linux/platform_device.h>
#include <asm/mach-types.h>

#include <asm/arch/ti_msp430.h>

#define NTRY 10

static int
read_rtc_1(unsigned long * value)
{
	int err;
	u8 v;
	int i;

	*value = 0;

	for (i = 0; i < 4; i++) {
		err = ti_msp430_read(0x12 + i, &v);
		if (err)
			return err;

		*value |= ((unsigned)v) << (i * 8);
	}

	return 0;
}

static int
read_rtc(unsigned long * value)
{
	unsigned long value1;
	int err;
	int iter = 0;

	err = read_rtc_1(value);
	if (err)
		return err;

	while (1) {
		err = read_rtc_1(&value1);
		if (err)
			return err;

		if (*value == value1)
			break;

		if ((*value + 1) == value1) {
			*value = value1;
			break;
		}

		if (iter++ > NTRY)
			return -EIO;

		*value = value1;
	}

	return 0;
}

static int
write_rtc_1(unsigned long value)
{
	int err;
	int i;

	for (i = 0; i < 4; i++) {
		err = ti_msp430_write(0x12 + i, (value >> (i * 8)) & 0xff);
		if (err)
			return err;
	}

	return 0;
}

static int
write_rtc(unsigned long value)
{
	int err;
	unsigned long value1;
	int iter = 0;

	while (1) {

		err = write_rtc_1(value);
		if (err)
			return err;

		err = read_rtc_1(&value1);
		if (err)
			return err;

		if (value == value1 || (value + 1) == value1)
			break;

		if (iter++ > NTRY)
			return -EIO;

	}

	return 0;
}

static int
dm355_read_time(struct device *dev, struct rtc_time *tm)
{
	unsigned long time;
	int err;

	err = read_rtc(&time);
	if (err)
		return err;

	rtc_time_to_tm(time, tm);

	return 0;
}

static int
dm355_set_time(struct device *dev, struct rtc_time *tm)
{
	unsigned long time;

	rtc_tm_to_time(tm, &time);

	return write_rtc(time);
}

static struct rtc_class_ops dm355_rtc_ops = {
	.read_time	= dm355_read_time,
	.set_time	= dm355_set_time,
};

static int __devinit dm355_rtc_probe(struct platform_device *pdev)
{
	struct rtc_device	*rtc;

	rtc = rtc_device_register(pdev->name, &pdev->dev,
			&dm355_rtc_ops, THIS_MODULE);
	if (IS_ERR(rtc))
		return PTR_ERR(rtc);

	platform_set_drvdata(pdev, rtc);
	return 0;
}

static int __devexit dm355_rtc_remove(struct platform_device *pdev)
{
	rtc_device_unregister(platform_get_drvdata(pdev));
	return 0;
}

static struct platform_driver dm355_rtc_driver = {
	.driver = {
		.name		= "rtc_davinci_dm355",
	},
	.probe		= dm355_rtc_probe,
	.remove		= __devexit_p(dm355_rtc_remove),
};

static int dm355_rtc_init(void)
{
	if (!machine_is_davinci_dm355_evm())
		return -ENODEV;

	return platform_driver_register(&dm355_rtc_driver);
}
module_init(dm355_rtc_init);

static void dm355_rtc_exit(void)
{
	platform_driver_unregister(&dm355_rtc_driver);
}
module_exit(dm355_rtc_exit);

MODULE_AUTHOR("Aleksey Makarov <amakarov@ru.mvista.com>");
MODULE_DESCRIPTION("RTC driver for TI DaVinci DM355");
MODULE_LICENSE("GPL");
