/*
 *  linux/arch/arm/mach-realview/core.c
 *
 *  Copyright (C) 1999 - 2003 ARM Limited
 *  Copyright (C) 2000 Deep Blue Solutions Ltd
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
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/sysdev.h>
#include <linux/interrupt.h>
#include <linux/amba/bus.h>
#include <linux/amba/clcd.h>
#include <linux/clocksource.h>
#include <linux/clockchips.h>

#include <asm/system.h>
#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/leds.h>
#include <asm/hardware/arm_timer.h>
#include <asm/hardware/icst307.h>

#include <asm/mach/arch.h>
#include <asm/mach/flash.h>
#include <asm/mach/irq.h>
#include <asm/mach/time.h>
#include <asm/mach/map.h>
#include <asm/mach/mmc.h>

#include <asm/hardware/gic.h>

#include "core.h"
#include "clock.h"

#define REALVIEW_REFCOUNTER	(__io_address(REALVIEW_SYS_BASE) + REALVIEW_SYS_24MHz_OFFSET)

/*
 * This is the RealView sched_clock implementation.  This has
 * a resolution of 41.7ns, and a maximum value of about 179s.
 */
unsigned long long sched_clock(void)
{
	unsigned long long v;

	v = (unsigned long long)readl(REALVIEW_REFCOUNTER) * 125;
	do_div(v, 3);

	return v;
}


#define REALVIEW_FLASHCTRL    (__io_address(REALVIEW_SYS_BASE) + REALVIEW_SYS_FLASH_OFFSET)

static int realview_flash_init(void)
{
	u32 val;

	val = __raw_readl(REALVIEW_FLASHCTRL);
	val &= ~REALVIEW_FLASHPROG_FLVPPEN;
	__raw_writel(val, REALVIEW_FLASHCTRL);

	return 0;
}

static void realview_flash_exit(void)
{
	u32 val;

	val = __raw_readl(REALVIEW_FLASHCTRL);
	val &= ~REALVIEW_FLASHPROG_FLVPPEN;
	__raw_writel(val, REALVIEW_FLASHCTRL);
}

static void realview_flash_set_vpp(int on)
{
	u32 val;

	val = __raw_readl(REALVIEW_FLASHCTRL);
	if (on)
		val |= REALVIEW_FLASHPROG_FLVPPEN;
	else
		val &= ~REALVIEW_FLASHPROG_FLVPPEN;
	__raw_writel(val, REALVIEW_FLASHCTRL);
}

static struct flash_platform_data realview_flash_data = {
	.map_name		= "cfi_probe",
	.width			= 4,
	.init			= realview_flash_init,
	.exit			= realview_flash_exit,
	.set_vpp		= realview_flash_set_vpp,
};

static struct resource realview_flash_resource = {
	.start			= REALVIEW_FLASH_BASE,
	.end			= REALVIEW_FLASH_BASE + REALVIEW_FLASH_SIZE,
	.flags			= IORESOURCE_MEM,
};

struct platform_device realview_flash_device = {
	.name			= "armflash",
	.id			= 0,
	.dev			= {
		.platform_data	= &realview_flash_data,
	},
	.num_resources		= 1,
	.resource		= &realview_flash_resource,
};

static struct resource realview_smc91x_resources[] = {
	[0] = {
		.start		= REALVIEW_ETH_BASE,
		.end		= REALVIEW_ETH_BASE + SZ_64K - 1,
		.flags		= IORESOURCE_MEM,
	},
	[1] = {
		.start		= IRQ_ETH,
		.end		= IRQ_ETH,
		.flags		= IORESOURCE_IRQ,
	},
};

struct platform_device realview_smc91x_device = {
	.name		= "smc91x",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(realview_smc91x_resources),
	.resource	= realview_smc91x_resources,
};


