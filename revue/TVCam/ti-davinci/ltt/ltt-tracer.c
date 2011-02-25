/*
 * ltt-tracer.c
 *
 * (C) Copyright	2005-2006 -
 * 		Mathieu Desnoyers (mathieu.desnoyers@polymtl.ca)
 *
 * Contains the kernel code for the Linux Trace Toolkit.
 *
 * Author:
 *	Mathieu Desnoyers (mathieu.desnoyers@polymtl.ca)
 *
 * Inspired from LTT :
 *	Karim Yaghmour (karim@opersys.com)
 *	Tom Zanussi (zanussi@us.ibm.com)
 *	Bob Wisniewski (bob@watson.ibm.com)
 * And from K42 :
 *  Bob Wisniewski (bob@watson.ibm.com)
 *
 * Changelog:
 *  22/09/06 Move to the marker/probes mechanism. (Mathieu Desnoyers)
 *  19/10/05, Complete lockless mechanism. (Mathieu Desnoyers)
 *	27/05/05, Modular redesign and rewrite. (Mathieu Desnoyers)

 * Comments :
 * num_active_traces protects the functors. Changing the pointer is an atomic
 * operation, but the functions can only be called when in tracing. It is then
 * safe to unload a module in which sits a functor when no tracing is active.
 *
 * filter_control functor is protected by incrementing its module refcount.
 * 
 */

#include <linux/config.h>
#include <linux/time.h>
#include <ltt/ltt-tracer.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/ltt-facilities.h>
#include <linux/rcupdate.h>
#include <linux/sched.h>
#include <linux/bitops.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#include <linux/kref.h>

/* Traces list writer locking */
DECLARE_MUTEX(ltt_traces_sem);

static struct timer_list ltt_async_wakeup_timer;

/* Default callbacks for modules */
int ltt_run_filter_default(void)
{
	return 1;
}

int ltt_filter_control_default
	(enum ltt_filter_control_msg msg, struct ltt_trace_struct *trace)
{
	return 0;
}

int ltt_statedump_default(struct ltt_trace_struct *trace)
{
	return 0;
}



/* Callbacks for registered modules */

int (*ltt_filter_control_functor)
	(enum ltt_filter_control_msg msg, struct ltt_trace_struct *trace) =
					ltt_filter_control_default;
struct module *ltt_filter_control_owner = NULL;

/* These function pointers are protected by trace activation check */

int (*ltt_run_filter_functor)(void) = ltt_run_filter_default;
struct module *ltt_run_filter_owner = NULL;

// FIXME : integrate the filter in the logging chain.
//
int (*ltt_statedump_functor)(struct ltt_trace_struct *trace) = 
					ltt_statedump_default;
struct module *ltt_statedump_owner = NULL;
					
/* Module registration methods */

int ltt_module_register(enum ltt_module_function name, void *function,
		struct module *owner)
{
	int ret = 0;
	
	switch (name) {
		case LTT_FUNCTION_RUN_FILTER:
			if (ltt_run_filter_owner != NULL) {
				ret = -EEXIST;
				goto end;
			}
			ltt_run_filter_functor = (int (*)(void))function;
			ltt_run_filter_owner = owner;
			break;
		case LTT_FUNCTION_FILTER_CONTROL:
			if (ltt_filter_control_owner != NULL) {
				ret = -EEXIST;
				goto end;
			}
			ltt_filter_control_functor = 
				(int (*)(enum ltt_filter_control_msg,
				struct ltt_trace_struct *))function;
			break;
		case LTT_FUNCTION_STATEDUMP:
			if (ltt_statedump_owner != NULL) {
				ret = -EEXIST;
				goto end;
			}
			ltt_statedump_functor = 
				(int (*)(struct ltt_trace_struct *))function;
			ltt_statedump_owner = owner;
			break;
	}

end:

	return ret;
}


void ltt_module_unregister(enum ltt_module_function name)
{
	switch (name) {
		case LTT_FUNCTION_RUN_FILTER:
			ltt_run_filter_functor = ltt_run_filter_default;
			ltt_run_filter_owner = NULL;
			/* Wait for preempt sections to finish */
			synchronize_sched();
			break;
		case LTT_FUNCTION_FILTER_CONTROL:
			ltt_filter_control_functor = ltt_filter_control_default;
			ltt_filter_control_owner = NULL;
			break;
		case LTT_FUNCTION_STATEDUMP:
			ltt_statedump_functor = ltt_statedump_default;
			ltt_statedump_owner = NULL;
			break;
	}

}

