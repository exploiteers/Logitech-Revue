/*
 *  linux/arch/x86-64/kernel/time.c
 *
 *  "High Precision Event Timer" based timekeeping.
 *
 *  Copyright (c) 1991,1992,1995  Linus Torvalds
 *  Copyright (c) 1994  Alan Modra
 *  Copyright (c) 1995  Markus Kuhn
 *  Copyright (c) 1996  Ingo Molnar
 *  Copyright (c) 1998  Andrea Arcangeli
 *  Copyright (c) 2002,2006  Vojtech Pavlik
 *  Copyright (c) 2003  Andi Kleen
 *  RTC support code taken from arch/i386/kernel/timers/time_hpet.c
 */

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/mc146818rtc.h>
#include <linux/time.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/sysdev.h>
#include <linux/bcd.h>
#include <linux/kallsyms.h>
#include <linux/acpi.h>
#ifdef CONFIG_ACPI
#include <acpi/achware.h>	/* for PM timer frequency */
#include <acpi/acpi_bus.h>
#endif
#include <asm/8253pit.h>
#include <asm/pgtable.h>
#include <asm/vsyscall.h>
#include <asm/timex.h>
#include <asm/proto.h>
#include <asm/hpet.h>
#include <asm/sections.h>
#include <asm/ltt.h>
#include <linux/cpufreq.h>
#include <linux/hpet.h>
#include <linux/clocksource.h>
#include <linux/clockchips.h>
#ifdef CONFIG_X86_LOCAL_APIC
#include <asm/apic.h>
#endif
#include <asm/delay.h>

extern void i8254_timer_resume(void);
extern int using_apic_timer;
extern struct clock_event_device pit_clockevent;


DEFINE_SPINLOCK(rtc_lock);
EXPORT_SYMBOL(rtc_lock);
DEFINE_RAW_SPINLOCK(i8253_lock);

#define USEC_PER_TICK (USEC_PER_SEC / HZ)
#define NSEC_PER_TICK (NSEC_PER_SEC / HZ)


int report_lost_ticks;				/* command line option */

volatile unsigned long jiffies = INITIAL_JIFFIES;

unsigned long profile_pc(struct pt_regs *regs)
{
	unsigned long pc = instruction_pointer(regs);

	/* Assume the lock function has either no stack frame or only a single 
	   word.  This checks if the address on the stack looks like a kernel 
	   text address.
	   There is a small window for false hits, but in that case the tick
	   is just accounted to the spinlock function.
	   Better would be to write these functions in assembler again
	   and check exactly. */
	if (!user_mode(regs) && in_lock_functions(pc)) {
		char *v = *(char **)regs->rsp;
		if ((v >= _stext && v <= _etext) ||
			(v >= _sinittext && v <= _einittext) ||
			(v >= (char *)MODULES_VADDR  && v <= (char *)MODULES_END))
			return (unsigned long)v;
		return ((unsigned long *)regs->rsp)[1];
	}
	return pc;
}
EXPORT_SYMBOL(profile_pc);

/*
 * In order to set the CMOS clock precisely, set_rtc_mmss has to be called 500
 * ms after the second nowtime has started, because when nowtime is written
 * into the registers of the CMOS clock, it will jump to the next second
 * precisely 500 ms later. Check the Motorola MC146818A or Dallas DS12887 data
 * sheet for details.
 */
