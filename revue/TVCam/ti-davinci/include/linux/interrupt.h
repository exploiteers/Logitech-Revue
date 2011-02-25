/* interrupt.h */
#ifndef _LINUX_INTERRUPT_H
#define _LINUX_INTERRUPT_H

#include <linux/kernel.h>
#include <linux/linkage.h>
#include <linux/bitops.h>
#include <linux/preempt.h>
#include <linux/cpumask.h>
#include <linux/irqreturn.h>
#include <linux/hardirq.h>
#include <linux/sched.h>
#include <linux/irqflags.h>
#include <asm/atomic.h>
#include <asm/ptrace.h>
#include <asm/system.h>

/*
 * These correspond to the IORESOURCE_IRQ_* defines in
 * linux/ioport.h to select the interrupt line behaviour.  When
 * requesting an interrupt without specifying a IRQF_TRIGGER, the
 * setting should be assumed to be "as already configured", which
 * may be as per machine or firmware initialisation.
 */
#define IRQF_TRIGGER_NONE	0x00000000
#define IRQF_TRIGGER_RISING	0x00000001
#define IRQF_TRIGGER_FALLING	0x00000002
#define IRQF_TRIGGER_HIGH	0x00000004
#define IRQF_TRIGGER_LOW	0x00000008
#define IRQF_TRIGGER_MASK	(IRQF_TRIGGER_HIGH | IRQF_TRIGGER_LOW | \
				 IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING)
#define IRQF_TRIGGER_PROBE	0x00000010

/*
 * These flags used only by the kernel as part of the
 * irq handling routines.
 *
 * IRQF_DISABLED - keep irqs disabled when calling the action handler
 * IRQF_SAMPLE_RANDOM - irq is used to feed the random generator
 * IRQF_SHARED - allow sharing the irq among several devices
 * IRQF_PROBE_SHARED - set by callers when they expect sharing mismatches to occur
 * IRQF_TIMER - Flag to mark this interrupt as timer interrupt
 */
#define IRQF_DISABLED		0x00000020
#define IRQF_SAMPLE_RANDOM	0x00000040
#define IRQF_SHARED		0x00000080
#define IRQF_PROBE_SHARED	0x00000100
#define IRQF_PERCPU		0x00000400
#define IRQF_NODELAY		0x00000800
#define IRQF_TIMER		(0x00000200 | IRQF_NODELAY)

/*
 * Migration helpers. Scheduled for removal in 1/2007
 * Do not use for new code !
 */
#define SA_INTERRUPT		IRQF_DISABLED
#define SA_SAMPLE_RANDOM	IRQF_SAMPLE_RANDOM
#define SA_SHIRQ		IRQF_SHARED
#define SA_PROBEIRQ		IRQF_PROBE_SHARED
#define SA_PERCPU		IRQF_PERCPU

#define SA_TRIGGER_LOW		IRQF_TRIGGER_LOW
#define SA_TRIGGER_HIGH		IRQF_TRIGGER_HIGH
#define SA_TRIGGER_FALLING	IRQF_TRIGGER_FALLING
#define SA_TRIGGER_RISING	IRQF_TRIGGER_RISING
#define SA_TRIGGER_MASK		IRQF_TRIGGER_MASK

struct irqaction {
	irqreturn_t (*handler)(int, void *, struct pt_regs *);
	unsigned long flags;
	cpumask_t mask;
	const char *name;
	void *dev_id;
	struct irqaction *next;
	int irq;
	struct proc_dir_entry *dir, *threaded;
};

extern irqreturn_t no_action(int cpl, void *dev_id, struct pt_regs *regs);
extern int request_irq(unsigned int,
		       irqreturn_t (*handler)(int, void *, struct pt_regs *),
		       unsigned long, const char *, void *);
extern void free_irq(unsigned int, void *);

/*
 * On lockdep we dont want to enable hardirqs in hardirq
 * context. Use local_irq_enable_in_hardirq() to annotate
 * kernel code that has to do this nevertheless (pretty much
 * the only valid case is for old/broken hardware that is
 * insanely slow).
 *
 * NOTE: in theory this might break fragile code that relies
 * on hardirq delivery - in practice we dont seem to have such
 * places left. So the only effect should be slightly increased
 * irqs-off latencies.
 */
#ifdef CONFIG_LOCKDEP
# define local_irq_enable_in_hardirq()	do { } while (0)
#else
# define local_irq_enable_in_hardirq()	local_irq_enable_nort()
#endif

