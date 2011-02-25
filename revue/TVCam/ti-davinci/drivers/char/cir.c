/*
 * cir.c - Driver for Consumer Infrared (CIR) (on Davinci-HD EVM)
 *
 * Copyright (C) 2007  Texas Instruments, India
 * Author: Suresh Rajashekara <suresh.r@ti.com>
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
#include <linux/jiffies.h>
#include <linux/spinlock.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <asm/arch/cir.h>
#include <asm/arch/i2c-client.h>

#define MODULE_NAME   "Consumer IR"

/* Globals */
static dev_t cir_dev;
static u32 cir_major_number;
static u32 cir_minor_number;
static struct cdev cir_cdev;
static atomic_t reference_count = ATOMIC_INIT(0);
static DECLARE_MUTEX(mutex);

static spinlock_t cir_lock = SPIN_LOCK_UNLOCKED;

#define RAW_KEY_CODES 4
/* If another key press is detected within this jiffies duration, then the
 * driver discards the key press. */
static int repeat_delay;	/* Value in jiffies */

#define MAX_KEYS_IN_BUFFER  4
static s16 decoded_keys[MAX_KEYS_IN_BUFFER];
static u8 raw_key_buf[RAW_KEY_CODES] = {0};

static int key_read;
static int key_write;
static unsigned long next_key_time;
u8 cir_key_idx;

DECLARE_COMPLETION(cir_keys);

static u32 cir_reg_get ( u32 reg );
static void cir_reg_set ( u32 reg, u8 reg_value );
static void cir_reset(void);
static void configure_cir_registers(void);
int cir_release(struct inode *inode, struct file *file);

static struct class *cir_class;

static void cir_platform_release(struct device *device)
{
	/* this function does nothing */
}

static struct platform_device cir_device = {
	.name = "cir",
	.id = 0,
	.dev = {
		.release = cir_platform_release,
	}
};

static struct device_driver cir_driver = {
	.name = "cir",
	.bus = &platform_bus_type,
};



static int cir_flush(void)
{
	u32 i;

	unsigned int flags;
	spin_lock_irqsave(&cir_lock, flags);

	cir_reset();
	configure_cir_registers();

	key_read = 0;
	key_write = 0;
	cir_key_idx = 0;

	for (i = 0; i < MAX_KEYS_IN_BUFFER; i++)
		decoded_keys[i] = -1;

	spin_unlock_irqrestore(&cir_lock, flags);

	return 0;
}

static s16 decode_cir_value(u8 *raw_cir_data)
{
	u16 i;
	u32 acc;
	u16 lowbit;
	u16 highbit;
	u16 cir_value = 0;

	/* do the whole RC-5 decoding thing.. */
	acc = ( raw_cir_data[0] << 0 )
		| ( raw_cir_data[1] << 8 )
		| ( raw_cir_data[2] << 16 )
		| ( raw_cir_data[3] << 24 );

	acc = acc >> 1;
	for ( i = 0 ; i < 16 ; i++ ) {
		cir_value <<= 1;

		/* Low & High bits */
		lowbit  = ( acc & 0x0002 );
		highbit = ( acc & 0x0001 );

		/* One bit */
		if ( ( highbit == 0 ) && ( lowbit != 0 ) )
			cir_value |= 1;

		acc = acc >> 2;
	}


	/* check if the start bit is valid */
	if (cir_value & 0x8000)	{
		/* good key received (hopefully), tell everyone about it. */
		return (cir_value >> 3) & 0xfff;
	}

	/* wrong key received.. */
	return -1;
}


static inline s8 get_stored_key(s16 *key)
{
	s8 ret = -1;
	unsigned int flags;

	spin_lock_irqsave(&cir_lock, flags);

	if (decoded_keys[key_read] != -1) {
		/* return valid key */
		*key =  decoded_keys[key_read];
		/*after the valid key is read,invalidate the buffer with -1 */
		decoded_keys[key_read] = -1;
		ret = 0;

		if (++key_read >= MAX_KEYS_IN_BUFFER)
			key_read = 0;
		}

	spin_unlock_irqrestore(&cir_lock, flags);

	return ret;
}