static void set_rtc_mmss(unsigned long nowtime)
{
	int real_seconds, real_minutes, cmos_minutes;
	unsigned char control, freq_select;
	unsigned long flags;

	spin_lock_irqsave(&rtc_lock, flags);

/*
 * Tell the clock it's being set and stop it.
 */

	control = CMOS_READ(RTC_CONTROL);
	CMOS_WRITE(control | RTC_SET, RTC_CONTROL);

	freq_select = CMOS_READ(RTC_FREQ_SELECT);
	CMOS_WRITE(freq_select | RTC_DIV_RESET2, RTC_FREQ_SELECT);

	cmos_minutes = CMOS_READ(RTC_MINUTES);
		BCD_TO_BIN(cmos_minutes);

/*
 * since we're only adjusting minutes and seconds, don't interfere with hour
 * overflow. This avoids messing with unknown time zones but requires your RTC
 * not to be off by more than 15 minutes. Since we're calling it only when
 * our clock is externally synchronized using NTP, this shouldn't be a problem.
 */

	real_seconds = nowtime % 60;
	real_minutes = nowtime / 60;
	if (((abs(real_minutes - cmos_minutes) + 15) / 30) & 1)
		real_minutes += 30;		/* correct for half hour time zone */
	real_minutes %= 60;

	if (abs(real_minutes - cmos_minutes) >= 30) {
		printk(KERN_WARNING "time.c: can't update CMOS clock "
		       "from %d to %d\n", cmos_minutes, real_minutes);
	} else {
		BIN_TO_BCD(real_seconds);
		BIN_TO_BCD(real_minutes);
		CMOS_WRITE(real_seconds, RTC_SECONDS);
		CMOS_WRITE(real_minutes, RTC_MINUTES);
	}

/*
 * The following flags have to be released exactly in this order, otherwise the
 * DS12887 (popular MC146818A clone with integrated battery and quartz) will
 * not reset the oscillator and will not update precisely 500 ms later. You
 * won't find this mentioned in the Dallas Semiconductor data sheets, but who
 * believes data sheets anyway ... -- Markus Kuhn
 */

	CMOS_WRITE(control, RTC_CONTROL);
	CMOS_WRITE(freq_select, RTC_FREQ_SELECT);

	spin_unlock_irqrestore(&rtc_lock, flags);
}

static void sync_cmos_clock(unsigned long dummy);

static DEFINE_TIMER(sync_cmos_timer, sync_cmos_clock, 0, 0);

#define USEC_AFTER	50000
#define USEC_BEFORE	50000
#define TICK_SIZE	(tick_nsec/1000)

static void sync_cmos_clock(unsigned long dummy)
{
	struct timeval now, next;
	int fail = 1;

	/*
	 * If we have an externally synchronized Linux clock, then update
	 * CMOS clock accordingly every ~11 minutes. Set_rtc_mmss() has to be
	 * called as close as possible to 500 ms before the new second starts.
	 * This code is run on a timer.  If the clock is set, that timer
	 * may not expire at the correct time.  Thus, we adjust...
	 */
	if (!ntp_synced())
		/*
		 * Not synced, exit, do not restart a timer (if one is
		 * running, let it run out).
		 */
		return;

	do_gettimeofday(&now);
	if (now.tv_usec >= USEC_AFTER - ((unsigned) TICK_SIZE) / 2 &&
	    now.tv_usec <= USEC_BEFORE + ((unsigned) TICK_SIZE) / 2) {
		set_rtc_mmss(now.tv_sec);
		fail = 0;
	}

	next.tv_usec = USEC_AFTER - now.tv_usec;
	if (next.tv_usec <= 0)
		next.tv_usec += USEC_PER_SEC;

	if (!fail)
		next.tv_sec = 659;
	else
		next.tv_sec = 0;

	if (next.tv_usec >= USEC_PER_SEC) {
		next.tv_sec++;
		next.tv_usec -= USEC_PER_SEC;
	}
	mod_timer(&sync_cmos_timer, jiffies + timeval_to_jiffies(&next));
}

void notify_arch_cmos_timer(void)
{
	mod_timer(&sync_cmos_timer, jiffies + 1);
}

void main_timer_handler(struct pt_regs *regs)
{
	pit_clockevent.event_handler(regs);
}

static irqreturn_t timer_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	main_timer_handler(regs);
#ifdef CONFIG_X86_LOCAL_APIC
	if (using_apic_timer)
		smp_send_timer_broadcast_ipi();
#endif
	return IRQ_HANDLED;
}


