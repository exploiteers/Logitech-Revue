/*
 * Setup code for PC-style Real-Time Clock.
 *
 * Author: Wade Farnsworth <wfarnsworth@mvista.com>
 *
 * 2007 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/mc146818rtc.h>

#include <asm/prom.h>

static int  __init add_rtc(void)
{
	struct device_node *np;
	struct platform_device *pd;
	struct resource res;

	np = of_find_compatible_node(NULL, NULL, "pnpPNP,b00");
	if (!np)
		return -ENODEV;

	if (of_address_to_resource(np, 0, &res)) {
		of_node_put(np);
		return -ENODEV;
	}

	/*
	 * RTC_PORT(x) is hardcoded in asm/mc146818rtc.h.  Verify that the
	 * address provided by the device node matches.
	 */
	if (res.start != RTC_PORT(0)) {
		of_node_put(np);
		return -ENODEV;
	}

	pd = platform_device_register_simple("rtc_cmos", -1,
					     &res, 1);
	of_node_put(np);
	if (IS_ERR(pd))
		return PTR_ERR(pd);

	/* setup RTC */
	CMOS_WRITE(RTC_SET, RTC_CONTROL);
	CMOS_WRITE(RTC_24H, RTC_CONTROL);

	/* ensure month, date, and week alarm fields are ignored */
	CMOS_WRITE(0, RTC_VALID);

	outb_p(0x7c, 0x72);
	outb_p(RTC_ALARM_DONT_CARE, 0x73);

	outb_p(0x7d, 0x72);
	outb_p(RTC_ALARM_DONT_CARE, 0x73);

	return 0;
}
fs_initcall(add_rtc);
