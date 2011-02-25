/*
 * Copyright (C) 2008 MontaVista Software Inc.
 * Copyright (C) 2008 Texas Instruments Inc
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option)any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

/*******************************************************************************
 * FILE PURPOSE:    LCD Module Driver Source
 *******************************************************************************
 * FILE DESCRIPTION:     Source code for Linux LCD Driver
 *
 * REVISION HISTORY:
 *
 * Date           Description                               Author
 *-----------------------------------------------------------------------------
 * 27 Aug 2003    Initial Creation                          Sharath Kumar
 *
 * 16 Dec 2003    Updates for 5.7                           Sharath Kumar
 *
 * (C) Copyright 2003, Texas Instruments, Inc
 ******************************************************************************/
#include <linux/config.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/ioport.h>
#include <linux/fcntl.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/proc_fs.h>
#include <linux/mm.h>
#include <linux/version.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <asm/system.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include "lcd.h"

#include <linux/moduleparam.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <asm/arch/da8xx_lcdc.h>
#include "lidd_hal.h"

#define    LCD_VERSION                 "0.2"
#define    MAX_LCD_SIZE                 ((MAX_ROWS)*(MAX_COLS))

/* LCD seek origin positions*/
#define  SEEK_CUR    1
#define  SEEK_END    2
#define  SEEK_SET    0

#define DA8XX_LCD_NAME "lcd_da8xx"

struct lcd_dev {
	void *hal_handle;
	char devname[10];
	struct semaphore sem;
	int rows;
	int columns;
	struct resource *lcd_res;
	struct clk *clk;
};

static struct lcd_dev *lcd_dev;
static int rows = DEFAULT_ROWS;
static int columns = DEFAULT_COLS;

static ssize_t tilcd_read(struct file *file, char *buf,
			  size_t count, loff_t *ppos)
{
	struct lcd_dev *lcd_dev = file->private_data;
	int ret = -1, max = lcd_dev->columns * lcd_dev->rows;
	char temp_buf[MAX_LCD_SIZE];
	int x_pos, y_pos;

	if (*ppos >= max)
		return 0;

	memset(temp_buf, 0, max);

	/* Limit the count to max LCD size if it is greater than that */
	count = (count > max) ? max : count;

	if (down_interruptible(&lcd_dev->sem))
		return -ERESTARTSYS;

	/* syncing file offset and cursor position */
	x_pos = ((long)*ppos) / lcd_dev->columns;
	y_pos = ((long)*ppos) % lcd_dev->columns;
	ti_lidd_hal_ioctl(lcd_dev->hal_handle, LIDD_GOTO_XY, (y_pos << 8) |
			  x_pos);

	ret = ti_lidd_hal_read(lcd_dev->hal_handle, temp_buf, count);

	if (ret > 0) {
		if (copy_to_user(buf, temp_buf, ret))
			ret = -EFAULT;
	}

	if (ret >= 0) {
		*ppos += ret;

		/* syncing file offset and cursor position */
		x_pos = ((long)*ppos) / lcd_dev->columns;
		y_pos = ((long)*ppos) % lcd_dev->columns;
		ti_lidd_hal_ioctl(lcd_dev->hal_handle, LIDD_GOTO_XY,
				  (y_pos << 8) | x_pos);
	}

	up(&lcd_dev->sem);
	return ret;
}

static ssize_t tilcd_write(struct file *file, const char *buf,
			   size_t count, loff_t *ppos)
{
	struct lcd_dev *lcd_dev = file->private_data;
	int ret = -1, max = lcd_dev->columns * lcd_dev->rows;
	char temp_buf[MAX_LCD_SIZE];
	int x_pos, y_pos;

	if (down_interruptible(&lcd_dev->sem))
		return -ERESTARTSYS;

	/* Limit the count to max LCD size if it is greater than that */
	count = (count > max) ? max : count;

	/* syncing file offset and cursor position */
	x_pos = ((long)*ppos) / lcd_dev->columns;
	y_pos = ((long)*ppos) % lcd_dev->columns;
	ti_lidd_hal_ioctl(lcd_dev->hal_handle, LIDD_GOTO_XY, (y_pos << 8) |
			  x_pos);

	if (copy_from_user(temp_buf, buf, count))
		ret = -EFAULT;
	else
		ret = ti_lidd_hal_write(lcd_dev->hal_handle, temp_buf, count);

	if (ret >= 0) {
		*ppos += ret;

		/* syncing file offset and cursor position */
		x_pos = (((long)*ppos) / lcd_dev->columns);
		y_pos = ((long)*ppos) % lcd_dev->columns;
		ti_lidd_hal_ioctl(lcd_dev->hal_handle, LIDD_GOTO_XY,
				  (y_pos << 8) | x_pos);
	}

	up(&lcd_dev->sem);
	return ret;
}