static struct resource realview_smsc911x_resources[] = {

#if defined(CONFIG_REALVIEW_PISMO_ETHERNET)
#define REALVIEW_PISMO_BASE	0x20000000
	[0] = {
		.start		= REALVIEW_PISMO_BASE,
		.end		= REALVIEW_PISMO_BASE + SZ_64K - 1,
		.flags		= IORESOURCE_MEM,
	},
	[1] = {
		.start		= 64 + 16,
		.end		= 64 + 16,
		.flags		= IORESOURCE_IRQ,
	},
#else
	[0] = {
		.start		= REALVIEW_ETH_BASE,
		.end		= REALVIEW_ETH_BASE + SZ_64K - 1,
		.flags		= IORESOURCE_MEM,
	},
	[1] = {
		.start		= IRQ_ETH,
		.end		= IRQ_ETH,
		.flags		= IORESOURCE_IRQ,
	},
#endif

};

struct smsc911x_platform_config {
	unsigned int irq_polarity;
	unsigned int irq_type;
};

static struct smsc911x_platform_config smsc911x_config_data = {
	.irq_polarity = 1,
	.irq_type = 1,
};

struct platform_device realview_smsc911x_device = {
	.name		= "smsc911x",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(realview_smsc911x_resources),
	.resource	= realview_smsc911x_resources,
	.dev = {
		.platform_data = &smsc911x_config_data,
	},
};

#if defined(CONFIG_REALVIEW_PISMO_ETHERNET)
/* Sets up pismo interface settings for high performance on smsc911x
 * e.g. 32-bit width, less wait states etc. and this is done differently
 * for each arm platform. So far it only works for pb926 chipselect 3.
 * (7 on pb) and eb chipselect 3 on pismo board. This is another hard-coded
 * hack for chipselect 0 on pb1176.
 */
static int __init pb1176_pismo_setup_cs0(void)
{
	/* We're setting up PISMO chipselect (cs switches: 11 on pismo board)
	 * on smc to run at 32-bit mode with good timings.
	 */
	void __iomem *smc_base;
	int ret = 0;

	/* Mapping SMC memory to modify */
	if (!request_mem_region(REALVIEW_SMC_BASE, SZ_4K, "RealView SMC")) {
		printk(KERN_ERR "%s: Unable to claim SMC memory\n", __FUNCTION__);
		ret = -ENOMEM;
		goto out;
	}

	smc_base = ioremap_nocache(REALVIEW_SMC_BASE, SZ_4K);
	if (!smc_base) {
		printk(KERN_ERR "%s: Cannot remap the SMC memory\n", __FUNCTION__);
		ret = -ENOMEM;
		goto release;
	}

	/* Modify timing and width settings */
	writel(0, smc_base + 0x00);
	writel(4, smc_base + 0x04);
	writel(3, smc_base + 0x08);
	writel(0, smc_base + 0x0C);
	writel(1, smc_base + 0x10);
	writel(0x00303021, smc_base + 0x14);
	writel(0, smc_base + 0x18);
	writel(0, smc_base + 0x1C);

	iounmap(smc_base);
release:
	release_mem_region(REALVIEW_SMC_BASE, SZ_4K);
out:
	return ret;
}
arch_initcall(pb1176_pismo_setup_cs0);
#endif


static struct resource realview_i2c_resource = {
	.start		= REALVIEW_I2C_BASE,
	.end		= REALVIEW_I2C_BASE + SZ_4K - 1,
	.flags		= IORESOURCE_MEM,
};

struct platform_device realview_i2c_device = {
	.name		= "versatile-i2c",
	.id		= -1,
	.num_resources	= 1,
	.resource	= &realview_i2c_resource,
};