unsigned long read_persistent_clock(void)
{
	unsigned int year, mon, day, hour, min, sec;
	unsigned long flags;
	unsigned extyear = 0;

	spin_lock_irqsave(&rtc_lock, flags);

	do {
		sec = CMOS_READ(RTC_SECONDS);
		min = CMOS_READ(RTC_MINUTES);
		hour = CMOS_READ(RTC_HOURS);
		day = CMOS_READ(RTC_DAY_OF_MONTH);
		mon = CMOS_READ(RTC_MONTH);
		year = CMOS_READ(RTC_YEAR);
#ifdef CONFIG_ACPI
		if (acpi_fadt.revision >= FADT2_REVISION_ID &&
					acpi_fadt.century)
			extyear = CMOS_READ(acpi_fadt.century);
#endif
	} while (sec != CMOS_READ(RTC_SECONDS));

	spin_unlock_irqrestore(&rtc_lock, flags);

	/*
	 * We know that x86-64 always uses BCD format, no need to check the
	 * config register.
 	 */

	BCD_TO_BIN(sec);
	BCD_TO_BIN(min);
	BCD_TO_BIN(hour);
	BCD_TO_BIN(day);
	BCD_TO_BIN(mon);
	BCD_TO_BIN(year);

	if (extyear) {
		BCD_TO_BIN(extyear);
		year += extyear;
		printk(KERN_INFO "Extended CMOS year: %d\n", extyear);
	} else { 
		/*
		 * x86-64 systems only exists since 2002.
		 * This will work up to Dec 31, 2100
	 	 */
		year += 2000;
	}

	return mktime(year, mon, day, hour, min, sec);
}

/*
 * pit_calibrate_tsc() uses the speaker output (channel 2) of
 * the PIT. This is better than using the timer interrupt output,
 * because we can read the value of the speaker with just one inb(),
 * where we need three i/o operations for the interrupt channel.
 * We count how many ticks the TSC does in 50 ms.
 */

static unsigned int __init pit_calibrate_tsc(void)
{
	unsigned long start, end;
	unsigned long flags;

	spin_lock_irqsave(&i8253_lock, flags);

	outb((inb(0x61) & ~0x02) | 0x01, 0x61);

	outb(0xb0, 0x43);
	outb((PIT_TICK_RATE / (1000 / 50)) & 0xff, 0x42);
	outb((PIT_TICK_RATE / (1000 / 50)) >> 8, 0x42);
	start = get_cycles_sync();
	while ((inb(0x61) & 0x20) == 0);
	end = get_cycles_sync();

	spin_unlock_irqrestore(&i8253_lock, flags);
	
	return (end - start) / 50;
}

#define PIT_MODE 0x43
#define PIT_CH0  0x40

static void __init __pit_init(int val, u8 mode)
{
	unsigned long flags;

	spin_lock_irqsave(&i8253_lock, flags);
	outb_p(mode, PIT_MODE);
	outb_p(val & 0xff, PIT_CH0);	/* LSB */
	outb_p(val >> 8, PIT_CH0);	/* MSB */
	spin_unlock_irqrestore(&i8253_lock, flags);
}

static void init_pit_timer(enum clock_event_mode mode,
			   struct clock_event_device *evt)
{
	unsigned long flags;

	spin_lock_irqsave(&i8253_lock, flags);

	switch(mode) {
	case CLOCK_EVT_PERIODIC:
		/* binary, mode 2, LSB/MSB, ch 0 */
		outb_p(0x34, PIT_MODE);
		udelay(10);
		outb_p(LATCH & 0xff , PIT_CH0); /* LSB */
		outb(LATCH >> 8 , PIT_CH0);     /* MSB */
		break;

	case CLOCK_EVT_ONESHOT:
		/* One shot setup */
		outb_p(0x38, PIT_MODE);
		udelay(10);
		break;
	case CLOCK_EVT_SHUTDOWN:
		outb_p(0x30, PIT_MODE);
		outb_p(0, PIT_CH0);	/* LSB */
		outb_p(0, PIT_CH0);	/* MSB */
		disable_irq(0);
		break;
	}
	spin_unlock_irqrestore(&i8253_lock, flags);
}

