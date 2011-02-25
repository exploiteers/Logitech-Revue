/* ltt-statedump.c
 *
 * Linux Trace Toolkit Kernel State Dump
 *
 * Copyright 2005 -
 * Jean-Hugues Deschenes <jean-hugues.deschenes@polymtl.ca>
 *
 * Changes:
 *	Eric Clement:           Add listing of network IP interface
 *	Mathieu Desnoyers	Fix kernel threads
 */

#include <linux/config.h>
#include <linux/init.h>
#include <linux/module.h>
#include <ltt/ltt-tracer.h>
#include <linux/netlink.h>
#include <linux/inet.h>
#include <linux/ip.h>
#include <linux/kthread.h>
#include <linux/proc_fs.h>
#include <linux/file.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/cpu.h>
#include <linux/netdevice.h>
#include <linux/inetdevice.h>
#include <linux/sched.h>
#include <ltt/ltt-facility-select-statedump.h>
#include <ltt/ltt-facility-statedump.h>

#define NB_PROC_CHUNK 20

static atomic_t kernel_threads_to_run;
static struct work_struct cpu_work[NR_CPUS];


/* in modules.c */
extern void list_modules(void);

static void ltt_enumerate_device(struct net_device *dev)
{
	struct in_device *in_dev;
	struct in_ifaddr *ifa;

	if (dev->flags & IFF_UP) {	
		in_dev = in_dev_get(dev);
		if (in_dev) {
			for (ifa = in_dev->ifa_list;
					ifa != NULL;
					ifa = ifa->ifa_next) {
				trace_statedump_enumerate_network_ip_interface(
					dev->name, ifa->ifa_address, LTTNG_UP);
			}
			in_dev_put(in_dev);		
		}
	} else
		trace_statedump_enumerate_network_ip_interface(dev->name,
					0, LTTNG_DOWN);
}

static inline int ltt_enumerate_network_ip_interface(void)
{
	struct net_device *list;
	
	read_lock(&dev_base_lock);
	for (list = dev_base; list != NULL; list = list->next)
		ltt_enumerate_device(list);
	read_unlock(&dev_base_lock);
	
	return 0;
}

static inline void ltt_enumerate_task_fd(struct task_struct *t,
		char *tmp)
{
	struct fdtable *fdt;
	struct file * filp;
	unsigned int i;
	char *path;

	if (!t->files)
		return;

	spin_lock(&t->files->file_lock);
	fdt = files_fdtable(t->files);
	for (i = 0; i < fdt->max_fds; i++) {
		filp = fcheck_files(t->files, i);
		if (!filp)
			continue;
		path = d_path(filp->f_dentry,
				filp->f_vfsmnt, tmp, PAGE_SIZE);
		/* Make sure we give at least some info */
		if (IS_ERR(path))
			trace_statedump_enumerate_file_descriptors(
				filp->f_dentry->d_name.name, t->pid, i);
		else
			trace_statedump_enumerate_file_descriptors(
					path, t->pid, i);
	}
	spin_unlock(&t->files->file_lock);
}

static inline int ltt_enumerate_file_descriptors(void)
{
	struct task_struct * t = &init_task;
	char *tmp = (char*)__get_free_page(GFP_KERNEL);

	/* Enumerate active file descriptors */
	do {
		read_lock(&tasklist_lock);
		if (t != &init_task)
			atomic_dec(&t->usage);
		t = next_task(t);
		atomic_inc(&t->usage);
		read_unlock(&tasklist_lock);
		task_lock(t);
		ltt_enumerate_task_fd(t, tmp);
		task_unlock(t);
	} while (t != &init_task);
	free_page((unsigned long)tmp);
	return 0;
}

static inline void ltt_enumerate_task_vm_maps(struct task_struct *t)
{
	struct mm_struct *mm;
	struct vm_area_struct *	map;
	unsigned long ino;

	/* get_task_mm does a task_lock... */
	mm = get_task_mm(t);
	if (!mm)
		return;

	map = mm->mmap;
	if (map) {
		down_read(&mm->mmap_sem);
		while (map) {
			if (map->vm_file)
				ino = map->vm_file->f_dentry->d_inode->i_ino;
			else
				ino = 0;
			trace_statedump_enumerate_vm_maps(
					t->pid,
					(void *)map->vm_start,
					(void *)map->vm_end,
					map->vm_flags,
					map->vm_pgoff << PAGE_SHIFT,
					ino);
			map = map->vm_next;
		}
		up_read(&mm->mmap_sem);
	}
	mmput(mm);
}

static inline int ltt_enumerate_vm_maps(void)
{
	struct task_struct *  t = &init_task;
	
	do {
		read_lock(&tasklist_lock);
		if (t != &init_task)
			atomic_dec(&t->usage);
		t = next_task(t);
		atomic_inc(&t->usage);
		read_unlock(&tasklist_lock);
		ltt_enumerate_task_vm_maps(t);
 	} while (t != &init_task);
	return 0;
}

