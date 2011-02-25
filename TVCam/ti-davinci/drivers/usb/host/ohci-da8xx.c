/*
 * OHCI HCD (Host Controller Driver) for USB.
 *
 * TI DA830 (OMAP-L137) Bus Glue
 *
 * Derived from: ohci-omap.c and ohci-s3c2410.c
 * Copyright (C) 2008-2009 MontaVista Software, Inc. <source@mvista.com>
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */

#include <linux/interrupt.h>
#include <linux/jiffies.h>
#include <linux/platform_device.h>
#include <linux/clk.h>

#include <asm/hardware.h>
#include <asm/arch/usb.h>

#ifndef CONFIG_ARCH_DA8XX
#error "This file is DA8xx bus glue.  Define CONFIG_ARCH_DA8XX."
#endif

static struct clk *usb11_clk;
static struct clk *usb20_clk;

/* Over-current indicator change bitmask */
static volatile u16 ocic_mask;

static void ohci_da8xx_clock(int on)
{
	u32 cfgchip2;

	cfgchip2 = davinci_cfg_readl(DA8XX_CFGCHIP(2));
	if (on) {
		clk_enable(usb11_clk);

		/*
		 * If USB 1.1 reference clock is sourced from USB 2.0 PHY, we
		 * need to enable the USB 2.0 module clocking, start its PHY,
		 * and not allow it to stop the clock during USB 2.0 suspend.
		 */
		if (!(cfgchip2 & CFGCHIP2_USB1PHYCLKMUX)) {
			clk_enable(usb20_clk);

			cfgchip2 &= ~(CFGCHIP2_RESET | CFGCHIP2_PHYPWRDN);
			cfgchip2 |= CFGCHIP2_PHY_PLLON;
			davinci_cfg_writel(cfgchip2, DA8XX_CFGCHIP(2));

			pr_info("Waiting for USB PHY clock good...\n");
			while (!(davinci_cfg_readl(DA8XX_CFGCHIP(2)) &
				 CFGCHIP2_PHYCLKGD))
				cpu_relax();
		}

		/* Enable USB 1.1 PHY */
		cfgchip2 |= CFGCHIP2_USB1SUSPENDM;
	} else {
		clk_disable(usb11_clk);
		if (!(cfgchip2 & CFGCHIP2_USB1PHYCLKMUX))
			clk_disable(usb20_clk);

		/* Disable USB 1.1 PHY */
		cfgchip2 &= ~CFGCHIP2_USB1SUSPENDM;
	}
	davinci_cfg_writel(cfgchip2, DA8XX_CFGCHIP(2));
}

/*
 * Handle the port over-current indicator change.
 */
static void ohci_da8xx_ocic_handler(struct da8xx_ohci_root_hub *hub,
				    unsigned port)
{
	ocic_mask |= 1 << port;

	/* Once over-current is detected, the port needs to be powered down */
	if (hub->get_oci(port) > 0)
		hub->set_power(port, 0);
}

static int ohci_da8xx_init(struct usb_hcd *hcd)
{
	struct device *dev		= hcd->self.controller;
	struct da8xx_ohci_root_hub *hub	= dev->platform_data;
	struct ohci_hcd	*ohci		= hcd_to_ohci(hcd);
	int result;
	u32 rh_a;

	dev_dbg(dev, "starting USB controller\n");

	ohci_da8xx_clock(1);

	/*
	 * DA830 only has 1 port connected to the pins but its HC root hub
	 * register A reports 2 ports, thus we'll have to override it...
	 */
	ohci->num_ports = 1;

	result = ohci_init(ohci);
	if (result < 0)
		return result;

	/*
	 * Since we're providing a board-specific root hub port power control
	 * and over-current reporting, we have to override the HC root hub A
	 * register's default value, so that ohci_hub_control() could return
	 * the correct hub descriptor...
	 */
	rh_a = ohci_readl(ohci, &ohci->regs->roothub.a);
	rh_a &= ~(RH_A_POTPGT | RH_A_NPS | RH_A_NOCP);
	rh_a |=  (hub->potpgt << 24) | RH_A_PSM | RH_A_OCPM;
	ohci_writel(ohci, rh_a, &ohci->regs->roothub.a);

	return result;
}

static void ohci_da8xx_stop(struct usb_hcd *hcd)
{
	ohci_stop(hcd);
	ohci_da8xx_clock(0);
}

static int ohci_da8xx_start(struct usb_hcd *hcd)
{
	struct ohci_hcd	*ohci		= hcd_to_ohci(hcd);
	int result;

	result = ohci_run(ohci);
	if (result < 0)
		ohci_da8xx_stop(hcd);

	return result;
}

/*
 * Update the status data from the hub with the over-current indicator change.
 */
