/*
 * TI Davinci DM365 Real Time Clock interface for Linux
 *
 * Copyright (C) 2008 Texas Instruments
 *
 * Based on drivers/rtc/rtc-omap.c by George G. Davis
 *
 * Copyright (C) 2006 David Brownell (new RTC framework)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/rtc.h>
#include <linux/bcd.h>
#include <linux/platform_device.h>

#include <asm/arch/hardware.h>
#include <linux/io.h>
#include <asm/mach/time.h>

/* The DM365 RTC is a simple RTC with 8kHz clock Count the following
 * Sec: 0 - 59 : BCD count
 * Min: 0 - 59 : BCD count
 * Hour: 0 - 23 : BCD count
 * Day: 0 - 0x7FFF(32767) : Binary count ( Over 89 years )

 * To generate the alarm event with the check the RTC count
 *
 * Free running Watch-dog Timer
 *
 * 16-bit Decrement Timer
 */

#define DM365_RTCIF_PID_REG		0x00
#define DM365_RTCIF_DMA_CMD_REG		0x04
#define DM365_RTCIF_DMA_DATA0_REG	0x08
#define DM365_RTCIF_DMA_DATA1_REG	0x0C
#define DM365_RTCIF_INT_ENA_REG		0x10
#define DM365_RTCIF_INT_FLG_REG		0x14

/* DM365_RTCIF_DMA_CMD_REG bit fields */
#define DM365_RTCIF_DMA_CMD_BUSY		(1<<31)
#define DM365_RTCIF_DMA_CMD_SIZE_2WORD		(1<<25)
#define DM365_RTCIF_DMA_CMD_DIR_READ		(1<<24)
#define DM365_RTCIF_DMA_CMD_BYTEENA1_LSB	(1<<20)
#define DM365_RTCIF_DMA_CMD_BYTEENA1_2ND_LSB	(1<<21)
#define DM365_RTCIF_DMA_CMD_BYTEENA1_3RD_LSB	(1<<22)
#define DM365_RTCIF_DMA_CMD_BYTEENA1_MSB	(1<<23)
#define DM365_RTCIF_DMA_CMD_BYTEENA1_MASK	(0x00F00000)
#define DM365_RTCIF_DMA_CMD_BYTEENA0_LSB	(1<<16)
#define DM365_RTCIF_DMA_CMD_BYTEENA0_2ND_LSB	(1<<17)
#define DM365_RTCIF_DMA_CMD_BYTEENA0_3RD_LSB	(1<<18)
#define DM365_RTCIF_DMA_CMD_BYTEENA0_MSB	(1<<19)
#define DM365_RTCIF_DMA_CMD_BYTEENA0_MASK	(0x000F0000)

#define DM365_RTCIF_INT_ENA_RTCSS_INTENA	(1<<1)
#define DM365_RTCIF_INT_ENA_RTCIF_INTENA	(1<<0)
#define DM365_RTCIF_INT_FLG_RTCSS_INTFLG	(1<<1)
#define DM365_RTCIF_INT_FLG_RTCIF_INTFLG	(1<<0)

#define DM365_RTCIF_INT_FLG_MASK	(DM365_RTCIF_INT_FLG_RTCSS_INTFLG \
					 | DM365_RTCIF_INT_FLG_RTCIF_INTFLG)

