/*
 *  linux/kernel/hrtimer.c
 *
 *  Copyright(C) 2005-2006, Thomas Gleixner <tglx@linutronix.de>
 *  Copyright(C) 2005-2006, Red Hat, Inc., Ingo Molnar
 *  Copyright(C) 2006	    Timesys Corp., Thomas Gleixner <tglx@timesys.com>
 *
 *  High-resolution kernel timers
 *
 *  In contrast to the low-resolution timeout API implemented in
 *  kernel/timer.c, hrtimers provide finer resolution and accuracy
 *  depending on system configuration and capabilities.
 *
 *  These timers are currently used for:
 *   - itimers
 *   - POSIX timers
 *   - nanosleep
 *   - precise in-kernel timing
 *
 *  Started by: Thomas Gleixner and Ingo Molnar
 *
 *  Credits:
 *	based on kernel/timer.c
 *
 *	Help, testing, suggestions, bugfixes, improvements were
 *	provided by:
 *
 *	George Anzinger, Andrew Morton, Steven Rostedt, Roman Zippel
 *	et. al.
 *
 *  For licencing details see kernel-base/COPYING
 */

#include <linux/clockchips.h>
#include <linux/cpu.h>
#include <linux/module.h>
#include <linux/percpu.h>
#include <linux/hrtimer.h>
#include <linux/notifier.h>
#include <linux/syscalls.h>
#include <linux/kallsyms.h>
#include <linux/interrupt.h>
#include <linux/clockchips.h>
#include <linux/profile.h>
#include <linux/seq_file.h>
#include <linux/err.h>
#include <linux/kernel_stat.h>

#include <asm/uaccess.h>

/**
 * ktime_get - get the monotonic time in ktime_t format
 *
 * returns the time in ktime_t format
 */
ktime_t ktime_get(void)
{
	struct timespec now;

	ktime_get_ts(&now);

	return timespec_to_ktime(now);
}

/**
 * ktime_get_real - get the real (wall-) time in ktime_t format
 *
 * returns the time in ktime_t format
 */
ktime_t ktime_get_real(void)
{
	struct timespec now;

	getnstimeofday(&now);

	return timespec_to_ktime(now);
}

EXPORT_SYMBOL_GPL(ktime_get_real);

/*
 * The timer bases:
 *
 * Note: If we want to add new timer bases, we have to skip the two
 * clock ids captured by the cpu-timers. We do this by holding empty
 * entries rather than doing math adjustment of the clock ids.
 * This ensures that we capture erroneous accesses to these clock ids
 * rather than moving them into the range of valid clock id's.
 */
DEFINE_PER_CPU(struct hrtimer_cpu_base, hrtimer_bases) =
{

	.clock_base =
	{
		{
			.index = CLOCK_REALTIME,
			.get_time = &ktime_get_real,
			.resolution = KTIME_LOW_RES,
		},
		{
			.index = CLOCK_MONOTONIC,
			.get_time = &ktime_get,
			.resolution = KTIME_LOW_RES,
		},
	}
};

/**
 * ktime_get_ts - get the monotonic clock in timespec format
 * @ts:		pointer to timespec variable
 *
 * The function calculates the monotonic clock from the realtime
 * clock and the wall_to_monotonic offset and stores the result
 * in normalized timespec format in the variable pointed to by ts.
 */
void ktime_get_ts(struct timespec *ts)
{
	struct timespec tomono;
	unsigned long seq;

	do {
		seq = read_seqbegin(&xtime_lock);
		getnstimeofday(ts);
		tomono = wall_to_monotonic;

	} while (read_seqretry(&xtime_lock, seq));

	set_normalized_timespec(ts, ts->tv_sec + tomono.tv_sec,
				ts->tv_nsec + tomono.tv_nsec);
}
EXPORT_SYMBOL_GPL(ktime_get_ts);

/*
 * Get the coarse grained time at the softirq based on xtime and
 * wall_to_monotonic.
 */
static void hrtimer_get_softirq_time(struct hrtimer_cpu_base *base)
{
	ktime_t xtim, tomono;
	unsigned long seq;

	do {
		seq = read_seqbegin(&xtime_lock);
		xtim = timespec_to_ktime(xtime);
		tomono = timespec_to_ktime(wall_to_monotonic);

	} while (read_seqretry(&xtime_lock, seq));

	base->clock_base[CLOCK_REALTIME].softirq_time = xtim;
	base->clock_base[CLOCK_MONOTONIC].softirq_time =
		ktime_add(xtim, tomono);
}

/*
 * Functions and macros which are different for UP/SMP systems are kept in a
 * single place
 */
#ifdef CONFIG_SMP

/*
 * We are using hashed locking: holding per_cpu(hrtimer_bases)[n].lock
 * means that all timers which are tied to this base via timer->base are
 * locked, and the base itself is locked too.
 *
 * So __run_timers/migrate_timers can safely modify all timers which could
 * be found on the lists/queues.
 *
 * When the timer's base is locked, and the timer removed from list, it is
 * possible to set timer->base = NULL and drop the lock: the timer remains
 * locked.
 */
static
struct hrtimer_clock_base *lock_hrtimer_base(const struct hrtimer *timer,
					     unsigned long *flags)
{
	struct hrtimer_clock_base *base;

	for (;;) {
		base = timer->base;
		if (likely(base != NULL)) {
			spin_lock_irqsave(&base->cpu_base->lock, *flags);
			if (likely(base == timer->base))
				return base;
			/* The timer has migrated to another CPU: */
			spin_unlock_irqrestore(&base->cpu_base->lock, *flags);
		}
		cpu_relax();
	}
}

/*
 * Switch the timer base to the current CPU when possible.
 */
static inline struct hrtimer_clock_base *
switch_hrtimer_base(struct hrtimer *timer, struct hrtimer_clock_base *base)
{
	struct hrtimer_clock_base *new_base;
	struct hrtimer_cpu_base *new_cpu_base;

	new_cpu_base = &__get_cpu_var(hrtimer_bases);
	new_base = &new_cpu_base->clock_base[base->index];

	if (base != new_base) {
		/*
		 * We are trying to schedule the timer on the local CPU.
		 * However we can't change timer's base while it is running,
		 * so we keep it on the same CPU. No hassle vs. reprogramming
		 * the event source in the high resolution case. The softirq
		 * code will take care of this when the timer function has
		 * completed. There is no conflict as we hold the lock until
		 * the timer is enqueued.
		 */
		if (unlikely(timer->state & HRTIMER_STATE_CALLBACK))
			return base;

		/* See the comment in lock_timer_base() */
		timer->base = NULL;
		spin_unlock(&base->cpu_base->lock);
		spin_lock(&new_base->cpu_base->lock);
		timer->base = new_base;
	}
	return new_base;
}

#else /* CONFIG_SMP */

static inline struct hrtimer_clock_base *
lock_hrtimer_base(const struct hrtimer *timer, unsigned long *flags)
{
	struct hrtimer_clock_base *base = timer->base;

	spin_lock_irqsave(&base->cpu_base->lock, *flags);

	return base;
}

# define switch_hrtimer_base(t, b)	(b)

#endif	/* !CONFIG_SMP */

/*
 * Functions for the union type storage format of ktime_t which are
 * too large for inlining:
 */
#if BITS_PER_LONG < 64
# ifndef CONFIG_KTIME_SCALAR
/**
 * ktime_add_ns - Add a scalar nanoseconds value to a ktime_t variable
 * @kt:		addend
 * @nsec:	the scalar nsec value to add
 *
 * Returns the sum of kt and nsec in ktime_t format
 */
