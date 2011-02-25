/*
 * linux/kernel/time/clockevents.c
 *
 * This file contains functions which manage clock event drivers.
 *
 * Copyright(C) 2005-2006, Thomas Gleixner <tglx@linutronix.de>
 * Copyright(C) 2005-2006, Red Hat, Inc., Ingo Molnar
 *
 * We have two types of clock event devices:
 * - global events (one device per system)
 * - local events (one device per cpu)
 *
 * We assign the various time(r) related interrupts to those devices
 *
 * - global tick
 * - profiling (per cpu)
 * - next timer events (per cpu)
 *
 * TODO:
 * - implement variable frequency profiling
 *
 * This code is licenced under the GPL version 2. For details see
 * kernel-base/COPYING.
 */

#include <linux/clockchips.h>
#include <linux/cpu.h>
#include <linux/irq.h>
#include <linux/init.h>
#include <linux/notifier.h>
#include <linux/module.h>
#include <linux/percpu.h>
#include <linux/profile.h>
#include <linux/sysdev.h>
#include <linux/hrtimer.h>
#include <linux/err.h>

#define MAX_CLOCK_EVENTS	4
#define GLOBAL_CLOCK_EVENT	MAX_CLOCK_EVENTS

struct event_descr {
	struct clock_event_device *event;
	unsigned int mode;
	unsigned int real_caps;
	struct irqaction action;
};

struct local_events {
	int installed;
	struct event_descr events[MAX_CLOCK_EVENTS];
	struct clock_event_device *nextevt;
};

/* Variables related to the global event device */
static __read_mostly struct event_descr global_eventdevice;

/*
 * Lock to protect the above.
 *
 * Only the public management functions have to take this lock. The fast path
 * of the framework, e.g. reprogramming the next event device is lockless as
 * it is per cpu.
 */
static DEFINE_RAW_SPINLOCK(events_lock);

/* Variables related to the per cpu local event devices */
static DEFINE_PER_CPU(struct local_events, local_eventdevices);

/*
 * Math helper. Convert a latch value (device ticks) to nanoseconds
 */
unsigned long clockevent_delta2ns(unsigned long latch,
				  struct clock_event_device *evt)
{
	u64 clc = ((u64) latch << evt->shift);

	do_div(clc, evt->mult);
	if (clc < 1000)
		clc = 1000;
	if (clc > LONG_MAX)
		clc = LONG_MAX;

	return (unsigned long) clc;
}

/*
 * Bootup and lowres handler: ticks only
 */
static void handle_tick(struct pt_regs *regs)
{
	write_seqlock(&xtime_lock);
	do_timer(1);
	write_sequnlock(&xtime_lock);
}

/*
 * Bootup and lowres handler: ticks and update_process_times
 */
static void handle_tick_update(struct pt_regs *regs)
{
	write_seqlock(&xtime_lock);
	do_timer(1);
	write_sequnlock(&xtime_lock);

	update_process_times(user_mode(regs));
}

/*
 * Bootup and lowres handler: ticks and profileing
 */
static void handle_tick_profile(struct pt_regs *regs)
{
	write_seqlock(&xtime_lock);
	do_timer(1);
	write_sequnlock(&xtime_lock);

	profile_tick(CPU_PROFILING, regs);
}

/*
 * Bootup and lowres handler: ticks, update_process_times and profiling
 */
static void handle_tick_update_profile(struct pt_regs *regs)
{
	write_seqlock(&xtime_lock);
	do_timer(1);
	write_sequnlock(&xtime_lock);

	update_process_times(user_mode(regs));
	profile_tick(CPU_PROFILING, regs);
}

/*
 * Bootup and lowres handler: update_process_times
 */
static void handle_update(struct pt_regs *regs)
{
	update_process_times(user_mode(regs));
}

/*
 * Bootup and lowres handler: update_process_times and profiling
 */
static void handle_update_profile(struct pt_regs *regs)
{
	update_process_times(user_mode(regs));
	profile_tick(CPU_PROFILING, regs);
}

