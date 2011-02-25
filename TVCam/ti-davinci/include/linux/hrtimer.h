/*
 *  include/linux/hrtimer.h
 *
 *  hrtimers - High-resolution kernel timers
 *
 *   Copyright(C) 2005, Thomas Gleixner <tglx@linutronix.de>
 *   Copyright(C) 2005, Red Hat, Inc., Ingo Molnar
 *
 *  data type definitions, declarations, prototypes
 *
 *  Started by: Thomas Gleixner and Ingo Molnar
 *
 *  For licencing details see kernel-base/COPYING
 */
#ifndef _LINUX_HRTIMER_H
#define _LINUX_HRTIMER_H

#include <linux/rbtree.h>
#include <linux/ktime.h>
#include <linux/timer.h>
#include <linux/init.h>
#include <linux/plist.h>
#include <linux/wait.h>

struct seq_file;
struct hrtimer_clock_base;
struct hrtimer_cpu_base;

/*
 * Mode arguments of xxx_hrtimer functions:
 */
enum hrtimer_mode {
	HRTIMER_MODE_ABS,	/* Time value is absolute */
	HRTIMER_MODE_REL,	/* Time value is relative to now */
};

/*
 * Return values for the callback function
 */
enum hrtimer_restart {
	HRTIMER_NORESTART,	/* Timer is not restarted */
	HRTIMER_RESTART,	/* Timer must be restarted */
};

/*
 * hrtimer callback modes:
 *
 *	HRTIMER_CB_SOFTIRQ:		Callback must run in softirq context
 *	HRTIMER_CB_IRQSAFE:		Callback may run in hardirq context
 *	HRTIMER_CB_IRQSAFE_NO_RESTART:	Callback may run in hardirq context and
 *					does not restart the timer
 *	HRTIMER_CB_IRQSAFE_NO_SOFTIRQ:	Callback must run in softirq context
 *					Special mode for tick emultation
 */
enum hrtimer_cb_mode {
	HRTIMER_CB_SOFTIRQ,
	HRTIMER_CB_IRQSAFE,
	HRTIMER_CB_IRQSAFE_NO_RESTART,
	HRTIMER_CB_IRQSAFE_NO_SOFTIRQ,
};

/*
 * Bit values to track state of the timer
 *
 * Possible states:
 *
 * 0x00		inactive
 * 0x01		enqueued into rbtree
 * 0x02		callback function running
 * 0x03		callback function running and enqueued
 *		(was requeued on another CPU)
 * 0x04		callback pending (high resolution mode)
 *
 * The "callback function running and enqueued" status is only possible on
 * SMP. It happens for example when a posix timer expired and the callback
 * queued a signal. Between dropping the lock which protects the posix timer
 * and reacquiring the base lock of the hrtimer, another CPU can deliver the
 * signal and rearm the timer. We have to preserve the callback running state,
 * as otherwise the timer could be removed before the softirq code finishes the
 * the handling of the timer.
 *
 * The HRTIMER_STATE_ENQUEUE bit is always or'ed to the current state to
 * preserve the HRTIMER_STATE_CALLBACK bit in the above scenario.
 *
 * All state transitions are protected by cpu_base->lock.
 */
#define HRTIMER_STATE_INACTIVE	0x00
#define HRTIMER_STATE_ENQUEUED	0x01
#define HRTIMER_STATE_CALLBACK	0x02
#define HRTIMER_STATE_PENDING	0x04

/**
 * struct hrtimer - the basic hrtimer structure
 * @node:	red black tree node for time ordered insertion
 * @expires:	the absolute expiry time in the hrtimers internal
 *		representation. The time is related to the clock on
 *		which the timer is based.
 * @function:	timer expiry callback function
 * @base:	pointer to the timer base (per cpu and per clock)
 * @state:	state information (See bit values above)
 * @cb_mode:	high resolution timer feature to select the callback execution
 *		 mode
 * @cb_entry:	list head to enqueue an expired timer into the callback list
 * @start_site:	timer statistics field to store the site where the timer
 *		was started
 * @start_comm: timer statistics field to store the name of the process which
 *		started the timer
 * @start_pid: timer statistics field to store the pid of the task which
 *		started the timer
 *
 * The hrtimer structure must be initialized by hrtimer_init()
 */
struct hrtimer {
	struct rb_node			node;
	ktime_t				expires;
	enum hrtimer_restart		(*function)(struct hrtimer *);
	struct hrtimer_clock_base	*base;
	unsigned long			state;
#ifdef CONFIG_HIGH_RES_TIMERS
	enum hrtimer_cb_mode		cb_mode;
#ifdef CONFIG_PREEMPT_RT
	struct plist_node		cb_entry;
#else
	struct list_head		cb_entry;
#endif
#endif
#ifdef CONFIG_TIMER_STATS
	void				*start_site;
	char				start_comm[16];
	int				start_pid;
#endif
};

