/*
 * drivers/char/watchdog/davinci_wdt.c
 *
 * Watchdog driver for DaVinci DM644x/DM646x processors
 *
 * Copyright (C) 2006 Texas Instruments.
 *
 * 2007 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/watchdog.h>
#include <linux/init.h>
#include <linux/bitops.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>

#include <asm/hardware.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#include <asm/arch/time.h>

#define MODULE_NAME "DAVINCI-WDT: "

#define DEFAULT_HEARTBEAT 60
#define MAX_HEARTBEAT     600	/* really the max margin is 264/27MHz*/

/* Timer register set definition */
static int heartbeat = DEFAULT_HEARTBEAT;

static spinlock_t io_lock;
static unsigned long wdt_status;
#define WDT_IN_USE        0
#define WDT_OK_TO_CLOSE   1
#define WDT_REGION_INITED 2
#define WDT_DEVICE_INITED 3

static struct resource	*wdt_mem;
static void __iomem	*wdt_base;

static void wdt_service(void)
{
	spin_lock(&io_lock);

	/* Put watchdog in service state */
	davinci_writel(WDTCR_WDKEY_SEQ0 << WDTCR_WDKEY_SHIFT, wdt_base + WDTCR);
	/* Put watchdog in active state */
	davinci_writel(WDTCR_WDKEY_SEQ1 << WDTCR_WDKEY_SHIFT, wdt_base + WDTCR);

	spin_unlock(&io_lock);
}

static void wdt_enable(void)
{
	u64 timer_margin = (u64)heartbeat * DAVINCI_CLOCK_TICK_RATE;
	u32 tgcr, wdtcr;

	spin_lock(&io_lock);

	/* Disable, internal clock source */
	davinci_writel(0, wdt_base + TCR);
	/* Reset timer, set mode to 64-bit watchdog, and unreset */
	davinci_writel(0, wdt_base + TGCR);
	tgcr =	(TGCR_TIMMODE_64BIT_WDOG << TGCR_TIMMODE_SHIFT) |
		(TGCR_UNRESET << TGCR_TIM12RS_SHIFT) |
		(TGCR_UNRESET << TGCR_TIM34RS_SHIFT);
	davinci_writel(tgcr, wdt_base + TGCR);
	/* Clear counter registers */
	davinci_writel(0, wdt_base + TIM12);
	davinci_writel(0, wdt_base + TIM34);
	/* Set timeout period */
	davinci_writel((u32) timer_margin, wdt_base + PRD12);
	davinci_writel((u32)(timer_margin >> 32), wdt_base + PRD34);
	/* Enable continuous timer mode */
	davinci_writel(TCR_ENAMODE_PERIODIC << ENAMODE12_SHIFT, wdt_base + TCR);
	/*
	 * Put watchdog in pre-active state.
	 * NOTE: once the WDT is in pre-active state, TIM12, TIM34,
	 * PRD12, PRD34, TCR, TGCR, WDTCR registers are write protected
	 * (except for the WDKEY field).
	 */
	wdtcr = (WDTCR_WDKEY_SEQ0 << WDTCR_WDKEY_SHIFT) |
		(WDTCR_WDEN_ENABLE << WDTCR_WDEN_SHIFT);
	davinci_writel(wdtcr, wdt_base + WDTCR);
	/* put watchdog in active state */
	wdtcr = (WDTCR_WDKEY_SEQ1 << WDTCR_WDKEY_SHIFT) |
		(WDTCR_WDEN_ENABLE << WDTCR_WDEN_SHIFT);
	davinci_writel(wdtcr, wdt_base + WDTCR);

	spin_unlock(&io_lock);
}

static int davinci_wdt_open(struct inode *inode, struct file *file)
{
	if (test_and_set_bit(WDT_IN_USE, &wdt_status))
		return -EBUSY;

	wdt_enable();

	return nonseekable_open(inode, file);
}

static ssize_t
davinci_wdt_write(struct file *file, const char *data, size_t len,
		  loff_t *ppos)
{
	if (len)
		wdt_service();

	return len;
}