ktime_t ktime_add_ns(const ktime_t kt, u64 nsec)
{
	ktime_t tmp;

	if (likely(nsec < NSEC_PER_SEC)) {
		tmp.tv64 = nsec;
	} else {
		unsigned long rem = do_div(nsec, NSEC_PER_SEC);

		tmp = ktime_set((long)nsec, rem);
	}

	return ktime_add(kt, tmp);
}
# endif /* !CONFIG_KTIME_SCALAR */

/*
 * Divide a ktime value by a nanosecond value
 */
static unsigned long ktime_divns(const ktime_t kt, s64 div)
{
	u64 dclc, inc, dns;
	int sft = 0;

	dclc = dns = ktime_to_ns(kt);
	inc = div;
	/* Make sure the divisor is less than 2^32: */
	while (div >> 32) {
		sft++;
		div >>= 1;
	}
	dclc >>= sft;
	do_div(dclc, (unsigned long) div);

	return (unsigned long) dclc;
}

#else /* BITS_PER_LONG < 64 */
# define ktime_divns(kt, div)		(unsigned long)((kt).tv64 / (div))
#endif /* BITS_PER_LONG >= 64 */

/* High resolution timer related functions */
#ifdef CONFIG_HIGH_RES_TIMERS

/*
 * High resolution timer enabled ?
 */
static int hrtimer_hres_enabled __read_mostly  = 1;

/*
 * Enable / Disable high resolution mode
 */
static int __init setup_hrtimer_hres(char *str)
{
	if (!strcmp(str, "off"))
		hrtimer_hres_enabled = 0;
	else if (!strcmp(str, "on"))
		hrtimer_hres_enabled = 1;
	else
		return 0;
	return 1;
}

__setup("highres=", setup_hrtimer_hres);

/*
 * Is the high resolution mode active ?
 */
static inline int hrtimer_hres_active(void)
{
	return __get_cpu_var(hrtimer_bases).hres_active;
}

/*
 * The time, when the last jiffy update happened. Protected by xtime_lock.
 */
static ktime_t last_jiffies_update;

/*
 * Reprogram the event source with checking both queues for the
 * next event
 * Called with interrupts disabled and base->lock held
 */
static void hrtimer_force_reprogram(struct hrtimer_cpu_base *cpu_base)
{
	int i;
	struct hrtimer_clock_base *base = cpu_base->clock_base;
	ktime_t expires;

	cpu_base->expires_next.tv64 = KTIME_MAX;

	for (i = 0; i < HRTIMER_MAX_CLOCK_BASES; i++, base++) {
		struct hrtimer *timer;

		if (!base->first)
			continue;
		timer = rb_entry(base->first, struct hrtimer, node);
		expires = ktime_sub(timer->expires, base->offset);
		if (expires.tv64 < cpu_base->expires_next.tv64)
			cpu_base->expires_next = expires;
	}

	if (cpu_base->expires_next.tv64 != KTIME_MAX)
		clockevents_set_next_event(cpu_base->expires_next, 1);
}

/*
 * Shared reprogramming for clock_realtime and clock_monotonic
 *
 * When a timer is enqueued and expires earlier than the already enqueued
 * timers, we have to check, whether it expires earlier than the timer for
 * which the clock event device was armed.
 *
 * Called with interrupts disabled and base->cpu_base.lock held
 */
static int hrtimer_reprogram(struct hrtimer *timer,
			     struct hrtimer_clock_base *base)
{
	ktime_t *expires_next = &__get_cpu_var(hrtimer_bases).expires_next;
	ktime_t expires = ktime_sub(timer->expires, base->offset);
	int res;

	/*
	 * When the callback is running, we do not reprogram the clock event
	 * device. The timer callback is either running on a different CPU or
	 * the callback is executed in the hrtimer_interupt context. The
	 * reprogramming is handled either by the softirq, which called the
	 * callback or at the end of the hrtimer_interrupt.
	 */
	if (timer->state & HRTIMER_STATE_CALLBACK)
		return 0;

	if (expires.tv64 >= expires_next->tv64)
		return 0;

	/*
	 * Clockevents returns -ETIME, when the event was in the past.
	 */
	res = clockevents_set_next_event(expires, 0);
	if (!IS_ERR_VALUE(res))
		*expires_next = expires;
	return res;
}


/*
 * Retrigger next event is called after clock was set
 *
 * Called with interrupts disabled via on_each_cpu()
 */
static void retrigger_next_event(void *arg)
{
	struct hrtimer_cpu_base *base;
	struct timespec realtime_offset;
	unsigned long seq;

	if (!hrtimer_hres_active())
		return;

	do {
		seq = read_seqbegin(&xtime_lock);
		set_normalized_timespec(&realtime_offset,
					-wall_to_monotonic.tv_sec,
					-wall_to_monotonic.tv_nsec);
	} while (read_seqretry(&xtime_lock, seq));

	base = &__get_cpu_var(hrtimer_bases);

	/* Adjust CLOCK_REALTIME offset */
	spin_lock(&base->lock);
	base->clock_base[CLOCK_REALTIME].offset =
		timespec_to_ktime(realtime_offset);

	hrtimer_force_reprogram(base);
	spin_unlock(&base->lock);
}

/*
 * Clock realtime was set
 *
 * Change the offset of the realtime clock vs. the monotonic
 * clock.
 *
 * We might have to reprogram the high resolution timer interrupt. On
 * SMP we call the architecture specific code to retrigger _all_ high
 * resolution timer interrupts. On UP we just disable interrupts and
 * call the high resolution interrupt code.
 */
void clock_was_set(void)
{
	/* Retrigger the CPU local events everywhere */
	on_each_cpu(retrigger_next_event, NULL, 0, 1);
}

/**
 * hrtimer_clock_notify - A clock source or a clock event has been installed
 *
 * Notify the per cpu softirqs to recheck the clock sources and events
 */
void hrtimer_clock_notify(void)
{
	int i;

	if (hrtimer_hres_enabled) {
		for_each_possible_cpu(i)
			set_bit(0, &per_cpu(hrtimer_bases, i).check_clocks);
	}
}

static const ktime_t nsec_per_hz = { .tv64 = NSEC_PER_SEC / HZ };

/*
 * We switched off the global tick source when switching to high resolution
 * mode. Update jiffies64.
 *
 * Must be called with interrupts disabled !
 *
 * FIXME: We need a mechanism to assign the update to a CPU. In principle this
 * is not hard, but when dynamic ticks come into play it starts to be. We don't
 * want to wake up a complete idle cpu just to update jiffies, so we need
 * something more intellegent than a mere "do this only on CPUx".
 */
static void update_jiffies64(ktime_t now)
{
	unsigned long seq;
	ktime_t delta;
	unsigned long ticks = 0;

	/* Preevaluate to avoid lock contention */
	do {
		seq = read_seqbegin(&xtime_lock);
		delta = ktime_sub(now, last_jiffies_update);
	} while (read_seqretry(&xtime_lock, seq));

	if (delta.tv64 < nsec_per_hz.tv64)
		return;

	/* Reevalute with xtime_lock held */
	write_seqlock(&xtime_lock);

	delta = ktime_sub(now, last_jiffies_update);
	if (delta.tv64 >= nsec_per_hz.tv64) {

		delta = ktime_sub(delta, nsec_per_hz);
		last_jiffies_update = ktime_add(last_jiffies_update,
						nsec_per_hz);

		/* Slow path for long timeouts */
		if (unlikely(delta.tv64 >= nsec_per_hz.tv64)) {
			s64 incr = ktime_to_ns(nsec_per_hz);

			ticks = ktime_divns(delta, incr);

			last_jiffies_update = ktime_add_ns(last_jiffies_update,
							   incr * ticks);
		}
		ticks++;
		do_timer(ticks);
	}
	write_sequnlock(&xtime_lock);
}