#if defined (CONFIG_MACH_REALVIEW_PB11MPC)
#define IRQ_DUMMY		-1
struct resource realview_cf_resources[] = {
	[0] = {
		.start		= REALVIEW_CF_BASE,
		.end		= REALVIEW_CF_BASE + SZ_4K - 1,
		.flags		= IORESOURCE_MEM,
	},
	[1] = {
		.start		= REALVIEW_CF_MEM_BASE,
		.end		= REALVIEW_CF_MEM_BASE + SZ_4K - 1,
		.flags		= IORESOURCE_MEM,
	},
	[2] = {
		.start		= IRQ_DUMMY,	/* FIXME: Find correct irq */
		.end		= IRQ_DUMMY,
		.flags		= IORESOURCE_IRQ,
	},
};

struct platform_device realview_cf_device = {
	.name		= "realview_cf",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(realview_cf_resources),
	.resource	= realview_cf_resources,
};
#endif

#define REALVIEW_SYSMCI	(__io_address(REALVIEW_SYS_BASE) + REALVIEW_SYS_MCI_OFFSET)

static unsigned int realview_mmc_status(struct device *dev)
{
	struct amba_device *adev = container_of(dev, struct amba_device, dev);
	u32 mask;

	if (adev->res.start == REALVIEW_MMCI0_BASE)
		mask = 1;
	else
		mask = 2;

	return readl(REALVIEW_SYSMCI) & mask;
}

struct mmc_platform_data realview_mmc0_plat_data = {
	.ocr_mask	= MMC_VDD_32_33|MMC_VDD_33_34,
	.status		= realview_mmc_status,
};

struct mmc_platform_data realview_mmc1_plat_data = {
	.ocr_mask	= MMC_VDD_32_33|MMC_VDD_33_34,
	.status		= realview_mmc_status,
};

/*
 * Clock handling
 */
static const struct icst307_params realview_oscvco_params = {
	.ref		= 24000,
	.vco_max	= 200000,
	.vd_min		= 4 + 8,
	.vd_max		= 511 + 8,
	.rd_min		= 1 + 2,
	.rd_max		= 127 + 2,
};

static void realview_oscvco_set(struct clk *clk, struct icst307_vco vco)
{
	void __iomem *sys_lock = __io_address(REALVIEW_SYS_BASE) + REALVIEW_SYS_LOCK_OFFSET;
#if defined(CONFIG_MACH_REALVIEW_PB1176)
	void __iomem *sys_osc = __io_address(REALVIEW_SYS_BASE) + REALVIEW_SYS_OSC0_OFFSET;
#else
	void __iomem *sys_osc = __io_address(REALVIEW_SYS_BASE) + REALVIEW_SYS_OSC4_OFFSET;
#endif
	u32 val;

	val = readl(sys_osc) & ~0x7ffff;
	val |= vco.v | (vco.r << 9) | (vco.s << 16);

	writel(0xa05f, sys_lock);
	writel(val, sys_osc);
	writel(0, sys_lock);
}

struct clk realview_clcd_clk = {
	.name	= "CLCDCLK",
	.params	= &realview_oscvco_params,
	.setvco = realview_oscvco_set,
};

/*
 * CLCD support.
 */
#define SYS_CLCD_NLCDIOON	(1 << 2)
#define SYS_CLCD_VDDPOSSWITCH	(1 << 3)
#define SYS_CLCD_PWR3V5SWITCH	(1 << 4)
#define SYS_CLCD_ID_MASK	(0x1f << 8)
#define SYS_CLCD_ID_SANYO_3_8	(0x00 << 8)
#define SYS_CLCD_ID_UNKNOWN_8_4	(0x01 << 8)
#define SYS_CLCD_ID_EPSON_2_2	(0x02 << 8)
#define SYS_CLCD_ID_SANYO_2_5	(0x07 << 8)
#define SYS_CLCD_ID_VGA		(0x1f << 8)

