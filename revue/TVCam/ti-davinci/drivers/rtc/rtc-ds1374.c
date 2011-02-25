/*
 * RTC client/driver for the Maxim/Dallas DS1374 Real-Time Clock over I2C
 *
 * Based on code by Randy Vinson <rvinson@mvista.com>,
 * which was based on the m41t00.c by Mark Greer <mgreer@mvista.com>.
 *
 * Copyright (C) 2006-2007 Freescale Semiconductor
 *
 * 2005 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */
/*
 * It would be more efficient to use i2c msgs/i2c_transfer directly but, as
 * recommened in .../Documentation/i2c/writing-clients section
 * "Sending and receiving", using SMBus level communication is preferred.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/rtc.h>
#include <linux/bcd.h>
#include <linux/workqueue.h>

#define DS1374_REG_TOD0		0x00 /* Time of Day */
#define DS1374_REG_TOD1		0x01
#define DS1374_REG_TOD2		0x02
#define DS1374_REG_TOD3		0x03
#define DS1374_REG_WDALM0	0x04 /* Watchdog/Alarm */
#define DS1374_REG_WDALM1	0x05
#define DS1374_REG_WDALM2	0x06
#define DS1374_REG_CR		0x07 /* Control */
#define DS1374_REG_CR_AIE	0x01 /* Alarm Int. Enable */
#define DS1374_REG_CR_WDALM	0x20 /* 1=Watchdog, 0=Alarm */
#define DS1374_REG_CR_WACE	0x40 /* WD/Alarm counter enable */
#define DS1374_REG_SR		0x08 /* Status */
#define DS1374_REG_SR_OSF	0x80 /* Oscillator Stop Flag */
#define DS1374_REG_SR_AF	0x01 /* Alarm Flag */
#define DS1374_REG_TCR		0x09 /* Trickle Charge */

struct ds1374 {
	struct i2c_client client;
	struct rtc_device *rtc;
	struct work_struct work;

	/* The mutex protects alarm operations, and prevents a race
	 * between the enable_irq() in the workqueue and the free_irq()
	 * in the remove function.
	 */
	struct mutex mutex;
	int exiting;
};

static struct i2c_driver ds1374_driver;

static int ds1374_read_rtc(struct i2c_client *client, u32 *time,
                           int reg, int nbytes)
{
	u8 buf[I2C_SMBUS_BLOCK_MAX];
	int ret;
	int i;

	if (nbytes > 4) {
		WARN_ON(1);
		return -EINVAL;
	}

	ret = i2c_smbus_read_i2c_block_data(client, reg, buf);

	if (ret < 0)
		return ret;
	if (ret < nbytes)
		return -EIO;

	for (i = nbytes - 1, *time = 0; i >= 0; i--)
		*time = (*time << 8) | buf[i];

	return 0;
}

static int ds1374_write_rtc(struct i2c_client *client, u32 time,
                            int reg, int nbytes)
{
	u8 buf[4];
	int i;

	if (nbytes > 4) {
		WARN_ON(1);
		return -EINVAL;
	}

	for (i = 0; i < nbytes; i++) {
		buf[i] = time & 0xff;
		time >>= 8;
	}

	return i2c_smbus_write_i2c_block_data(client, reg, nbytes, buf);
}

static int ds1374_check_rtc_status(struct i2c_client *client)
{
	int ret = 0;
	int control, stat;

	stat = i2c_smbus_read_byte_data(client, DS1374_REG_SR);
	if (stat < 0)
		return stat;

	if (stat & DS1374_REG_SR_OSF)
		dev_warn(&client->dev,
		         "oscillator discontinuity flagged, "
		         "time unreliable\n");

	stat &= ~(DS1374_REG_SR_OSF | DS1374_REG_SR_AF);

	ret = i2c_smbus_write_byte_data(client, DS1374_REG_SR, stat);
	if (ret < 0)
		return ret;

	/* If the alarm is pending, clear it before requesting
	 * the interrupt, so an interrupt event isn't reported
	 * before everything is initialized.
	 */

	control = i2c_smbus_read_byte_data(client, DS1374_REG_CR);
	if (control < 0)
		return control;

	control &= ~(DS1374_REG_CR_WACE | DS1374_REG_CR_AIE);
	return i2c_smbus_write_byte_data(client, DS1374_REG_CR, control);
}

static int ds1374_read_time(struct device *dev, struct rtc_time *time)
{
	struct ds1374 *ds1374 = dev_get_drvdata(dev);
	u32 itime;
	int ret;

	ret = ds1374_read_rtc(&ds1374->client, &itime, DS1374_REG_TOD0, 4);
	if (!ret)
		rtc_time_to_tm(itime, time);

	return ret;
}