EXPORT_SYMBOL_GPL(ltt_module_register);
EXPORT_SYMBOL_GPL(ltt_module_unregister);

static LIST_HEAD(ltt_transport_list);

void ltt_transport_register(struct ltt_transport *transport)
{
	down(&ltt_traces_sem);
	list_add_tail(&transport->node, &ltt_transport_list);
	up(&ltt_traces_sem);
}

void ltt_transport_unregister(struct ltt_transport *transport)
{
	down(&ltt_traces_sem);
	list_del(&transport->node);
	up(&ltt_traces_sem);
}

EXPORT_SYMBOL_GPL(ltt_transport_register);
EXPORT_SYMBOL_GPL(ltt_transport_unregister);


static inline int is_channel_overwrite(enum ltt_channels chan,
	enum trace_mode mode)
{
	switch (mode) {
		case LTT_TRACE_NORMAL:
			return 0;
		case LTT_TRACE_FLIGHT:
			switch (chan) {
				case LTT_CHANNEL_FACILITIES:
					return 0;
				default:
					return 1;
			}
		case LTT_TRACE_HYBRID:
			switch (chan) {
				case LTT_CHANNEL_CPU:
					return 1;
				default:
					return 0;
			}
		default:
			return 0;
	}
}


void ltt_write_trace_header(struct ltt_trace_struct *trace,
		struct ltt_trace_header *header)
{
	header->magic_number = LTT_TRACER_MAGIC_NUMBER;
	header->major_version = LTT_TRACER_VERSION_MAJOR;
	header->minor_version = LTT_TRACER_VERSION_MINOR;
	header->float_word_order = 0;	 /* Kernel : no floating point */
	header->arch_type = LTT_ARCH_TYPE;
	header->arch_size = sizeof(void*);
	header->arch_variant = LTT_ARCH_VARIANT;
	switch (trace->mode) {
		case LTT_TRACE_NORMAL:
			header->flight_recorder = 0;
			break;
		case LTT_TRACE_FLIGHT:
			header->flight_recorder = 1;
			break;
		case LTT_TRACE_HYBRID:
			header->flight_recorder = 2;
			break;
		default:
			header->flight_recorder = 0;
	}
	
#ifdef CONFIG_LTT_HEARTBEAT_EVENT
	header->has_heartbeat = 1;
#else
	header->has_heartbeat = 0;
#endif //CONFIG_LTT_HEARTBEAT_EVENT

#ifdef CONFIG_LTT_ALIGNMENT
	header->has_alignment = sizeof(void*);
#else
	header->has_alignment = 0;
#endif

	header->freq_scale = trace->freq_scale;
	header->start_freq = trace->start_freq;
	header->start_tsc = trace->start_tsc;
	header->start_monotonic = trace->start_monotonic;
	header->start_time_sec = trace->start_time.tv_sec;
	header->start_time_usec = trace->start_time.tv_usec;
}
EXPORT_SYMBOL_GPL(ltt_write_trace_header);

static void trace_async_wakeup(struct ltt_trace_struct *trace)
{
	/* Must check each channel for pending read wakeup */
	trace->ops->wakeup_channel(trace->channel.facilities);
	trace->ops->wakeup_channel(trace->channel.interrupts);
	trace->ops->wakeup_channel(trace->channel.processes);
	trace->ops->wakeup_channel(trace->channel.modules);
	trace->ops->wakeup_channel(trace->channel.cpu);
	trace->ops->wakeup_channel(trace->channel.network);
}

/* Timer to send async wakeups to the readers */
static void async_wakeup(unsigned long data)
{
	struct ltt_trace_struct *trace;
	preempt_disable();
	list_for_each_entry_rcu(trace, &ltt_traces.head, list) {
		trace_async_wakeup(trace);
	}
	preempt_enable();

	del_timer(&ltt_async_wakeup_timer);
	ltt_async_wakeup_timer.expires = jiffies + 1;
	add_timer(&ltt_async_wakeup_timer);
}




