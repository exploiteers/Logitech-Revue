/*
 * DaVinci platform-level USB stuff
 *
 * MontaVista Software, Inc. <source@mvista.com>
 *
 * 2008 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 *
 *	otg_{get,set}_transceiver() are from arm/plat-omap/usb.c.
 *	Copyright (C) 2004 Texas Instruments, Inc.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/usb/otg.h>

static struct otg_transceiver *xceiv;

/**
 * otg_get_transceiver - find the (single) OTG transceiver driver
 *
 * Returns the transceiver driver, after getting a refcount to it; or
 * null if there is no such transceiver.  The caller is responsible for
 * releasing that count.
 */
struct otg_transceiver *otg_get_transceiver(void)
{
	pr_debug("%s xceiv=0x%p\n", __func__, xceiv);
	if (xceiv)
		get_device(xceiv->dev);
	return xceiv;
}
EXPORT_SYMBOL(otg_get_transceiver);

int otg_set_transceiver(struct otg_transceiver *x)
{
	pr_debug("%s xceiv=0x%p  x=0x%p\n", __func__, xceiv, x);
	if (xceiv && x)
		return -EBUSY;
	xceiv = x;
	return 0;
}
EXPORT_SYMBOL(otg_set_transceiver);
