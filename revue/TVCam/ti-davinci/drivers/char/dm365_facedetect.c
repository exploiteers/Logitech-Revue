/*
 * DM365 Face detect driver
 *
 * Character device driver for the DM365 Face detect hardware IP
 *
 * Copyright (C) 2009 Texas Instruments Incorporated - http://www.ti.com/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>	/* printk() */
#include <linux/slab.h>		/* kmalloc() */
#include <linux/fs.h>		/* everything... */
#include <linux/errno.h>	/* error codes */
#include <linux/types.h>	/* size_t */
#include <linux/cdev.h>		/* Used for struct cdev */
#include <linux/dma-mapping.h>	/* For class_simple_create */
#include <linux/interrupt.h>	/* For IRQ_HANDLED and irqreturn_t */
#include <linux/uaccess.h>	/* For VERIFY_ READ/WRITE/copy_from_user */
#include <linux/device.h>
#include <linux/platform_device.h>
#include <asm/arch/dm365_facedetect.h>
#include "dm365_facedetect_hw.h"

#define DRIVERNAME  "DM365FACEDETECT"

static struct class *facedetect_class;
struct device *facedetect_dev;

/* global object of facedetect_device structure */
static struct facedetect_device facdetectdevice;

/* Keeps track of how many times the device driver has been opened */
static atomic_t reference_count = ATOMIC_INIT(0);

int facedetect_open(struct inode *inode, struct file *filp)
{
	struct facedetect_params_t *config = NULL;

	if (atomic_inc_return(&reference_count) != 1) {
		dev_err(facedetect_dev,
			"facedetect_open: device is already openend\n");
		return -EBUSY;
	}

	/* allocate memory for a new configuration */
	config = kmalloc(sizeof(struct facedetect_params_t), GFP_KERNEL);
	if (!config)
		return -ENOMEM;

	/*
	 * Store the pointer of facedetect_params_t in private_data member of
	 * file and facedetect_params member of facedetect_device
	 */
	filp->private_data = config;
	facdetectdevice.opened = 1;

	return 0;
}

int facedetect_release(struct inode *inode, struct file *filp)
{
	/* get the configuratin from private_date member of file */
	struct facedetect_params_t *config = filp->private_data;
	struct facedetect_device *device = &facdetectdevice;

	/* Disable FD interrupt */
	facedetect_int_enable(0);

	if (atomic_dec_and_test(&reference_count)) {
		/* change the device status to available */
		device->opened = 0;
	}
	/* free the memory allocated to configuration */
	kfree(config);

	/*
	 * Assign null to private_data member of file and params
	 * member of device
	 */
	filp->private_data = NULL;

	return 0;

}