static struct clcd_panel vga = {
	.mode		= {
		.name		= "VGA",
		.refresh	= 60,
#if defined(CONFIG_MACH_REALVIEW_PB11MPC)
		.xres		= 1024,   /* Pixels per line */
		.yres		= 768,    /* Lines per panel */
		.pixclock	= 15748,  /* picoseconds per pixel */
		.left_margin	= 152, /* Horizontal Back Porch */
		.right_margin	= 48, /* Horizontal Front Porch */
		.upper_margin	= 23, /* Vertical Back Porch */
		.lower_margin	= 3, /* Vertical Front Porch */
		.hsync_len	= 104,     /* Horizontal Sync Width */
		.vsync_len	= 4,      /* Vertical Sync Width */
#elif defined(CONFIG_MACH_REALVIEW_PB1176)
		.xres		= 1024,   /* Pixels per line */
		.yres		= 768,    /* Lines per panel */
		.pixclock	= 15748,  /* picoseconds per pixel */
		.left_margin	= 152, /* Horizontal Back Porch */
		.right_margin	= 48, /* Horizontal Front Porch */
		.upper_margin	= 23, /* Vertical Back Porch */
		.lower_margin	= 3, /* Vertical Front Porch */
		.hsync_len	= 104,     /* Horizontal Sync Width */
		.vsync_len	= 4,      /* Vertical Sync Width */
#else
		.xres		= 640,
		.yres		= 480,
		.pixclock	= 39721,
		.left_margin	= 40,
		.right_margin	= 24,
		.upper_margin	= 32,
		.lower_margin	= 11,
		.hsync_len	= 96,
		.vsync_len	= 2,
#endif
		.sync		= 0,
		.vmode		= FB_VMODE_NONINTERLACED,
	},
	.width		= -1,
	.height		= -1,
	.tim2		= TIM2_BCD | TIM2_IPC,
#if defined(CONFIG_MACH_REALVIEW_PB11MPC) || defined(CONFIG_MACH_REALVIEW_PB1176)
	.cntl		= CNTL_LCDTFT | CNTL_LCDVCOMP(1) | CNTL_BGR,
#else
	.cntl		= CNTL_LCDTFT | CNTL_LCDVCOMP(1),
#endif
	.bpp		= 16,
};

static struct clcd_panel sanyo_3_8_in = {
	.mode		= {
		.name		= "Sanyo QVGA",
		.refresh	= 116,
		.xres		= 320,
		.yres		= 240,
		.pixclock	= 100000,
		.left_margin	= 6,
		.right_margin	= 6,
		.upper_margin	= 5,
		.lower_margin	= 5,
		.hsync_len	= 6,
		.vsync_len	= 6,
		.sync		= 0,
		.vmode		= FB_VMODE_NONINTERLACED,
	},
	.width		= -1,
	.height		= -1,
	.tim2		= TIM2_BCD,
	.cntl		= CNTL_LCDTFT | CNTL_LCDVCOMP(1),
	.bpp		= 16,
};

static struct clcd_panel sanyo_2_5_in = {
	.mode		= {
		.name		= "Sanyo QVGA Portrait",
		.refresh	= 116,
		.xres		= 240,
		.yres		= 320,
		.pixclock	= 100000,
		.left_margin	= 20,
		.right_margin	= 10,
		.upper_margin	= 2,
		.lower_margin	= 2,
		.hsync_len	= 10,
		.vsync_len	= 2,
		.sync		= FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
		.vmode		= FB_VMODE_NONINTERLACED,
	},
	.width		= -1,
	.height		= -1,
	.tim2		= TIM2_IVS | TIM2_IHS | TIM2_IPC,
	.cntl		= CNTL_LCDTFT | CNTL_LCDVCOMP(1),
	.bpp		= 16,
};

static struct clcd_panel epson_2_2_in = {
	.mode		= {
		.name		= "Epson QCIF",
		.refresh	= 390,
		.xres		= 176,
		.yres		= 220,
		.pixclock	= 62500,
		.left_margin	= 3,
		.right_margin	= 2,
		.upper_margin	= 1,
		.lower_margin	= 0,
		.hsync_len	= 3,
		.vsync_len	= 2,
		.sync		= 0,
		.vmode		= FB_VMODE_NONINTERLACED,
	},
	.width		= -1,
	.height		= -1,
	.tim2		= TIM2_BCD | TIM2_IPC,
	.cntl		= CNTL_LCDTFT | CNTL_LCDVCOMP(1),
	.bpp		= 16,
};