/* RTC registers */
#define RTCSS_RTC_BASE			0x10
#define RTCSS_RTC_CTRL_REG		(RTCSS_RTC_BASE + 0x00)
#define RTCSS_RTC_WDT_REG		(RTCSS_RTC_BASE + 0x01)
#define RTCSS_RTC_TMR0_REG		(RTCSS_RTC_BASE + 0x02)
#define RTCSS_RTC_TMR1_REG		(RTCSS_RTC_BASE + 0x03)
#define RTCSS_RTC_CCTRL_REG		(RTCSS_RTC_BASE + 0x04)
#define RTCSS_RTC_SEC_REG		(RTCSS_RTC_BASE + 0x05)
#define RTCSS_RTC_MIN_REG		(RTCSS_RTC_BASE + 0x06)
#define RTCSS_RTC_HOUR_REG		(RTCSS_RTC_BASE + 0x07)
#define RTCSS_RTC_DAY0_REG		(RTCSS_RTC_BASE + 0x08)
#define RTCSS_RTC_DAY1_REG		(RTCSS_RTC_BASE + 0x09)
#define RTCSS_RTC_ALARM_MIN_REG		(RTCSS_RTC_BASE + 0x0A)
#define RTCSS_RTC_ALARM_HOUR_REG	(RTCSS_RTC_BASE + 0x0B)
#define RTCSS_RTC_ALARM_DAY0_REG	(RTCSS_RTC_BASE + 0x0C)
#define RTCSS_RTC_ALARM_DAY1_REG	(RTCSS_RTC_BASE + 0x0D)
#define RTCSS_RTC_CLKC_CNT		(RTCSS_RTC_BASE + 0x10)

/* RTCSS_RTC_CTRL_REG bit fields: */
#define RTCSS_RTC_CTRL_WDTBUS		(1<<7)
#define RTCSS_RTC_CTRL_WEN		(1<<6)
#define RTCSS_RTC_CTRL_WDRT		(1<<5)
#define RTCSS_RTC_CTRL_WDTFLG		(1<<4)
#define RTCSS_RTC_CTRL_TE		(1<<3)
#define RTCSS_RTC_CTRL_TIEN		(1<<2)
#define RTCSS_RTC_CTRL_TMRFLG		(1<<1)
#define RTCSS_RTC_CTRL_TMMD_FREERUN	(1<<1)

/* RTCSS_RTC_CCTRL_REG bit fields: */
#define RTCSS_RTC_CCTRL_CALBUSY	(1<<7)
#define RTCSS_RTC_CCTRL_DAEN	(1<<5)
#define RTCSS_RTC_CCTRL_HAEN	(1<<4)
#define RTCSS_RTC_CCTRL_MAEN	(1<<3)
#define RTCSS_RTC_CCTRL_ALMFLG	(1<<2)
#define RTCSS_RTC_CCTRL_AIEN	(1<<1)
#define RTCSS_RTC_CCTRL_CAEN	(1<<0)

#define rtcif_read(addr)	__raw_readl( \
		(unsigned int *)((u32)dm365_rtc_base + (u32)(addr)))
#define rtcif_write(val, addr)	__raw_writel(val, \
		(unsigned int *)((u32)dm365_rtc_base + (u32)(addr)))

static DEFINE_SPINLOCK(dm365_rtc_lock);

static void __iomem	*dm365_rtc_base;
static resource_size_t	dm365_rtc_pbase;
static size_t		dm365_rtc_base_size;
static int 		dm365_rtc_irq;

/* platform_bus isn't hotpluggable, so for static linkage it'd be safe
 * to get rid of probe() and remove() code ... too bad the driver struct
 * remembers probe(), that's about 25% of the runtime footprint!!
 */
#ifndef	MODULE
#undef	__devexit
#undef	__devexit_p
#define	__devexit	__exit
#define	__devexit_p	__exit_p
#endif

void rtcss_wait_busy(void)
{
	unsigned int count = 0;
	while ((rtcif_read(DM365_RTCIF_DMA_CMD_REG) 
		& DM365_RTCIF_DMA_CMD_BUSY) != 0)
	{
		
		if(count++>20000)
		{
			break;
		}
	}
	if(count>20000)
		printk("rtc wait time out !!!!!\r\n");
	
	
	
}

static int rtcss_write_rtc(unsigned long val, u8 addr)
{
	unsigned int cmd;

//	while (rtcif_read(DM365_RTCIF_DMA_CMD_REG) & DM365_RTCIF_DMA_CMD_BUSY);
	rtcss_wait_busy();

	rtcif_write(DM365_RTCIF_INT_ENA_RTCSS_INTENA |
		    DM365_RTCIF_INT_ENA_RTCIF_INTENA, DM365_RTCIF_INT_FLG_REG);

	cmd = DM365_RTCIF_DMA_CMD_BYTEENA0_LSB | addr;

	rtcif_write(cmd, DM365_RTCIF_DMA_CMD_REG);

	rtcif_write(val, DM365_RTCIF_DMA_DATA0_REG);

//	while (rtcif_read(DM365_RTCIF_DMA_CMD_REG) & DM365_RTCIF_DMA_CMD_BUSY);
	rtcss_wait_busy();

	return rtcif_read(DM365_RTCIF_INT_FLG_REG);
}

