/*
 * Copyright 2008 MontaVista Software Inc.
 * Author: Aleksey Makarov <amakarov@ru.mvista.com>
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/delay.h>

#include <linux/uaccess.h>

#include <asm/arch/i2c-client.h>

#define CIR_DM6446_DEV_COUNT	1

static dev_t cir_dm6446_dev;
static u32 cir_dm6446_major_number;
static u32 cir_dm6446_minor_number;
static struct cdev cir_dm6446_cdev;

static struct class *cir_class;

static void cir_dm6446_platform_release(struct device *device)
{
}

static struct platform_device cir_dm6446_device = {
	.name = "cir_dm6446",
	.id = 0,
	.dev = {
		.release = cir_dm6446_platform_release,
	}
};

static struct device_driver cir_dm6446_driver = {
	.name = "cir_dm6446",
	.bus = &platform_bus_type,
};


#define MESSAGE_LENGTH 255
static u8 message[MESSAGE_LENGTH];
static unsigned int next_value;

static inline u8 get_values_number(void)
{
	if (message[0] < 2)
		return 0;

	return (message[0] - 2) / 2;
}

static int read_ir_values(void)
{
	int err;

	message[0] = 2;
	message[1] = 2;
	err = davinci_i2c_write(2, message, 0x23);
	if (err)
		return err;

	msleep(1);

	err = davinci_i2c_read(MESSAGE_LENGTH, message, 0x23);
	if (err)
		return err;

	msleep(1);

	next_value = 0;

	return 0;
}

static int get_next_value(u16 *value)
{
	unsigned int index;
	int err;

	if (get_values_number() <= next_value) {
		err = read_ir_values();
		if (err)
			return err;
	}

	if (get_values_number() <= next_value)
		return 1;

	index = next_value * 2 + 2;
	*value = message[index] | (message[index + 1] << 8);
	next_value++;

	return 0;
}

ssize_t cir_dm6446_read(struct file *file, char __user *buff,
		size_t size, loff_t *loff)
{
	u16 value;
	ssize_t retval = 0;
	int err;

	if (size < sizeof(u16)) {
		printk(KERN_ERR "Invalid size requested for read\n");
		return -EINVAL;
	}

	while (size >= sizeof(u16)) {

		err = get_next_value(&value);
		if (err < 0)
			return err;
		if (err == 1)
			break;

		if (copy_to_user(buff, &value, sizeof(value)) != 0)
			return -EFAULT;

		buff += sizeof(u16);
		size -= sizeof(u16);
		*loff += sizeof(u16);
		retval += sizeof(u16);

	}

	return retval;
}

static struct file_operations cir_dm6446_fops = {
	.owner   = THIS_MODULE,
	.read    = cir_dm6446_read,
};

static int __init cir_dm6446_init(void)
{
	s8 retval = 0;

	cir_dm6446_major_number = 0;
	cir_dm6446_minor_number = 0;
	cir_class = NULL;

	retval = alloc_chrdev_region(&cir_dm6446_dev, cir_dm6446_minor_number,
			CIR_DM6446_DEV_COUNT, "/dev/cir");
	if (retval < 0) {
		printk(KERN_ERR "Unable to register the CIR device\n");
		retval = -ENODEV;
		goto failure;
	}

	cir_dm6446_major_number = MAJOR(cir_dm6446_dev);

	cdev_init(&cir_dm6446_cdev, &cir_dm6446_fops);
	cir_dm6446_cdev.owner = THIS_MODULE;
	cir_dm6446_cdev.ops = &cir_dm6446_fops;

	retval = cdev_add(&cir_dm6446_cdev, cir_dm6446_dev,
			CIR_DM6446_DEV_COUNT);
	if (retval) {
		unregister_chrdev_region(cir_dm6446_dev, CIR_DM6446_DEV_COUNT);
		printk(KERN_ERR "Error %d adding CIR device\n", retval);
		goto failure;
	}

	cir_class = class_create(THIS_MODULE, "cir");

	if (!cir_class) {
		unregister_chrdev_region(cir_dm6446_dev, CIR_DM6446_DEV_COUNT);
		cdev_del(&cir_dm6446_cdev);
		goto failure;
	}

	if (driver_register(&cir_dm6446_driver) != 0) {
		unregister_chrdev_region(cir_dm6446_dev, CIR_DM6446_DEV_COUNT);
		cdev_del(&cir_dm6446_cdev);
		class_destroy(cir_class);
		goto failure;
	}
	/* Register the drive as a platform device */
	if (platform_device_register(&cir_dm6446_device) != 0) {
		driver_unregister(&cir_dm6446_driver);
		unregister_chrdev_region(cir_dm6446_dev, CIR_DM6446_DEV_COUNT);
		cdev_del(&cir_dm6446_cdev);
		class_destroy(cir_class);
		goto failure;
	}

	cir_dm6446_dev = MKDEV(cir_dm6446_major_number, 0);

	class_device_create(cir_class, NULL, cir_dm6446_dev, NULL, "cir");

 failure:
	return retval;
}

static void __exit cir_dm6446_exit(void)
{
	printk(KERN_INFO "Unregistering the CIR device");

	driver_unregister(&cir_dm6446_driver);
	platform_device_unregister(&cir_dm6446_device);
	cir_dm6446_dev = MKDEV(cir_dm6446_major_number, 0);
	class_device_destroy(cir_class, cir_dm6446_dev);
	class_destroy(cir_class);
	cdev_del(&cir_dm6446_cdev);
	unregister_chrdev_region(cir_dm6446_dev, CIR_DM6446_DEV_COUNT);

	return;
}

module_init(cir_dm6446_init);
module_exit(cir_dm6446_exit);

MODULE_AUTHOR("MontaVista");
MODULE_DESCRIPTION("Consumer Infrared (CIR) Driver for TI DaVinci DM6446");
MODULE_LICENSE("GPL");