/*
 * Bootup and lowres handler: profiling
 */
static void handle_profile(struct pt_regs *regs)
{
	profile_tick(CPU_PROFILING, regs);
}

/*
 * Noop handler when we shut down an event device
 */
static void handle_noop(struct pt_regs *regs)
{
}

/*
 * Lookup table for bootup and lowres event assignment
 *
 * The event handler is choosen by the capability flags of the clock event
 * device.
 */
static void __read_mostly *event_handlers[] = {
	handle_noop,			/* 0: No capability selected */
	handle_tick,			/* 1: Tick only	*/
	handle_update,			/* 2: Update process times */
	handle_tick_update,		/* 3: Tick + update process times */
	handle_profile,			/* 4: Profiling int */
	handle_tick_profile,		/* 5: Tick + Profiling int */
	handle_update_profile,		/* 6: Update process times +
					      profiling */
	handle_tick_update_profile,	/* 7: Tick + update process times +
					      profiling */
#ifdef CONFIG_HIGH_RES_TIMERS
	hrtimer_interrupt,		/* 8: Reprogrammable event device */
#endif
};

/*
 * Start up an event device
 */
static void startup_event(struct clock_event_device *evt, unsigned int caps)
{
	int mode;

	if (caps == CLOCK_CAP_NEXTEVT)
		mode = CLOCK_EVT_ONESHOT;
	else
		mode = CLOCK_EVT_PERIODIC;

	evt->set_mode(mode, evt);
}

/*
 * Setup an event device. Assign an handler and start it up
 */
static void setup_event(struct event_descr *descr,
			struct clock_event_device *evt, unsigned int caps)
{
	void *handler = event_handlers[caps];

	/* Set the event handler */
	evt->event_handler = handler;

	/* Store all relevant information */
	descr->real_caps = caps;

	startup_event(evt, caps);

	printk(KERN_INFO "Clock event device %s configured with caps set: "
	       "%02x\n", evt->name, descr->real_caps);
}

/**
 * register_global_clockevent - register the device which generates
 *			     global clock events
 * @evt:	The device which generates global clock events (ticks)
 *
 * This can be a device which is only necessary for bootup. On UP systems this
 * might be the only event device which is used for everything including
 * high resolution events.
 *
 * When a cpu local event device is installed the global event device is
 * switched off in the high resolution timer / tickless mode.
 */
int __init register_global_clockevent(struct clock_event_device *evt)
{
	/* Already installed? */
	if (global_eventdevice.event) {
		printk(KERN_ERR "Global clock event device already installed: "
		       "%s. Ignoring new global eventsoruce %s\n",
		       global_eventdevice.event->name,
		       evt->name);
		return -EBUSY;
	}

	/* Preset the handler in any case */
	evt->event_handler = handle_noop;

	/*
	 * Check, whether it is a valid global event device
	 */
	if (!(evt->capabilities & CLOCK_BASE_CAPS_MASK)) {
		printk(KERN_ERR "Unsupported clock event device %s\n",
		       evt->name);
		return -EINVAL;
	}

	/*
	 * On UP systems the global clock event device can be used as the next
	 * event device. On SMP this is disabled because the next event device
	 * must be per CPU.
	 */
	if (num_possible_cpus() > 1)
		evt->capabilities &= ~CLOCK_CAP_NEXTEVT;


	/* Mask out high resolution capabilities for now */
	global_eventdevice.event = evt;
	setup_event(&global_eventdevice, evt,
		    evt->capabilities & CLOCK_BASE_CAPS_MASK);
	return 0;
}

/*
 * Mask out the functionality which is covered by the new event device
 * and assign a new event handler.
 */
