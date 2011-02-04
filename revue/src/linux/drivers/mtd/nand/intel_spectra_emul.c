/*
 * Intel Spectra IOCTL emulator.
 *
 * Copyright (c) 2010 Google, Inc
 * Eugene Surovegin <surovegin@google.com> or <ebs@ebshome.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/uaccess.h>
#include <linux/sched.h>

#define DRV_NAME        	"spectra"
#define DRV_VERSION     	"0.1"
#define DRV_DESC        	"Intel Spectra emulation driver"

MODULE_DESCRIPTION(DRV_DESC);
MODULE_VERSION(DRV_VERSION);
MODULE_AUTHOR("Eugene Surovegin <surovegin@google.com> or <ebs@ebshome.net>");
MODULE_LICENSE("GPL");

#define DBG_LEVEL 		0

#if DBG_LEVEL > 0
#  define DBG(f,x...)		printk(KERN_ERR DRV_NAME ": " f, ##x)
#else
#  define DBG(f,x...)		((void)0)
#endif

#define CEFDK_CFG_PARTITION	CONFIG_MTD_NAND_INTEL_SPECTRA_EMUL_PARTITION
/* Start block is fixed */
#define CEFDK_CFG_PARTITION_OFFSET	48

/* IOCTL interface */
struct spectra_device_info {
	__u16 __rsrv0;
	__u32 __rsrv1;
	__u16 __rsrv2[3];

	__u16 blocks;
	__u16 blockpages;
	__u16 pagesize;		/* including OOB */
	__u16 writesize;
	__u16 oobsize;

	__u16 __rsrv4[2];

	__u32 blocksize;	/* including OOB */
	__u32 erasesize;

	__u16 __rsrv5;
	__u8 __rsrv6;
	__u16 __rsrv7[12];
};

#define SPECTRA_IO_GET_INFO		0x8813

struct spectra_ioctl_req {
	__u32 page_count;
	__u32 addr;
	__u32 block;
	__u16 page;
};

#define SPECTRA_IO_READ_PAGES		0x8810
#define SPECTRA_IO_CHECK_BAD_BLOCK	0x8814
#define SPECTRA_IO_ERASE		0x8812

struct spectra_device {
	struct mtd_info *mtd;
	struct spectra_device_info device_info;
	char buf[NAND_MAX_PAGESIZE];
};

static int spectra_open(struct inode *inode, struct file *file)
{
	struct spectra_device *dev;
	struct mtd_info *mtd;
	int err;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (unlikely(!dev))
		return -ENOMEM;

	dev->mtd = mtd = get_mtd_device_nm(CEFDK_CFG_PARTITION);
	if (unlikely(IS_ERR(mtd))) {
		printk(KERN_ERR DRV_NAME
		       ": mtd partition '" CEFDK_CFG_PARTITION "' not found\n");
		err = -ENXIO;
		goto out_free;
	}

	if (unlikely(mtd->type != MTD_NANDFLASH)) {
		printk(KERN_ERR DRV_NAME ": not a NAND partition\n");
		err = -ENXIO;
		goto out_put;
	}

	BUG_ON(mtd->writesize > sizeof(dev->buf));

	dev->device_info.writesize = mtd->writesize;
	dev->device_info.oobsize = mtd->oobsize;
	dev->device_info.pagesize = mtd->writesize + mtd->oobsize;

	dev->device_info.erasesize = mtd->erasesize;
	dev->device_info.blockpages = mtd->erasesize / mtd->writesize;
	dev->device_info.blocksize =
	    dev->device_info.pagesize * dev->device_info.blockpages;

	dev->device_info.blocks = CEFDK_CFG_PARTITION_OFFSET +
	    mtd->size / mtd->erasesize;

	file->private_data = dev;
	return 0;

      out_put:
	put_mtd_device(dev->mtd);
      out_free:
	kfree(dev);
	return err;
}

static int spectra_release(struct inode *inode, struct file *file)
{
	struct spectra_device *dev = file->private_data;
	put_mtd_device(dev->mtd);
	kfree(dev);
	return 0;
}