void ltt_wakeup_writers(void *private)
{
	struct ltt_channel_buf_struct *ltt_buf = private;
	wake_up_interruptible(&ltt_buf->write_wait);
}
EXPORT_SYMBOL_GPL(ltt_wakeup_writers);


/* _ltt_trace_find :
 * find a trace by given name.
 *
 * Returns a pointer to the trace structure, NULL if not found. */
static struct ltt_trace_struct *_ltt_trace_find(char *trace_name)
{
	int compare;
	struct ltt_trace_struct *trace, *found=NULL;
	
	list_for_each_entry(trace, &ltt_traces.head, list) {
		compare = strncmp(trace->trace_name, trace_name, NAME_MAX);

		if (compare == 0) {
			found = trace;
			break;
		}
	}
	
	return found;
}

/* This function must be called with traces semaphore held. */
static int _ltt_trace_create(char *trace_name,	enum trace_mode mode,
				struct ltt_trace_struct *new_trace)
{
	int err = EPERM;

	if (_ltt_trace_find(trace_name) != NULL) {
		printk(KERN_ERR "LTT : Trace %s already exists\n", trace_name);
		err = EEXIST;
		goto traces_error;
	}
	list_add_rcu(&new_trace->list, &ltt_traces.head);
	synchronize_sched();
	/* Everything went fine, finish creation */
	return 0;

	/* Error handling */
traces_error:
	return err;
}

static void print_channel_errors(struct ltt_channel_struct *ltt_chan)
{
	struct ltt_trace_struct *trace = ltt_chan->trace;
	unsigned int i;

	for (i = 0; i < NR_CPUS; i++) {
		if (atomic_read(&ltt_chan->buf[i].events_lost))
			printk(KERN_ALERT 
				"LTT : %s : %d events lost "
				"in %s channel (cpu %u).\n",
				ltt_chan->channel_name,
				atomic_read(&ltt_chan->buf[i].events_lost),
				ltt_chan->channel_name, i);
		if (atomic_read(&ltt_chan->buf[i].corrupted_subbuffers))
			printk(KERN_ALERT 
				"LTT : %s : %d corrupted subbuffers "
				"in %s channel (cpu %u).\n",
				ltt_chan->channel_name,
				atomic_read(
					&ltt_chan->buf[i].corrupted_subbuffers),
				ltt_chan->channel_name, i);

		trace->ops->print_errors(trace, ltt_chan, i);
	}
}

static void ltt_release_trace(struct kref *kref)
{
	struct ltt_trace_struct *trace = container_of(kref,
			struct ltt_trace_struct, kref);
	kfree(trace);
}

static void ltt_release_channel(struct kref *kref)
{
	unsigned int i;
	struct ltt_channel_struct *ltt_chan = container_of(kref,
			struct ltt_channel_struct, kref);

	print_channel_errors(ltt_chan);
	for (i = 0; i < NR_CPUS; i++)
		kfree(ltt_chan->buf[i].commit_count);
	kfree(ltt_chan);
}

void ltt_buffer_destroy(struct ltt_channel_struct *ltt_chan)
{
	kref_put(&ltt_chan->kref, ltt_release_channel);
	kref_put(&ltt_chan->trace->kref, ltt_release_trace);
}
EXPORT_SYMBOL_GPL(ltt_buffer_destroy);


static void init_error_count(struct ltt_channel_struct *ltt_chan)
{	
	unsigned int i;
	for (i = 0; i < NR_CPUS; i++) {
		atomic_set(&ltt_chan->buf[i].events_lost, 0);
		atomic_set(&ltt_chan->buf[i].corrupted_subbuffers, 0);
	}
}

static inline void prepare_chan_size_num(unsigned *subbuf_size,
	unsigned *n_subbufs, unsigned default_size, unsigned default_n_subbufs)
{
	if (*subbuf_size == 0)
		*subbuf_size = default_size;
	if (*n_subbufs == 0)
		*n_subbufs = default_n_subbufs;
	*subbuf_size = 1 << get_count_order(*subbuf_size);
	*n_subbufs = 1 << get_count_order(*n_subbufs);

	/* Subbuf size and number must both be power of two */
	WARN_ON(hweight32(*subbuf_size) != 1);
	WARN_ON(hweight32(*n_subbufs) != 1);
}