#ifdef CONFIG_NO_HZ
/**
 * hrtimer_update_jiffies - update jiffies when idle was interrupted
 *
 * Called from interrupt entry when the CPU was idle
 *
 * In case the sched_tick was stopped on this CPU, we have to check if jiffies
 * must be updated. Otherwise an interrupt handler could use a stale jiffy
 * value.
 */
void hrtimer_update_jiffies(void)
{
	struct hrtimer_cpu_base *cpu_base = &__get_cpu_var(hrtimer_bases);
	unsigned long flags;
	ktime_t now;

	if (!cpu_base->tick_stopped || !cpu_base->hres_active)
		return;

	now = ktime_get();

	local_irq_save(flags);
	update_jiffies64(now);
	local_irq_restore(flags);
}

/**
 * hrtimer_stop_sched_tick - stop the idle tick from the idle task
 *
 * When the next event is more than a tick into the future, stop the idle tick
 * Called either from the idle loop or from irq_exit() when a idle period was
 * just interrupted by a interrupt which did not cause a reschedule.
 */
void hrtimer_stop_sched_tick(void)
{
	struct hrtimer_cpu_base *cpu_base = &__get_cpu_var(hrtimer_bases);
	unsigned long seq, last_jiffies, next_jiffies;
	ktime_t last_update, expires, now;
	unsigned long delta_jiffies;
	unsigned long flags;

	if (unlikely(!cpu_base->hres_active))
		return;

	local_irq_save(flags);

	if (need_resched() || need_resched_delayed())
		goto out;

	now = ktime_get();
	/*
	 * When called from irq_exit we need to account the idle sleep time
	 * correctly.
	 */
	if (cpu_base->tick_stopped) {
		ktime_t delta = ktime_sub(now, cpu_base->idle_entrytime);

		cpu_base->idle_sleeptime = ktime_add(cpu_base->idle_sleeptime,
						     delta);
	}

	cpu_base->idle_entrytime = now;
	cpu_base->idle_calls++;

	/* Read jiffies and the time when jiffies were updated last */
	do {
		seq = read_seqbegin(&xtime_lock);
		last_update = last_jiffies_update;
		last_jiffies = jiffies;
	} while (read_seqretry(&xtime_lock, seq));

	/* Get the next timer wheel timer */
	next_jiffies = get_next_timer_interrupt(last_jiffies);
	delta_jiffies = next_jiffies - last_jiffies;

	if ((long)delta_jiffies >= 1) {
		/*
		 * hrtimer_stop_sched_tick can be called several times before
		 * the hrtimer_restart_sched_tick is called. This happens when
		 * interrupts arrive which do not cause a reschedule. In the
		 * first call we save the current tick time, so we can restart
		 * the scheduler tick in hrtimer_restart_sched_tick.
		 */
		if (!cpu_base->tick_stopped) {
			cpu_base->idle_tick = cpu_base->sched_timer.expires;
			cpu_base->tick_stopped = 1;
			cpu_base->idle_jiffies = last_jiffies;
		}
		/* calculate the expiry time for the next timer wheel timer */
		expires = ktime_add_ns(last_update,
				       nsec_per_hz.tv64 * delta_jiffies);
		hrtimer_start(&cpu_base->sched_timer, expires,
			      HRTIMER_MODE_ABS);
		cpu_base->idle_sleeps++;
	} else {
		/* Raise the softirq if the timer wheel is behind jiffies */
		if ((long) delta_jiffies < 0)
			raise_softirq_irqoff(TIMER_SOFTIRQ);
	}
 out:
	local_irq_restore(flags);
}

/**
 * hrtimer_restart_sched_tick - restart the idle tick from the idle task
 *
 * Restart the idle tick when the CPU is woken up from idle
 */
void hrtimer_restart_sched_tick(void)
{
	struct hrtimer_cpu_base *cpu_base = &__get_cpu_var(hrtimer_bases);
	unsigned long ticks;
	ktime_t now, delta;

	if (!cpu_base->hres_active || !cpu_base->tick_stopped)
		return;

	/* Update jiffies first */
	now = ktime_get();
	hrtimer_trace(now, 0);

	local_irq_disable();
	update_jiffies64(now);

	/* Account the idle time */
	delta = ktime_sub(now, cpu_base->idle_entrytime);
	cpu_base->idle_sleeptime = ktime_add(cpu_base->idle_sleeptime, delta);

	/*
	 * We stopped the tick in idle. Update process times would miss the
	 * time we slept as update_process_times does only a 1 tick
	 * accounting. Enforce that this is accounted to idle !
	 */
	ticks = jiffies - cpu_base->idle_jiffies;
	/*
	 * We might be one off. Do not randomly account a huge number of ticks!
	 */
	if (ticks && ticks < LONG_MAX) {
		add_preempt_count(HARDIRQ_OFFSET);
		account_system_time(current, HARDIRQ_OFFSET,
				    jiffies_to_cputime(ticks));
		sub_preempt_count(HARDIRQ_OFFSET);
	}

	/*
	 * Cancel the scheduled timer and restore the tick
	 */
	cpu_base->tick_stopped  = 0;
	hrtimer_cancel(&cpu_base->sched_timer);
	cpu_base->sched_timer.expires = cpu_base->idle_tick;

	while (1) {
		/* Forward the time to expire in the future */
		hrtimer_forward(&cpu_base->sched_timer, now, nsec_per_hz);
		hrtimer_start(&cpu_base->sched_timer,
			      cpu_base->sched_timer.expires, HRTIMER_MODE_ABS);

		/* Check, if the timer was already in the past */
		if (hrtimer_active(&cpu_base->sched_timer))
			break;
		/* Update jiffies and reread time */
		update_jiffies64(now);
		now = ktime_get();
	}
	local_irq_enable();
}

/**
 * show_no_hz_stats - print out the no hz statistics
 *
 * The no_hz statistics are appended at the end of /proc/stats
 *
 * I: total number of idle calls
 * S: number of idle calls which stopped the sched tick
 * T: Summed up sleep time in idle with sched tick stopped (unit is seconds)
 * A: Average sleep time: T/S (unit is seconds)
 * E: Total number of timer interrupt events
 */
void show_no_hz_stats(struct seq_file *p)
{
	unsigned long calls = 0, sleeps = 0, events = 0;
	struct timeval tsum, tavg;
	ktime_t totaltime = { .tv64 = 0 };
	int cpu;

	for_each_online_cpu(cpu) {
		struct hrtimer_cpu_base *base = &per_cpu(hrtimer_bases, cpu);

		calls += base->idle_calls;
		sleeps += base->idle_sleeps;
		totaltime = ktime_add(totaltime, base->idle_sleeptime);
		events += base->nr_events;

#ifdef CONFIG_SMP
		tsum = ktime_to_timeval(base->idle_sleeptime);
		if (base->idle_sleeps) {
			uint64_t nsec = ktime_to_ns(base->idle_sleeptime);

			do_div(nsec, base->idle_sleeps);
			tavg = ns_to_timeval(nsec);
		} else
			tavg.tv_sec = tavg.tv_usec = 0;

		seq_printf(p, "nohz cpu%d I:%lu S:%lu T:%d.%06d A:%d.%06d E: %lu\n",
			   cpu, base->idle_calls, base->idle_sleeps,
			   (int) tsum.tv_sec, (int) tsum.tv_usec,
			   (int) tavg.tv_sec, (int) tavg.tv_usec,
			   base->nr_events);
#endif
	}

	tsum = ktime_to_timeval(totaltime);
	if (sleeps) {
		uint64_t nsec = ktime_to_ns(totaltime);

			do_div(nsec, sleeps);
			tavg = ns_to_timeval(nsec);
	} else
		tavg.tv_sec = tavg.tv_usec = 0;

	seq_printf(p, "nohz total I:%lu S:%lu T:%d.%06d A:%d.%06d E: %lu\n",
		   calls, sleeps,
		   (int) tsum.tv_sec, (int) tsum.tv_usec,
		   (int) tavg.tv_sec, (int) tavg.tv_usec,
		   events);
}