static u8 rtcss_read_rtc(u8 addr)
{
	unsigned int cmd;
	u8 regval;

//	while (rtcif_read(DM365_RTCIF_DMA_CMD_REG) & DM365_RTCIF_DMA_CMD_BUSY);
	rtcss_wait_busy();

	cmd = DM365_RTCIF_DMA_CMD_DIR_READ | DM365_RTCIF_DMA_CMD_BYTEENA0_LSB |
		addr;
	rtcif_write(cmd, DM365_RTCIF_DMA_CMD_REG);

	rtcif_write(DM365_RTCIF_INT_ENA_RTCSS_INTENA |
		    DM365_RTCIF_INT_ENA_RTCIF_INTENA, DM365_RTCIF_INT_FLG_REG);

//	while (rtcif_read(DM365_RTCIF_DMA_CMD_REG) & DM365_RTCIF_DMA_CMD_BUSY);
	rtcss_wait_busy();

	regval = rtcif_read(DM365_RTCIF_DMA_DATA0_REG);
	return regval;
}

static irqreturn_t dm365_rtc_isr(int irq, void *class_dev, struct pt_regs *regs)
{
	unsigned long events = 0;
	u32 irq_flg;

	irq_flg = rtcif_read(DM365_RTCIF_INT_FLG_REG);

	if ((irq_flg & DM365_RTCIF_INT_FLG_MASK)) {
		if (irq_flg & DM365_RTCIF_INT_FLG_RTCIF_INTFLG) {
			rtcif_write(DM365_RTCIF_INT_ENA_RTCSS_INTENA |
					DM365_RTCIF_INT_ENA_RTCIF_INTENA,
					DM365_RTCIF_INT_FLG_REG);
		} else	/* must be alarm irq? */
			events |= RTC_IRQF | RTC_AF;

		rtc_update_irq(class_dev, 1, events);
		return IRQ_HANDLED;
	}
	return IRQ_NONE;
}

#ifdef	CONFIG_RTC_INTF_DEV

static int dm365_rtc_update_timer(unsigned int cmd)
{
	u8 rtc_ctrl;

	rtc_ctrl = rtcss_read_rtc(RTCSS_RTC_CTRL_REG);
	switch (cmd) {
	case RTC_UIE_ON:
		while (rtcss_read_rtc(RTCSS_RTC_CTRL_REG)
			& RTCSS_RTC_CTRL_WDTBUS);
		rtc_ctrl |= RTCSS_RTC_CTRL_TE;
		rtcss_write_rtc(rtc_ctrl, RTCSS_RTC_CTRL_REG);
		rtcss_write_rtc(0x0, RTCSS_RTC_CLKC_CNT);
		rtc_ctrl |= RTCSS_RTC_CTRL_TIEN | RTCSS_RTC_CTRL_TMMD_FREERUN;
		rtcss_write_rtc(rtc_ctrl, RTCSS_RTC_CTRL_REG);
		rtcss_write_rtc(0x80, RTCSS_RTC_TMR0_REG);
		rtcss_write_rtc(0x0, RTCSS_RTC_TMR1_REG);
		break;
	case RTC_UIE_OFF:
		rtc_ctrl = rtcss_read_rtc(RTCSS_RTC_CTRL_REG);
		rtc_ctrl &= ~RTCSS_RTC_CTRL_TIEN;
		rtcss_write_rtc(rtc_ctrl, RTCSS_RTC_CTRL_REG);
		break;
	}

	return 0;
}