static int ltt_trace_create(char *trace_name, char *trace_type,
		enum trace_mode mode,
		unsigned subbuf_size_low, unsigned n_subbufs_low,
		unsigned subbuf_size_med, unsigned n_subbufs_med,
		unsigned subbuf_size_high, unsigned n_subbufs_high)
{
	int err = 0;
	struct ltt_trace_struct *new_trace;
	unsigned long flags;
	struct ltt_transport *tran, *transport = NULL;
	
	prepare_chan_size_num(&subbuf_size_low, &n_subbufs_low,
		LTT_DEFAULT_SUBBUF_SIZE_LOW, LTT_DEFAULT_N_SUBBUFS_LOW);

	prepare_chan_size_num(&subbuf_size_med, &n_subbufs_med,
		LTT_DEFAULT_SUBBUF_SIZE_MED, LTT_DEFAULT_N_SUBBUFS_MED);

	prepare_chan_size_num(&subbuf_size_high, &n_subbufs_high,
		LTT_DEFAULT_SUBBUF_SIZE_HIGH, LTT_DEFAULT_N_SUBBUFS_HIGH);

	new_trace = kzalloc(sizeof(struct ltt_trace_struct), GFP_KERNEL);
	if (!new_trace) {
		printk(KERN_ERR
			"LTT : Unable to allocate memory for trace %s\n",
			trace_name);
		err = ENOMEM;
		goto traces_error;
	}

	kref_init(&new_trace->kref);
	kref_init(&new_trace->ltt_transport_kref);
	kref_get(&new_trace->kref);
	new_trace->active = 0;
	strncpy(new_trace->trace_name, trace_name, NAME_MAX);
	new_trace->paused = 0;
	new_trace->mode = mode;
	new_trace->freq_scale = ltt_freq_scale();

	down(&ltt_traces_sem);
	list_for_each_entry(tran, &ltt_transport_list, node) {
		if (!strcmp(tran->name, trace_type)) {
			transport = tran;
			break;
		}
	}

	if (!transport) {
		err = EINVAL;
		printk(KERN_ERR	"LTT : Transport %s is not present.\n", trace_type);
		up(&ltt_traces_sem);
		goto trace_error;
	}

	if (!try_module_get(transport->owner)) {
		err = ENODEV;
		printk(KERN_ERR	"LTT : Can't lock transport module.\n");
		up(&ltt_traces_sem);
		goto trace_error;
	}
	up(&ltt_traces_sem);

	new_trace->transport = transport;
	new_trace->ops = &transport->ops;

	err = new_trace->ops->create_dirs(new_trace);
	if (err)
		goto dirs_error;

	local_irq_save(flags);
	new_trace->start_freq = ltt_frequency();
	new_trace->start_tsc = ltt_get_timestamp64();
	do_gettimeofday(&new_trace->start_time);
	local_irq_restore(flags);
	
	/* Always put the facilities channel in non-overwrite mode :
	 * This is a very low traffic channel and it can't afford to have its
	 * data overwritten : this data (facilities info) is necessary to be
	 * able to read the trace.
	 *
	 * WARNING : The heartbeat time _shouldn't_ write events in the
	 * facilities channel as it would make the traffic too high. This is a
	 * problematic case with flight recorder mode. FIXME
	 */
	err = new_trace->ops->create_channel(trace_name, new_trace,
			new_trace->dentry.control_root, LTT_FACILITIES_CHANNEL,
			&new_trace->channel.facilities, subbuf_size_low,
			n_subbufs_low, is_channel_overwrite(LTT_CHANNEL_FACILITIES, mode));
	if (err != 0) {
		goto facilities_error;
	}
	err = new_trace->ops->create_channel(trace_name, new_trace,
			new_trace->dentry.control_root, LTT_INTERRUPTS_CHANNEL,
			&new_trace->channel.interrupts, subbuf_size_low,
			n_subbufs_low, is_channel_overwrite(LTT_CHANNEL_INTERRUPTS, mode));
	if (err != 0) {
		goto interrupts_error;
	}
	err = new_trace->ops->create_channel(trace_name, new_trace,
			new_trace->dentry.control_root, LTT_PROCESSES_CHANNEL,
			&new_trace->channel.processes, subbuf_size_med,
			n_subbufs_med, is_channel_overwrite(LTT_CHANNEL_PROCESSES, mode));
	if (err != 0) {
		goto processes_error;
	}
	err = new_trace->ops->create_channel(trace_name, new_trace,
			new_trace->dentry.control_root, LTT_MODULES_CHANNEL,
			&new_trace->channel.modules, subbuf_size_low,
			n_subbufs_low, is_channel_overwrite(LTT_CHANNEL_MODULES, mode));
	if (err != 0) {
		goto modules_error;
	}
	err = new_trace->ops->create_channel(trace_name, new_trace,
			new_trace->dentry.trace_root, LTT_CPU_CHANNEL,
			&new_trace->channel.cpu, subbuf_size_high,
			n_subbufs_high, is_channel_overwrite(LTT_CHANNEL_CPU, mode));
	if (err != 0) {
		goto cpu_error;
	}
	err = new_trace->ops->create_channel(trace_name, new_trace,
			new_trace->dentry.control_root, LTT_NETWORK_CHANNEL,
			&new_trace->channel.network, subbuf_size_low,
			n_subbufs_low, is_channel_overwrite(LTT_CHANNEL_NETWORK, mode));
	if (err != 0) {
		goto network_error;
	}
	
	init_error_count(new_trace->channel.facilities);
	init_error_count(new_trace->channel.interrupts);
	init_error_count(new_trace->channel.processes);
	init_error_count(new_trace->channel.modules);
	init_error_count(new_trace->channel.cpu);
	init_error_count(new_trace->channel.network);

	down(&ltt_traces_sem);

	err = _ltt_trace_create(trace_name, mode, new_trace);

	up(&ltt_traces_sem);
	if (err != 0)
		goto lock_create_error;
	return err;

lock_create_error:
	new_trace->ops->remove_channel(new_trace->channel.network);
network_error:
	new_trace->ops->remove_channel(new_trace->channel.cpu);
cpu_error:
	new_trace->ops->remove_channel(new_trace->channel.modules);
modules_error:
	new_trace->ops->remove_channel(new_trace->channel.processes);
processes_error:
	new_trace->ops->remove_channel(new_trace->channel.interrupts);
interrupts_error:
	new_trace->ops->remove_channel(new_trace->channel.facilities);
facilities_error:
	new_trace->ops->remove_dirs(new_trace);
dirs_error:
	module_put(transport->owner);
trace_error:
	kref_put(&new_trace->kref, ltt_release_trace);
traces_error:
	return err;
}

