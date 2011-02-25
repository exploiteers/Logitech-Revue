/*
 * ltt-heartbeat.c
 *
 * (C) Copyright	2006 -
 * 		Mathieu Desnoyers (mathieu.desnoyers@polymtl.ca)
 *
 * notes : heartbeat timer cannot be used for early tracing in the boot process,
 * as it depends on timer interrupts.
 *
 * The timer needs to be only on one CPU to support hotplug.
 * We have the choice between schedule_delayed_work_on and an IPI to get each
 * CPU to write the heartbeat. IPI have been chosen because it is considered
 * faster than passing through the timer to get the work scheduled on all the
 * CPUs.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/ltt-core.h>
#include <linux/workqueue.h>
#include <linux/cpu.h>
#include <linux/timex.h>
#include <ltt/ltt-facility-select-core.h>
#include <ltt/ltt-facility-core.h>

/* How often the LTT per-CPU timers fire */
#define LTT_PERCPU_TIMER_FREQ  (HZ/10)

static struct timer_list heartbeat_timer;
static unsigned int precalc_heartbeat_expire = 0;

#ifdef CONFIG_LTT_SYNTHETIC_TSC
/* For architectures with 32 bits TSC */
static struct synthetic_tsc_struct {
	u32 tsc[2][2];	/* a pair of 2 32 bits. [0] is the MSB, [1] is LSB */
	unsigned int index;	/* Index of the current synth. tsc. */
} ____cacheline_aligned synthetic_tsc[NR_CPUS];

/* Called from one CPU, before any tracing starts, to init each structure */
static void ltt_heartbeat_init_synthetic_tsc(void)
{
	int cpu;
	for (cpu = 0; cpu < NR_CPUS; cpu++) {
		synthetic_tsc[cpu].tsc[0][0] = 0;	
		synthetic_tsc[cpu].tsc[0][1] = 0;	
		synthetic_tsc[cpu].tsc[1][0] = 0;
		synthetic_tsc[cpu].tsc[1][1] = 0;
		synthetic_tsc[cpu].index = 0;
	}
}

/* Called from heartbeat IPI : either in interrupt or process context */
static void ltt_heartbeat_update_synthetic_tsc(void)
{
	struct synthetic_tsc_struct *cpu_synth;
	u32 tsc;

	preempt_disable();
	cpu_synth = &synthetic_tsc[smp_processor_id()];
	tsc = (u32)get_cycles();	/* We deal with a 32 LSB TSC */

	if (tsc < cpu_synth->tsc[cpu_synth->index][1]) {
		unsigned int new_index = cpu_synth->index ? 0 : 1; /* 0 <-> 1 */
		/* Overflow */
		/* Non atomic update of the non current synthetic TSC, followed
		 * by an atomic index change. There is no write concurrency,
		 * so the index read/write does not need to be atomic. */
		cpu_synth->tsc[new_index][1] = tsc; /* LSB update */
		cpu_synth->tsc[new_index][0] =
			cpu_synth->tsc[cpu_synth->index][0]+1; /* MSB update */
		cpu_synth->index = new_index;	/* atomic change of index */
	} else {
		/* No overflow : we can simply update the 32 LSB of the current
		 * synthetic TSC as it's an atomic write. */
		cpu_synth->tsc[cpu_synth->index][1] = tsc;
	}
	preempt_enable();
}

/* Called from buffer switch : in _any_ context (even NMI) */
u64 ltt_heartbeat_read_synthetic_tsc(void)
{
	struct synthetic_tsc_struct *cpu_synth;
	u64 ret;
	unsigned int index;
	u32 tsc;

	preempt_disable();
	cpu_synth = &synthetic_tsc[smp_processor_id()];
	index = cpu_synth->index; /* atomic read */
	tsc = (u32)get_cycles();	/* We deal with a 32 LSB TSC */

	if (tsc < cpu_synth->tsc[index][1]) {
		/* Overflow */
		ret = ((u64)(cpu_synth->tsc[index][0]+1) << 32) | ((u64)tsc);
	} else {
		/* no overflow */
		ret = ((u64)cpu_synth->tsc[index][0] << 32) | ((u64)tsc);
	}
	preempt_enable();
	return ret;
}
EXPORT_SYMBOL_GPL(ltt_heartbeat_read_synthetic_tsc);
#endif //CONFIG_LTT_SYNTHETIC_TSC


