/*
 *
 * TI DaVinciHD PCI Module file
 *
 * Copyright (C) 2007 Texas Instruments
 *
 * ----------------------------------------------------------------------------
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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * ----------------------------------------------------------------------------
 *
 */
#include <linux/config.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/cdev.h>
#include <linux/spinlock.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <asm/arch/pcimodule.h>
#include <asm/arch/pcihal.h>

/** ============================================================================
 *  @const  async_queue
 *
 *  @desc   Structure for sending asynchronous messages to user applications
 *  ============================================================================
 */
static struct fasync_struct *async_queue;

/** ============================================================================
 *  @const  pcimodule_sh_int
 *
 *  @desc   Argument required for passing to shared interrupt handlers
 *  ============================================================================
 */
static int pcimodule_sh_int;

/** ============================================================================
 *  @name   pcimodule_dev_id
 *
 *  @desc   Device ID of driver.
 *  ============================================================================
 */
static dev_t pcimodule_dev_id;

/** ============================================================================
 *  @name   pcimodule_cdev
 *
 *  @desc   Class device object for sys/class/ entry
 *  ============================================================================
 */
static struct cdev pcimodule_cdev;

/** ============================================================================
 *  @name   pcimodule_class
 *
 *  @desc   Class entry for sys file system
 *  ============================================================================
 */
static struct class *pcimodule_class;

/** ============================================================================
 *  @name   sync_lock
 *
 *  @desc   Spin lock structure to provide protection from interrupts,
 *          tasks and tasklets.
 *  ============================================================================
 */
static spinlock_t sync_lock;

/** ============================================================================
 *  @func   drv_fasync
 *
 *  @desc   Function invoked by OS whenever F_SETFL is called on fcntl.
 *
 *  ============================================================================
 */
int drv_fasync(int fd, struct file *filp, int mode)
{
	/* Hook up the our file descriptor to the signal */
	return fasync_helper(fd, filp, mode, &async_queue);
}

/** ============================================================================
 *  @name   drv_release
 *
 *  @desc   Linux specific function to close the driver.
 *  ============================================================================
 */
static int drv_release(struct inode *inode, struct file *filp)
{
	/* Clean up the async notification method */
	return drv_fasync(-1, filp, 0);
}

/** ============================================================================
 *  @name   drv_ioctl
 *
 *  @desc   Function to invoke the APIs through ioctl.
 *  ============================================================================
 */
static int drv_ioctl(struct inode *inode, struct file *filp,
		       unsigned int cmd, unsigned long args)
{
	unsigned int irqFlags;
	int ret = 0;

	switch (cmd) {
	case PCIMODULE_CMD_GENNOTIFY:
	{
		enum pcimodule_intr_num intr_num
			= *(enum pcimodule_intr_num *)args;
		unsigned int mask = 0;

		switch (intr_num) {
		case PCIMODULE_SOFT_INT0:
			mask = PCI_SOFTINT0_MASK;
			break;
		case PCIMODULE_SOFT_INT1:
			mask = PCI_SOFTINT1_MASK;
			break;
		case PCIMODULE_SOFT_INT2:
			mask = PCI_SOFTINT2_MASK;
			break;
		case PCIMODULE_SOFT_INT3:
			mask = PCI_SOFTINT3_MASK;
			break;
		default:
			printk(KERN_DEBUG"Bad interrupt number\n");
			ret = -1;
			goto ioctl_out;
		}	/* switch(intr_num) */

		spin_lock_irqsave(&sync_lock, irqFlags);
		REG32(PCI_REG_HINTSET)   |= mask;
		REG32(PCI_REG_INTSTATUS) |= mask;
		spin_unlock_irqrestore(&sync_lock, irqFlags);
		break;
	}

	case PCIMODULE_CMD_GETBARLOCN:
	{
		u32 reg_data;
		struct pcimodule_bar_mapping *bar_mapping
			= (struct pcimodule_bar_mapping *)args;
		if (!bar_mapping) {
			printk(KERN_DEBUG "NULL structure passed\n");
			ret = -1;
			goto ioctl_out;
		}

		if ((PCIMODULE_BAR_0 > bar_mapping->bar_num)
			|| (PCIMODULE_BAR_5 < bar_mapping->bar_num)) {
			printk(KERN_DEBUG "Bad BAR number\n");
			bar_mapping->mapping = 0;
			ret = -1;
			goto ioctl_out;
		} else {
			reg_data = REG32(PCI_REG_BAR0TRL +
					(4 * bar_mapping->bar_num));
			if (copy_to_user(&bar_mapping->mapping, &reg_data,
					sizeof(reg_data)))
				ret = -1;
		}
		break;
	}

	default:
		ret = -1;
	}	/* switch(cmd) */

ioctl_out:
	return ret;
}