/* Must be called while sure that trace is in the list. */
static int _ltt_trace_destroy(struct ltt_trace_struct	*trace)
{
	int err = EPERM;
	
	if (trace == NULL) {
		err = ENOENT;
		goto traces_error;
	}
	if (trace->active) {
		printk(KERN_ERR
			"LTT : Can't destroy trace %s : tracer is active\n",
			trace->trace_name);
		err = EBUSY;
		goto active_error;
	}
	/* Everything went fine */
	list_del_rcu(&trace->list);
	synchronize_sched();
	/* If no more trace in the list, we can free the unused facilities */
	if (list_empty(&ltt_traces.head))
		ltt_facility_free_unused();
	return 0;

	/* error handling */
active_error:
traces_error:
	return err;
}

/* Sleepable part of the destroy */
static void __ltt_trace_destroy(struct ltt_trace_struct	*trace)
{
	trace->ops->finish_channel(trace->channel.facilities);
	trace->ops->finish_channel(trace->channel.interrupts);
	trace->ops->finish_channel(trace->channel.processes);
	trace->ops->finish_channel(trace->channel.modules);
	trace->ops->finish_channel(trace->channel.cpu);
	trace->ops->finish_channel(trace->channel.network);

	flush_scheduled_work();

	if (ltt_traces.num_active_traces == 0) {
		/* We stop the asynchronous delivery of reader wakeup, but
		 * we must make one last check for reader wakeups pending. */
		del_timer(&ltt_async_wakeup_timer);
	}
	/* The currently destroyed trace is not in the trace list anymore,
	 * so it's safe to call the async wakeup ourself. It will deliver
	 * the last subbuffers. */
	trace_async_wakeup(trace);

	trace->ops->remove_channel(trace->channel.facilities);
	trace->ops->remove_channel(trace->channel.interrupts);
	trace->ops->remove_channel(trace->channel.processes);
	trace->ops->remove_channel(trace->channel.modules);
	trace->ops->remove_channel(trace->channel.cpu);
	trace->ops->remove_channel(trace->channel.network);

	trace->ops->remove_dirs(trace);

	module_put(trace->transport->owner);

	kref_put(&trace->kref, ltt_release_trace);
}