#ifdef CONFIG_GENERIC_HARDIRQS
extern void disable_irq_nosync(unsigned int irq);
extern void disable_irq(unsigned int irq);
extern void enable_irq(unsigned int irq);

/*
 * Special lockdep variants of irq disabling/enabling.
 * These should be used for locking constructs that
 * know that a particular irq context which is disabled,
 * and which is the only irq-context user of a lock,
 * that it's safe to take the lock in the irq-disabled
 * section without disabling hardirqs.
 *
 * On !CONFIG_LOCKDEP they are equivalent to the normal
 * irq disable/enable methods.
 */
static inline void disable_irq_nosync_lockdep(unsigned int irq)
{
	disable_irq_nosync(irq);
#ifdef CONFIG_LOCKDEP
	local_irq_disable();
#endif
}

static inline void disable_irq_lockdep(unsigned int irq)
{
	disable_irq(irq);
#ifdef CONFIG_LOCKDEP
	local_irq_disable();
#endif
}

static inline void enable_irq_lockdep(unsigned int irq)
{
#ifdef CONFIG_LOCKDEP
	local_irq_enable();
#endif
	enable_irq(irq);
}

/* IRQ wakeup (PM) control: */
extern int set_irq_wake(unsigned int irq, unsigned int on);

static inline int enable_irq_wake(unsigned int irq)
{
	return set_irq_wake(irq, 1);
}

static inline int disable_irq_wake(unsigned int irq)
{
	return set_irq_wake(irq, 0);
}

#else /* !CONFIG_GENERIC_HARDIRQS */
/*
 * NOTE: non-genirq architectures, if they want to support the lock
 * validator need to define the methods below in their asm/irq.h
 * files, under an #ifdef CONFIG_LOCKDEP section.
 */
# ifndef CONFIG_LOCKDEP
#  define disable_irq_nosync_lockdep(irq)	disable_irq_nosync(irq)
#  define disable_irq_lockdep(irq)		disable_irq(irq)
#  define enable_irq_lockdep(irq)		enable_irq(irq)
# endif

#endif /* CONFIG_GENERIC_HARDIRQS */

#ifndef __ARCH_SET_SOFTIRQ_PENDING
#define set_softirq_pending(x) (local_softirq_pending() = (x))
// FIXME: PREEMPT_RT: set_bit()?
#define or_softirq_pending(x)  (local_softirq_pending() |= (x))
#endif

/*
 * Temporary defines for UP kernels, until all code gets fixed.
 */
#ifndef CONFIG_SMP
static inline void __deprecated cli(void)
{
	local_irq_disable();
}
static inline void __deprecated sti(void)
{
	local_irq_enable();
}
static inline void __deprecated save_flags(unsigned long *x)
{
	local_save_flags(*x);
}
#define save_flags(x) save_flags(&x)
static inline void __deprecated restore_flags(unsigned long x)
{
	local_irq_restore(x);
}

static inline void __deprecated save_and_cli(unsigned long *x)
{
	local_irq_save(*x);
}
#define save_and_cli(x)	save_and_cli(&x)
#endif /* CONFIG_SMP */

#ifdef CONFIG_PREEMPT_RT
# define local_bh_disable()		do { } while (0)
# define __local_bh_disable(ip)		do { } while (0)
# define _local_bh_enable()		do { } while (0)
# define local_bh_enable()		do { } while (0)
# define local_bh_enable_ip(ip)		do { } while (0)
#else
extern void local_bh_disable(void);
extern void _local_bh_enable(void);
extern void local_bh_enable(void);
extern void local_bh_enable_ip(unsigned long ip);
#endif

/* PLEASE, avoid to allocate new softirqs, if you need not _really_ high
   frequency threaded job scheduling. For almost all the purposes
   tasklets are more than enough. F.e. all serial device BHs et
   al. should be converted to tasklets, not to softirqs.
 */

enum
{
	HI_SOFTIRQ=0,
	TIMER_SOFTIRQ,
	NET_TX_SOFTIRQ,
	NET_RX_SOFTIRQ,
	BLOCK_SOFTIRQ,
	TASKLET_SOFTIRQ,
#ifdef CONFIG_HIGH_RES_TIMERS
	HRTIMER_SOFTIRQ,
#endif
	RCU_SOFTIRQ,	/* Preferable RCU should always be the last softirq */
	/* Entries after this are ignored in split softirq mode */
	MAX_SOFTIRQ,
};

/* softirq mask and active fields moved to irq_cpustat_t in
 * asm/hardirq.h to get better cache usage.  KAO
 */

struct softirq_action
{
	void	(*action)(struct softirq_action *);
	void	*data;
};