static inline void store_received_key(s16 key)
{
	unsigned int flags;

	spin_lock_irqsave(&cir_lock, flags);
	/*store the decoded key into the buffer which will be read later*/
	decoded_keys[key_write] = key;

	if (++key_write >= MAX_KEYS_IN_BUFFER)
			key_write = 0;

	spin_unlock_irqrestore(&cir_lock, flags);
}

ssize_t cir_read (struct file *file, char __user *buff, size_t size, loff_t
		  *loff)
{
	u16 key;

	/* we only allow one key read at a time */
	if (size != sizeof(u16)) {
		printk (KERN_ERR "Invalid size requested for read\n");
		return -EFAULT;
	}

begin:
	if (get_stored_key(&key) != -1) {
		/* there is a key to be read.. */
		if (copy_to_user(buff, &key, sizeof(u16)) != 0) return -EFAULT;
	} else {

		/* tell non blocking applications to come again */
	if (file->f_flags & O_NONBLOCK) {
			return -EAGAIN;
		} else { /* ask blocking applications to sleep comfortably */
			if (wait_for_completion_interruptible(&cir_keys))
				return -ERESTARTSYS;
			goto begin;
		}
	}

	return size;
}

static u32 cir_reg_get ( u32 reg )
{
	u32 reg_value;
	u32 lcr = 0;
	u32 preg32 = IO_ADDRESS((CIR_BASE) + (reg & ~(USE_LCR_80|USE_LCR_BF)));
	u32 set_lcr_80 = ((u32)reg) & USE_LCR_80;
	u32 set_lcr_bf = ((u32)reg) & USE_LCR_BF;

	/* Set LCR if needed */
	if ( set_lcr_80 ) {
		lcr = cir_reg_get( LCR );
		cir_reg_set( LCR, 0x80 );
	}

	if ( set_lcr_bf ) {
		lcr = cir_reg_get( LCR );
		cir_reg_set( LCR, 0xbf );
	}

	/* Get UART register */
	reg_value = *( volatile u32 * )( preg32 );

	/* Return LCR reg if necessary */
	if ( set_lcr_80 || set_lcr_bf ) {
		cir_reg_set( LCR, lcr );
	}

	return reg_value;
}

static void cir_reg_set ( u32 reg, u8 reg_value )
{
	u32 lcr = 0;
	u32 preg32 = IO_ADDRESS((CIR_BASE) + (reg & ~(USE_LCR_80|USE_LCR_BF)));
	u32 set_lcr_80 = ((u32)reg) & USE_LCR_80;
	u32 set_lcr_bf = ((u32)reg) & USE_LCR_BF;

	/* Set LCR if needed */
	if ( set_lcr_80 ) {
		lcr = cir_reg_get( LCR );
		cir_reg_set( LCR, 0x80 );
	}

	if ( set_lcr_bf ) {
		lcr = cir_reg_get( LCR );
		cir_reg_set( LCR, 0xbf );
	}

	/* Set UART register */
	*( volatile u32 * )( preg32 ) = reg_value;

	/* Return LCR reg if necessary */
	if ( set_lcr_80 || set_lcr_bf ) {
		cir_reg_set( LCR, lcr );
	}
}

static void configure_cir_registers (void)
{

	/* Enable FIFO, set the granularity to
	   cause interrupt after 4 bytes in FIFO */
	cir_reg_set(FCR, 0x1);
	cir_reg_set(TLR, 0x10);
	cir_reg_set(SCR, 0x80);

	cir_reg_set( EFR, 0x10 );
	cir_reg_set( IER, 0 );
	cir_reg_set( MCR, 0 );
	cir_reg_set( EFR, 0 );
	cir_reg_set( LCR, 0 );
	cir_reg_set( MDR1, 0x07 );
	cir_reg_set( LCR, 0xbf );
	cir_reg_set( IIR, 0x10 );
	cir_reg_set( LCR, 0x87 );
	cir_reg_set( IER, 0x05 );
	cir_reg_set( RHR, 0x35 );
	cir_reg_set( LCR, 0x06 );
	cir_reg_set( MCR, 0x01 );
	cir_reg_set( IER, 0x01 );
	cir_reg_set( EBLR, 0x05 );
	cir_reg_set( SFLSR, 0x01 );
	cir_reg_set( RESUME, 0x00 );
	cir_reg_set( SFREGL, 0x04 );
	cir_reg_set( SFREGH, 0x00 );
	cir_reg_set( LCR, 0x07 );
	cir_reg_set( CFPS, 56 );
	cir_reg_set( MDR2, 0x58 );
	cir_reg_set( MDR1, 0x06 );
}