static void heartbeat_ipi(void *info)
{
#ifdef CONFIG_LTT_SYNTHETIC_TSC
	ltt_heartbeat_update_synthetic_tsc();
#endif //CONFIG_LTT_SYNTHETIC_TSC

#ifdef CONFIG_LTT_HEARTBEAT_EVENT
	/* Log a heartbeat event for each trace, each tracefile */
	trace_core_time_heartbeat(GET_CHANNEL_INDEX(facilities));
	trace_core_time_heartbeat(GET_CHANNEL_INDEX(interrupts));
	trace_core_time_heartbeat(GET_CHANNEL_INDEX(processes));
	trace_core_time_heartbeat(GET_CHANNEL_INDEX(modules));
	trace_core_time_heartbeat(GET_CHANNEL_INDEX(cpu));
	trace_core_time_heartbeat(GET_CHANNEL_INDEX(network));
#endif //CONFIG_LTT_HEARTBEAT_EVENT
}

/* We need to be in process context to do an IPI */
static void heartbeat_work(void *dummy)
{
	on_each_cpu(heartbeat_ipi, NULL, 1, 0);
}

static DECLARE_WORK(hb_work, heartbeat_work, NULL);

/**
 * heartbeat_timer : - Timer function generating hearbeat.
 * @data: unused
 *
 * Guarantees at least 1 execution of heartbeat before low word of TSC wraps.
 */
static void heartbeat_timer_fct(unsigned long data)
{
	PREPARE_WORK(&hb_work, heartbeat_work, NULL);
	schedule_work(&hb_work);
	
	mod_timer(&heartbeat_timer, jiffies + precalc_heartbeat_expire);
}


/**
 * init_heartbeat_timer: - Start timer generating hearbeat events.
 */
static void init_heartbeat_timer(void)
{
	if (loops_per_jiffy > 0) {
		printk(KERN_DEBUG "LTT : ltt-heartbeat start\n");
		precalc_heartbeat_expire = ( 0xffffffffUL/(loops_per_jiffy << 1)
			- 1 - LTT_PERCPU_TIMER_FREQ) >> 1;

		heartbeat_work(NULL);

		init_timer(&heartbeat_timer);
		heartbeat_timer.function = heartbeat_timer_fct;
		heartbeat_timer.expires = jiffies + precalc_heartbeat_expire;
		add_timer(&heartbeat_timer);
	} else
		printk(KERN_WARNING
			"LTT: No TSC for heartbeat timer "
			"- continuing without one \n");
}

/**
 * delete_heartbeat_timer: - Stop timer generating hearbeat events.
 */
static void delete_heartbeat_timer(void)
{
 	if (loops_per_jiffy > 0) {
		printk(KERN_DEBUG "LTT : ltt-heartbeat stop\n");
		del_timer(&heartbeat_timer);
	}
}


int ltt_heartbeat_trigger(enum ltt_heartbeat_functor_msg msg)
{
	printk(KERN_DEBUG "LTT : ltt-heartbeat trigger\n");
	switch (msg) {
		case LTT_HEARTBEAT_START:
			init_heartbeat_timer();
			break;
		case LTT_HEARTBEAT_STOP:
			delete_heartbeat_timer();
			break;
	}
	return 0;
}

EXPORT_SYMBOL_GPL(ltt_heartbeat_trigger);

static int __init ltt_heartbeat_init(void)
{
	printk(KERN_INFO "LTT : ltt-heartbeat init\n");
#ifdef CONFIG_LTT_SYNTHETIC_TSC
	ltt_heartbeat_init_synthetic_tsc();
#endif //CONFIG_LTT_SYNTHETIC_TSC
	return 0;
}

__initcall(ltt_heartbeat_init);


