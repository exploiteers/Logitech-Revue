/* ltt-probe-kernel.c
 *
 * Loads a function at a marker call site.
 *
 * (C) Copyright 2006 Mathieu Desnoyers <mathieu.desnoyers@polymtl.ca>
 *
 * This file is released under the GPLv2.
 * See the file COPYING for more details.
 */

#include <linux/marker.h>
#include <linux/module.h>
#include <linux/ptrace.h>
#include <linux/time.h>
#include <ltt/ltt-facility-select-default.h>
#include <ltt/ltt-facility-select-kernel.h>
#include <ltt/ltt-facility-select-process.h>
#include <ltt/ltt-facility-kernel.h>
#include <ltt/ltt-facility-process.h>
#include <ltt/ltt-facility-timer.h>
#include <ltt/ltt-stack.h>

#define KERNEL_THREAD_CREATE_FORMAT "%ld %p"
void probe_kernel_thread_create(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	long pid;
	void *fn;

	/* Assign args */
	va_start(ap, format);
	pid = va_arg(ap, typeof(pid));
	fn = va_arg(ap, typeof(fn));

	/* Call tracer */
	trace_process_kernel_thread(pid, fn);
	
	va_end(ap);
}

#define KERNEL_SCHED_TRY_WAKEUP_FORMAT "%d %ld"
void probe_kernel_sched_try_wakeup(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	int pid;
	long state;

	/* Assign args */
	va_start(ap, format);
	pid = va_arg(ap, typeof(pid));
	state = va_arg(ap, typeof(state));

	/* Call tracer */
	trace_process_wakeup(pid, state);
	
	va_end(ap);
}

#define KERNEL_SCHED_WAKEUP_NEW_TASK_FORMAT "%d %ld"
void probe_kernel_sched_wakeup_new_task(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	int pid;
	long state;

	/* Assign args */
	va_start(ap, format);
	pid = va_arg(ap, typeof(pid));
	state = va_arg(ap, typeof(state));

	/* Call tracer */
	trace_process_wakeup(pid, state);
	
	va_end(ap);
}


#define KERNEL_SCHED_SCHEDULE_FORMAT "%d %d %ld"
void probe_kernel_sched_schedule(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	int prev_pid, next_pid;
	long state;

	/* Assign args */
	va_start(ap, format);
	prev_pid = va_arg(ap, typeof(prev_pid));
	next_pid = va_arg(ap, typeof(next_pid));
	state = va_arg(ap, typeof(state));

	/* Call tracer */
	trace_process_schedchange(prev_pid, next_pid, state);
	
	va_end(ap);
}

#define KERNEL_SCHED_WAIT_TASK_FORMAT "%d %ld"
void probe_kernel_sched_wait_task(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	int pid;
	long state;

	/* Assign args */
	va_start(ap, format);
	pid = va_arg(ap, typeof(pid));
	state = va_arg(ap, typeof(state));

	/* Call tracer */
	trace_process_wakeup(pid, state);
	
	va_end(ap);
}

#define KERNEL_SCHED_MIGRATE_TASK_FORMAT "%d %ld %d"
void probe_kernel_sched_migrate_task(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	int pid;
	long state;
	int dest_cpu;

	/* Assign args */
	va_start(ap, format);
	pid = va_arg(ap, typeof(pid));
	state = va_arg(ap, typeof(state));
	dest_cpu = va_arg(ap, typeof(dest_cpu));

	/* Call tracer */
	trace_process_wakeup(pid, state);
	/* TODO Create a specific event that includes the dest cpu */
	
	va_end(ap);
}

#define KERNEL_PROCESS_FORK_FORMAT "%d %d %d"
void probe_kernel_process_fork(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	int parent_pid, child_pid, child_tgid;

	/* Assign args */
	va_start(ap, format);
	parent_pid = va_arg(ap, typeof(parent_pid));
	child_pid = va_arg(ap, typeof(child_pid));
	child_tgid = va_arg(ap, typeof(child_tgid));

	/* Call tracer */
	trace_process_fork(parent_pid, child_pid, child_tgid);
	
	va_end(ap);
}