static void cir_reset( void )
{
	u32 val = 0;


	cir_reg_set( EFR, 0x10 );
	cir_reg_set( IER, 0 );
	cir_reg_set( MCR, 0 );
	cir_reg_set( EFR, 0 );
	cir_reg_set( LCR, 0 );
	cir_reg_set( MDR1, 0x07 );
	cir_reg_get ( RESUME );

	/* disable FIFO and clear RX and TX fifos  */
	val = cir_reg_get(FCR);
	val &= 0xFE;
	val |= 0x6;
	cir_reg_set(FCR, val);
}

int cir_open (struct inode *inode, struct file *file)
{
	int i;

	INIT_COMPLETION(cir_keys);
	davinci_i2c_expander_op(CIR_MOD_DM646X, 0);
	next_key_time = jiffies;

	if (file->f_mode == FMODE_WRITE) {
		printk (KERN_ERR "TX Not supported\n");
		return -EACCES;
	}

	if (atomic_inc_return(&reference_count) > 1) {
		atomic_dec(&reference_count);
		return -EACCES;
	}

	for (i = 0; i < MAX_KEYS_IN_BUFFER; i++)
		decoded_keys[i] = -1;

	cir_reset();
	configure_cir_registers();

	return 0;
}

int cir_release (struct inode *inode, struct file *file)
{
	complete_all(&cir_keys);
	cir_reset();
	atomic_dec(&reference_count);
	return 0;
}

static irqreturn_t cir_irq_handler (int irq, void *dev_id, struct pt_regs *regs)
{
	s16 key;
	/* loop while fifo not empty */
	while ((cir_reg_get(LSR) & 0x1) != 1) {

		raw_key_buf[cir_key_idx++] = cir_reg_get(RHR);

		/* if all the codes necessary for for decode are received */
		if (cir_key_idx == RAW_KEY_CODES) {
			/* lets see if the debounce period is complete */
			if (time_after(jiffies, next_key_time)) {
				/* .. go ahead and decode the value */
				key = decode_cir_value(raw_key_buf);
				if (key != -1) {
					store_received_key(key);
					next_key_time = jiffies + repeat_delay;
				complete (&cir_keys);
			}
				else
					cir_flush();
			}
			cir_key_idx = 0;
		}
	}

	cir_reg_get(RESUME);

	return IRQ_HANDLED;
}

int cir_ioctl ( struct inode *inode, struct file *filp, unsigned int cmd,
		unsigned long arg )
{
	int err = 0, time_val = 0, ret_val = 0;

	if ( _IOC_TYPE(cmd) != CIR_IOC_MAGIC ) {
		return -ENOTTY;
	}

	if ( _IOC_NR(cmd) > CIR_IOC_MAXNR ) {
		return -ENOTTY;
	}

	if ( _IOC_DIR(cmd) & _IOC_READ ) {
		err = !access_ok( VERIFY_WRITE, (void __user *)arg,
				   _IOC_SIZE(cmd));
	} else if ( _IOC_DIR(cmd) & _IOC_WRITE ) {
		err =  !access_ok( VERIFY_READ, (void __user *)arg,
				   _IOC_SIZE(cmd));
	}

	if (err) {
		return -EFAULT;
	}

	switch ( cmd ) {

	case CIR_FLUSH:
		cir_flush();
		return 0;

		/* Value sent by user will be in ms, we convert that */
		/* to jiffies */
	case CIR_SET_REPEAT_DELAY:
		ret_val = __get_user (time_val, (int __user *)arg);
		if (ret_val == 0) {
			repeat_delay = time_val/10;
			return (repeat_delay*10);
		} else {
			return -EFAULT;
		}
		break;

		/* Value returned from this ioctl has to be in ms */
	case CIR_GET_REPEAT_DELAY:
		ret_val = __put_user ((repeat_delay*10), (int __user *)arg);
		if ( ret_val == 0 ) {
			return 0;
		} else {
			return -EFAULT;
		}
		break;

	default:
		printk (KERN_ERR "Unknow IOCTL command\n");
		break;
	}

	return -ENOTTY;
}