static int tilcd_ioctl(struct inode *inode, struct file *file,
		       unsigned int cmd, unsigned long arg)
{
	struct lcd_dev *lcd_dev = file->private_data;
	int ret = 0;
	struct lcd_pos_arg lcd_pos;
	unsigned long hal_arg = arg;
	unsigned int read_val;

	if (down_interruptible(&lcd_dev->sem))
		return -ERESTARTSYS;

	if (cmd == LIDD_GOTO_XY) {
		if (copy_from_user
		    (&lcd_pos, (char *)arg, sizeof(struct lcd_pos_arg)))
			ret = -EFAULT;
		/* syncing file offset and cursor position */
		file->f_pos = lcd_pos.row * lcd_dev->columns + lcd_pos.column;
		hal_arg = lcd_pos.row | (lcd_pos.column << 8);
	}

	/* The commands below require something to be written to user buffer */
	if (cmd == LIDD_RD_CMD || cmd == LIDD_RD_CHAR)
		hal_arg = (unsigned long)&read_val;

	ret = ti_lidd_hal_ioctl(lcd_dev->hal_handle, cmd, hal_arg);

	switch (cmd) {
	case LIDD_RD_CMD:
	case LIDD_RD_CHAR:
		if (copy_to_user((char *)arg, &read_val, sizeof(int)))
			ret = -EFAULT;
		break;

	default:
		break;
	}

	up(&lcd_dev->sem);
	return ret;
}

static int tilcd_open(struct inode *inode, struct file *file)
{
	/* Set the private data. cannot be done in init through cdev */
	file->private_data = lcd_dev;

	ti_lidd_hal_open(lcd_dev->hal_handle);

	return 0;
}

static int tilcd_release(struct inode *inode, struct file *file)
{
	ti_lidd_hal_close(lcd_dev->hal_handle);
	return 0;
}

static loff_t tilcd_lseek(struct file *file, loff_t offset, int orig)
{

	struct lcd_dev *lcd_dev = file->private_data;
	int max = lcd_dev->columns * lcd_dev->rows;
	int x_pos, y_pos;

	if (down_interruptible(&lcd_dev->sem))
		return -ERESTARTSYS;

	switch (orig) {
	case SEEK_END:
		offset += max;
		break;

	case SEEK_CUR:
		offset += file->f_pos;
	case SEEK_SET:
		break;

	default:
		up(&lcd_dev->sem);
		return -EINVAL;
	}

	offset = (long)offset % (long)max;

	file->f_pos = offset;

	x_pos = (((long)offset) / lcd_dev->columns);
	y_pos = ((long)offset) % lcd_dev->columns;
	ti_lidd_hal_ioctl(lcd_dev->hal_handle, LIDD_GOTO_XY, (y_pos << 8) |
			  x_pos);

	up(&lcd_dev->sem);
	return file->f_pos;
}

struct file_operations tilcd_fops = {
	.owner = THIS_MODULE,
	.read = tilcd_read,
	.write = tilcd_write,
	.ioctl = tilcd_ioctl,
	.open = tilcd_open,
	.release = tilcd_release,
	.llseek = tilcd_lseek
};

static struct miscdevice da8xx_lcd_miscdev = {
	.minor = 0,
	.name = DA8XX_LCD_NAME,
	.fops = &tilcd_fops,
};