#define KERNEL_PRINTK_FORMAT "%p"
void probe_kernel_printk(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	void *ip;

	/* Assign args */
	va_start(ap, format);
	ip = va_arg(ap, typeof(ip));

	/* Call tracer */
	trace_kernel_printk(ip);
	
	va_end(ap);
}

#define KERNEL_VPRINTK_FORMAT "%hu %d %s %p"
void probe_kernel_vprintk(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	unsigned int loglevel;
	int len;
	const char *buf;
	void *ip;
	lttng_sequence_kernel_vprintk_text seq;

	/* Assign args */
	va_start(ap, format);
	loglevel = va_arg(ap, typeof(loglevel));
	len = va_arg(ap, typeof(len));
	buf = va_arg(ap, typeof(buf));
	ip = va_arg(ap, typeof(ip));
	seq.array = buf;
	seq.len = len;

	/* Call tracer */
	trace_kernel_vprintk(loglevel, &seq, ip);
	
	va_end(ap);
}


#define KERNEL_PROCESS_FREE_FORMAT "%d"
void probe_kernel_process_free(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	int pid;

	/* Assign args */
	va_start(ap, format);
	pid = va_arg(ap, typeof(pid));

	/* Call tracer */
	trace_process_free(pid);
	
	va_end(ap);
}

#define KERNEL_PROCESS_WAIT_FORMAT "%d"
void probe_kernel_process_wait(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	int pid;

	/* Assign args */
	va_start(ap, format);
	pid = va_arg(ap, typeof(pid));

	/* Call tracer */
	trace_process_wait(current->pid, pid);
	
	va_end(ap);
}

#define KERNEL_PROCESS_EXIT_FORMAT "%d"
void probe_kernel_process_exit(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	int pid;

	/* Assign args */
	va_start(ap, format);
	pid = va_arg(ap, typeof(pid));

	/* Call tracer */
	trace_process_exit(pid);
	
	va_end(ap);
}



#define KERNEL_TIMER_SET_ITIMER_FORMAT "%d %ld %ld %ld %ld"
void probe_kernel_timer_set_itimer(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	int which;
	long interval_tv_sec, interval_tv_usec, value_tv_sec, value_tv_usec;

	/* Assign args */
	va_start(ap, format);
	which = va_arg(ap, typeof(which));
	interval_tv_sec = va_arg(ap, typeof(interval_tv_sec));
	interval_tv_usec = va_arg(ap, typeof(interval_tv_usec));
	value_tv_sec = va_arg(ap, typeof(value_tv_sec));
	value_tv_usec = va_arg(ap, typeof(value_tv_usec));

	/* Call tracer */
	trace_timer_set_itimer(which, interval_tv_sec, interval_tv_usec,
		value_tv_sec, value_tv_usec);
	
	va_end(ap);
}

#define KERNEL_TIMER_EXPIRED_FORMAT "%d"
void probe_kernel_timer_expired(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	int pid;

	/* Assign args */
	va_start(ap, format);
	pid = va_arg(ap, typeof(pid));

	/* Call tracer */
	trace_timer_expired(pid);
	
	va_end(ap);
}

#define KERNEL_SOFTIRQ_ENTRY_FORMAT "%lu"
void probe_kernel_softirq_entry(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	unsigned long softirq_num;

	/* Assign args */
	va_start(ap, format);
	softirq_num = va_arg(ap, typeof(softirq_num));

	/* Call tracer */
	trace_kernel_soft_irq_entry(softirq_num);
	
	va_end(ap);
}

#define KERNEL_SOFTIRQ_EXIT_FORMAT "%lu"
void probe_kernel_softirq_exit(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	unsigned long softirq_num;

	/* Assign args */
	va_start(ap, format);
	softirq_num = va_arg(ap, typeof(softirq_num));

	/* Call tracer */
	trace_kernel_soft_irq_exit(softirq_num);
	
	va_end(ap);
}