/*
 * Detect which LCD panel is connected, and return the appropriate
 * clcd_panel structure.  Note: we do not have any information on
 * the required timings for the 8.4in panel, so we presently assume
 * VGA timings.
 */
static struct clcd_panel *realview_clcd_panel(void)
{
	void __iomem *sys_clcd = __io_address(REALVIEW_SYS_BASE) + REALVIEW_SYS_CLCD_OFFSET;
	struct clcd_panel *panel = &vga;
	u32 val;

	val = readl(sys_clcd) & SYS_CLCD_ID_MASK;
	if (val == SYS_CLCD_ID_SANYO_3_8)
		panel = &sanyo_3_8_in;
	else if (val == SYS_CLCD_ID_SANYO_2_5)
		panel = &sanyo_2_5_in;
	else if (val == SYS_CLCD_ID_EPSON_2_2)
		panel = &epson_2_2_in;
	else if (val == SYS_CLCD_ID_VGA)
		panel = &vga;
	else {
		printk(KERN_ERR "CLCD: unknown LCD panel ID 0x%08x, using VGA\n",
			val);
		panel = &vga;
	}

	return panel;
}

/*
 * Disable all display connectors on the interface module.
 */
static void realview_clcd_disable(struct clcd_fb *fb)
{
	void __iomem *sys_clcd = __io_address(REALVIEW_SYS_BASE) + REALVIEW_SYS_CLCD_OFFSET;
	u32 val;

	val = readl(sys_clcd);
	val &= ~SYS_CLCD_NLCDIOON | SYS_CLCD_PWR3V5SWITCH;
	writel(val, sys_clcd);
}

/*
 * Enable the relevant connector on the interface module.
 */
static void realview_clcd_enable(struct clcd_fb *fb)
{
	void __iomem *sys_clcd = __io_address(REALVIEW_SYS_BASE) + REALVIEW_SYS_CLCD_OFFSET;
	u32 val;

	/*
	 * Enable the PSUs
	 */
	val = readl(sys_clcd);
	val |= SYS_CLCD_NLCDIOON | SYS_CLCD_PWR3V5SWITCH;
	writel(val, sys_clcd);
}

#if defined(CONFIG_MACH_REALVIEW_PB11MPC) ||  defined(CONFIG_MACH_REALVIEW_PB1176)
static unsigned long framesize = SZ_1M*3/2;
#else
static unsigned long framesize = SZ_1M;
#endif

static int realview_clcd_setup(struct clcd_fb *fb)
{
	dma_addr_t dma;

	fb->panel		= realview_clcd_panel();

	fb->fb.screen_base = dma_alloc_writecombine(&fb->dev->dev, framesize,
						    &dma, GFP_KERNEL);
	if (!fb->fb.screen_base) {
		printk(KERN_ERR "CLCD: unable to map framebuffer\n");
		return -ENOMEM;
	}

	fb->fb.fix.smem_start	= dma;
	fb->fb.fix.smem_len	= framesize;

	return 0;
}

static int realview_clcd_mmap(struct clcd_fb *fb, struct vm_area_struct *vma)
{
	return dma_mmap_writecombine(&fb->dev->dev, vma,
				     fb->fb.screen_base,
				     fb->fb.fix.smem_start,
				     fb->fb.fix.smem_len);
}

static void realview_clcd_remove(struct clcd_fb *fb)
{
	dma_free_writecombine(&fb->dev->dev, fb->fb.fix.smem_len,
			      fb->fb.screen_base, fb->fb.fix.smem_start);
}