static void pit_next_event(unsigned long delta, struct clock_event_device *evt)
{
	unsigned long flags;

	spin_lock_irqsave(&i8253_lock, flags);
	outb_p(delta & 0xff , PIT_CH0); /* LSB */
	outb(delta >> 8 , PIT_CH0);     /* MSB */
	spin_unlock_irqrestore(&i8253_lock, flags);
}

struct clock_event_device pit_clockevent = {
	.name		= "pit",
	.capabilities   = CLOCK_CAP_TICK | CLOCK_CAP_PROFILE | CLOCK_CAP_UPDATE
#ifndef CONFIG_SMP
			| CLOCK_CAP_NEXTEVT
#endif
	,
	.set_mode       = init_pit_timer,
	.set_next_event = pit_next_event,
	.shift		= 32,
};

void setup_pit_timer(void)
{
	pit_clockevent.mult = div_sc(CLOCK_TICK_RATE, NSEC_PER_SEC, 32);
	pit_clockevent.max_delta_ns =
		clockevent_delta2ns(0x7FFF, &pit_clockevent);
	pit_clockevent.min_delta_ns =
		clockevent_delta2ns(0xF, &pit_clockevent);
	register_global_clockevent(&pit_clockevent);
}


void __init pit_init(void)
{
	__pit_init(LATCH, 0x34); /* binary, mode 2, LSB/MSB, ch 0 */
}

void __init pit_stop_interrupt(void)
{
	__pit_init(0, 0x30); /* mode 0 */
}

void __init stop_timer_interrupt(void)
{
	char *name;
	if (hpet_address) {
		name = "HPET";
		hpet_stop();
	} else {
		name = "PIT";
		pit_stop_interrupt();
	}
	printk(KERN_INFO "timer: %s interrupt stopped.\n", name);
}

int __init time_setup(char *str)
{
	report_lost_ticks = 1;
	return 1;
}

static struct irqaction irq0 = {
	timer_interrupt, IRQF_DISABLED | IRQF_NODELAY, CPU_MASK_NONE, "timer", NULL, NULL
};

void __init time_init(void)
{
	char *timename;

	if (nohpet)
		hpet_address = 0;

	if (hpet_arch_init())
		hpet_address = 0;

	setup_pit_timer();

	if (hpet_use_timer) {
		/* set tick_nsec to use the proper rate for HPET */
		tick_nsec = TICK_NSEC_HPET;
		cpu_khz = hpet_calibrate_tsc();
		timename = "HPET";
	} else {
		pit_init();
		cpu_khz = pit_calibrate_tsc();
		timename = "PIT";
	}

	if (unsynchronized_tsc())
		mark_tsc_unstable();

	printk(KERN_INFO "time.c: Detected %d.%03d MHz processor.\n",
		cpu_khz / 1000, cpu_khz % 1000);
	setup_irq(0, &irq0);

	set_cyc2ns_scale(cpu_khz);
}


__setup("report_lost_ticks", time_setup);

/*
 * sysfs support for the timer.
 */
static int timer_resume(struct sys_device *dev)
{
	if (hpet_address)
		hpet_reenable();
	else
		i8254_timer_resume();

	touch_softlockup_watchdog();
	return 0;
}

static struct sysdev_class timer_sysclass = {
	.resume = timer_resume,
	set_kset_name("timer"),
};

/* XXX this driverfs stuff should probably go elsewhere later -john */
static struct sys_device device_timer = {
	.id	= 0,
	.cls	= &timer_sysclass,
};

static int time_init_device(void)
{
	int error = sysdev_class_register(&timer_sysclass);
	if (!error)
		error = sysdev_register(&device_timer);
	return error;
}

device_initcall(time_init_device);