#define KERNEL_TASKLET_LOW_ENTRY_FORMAT "%p %lu"
void probe_kernel_tasklet_low_entry(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	void *address;
	unsigned long data;
	enum lttng_tasklet_priority prio;

	/* Assign args */
	va_start(ap, format);
	address = va_arg(ap, typeof(address));
	data = va_arg(ap, typeof(data));

	/* Extra args */
	prio = LTTNG_LOW;

	/* Call tracer */
	trace_kernel_tasklet_entry(prio, address, data);
	
	va_end(ap);
}

#define KERNEL_TASKLET_LOW_EXIT_FORMAT "%p %lu"
void probe_kernel_tasklet_low_exit(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	void *address;
	unsigned long data;
	enum lttng_tasklet_priority prio;

	/* Assign args */
	va_start(ap, format);
	address = va_arg(ap, typeof(address));
	data = va_arg(ap, typeof(data));

	/* Extra args */
	prio = LTTNG_LOW;

	/* Call tracer */
	trace_kernel_tasklet_exit(prio, address, data);
	
	va_end(ap);
}

#define KERNEL_TASKLET_HIGH_ENTRY_FORMAT "%p %lu"
void probe_kernel_tasklet_high_entry(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	void *address;
	unsigned long data;
	enum lttng_tasklet_priority prio;

	/* Assign args */
	va_start(ap, format);
	address = va_arg(ap, typeof(address));
	data = va_arg(ap, typeof(data));

	/* Extra args */
	prio = LTTNG_HIGH;

	/* Call tracer */
	trace_kernel_tasklet_entry(prio, address, data);
	
	va_end(ap);
}

#define KERNEL_TASKLET_HIGH_EXIT_FORMAT "%p %lu"
void probe_kernel_tasklet_high_exit(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	void *address;
	unsigned long data;
	enum lttng_tasklet_priority prio;

	/* Assign args */
	va_start(ap, format);
	address = va_arg(ap, typeof(address));
	data = va_arg(ap, typeof(data));

	/* Extra args */
	prio = LTTNG_HIGH;

	/* Call tracer */
	trace_kernel_tasklet_exit(prio, address, data);
	
	va_end(ap);
}

#define KERNEL_TIMER_UPDATE_TIME_FORMAT \
	"%llu struct timespec %p struct timespec %p struct pt_regs %p"
void probe_kernel_timer_update_time(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	unsigned long long jiffies;
	struct timespec *wall;
	struct timespec *wall_to_monotonic;
	struct pt_regs *regs;

	/* Assign args */
	va_start(ap, format);
	jiffies = va_arg(ap, typeof(jiffies));
	wall = va_arg(ap, typeof(wall));
	wall_to_monotonic = va_arg(ap, typeof(wall_to_monotonic));
	regs = va_arg(ap, typeof(regs));

	/* Call tracer */
	trace_timer_update_time(jiffies, wall->tv_sec, wall->tv_nsec,
		wall_to_monotonic->tv_sec, wall_to_monotonic->tv_nsec);

	/* Other things */
#ifdef CONFIG_LTT_STACK_INTERRUPT
	do_trace_kernel_stack_dump(regs);
#endif
	va_end(ap);
}

#define KERNEL_TIMER_TIMEOUT_FORMAT "%d"
void probe_kernel_timer_timeout(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	int pid;

	/* Assign args */
	va_start(ap, format);
	pid = va_arg(ap, typeof(pid));

	/* Call tracer */
	trace_timer_expired(pid);
	
	va_end(ap);
}

#define KERNEL_TIMER_SET_FORMAT "%lu %p %lu"
void probe_kernel_timer_set(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	unsigned long expires;
	void *function;
	unsigned long data;

	/* Assign args */
	va_start(ap, format);
	expires = va_arg(ap, typeof(expires));
	function = va_arg(ap, typeof(function));
	data = va_arg(ap, typeof(data));

	/* Call tracer */
	trace_timer_set_timer(expires, function, data);
	
	va_end(ap);
}

#define KERNEL_SIGNAL_FORMAT "%d %d"
void probe_kernel_signal(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	int pid;
	int sig;

	/* Assign args */
	va_start(ap, format);
	pid = va_arg(ap, typeof(pid));
	sig = va_arg(ap, typeof(sig));

	/* Call tracer */
	trace_process_signal(pid, sig);
	
	va_end(ap);
}