static int
dm365_rtc_ioctl(struct device *dev, unsigned int cmd, unsigned long arg)
{
	u8 rtc_ctrl;
	unsigned long flags;

	switch (cmd) {
	case RTC_AIE_OFF:
	case RTC_AIE_ON:
	case RTC_UIE_OFF:
	case RTC_UIE_ON:
	case RTC_IRQP_READ:
	case RTC_IRQP_SET:
	case RTC_PIE_OFF:
	case RTC_PIE_ON:
	case RTC_WIE_ON:
	case RTC_WIE_OFF:
		break;
	default:
		return -ENOIOCTLCMD;
	}

	spin_lock_irqsave(&dm365_rtc_lock, flags);
	rtc_ctrl = rtcss_read_rtc(RTCSS_RTC_CCTRL_REG);

	switch (cmd) {
		/* AIE = Alarm Interrupt Enable */
	case RTC_AIE_OFF:
		rtc_ctrl &= ~RTCSS_RTC_CCTRL_AIEN;
		break;
	case RTC_AIE_ON:
		rtc_ctrl |= RTCSS_RTC_CCTRL_AIEN;
		break;
	case RTC_WIE_ON:
		rtc_ctrl |= (RTCSS_RTC_CTRL_WEN | RTCSS_RTC_CTRL_WDTFLG);
		break;
	case RTC_WIE_OFF:
		rtc_ctrl &= ~RTCSS_RTC_CTRL_WEN;
		break;
		/* UIE = Update Interrupt Enable (1/second) */
	case RTC_UIE_OFF:
	case RTC_UIE_ON:
		dm365_rtc_update_timer(cmd);
	}

	rtcss_write_rtc(rtc_ctrl, RTCSS_RTC_CTRL_REG);
	spin_unlock_irqrestore(&dm365_rtc_lock, flags);

	return 0;
}

#else
#define	dm365_rtc_ioctl	NULL
#endif

static int convertfromdays(u16 days, struct rtc_time *tm)
{
	int tmp_days, year, mon;

	for (year = 2000;; year++) {
		tmp_days = rtc_year_days(1, 12, year);
		if (days >= tmp_days)
			days -= tmp_days;
		else {
			for (mon = 0;; mon++) {
				if (days >=
				    (tmp_days = rtc_month_days(mon, year))) {
					days -= tmp_days;
				} else {
					tm->tm_year = year - 1900;
					tm->tm_mon = mon;
					tm->tm_mday = days + 1;
					break;
				}
			}
			break;
		}
	}
	return 0;
}

static int convert2days(u16 *days, struct rtc_time *tm)
{
	int i;
	*days = 0;
	/* epoch == 1900 */
	if (tm->tm_year < 100 || tm->tm_year > 199)
		return -EINVAL;

	for (i = 2000; i < 1900 + tm->tm_year; i++)
		*days += rtc_year_days(1, 12, i);

	*days += rtc_year_days(tm->tm_mday, tm->tm_mon, 1900 + tm->tm_year);

	return 0;
}

static int dm365_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	u16 days = 0;
	u8 day0, day1;
	unsigned long flags;

	spin_lock_irqsave(&dm365_rtc_lock, flags);

	while (rtcss_read_rtc(RTCSS_RTC_CCTRL_REG) & RTCSS_RTC_CCTRL_CALBUSY) ;

	tm->tm_sec = BCD2BIN(rtcss_read_rtc(RTCSS_RTC_SEC_REG));

	while (rtcss_read_rtc(RTCSS_RTC_CCTRL_REG) & RTCSS_RTC_CCTRL_CALBUSY) ;

	tm->tm_min = BCD2BIN(rtcss_read_rtc(RTCSS_RTC_MIN_REG));

	while (rtcss_read_rtc(RTCSS_RTC_CCTRL_REG) & RTCSS_RTC_CCTRL_CALBUSY) ;

	tm->tm_hour = BCD2BIN(rtcss_read_rtc(RTCSS_RTC_HOUR_REG));

	while (rtcss_read_rtc(RTCSS_RTC_CCTRL_REG) & RTCSS_RTC_CCTRL_CALBUSY) ;

	day0 = rtcss_read_rtc(RTCSS_RTC_DAY0_REG);

	while (rtcss_read_rtc(RTCSS_RTC_CCTRL_REG) & RTCSS_RTC_CCTRL_CALBUSY) ;

	day1 = rtcss_read_rtc(RTCSS_RTC_DAY1_REG);

	spin_unlock_irqrestore(&dm365_rtc_lock, flags);

	days |= day1;
	days <<= 8;
	days |= day0;

	if (convertfromdays(days, tm) < 0)
		return -EINVAL;

	return 0;
}