static int spectra_read_pages(struct spectra_device *dev,
			      const struct spectra_ioctl_req *req)
{
	void __user *dst = (void __force __user *)req->addr;
	struct mtd_info *mtd = dev->mtd;
	loff_t off;
	int i;

	DBG("read_pages(%d:%d, %d) -> 0x%08x\n",
	    req->block, req->page, req->page_count, req->addr);

	if (unlikely(req->block < CEFDK_CFG_PARTITION_OFFSET ||
		     req->block >= dev->device_info.blocks))
		return -EINVAL;

	off = (req->block - CEFDK_CFG_PARTITION_OFFSET) * mtd->erasesize +
	    req->page * mtd->writesize;
	for (i = 0; i < req->page_count; ++i) {
		size_t len;

		int res = mtd->read(mtd, off, mtd->writesize, &len, dev->buf);
		if (unlikely((res && res != -EUCLEAN) || len != mtd->writesize))
			return res;

		if (unlikely(copy_to_user(dst, dev->buf, len)))
			return -EFAULT;

		dst += len;
		off += len;
	}

	return 0;
}

static int spectra_is_bad_block(struct spectra_device *dev, unsigned long block)
{
	loff_t off;

	if (unlikely(block < CEFDK_CFG_PARTITION_OFFSET ||
		     block >= dev->device_info.blocks))
		return -EINVAL;

	off = (block - CEFDK_CFG_PARTITION_OFFSET) * dev->mtd->erasesize;
	return dev->mtd->block_isbad(dev->mtd, off);
}

static void spectra_erase_callback (struct erase_info *instr)
{
        wake_up((wait_queue_head_t *)instr->priv);
}

static int spectra_erase_block(struct spectra_device *dev, unsigned long block)
{
	DBG("erase_block=%lu\n", block);

	// TODO(richard) actually erasing the block destroys the mac address
	// it is not clear why libnand_config erases these blocks and does
	// not re-write them. this needs investigating with Intel.

#if 1
	return 0;
#else
	int ret;
	struct erase_info *erase;
	wait_queue_head_t waitq;
	DECLARE_WAITQUEUE(wait, current);

	init_waitqueue_head(&waitq);

	if (unlikely(block < CEFDK_CFG_PARTITION_OFFSET ||
		     block >= dev->device_info.blocks))
		return -EINVAL;

	erase = kzalloc(sizeof(struct erase_info), GFP_KERNEL);
	if (!erase)
		return -ENOMEM;

	erase->addr = (block - CEFDK_CFG_PARTITION_OFFSET) * dev->mtd->erasesize;
	erase->len = dev->device_info.erasesize;

	erase->mtd = dev->mtd;
	erase->callback = spectra_erase_callback;
	erase->priv = (unsigned long) &waitq;

	ret = dev->mtd->erase(dev->mtd, erase);
	if (!ret) {
		set_current_state(TASK_UNINTERRUPTIBLE);
		add_wait_queue(&waitq, &wait);
		if (erase->state != MTD_ERASE_DONE &&
		    erase->state != MTD_ERASE_FAILED)
			schedule();
		remove_wait_queue(&waitq, &wait);
		set_current_state(TASK_RUNNING);

		ret = (erase->state == MTD_ERASE_FAILED)?-EIO:0;
	}

	kfree(erase);

	return ret;
#endif
}

static int spectra_ioctl(struct inode *inode, struct file *file,
			 unsigned int cmd, unsigned long arg)
{
	struct spectra_device *dev = file->private_data;
	struct spectra_ioctl_req req;
	int res;

	DBG("ioctl(0x%04x, 0x%08lx)\n", cmd, arg);

	switch (cmd) {
	case SPECTRA_IO_GET_INFO:
		res = copy_to_user((void __user *)arg, &dev->device_info,
				   sizeof(dev->device_info)) ? -EFAULT : 0;
		break;

	case SPECTRA_IO_CHECK_BAD_BLOCK:
		res = spectra_is_bad_block(dev, arg);
		break;

	case SPECTRA_IO_READ_PAGES:
		if (unlikely(copy_from_user(&req, (void __user *)arg,
					    sizeof(req))))
			res = -EFAULT;
		else
			res = spectra_read_pages(dev, &req);
		break;

	case SPECTRA_IO_ERASE:
		res = spectra_erase_block(dev, arg);
		break;

	default:
		printk(KERN_WARNING DRV_NAME
		       ": unsupported IOCTL 0x%04x\n", cmd);
		res = -ENOTTY;
	}

	return res;
}

static struct file_operations spectra_emul_fops = {
	.owner = THIS_MODULE,
	.open = spectra_open,
	.ioctl = spectra_ioctl,
	.release = spectra_release,
};

static struct miscdevice spectra_emul_miscdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = DRV_NAME,
	.fops = &spectra_emul_fops,
};

static int __init intel_spectra_emul_init(void)
{
	return misc_register(&spectra_emul_miscdev);
}

static void __exit intel_spectra_emul_fini(void)
{
	misc_deregister(&spectra_emul_miscdev);
}

module_init(intel_spectra_emul_init);
module_exit(intel_spectra_emul_fini);