#define KERNEL_KTHREAD_STOP_FORMAT "%d"
void probe_kernel_kthread_stop(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	int pid;

	/* Assign args */
	va_start(ap, format);
	pid = va_arg(ap, typeof(pid));

	/* Call tracer */
	/* TODO */

	va_end(ap);
}

#define KERNEL_KTHREAD_STOP_RET_FORMAT "%d"
void probe_kernel_kthread_stop_ret(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	int ret;

	/* Assign args */
	va_start(ap, format);
	ret = va_arg(ap, typeof(ret));

	/* Call tracer */
	/* TODO */

	va_end(ap);
}

#define KERNEL_MODULE_FREE_FORMAT "%s"
void probe_kernel_module_free(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	const char *name;

	/* Assign args */
	va_start(ap, format);
	name = va_arg(ap, typeof(name));

	/* Call tracer */
	trace_kernel_module_free(name);
	
	va_end(ap);
}

#define KERNEL_MODULE_LOAD_FORMAT "%s"
void probe_kernel_module_load(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	const char *name;

	/* Assign args */
	va_start(ap, format);
	name = va_arg(ap, typeof(name));

	/* Call tracer */
	trace_kernel_module_load(name);
	
	va_end(ap);
}


#define KERNEL_IRQ_ENTRY_FORMAT "%u %u"
void probe_kernel_irq_entry(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	unsigned int irq;
	unsigned int mode;

	/* Assign args */
	va_start(ap, format);
	irq = va_arg(ap, typeof(irq));
	mode = va_arg(ap, typeof(mode));

	/* Call tracer */
	trace_kernel_irq_entry(irq, (enum lttng_irq_mode)mode);
	
	va_end(ap);
}

#define KERNEL_IRQ_EXIT_FORMAT MARK_NOARGS
void probe_kernel_irq_exit(const char *format, ...)
{
	/* Call tracer */
	trace_kernel_irq_exit();
}