static int dm365_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
	u16 days;
	u8 new_cctrl;
	unsigned long flags;

	if (convert2days(&days, tm) < 0)
		return -EINVAL;

	spin_lock_irqsave(&dm365_rtc_lock, flags);

	while (rtcss_read_rtc(RTCSS_RTC_CCTRL_REG) & RTCSS_RTC_CCTRL_CALBUSY) ;

	rtcss_write_rtc(BIN2BCD(tm->tm_sec), RTCSS_RTC_SEC_REG);

	while (rtcss_read_rtc(RTCSS_RTC_CCTRL_REG) & RTCSS_RTC_CCTRL_CALBUSY) ;

	rtcss_write_rtc(BIN2BCD(tm->tm_min), RTCSS_RTC_MIN_REG);

	while (rtcss_read_rtc(RTCSS_RTC_CCTRL_REG) & RTCSS_RTC_CCTRL_CALBUSY) ;

	rtcss_write_rtc(BIN2BCD(tm->tm_hour), RTCSS_RTC_HOUR_REG);

	while (rtcss_read_rtc(RTCSS_RTC_CCTRL_REG) & RTCSS_RTC_CCTRL_CALBUSY) ;

	rtcss_write_rtc(days & 0xFF, RTCSS_RTC_DAY0_REG);

	while (rtcss_read_rtc(RTCSS_RTC_CCTRL_REG) & RTCSS_RTC_CCTRL_CALBUSY) ;

	rtcss_write_rtc((days & 0xFF00) >> 8, RTCSS_RTC_DAY1_REG);

	new_cctrl = rtcss_read_rtc(RTCSS_RTC_CCTRL_REG);

	new_cctrl |= RTCSS_RTC_CCTRL_CAEN;

	rtcss_write_rtc(new_cctrl, RTCSS_RTC_CCTRL_REG);

	spin_unlock_irqrestore(&dm365_rtc_lock, flags);

	return 0;
}

static int dm365_rtc_read_alarm(struct device *dev, struct rtc_wkalrm *alm)
{
	u16 days = 0;
	u8 day0, day1;
	unsigned long flags;

	spin_lock_irqsave(&dm365_rtc_lock, flags);

	while (rtcss_read_rtc(RTCSS_RTC_CCTRL_REG) & RTCSS_RTC_CCTRL_CALBUSY) ;

	alm->time.tm_min = BCD2BIN(rtcss_read_rtc(RTCSS_RTC_ALARM_MIN_REG));

	while (rtcss_read_rtc(RTCSS_RTC_CCTRL_REG) & RTCSS_RTC_CCTRL_CALBUSY) ;

	alm->time.tm_hour = BCD2BIN(rtcss_read_rtc(RTCSS_RTC_ALARM_HOUR_REG));

	while (rtcss_read_rtc(RTCSS_RTC_CCTRL_REG) & RTCSS_RTC_CCTRL_CALBUSY) ;

	day0 = rtcss_read_rtc(RTCSS_RTC_ALARM_DAY0_REG);

	while (rtcss_read_rtc(RTCSS_RTC_CCTRL_REG) & RTCSS_RTC_CCTRL_CALBUSY) ;

	day1 = rtcss_read_rtc(RTCSS_RTC_ALARM_DAY1_REG);

	spin_unlock_irqrestore(&dm365_rtc_lock, flags);
	days |= day1;
	days <<= 8;
	days |= day0;

	if (convertfromdays(days, &alm->time) < 0)
		return -EINVAL;

	alm->pending = !!(rtcss_read_rtc(RTCSS_RTC_CCTRL_REG) &
			  RTCSS_RTC_CCTRL_AIEN);
	alm->enabled = alm->pending && device_may_wakeup(dev);

	return 0;
}