int facedetect_ioctl(struct inode *inode, struct file *filp, unsigned int cmd,
		     unsigned long arg)
{
	int ret = 0;

	/* Before decoding check for correctness of cmd */
	if (_IOC_TYPE(cmd) != FACE_DETECT_IOC_BASE) {
		dev_dbg(facedetect_dev, "Bad command Value \n");
		return -1;
	}
	if (_IOC_NR(cmd) > FACE_DETECT_IOC_MAXNR) {
		dev_dbg(facedetect_dev, "Bad command Value\n");
		return -1;
	}

	/* Verify accesses */
	if (_IOC_DIR(cmd) & _IOC_READ)
		ret = !access_ok(VERIFY_WRITE, (void *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		ret = !access_ok(VERIFY_READ, (void *)arg, _IOC_SIZE(cmd));
	if (ret) {
		dev_err(facedetect_dev, "access denied\n");
		return -1;	/* error in access */
	}

	switch (cmd) {

	case FACE_DETECT_SET_HW_PARAM:
		ret =
		    facedetect_set_hw_param((struct facedetect_params_t *)arg);
		break;

	case FACE_DETECT_EXECUTE:
		ret = facedetect_execute((struct facedetect_params_t *)arg);
		break;

	case FACE_DETECT_SET_BUFFER:
		ret = facedetect_set_buffer((struct facedetect_params_t *)arg);
		break;

	case FACE_DETECT_GET_HW_PARAM:
		ret =
		    facedetect_get_hw_param((struct facedetect_params_t *)arg);
		break;

	default:
		ret = -EINVAL;
		break;
	}
	return ret;

}

static void facedetect_platform_release(struct device *device)
{
	/* This is called when the reference count goes to zero */
}

static int __init facedetect_probe(struct device *device)
{
	facedetect_dev = device;
	return 0;
}

static int facedetect_remove(struct device *device)
{
	return 0;
}

/*
 * Global variable of type file_operations containing function
 * pointers of file operations
 */
static const struct file_operations facedetect_fops = {
	.owner = THIS_MODULE,
	.open = facedetect_open,
	.release = facedetect_release,
	.ioctl = facedetect_ioctl,
};

/* global variable of type cdev to register driver to the kernel */
static struct cdev cdev;

/* global variable which keeps major and minor number of the driver in it */
static dev_t dev;

static struct device_driver facedetect_driver = {
	.name = "dm365_facedetect",
	.bus = &platform_bus_type,
	.probe = facedetect_probe,
	.remove = facedetect_remove,
};

static struct platform_device facedetect_pt_device = {
	.name = "dm365_facedetect",
	.id = 0,
	.dev = {
		.release = facedetect_platform_release,
		}
};

int __init facedetect_init(void)
{
	int result;

	/*
	 * Register the driver in the kernel
	 * Dynmically get the major number for the driver using
	 * alloc_chrdev_region function
	 */
	result = alloc_chrdev_region(&dev, 0, 1, DRIVERNAME);

	/* if it fails return error */
	if (result < 0) {
		dev_dbg(facedetect_dev,
			"Error registering facedetect character device\n");
		return -ENODEV;
	}

	printk(KERN_INFO "facedetect major#: %d, minor# %d\n", MAJOR(dev),
		MINOR(dev));

	/* initialize cdev with file operations */
	cdev_init(&cdev, &facedetect_fops);

	cdev.owner = THIS_MODULE;
	cdev.ops = &facedetect_fops;

	/* add cdev to the kernel */
	result = cdev_add(&cdev, dev, 1);

	if (result) {
		unregister_chrdev_region(dev, 1);
		dev_dbg(facedetect_dev,
			"Error adding facedetect char device: error no:%d\n",
			result);
		return -EINVAL;
	}

	/* register driver as a platform driver */
	if (driver_register(&facedetect_driver) != 0) {
		unregister_chrdev_region(dev, 1);
		cdev_del(&cdev);
		return -EINVAL;
	}

	/* Register the drive as a platform device */
	if (platform_device_register(&facedetect_pt_device) != 0) {
		driver_unregister(&facedetect_driver);
		unregister_chrdev_region(dev, 1);
		cdev_del(&cdev);
		return -EINVAL;
	}

	facedetect_class = class_create(THIS_MODULE, "dm365_facedetect");
	if (!facedetect_class) {
		dev_dbg(facedetect_dev,
			"Error creating facedetect device class\n");
		driver_unregister(&facedetect_driver);
		platform_device_unregister(&facedetect_pt_device);
		unregister_chrdev_region(dev, 1);
		unregister_chrdev(MAJOR(dev), DRIVERNAME);
		cdev_del(&cdev);
		return -EINVAL;
	}
	/* register simple device class */
	class_device_create(facedetect_class, NULL, dev, NULL,
			    "dm365_facedetect");

	if (facedetect_init_hw_setup() != 0) {
		dev_dbg(facedetect_dev,
			"Error initializing facedetect hardware\n");
		class_device_destroy(facedetect_class, dev);
		driver_unregister(&facedetect_driver);
		platform_device_unregister(&facedetect_pt_device);
		unregister_chrdev_region(dev, 1);
		unregister_chrdev(MAJOR(dev), DRIVERNAME);
		cdev_del(&cdev);
		return -EINVAL;
	}

	facedetect_init_interrupt();

	printk(KERN_INFO "facedetect driver registered\n");

	return 0;
}

void __exit facedetect_cleanup(void)
{
	/* remove major number allocated to this driver */
	unregister_chrdev_region(dev, 1);

	/* Remove platform driver */
	driver_unregister(&facedetect_driver);

	/* remove simple class device */
	class_device_destroy(facedetect_class, dev);

	/* destroy simple class */
	class_destroy(facedetect_class);

	/* remove platform device */
	platform_device_unregister(&facedetect_pt_device);

	cdev_del(&cdev);

	/* unregistering the driver from the kernel */
	unregister_chrdev(MAJOR(dev), DRIVERNAME);
}

module_init(facedetect_init);
module_exit(facedetect_cleanup);
MODULE_LICENSE("GPL");