static int __init probe_init(void)
{
	int result;
	result = marker_set_probe("kernel_thread_create",
			KERNEL_THREAD_CREATE_FORMAT,
			probe_kernel_thread_create);
	if (!result)
		goto cleanup;
	result = marker_set_probe("kernel_sched_try_wakeup",
			KERNEL_SCHED_TRY_WAKEUP_FORMAT,
			probe_kernel_sched_try_wakeup);
	if (!result)
		goto cleanup;
	result = marker_set_probe("kernel_sched_wakeup_new_task",
			KERNEL_SCHED_WAKEUP_NEW_TASK_FORMAT,
			probe_kernel_sched_wakeup_new_task);
	if (!result)
		goto cleanup;
	result = marker_set_probe("kernel_sched_schedule",
			KERNEL_SCHED_SCHEDULE_FORMAT,
			probe_kernel_sched_schedule);
	if (!result)
		goto cleanup;
#ifdef CONFIG_SMP
	result = marker_set_probe("kernel_sched_wait_task",
			KERNEL_SCHED_WAIT_TASK_FORMAT,
			probe_kernel_sched_wait_task);
	if (!result)
		goto cleanup;
	result = marker_set_probe("kernel_sched_migrate_task",
			KERNEL_SCHED_MIGRATE_TASK_FORMAT,
			probe_kernel_sched_migrate_task);
	if (!result)
		goto cleanup;
#endif

	result = marker_set_probe("kernel_process_fork",
			KERNEL_PROCESS_FORK_FORMAT,
			probe_kernel_process_fork);
	if (!result)
		goto cleanup;
	result = marker_set_probe("kernel_printk",
			KERNEL_PRINTK_FORMAT,
			probe_kernel_printk);
	if (!result)
		goto cleanup;
	result = marker_set_probe("kernel_vprintk",
			KERNEL_VPRINTK_FORMAT,
			probe_kernel_vprintk);
	if (!result)
		goto cleanup;
	result = marker_set_probe("kernel_process_free",
			KERNEL_PROCESS_FREE_FORMAT,
			probe_kernel_process_free);
	if (!result)
		goto cleanup;
	result = marker_set_probe("kernel_process_wait",
			KERNEL_PROCESS_WAIT_FORMAT,
			probe_kernel_process_wait);
	if (!result)
		goto cleanup;
	result = marker_set_probe("kernel_process_exit",
			KERNEL_PROCESS_EXIT_FORMAT,
			probe_kernel_process_exit);
	if (!result)
		goto cleanup;
	result = marker_set_probe("kernel_timer_set_itimer",
			KERNEL_TIMER_SET_ITIMER_FORMAT,
			probe_kernel_timer_set_itimer);
	if (!result)
		goto cleanup;
	result = marker_set_probe("kernel_timer_expired",
			KERNEL_TIMER_EXPIRED_FORMAT,
			probe_kernel_timer_expired);
	if (!result)
		goto cleanup;
	result = marker_set_probe("kernel_softirq_entry",
			KERNEL_SOFTIRQ_ENTRY_FORMAT,
			probe_kernel_softirq_entry);
	if (!result)
		goto cleanup;
	result = marker_set_probe("kernel_softirq_exit",
			KERNEL_SOFTIRQ_EXIT_FORMAT,
			probe_kernel_softirq_exit);
	if (!result)
		goto cleanup;
	result = marker_set_probe("kernel_tasklet_low_entry",
			KERNEL_TASKLET_LOW_ENTRY_FORMAT,
			probe_kernel_tasklet_low_entry);
	if (!result)
		goto cleanup;
	result = marker_set_probe("kernel_tasklet_low_exit",
			KERNEL_TASKLET_LOW_EXIT_FORMAT,
			probe_kernel_tasklet_low_exit);
	if (!result)
		goto cleanup;
	result = marker_set_probe("kernel_tasklet_high_entry",
			KERNEL_TASKLET_HIGH_ENTRY_FORMAT,
			probe_kernel_tasklet_high_entry);
	if (!result)
		goto cleanup;
	result = marker_set_probe("kernel_tasklet_high_exit",
			KERNEL_TASKLET_HIGH_EXIT_FORMAT,
			probe_kernel_tasklet_high_exit);
	if (!result)
		goto cleanup;
	/* TODO: marked as TODO in kernel/timer.c */
/* 	result = marker_set_probe("kernel_timer_update_time", */
/* 			KERNEL_TIMER_UPDATE_TIME_FORMAT, */
/* 			probe_kernel_timer_update_time); */
/* 	if (!result) */
/* 		goto cleanup; */
	result = marker_set_probe("kernel_timer_timeout",
			KERNEL_TIMER_TIMEOUT_FORMAT,
			probe_kernel_timer_timeout);
	if (!result)
		goto cleanup;
	result = marker_set_probe("kernel_timer_set",
			KERNEL_TIMER_SET_FORMAT,
			probe_kernel_timer_set);
	if (!result)
		goto cleanup;
	result = marker_set_probe("kernel_signal",
			KERNEL_SIGNAL_FORMAT,
			probe_kernel_signal);
	if (!result)
		goto cleanup;
	result = marker_set_probe("kernel_kthread_stop",
			KERNEL_KTHREAD_STOP_FORMAT,
			probe_kernel_kthread_stop);
	if (!result)
		goto cleanup;
	result = marker_set_probe("kernel_kthread_stop_ret",
			KERNEL_KTHREAD_STOP_RET_FORMAT,
			probe_kernel_kthread_stop_ret);
	if (!result)
		goto cleanup;
	result = marker_set_probe("kernel_module_free",
			KERNEL_MODULE_FREE_FORMAT,
			probe_kernel_module_free);
	if (!result)
		goto cleanup;
	result = marker_set_probe("kernel_module_load",
			KERNEL_MODULE_LOAD_FORMAT,
			probe_kernel_module_load);
	if (!result)
		goto cleanup;
	result = marker_set_probe("kernel_irq_entry",
			KERNEL_IRQ_ENTRY_FORMAT,
			probe_kernel_irq_entry);
	if (!result)
		goto cleanup;
	result = marker_set_probe("kernel_irq_exit",
			KERNEL_IRQ_EXIT_FORMAT,
			probe_kernel_irq_exit);
	if (!result)
		goto cleanup;

	return 0;

cleanup:
	marker_remove_probe(probe_kernel_thread_create);
	marker_remove_probe(probe_kernel_sched_try_wakeup);
	marker_remove_probe(probe_kernel_sched_wakeup_new_task);
	marker_remove_probe(probe_kernel_sched_schedule);
	marker_remove_probe(probe_kernel_sched_wait_task);
	marker_remove_probe(probe_kernel_sched_migrate_task);
	marker_remove_probe(probe_kernel_process_fork);
	marker_remove_probe(probe_kernel_printk);
	marker_remove_probe(probe_kernel_vprintk);
	marker_remove_probe(probe_kernel_process_free);
	marker_remove_probe(probe_kernel_process_wait);
	marker_remove_probe(probe_kernel_process_exit);
	marker_remove_probe(probe_kernel_timer_set_itimer);
	marker_remove_probe(probe_kernel_timer_expired);
	marker_remove_probe(probe_kernel_softirq_entry);
	marker_remove_probe(probe_kernel_softirq_exit);
	marker_remove_probe(probe_kernel_tasklet_low_entry);
	marker_remove_probe(probe_kernel_tasklet_low_exit);
	marker_remove_probe(probe_kernel_tasklet_high_entry);
	marker_remove_probe(probe_kernel_tasklet_high_exit);
	marker_remove_probe(probe_kernel_timer_update_time);
	marker_remove_probe(probe_kernel_timer_timeout);
	marker_remove_probe(probe_kernel_timer_set);
	marker_remove_probe(probe_kernel_signal);
	marker_remove_probe(probe_kernel_kthread_stop);
	marker_remove_probe(probe_kernel_kthread_stop_ret);
	marker_remove_probe(probe_kernel_module_free);
	marker_remove_probe(probe_kernel_module_load);
	marker_remove_probe(probe_kernel_irq_entry);
	marker_remove_probe(probe_kernel_irq_exit);
	return -EPERM;
}