static int dm365_rtc_set_alarm(struct device *dev, struct rtc_wkalrm *alm)
{
	u16 days;
	u8 new_cctrl;
	unsigned long flags;

	/* Much userspace code uses RTC_ALM_SET, thus "don't care" for
	 * day/month/year specifies alarms up to 24 hours in the future.
	 * So we need to handle that ... but let's ignore the "don't care"
	 * values for hours/minutes/seconds.
	 */
	if (alm->time.tm_mday <= 0 && alm->time.tm_mon < 0
	    && alm->time.tm_year < 0) {
		struct rtc_time tm;
		unsigned long now, then;

		dm365_rtc_read_time(dev, &tm);
		rtc_tm_to_time(&tm, &now);

		alm->time.tm_mday = tm.tm_mday;
		alm->time.tm_mon = tm.tm_mon;
		alm->time.tm_year = tm.tm_year;
		rtc_tm_to_time(&alm->time, &then);

		/* sometimes the alarm wraps into tomorrow */
		if (then < now) {
			rtc_time_to_tm(now + 24 * 60 * 60, &tm);
			alm->time.tm_mday = tm.tm_mday;
			alm->time.tm_mon = tm.tm_mon;
			alm->time.tm_year = tm.tm_year;
		}
	}

	if (convert2days(&days, &alm->time) < 0)
		return -EINVAL;

	spin_lock_irqsave(&dm365_rtc_lock, flags);

	while (rtcss_read_rtc(RTCSS_RTC_CCTRL_REG) & RTCSS_RTC_CCTRL_CALBUSY) ;

	rtcss_write_rtc(BIN2BCD(alm->time.tm_min), RTCSS_RTC_ALARM_MIN_REG);

	while (rtcss_read_rtc(RTCSS_RTC_CCTRL_REG) & RTCSS_RTC_CCTRL_CALBUSY) ;

	rtcss_write_rtc(BIN2BCD(alm->time.tm_hour), RTCSS_RTC_ALARM_HOUR_REG);

	while (rtcss_read_rtc(RTCSS_RTC_CCTRL_REG) & RTCSS_RTC_CCTRL_CALBUSY) ;

	rtcss_write_rtc(days & 0xFF, RTCSS_RTC_ALARM_DAY0_REG);

	while (rtcss_read_rtc(RTCSS_RTC_CCTRL_REG) & RTCSS_RTC_CCTRL_CALBUSY) ;

	rtcss_write_rtc((days & 0xFF00) >> 8, RTCSS_RTC_ALARM_DAY1_REG);

	new_cctrl = rtcss_read_rtc(RTCSS_RTC_CCTRL_REG);

	if (alm->enabled)
		new_cctrl |= (RTCSS_RTC_CCTRL_DAEN |
			      RTCSS_RTC_CCTRL_HAEN |
			      RTCSS_RTC_CCTRL_MAEN |
			      RTCSS_RTC_CCTRL_ALMFLG | RTCSS_RTC_CCTRL_AIEN);
	else
		new_cctrl &= ~RTCSS_RTC_CCTRL_AIEN;

	rtcss_write_rtc(new_cctrl, RTCSS_RTC_CCTRL_REG);

	spin_unlock_irqrestore(&dm365_rtc_lock, flags);

	return 0;
}

static struct rtc_class_ops dm365_rtc_ops = {
#ifdef CONFIG_RTC_INTF_DEV
	.ioctl = dm365_rtc_ioctl,
#endif
	.read_time = dm365_rtc_read_time,
	.set_time = dm365_rtc_set_time,
	.read_alarm = dm365_rtc_read_alarm,
	.set_alarm = dm365_rtc_set_alarm,
};