static int ohci_da8xx_hub_status_data(struct usb_hcd *hcd, char *buf)
{
	int length		= ohci_hub_status_data(hcd, buf);

	/* See if we have OCIC bit set on port 1 */
	if (ocic_mask & (1 << 1)) {
		dev_dbg(hcd->self.controller, "over-current indicator change "
			"on port 1\n");

		if (!length)
			length = 1;

		buf[0] |= 1 << 1;
	}
	return length;
}

/*
 * Look at the control requests to the root hub and see if we need to override.
 */
static int ohci_da8xx_hub_control(struct usb_hcd *hcd, u16 typeReq, u16 wValue,
				  u16 wIndex, char *buf, u16 wLength)
{
	struct device *dev		= hcd->self.controller;
	struct da8xx_ohci_root_hub *hub	= dev->platform_data;
	int temp;

	switch (typeReq) {
	case GetPortStatus:
		/* Check the port number */
		if (wIndex != 1)
			break;

		dev_dbg(dev, "GetPortStatus(%u)\n", wIndex);

		temp = roothub_portstatus(hcd_to_ohci(hcd), wIndex - 1);

		/* The port power status (PPS) bit defaults to 1 */
		if (hub->get_power(wIndex) == 0)
			temp &= ~RH_PS_PPS;

		/* The port over-current indicator (POCI) bit is always 0 */
		if (hub->get_oci(wIndex) > 0)
			temp |=  RH_PS_POCI;

		/* The over-current indicator change (OCIC) bit is 0 too */
		if (ocic_mask & (1 << 1))
			temp |=  RH_PS_OCIC;

		put_unaligned(cpu_to_le32(temp), (__le32 *)buf);
		return 0;
	case SetPortFeature:
		temp = 1;
		goto check_port;
	case ClearPortFeature:
		temp = 0;

check_port:
		/* Check the port number */
		if (wIndex != 1)
			break;

		switch (wValue) {
		case USB_PORT_FEAT_POWER:
			dev_dbg(dev, "%sPortFeature(%u): %s\n",
				temp ? "Set" : "Clear", wIndex, "POWER");

			return hub->set_power(wIndex, temp) ? -EPIPE : 0;
		case USB_PORT_FEAT_C_OVER_CURRENT:
			dev_dbg(dev, "%sPortFeature(%u): %s\n",
				temp ? "Set" : "Clear", wIndex,
				"C_OVER_CURRENT");

			if (temp)
				ocic_mask |= 1 << wIndex;
			else
				ocic_mask &= ~(1 << wIndex);
			return 0;
		}
	}

	return ohci_hub_control(hcd, typeReq, wValue, wIndex, buf, wLength);
}

/*
 * We don't have OTG but the Mentor USB core in DA8xx does have it, so we
 * have to define an empty function to silence the warning in ohci-hcd.c...
 */
#ifdef CONFIG_USB_OTG
static void start_hnp(struct ohci_hcd *ohci) {}
#endif

static const struct hc_driver ohci_da8xx_hc_driver = {
	.description		= hcd_name,
	.product_desc		= "DA8xx OHCI",
	.hcd_priv_size		= sizeof(struct ohci_hcd),

	/*
	 * generic hardware linkage
	 */
	.irq			= ohci_irq,
	.flags			= HCD_USB11 | HCD_MEMORY,

	/*
	 * basic lifecycle operations
	 */
	.reset			= ohci_da8xx_init,
	.start			= ohci_da8xx_start,
	.stop			= ohci_da8xx_stop,
	.shutdown		= ohci_shutdown,

	/*
	 * managing i/o requests and associated device resources
	 */
	.urb_enqueue		= ohci_urb_enqueue,
	.urb_dequeue		= ohci_urb_dequeue,
	.endpoint_disable	= ohci_endpoint_disable,

	/*
	 * scheduling support
	 */
	.get_frame_number	= ohci_get_frame,

	/*
	 * root hub support
	 */
	.hub_status_data	= ohci_da8xx_hub_status_data,
	.hub_control		= ohci_da8xx_hub_control,
	.hub_irq_enable 	= ohci_rhsc_enable,
#ifdef	CONFIG_PM
	.bus_suspend		= ohci_bus_suspend,
	.bus_resume		= ohci_bus_resume,
#endif
	.start_port_reset	= ohci_start_port_reset,
};

/*-------------------------------------------------------------------------*/


/**
 * usb_hcd_da8xx_probe - initialize DA8xx-based HCDs
 * Context: !in_interrupt()
 *
 * Allocates basic resources for this USB host controller, and
 * then invokes the start() method for the HCD associated with it
 * through the hotplug entry's driver_data.
 */
