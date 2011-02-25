/******************************************************************************
 * ltt-syscall.c
 *
 * Mathieu Desnoyers (mathieu.desnoyers@polymtl.ca)
 * March 2006
 *
 * LTT userspace tracing syscalls
 */

#include <linux/errno.h>
#include <linux/syscalls.h>
#include <linux/sched.h>
#include <linux/ltt-facilities.h>
#include <ltt/ltt-tracer.h>

#include <asm/uaccess.h>

/* User event logging function */
static inline int trace_user_event(unsigned int facility_id,
		unsigned int event_id,
		void __user *data, size_t data_size, int blocking,
		int high_priority)
{
	int ret = 0;
	unsigned int index;
	struct ltt_channel_struct *channel;
	struct ltt_trace_struct *trace;
	void *transport_data;
	void *buffer = NULL;
	size_t real_to_base = 0; /* buffer allocated on arch_size alignment */
	size_t *to_base = &real_to_base;
	size_t real_to = 0;
	size_t *to = &real_to;
	size_t real_len = 0;
	size_t *len = &real_len;
	size_t reserve_size;
	size_t slot_size;
	u64 tsc;
	size_t before_hdr_pad, after_hdr_pad, header_size;
	struct user_dbg_data dbg;

	dbg.avail_size = 0;
	dbg.write = 0;
	dbg.read = 0;

	if (ltt_traces.num_active_traces == 0)
		return 0;

	/* Assume that the padding for alignment starts at a
	 * sizeof(void *) address. */

	reserve_size = data_size;

	if (high_priority)
		index = GET_CHANNEL_INDEX(processes);
	else
		index = GET_CHANNEL_INDEX(cpu);

	preempt_disable();

	if (blocking) {
		/* User space requested blocking mode :
		 * If one of the active traces has free space below a specific
		 * threshold value, we reenable preemption and block. */
block_test_begin:
		list_for_each_entry_rcu(trace, &ltt_traces.head, list) {
			if (!trace->active)
  				continue;

			if (trace->ops->user_blocking(trace, index, data_size,
							&dbg))
 				goto block_test_begin;
		}
	}
	ltt_nesting[smp_processor_id()]++;
	list_for_each_entry_rcu(trace, &ltt_traces.head, list) {
		if (!trace->active)
			continue;
		channel = ltt_get_channel_from_index(trace, index);
		slot_size = 0;
		buffer = ltt_reserve_slot(trace, channel, &transport_data,
			reserve_size, &slot_size, &tsc,
			&before_hdr_pad, &after_hdr_pad, &header_size);
		if (!buffer) {
			if (blocking)
				trace->ops->user_errors(trace,
					index, data_size, &dbg);
			continue; /* buffer full */
		}
		*to_base = *to = *len = 0;
		ltt_write_event_header(trace, channel, buffer,
			facility_id, event_id,
			reserve_size, before_hdr_pad, tsc);
		*to_base += before_hdr_pad + after_hdr_pad + header_size;
		/* Hope the user pages are not swapped out. In the rare case
		 * where it is, the slot will be zeroed and EFAULT returned. */
		if (__copy_from_user_inatomic(buffer+*to_base+*to, data,
					data_size))
			ret = -EFAULT;	/* Data is garbage in the slot */
		ltt_commit_slot(channel, &transport_data, buffer, slot_size);
		if (ret != 0)
			break;
	}
	ltt_nesting[smp_processor_id()]--;
	preempt_enable_no_resched();
	return ret;
}

asmlinkage long sys_ltt_trace_generic(unsigned int facility_id,
		unsigned int event_id,
		void __user *data,
		size_t data_size,
		int blocking,
		int high_priority)
{
	if (!ltt_facility_user_access_ok(facility_id))
		return -EPERM;
	if (!access_ok(VERIFY_READ, data, data_size))
			return -EFAULT;
	
	return trace_user_event(facility_id, event_id, data, data_size,
			blocking, high_priority);
}

asmlinkage long sys_ltt_register_generic(unsigned int __user *facility_id,
		const struct user_facility_info __user *info)
{
	struct user_facility_info kinfo;
	int fac_id;
	unsigned int i;

	/* Check if the process has already registered the maximum number of 
	 * allowed facilities */
	if (current->ltt_facilities[LTT_FAC_PER_PROCESS-1] != 0)
		return -EPERM;
	
	if (copy_from_user(&kinfo, info, sizeof(*info)))
		return -EFAULT;

	/* Verify if facility is already registered */
	printk(KERN_DEBUG "LTT register generic for %s\n", kinfo.name);
	fac_id = ltt_facility_verify(LTT_FACILITY_TYPE_USER,
				kinfo.name,
				kinfo.num_events,
				kinfo.checksum,
				kinfo.int_size,
				kinfo.long_size,
				kinfo.pointer_size,
				kinfo.size_t_size,
				kinfo.alignment);
	
	printk(KERN_DEBUG "LTT verify return %d\n", fac_id);
	if (fac_id > 0)
		goto found;
	
	fac_id = ltt_facility_register(LTT_FACILITY_TYPE_USER,
				kinfo.name,
				kinfo.num_events,
				kinfo.checksum,
				kinfo.int_size,
				kinfo.long_size,
				kinfo.pointer_size,
				kinfo.size_t_size,
				kinfo.alignment);

	printk(KERN_DEBUG "LTT register return %d\n", fac_id);
	if (fac_id == 0)
		return -EPERM;
	if (fac_id < 0)
		return fac_id;	/* Error */
found:
	get_task_struct(current->group_leader);
	for (i = 0; i < LTT_FAC_PER_PROCESS; i++) {
		if (current->group_leader->ltt_facilities[i] == 0) {
			current->group_leader->ltt_facilities[i] =
				(ltt_facility_t)fac_id;
			break;
		}
	}
	put_task_struct(current->group_leader);
	/* Write facility_id */
	put_user((unsigned int)fac_id, facility_id);
	return 0;
}