#endif

/*
 * We rearm the timer until we get disabled by the idle code
 * Called with interrupts disabled.
 */
static enum hrtimer_restart hrtimer_sched_tick(struct hrtimer *timer)
{
	struct hrtimer_cpu_base *cpu_base =
		container_of(timer, struct hrtimer_cpu_base, sched_timer);
	ktime_t now = ktime_get();

	/* Check, if the jiffies need an update */
	update_jiffies64(now);

	/*
	 * Do not call, when we are not in irq context and have
	 * no valid regs pointer
	 */
	if (cpu_base->sched_regs) {
#ifdef CONFIG_NO_HZ
		/*
		 * When we are idle and the tick is stopped, we have to touch
		 * the watchdog as we might not schedule for a really long
		 * time. This happens on complete idle SMP systems while
		 * waiting on the login prompt. We also increment the "start of
		 * idle" jiffy stamp so the idle accounting adjustment we do
		 * when we go busy again does not account too much ticks.
		 */
		if (cpu_base->tick_stopped) {
			touch_softlockup_watchdog();
			cpu_base->idle_jiffies++;
		}
#endif
		/*
		 * update_process_times() might take tasklist_lock, hence
		 * drop the base lock. sched-tick hrtimers are per-CPU and
		 * never accessible by userspace APIs, so this is safe to do.
		 */
		spin_unlock(&cpu_base->lock);
		update_process_times(user_mode(cpu_base->sched_regs));
		profile_tick(CPU_PROFILING, cpu_base->sched_regs);
		spin_lock(&cpu_base->lock);
	}

#ifdef CONFIG_NO_HZ
	/* Do not restart, when we are in the idle loop */
	if (cpu_base->tick_stopped)
		return HRTIMER_NORESTART;
#endif

	hrtimer_forward(timer, now, nsec_per_hz);

	return HRTIMER_RESTART;
}

/*
 * A change in the clock source or clock events was detected.
 * Check the clock source and the events, whether we can switch to
 * high resolution mode or not.
 *
 * TODO: Handle the removal of clock sources / events
 */