static int ds1374_set_time(struct device *dev, struct rtc_time *time)
{
	struct ds1374 *ds1374 = dev_get_drvdata(dev);
	unsigned long itime;

	rtc_tm_to_time(time, &itime);
	return ds1374_write_rtc(&ds1374->client, itime, DS1374_REG_TOD0, 4);
}

static int ds1374_ioctl(struct device *dev, unsigned int cmd, unsigned long arg)
{
	struct ds1374 *ds1374 = dev_get_drvdata(dev);
	struct i2c_client *client = &ds1374->client;
	int ret = -ENOIOCTLCMD;

	mutex_lock(&ds1374->mutex);

	switch (cmd) {
	case RTC_AIE_OFF:
		ret = i2c_smbus_read_byte_data(client, DS1374_REG_CR);
		if (ret < 0)
			goto out;

		ret &= ~DS1374_REG_CR_WACE;

		ret = i2c_smbus_write_byte_data(client, DS1374_REG_CR, ret);
		if (ret < 0)
			goto out;

		break;

	case RTC_AIE_ON:
		ret = i2c_smbus_read_byte_data(client, DS1374_REG_CR);
		if (ret < 0)
			goto out;

		ret |= DS1374_REG_CR_WACE | DS1374_REG_CR_AIE;
		ret &= ~DS1374_REG_CR_WDALM;

		ret = i2c_smbus_write_byte_data(client, DS1374_REG_CR, ret);
		if (ret < 0)
			goto out;

		break;
	}

out:
	mutex_unlock(&ds1374->mutex);
	return ret;
}

static struct rtc_class_ops ds1374_rtc_ops = {
	.read_time = ds1374_read_time,
	.set_time = ds1374_set_time,
	.ioctl = ds1374_ioctl,
};

static struct i2c_driver ds1374_driver;

static int ds1374_probe(struct i2c_adapter *adapter, int address, int kind)
{
	struct ds1374 *ds1374;
	struct i2c_client *client;
	int ret;

	ds1374 = kzalloc(sizeof(struct ds1374), GFP_KERNEL);
	if (!ds1374)
		return -ENOMEM;

	client = &ds1374->client;
	client->addr = address;
	client->adapter = adapter;
	client->driver = &ds1374_driver;
	client->flags = 0;
	strlcpy(client->name, "ds1374", I2C_NAME_SIZE);

	i2c_set_clientdata(client, ds1374);

	mutex_init(&ds1374->mutex);

	ret = i2c_attach_client(client);
	if (ret)
		goto attach_failed;

	ret = ds1374_check_rtc_status(client);
	if (ret)
		goto status_failed;

	ds1374->rtc = rtc_device_register(client->name, &client->dev,
	                                  &ds1374_rtc_ops, THIS_MODULE);
	if (IS_ERR(ds1374->rtc)) {
		ret = PTR_ERR(ds1374->rtc);
		dev_err(&client->dev, "unable to register the class device\n");
		goto rtc_register_failed;
	}

	return 0;

rtc_register_failed:
status_failed:
	i2c_detach_client(client);
attach_failed:
	i2c_set_clientdata(client, NULL);
	kfree(ds1374);
	printk(KERN_ERR "ds1374: failed to probe\n");
	return ret;
}

static unsigned short ignore[] = { I2C_CLIENT_END };
static unsigned short normal_addr[] = { 0x68, I2C_CLIENT_END };

static struct i2c_client_address_data addr_data = {
	.normal_i2c = normal_addr,
	.probe = ignore,
	.ignore = ignore,
};

static int ds1374_attach(struct i2c_adapter *adap)
{
	return i2c_probe(adap, &addr_data, ds1374_probe);
}

static int ds1374_detach(struct i2c_client *client)
{
	int ret;
	struct ds1374 *ds1374 = i2c_get_clientdata(client);

	rtc_device_unregister(ds1374->rtc);

	ret = i2c_detach_client(client);
	if (ret)
		return ret;

	kfree(i2c_get_clientdata(client));

	return 0;
}

static struct i2c_driver ds1374_driver = {
	.driver = {
		.name = "ds1374",
		.owner = THIS_MODULE,
	},
	.id = I2C_DRIVERID_DS1374,
	.attach_adapter = ds1374_attach,
	.detach_client = ds1374_detach,
};

static int __init ds1374_init(void)
{
	return i2c_add_driver(&ds1374_driver);
}

static void __exit ds1374_exit(void)
{
	i2c_del_driver(&ds1374_driver);
}

module_init(ds1374_init);
module_exit(ds1374_exit);

MODULE_AUTHOR("Scott Wood <scottwood@freescale.com>");
MODULE_DESCRIPTION("Maxim/Dallas DS1374 RTC Driver");
MODULE_LICENSE("GPL");