static int ltt_trace_destroy(char *trace_name)
{
	int err = 0;
	struct ltt_trace_struct* trace;

	down(&ltt_traces_sem);
	trace = _ltt_trace_find(trace_name);
	err = _ltt_trace_destroy(trace);
	if (err)
		goto error;
	up(&ltt_traces_sem);
	__ltt_trace_destroy(trace);
	return err;

	/* Error handling */
error:
	up(&ltt_traces_sem);
	return err;
}

/* must be called from within a traces lock. */
static int _ltt_trace_start(struct ltt_trace_struct* trace)
{
	int err = 0;

	if (trace == NULL) {
		err = ENOENT;
		goto traces_error;
	}
	if (trace->active)
		printk(KERN_INFO "LTT : Tracing already active for trace %s\n",
				trace->trace_name);
	if (!try_module_get(ltt_run_filter_owner)) {
		err = ENODEV;
		printk(KERN_ERR "LTT : Can't lock filter module.\n");
		goto get_ltt_run_filter_error;
	}
	if (ltt_traces.num_active_traces == 0) {
#ifdef CONFIG_LTT_HEARTBEAT
		if (ltt_heartbeat_trigger(LTT_HEARTBEAT_START)) {
			err = ENODEV;
			printk(KERN_ERR
				"LTT : Heartbeat timer module not present.\n");
			goto ltt_heartbeat_error;
		}
#endif //CONFIG_LTT_HEARTBEAT
		init_timer(&ltt_async_wakeup_timer);
		ltt_async_wakeup_timer.function = async_wakeup;
		ltt_async_wakeup_timer.expires = jiffies + 1;
		add_timer(&ltt_async_wakeup_timer);
	}
	trace->active = 1;
	ltt_traces.num_active_traces++;	/* Read by trace points without
					 * protection : be careful */
	return err;

	/* error handling */
#ifdef CONFIG_LTT_HEARTBEAT
ltt_heartbeat_error:
#endif //CONFIG_LTT_HEARTBEAT
	module_put(ltt_run_filter_owner);
get_ltt_run_filter_error:
traces_error:
	return err;
}

static int ltt_trace_start(char *trace_name)
{
	int err = 0;
	struct ltt_trace_struct* trace;

	down(&ltt_traces_sem);

	trace = _ltt_trace_find(trace_name);
	if (trace == NULL)
		goto no_trace;
	err = _ltt_trace_start(trace);

	up(&ltt_traces_sem);
	
	/* Call the kernel state dump.
	 * Events will be mixed with real kernel events, it's ok.
	 * Notice that there is no protection on the trace : that's exactly
	 * why we iterate on the list and check for trace equality instead of
	 * directly using this trace handle inside the logging function. */
	
	ltt_facility_state_dump(trace);

	if (!try_module_get(ltt_statedump_owner)) {
		err = ENODEV;
		printk(KERN_ERR
			"LTT : Can't lock state dump module.\n");
	} else {
		ltt_statedump_functor(trace);
		module_put(ltt_statedump_owner);
	}

	return err;

	/* Error handling */
no_trace:
	up(&ltt_traces_sem);
	return err;
}