static void hrtimer_check_clocks(void)
{
	struct hrtimer_cpu_base *base = &__get_cpu_var(hrtimer_bases);
	unsigned long flags;
	ktime_t now;

	if (!test_and_clear_bit(0, &base->check_clocks))
		return;

	if (!timekeeping_is_continuous())
		return;

	if (!clockevents_next_event_available())
		return;

	local_irq_save(flags);

	if (base->hres_active) {
		local_irq_restore(flags);
		return;
	}

	now = ktime_get();
	if (clockevents_init_next_event()) {
		local_irq_restore(flags);
		return;
	}
	base->hres_active = 1;
	base->clock_base[CLOCK_REALTIME].resolution = KTIME_HIGH_RES;
	base->clock_base[CLOCK_MONOTONIC].resolution = KTIME_HIGH_RES;

	/* Did we start the jiffies update yet ? */
	if (last_jiffies_update.tv64 == 0) {
		write_seqlock(&xtime_lock);
		last_jiffies_update = now;
		write_sequnlock(&xtime_lock);
	}

	/*
	 * Emulate tick processing via per-CPU hrtimers:
	 */
	hrtimer_init(&base->sched_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	base->sched_timer.function = hrtimer_sched_tick;
	base->sched_timer.cb_mode = HRTIMER_CB_IRQSAFE_NO_SOFTIRQ;
	hrtimer_start(&base->sched_timer, nsec_per_hz, HRTIMER_MODE_REL);

	/* "Retrigger" the interrupt to get things going */
	retrigger_next_event(NULL);
	local_irq_restore(flags);
	printk(KERN_INFO "Switched to high resolution mode on CPU %d\n",
	       smp_processor_id());
}

#ifndef CONFIG_PREEMPT_RT

static inline int hrtimer_base_cb_pending(const struct hrtimer_cpu_base *base)
{
	return !list_empty(&base->cb_pending);
}

static inline
struct hrtimer *hrtimer_base_get_cb_pending(struct hrtimer_cpu_base *base)
{
	return list_entry(base->cb_pending.next, struct hrtimer, cb_entry);
}

/*
 * Remove a timer from the callback pending list
 */
static inline void hrtimer_remove_cb_pending(struct hrtimer *timer)
{
	list_del_init(&timer->cb_entry);
}

static inline void hrtimer_add_cb_pending(struct hrtimer *timer,
					  struct hrtimer_clock_base *base)
{
	list_add(&timer->cb_entry, &base->cpu_base->cb_pending);
}

static inline void hrtimer_init_base_cb_pending(struct hrtimer_cpu_base *base)
{
	INIT_LIST_HEAD(&base->cb_pending);
}

/*
 * Initialize the high resolution related parts of a hrtimer
 */
static inline void hrtimer_init_timer_hres(struct hrtimer *timer)
{
	INIT_LIST_HEAD(&timer->cb_entry);
}

static inline void hrtimer_update_timer_prio(struct hrtimer *timer)
{
}

static inline void hrtimer_raise_softirq(struct hrtimer_cpu_base *base)
{
	raise_softirq_irqoff(HRTIMER_SOFTIRQ);
}

static inline void hrtimer_adjust_softirq_prio(struct hrtimer_cpu_base *base)
{
}

#else

static inline int hrtimer_base_cb_pending(const struct hrtimer_cpu_base *base)
{
	return !plist_head_empty(&base->cb_pending);
}

static inline
struct hrtimer *hrtimer_base_get_cb_pending(struct hrtimer_cpu_base *base)
{
	return plist_first_entry(&base->cb_pending, struct hrtimer, cb_entry);
}

static inline void hrtimer_remove_cb_pending(struct hrtimer *timer)
{
	plist_del(&timer->cb_entry, &timer->base->cpu_base->cb_pending);
}

static inline void hrtimer_add_cb_pending(struct hrtimer *timer,
					  struct hrtimer_clock_base *base)
{
	plist_add(&timer->cb_entry, &base->cpu_base->cb_pending);
}

static inline void hrtimer_init_timer_hres(struct hrtimer *timer)
{
	plist_node_init(&timer->cb_entry, current->normal_prio);
}

static inline void hrtimer_init_base_cb_pending(struct hrtimer_cpu_base *base)
{
	plist_head_init(&base->cb_pending, &base->lock);
}

static inline void hrtimer_update_timer_prio(struct hrtimer *timer)
{
	timer->cb_entry.prio = current->normal_prio;
}

static inline int hrtimer_get_softirq_prio(struct hrtimer_cpu_base *base)
{
	if (hrtimer_base_cb_pending(base)) {
		struct hrtimer *timer = hrtimer_base_get_cb_pending(base);

		if (rt_prio(timer->cb_entry.prio))
			return timer->cb_entry.prio;
	}
	/* Standard softirq prio: 1 */
	return (MAX_RT_PRIO - 1) - 1;
}

static inline void hrtimer_adjust_softirq_prio(struct hrtimer_cpu_base *base)
{
	int prio = hrtimer_get_softirq_prio(base);

	if (current->normal_prio != prio) {
		set_task_prio(current, prio);
		spin_unlock_irq(&base->lock);
		cond_resched();
		spin_lock_irq(&base->lock);
	}
}

static inline void hrtimer_raise_softirq(struct hrtimer_cpu_base *base)
{
	raise_softirq_irqoff_prio(HRTIMER_SOFTIRQ,
				  hrtimer_get_softirq_prio(base));
}

#endif

/*
 * Check, whether the timer is on the callback pending list
 */
static inline int hrtimer_cb_pending(const struct hrtimer *timer)
{
	return timer->state == HRTIMER_STATE_PENDING;
}

static inline void hrtimer_init_hres(struct hrtimer_cpu_base *base)
{
	base->expires_next.tv64 = KTIME_MAX;
	set_bit(0, &base->check_clocks);
	base->hres_active = 0;
	hrtimer_init_base_cb_pending(base);
}

/*
 * When High resolution timers are active, try to reprogram. Note, that in case
 * the state has HRTIMER_STATE_CALLBACK set, no reprogramming and no expiry
 * check happens. The timer gets enqueued into the rbtree. The reprogramming
 * and expiry check is done in the hrtimer_interrupt or in the softirq.
 */
static inline int hrtimer_enqueue_reprogram(struct hrtimer *timer,
					    struct hrtimer_clock_base *base)
{
	if (base->cpu_base->hres_active && hrtimer_reprogram(timer, base)) {

		/* Timer is expired, act upon the callback mode */
		switch(timer->cb_mode) {
		case HRTIMER_CB_IRQSAFE_NO_RESTART:
			/*
			 * We can call the callback from here. No restart
			 * happens, so no danger of recursion
			 */
			BUG_ON(timer->function(timer) != HRTIMER_NORESTART);
			return 1;
		case HRTIMER_CB_IRQSAFE_NO_SOFTIRQ:
			/*
			 * This is solely for the sched tick emulation with
			 * dynamic tick support to ensure that we do not
			 * restart the tick right on the edge and end up with
			 * the tick timer in the softirq ! The calling site
			 * takes care of this.
			 */
			return 1;
		case HRTIMER_CB_IRQSAFE:
		case HRTIMER_CB_SOFTIRQ:
			/*
			 * Move everything else into the softirq pending list !
			 */
			hrtimer_add_cb_pending(timer, base);
			timer->state = HRTIMER_STATE_PENDING;
			hrtimer_raise_softirq(base->cpu_base);
			return 1;
		default:
			BUG();
		}
	}
	return 0;
}

/*
 * Called after timekeeping resumed and updated jiffies64. Set the jiffies
 * update time to now.
 */
static inline void hrtimer_resume_jiffy_update(void)
{
	unsigned long flags;
	ktime_t now = ktime_get();

	write_seqlock_irqsave(&xtime_lock, flags);
	last_jiffies_update = now;
	write_sequnlock_irqrestore(&xtime_lock, flags);
}

#else

static inline int hrtimer_hres_active(void) { return 0; }
static inline void hrtimer_check_clocks(void) { }
static inline void hrtimer_force_reprogram(struct hrtimer_cpu_base *base) { }
static inline int hrtimer_enqueue_reprogram(struct hrtimer *timer,
					    struct hrtimer_clock_base *base)
{
	return 0;
}
static inline int hrtimer_cb_pending(struct hrtimer *timer) { return 0; }
static inline void hrtimer_remove_cb_pending(struct hrtimer *timer) { }
static inline void hrtimer_init_hres(struct hrtimer_cpu_base *base) { }
static inline void hrtimer_init_timer_hres(struct hrtimer *timer) { }
static inline void hrtimer_resume_jiffy_update(void) { }
static inline void hrtimer_update_timer_prio(struct hrtimer *timer) { }

#endif /* CONFIG_HIGH_RES_TIMERS */

static inline int hrtimer_is_queued(struct hrtimer *timer)
{
	return timer->state != HRTIMER_STATE_INACTIVE &&
		timer->state != HRTIMER_STATE_CALLBACK;
}

#ifdef CONFIG_TIMER_STATS
void __timer_stats_hrtimer_set_start_info(struct hrtimer *timer, void *addr)
{
	if (timer->start_site)
		return;

	timer->start_site = addr;
	memcpy(timer->start_comm, current->comm, TASK_COMM_LEN);
	timer->start_pid = current->pid;
}
#endif

/*
 * Timekeeping resumed notification
 */
void hrtimer_notify_resume(void)
{
	hrtimer_resume_jiffy_update();
	clockevents_resume_events();
	clock_was_set();
}

#ifdef CONFIG_PREEMPT_SOFTIRQS
# define wake_up_timer_waiters(b)	wake_up(&(b)->wait)

/**
 * hrtimer_wait_for_timer - Wait for a running timer
 *
 * @timer:	timer to wait for
 *
 * The function waits in case the timers callback function is
 * currently executed on the waitqueue of the timer base. The
 * waitqueue is woken up after the timer callback function has
 * finished execution.
 */
void hrtimer_wait_for_timer(const struct hrtimer *timer)
{
	struct hrtimer_clock_base *base = timer->base;

	if (base && base->cpu_base)
		wait_event(base->cpu_base->wait,
				!(timer->state & HRTIMER_STATE_CALLBACK));
}

#else
# define wake_up_timer_waiters(b)	do { } while (0)
#endif

/*
 * Counterpart to lock_timer_base above:
 */
static inline
void unlock_hrtimer_base(const struct hrtimer *timer, unsigned long *flags)
{
	spin_unlock_irqrestore(&timer->base->cpu_base->lock, *flags);
}

/**
 * hrtimer_forward - forward the timer expiry
 * @timer:	hrtimer to forward
 * @now:	forward past this time
 * @interval:	the interval to forward
 *
 * Forward the timer expiry so it will expire in the future.
 * Returns the number of overruns.
 */
unsigned long
hrtimer_forward(struct hrtimer *timer, ktime_t now, ktime_t interval)
{
	unsigned long orun = 1;
	ktime_t delta;

	delta = ktime_sub(now, timer->expires);

	if (delta.tv64 < 0)
		return 0;

	if (interval.tv64 < timer->base->resolution.tv64)
		interval.tv64 = timer->base->resolution.tv64;

	if (unlikely(delta.tv64 >= interval.tv64)) {
		s64 incr = ktime_to_ns(interval);

		orun = ktime_divns(delta, incr);
		timer->expires = ktime_add_ns(timer->expires, incr * orun);
		if (timer->expires.tv64 > now.tv64)
			return orun;
		/*
		 * This (and the ktime_add() below) is the
		 * correction for exact:
		 */
		orun++;
	}
	timer->expires = ktime_add(timer->expires, interval);
	/*
	 * Make sure, that the result did not wrap with a very large
	 * interval.
	 */
	if (timer->expires.tv64 < 0)
		timer->expires = ktime_set(KTIME_SEC_MAX, 0);

	return orun;
}

/*
 * enqueue_hrtimer - internal function to (re)start a timer
 *
 * The timer is inserted in expiry order. Insertion into the
 * red black tree is O(log(n)). Must hold the base lock.
 */
static void enqueue_hrtimer(struct hrtimer *timer,
			    struct hrtimer_clock_base *base, int reprogram)
{
	struct rb_node **link = &base->active.rb_node;
	struct rb_node *parent = NULL;
	struct hrtimer *entry;

	hrtimer_trace(timer->expires, (unsigned long) timer);

	/*
	 * Find the right place in the rbtree:
	 */
	while (*link) {
		parent = *link;
		entry = rb_entry(parent, struct hrtimer, node);
		/*
		 * We dont care about collisions. Nodes with
		 * the same expiry time stay together.
		 */
		if (timer->expires.tv64 < entry->expires.tv64)
			link = &(*link)->rb_left;
		else
			link = &(*link)->rb_right;
	}

	/*
	 * Insert the timer to the rbtree and check whether it
	 * replaces the first pending timer
	 */
	if (!base->first || timer->expires.tv64 <
	    rb_entry(base->first, struct hrtimer, node)->expires.tv64) {
		/*
		 * Reprogram the clock event device. When the timer is already
		 * expired hrtimer_enqueue_reprogram has either called the
		 * callback or added it to the pending list and raised the
		 * softirq.
		 *
		 * This is a NOP for !HIGHRES
		 */
		if (reprogram && hrtimer_enqueue_reprogram(timer, base))
			return;

		base->first = &timer->node;
	}

	rb_link_node(&timer->node, parent, link);
	rb_insert_color(&timer->node, &base->active);
	/*
	 * HRTIMER_STATE_ENQUEUED is or'ed to the current state to preserve the
	 * state of a possibly running callback.
	 */
	timer->state |= HRTIMER_STATE_ENQUEUED;
}

/*
 * __remove_hrtimer - internal function to remove a timer
 *
 * Caller must hold the base lock.
 *
 * High resolution timer mode reprograms the clock event device when the
 * timer is the one which expires next. The caller can disable this by setting
 * reprogram to zero. This is useful, when the context does a reprogramming
 * anyway (e.g. timer interrupt)
 */
static void __remove_hrtimer(struct hrtimer *timer,
			     struct hrtimer_clock_base *base,
			     unsigned long newstate, int reprogram)
{
	/* High res. callback list. NOP for !HIGHRES */
	if (hrtimer_cb_pending(timer))
		hrtimer_remove_cb_pending(timer);
	else {
		/*
		 * Remove the timer from the rbtree and replace the
		 * first entry pointer if necessary.
		 */
		if (base->first == &timer->node) {
			base->first = rb_next(&timer->node);
			/* Reprogram the clock event device. if enabled */
			if (reprogram && hrtimer_hres_active())
				hrtimer_force_reprogram(base->cpu_base);
		}
		rb_erase(&timer->node, &base->active);
	}
	timer->state = newstate;
}

/*
 * remove hrtimer, called with base lock held
 */
static inline int
remove_hrtimer(struct hrtimer *timer, struct hrtimer_clock_base *base)
{
	if (hrtimer_is_queued(timer)) {
		int reprogram;

		hrtimer_trace(timer->expires, (unsigned long) timer);

		/*
		 * Remove the timer and force reprogramming when high
		 * resolution mode is active and the timer is on the current
		 * CPU. If we remove a timer on another CPU, reprogramming is
		 * skipped. The interrupt event on this CPU is fired and
		 * reprogramming happens in the interrupt handler. This is a
		 * rare case and less expensive than a smp call.
		 */
		timer_stats_hrtimer_clear_start_info(timer);
		reprogram = base->cpu_base == &__get_cpu_var(hrtimer_bases);
		__remove_hrtimer(timer, base, HRTIMER_STATE_INACTIVE,
				 reprogram);
		return 1;
	}
	return 0;
}

/**
 * hrtimer_start - (re)start an relative timer on the current CPU
 * @timer:	the timer to be added
 * @tim:	expiry time
 * @mode:	expiry mode: absolute (HRTIMER_ABS) or relative (HRTIMER_REL)
 *
 * Returns:
 *  0 on success
 *  1 when the timer was active
 */
int
hrtimer_start(struct hrtimer *timer, ktime_t tim, const enum hrtimer_mode mode)
{
	struct hrtimer_clock_base *base, *new_base;
	unsigned long flags;
	int ret;

	base = lock_hrtimer_base(timer, &flags);

	/* Remove an active timer from the queue: */
	ret = remove_hrtimer(timer, base);

	/* Switch the timer base, if necessary: */
	new_base = switch_hrtimer_base(timer, base);

	hrtimer_update_timer_prio(timer);

	if (mode == HRTIMER_MODE_REL) {
		tim = ktime_add(tim, new_base->get_time());
		/*
		 * CONFIG_TIME_LOW_RES is a temporary way for architectures
		 * to signal that they simply return xtime in
		 * do_gettimeoffset(). In this case we want to round up by
		 * resolution when starting a relative timer, to avoid short
		 * timeouts. This will go away with the GTOD framework.
		 */
#ifdef CONFIG_TIME_LOW_RES
		tim = ktime_add(tim, base->resolution);
#endif
	}
	timer->expires = tim;

	timer_stats_hrtimer_set_start_info(timer);

	enqueue_hrtimer(timer, new_base, base == new_base);

	unlock_hrtimer_base(timer, &flags);

	return ret;
}
EXPORT_SYMBOL_GPL(hrtimer_start);

/**
 * hrtimer_try_to_cancel - try to deactivate a timer
 * @timer:	hrtimer to stop
 *
 * Returns:
 *  0 when the timer was not active
 *  1 when the timer was active
 * -1 when the timer is currently excuting the callback function and
 *    cannot be stopped
 */
int hrtimer_try_to_cancel(struct hrtimer *timer)
{
	struct hrtimer_clock_base *base;
	unsigned long flags;
	int ret = -1;

	base = lock_hrtimer_base(timer, &flags);

	if (!(timer->state & HRTIMER_STATE_CALLBACK))
		ret = remove_hrtimer(timer, base);

	unlock_hrtimer_base(timer, &flags);

	return ret;

}
EXPORT_SYMBOL_GPL(hrtimer_try_to_cancel);

/**
 * hrtimer_cancel - cancel a timer and wait for the handler to finish.
 * @timer:	the timer to be cancelled
 *
 * Returns:
 *  0 when the timer was not active
 *  1 when the timer was active
 */
int hrtimer_cancel(struct hrtimer *timer)
{
	for (;;) {
		int ret = hrtimer_try_to_cancel(timer);

		if (ret >= 0)
			return ret;
		hrtimer_wait_for_timer(timer);
	}
}
EXPORT_SYMBOL_GPL(hrtimer_cancel);

/**
 * hrtimer_get_remaining - get remaining time for the timer
 * @timer:	the timer to read
 */
ktime_t hrtimer_get_remaining(const struct hrtimer *timer)
{
	struct hrtimer_clock_base *base;
	unsigned long flags;
	ktime_t rem;

	base = lock_hrtimer_base(timer, &flags);
	rem = ktime_sub(timer->expires, timer->base->get_time());
	unlock_hrtimer_base(timer, &flags);

	return rem;
}
EXPORT_SYMBOL_GPL(hrtimer_get_remaining);

#ifdef CONFIG_NO_IDLE_HZ
/**
 * hrtimer_get_next_event - get the time until next expiry event
 *
 * Returns the delta to the next expiry event or KTIME_MAX if no timer
 * is pending.
 */
ktime_t hrtimer_get_next_event(void)
{
	struct hrtimer_cpu_base *cpu_base = &__get_cpu_var(hrtimer_bases);
	struct hrtimer_clock_base *base = cpu_base->clock_base;
	ktime_t delta, mindelta = { .tv64 = KTIME_MAX };
	unsigned long flags;
	int i;

	spin_lock_irqsave(&cpu_base->lock, flags);

	for (i = 0; i < HRTIMER_MAX_CLOCK_BASES; i++, base++) {
		struct hrtimer *timer;

		if (!base->first)
			continue;

		timer = rb_entry(base->first, struct hrtimer, node);
		delta.tv64 = timer->expires.tv64;
		delta = ktime_sub(delta, base->get_time());
		if (delta.tv64 < mindelta.tv64)
			mindelta.tv64 = delta.tv64;
	}

	spin_unlock_irqrestore(&cpu_base->lock, flags);

	if (mindelta.tv64 < 0)
		mindelta.tv64 = 0;
	return mindelta;
}
#endif

/**
 * hrtimer_init - initialize a timer to the given clock
 * @timer:	the timer to be initialized
 * @clock_id:	the clock to be used
 * @mode:	timer mode abs/rel
 */
void hrtimer_init(struct hrtimer *timer, clockid_t clock_id,
		  enum hrtimer_mode mode)
{
	struct hrtimer_cpu_base *cpu_base;

	memset(timer, 0, sizeof(struct hrtimer));

	cpu_base = &__raw_get_cpu_var(hrtimer_bases);

	if (clock_id == CLOCK_REALTIME && mode != HRTIMER_MODE_ABS)
		clock_id = CLOCK_MONOTONIC;

	timer->base = &cpu_base->clock_base[clock_id];
	hrtimer_init_timer_hres(timer);

#ifdef CONFIG_TIMER_STATS
	timer->start_site = NULL;
	timer->start_pid = -1;
	memset(timer->start_comm, 0, TASK_COMM_LEN);
#endif
}
EXPORT_SYMBOL_GPL(hrtimer_init);

/**
 * hrtimer_get_res - get the timer resolution for a clock
 * @which_clock: which clock to query
 * @tp:		 pointer to timespec variable to store the resolution
 *
 * Store the resolution of the clock selected by which_clock in the
 * variable pointed to by tp.
 */
int hrtimer_get_res(const clockid_t which_clock, struct timespec *tp)
{
	struct hrtimer_cpu_base *cpu_base;

	cpu_base = &__raw_get_cpu_var(hrtimer_bases);
	*tp = ktime_to_timespec(cpu_base->clock_base[which_clock].resolution);

	return 0;
}
EXPORT_SYMBOL_GPL(hrtimer_get_res);

#ifdef CONFIG_HIGH_RES_TIMERS

/*
 * High resolution timer interrupt
 * Called with interrupts disabled
 */
void hrtimer_interrupt(struct pt_regs *regs)
{
	struct hrtimer_cpu_base *cpu_base = &__get_cpu_var(hrtimer_bases);
	struct hrtimer_clock_base *base;
	ktime_t expires_next, now;
	int i, raise = 0;

	BUG_ON(!cpu_base->hres_active);

	/* Store the regs for an possible sched_timer callback */
	cpu_base->sched_regs = regs;
	cpu_base->nr_events++;

 retry:
	now = ktime_get();
	hrtimer_trace(now, 0);
	expires_next.tv64 = KTIME_MAX;

	base = cpu_base->clock_base;

	for (i = 0; i < HRTIMER_MAX_CLOCK_BASES; i++) {
		ktime_t basenow;
		struct rb_node *node;

		spin_lock(&cpu_base->lock);

		basenow = ktime_add(now, base->offset);

		while ((node = base->first)) {
			struct hrtimer *timer;

			timer = rb_entry(node, struct hrtimer, node);

			if (basenow.tv64 < timer->expires.tv64) {
				ktime_t expires;

				expires = ktime_sub(timer->expires,
						    base->offset);
				if (expires.tv64 < expires_next.tv64)
					expires_next = expires;
				break;
			}

			hrtimer_trace(timer->expires, (unsigned long) timer);

			/* Move softirq callbacks to the pending list */
			if (timer->cb_mode == HRTIMER_CB_SOFTIRQ) {
				__remove_hrtimer(timer, base,
						 HRTIMER_STATE_PENDING, 0);
				hrtimer_add_cb_pending(timer, base);
				raise = 1;
				continue;
			}

			__remove_hrtimer(timer, base,
					 HRTIMER_STATE_CALLBACK, 0);
			timer_stats_account_hrtimer(timer);

			if (timer->function(timer) != HRTIMER_NORESTART) {
				BUG_ON(timer->state != HRTIMER_STATE_CALLBACK);
				/*
				 * Do not reprogram. We do this when we break
				 * out of the loop !
				 */
				enqueue_hrtimer(timer, base, 0);
			}
			timer->state &= ~HRTIMER_STATE_CALLBACK;
		}
		spin_unlock(&cpu_base->lock);
		base++;
	}

	cpu_base->expires_next = expires_next;

	/* Reprogramming necessary ? */
	if (expires_next.tv64 != KTIME_MAX) {
		if (clockevents_set_next_event(expires_next, 0))
			goto retry;
	}

	/* Invalidate regs */
	cpu_base->sched_regs = NULL;

	/* Raise softirq ? */
	if (raise)
		hrtimer_raise_softirq(cpu_base);
}

static void run_hrtimer_softirq(struct softirq_action *h)
{
	struct hrtimer_cpu_base *cpu_base = &__get_cpu_var(hrtimer_bases);

	spin_lock_irq(&cpu_base->lock);

	while (hrtimer_base_cb_pending(cpu_base)) {
		enum hrtimer_restart (*fn)(struct hrtimer *);
		struct hrtimer *timer;
		int restart;

		timer = hrtimer_base_get_cb_pending(cpu_base);

		timer_stats_account_hrtimer(timer);

		fn = timer->function;
		__remove_hrtimer(timer, timer->base, HRTIMER_STATE_CALLBACK, 0);
		spin_unlock_irq(&cpu_base->lock);

		restart = fn(timer);

		spin_lock_irq(&cpu_base->lock);

		timer->state &= ~HRTIMER_STATE_CALLBACK;
		if (restart == HRTIMER_RESTART) {
			BUG_ON(hrtimer_active(timer));
			/*
			 * Enqueue the timer, allow reprogramming of the event
			 * device
			 */
			enqueue_hrtimer(timer, timer->base, 1);
		} else if (hrtimer_active(timer)) {
			/*
			 * If the timer was rearmed on another CPU, reprogram
			 * the event device.
			 */
			if (timer->base->first == &timer->node)
				hrtimer_reprogram(timer, timer->base);
		}

		hrtimer_adjust_softirq_prio(cpu_base);
	}
	spin_unlock_irq(&cpu_base->lock);

	wake_up_timer_waiters(cpu_base);
}

#endif	/* CONFIG_HIGH_RES_TIMERS */

/*
 * Expire the per base hrtimer-queue:
 */
static inline void run_hrtimer_queue(struct hrtimer_cpu_base *cpu_base,
				     int index)
{
	struct rb_node *node;
	struct hrtimer_clock_base *base = &cpu_base->clock_base[index];

	if (!base->first)
		return;

	if (base->get_softirq_time)
		base->softirq_time = base->get_softirq_time();

	spin_lock_irq(&cpu_base->lock);

	while ((node = base->first)) {
		struct hrtimer *timer;
		enum hrtimer_restart (*fn)(struct hrtimer *);
		int restart;

		timer = rb_entry(node, struct hrtimer, node);
		if (base->softirq_time.tv64 <= timer->expires.tv64)
			break;

		timer_stats_account_hrtimer(timer);

		fn = timer->function;
		__remove_hrtimer(timer, base, HRTIMER_STATE_CALLBACK, 0);
		spin_unlock_irq(&cpu_base->lock);

		restart = fn(timer);

		spin_lock_irq(&cpu_base->lock);

		timer->state &= ~HRTIMER_STATE_CALLBACK;
		if (restart != HRTIMER_NORESTART) {
			BUG_ON(hrtimer_active(timer));
			enqueue_hrtimer(timer, base, 0);
		}
	}
	spin_unlock_irq(&cpu_base->lock);

	wake_up_timer_waiters(cpu_base);
}

/*
 * Called from timer softirq every jiffy, expire hrtimers:
 *
 * For HRT its the fall back code to run the softirq in the timer
 * softirq context in case the hrtimer initialization failed or has
 * not been done yet.
 */
void hrtimer_run_queues(void)
{
	struct hrtimer_cpu_base *cpu_base = &__get_cpu_var(hrtimer_bases);
	int i;

	hrtimer_check_clocks();

	if (hrtimer_hres_active())
		return;

	hrtimer_get_softirq_time(cpu_base);

	for (i = 0; i < HRTIMER_MAX_CLOCK_BASES; i++)
		run_hrtimer_queue(cpu_base, i);
}

/*
 * Sleep related functions:
 */
static enum hrtimer_restart hrtimer_wakeup(struct hrtimer *timer)
{
	struct hrtimer_sleeper *t =
		container_of(timer, struct hrtimer_sleeper, timer);
	struct task_struct *task = t->task;

	t->task = NULL;
	if (task)
		wake_up_process(task);

	return HRTIMER_NORESTART;
}

void hrtimer_init_sleeper(struct hrtimer_sleeper *sl, struct task_struct *task)
{
	sl->timer.function = hrtimer_wakeup;
	sl->task = task;
#ifdef CONFIG_HIGH_RES_TIMERS
	sl->timer.cb_mode = HRTIMER_CB_IRQSAFE_NO_RESTART;
#endif
}

static int __sched do_nanosleep(struct hrtimer_sleeper *t, enum hrtimer_mode mode)
{
	hrtimer_init_sleeper(t, current);

	do {
		set_current_state(TASK_INTERRUPTIBLE);
		hrtimer_start(&t->timer, t->timer.expires, mode);

		if (likely(t->task))
			schedule();

		hrtimer_cancel(&t->timer);
		mode = HRTIMER_MODE_ABS;

	} while (t->task && !signal_pending(current));

	return t->task == NULL;
}

long __sched hrtimer_nanosleep_restart(struct restart_block *restart)
{
	struct hrtimer_sleeper t;
	struct timespec __user *rmtp;
	struct timespec tu;
	ktime_t time;

	restart->fn = do_no_restart_syscall;

	hrtimer_init(&t.timer, restart->arg0, HRTIMER_MODE_ABS);
	t.timer.expires.tv64 = ((u64)restart->arg3 << 32) | (u64) restart->arg2;

	if (do_nanosleep(&t, HRTIMER_MODE_ABS))
		return 0;

	rmtp = (struct timespec __user *) restart->arg1;
	if (rmtp) {
		time = ktime_sub(t.timer.expires, t.timer.base->get_time());
		if (time.tv64 <= 0)
			return 0;
		tu = ktime_to_timespec(time);
		if (copy_to_user(rmtp, &tu, sizeof(tu)))
			return -EFAULT;
	}

	restart->fn = hrtimer_nanosleep_restart;

	/* The other values in restart are already filled in */
	return -ERESTART_RESTARTBLOCK;
}

long hrtimer_nanosleep(struct timespec *rqtp, struct timespec __user *rmtp,
		       const enum hrtimer_mode mode, const clockid_t clockid)
{
	struct restart_block *restart;
	struct hrtimer_sleeper t;
	struct timespec tu;
	ktime_t rem;

	hrtimer_init(&t.timer, clockid, mode);
	t.timer.expires = timespec_to_ktime(*rqtp);
	if (do_nanosleep(&t, mode))
		return 0;

	/* Absolute timers do not update the rmtp value and restart: */
	if (mode == HRTIMER_MODE_ABS)
		return -ERESTARTNOHAND;

	if (rmtp) {
		rem = ktime_sub(t.timer.expires, t.timer.base->get_time());
		if (rem.tv64 <= 0)
			return 0;
		tu = ktime_to_timespec(rem);
		if (copy_to_user(rmtp, &tu, sizeof(tu)))
			return -EFAULT;
	}

	restart = &current_thread_info()->restart_block;
	restart->fn = hrtimer_nanosleep_restart;
	restart->arg0 = (unsigned long) t.timer.base->index;
	restart->arg1 = (unsigned long) rmtp;
	restart->arg2 = t.timer.expires.tv64 & 0xFFFFFFFF;
	restart->arg3 = t.timer.expires.tv64 >> 32;

	return -ERESTART_RESTARTBLOCK;
}

asmlinkage long
sys_nanosleep(struct timespec __user *rqtp, struct timespec __user *rmtp)
{
	struct timespec tu;

	if (copy_from_user(&tu, rqtp, sizeof(tu)))
		return -EFAULT;

	if (!timespec_valid(&tu))
		return -EINVAL;

	return hrtimer_nanosleep(&tu, rmtp, HRTIMER_MODE_REL, CLOCK_MONOTONIC);
}

/*
 * Functions related to boot-time initialization:
 */
static void __devinit init_hrtimers_cpu(int cpu)
{
	struct hrtimer_cpu_base *cpu_base = &per_cpu(hrtimer_bases, cpu);
	int i;

	spin_lock_init(&cpu_base->lock);
	lockdep_set_class(&cpu_base->lock, &cpu_base->lock_key);

	for (i = 0; i < HRTIMER_MAX_CLOCK_BASES; i++)
		cpu_base->clock_base[i].cpu_base = cpu_base;

	hrtimer_init_hres(cpu_base);
#ifdef CONFIG_PREEMPT_SOFTIRQS
	init_waitqueue_head(&cpu_base->wait);
#endif
}

#ifdef CONFIG_HOTPLUG_CPU

static void migrate_hrtimer_list(struct hrtimer_clock_base *old_base,
				struct hrtimer_clock_base *new_base)
{
	struct hrtimer *timer;
	struct rb_node *node;

	while ((node = rb_first(&old_base->active))) {
		timer = rb_entry(node, struct hrtimer, node);
		BUG_ON(timer->state & HRTIMER_STATE_CALLBACK);
		__remove_hrtimer(timer, old_base, HRTIMER_STATE_INACTIVE, 0);
		timer->base = new_base;
		/*
		 * Enqueue the timer. Allow reprogramming of the event device
		 */
		enqueue_hrtimer(timer, new_base, 1);
	}
}

static void migrate_hrtimers(int cpu)
{
	struct hrtimer_cpu_base *old_base, *new_base;
	int i;

	BUG_ON(cpu_online(cpu));
	old_base = &per_cpu(hrtimer_bases, cpu);
	new_base = &get_cpu_var(hrtimer_bases);

	local_irq_disable();

	spin_lock(&new_base->lock);
	spin_lock(&old_base->lock);

	for (i = 0; i < HRTIMER_MAX_CLOCK_BASES; i++) {
		migrate_hrtimer_list(&old_base->clock_base[i],
				     &new_base->clock_base[i]);
	}
	spin_unlock(&old_base->lock);
	spin_unlock(&new_base->lock);

	local_irq_enable();
	put_cpu_var(hrtimer_bases);
}
#endif /* CONFIG_HOTPLUG_CPU */

static int __cpuinit hrtimer_cpu_notify(struct notifier_block *self,
					unsigned long action, void *hcpu)
{
	long cpu = (long)hcpu;

	switch (action) {

	case CPU_UP_PREPARE:
		init_hrtimers_cpu(cpu);
		break;

#ifdef CONFIG_HOTPLUG_CPU
	case CPU_DEAD:
		migrate_hrtimers(cpu);
		break;
#endif

	default:
		break;
	}

	return NOTIFY_OK;
}

static struct notifier_block __cpuinitdata hrtimers_nb = {
	.notifier_call = hrtimer_cpu_notify,
};

void __init hrtimers_init(void)
{
	hrtimer_cpu_notify(&hrtimers_nb, (unsigned long)CPU_UP_PREPARE,
			  (void *)(long)smp_processor_id());
	register_cpu_notifier(&hrtimers_nb);
#ifdef CONFIG_HIGH_RES_TIMERS
	open_softirq(HRTIMER_SOFTIRQ, run_hrtimer_softirq, NULL);
#endif
}