asmlinkage void do_softirq(void);
extern void open_softirq(int nr, void (*action)(struct softirq_action*), void *data);
extern void softirq_init(void);

#ifdef CONFIG_PREEMPT_HARDIRQS
# define __raise_softirq_irqoff(nr) raise_softirq_irqoff(nr)
# define __do_raise_softirq_irqoff(nr) do { or_softirq_pending(1UL << (nr)); } while (0)
#else
# define __raise_softirq_irqoff(nr) do { or_softirq_pending(1UL << (nr)); } while (0)
# define __do_raise_softirq_irqoff(nr) __raise_softirq_irqoff(nr)
#endif

extern void FASTCALL(raise_softirq_irqoff_prio(unsigned int nr, int prio));
extern void FASTCALL(raise_softirq_irqoff(unsigned int nr));
extern void FASTCALL(raise_softirq(unsigned int nr));
extern void wakeup_irqd(void);

#ifdef CONFIG_PREEMPT_SOFTIRQS
extern void wait_for_softirq(int softirq);
#else
# define wait_for_softirq(x) do {} while(0)
#endif

/* Tasklets --- multithreaded analogue of BHs.

   Main feature differing them of generic softirqs: tasklet
   is running only on one CPU simultaneously.

   Main feature differing them of BHs: different tasklets
   may be run simultaneously on different CPUs.

   Properties:
   * If tasklet_schedule() is called, then tasklet is guaranteed
     to be executed on some cpu at least once after this.
   * If the tasklet is already scheduled, but its excecution is still not
     started, it will be executed only once.
   * If this tasklet is already running on another CPU, it is rescheduled
     for later.
   * Schedule must not be called from the tasklet itself (a lockup occurs)
   * Tasklet is strictly serialized wrt itself, but not
     wrt another tasklets. If client needs some intertask synchronization,
     he makes it with spinlocks.
 */

struct tasklet_struct
{
	struct tasklet_struct *next;
	unsigned long state;
	atomic_t count;
	void (*func)(unsigned long);
	unsigned long data;
};

#define DECLARE_TASKLET(name, func, data) \
struct tasklet_struct name = { NULL, 0, ATOMIC_INIT(0), func, data }

#define DECLARE_TASKLET_DISABLED(name, func, data) \
struct tasklet_struct name = { NULL, 0, ATOMIC_INIT(1), func, data }


enum
{
	TASKLET_STATE_SCHED,	/* Tasklet is scheduled for execution */
	TASKLET_STATE_RUN,	/* Tasklet is running (SMP only) */
	TASKLET_STATE_PENDING	/* Tasklet is pending */
};

#define TASKLET_STATEF_SCHED	(1 << TASKLET_STATE_SCHED)
#define TASKLET_STATEF_RUN	(1 << TASKLET_STATE_RUN)
#define TASKLET_STATEF_PENDING	(1 << TASKLET_STATE_PENDING)

#if defined(CONFIG_SMP) || defined(CONFIG_PREEMPT_RT)
static inline int tasklet_trylock(struct tasklet_struct *t)
{
	return !test_and_set_bit(TASKLET_STATE_RUN, &(t)->state);
}

static inline int tasklet_tryunlock(struct tasklet_struct *t)
{
	return cmpxchg(&t->state, TASKLET_STATEF_RUN, 0) == TASKLET_STATEF_RUN;
}

static inline void tasklet_unlock(struct tasklet_struct *t)
{
	smp_mb__before_clear_bit(); 
	clear_bit(TASKLET_STATE_RUN, &(t)->state);
}

static inline void tasklet_unlock_wait(struct tasklet_struct *t)
{
	while (test_bit(TASKLET_STATE_RUN, &(t)->state)) { barrier(); }
}
#else
# define tasklet_trylock(t)		1
# define tasklet_tryunlock(t)		1
# define tasklet_unlock_wait(t)		do { } while (0)
# define tasklet_unlock(t)		do { } while (0)
#endif

extern void FASTCALL(__tasklet_schedule(struct tasklet_struct *t));

static inline void tasklet_schedule(struct tasklet_struct *t)
{
	if (!test_and_set_bit(TASKLET_STATE_SCHED, &t->state))
		__tasklet_schedule(t);
}

extern void FASTCALL(__tasklet_hi_schedule(struct tasklet_struct *t));

static inline void tasklet_hi_schedule(struct tasklet_struct *t)
{
	if (!test_and_set_bit(TASKLET_STATE_SCHED, &t->state))
		__tasklet_hi_schedule(t);
}