/**
 * struct hrtimer_sleeper - simple sleeper structure
 * @timer:	embedded timer structure
 * @task:	task to wake up
 *
 * task is set to NULL, when the timer expires.
 */
struct hrtimer_sleeper {
	struct hrtimer timer;
	struct task_struct *task;
};

/**
 * struct hrtimer_base - the timer base for a specific clock
 * @index:		clock type index for per_cpu support when moving a
 *			timer to a base on another cpu.
 * @active:		red black tree root node for the active timers
 * @first:		pointer to the timer node which expires first
 * @resolution:		the resolution of the clock, in nanoseconds
 * @get_time:		function to retrieve the current time of the clock
 * @get_softirq_time:	function to retrieve the current time from the softirq
 * @softirq_time:	the time when running the hrtimer queue in the softirq
 * @cb_pending:		list of timers where the callback is pending
 * @offset:		offset of this clock to the monotonic base
 * @reprogram:		function to reprogram the timer event
 */
struct hrtimer_clock_base {
	struct hrtimer_cpu_base	*cpu_base;
	clockid_t		index;
	struct rb_root		active;
	struct rb_node		*first;
	ktime_t			resolution;
	ktime_t			(*get_time)(void);
	ktime_t			(*get_softirq_time)(void);
	ktime_t			softirq_time;
#ifdef CONFIG_HIGH_RES_TIMERS
	ktime_t			offset;
	int			(*reprogram)(struct hrtimer *t,
					     struct hrtimer_clock_base *b,
					     ktime_t n);
#endif
};

#define HRTIMER_MAX_CLOCK_BASES 2

/*
 * struct hrtimer_cpu_base - the per cpu clock bases
 * @lock:		lock protecting the base and associated clock bases
 *			and timers
 * @lock_key:		the lock_class_key for use with lockdep
 * @clock_base:		array of clock bases for this cpu
 * @curr_timer:		the timer which is executing a callback right now
 * @expires_next:	absolute time of the next event which was scheduled
 *			via clock_set_next_event()
 * @hres_active:	State of high resolution mode
 * @check_clocks:	Indictator, when set evaluate time source and clock
 *			event devices whether high resolution mode can be
 *			activated.
 * @cb_pending:		Expired timers are moved from the rbtree to this
 *			list in the timer interrupt. The list is processed
 *			in the softirq.
 * @sched_timer:	hrtimer to schedule the periodic tick in high
 *			resolution mode
 * @sched_regs:		Temporary storage for pt_regs for the sched_timer
 *			callback
 * @nr_events:		Total number of timer interrupt events
 * @idle_tick:		Store the last idle tick expiry time when the tick
 *			timer is modified for idle sleeps. This is necessary
 *			to resume the tick timer operation in the timeline
 *			when the CPU returns from idle
 * @tick_stopped:	Indicator that the idle tick has been stopped
 * @idle_jiffies:	jiffies at the entry to idle for idle time accounting
 * @idle_calls:		Total number of idle calls
 * @idle_sleeps:	Number of idle calls, where the sched tick was stopped
 * @idle_entrytime:	Time when the idle call was entered
 * @idle_sleeptime:	Sum of the time slept in idle with sched tick stopped
 */
struct hrtimer_cpu_base {
	raw_spinlock_t			lock;
	struct lock_class_key		lock_key;
	struct hrtimer_clock_base	clock_base[HRTIMER_MAX_CLOCK_BASES];
#ifdef CONFIG_HIGH_RES_TIMERS
	ktime_t				expires_next;
	int				hres_active;
	unsigned long			check_clocks;
#ifdef CONFIG_PREEMPT_RT
	struct plist_head		cb_pending;
#else
	struct list_head		cb_pending;
#endif
	struct hrtimer			sched_timer;
	struct pt_regs			*sched_regs;
	unsigned long			nr_events;
#endif
#ifdef CONFIG_NO_HZ
	ktime_t				idle_tick;
	int				tick_stopped;
	unsigned long			idle_jiffies;
	unsigned long			idle_calls;
	unsigned long			idle_sleeps;
	ktime_t				idle_entrytime;
	ktime_t				idle_sleeptime;
#endif
#ifdef CONFIG_PREEMPT_SOFTIRQS
	wait_queue_head_t		wait;
#endif
};

#ifdef CONFIG_HIGH_RES_TIMERS

extern void hrtimer_clock_notify(void);
extern void clock_was_set(void);
extern void hrtimer_interrupt(struct pt_regs *regs);

/*
 * In high resolution mode the time reference must be read accurate
 */
static inline ktime_t hrtimer_cb_get_time(struct hrtimer *timer)
{
	return timer->base->get_time();
}

/*
 * The resolution of the clocks. The resolution value is returned in
 * the clock_getres() system call to give application programmers an
 * idea of the (in)accuracy of timers. Timer values are rounded up to
 * this resolution values.
 */
# define KTIME_HIGH_RES		(ktime_t) { .tv64 = 1 }
# define KTIME_MONOTONIC_RES	KTIME_HIGH_RES

#else