static int __devinit da8xx_lcd_probe(struct platform_device *device)
{
	struct ti_lidd_info lcd_info;
	struct resource *lcdc_regs;
	struct da8xx_lcdc_platform_data *lcd_pdata = device->dev.platform_data;
	int ret = 0;

	if (lcd_pdata == NULL) {
		dev_err(&device->dev, "Can not get platform data\n");
		return -ENOENT;
	}

	lcd_dev = kzalloc(sizeof(struct lcd_dev), GFP_KERNEL);
	if (!lcd_dev) {
		dev_err(&device->dev, "Can't allocate memory for device\n");
		return -ENOMEM;
	}

	lcdc_regs = platform_get_resource(device, IORESOURCE_MEM, 0);
	if (!lcdc_regs) {
		dev_err(&device->dev,
			"Can not get memory resource for LCD controller\n");
		ret = -ENOENT;
		goto err_get_resource;
	}

	lcd_dev->lcd_res = request_mem_region(lcdc_regs->start,
					      lcdc_regs->end -
					      lcdc_regs->start + 1,
					      device->name);
	if (lcd_dev->lcd_res == NULL) {
		dev_err(&device->dev, "Request memory for registers failed\n");
		ret = -ENOENT;
		goto err_get_resource;
	}

	lcd_info.base_addr = IO_ADDRESS(lcdc_regs->start);
	lcd_dev->clk = clk_get(&device->dev, lcd_pdata->lcdc_clk_name);
	if (IS_ERR(lcd_dev->clk)) {
		printk(KERN_ERR "Can not get LCD clock\n");
		ret = -ENOENT;
		goto err_clk_get;
	}
	clk_enable(lcd_dev->clk);
	lcd_dev->rows = lcd_info.disp_row = rows;
	lcd_dev->columns = lcd_info.disp_col = columns;
	lcd_info.line_wrap = 1;
	lcd_info.cursor_blink = 1;
	lcd_info.cursor_show = 1;
	lcd_info.lcd_type = 4;	/* HITACHI */
	lcd_info.num_lcd = 1;

	lcd_dev->hal_handle = ti_lidd_hal_init(&lcd_info);
	if (!lcd_dev->hal_handle) {
		printk(KERN_ERR "LCD: hal not initialized\n");
		ret = -EIO;
		goto err_hal_init;
	}

	sprintf(lcd_dev->devname, DA8XX_LCD_NAME);

	ret = misc_register(&da8xx_lcd_miscdev);
	if (ret < 0) {
		printk(KERN_ERR "LCD: misc_register failed\n");
		ret = -EIO;
		goto err_misc_register;
	}

	sema_init(&lcd_dev->sem, 1);

	platform_set_drvdata(device, lcd_dev);

	return 0;

err_misc_register:
	ti_lidd_hal_cleanup(lcd_dev->hal_handle);
err_hal_init:
	clk_disable(lcd_dev->clk);
	clk_put(lcd_dev->clk);
err_clk_get:
	release_resource(lcd_dev->lcd_res);
err_get_resource:
	kfree(lcd_dev);
	return ret;
}

static int da8xx_lcd_remove(struct platform_device *dev)
{
	misc_deregister(&da8xx_lcd_miscdev);
	ti_lidd_hal_cleanup(lcd_dev->hal_handle);
	clk_disable(lcd_dev->clk);
	clk_put(lcd_dev->clk);
	release_resource(lcd_dev->lcd_res);
	kfree(lcd_dev);
	return 0;
}

static struct platform_driver da8xx_lcd_driver = {
	.probe	= da8xx_lcd_probe,
	.remove = da8xx_lcd_remove,
	.driver = {
		   .name = "da8xx_lcdc",
		   .owner = THIS_MODULE,
	},
};

static int __init da8xx_lcd_init(void)
{
	return platform_driver_register(&da8xx_lcd_driver);
}

static void __exit da8xx_lcd_cleanup(void)
{
	platform_driver_unregister(&da8xx_lcd_driver);
}

module_init(da8xx_lcd_init);
module_exit(da8xx_lcd_cleanup);

module_param(rows, int, 0);
module_param(columns, int, 0);

MODULE_DESCRIPTION("Driver for TI CHARACTER LCD");
MODULE_AUTHOR("Maintainer: Sharath Kumar <krs@ti.com>");
MODULE_LICENSE("GPL");
MODULE_VERSION(LCD_VERSION);