struct clcd_board clcd_plat_data = {
	.name		= "RealView",
	.check		= clcdfb_check,
	.decode		= clcdfb_decode,
	.disable	= realview_clcd_disable,
	.enable		= realview_clcd_enable,
	.setup		= realview_clcd_setup,
	.mmap		= realview_clcd_mmap,
	.remove		= realview_clcd_remove,
};

#ifdef CONFIG_LEDS
#define VA_LEDS_BASE (__io_address(REALVIEW_SYS_BASE) + REALVIEW_SYS_LED_OFFSET)

void realview_leds_event(led_event_t ledevt)
{
	unsigned long flags;
	u32 val;

	local_irq_save(flags);
	val = readl(VA_LEDS_BASE);

	switch (ledevt) {
	case led_idle_start:
		val = val & ~REALVIEW_SYS_LED0;
		break;

	case led_idle_end:
		val = val | REALVIEW_SYS_LED0;
		break;

	case led_timer:
		val = val ^ REALVIEW_SYS_LED1;
		break;

	case led_halted:
		val = 0;
		break;

	default:
		break;
	}

	writel(val, VA_LEDS_BASE);
	local_irq_restore(flags);
}
#endif	/* CONFIG_LEDS */

/*
 * Where is the timer (VA)?
 */
#define TIMER0_VA_BASE		 __io_address(REALVIEW_TIMER0_1_BASE)
#define TIMER1_VA_BASE		(__io_address(REALVIEW_TIMER0_1_BASE) + 0x20)
#define TIMER2_VA_BASE		 __io_address(REALVIEW_TIMER2_3_BASE)
#define TIMER3_VA_BASE		(__io_address(REALVIEW_TIMER2_3_BASE) + 0x20)

/*
 * How long is the timer interval?
 */
#define TIMER_INTERVAL	(TICKS_PER_uSEC * mSEC_10)
#if TIMER_INTERVAL >= 0x100000
#define TIMER_RELOAD	(TIMER_INTERVAL >> 8)
#define TIMER_DIVISOR	(TIMER_CTRL_DIV256)
#define TICKS2USECS(x)	(256 * (x) / TICKS_PER_uSEC)
#elif TIMER_INTERVAL >= 0x10000
#define TIMER_RELOAD	(TIMER_INTERVAL >> 4)		/* Divide by 16 */
#define TIMER_DIVISOR	(TIMER_CTRL_DIV16)
#define TICKS2USECS(x)	(16 * (x) / TICKS_PER_uSEC)
#else
#define TIMER_RELOAD	(TIMER_INTERVAL)
#define TIMER_DIVISOR	(TIMER_CTRL_DIV1)
#define TICKS2USECS(x)	((x) / TICKS_PER_uSEC)
#endif

static void timer_set_mode(enum clock_event_mode mode,
			   struct clock_event_device *clk)
{
	unsigned long ctrl;

	switch (mode) {
	case CLOCK_EVT_PERIODIC:
		writel(TIMER_RELOAD, TIMER0_VA_BASE + TIMER_LOAD);

		ctrl = TIMER_CTRL_PERIODIC;
		ctrl |= TIMER_CTRL_32BIT | TIMER_CTRL_IE | TIMER_CTRL_ENABLE;
		break;
	case CLOCK_EVT_ONESHOT:
		/* period set, and timer enabled in 'next_event' hook */
		ctrl = TIMER_CTRL_ONESHOT;
		ctrl |= TIMER_CTRL_32BIT | TIMER_CTRL_IE;
		break;
	default:
		ctrl = 0;
	}

	writel(ctrl, TIMER0_VA_BASE + TIMER_CTRL);
}

static void timer_set_next_event(unsigned long evt,
				struct clock_event_device *unused)
{
	unsigned long ctrl = readl(TIMER0_VA_BASE + TIMER_CTRL);

	writel(evt, TIMER0_VA_BASE + TIMER_LOAD);
	writel(ctrl | TIMER_CTRL_ENABLE, TIMER0_VA_BASE + TIMER_CTRL);
}