static void recalc_active_event(struct event_descr *descr,
				unsigned int newcaps)
{
	unsigned int caps;

	if (!descr->real_caps)
		return;

	/* Mask the overlapping bits */
	caps = descr->real_caps & ~newcaps;

	/* Assign the new event handler */
	if (caps) {
		descr->event->event_handler = event_handlers[caps];
		printk(KERN_INFO "Clock event device %s new caps set: %02x\n" ,
		       descr->event->name, caps);
	} else {
		descr->event->event_handler = handle_noop;

		if (descr->event->set_mode)
			descr->event->set_mode(CLOCK_EVT_SHUTDOWN,
					       descr->event);

		printk(KERN_INFO "Clock event device %s disabled\n" ,
		       descr->event->name);
	}
	descr->real_caps = caps;
}

/*
 * Recalc the events and reassign the handlers if necessary
 *
 * Called with event_lock held to protect the global event device.
 */
static int recalc_events(struct local_events *devices,
			 struct clock_event_device *evt, unsigned int caps,
			 int new)
{
	int i;

	if (new && devices->installed == MAX_CLOCK_EVENTS)
		return -ENOSPC;

	/*
	 * If there is no handler and this is not a next-event capable
	 * event device, refuse to handle it
	 */
	if ((!evt->capabilities & CLOCK_CAP_NEXTEVT) && !event_handlers[caps]) {
		printk(KERN_ERR "Unsupported clock event device %s\n",
		       evt->name);
		return -EINVAL;
	}

	if (caps && global_eventdevice.event && global_eventdevice.event != evt)
		recalc_active_event(&global_eventdevice, caps);

	for (i = 0; i < devices->installed; i++) {
		if (devices->events[i].event != evt)
			recalc_active_event(&devices->events[i], caps);
	}

	if (new)
		devices->events[devices->installed++].event = evt;

	if (caps) {
		/* Is next_event event device going to be installed? */
		if (caps & CLOCK_CAP_NEXTEVT)
			caps = CLOCK_CAP_NEXTEVT;

		setup_event(&devices->events[devices->installed],
			    evt, caps);
	} else
		printk(KERN_INFO "Inactive clock event device %s registered\n",
		       evt->name);

	return 0;
}

/**
 * register_local_clockevent - Set up a cpu local clock event device
 * @evt:	event device to be registered
 */
int register_local_clockevent(struct clock_event_device *evt)
{
	struct local_events *devices = &__get_cpu_var(local_eventdevices);
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&events_lock, flags);

	/* Preset the handler in any case */
	evt->event_handler = handle_noop;

	/* Recalc event devices and maybe reassign handlers */
	ret = recalc_events(devices, evt,
			    evt->capabilities & CLOCK_BASE_CAPS_MASK, 1);

	spin_unlock_irqrestore(&events_lock, flags);

	/*
	 * Trigger hrtimers, when the event device is next-event
	 * capable
	 */
	if (!ret && (evt->capabilities & CLOCK_CAP_NEXTEVT))
		hrtimer_clock_notify();

	return ret;
}
EXPORT_SYMBOL_GPL(register_local_clockevent);

/*
 * Find a next-event capable event device
 *
 * Called with event_lock held to protect the global event device.
 */
static int get_next_event_device(void)
{
	struct local_events *devices = &__get_cpu_var(local_eventdevices);
	int i;

	for (i = 0; i < devices->installed; i++) {
		struct clock_event_device *evt;

		evt = devices->events[i].event;
		if (evt->capabilities & CLOCK_CAP_NEXTEVT)
			return i;
	}

	if (global_eventdevice.event->capabilities & CLOCK_CAP_NEXTEVT)
		return GLOBAL_CLOCK_EVENT;

	return -ENODEV;
}

/**
 * clockevents_next_event_available - Check for a installed next-event device
 *
 * Returns 1, when such a device exists, otherwise 0
 */
int clockevents_next_event_available(void)
{
	unsigned long flags;
	int idx;

	spin_lock_irqsave(&events_lock, flags);
	idx = get_next_event_device();
	spin_unlock_irqrestore(&events_lock, flags);

	return IS_ERR_VALUE(idx) ? 0 : 1;
}

