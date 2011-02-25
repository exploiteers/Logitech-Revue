/* *
 * Copyright (C) 2009 Logitech Inc
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/ioport.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/platform_device.h>
#include <linux/major.h>
#include <asm/arch/dm365_logi.h>

MODULE_LICENSE("GPL");

// global driver registration data
static struct cdev c_dev;
static struct class *logi_class;
static dev_t dev;
struct device *logidev;

// DM365 base address
void __iomem	*DM365base;
// tables
void	*table[LOGI_MAX_TABLES];

// open
static int
logi_open(struct inode *inode, struct file *filp)
{
	return 0;
}

// close
static int
logi_close(struct inode *inode, struct file *filp)
{
	return 0;
}

// release
static void
logi_release(struct device *device)
{
}

// probe
static int
logi_probe(struct device *device)
{
	return 0;
}

// remove
static int
logi_remove(struct device *device)
{
	return 0;
}

// ioctl
static int
logi_ioctl(struct inode *inode, struct file *filep,
		    unsigned int cmd, unsigned long arg)
{
	int		err = 0, result = 0;

    if(_IOC_TYPE(cmd) != LOGI_MAGIC_NO) return -ENOTTY;
	if(_IOC_NR(cmd) > LOGI_MAX_NR) return -ENOTTY;

    if(_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
	if(err) return -EFAULT;

	switch(cmd){
		case LOGI_IO_REG:{
			struct LOGI_IO_REGISTER	io;

			// get ioctl struct
			if(copy_from_user(&io, (struct LOGI_IO_REGISTER *)arg, sizeof(struct LOGI_IO_REGISTER))){
				return -EFAULT;
			}
			// validate register
			switch(io.size){
				case LOGI_BYTE: if(io.reg >= DM365_PHYS_SIZE) return -EFAULT; break;
				case LOGI_WORD: if(io.reg >= (DM365_PHYS_SIZE - 1)) return -EFAULT; break;
				case LOGI_LONG: if(io.reg >= (DM365_PHYS_SIZE - 3)) return -EFAULT; break;
				default: return -EFAULT;
			}
			// perform the operation
			switch(io.rd){
				// read
				case true:{
					switch(io.size){
						case LOGI_BYTE:{
							io.value = ioread8(DM365base + io.reg);
							break;
						}
						case LOGI_WORD:{
							io.value = ioread16(DM365base + io.reg);
							break;
						}
						case LOGI_LONG:{
							io.value = ioread32(DM365base + io.reg);
							break;
						}
						default:{
							return -EFAULT;
						}
					}
					if(copy_to_user((struct LOGI_IO_REGISTER *)arg, &io, sizeof(struct LOGI_IO_REGISTER))){
						return -EFAULT;
					}
					break;
				}
				// write
				case false:{
					switch(io.size){
						case LOGI_BYTE:{
							iowrite8(io.value, DM365base + io.reg);
							break;
						}
						case LOGI_WORD:{
							iowrite16(io.value, DM365base + io.reg);
							break;
						}
						case LOGI_LONG:{
							iowrite32(io.value, DM365base + io.reg);
							break;
						}
						default:{
							return -EFAULT;
						}
					}
				}
			}
			break;
		}
		case LOGI_SET_TABLE:{
			int					i;
			struct LOGI_TABLE	tbl;
			void				*data;	// actual table data pointer - globals hold the allocated pointers
			void				*tmp;	// used for storing the global allocated pointer
			unsigned long		phys;	// actual physical address of *data

			// get ioctl struct
			if(copy_from_user(&tbl, (struct LOGI_TABLE *)arg, sizeof(struct LOGI_TABLE))){
				return -EFAULT;
			}
			// validate register
			if(tbl.reg >= ((DM365_PHYS_SIZE - 3) - 4)){
				return -EFAULT;
			}
			// check for available table
			for(i=0; i<LOGI_MAX_TABLES; i++){
				if(!table[i]){
					break;
				}
			}
			if(i >= LOGI_MAX_TABLES){
				return -EFAULT;
			}
			// allocate kernel memory for table data
			if(!(tmp = kmalloc(tbl.size + 4, GFP_USER))){
				return -EFAULT;
			}
			// get physical address
			phys = __pa(tmp);
			// adjust physical address
			phys += 4; phys &= 0xFFFFFFFC;
			// get virtual address of physical
			data = __va(phys);
			// get table data
			if(copy_from_user(data, tbl._data, tbl.size)){
				kfree(tmp);
				return -EFAULT;
			}
			// save to table
			table[i] = tmp;
			tbl.tindex = i;
			// write the physical address
			iowrite32(phys >> 16, DM365base + tbl.reg);
			iowrite32(phys & 0x0000FFFF, DM365base + tbl.reg + 4);
			// return table index
			if(copy_to_user((struct LOGI_TABLE *)arg, &tbl, sizeof(struct LOGI_TABLE))){
				return -EFAULT;
			}
			break;
		}
		case LOGI_DEL_TABLE:{
			struct LOGI_TABLE	tbl;

			// get ioctl struct
			if(copy_from_user(&tbl, (struct LOGI_TABLE *)arg, sizeof(struct LOGI_TABLE))){
				return -EFAULT;
			}
			// check for allocated table pointer
			if((tbl.tindex >= LOGI_MAX_TABLES) || (!table[tbl.tindex])){
				return -EFAULT;
			}
			// clear the addresses
			iowrite32(0, DM365base + tbl.reg);
			iowrite32(0, DM365base + tbl.reg + 4);
			// free kernel memory
			kfree(table[tbl.tindex]);
			// NULL indicates not allocated
			table[tbl.tindex] = NULL;
			break;
		}
		default:{
			result = -EINVAL;
			break;
		}
	}

	return result;
}

static struct file_operations logi_fops = {
	.owner =	THIS_MODULE,
	.open =		logi_open,
	.ioctl =	logi_ioctl,
	.release =	logi_close
};

static struct platform_device logidevice = {
	.name =	"dm365_logi",
	.id =	2,
	.dev =	{
		.release = logi_release,
	}
};

static struct device_driver logi_driver = {
	.name =		"dm365_logi",
	.bus =		&platform_bus_type,
	.probe =	logi_probe,
	.remove =	logi_remove,
};

// __init logi_init
//
// driver initialization
int
__init logi_init(void)
{
	int		i;
	struct resource	*rc;

	// register the driver - get major number
	if(alloc_chrdev_region(&dev, 0, 1, DRIVERNAME) < 0){
		printk(KERN_ERR "dm365_logi: alloc_chrdev_region error");
		return -ENODEV;
	}
	// save to dmesg
	printk(KERN_INFO "dm365_logi major#: %d\n", MAJOR(dev));
	// initialize character device
	cdev_init(&c_dev, &logi_fops);
	c_dev.owner = THIS_MODULE;
	c_dev.ops = &logi_fops;
	if(cdev_add(&c_dev, dev, 1)){
		printk(KERN_ERR "dm365_logi: cdev_add error");
		unregister_chrdev_region(dev, 1);
		return -ENODEV;
	}
	// register as a platform driver
	if(driver_register(&logi_driver)){
		unregister_chrdev_region(dev, 1);
		cdev_del(&c_dev);
		return -EINVAL;
	}
	// register as a platform device
	if(platform_device_register(&logidevice)){
		driver_unregister(&logi_driver);
		unregister_chrdev_region(dev, 1);
		cdev_del(&c_dev);
		return -EINVAL;
	}
	// create class
	if(!(logi_class = class_create(THIS_MODULE, CLASS_NAME))){
		printk(KERN_ERR "dm365_logi: class_create error\n");
		driver_unregister(&logi_driver);
		platform_device_unregister(&logidevice);
		unregister_chrdev_region(dev, 1);
		unregister_chrdev(MAJOR(dev), DRIVERNAME);
		cdev_del(&c_dev);
		return -EINVAL;
	}
	// register device class
	class_device_create(logi_class, NULL, dev, NULL, CLASS_NAME);
	// clear tables
	for(i=0; i<LOGI_MAX_TABLES; i++){
		table[i] = NULL;
	}
	// allocate DM365 registers
	if(!(rc = request_mem_region(DM365_PHYS_ADDR, DM365_PHYS_SIZE, CLASS_NAME))){
		printk(KERN_ERR "dm365_logi: request_mem_region error\n");
		class_device_destroy(logi_class, dev);
		class_destroy(logi_class);
		platform_device_unregister(&logidevice);
		driver_unregister(&logi_driver);
		unregister_chrdev_region(dev, 1);
		unregister_chrdev(MAJOR(dev), DRIVERNAME);
		cdev_del(&c_dev);
		return -EINVAL;
	}
	// map the DM365 registers
	if(!(DM365base = ioremap(DM365_PHYS_ADDR, DM365_PHYS_SIZE))){
		printk(KERN_ERR "dm365_logi: ioremap error\n");
		class_device_destroy(logi_class, dev);
		class_destroy(logi_class);
		platform_device_unregister(&logidevice);
		driver_unregister(&logi_driver);
		unregister_chrdev_region(dev, 1);
		unregister_chrdev(MAJOR(dev), DRIVERNAME);
		cdev_del(&c_dev);
		return -EINVAL;
	}
	printk(KERN_INFO "dm365_logi driver initialized\n");

	return 0;
}

// __exit logi_exit
//
// driver unload
void
__exit logi_exit(void)
{
	int		i;

	// free up tables
	for(i=0; i<LOGI_MAX_TABLES; i++){
		if(table[i]){
			kfree(table[i]);
			table[i] = NULL;
		}
	}
	// unmap
	iounmap(DM365base);
	// unregister 
	class_device_destroy(logi_class, dev);
	class_destroy(logi_class);
	platform_device_unregister(&logidevice);
	driver_unregister(&logi_driver);
	unregister_chrdev_region(dev, 1);
	unregister_chrdev(MAJOR(dev), DRIVERNAME);
	cdev_del(&c_dev);
	printk(KERN_INFO "dm365_logi driver unloaded\n");
}

module_init(logi_init)
module_exit(logi_exit)
MODULE_LICENSE("GPL");