/* must be called from within traces lock */
static int _ltt_trace_stop(struct ltt_trace_struct* trace)
{
	int err = EPERM;

	if (trace == NULL) {
		err = ENOENT;
		goto traces_error;
	}
	if (!trace->active)
		printk(KERN_INFO "LTT : Tracing not active for trace %s\n",
				trace->trace_name);
	if (trace->active) {
		trace->active = 0;
		ltt_traces.num_active_traces--;
		synchronize_sched(); /* Wait for each tracing to be finished */
	}
	if (ltt_traces.num_active_traces == 0) {
#ifdef CONFIG_LTT_HEARTBEAT
	/* stop the heartbeat if we are the last active trace */
		ltt_heartbeat_trigger(LTT_HEARTBEAT_STOP);
#endif //CONFIG_LTT_HEARTBEAT
	}
	module_put(ltt_run_filter_owner);
	/* Everything went fine */
	return 0;

	/* Error handling */
traces_error:
	return err;
}

static int ltt_trace_stop(char *trace_name)
{
	int err = 0;
	struct ltt_trace_struct* trace;

	down(&ltt_traces_sem);
	trace = _ltt_trace_find(trace_name);
	err = _ltt_trace_stop(trace);
	up(&ltt_traces_sem);
	return err;
}


/* Exported functions */

int ltt_control(enum ltt_control_msg msg, char *trace_name, char *trace_type,
		union ltt_control_args args)
{
	int err = EPERM;
	
	printk(KERN_ALERT "ltt_control : trace %s\n", trace_name);
	switch (msg) {
		case LTT_CONTROL_START:
			printk(KERN_DEBUG "Start tracing %s\n", trace_name);
			err = ltt_trace_start(trace_name);
			break;
		case LTT_CONTROL_STOP:
			printk(KERN_DEBUG "Stop tracing %s\n", trace_name);
			err = ltt_trace_stop(trace_name);
			break;
		case LTT_CONTROL_CREATE_TRACE:
			printk(KERN_DEBUG "Creating trace %s\n", trace_name);
			err = ltt_trace_create(trace_name, trace_type,
				args.new_trace.mode,
				args.new_trace.subbuf_size_low,
				args.new_trace.n_subbufs_low,
				args.new_trace.subbuf_size_med,
				args.new_trace.n_subbufs_med,
				args.new_trace.subbuf_size_high,
				args.new_trace.n_subbufs_high);
			break;
		case LTT_CONTROL_DESTROY_TRACE:
			printk(KERN_DEBUG "Destroying trace %s\n", trace_name);
			err = ltt_trace_destroy(trace_name);
			break;
	}
	return err;
}
EXPORT_SYMBOL_GPL(ltt_control);


int ltt_filter_control(enum ltt_filter_control_msg msg, char *trace_name)
{
	int err;
	struct ltt_trace_struct *trace;

	printk(KERN_DEBUG "ltt_filter_control : trace %s\n", trace_name);
	down(&ltt_traces_sem);
	trace = _ltt_trace_find(trace_name);
	if (trace == NULL) {
		printk(KERN_ALERT
			"Trace does not exist. Cannot proxy control request\n");
		err = ENOENT;
		goto trace_error;
	}
	if (!try_module_get(ltt_filter_control_owner)) {
		err = ENODEV;
		goto get_module_error;
	}
	switch (msg) {
		case LTT_FILTER_DEFAULT_ACCEPT:
			printk(KERN_DEBUG
				"Proxy filter default accept %s\n", trace_name);
			err = (*ltt_filter_control_functor)(msg, trace);
			break;
		case LTT_FILTER_DEFAULT_REJECT:
			printk(KERN_DEBUG
				"Proxy filter default reject %s\n", trace_name);
			err = (*ltt_filter_control_functor)(msg, trace);
			break;
		default:
			err = EPERM;
	}
	module_put(ltt_filter_control_owner);
	
get_module_error:
trace_error:
	up(&ltt_traces_sem);
	return err;
}
EXPORT_SYMBOL_GPL(ltt_filter_control);

static void __exit ltt_exit(void)
{
	struct ltt_trace_struct *trace;
	
	down(&ltt_traces_sem);
	/* Stop each trace and destroy them */
	list_for_each_entry_rcu(trace, &ltt_traces.head, list) {
		_ltt_trace_stop(trace);
		_ltt_trace_destroy(trace);/* it's doing a synchronize_sched() */
		__ltt_trace_destroy(trace);
	}
	up(&ltt_traces_sem);
}

module_exit(ltt_exit)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mathieu Desnoyers");
MODULE_DESCRIPTION("Linux Trace Toolkit Next Generation Tracer");