/**
 * clockevents_init_next_event - switch to next event (oneshot) mode
 *
 * Switch to one shot mode. On SMP systems the global event (tick) device is
 * switched off. It is replaced by a hrtimer. On UP systems the global event
 * device might be the only one and can be used as the next event device too.
 *
 * Returns 0 on success, otherwise an error code.
 */
int clockevents_init_next_event(void)
{
	struct local_events *devices = &__get_cpu_var(local_eventdevices);
	struct clock_event_device *nextevt;
	unsigned long flags;
	int idx, ret = -ENODEV;

	if (devices->nextevt)
		return -EBUSY;

	spin_lock_irqsave(&events_lock, flags);

	idx = get_next_event_device();
	if (idx < 0)
		goto out_unlock;

	if (idx == GLOBAL_CLOCK_EVENT)
		nextevt = global_eventdevice.event;
	else
		nextevt = devices->events[idx].event;

	ret = recalc_events(devices, nextevt, CLOCK_CAPS_MASK, 0);
	if (!ret)
		devices->nextevt = nextevt;
 out_unlock:
	spin_unlock_irqrestore(&events_lock, flags);

	return ret;
}

/**
 * clockevents_set_next_event - Reprogram the clock event device.
 * @expires:	absolute expiry time (monotonic clock)
 * @force:	when set, enforce reprogramming, even if the event is in the
 *		past
 *
 * Returns 0 on success, -ETIME when the event is in the past and force is not
 * set.
 */
int clockevents_set_next_event(ktime_t expires, int force)
{
	struct local_events *devices = &__get_cpu_var(local_eventdevices);
	int64_t delta = ktime_to_ns(ktime_sub(expires, ktime_get()));
	struct clock_event_device *nextevt = devices->nextevt;
	unsigned long long clc;

	if (delta <= 0 && !force)
		return -ETIME;

	if (delta > (int64_t)nextevt->max_delta_ns)
		delta = nextevt->max_delta_ns;
	if (delta < (int64_t)nextevt->min_delta_ns)
		delta = nextevt->min_delta_ns;

	clc = delta * nextevt->mult;
	clc >>= nextevt->shift;
	nextevt->set_next_event((unsigned long)clc, devices->nextevt);
	hrtimer_trace(expires, clc);

	return 0;
}

/*
 * Resume the cpu local clock events
 */
static void clockevents_resume_local_events(void *arg)
{
	struct local_events *devices = &__get_cpu_var(local_eventdevices);
	int i;

	for (i = 0; i < devices->installed; i++) {
		if (devices->events[i].real_caps)
			startup_event(devices->events[i].event,
				      devices->events[i].real_caps);
	}
	touch_softlockup_watchdog();
}

/**
 * clockevents_resume_events - resume the active clock devices
 *
 * Called after timekeeping is functional again
 */
void clockevents_resume_events(void)
{
	unsigned long flags;

	local_irq_save(flags);

	/* Resume global event device */
	if (global_eventdevice.real_caps)
		startup_event(global_eventdevice.event,
			      global_eventdevice.real_caps);

	local_irq_restore(flags);

	/* Restart the CPU local events everywhere */
	on_each_cpu(clockevents_resume_local_events, NULL, 0, 1);
}

/*
 * Functions related to initialization and hotplug
 */
static int clockevents_cpu_notify(struct notifier_block *self,
				  unsigned long action, void *hcpu)
{
	switch(action) {
	case CPU_UP_PREPARE:
		break;
#ifdef CONFIG_HOTPLUG_CPU
	case CPU_DEAD:
		/*
		 * Do something sensible here !
		 * Disable the cpu local clock event devices ???
		 */
		break;
#endif
	default:
		break;
	}
	return NOTIFY_OK;
}

static struct notifier_block __devinitdata clockevents_nb = {
	.notifier_call	= clockevents_cpu_notify,
};

void __init clockevents_init(void)
{
	clockevents_cpu_notify(&clockevents_nb, (unsigned long)CPU_UP_PREPARE,
				(void *)(long)smp_processor_id());
	register_cpu_notifier(&clockevents_nb);
}
