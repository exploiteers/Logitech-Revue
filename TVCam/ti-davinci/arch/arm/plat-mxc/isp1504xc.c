/*
 * Copyright 2005-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/usb/fsl_xcvr.h>
#include <linux/io.h>

#include <asm/hardware.h>
#include <asm/arch/arc_otg.h>

/* ISP 1504 register addresses */
#define ISP1504_VID_LOW		0x00	/* Vendor ID low */
#define ISP1504_VID_HIGH	0x01	/* Vendor ID high */
#define ISP1504_PID_LOW		0x02	/* Product ID low */
#define ISP1504_PID_HIGH	0x03	/* Product ID high */
#define ISP1504_ITFCTL		0x07	/* Interface Control */
#define ISP1504_OTGCTL		0x0A	/* OTG Control */

/* add to above register address to access Set/Clear functions */
#define ISP1504_REG_SET		0x01
#define ISP1504_REG_CLEAR	0x02

/* 1504 OTG Control Register bits */
#define USE_EXT_VBUS_IND	(1 << 7)	/* Use ext. Vbus indicator */
#define DRV_VBUS_EXT		(1 << 6)	/* Drive Vbus external */
#define DRV_VBUS		(1 << 5)	/* Drive Vbus */
#define CHRG_VBUS		(1 << 4)	/* Charge Vbus */
#define DISCHRG_VBUS		(1 << 3)	/* Discharge Vbus */
#define DM_PULL_DOWN		(1 << 2)	/* enable DM Pull Down */
#define DP_PULL_DOWN		(1 << 1)	/* enable DP Pull Down */
#define ID_PULL_UP		(1 << 0)	/* enable ID Pull Up */

#ifdef DEBUG
/*!
 * read ULPI register 'reg' thru VIEWPORT register 'view'
 *
 * @param       reg   register to read
 * @param       view  the ULPI VIEWPORT register address
 * @return	return isp1504 register value
 */
static u8 isp1504_read(int reg, u32 *view)
{
	u32 data;

	/* make sure interface is running */
	if (!(__raw_readl(view) && ULPIVW_SS)) {
		__raw_writel(ULPIVW_WU, view);
		do {		/* wait for wakeup */
			data = __raw_readl(view);
		} while (data & ULPIVW_WU);
	}

	/* read the register */
	__raw_writel((ULPIVW_RUN | (reg << ULPIVW_ADDR_SHIFT)), view);

	do {			/* wait for completion */
		data = __raw_readl(view);
	} while (data & ULPIVW_RUN);

	return (u8) (data >> ULPIVW_RDATA_SHIFT) & ULPIVW_RDATA_MASK;
}
#endif

/*!
 * set bits into OTG ISP1504 register 'reg' thru VIEWPORT register 'view'
 *
 * @param       bits  set value
 * @param	reg   which register
 * @param       view  the ULPI VIEWPORT register address
 */
static void isp1504_set(u8 bits, int reg, u32 *view)
{
	u32 data;

	/* make sure interface is running */
	if (!(__raw_readl(view) && ULPIVW_SS)) {
		__raw_writel(ULPIVW_WU, view);
		do {		/* wait for wakeup */
			data = __raw_readl(view);
		} while (data & ULPIVW_WU);
	}

	__raw_writel((ULPIVW_RUN | ULPIVW_WRITE |
		      ((reg + ISP1504_REG_SET) << ULPIVW_ADDR_SHIFT) |
		      ((bits & ULPIVW_WDATA_MASK) << ULPIVW_WDATA_SHIFT)),
		     view);

	while (__raw_readl(view) & ULPIVW_RUN)	/* wait for completion */
		continue;
}

/*!
 * clear bits in OTG ISP1504 register 'reg' thru VIEWPORT register 'view'
 *
 * @param       bits  bits to clear
 * @param	reg   in this register
 * @param       view  the ULPI VIEWPORT register address
 */
static void isp1504_clear(u8 bits, int reg, u32 *view)
{
	__raw_writel((ULPIVW_RUN | ULPIVW_WRITE |
		      ((reg + ISP1504_REG_CLEAR) << ULPIVW_ADDR_SHIFT) |
		      ((bits & ULPIVW_WDATA_MASK) << ULPIVW_WDATA_SHIFT)),
		     view);

	while (__raw_readl(view) & ULPIVW_RUN)	/* wait for completion */
		continue;
}