static inline void list_interrupts(void)
{
	unsigned int i;
	unsigned long flags = 0;

	/* needs irq_desc */
	for (i = 0; i < NR_IRQS; i++) {
		struct irqaction * action;
		const char *irq_chip_name =
			irq_desc[i].chip->name ? : "unnamed_irq_chip";

		spin_lock_irqsave(&irq_desc[i].lock, flags);
		for (action = irq_desc[i].action;
			action; action = action->next) {
			trace_statedump_enumerate_interrupts(
				irq_chip_name,
				action->name,
				i);
		}
		spin_unlock_irqrestore(&irq_desc[i].lock, flags);
	}
}

static inline int ltt_enumerate_process_states(void)
{
	struct task_struct *t = &init_task;
	struct task_struct *p = t;
	enum lttng_process_status status;
	enum lttng_thread_type type;
	enum lttng_execution_mode mode;
	enum lttng_execution_submode submode;
	
	do {
		mode = LTTNG_MODE_UNKNOWN;
		submode = LTTNG_UNKNOWN;

		read_lock(&tasklist_lock);
		if (t != &init_task) {
			atomic_dec(&t->usage);
			t = next_thread(t);
		}
		if (t == p) {
			t = p = next_task(t);
		}
		atomic_inc(&t->usage);
		read_unlock(&tasklist_lock);
		
		task_lock(t);
		
		if (t->exit_state == EXIT_ZOMBIE)
			status = LTTNG_ZOMBIE;
		else if (t->exit_state == EXIT_DEAD)
			status = LTTNG_DEAD;
		else if (t->state == TASK_RUNNING) {
			/* Is this a forked child that has not run yet? */
			if (list_empty(&t->run_list))
				status = LTTNG_WAIT_FORK;
			else
				/* All tasks are considered as wait_cpu;
				 * the viewer will sort out if the task was
				 * really running at this time. */
				status = LTTNG_WAIT_CPU;
		}
		else if (t->state & 
			(TASK_INTERRUPTIBLE | TASK_UNINTERRUPTIBLE)) {
			/* Task is waiting for something to complete */
			status = LTTNG_WAIT;
		}
		else
			status = LTTNG_UNNAMED;
		submode = LTTNG_NONE;

		/* Verification of t->mm is to filter out kernel threads;
		 * Viewer will further filter out if a user-space thread was
		 * in syscall mode or not */
		if (t->mm)
			type = LTTNG_USER_THREAD;
		else
			type = LTTNG_KERNEL_THREAD;
			
		trace_statedump_enumerate_process_state(
				t->pid, t->parent->pid, t->comm,
				type, mode, submode, status, t->tgid);
		task_unlock(t);
	} while (t != &init_task);

	return 0;
}

void ltt_statedump_work_func(void *p)
{
	if (atomic_dec_and_test(&kernel_threads_to_run)) {
		/* If we are the last thread, wake up do_ltt_statedump */
		wake_up_process((struct task_struct *)p);
	}
}

static int do_ltt_statedump(void)
{
	int cpu;

	printk(KERN_DEBUG "do_ltt_statedump\n");
 	ltt_enumerate_process_states();
	ltt_enumerate_file_descriptors();
	list_modules();
	ltt_enumerate_vm_maps();
	list_interrupts();
	ltt_enumerate_network_ip_interface();
	
	/* Fire off a work queue on each CPU. Their sole purpose in life
	 * is to guarantee that each CPU has been in a state where is was in
	 * syscall mode (i.e. not in a trap, an IRQ or a soft IRQ) */
	lock_cpu_hotplug();
	atomic_set(&kernel_threads_to_run, num_online_cpus());
	__set_current_state(TASK_UNINTERRUPTIBLE);
	for_each_online_cpu(cpu) {
		INIT_WORK(&cpu_work[cpu], ltt_statedump_work_func, current);
		schedule_delayed_work_on(cpu,&cpu_work[cpu],0);
	}
	unlock_cpu_hotplug();
	/* Wait for all threads to run */
	schedule();
	BUG_ON(atomic_read(&kernel_threads_to_run) != 0);
	/* Our work is done */
	printk(KERN_DEBUG "trace_statedump_statedump_end\n");
	trace_statedump_statedump_end();
	return 0;
}

int ltt_statedump_start(struct ltt_trace_struct *trace)
{
	printk(KERN_DEBUG "ltt_statedump_start\n");

	return do_ltt_statedump();
}

/* Dynamic facility. */
static int __init statedump_init(void)
{
	int ret;
	printk(KERN_INFO "LTT : ltt-facility-statedump init\n");
	ret = ltt_module_register(LTT_FUNCTION_STATEDUMP,
			ltt_statedump_start,THIS_MODULE);
	return ret;
}

static void __exit statedump_exit(void)
{
	printk(KERN_INFO "LTT : ltt-facility-statedump exit\n");
	ltt_module_unregister(LTT_FUNCTION_STATEDUMP);
}

module_init(statedump_init)
module_exit(statedump_exit)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jean-Hugues Deschenes");
MODULE_DESCRIPTION("Linux Trace Toolkit Statedump");