static inline void tasklet_disable_nosync(struct tasklet_struct *t)
{
	atomic_inc(&t->count);
	smp_mb__after_atomic_inc();
}

static inline void tasklet_disable(struct tasklet_struct *t)
{
	tasklet_disable_nosync(t);
	tasklet_unlock_wait(t);
	smp_mb();
}

extern fastcall void tasklet_enable(struct tasklet_struct *t);
extern fastcall void tasklet_hi_enable(struct tasklet_struct *t);

extern void tasklet_kill(struct tasklet_struct *t);
extern void tasklet_kill_immediate(struct tasklet_struct *t, unsigned int cpu);
extern void tasklet_init(struct tasklet_struct *t,
			 void (*func)(unsigned long), unsigned long data);
void takeover_tasklets(unsigned int cpu);

/*
 * Autoprobing for irqs:
 *
 * probe_irq_on() and probe_irq_off() provide robust primitives
 * for accurate IRQ probing during kernel initialization.  They are
 * reasonably simple to use, are not "fooled" by spurious interrupts,
 * and, unlike other attempts at IRQ probing, they do not get hung on
 * stuck interrupts (such as unused PS2 mouse interfaces on ASUS boards).
 *
 * For reasonably foolproof probing, use them as follows:
 *
 * 1. clear and/or mask the device's internal interrupt.
 * 2. sti();
 * 3. irqs = probe_irq_on();      // "take over" all unassigned idle IRQs
 * 4. enable the device and cause it to trigger an interrupt.
 * 5. wait for the device to interrupt, using non-intrusive polling or a delay.
 * 6. irq = probe_irq_off(irqs);  // get IRQ number, 0=none, negative=multiple
 * 7. service the device to clear its pending interrupt.
 * 8. loop again if paranoia is required.
 *
 * probe_irq_on() returns a mask of allocated irq's.
 *
 * probe_irq_off() takes the mask as a parameter,
 * and returns the irq number which occurred,
 * or zero if none occurred, or a negative irq number
 * if more than one irq occurred.
 */

#if defined(CONFIG_GENERIC_HARDIRQS) && !defined(CONFIG_GENERIC_IRQ_PROBE) 
static inline unsigned long probe_irq_on(void)
{
	return 0;
}
static inline int probe_irq_off(unsigned long val)
{
	return 0;
}
static inline unsigned int probe_irq_mask(unsigned long val)
{
	return 0;
}
#else
extern unsigned long probe_irq_on(void);	/* returns 0 on failure */
extern int probe_irq_off(unsigned long);	/* returns 0 or negative on failure */
extern unsigned int probe_irq_mask(unsigned long);	/* returns mask of ISA interrupts */
#endif

#ifdef CONFIG_PREEMPT_RT
# define local_irq_disable_nort()	do { BUG_ON(in_interrupt()); } while (0)
# define local_irq_enable_nort()	do { BUG_ON(in_interrupt()); } while (0)
# define local_irq_save_nort(flags)	do { local_save_flags(flags); WARN_ON(in_interrupt()); } while (0)
# define local_irq_restore_nort(flags)	do { (void)(flags); WARN_ON(in_interrupt()); } while (0)
# define spin_lock_nort(lock)		do { } while (0)
# define spin_unlock_nort(lock)		do { } while (0)
# define spin_lock_bh_nort(lock)	do { } while (0)
# define spin_unlock_bh_nort(lock)	do { } while (0)
# define spin_lock_rt(lock)		spin_lock(lock)
# define spin_unlock_rt(lock)		spin_unlock(lock)
# define smp_processor_id_rt(cpu)	(cpu)
# define in_atomic_rt()			(!oops_in_progress && \
					  (in_atomic() || irqs_disabled()))
# define read_trylock_rt(lock)		({read_lock(lock); 1; })
#else
# define local_irq_disable_nort()	local_irq_disable()
# define local_irq_enable_nort()	local_irq_enable()
# define local_irq_save_nort(flags)	local_irq_save(flags)
# define local_irq_restore_nort(flags)	local_irq_restore(flags)
# define spin_lock_rt(lock)		do { } while (0)
# define spin_unlock_rt(lock)		do { } while (0)
# define spin_lock_nort(lock)		spin_lock(lock)
# define spin_unlock_nort(lock)		spin_unlock(lock)
# define spin_lock_bh_nort(lock)	spin_lock_bh(lock)
# define spin_unlock_bh_nort(lock)	spin_unlock_bh(lock)
# define smp_processor_id_rt(cpu)	smp_processor_id()
# define in_atomic_rt()			0
# define read_trylock_rt(lock)		read_trylock(lock)
#endif

#endif