int gpio_usbotg_hs_active(void);

static void isp1508_fix(u32 *view)
{
	/* Set bits IND_PASS_THRU and IND_COMPL */
	isp1504_set(0x60, ISP1504_ITFCTL, view);

	/* Set bit USE_EXT_VBUS_IND */
	isp1504_set(USE_EXT_VBUS_IND, ISP1504_OTGCTL, view);
}

/*!
 * set vbus power
 *
 * @param       view  viewport register
 * @param       on    power on or off
 */
static void isp1504_set_vbus_power(u32 *view, int on)
{
	pr_debug("real %s(on=%d) view=0x%p\n", __FUNCTION__, on, view);

	pr_debug("ULPI Vendor ID 0x%x    Product ID 0x%x\n",
		 (isp1504_read(ISP1504_VID_HIGH, view) << 8) |
		 isp1504_read(ISP1504_VID_LOW, view),
		 (isp1504_read(ISP1504_PID_HIGH, view) << 8) |
		 isp1504_read(ISP1504_PID_LOW, view));

	pr_debug("OTG Control before=0x%x\n",
		 isp1504_read(ISP1504_OTGCTL, view));

	if (on) {
		isp1504_set(DRV_VBUS_EXT |	/* enable external Vbus */
			    DRV_VBUS |	/* enable internal Vbus */
			    USE_EXT_VBUS_IND |	/* use external indicator */
			    CHRG_VBUS,	/* charge Vbus */
			    ISP1504_OTGCTL, view);

	} else {
		isp1508_fix(view);

		isp1504_clear(DRV_VBUS_EXT |	/* disable external Vbus */
			      DRV_VBUS,	/* disable internal Vbus */
			      ISP1504_OTGCTL, view);

		isp1504_set(USE_EXT_VBUS_IND |	/* use external indicator */
			    DISCHRG_VBUS,	/* discharge Vbus */
			    ISP1504_OTGCTL, view);
	}

	pr_debug("OTG Control after = 0x%x\n",
		 isp1504_read(ISP1504_OTGCTL, view));
}

/*!
 * set remote wakeup
 *
 * @param       view  viewport register
 */
static void isp1504_set_remote_wakeup(u32 *view)
{
	__raw_writel(~ULPIVW_WRITE & __raw_readl(view), view);
	__raw_writel((1 << ULPIVW_PORT_SHIFT) | __raw_readl(view), view);
	__raw_writel(ULPIVW_RUN | __raw_readl(view), view);

	while (__raw_readl(view) & ULPIVW_RUN)	/* wait for completion */
		continue;
}

static void isp1504_init(struct fsl_xcvr_ops *this)
{
	pr_debug("%s:\n", __FUNCTION__);
}

static void isp1504_uninit(struct fsl_xcvr_ops *this)
{
	pr_debug("%s:\n", __FUNCTION__);
}

static struct fsl_xcvr_ops isp1504_ops = {
	.name = "isp1504",
	.xcvr_type = PORTSC_PTS_ULPI,
	.init = isp1504_init,
	.uninit = isp1504_uninit,
	.set_host = NULL,
	.set_device = NULL,
	.set_vbus_power = isp1504_set_vbus_power,
	.set_remote_wakeup = isp1504_set_remote_wakeup,
};

void fsl_usb_xcvr_register(struct fsl_xcvr_ops *xcvr_ops);

static int __init isp1504xc_init(void)
{
	pr_debug("%s\n", __FUNCTION__);

	fsl_usb_xcvr_register(&isp1504_ops);

	return 0;
}

void fsl_usb_xcvr_unregister(struct fsl_xcvr_ops *xcvr_ops);

static void __exit isp1504xc_exit(void)
{
	fsl_usb_xcvr_unregister(&isp1504_ops);
}

module_init(isp1504xc_init);
module_exit(isp1504xc_exit);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("isp1504 xcvr driver");
MODULE_LICENSE("GPL");