static int __devinit dm365_rtc_probe(struct platform_device *pdev)
{
	struct resource *res, *mem;
	struct rtc_device *rtc;
	u8 new_ctrl = 0;

	dm365_rtc_irq = platform_get_irq(pdev, 0);
	if (dm365_rtc_irq <= 0) {
		pr_debug("%s: no RTC irq?\n", pdev->name);
		return -ENOENT;
	}

	/* NOTE: using static mapping for RTC registers */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res && res->start != DM365_RTC_BASE) {
		pr_debug("%s: RTC registers at %08x, expected %08x\n",
			 pdev->name, (unsigned)res->start, DM365_RTC_BASE);
		return -ENOENT;
	}

	dm365_rtc_pbase = res->start;
	dm365_rtc_base_size = res->end - res->start + 1;

	if (res)
		mem = request_mem_region(res->start,
					 dm365_rtc_base_size, pdev->name);
	else
		mem = NULL;
	if (!mem) {
		pr_debug("%s: RTC registers at %08x are not free\n",
			 pdev->name, DM365_RTC_BASE);
		return -EBUSY;
	}

	dm365_rtc_base = ioremap(res->start, dm365_rtc_base_size);
	if (dm365_rtc_base == NULL) {
		pr_debug("%s: Can't ioremap MEM resource.\n", pdev->name);
		goto fail;
	}

	rtc =
	    rtc_device_register(pdev->name, &pdev->dev, &dm365_rtc_ops,
				THIS_MODULE);
	if (IS_ERR(rtc)) {
		pr_debug("%s: can't register RTC device, err %ld\n",
			 pdev->name, PTR_ERR(rtc));
		goto fail1;
	}
	platform_set_drvdata(pdev, rtc);
	class_set_devdata(&rtc->class_dev, mem);

	rtcif_write(0, DM365_RTCIF_INT_ENA_REG);
	rtcif_write(0, DM365_RTCIF_INT_FLG_REG);

	rtcss_write_rtc(0, RTCSS_RTC_CTRL_REG);
	rtcss_write_rtc(0, RTCSS_RTC_CCTRL_REG);

	if (request_irq(dm365_rtc_irq, dm365_rtc_isr, SA_INTERRUPT,
			rtc->class_dev.class_id, &rtc->class_dev)) {
		pr_debug("%s: RTC timer interrupt IRQ%d already claimed\n",
			 pdev->name, dm365_rtc_irq);
		goto fail0;
	}

	new_ctrl |= (DM365_RTCIF_INT_ENA_RTCSS_INTENA
		     | DM365_RTCIF_INT_ENA_RTCIF_INTENA);
	rtcif_write(new_ctrl, DM365_RTCIF_INT_ENA_REG);

	while (rtcss_read_rtc(RTCSS_RTC_CCTRL_REG) & RTCSS_RTC_CCTRL_CALBUSY)
		rtcss_read_rtc(RTCSS_RTC_SEC_REG);

	rtcss_write_rtc(RTCSS_RTC_CCTRL_CAEN, RTCSS_RTC_CCTRL_REG);
	device_init_wakeup(&pdev->dev, 0);

	return 0;

fail0:
	rtc_device_unregister(rtc);
fail1:
	iounmap(dm365_rtc_base);
fail:
	release_mem_region(dm365_rtc_pbase, dm365_rtc_base_size);

	return -EIO;
}

static int __devexit dm365_rtc_remove(struct platform_device *pdev)
{
	struct rtc_device *rtc = platform_get_drvdata(pdev);

	device_init_wakeup(&pdev->dev, 0);

	/* leave rtc running, but disable irqs */
	rtcif_write(0, DM365_RTCIF_INT_ENA_REG);

	free_irq(dm365_rtc_irq, rtc);

	release_resource(class_get_devdata(&rtc->class_dev));
	rtc_device_unregister(rtc);
	return 0;
}

MODULE_ALIAS("dm365_rtc");
static struct platform_driver dm365_rtc_driver = {
	.probe 		= dm365_rtc_probe,
	.remove 	= __devexit_p(dm365_rtc_remove),
	.driver 	= {
		.name = "rtc_davinci_dm365",
		.owner = THIS_MODULE,
	},
};

static int __init rtc_init(void)
{
	return platform_driver_register(&dm365_rtc_driver);
}

static void __exit rtc_exit(void)
{
	platform_driver_unregister(&dm365_rtc_driver);
}

module_init(rtc_init);
module_exit(rtc_exit);

MODULE_AUTHOR("Hui Geng");
MODULE_LICENSE("GPL");