static struct file_operations cir_fops = {
	.owner   = THIS_MODULE,
	.read    = cir_read,
	.open    = cir_open,
	.release = cir_release,
	.ioctl   = cir_ioctl,
};

static int __init cir_init ( void )
{
	s8 retval = 0;
	struct clk *clkp;

	cir_major_number = 0;
	cir_minor_number = 0;
	cir_class = NULL;

	repeat_delay = 20;	/* in Jiffies */

	cir_key_idx = 0;

	clkp = clk_get (NULL, "UART2");

	if ( IS_ERR ( clkp ) ) {
		printk (KERN_ERR "Unable to get the clock for CIR\n");
		goto failure;
	} else
		clk_enable (clkp);

	if (cir_major_number) {	/* If major number is specified */
		cir_dev = MKDEV(cir_major_number, 0);
		retval = register_chrdev_region (cir_dev,
						 CIR_DEV_COUNT,
						 "/dev/cir");
	} else {			/* If major number is not specified */
		retval = alloc_chrdev_region (&cir_dev,
					      cir_minor_number,
					      CIR_DEV_COUNT,
					      "/dev/cir");
		cir_major_number = MAJOR(cir_dev);
	}

	if (retval < 0) {
		printk (KERN_ERR "Unable to register the CIR device\n");
		retval = -ENODEV;
		goto failure;
	} else {
		printk(KERN_INFO "CIR device registered successfully "
			  "(Major = %d, Minor = %d)\n",
			  MAJOR(cir_dev), MINOR(cir_dev) );
	}

	cdev_init (&cir_cdev, &cir_fops);
	cir_cdev.owner = THIS_MODULE;
	cir_cdev.ops = &cir_fops;

	/* You should not call cdev_add until your driver is completely ready to
	   handle operations on the device*/
	retval = cdev_add (&cir_cdev, cir_dev, CIR_DEV_COUNT);

	if (retval) {
		unregister_chrdev_region (cir_dev, CIR_DEV_COUNT);
		printk (KERN_ERR "Error %d adding CIR device\n", retval);
		goto failure;
	}

	retval = request_irq (CIR_IRQ, cir_irq_handler,
			      SA_INTERRUPT|SA_SAMPLE_RANDOM, "Consumer IR",
			      NULL);
	if (retval) {
		unregister_chrdev_region (cir_dev, CIR_DEV_COUNT);
		cdev_del (&cir_cdev);
		printk (KERN_ERR "Unable to register CIR IRQ %d\n", CIR_IRQ );
		goto failure;
	}

	cir_class = class_create(THIS_MODULE, "cir");

	if (!cir_class) {
		unregister_chrdev_region (cir_dev, CIR_DEV_COUNT);
		cdev_del(&cir_cdev);
		goto failure;
	}

	if (driver_register(&cir_driver) != 0) {
		unregister_chrdev_region (cir_dev, CIR_DEV_COUNT);
		cdev_del(&cir_cdev);
		class_destroy(cir_class);
		goto failure;
	}
	/* Register the drive as a platform device */
	if (platform_device_register(&cir_device) != 0) {
		driver_unregister(&cir_driver);
		unregister_chrdev_region (cir_dev, CIR_DEV_COUNT);
		cdev_del(&cir_cdev);
		class_destroy(cir_class);
		goto failure;
	}

	cir_dev = MKDEV(cir_major_number, 0);

	class_device_create(cir_class, NULL, cir_dev, NULL, "cir");

	spin_lock_init(&cir_lock);

 failure:
	return retval;
}

static void __exit cir_exit ( void )
{
	printk (KERN_INFO "Unregistering the CIR device");

	driver_unregister(&cir_driver);
	platform_device_unregister(&cir_device);
	free_irq(CIR_IRQ, NULL);
	cir_dev = MKDEV(cir_major_number, 0);
	class_device_destroy(cir_class, cir_dev);
	class_destroy(cir_class);
	cdev_del(&cir_cdev);
	unregister_chrdev_region(cir_dev, CIR_DEV_COUNT);

	return;
}

module_init (cir_init);
module_exit (cir_exit);

MODULE_AUTHOR ( "Texas Instruments" );
MODULE_DESCRIPTION ( "Consumer Infrared (CIR) Driver" );
MODULE_LICENSE ( "GPL" );
