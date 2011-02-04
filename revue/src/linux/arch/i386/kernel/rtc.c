#include <linux/platform_device.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/mc146818rtc.h>

#if !defined(CONFIG_PNPACPI) && defined(CONFIG_RTC_DRV_CMOS)
static struct resource rtc_resources[] = { 
	[0] = {
		.start		= RTC_PORT(0),
		.end		= RTC_PORT(1),
		.flags		= IORESOURCE_IO,
	},
	[1] = {
		.start		= RTC_IRQ,
		.end		= RTC_IRQ,
		.flags		= IORESOURCE_IRQ,
	}
};

static struct platform_device rtc_device = {
	.name		= "rtc_cmos",
	.id		= -1,
	.resource	= rtc_resources,
	.num_resources	= ARRAY_SIZE(rtc_resources),
};

static __init int add_rtc(void)
{
	int ret;

	ret = platform_device_register(&rtc_device);

	return ret;
}
device_initcall(add_rtc);
#endif
