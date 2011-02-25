/*
 * kernel/time/timer_stats.c
 *
 * Collect timer usage statistics.
 *
 * Copyright(C) 2006, Red Hat, Inc., Ingo Molnar
 * Copyright(C) 2006 Timesys Corp., Thomas Gleixner <tglx@timesys.com>
 *
 * timer_stats is based on timer_top, a similar functionality which was part of
 * Con Kolivas dyntick patch set. It was developed by Daniel Petrini at the
 * Instituto Nokia de Tecnologia - INdT - Manaus. timer_top's design was based
 * on dynamic allocation of the statistics entries rather than the static array
 * which is used by timer_stats. It was written for the pre hrtimer kernel code
 * and therefor did not take hrtimers into account. Nevertheless it provided
 * the base for the timer_stats implementation and was a helpful source of
 * inspiration in the first place. Kudos to Daniel and the Nokia folks for this
 * effort.
 *
 * timer_top.c is
 *	Copyright (C) 2005 Instituto Nokia de Tecnologia - INdT - Manaus
 *	Written by Daniel Petrini <d.pensator@gmail.com>
 *	timer_top.c was released under the GNU General Public License version 2
 *
 * We export the addresses and counting of timer functions being called,
 * the pid and cmdline from the owner process if applicable.
 *
 * Start/stop data collection:
 * # echo 1[0] >/proc/timer_stats
 *
 * Display the collected information:
 * # cat /proc/timer_stats
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/list.h>
#include <linux/proc_fs.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/seq_file.h>
#include <linux/kallsyms.h>

#include <asm/uaccess.h>

enum tstats_stat {
	TSTATS_INACTIVE,
	TSTATS_ACTIVE,
	TSTATS_READOUT,
	TSTATS_RESET,
};

struct tstats_entry {
	void			*timer;
	void			*start_func;
	void			*expire_func;
	unsigned long		counter;
	pid_t			pid;
	char			comm[TASK_COMM_LEN + 1];
};

#define TSTATS_MAX_ENTRIES	1024

static struct tstats_entry tstats[TSTATS_MAX_ENTRIES];
static DEFINE_RAW_SPINLOCK(tstats_lock);
static enum tstats_stat tstats_status;
static ktime_t tstats_time;

/**
 * timer_stats_update_stats - Update the statistics for a timer.
 * @timer:	pointer to either a timer_list or a hrtimer
 * @pid:	the pid of the task which set up the timer
 * @startf:	pointer to the function which did the timer setup
 * @timerf:	pointer to the timer callback function of the timer
 * @comm:	name of the process which set up the timer
 *
 * When the timer is already registered, then the event counter is
 * incremented. Otherwise the timer is registered in a free slot.
 */
void timer_stats_update_stats(void *timer, pid_t pid, void *startf,
			      void *timerf, char * comm)
{
	struct tstats_entry *entry = tstats;
	unsigned long flags;
	int i;

	spin_lock_irqsave(&tstats_lock, flags);
	if (tstats_status != TSTATS_ACTIVE)
		goto out_unlock;

	for (i = 0; i < TSTATS_MAX_ENTRIES; i++, entry++) {
		if (entry->timer == timer &&
		    entry->start_func == startf &&
		    entry->expire_func == timerf &&
		    entry->pid == pid) {

			entry->counter++;
			break;
		}
		if (!entry->timer) {
			entry->timer = timer;
			entry->start_func = startf;
			entry->expire_func = timerf;
			entry->counter = 1;
			entry->pid = pid;
			memcpy(entry->comm, comm, TASK_COMM_LEN);
			entry->comm[TASK_COMM_LEN] = 0;
			break;
		}
	}

 out_unlock:
	spin_unlock_irqrestore(&tstats_lock, flags);
}

static void print_name_offset(struct seq_file *m, unsigned long addr)
{
	char namebuf[KSYM_NAME_LEN+1];
	unsigned long size, offset;
	const char *sym_name;
	char *modname;

	sym_name = kallsyms_lookup(addr, &size, &offset, &modname, namebuf);
	if (sym_name)
		seq_printf(m, "%s", sym_name);
	else
		seq_printf(m, "<%p>", (void *)addr);
}

static int tstats_show(struct seq_file *m, void *v)
{
	struct tstats_entry *entry = tstats;
	struct timespec period;
	unsigned long ms;
	long events = 0;
	int i;

	spin_lock_irq(&tstats_lock);
	switch(tstats_status) {
	case TSTATS_ACTIVE:
		tstats_time = ktime_sub(ktime_get(), tstats_time);
	case TSTATS_INACTIVE:
		tstats_status = TSTATS_READOUT;
		break;
	default:
		spin_unlock_irq(&tstats_lock);
		return -EBUSY;
	}
	spin_unlock_irq(&tstats_lock);

	period = ktime_to_timespec(tstats_time);
	ms = period.tv_nsec % 1000000;

	seq_printf(m, "Timerstats sample period: %ld.%3ld s\n",
		   period.tv_sec, ms);

	for (i = 0; i < TSTATS_MAX_ENTRIES && entry->timer; i++, entry++) {
		seq_printf(m, "%4lu, %5d %-16s ", entry->counter, entry->pid,
			   entry->comm);

		print_name_offset(m, (unsigned long)entry->start_func);
		seq_puts(m, " (");
		print_name_offset(m, (unsigned long)entry->expire_func);
		seq_puts(m, ")\n");
		events += entry->counter;
	}

	ms += period.tv_sec * 1000;
	if (events && period.tv_sec)
		seq_printf(m, "%ld total events, %ld.%ld events/sec\n", events,
			   events / period.tv_sec, events * 1000 / ms);
	else
		seq_printf(m, "%ld total events\n", events);

	tstats_status = TSTATS_INACTIVE;
	return 0;
}

static ssize_t tstats_write(struct file *file, const char __user *buf,
			    size_t count, loff_t *offs)
{
	char ctl[2];

	if (count != 2 || *offs)
		return -EINVAL;

	if (copy_from_user(ctl, buf, count))
		return -EFAULT;

	switch (ctl[0]) {
	case '0':
		spin_lock_irq(&tstats_lock);
		if (tstats_status == TSTATS_ACTIVE) {
			tstats_status = TSTATS_INACTIVE;
			tstats_time = ktime_sub(ktime_get(), tstats_time);
		}
		spin_unlock_irq(&tstats_lock);
		break;
	case '1':
		spin_lock_irq(&tstats_lock);
		if (tstats_status == TSTATS_INACTIVE) {
			tstats_status = TSTATS_RESET;
			memset(tstats, 0, sizeof(tstats));
			tstats_time = ktime_get();
			tstats_status = TSTATS_ACTIVE;
		}
		spin_unlock_irq(&tstats_lock);
		break;
	default:
		count = -EINVAL;
	}

	return count;
}

static int tstats_open(struct inode *inode, struct file *filp)
{
	return single_open(filp, tstats_show, NULL);
}

static struct file_operations tstats_fops = {
	.open		= tstats_open,
	.read		= seq_read,
	.write		= tstats_write,
	.llseek		= seq_lseek,
	.release	= seq_release,
};

static int __init init_tstats(void)
{
	struct proc_dir_entry *pe;

	pe = create_proc_entry("timer_stats", 0666, NULL);

	if (!pe)
		return -ENOMEM;

	pe->proc_fops = &tstats_fops;

	return 0;
}
module_init(init_tstats);