/** ============================================================================
 *  @name   drv_mmap
 *
 *  @desc   Mmap function implementation.
 *  ============================================================================
 */
static int drv_mmap(struct file *filp, struct vm_area_struct *vma)
{
	int ret = 0;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	if (remap_pfn_range(vma,
			vma->vm_start,
			vma->vm_pgoff,
			vma->vm_end - vma->vm_start,
			vma->vm_page_prot)) {
		ret = -EAGAIN ;
	}
	return ret;
}

/** ============================================================================
 *  @func   pcimodule_pci_callback
 *
 *  @desc   Function invoked by OS whenever an interrupt occurs.
 *
 *  @modif  None
 *  ============================================================================
 */
static irqreturn_t pcimodule_pci_callback(int irq, void *arg,
					  struct pt_regs *flags)
{
	/* Clear interrupt from HOST */
	REG32(PCI_REG_INTSTATUS) |= (PCI_SOFTINT0_MASK
		| PCI_SOFTINT1_MASK | PCI_SOFTINT2_MASK | PCI_SOFTINT3_MASK);

	/* Notify all the waiting applications using SIGIO */
	kill_fasync(&async_queue, SIGIO, POLL_IN);

	return IRQ_HANDLED;
}

/** ============================================================================
 *  @name   driver_ops
 *
 *  @desc   Function to invoke the APIs through ioctl.
 *  ============================================================================
 */
static struct file_operations driver_ops = {
	.release = drv_release,
	.ioctl =   drv_ioctl,
	.mmap =    drv_mmap,
	.fasync =  drv_fasync,
};

/* =============================================================================
 *  @func   init_module
 *
 *  @desc   This function initializes the PCI daemon driver.
 *
 *  @modif  None.
 *  ============================================================================
 */
static int __init pcimodule_init(void)
{
	/* ---------------------------------------------------------------------
	 *  1.  Register a character driver to send data to this driver
	 * ---------------------------------------------------------------------
	 */
	if (0 != alloc_chrdev_region(&pcimodule_dev_id, 0, 1, PCIMODULE_NAME))
		goto init_out;

	/* ---------------------------------------------------------------------
	 *  2.  Create the node for access through the application
	 * ---------------------------------------------------------------------
	 */
	cdev_init(&pcimodule_cdev, &driver_ops);
	pcimodule_cdev.owner = THIS_MODULE;
	pcimodule_cdev.ops = &driver_ops;
	if (0 != cdev_add(&pcimodule_cdev, pcimodule_dev_id, 1))
		goto init_handle_node_create_fail;

	/* ---------------------------------------------------------------------
	 *  3.  Creation of a class structure for pseudo file system access
	 * ---------------------------------------------------------------------
	 */
	pcimodule_class = class_create(THIS_MODULE, PCIMODULE_NAME);
	if (!pcimodule_class)
		goto init_handle_class_create_fail;

	class_device_create(pcimodule_class, NULL, pcimodule_dev_id,
			    NULL, PCIMODULE_NAME);

	/* ---------------------------------------------------------------------
	 *  4.  Attached IRQ handler for PCI interrupt
	 * ---------------------------------------------------------------------
	 */
	if (0 != request_irq(PCIMODULE_PCI_IRQ,
			pcimodule_pci_callback, SA_SHIRQ,
			"PCIMODULE", &pcimodule_sh_int))
		goto init_handle_request_irq_fail;

	/* ---------------------------------------------------------------------
	 *  5.  Initialize the spinlock
	 * ---------------------------------------------------------------------
	 */
	spin_lock_init(&sync_lock);

	return 0;

init_handle_request_irq_fail:
	class_destroy(pcimodule_class);
init_handle_class_create_fail:
	cdev_del(&pcimodule_cdev);
init_handle_node_create_fail:
	unregister_chrdev_region(pcimodule_dev_id, 1);
init_out:
	return -1;
}

/* =============================================================================
 *  @func   cleanup_module
 *
 *  @desc   This function finalizes the PCI daemon driver.
 *
 *  @modif  None.
 *  ============================================================================
 */
static void __exit pcimodule_cleanup(void)
{
	/* Free the interrupt handler */
	free_irq(PCIMODULE_PCI_IRQ, &pcimodule_sh_int);

	/* Destroy the class entry */
	class_destroy(pcimodule_class);

	/* Class Decice clean-up */
	cdev_del(&pcimodule_cdev);

	/* unregister the driver with kernel */
	unregister_chrdev_region(pcimodule_dev_id, 1);
}

module_init(pcimodule_init);
module_exit(pcimodule_cleanup);
MODULE_LICENSE("GPL");