static struct clock_event_device timer0_clockevent =	 {
	.name		= "timer0",
	.shift		= 32,
	.capabilities   = CLOCK_CAP_TICK | CLOCK_CAP_UPDATE |
			CLOCK_CAP_NEXTEVT | CLOCK_CAP_PROFILE,
	.set_mode	= timer_set_mode,
	.set_next_event	= timer_set_next_event,
};

static void __init realview_clockevents_init(void)
{
	timer0_clockevent.mult =
		div_sc(1000000, NSEC_PER_SEC, timer0_clockevent.shift);
	timer0_clockevent.max_delta_ns =
		clockevent_delta2ns(0xffffffff, &timer0_clockevent);
	timer0_clockevent.min_delta_ns =
		clockevent_delta2ns(0xf, &timer0_clockevent);

	register_global_clockevent(&timer0_clockevent);
}

/*
 * IRQ handler for the timer
 */
static irqreturn_t realview_timer_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	struct clock_event_device *evt = &timer0_clockevent;

	// ...clear the interrupt
	writel(1, TIMER0_VA_BASE + TIMER_INTCLR);

	evt->event_handler(regs);

	return IRQ_HANDLED;
}

static struct irqaction realview_timer_irq = {
	.name		= "RealView Timer Tick",
	.flags		= IRQF_DISABLED | IRQF_TIMER,
	.handler	= realview_timer_interrupt,
};

static cycle_t realview_get_cycles(void)
{
	return ~readl(TIMER3_VA_BASE + TIMER_VALUE);
}

static struct clocksource clocksource_realview = {
	.name	= "timer3",
	.rating	= 200,
	.read	= realview_get_cycles,
	.mask	= CLOCKSOURCE_MASK(32),
	.shift	= 20,
	.is_continuous  = 1,
};

static void __init realview_clocksource_init(void)
{
	/* setup timer 0 as free-running clocksource */
	writel(0, TIMER3_VA_BASE + TIMER_CTRL);
	writel(0xffffffff, TIMER3_VA_BASE + TIMER_LOAD);
	writel(0xffffffff, TIMER3_VA_BASE + TIMER_VALUE);
	writel(TIMER_CTRL_32BIT | TIMER_CTRL_ENABLE | TIMER_CTRL_PERIODIC,
		TIMER3_VA_BASE + TIMER_CTRL);

	clocksource_realview.mult =
		clocksource_khz2mult(1000, clocksource_realview.shift);
	clocksource_register(&clocksource_realview);
}

/*
 * Set up timer interrupt, and return the current time in seconds.
 */
static void __init realview_timer_init(void)
{
	u32 val;

	/* 
	 * set clock frequency: 
	 *	REALVIEW_REFCLK is 32KHz
	 *	REALVIEW_TIMCLK is 1MHz
	 */
	val = readl(__io_address(REALVIEW_SCTL_BASE));
	writel((REALVIEW_TIMCLK << REALVIEW_TIMER1_EnSel) |
	       (REALVIEW_TIMCLK << REALVIEW_TIMER2_EnSel) | 
	       (REALVIEW_TIMCLK << REALVIEW_TIMER3_EnSel) |
	       (REALVIEW_TIMCLK << REALVIEW_TIMER4_EnSel) | val,
	       __io_address(REALVIEW_SCTL_BASE));

	/*
	 * Initialise to a known state (all timers off)
	 */
	writel(0, TIMER0_VA_BASE + TIMER_CTRL);
	writel(0, TIMER1_VA_BASE + TIMER_CTRL);
	writel(0, TIMER2_VA_BASE + TIMER_CTRL);
	writel(0, TIMER3_VA_BASE + TIMER_CTRL);

	/* 
	 * Make irqs happen for the system timer
	 */
	setup_irq(IRQ_TIMERINT0_1, &realview_timer_irq);

	realview_clocksource_init();
	realview_clockevents_init();
}

struct sys_timer realview_timer = {
	.init		= realview_timer_init,
};