static struct watchdog_info ident = {
	.options = WDIOF_KEEPALIVEPING,
	.identity = "DaVinci Watchdog",
};

static int
davinci_wdt_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
		  unsigned long arg)
{
	int ret = -ENOIOCTLCMD;

	switch (cmd) {
	case WDIOC_GETSUPPORT:
		ret = copy_to_user((struct watchdog_info *)arg, &ident,
				   sizeof(ident)) ? -EFAULT : 0;
		break;

	case WDIOC_GETSTATUS:
	case WDIOC_GETBOOTSTATUS:
		ret = put_user(0, (int *)arg);
		break;

	case WDIOC_GETTIMEOUT:
		ret = put_user(heartbeat, (int *)arg);
		break;

	case WDIOC_KEEPALIVE:
		wdt_service();
		ret = 0;
		break;
	}
	return ret;
}

static int davinci_wdt_release(struct inode *inode, struct file *file)
{
	wdt_service();
	clear_bit(WDT_IN_USE, &wdt_status);

	return 0;
}

static const struct file_operations davinci_wdt_fops = {
	.owner = THIS_MODULE,
	.llseek = no_llseek,
	.write = davinci_wdt_write,
	.ioctl = davinci_wdt_ioctl,
	.open = davinci_wdt_open,
	.release = davinci_wdt_release,
};

static struct miscdevice davinci_wdt_miscdev = {
	.minor = WATCHDOG_MINOR,
	.name = "watchdog",
	.fops = &davinci_wdt_fops,
};

static int davinci_wdt_probe(struct platform_device *pdev)
{
	int ret = 0, size;
	struct resource *res;

	spin_lock_init(&io_lock);

	if (heartbeat < 1 || heartbeat > MAX_HEARTBEAT)
		heartbeat = DEFAULT_HEARTBEAT;

	printk(KERN_INFO MODULE_NAME
		"DaVinci Watchdog Timer: heartbeat %d sec\n", heartbeat);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		printk(KERN_INFO MODULE_NAME
			"failed to get memory region resource\n");
		return -ENOENT;
	}

	size = res->end - res->start + 1;
	wdt_mem = request_mem_region(res->start, size, pdev->name);

	if (wdt_mem == NULL) {
		printk(KERN_INFO MODULE_NAME "failed to get memory region\n");
		return -ENOENT;
	}
	wdt_base = (void __iomem *)(res->start);

	ret = misc_register(&davinci_wdt_miscdev);
	if (ret < 0) {
		printk(KERN_ERR MODULE_NAME "cannot register misc device\n");
		release_resource(wdt_mem);
		kfree(wdt_mem);
	} else {
		set_bit(WDT_DEVICE_INITED, &wdt_status);
	}

	return ret;
}

static int davinci_wdt_remove(struct platform_device *pdev)
{
	misc_deregister(&davinci_wdt_miscdev);
	if (wdt_mem) {
		release_resource(wdt_mem);
		kfree(wdt_mem);
		wdt_mem = NULL;
	}
	return 0;
}

static struct platform_driver platform_wdt_driver = {
	.driver = {
		.name = "watchdog",
	},
	.probe = davinci_wdt_probe,
	.remove = davinci_wdt_remove,
};

static int __init davinci_wdt_init(void)
{
	return platform_driver_register(&platform_wdt_driver);
}

static void __exit davinci_wdt_exit(void)
{
	return platform_driver_unregister(&platform_wdt_driver);
}

module_init(davinci_wdt_init);
module_exit(davinci_wdt_exit);

MODULE_AUTHOR("Texas Instruments");
MODULE_DESCRIPTION("DaVinci Watchdog Driver");

module_param(heartbeat, int, 0);
MODULE_PARM_DESC(heartbeat,
		 "Watchdog heartbeat period in seconds from 1 to "
		 __MODULE_STRING(MAX_HEARTBEAT) ", default "
		 __MODULE_STRING(DEFAULT_HEARTBEAT));

MODULE_LICENSE("GPL");
MODULE_ALIAS_MISCDEV(WATCHDOG_MINOR);