static void __exit probe_fini(void)
{
	marker_remove_probe(probe_kernel_thread_create);
	marker_remove_probe(probe_kernel_sched_try_wakeup);
	marker_remove_probe(probe_kernel_sched_wakeup_new_task);
	marker_remove_probe(probe_kernel_sched_schedule);
	marker_remove_probe(probe_kernel_sched_wait_task);
	marker_remove_probe(probe_kernel_sched_migrate_task);
	marker_remove_probe(probe_kernel_process_fork);
	marker_remove_probe(probe_kernel_printk);
	marker_remove_probe(probe_kernel_vprintk);
	marker_remove_probe(probe_kernel_process_free);
	marker_remove_probe(probe_kernel_process_wait);
	marker_remove_probe(probe_kernel_process_exit);
	marker_remove_probe(probe_kernel_timer_set_itimer);
	marker_remove_probe(probe_kernel_timer_expired);
	marker_remove_probe(probe_kernel_softirq_entry);
	marker_remove_probe(probe_kernel_softirq_exit);
	marker_remove_probe(probe_kernel_tasklet_low_entry);
	marker_remove_probe(probe_kernel_tasklet_low_exit);
	marker_remove_probe(probe_kernel_tasklet_high_entry);
	marker_remove_probe(probe_kernel_tasklet_high_exit);
	marker_remove_probe(probe_kernel_timer_update_time);
	marker_remove_probe(probe_kernel_timer_timeout);
	marker_remove_probe(probe_kernel_timer_set);
	marker_remove_probe(probe_kernel_signal);
	marker_remove_probe(probe_kernel_kthread_stop);
	marker_remove_probe(probe_kernel_kthread_stop_ret);
	marker_remove_probe(probe_kernel_module_free);
	marker_remove_probe(probe_kernel_module_load);
	marker_remove_probe(probe_kernel_irq_entry);
	marker_remove_probe(probe_kernel_irq_exit);
}


module_init(probe_init);
module_exit(probe_fini);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mathieu Desnoyers");
MODULE_DESCRIPTION("Kernel Probe");

