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

#include <asm/uaccess.h>
#include <asm/arch/ti_msp430.h>

#define CIR_DM355_DEV_COUNT	1

#define COMMAND_CODE		0x00
#define STATUS_CODE		0x01
#define IR_DATA_COUNT		0x16
#define IR_DATA_HIGH_BYTE	0x17
#define IR_DATA_LOW_BYTE	0x18

static dev_t cir_dm355_dev;
static u32 cir_dm355_major_number;
static u32 cir_dm355_minor_number;
static struct cdev cir_dm355_cdev;

static struct class *cir_class;

static void cir_dm355_platform_release(struct device *device)
{
}

static struct platform_device cir_dm355_device = {
	.name = "cir_dm355",
	.id = 0,
	.dev = {
		.release = cir_dm355_platform_release,
	}
};

static struct device_driver cir_dm355_driver = {
	.name = "cir_dm355",
	.bus = &platform_bus_type,
};

static int reset_done;

static int reset_cir(void)
{

	int err;

	/*reset status code register */
	err = ti_msp430_write(STATUS_CODE, 0x00);
	if (err) {
		printk (KERN_ERR "ti_msp430_read() error: %d\n", err);
		return err;
	}

	/* reinitialize IR remote */
	err = ti_msp430_write(COMMAND_CODE, 0x05);
	if (err) {
		printk (KERN_ERR "ti_msp430_read() error: %d\n", err);
		return err;
	}

	reset_done = 1;

	return 0;
}

static int read_status(void)
{

	int err;
	u8 value;

	err = ti_msp430_read(STATUS_CODE, &value);
	if (err) {
		printk (KERN_ERR "ti_msp430_read() error: %d\n", err);
		return err;
	}

	return (int)value;
}

ssize_t cir_dm355_read (struct file *file, char __user *buff, size_t size, loff_t
		  *loff)
{
	u8 value;
	u16 value_full;
	ssize_t retval = 0;
	int err;

	if (!reset_done) {
		err = reset_cir();
		if (err)
			return err;
	}

	if (size < sizeof(u16)) {
		printk (KERN_ERR "Invalid size requested for read\n");
		return -EINVAL;
	}

	while (size >= sizeof(u16)) {

		err = ti_msp430_read(IR_DATA_HIGH_BYTE, &value);
		if (err) {
			printk (KERN_ERR "ti_msp430_read() error: %d\n", err);
			return err;
		}

		value_full = value << 8;

		err = ti_msp430_read(IR_DATA_LOW_BYTE, &value);
		if (err) {
			printk (KERN_ERR "ti_msp430_read() error: %d\n", err);
			return err;
		}

		value_full |= value;

		if (value_full == 0xdead)
			break;

		if (copy_to_user(buff, &value_full, sizeof(value_full)) != 0)
			return -EFAULT;

		buff += sizeof(u16);
		size -= sizeof(u16);
		*loff += sizeof(u16);
		retval += sizeof(u16);

		err = read_status();
		if (err < 0)
			return err;
		if (err & 0x08) {
			reset_cir();
			return -EIO;
		}

	}

	return retval;
}

static struct file_operations cir_dm355_fops = {
	.owner   = THIS_MODULE,
	.read    = cir_dm355_read,
};

static int __init cir_dm355_init ( void )
{
	s8 retval = 0;

	cir_dm355_major_number = 0;
	cir_dm355_minor_number = 0;
	cir_class = NULL;

	retval = alloc_chrdev_region (&cir_dm355_dev, cir_dm355_minor_number,
			CIR_DM355_DEV_COUNT, "/dev/cir");
	if (retval < 0) {
		printk (KERN_ERR "Unable to register the CIR device\n");
		retval = -ENODEV;
		goto failure;
	}

	cir_dm355_major_number = MAJOR(cir_dm355_dev);

	cdev_init (&cir_dm355_cdev, &cir_dm355_fops);
	cir_dm355_cdev.owner = THIS_MODULE;
	cir_dm355_cdev.ops = &cir_dm355_fops;

	retval = cdev_add (&cir_dm355_cdev, cir_dm355_dev, CIR_DM355_DEV_COUNT);
	if (retval) {
		unregister_chrdev_region (cir_dm355_dev, CIR_DM355_DEV_COUNT);
		printk (KERN_ERR "Error %d adding CIR device\n", retval);
		goto failure;
	}

	cir_class = class_create(THIS_MODULE, "cir");

	if (!cir_class) {
		unregister_chrdev_region (cir_dm355_dev, CIR_DM355_DEV_COUNT);
		cdev_del(&cir_dm355_cdev);
		goto failure;
	}

	if (driver_register(&cir_dm355_driver) != 0) {
		unregister_chrdev_region (cir_dm355_dev, CIR_DM355_DEV_COUNT);
		cdev_del(&cir_dm355_cdev);
		class_destroy(cir_class);
		goto failure;
	}
	/* Register the drive as a platform device */
	if (platform_device_register(&cir_dm355_device) != 0) {
		driver_unregister(&cir_dm355_driver);
		unregister_chrdev_region (cir_dm355_dev, CIR_DM355_DEV_COUNT);
		cdev_del(&cir_dm355_cdev);
		class_destroy(cir_class);
		goto failure;
	}

	cir_dm355_dev = MKDEV(cir_dm355_major_number, 0);

	class_device_create(cir_class, NULL, cir_dm355_dev, NULL, "cir");

 failure:
	return retval;
}

static void __exit cir_dm355_exit ( void )
{
	printk (KERN_INFO "Unregistering the CIR device");

	driver_unregister(&cir_dm355_driver);
	platform_device_unregister(&cir_dm355_device);
	cir_dm355_dev = MKDEV(cir_dm355_major_number, 0);
	class_device_destroy(cir_class, cir_dm355_dev);
	class_destroy(cir_class);
	cdev_del(&cir_dm355_cdev);
	unregister_chrdev_region(cir_dm355_dev, CIR_DM355_DEV_COUNT);

	return;
}

module_init (cir_dm355_init);
module_exit (cir_dm355_exit);

MODULE_AUTHOR ( "MontaVista" );
MODULE_DESCRIPTION ( "Consumer Infrared (CIR) Driver for TI DaVinci DM355" );
MODULE_LICENSE ( "GPL" );