static int usb_hcd_da8xx_probe(const struct hc_driver *driver,
			       struct platform_device *pdev)
{
	struct da8xx_ohci_root_hub *hub	= pdev->dev.platform_data;
	struct usb_hcd	*hcd;
	struct resource *mem;
	int error, irq;

	if (hub == NULL)
		return -ENODEV;

	usb11_clk = clk_get(&pdev->dev, "USB11CLK");
	if (IS_ERR(usb11_clk))
		return PTR_ERR(usb11_clk);

	usb20_clk = clk_get(&pdev->dev, "USB20CLK");
	if (IS_ERR(usb20_clk)) {
		error = PTR_ERR(usb20_clk);
		goto err0;
	}

	hcd = usb_create_hcd(driver, &pdev->dev, pdev->dev.bus_id);
	if (!hcd) {
		error = -ENOMEM;
		goto err1;
	}

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem) {
		error = -ENODEV;
		goto err2;
	}
	hcd->rsrc_start = mem->start;
	hcd->rsrc_len = mem->end - mem->start + 1;

	if (!request_mem_region(hcd->rsrc_start, hcd->rsrc_len, hcd_name)) {
		dev_dbg(&pdev->dev, "request_mem_region failed\n");
		error = -EBUSY;
		goto err2;
	}

	hcd->regs = (void __iomem *)(long)IO_ADDRESS(hcd->rsrc_start);

	ohci_hcd_init(hcd_to_ohci(hcd));

	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		error = -ENODEV;
		goto err3;
	}
	error = usb_add_hcd(hcd, irq, IRQF_DISABLED);
	if (error)
		goto err4;

	error = hub->ocic_notify(ohci_da8xx_ocic_handler);
	if (!error)
		return 0;

err4:
	usb_remove_hcd(hcd);
err3:
	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
err2:
	usb_put_hcd(hcd);
err1:
	clk_put(usb20_clk);
err0:
	clk_put(usb11_clk);
	return error;
}

/**
 * usb_hcd_da8xx_remove - shutdown processing for DA8xx-based HCDs
 * @dev: USB Host Controller being removed
 * Context: !in_interrupt()
 *
 * Reverses the effect of usb_hcd_da8xx_probe(), first invoking
 * the HCD's stop() method.  It is always called from a thread
 * context, normally "rmmod", "apmd", or something similar.
 */
static inline void
usb_hcd_da8xx_remove(struct usb_hcd *hcd, struct platform_device *pdev)
{
	struct da8xx_ohci_root_hub *hub	= pdev->dev.platform_data;

	hub->ocic_notify(NULL);
	usb_remove_hcd(hcd);
	release_mem_region(hcd->rsrc_start, hcd->rsrc_len);
	usb_put_hcd(hcd);
	clk_put(usb20_clk);
	clk_put(usb11_clk);
}

static int ohci_hcd_da8xx_drv_probe(struct platform_device *dev)
{
	return usb_hcd_da8xx_probe(&ohci_da8xx_hc_driver, dev);
}

static int ohci_hcd_da8xx_drv_remove(struct platform_device *dev)
{
	struct usb_hcd	*hcd = platform_get_drvdata(dev);

	usb_hcd_da8xx_remove(hcd, dev);
	platform_set_drvdata(dev, NULL);

	return 0;
}

#ifdef CONFIG_PM
static int ohci_da8xx_suspend(struct platform_device *dev, pm_message_t message)
{
	struct usb_hcd	*hcd	= platform_get_drvdata(dev);
	struct ohci_hcd	*ohci	= hcd_to_ohci(hcd);

	if (time_before(jiffies, ohci->next_statechange))
		msleep(5);
	ohci->next_statechange = jiffies;

	ohci_da8xx_clock(0);
	hcd->state = HC_STATE_SUSPENDED;
	dev->dev.power.power_state = PMSG_SUSPEND;
	return 0;
}

static int ohci_da8xx_resume(struct platform_device *dev)
{
	struct usb_hcd	*hcd	= platform_get_drvdata(dev);
	struct ohci_hcd	*ohci	= hcd_to_ohci(hcd);

	if (time_before(jiffies, ohci->next_statechange))
		msleep(5);
	ohci->next_statechange = jiffies;

	ohci_da8xx_clock(1);
	dev->dev.power.power_state = PMSG_ON;
	usb_hcd_resume_root_hub(hcd);
	return 0;
}
#endif

/*
 * Driver definition to register with platform structure.
 */
static struct platform_driver ohci_hcd_da8xx_driver = {
	.probe		= ohci_hcd_da8xx_drv_probe,
	.remove		= ohci_hcd_da8xx_drv_remove,
	.shutdown 	= usb_hcd_platform_shutdown,
#ifdef	CONFIG_PM
	.suspend	= ohci_da8xx_suspend,
	.resume		= ohci_da8xx_resume,
#endif
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "ohci",
	},
};