# define KTIME_MONOTONIC_RES	KTIME_LOW_RES

/*
 * clock_was_set() is a NOP for non- high-resolution systems. The
 * time-sorted order guarantees that a timer does not expire early and
 * is expired in the next softirq when the clock was advanced.
 * (we still call the warp-check debugging code)
 */
static inline void clock_was_set(void) { }
static inline void hrtimer_clock_notify(void) { }

/*
 * In non high resolution mode the time reference is taken from
 * the base softirq time variable.
 */
static inline ktime_t hrtimer_cb_get_time(struct hrtimer *timer)
{
	return timer->base->softirq_time;
}

#endif

extern ktime_t ktime_get(void);
extern ktime_t ktime_get_real(void);

# if (BITS_PER_LONG == 64) || defined(CONFIG_KTIME_SCALAR)
#  define hrtimer_trace(a,b)		trace_special_u64((a).tv64,b)
# else
#  define hrtimer_trace(a,b)		trace_special((a).tv.sec,(a).tv.nsec,b)
# endif

/* Exported timer functions: */

/* Initialize timers: */
extern void hrtimer_init(struct hrtimer *timer, clockid_t which_clock,
			 enum hrtimer_mode mode);

/* Basic timer operations: */
extern int hrtimer_start(struct hrtimer *timer, ktime_t tim,
			 const enum hrtimer_mode mode);
extern int hrtimer_cancel(struct hrtimer *timer);
extern int hrtimer_try_to_cancel(struct hrtimer *timer);

static inline int hrtimer_restart(struct hrtimer *timer)
{
	return hrtimer_start(timer, timer->expires, HRTIMER_MODE_ABS);
}

/* Softirq preemption could deadlock timer removal */
#ifdef CONFIG_PREEMPT_SOFTIRQS
  extern void hrtimer_wait_for_timer(const struct hrtimer *timer);
#else
# define hrtimer_wait_for_timer(timer)	do { cpu_relax(); } while (0)
#endif

/* Query timers: */
extern ktime_t hrtimer_get_remaining(const struct hrtimer *timer);
extern int hrtimer_get_res(const clockid_t which_clock, struct timespec *tp);

#ifdef CONFIG_NO_IDLE_HZ
extern ktime_t hrtimer_get_next_event(void);
#endif

/*
 * A timer is active, when it is enqueued into the rbtree or the callback
 * function is running.
 */
static inline int hrtimer_active(const struct hrtimer *timer)
{
	return timer->state != HRTIMER_STATE_INACTIVE;
}

/* Forward a hrtimer so it expires after now: */
extern unsigned long
hrtimer_forward(struct hrtimer *timer, ktime_t now, ktime_t interval);

/* Precise sleep: */
extern long hrtimer_nanosleep(struct timespec *rqtp,
			      struct timespec __user *rmtp,
			      const enum hrtimer_mode mode,
			      const clockid_t clockid);
extern long hrtimer_nanosleep_restart(struct restart_block *restart_block);

extern void hrtimer_init_sleeper(struct hrtimer_sleeper *sl,
				 struct task_struct *tsk);

/* Soft interrupt function to run the hrtimer queues: */
extern void hrtimer_run_queues(void);

/* Resume notification */
void hrtimer_notify_resume(void);

#ifdef CONFIG_NO_HZ
extern void hrtimer_stop_sched_tick(void);
extern void hrtimer_restart_sched_tick(void);
extern void hrtimer_update_jiffies(void);
extern void show_no_hz_stats(struct seq_file *p);
#else
static inline void hrtimer_stop_sched_tick(void) { }
static inline void hrtimer_restart_sched_tick(void) { }
static inline void hrtimer_update_jiffies(void) { }
static inline void show_no_hz_stats(struct seq_file *p) { }
#endif

/* Bootup initialization: */
extern void __init hrtimers_init(void);

/*
 * Timer-statistics info:
 */
#ifdef CONFIG_TIMER_STATS

extern void timer_stats_update_stats(void *timer, pid_t pid, void *startf,
				     void *timerf, char * comm);

static inline void timer_stats_account_hrtimer(struct hrtimer *timer)
{
	timer_stats_update_stats(timer, timer->start_pid, timer->start_site,
				 timer->function, timer->start_comm);
}

extern void __timer_stats_hrtimer_set_start_info(struct hrtimer *timer,
						 void *addr);

static inline void timer_stats_hrtimer_set_start_info(struct hrtimer *timer)
{
	__timer_stats_hrtimer_set_start_info(timer, __builtin_return_address(0));
}

static inline void timer_stats_hrtimer_clear_start_info(struct hrtimer *timer)
{
	timer->start_site = NULL;
}
#else
static inline void timer_stats_account_hrtimer(struct hrtimer *timer)
{
}

static inline void timer_stats_hrtimer_set_start_info(struct hrtimer *timer)
{
}

static inline void timer_stats_hrtimer_clear_start_info(struct hrtimer *timer)
{
}
#endif

#endif
