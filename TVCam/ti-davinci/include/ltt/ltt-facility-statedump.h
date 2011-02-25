#ifndef _LTT_FACILITY_STATEDUMP_H_
#define _LTT_FACILITY_STATEDUMP_H_

#include <linux/types.h>
#include <ltt/ltt-facility-id-statedump.h>
#include <ltt/ltt-tracer.h>

/* Named types */

enum lttng_thread_type {
	LTTNG_USER_THREAD = 0,
	LTTNG_KERNEL_THREAD = 1,
};

enum lttng_execution_mode {
	LTTNG_USER_MODE = 0,
	LTTNG_SYSCALL = 1,
	LTTNG_TRAP = 2,
	LTTNG_IRQ = 3,
	LTTNG_SOFTIRQ = 4,
	LTTNG_MODE_UNKNOWN = 5,
};

enum lttng_execution_submode {
	LTTNG_NONE = 0,
	LTTNG_UNKNOWN = 1,
};

enum lttng_process_status {
	LTTNG_UNNAMED = 0,
	LTTNG_WAIT_FORK = 1,
	LTTNG_WAIT_CPU = 2,
	LTTNG_EXIT = 3,
	LTTNG_ZOMBIE = 4,
	LTTNG_WAIT = 5,
	LTTNG_RUN = 6,
	LTTNG_DEAD = 7,
};

enum lttng_interface_state {
	LTTNG_UP = 0,
	LTTNG_DOWN = 1,
};

/* Event enumerate_file_descriptors structures */
static inline void lttng_write_string_statedump_enumerate_file_descriptors_name(
		char *buffer,
		size_t *to_base,
		size_t *to,
		const char **from,
		size_t *len,
		const char * obj)
{
	size_t size;
	size_t align;

	/* Flush pending memcpy */
	if (*len != 0) {
		if (buffer != NULL)
			memcpy(buffer+*to_base+*to, *from, *len);
	}
	*to += *len;
	*len = 0;

	align = sizeof(char);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	/* Contains variable sized fields : must explode the structure */

	size = strlen(obj) + 1; /* Include final NULL char. */
	if (buffer != NULL)
		memcpy(buffer+*to_base+*to, obj, size);
	*to += size;

	/* Realign the *to_base on arch size, set *to to 0 */
	*to += ltt_align(*to, sizeof(void *));
	*to_base = *to_base+*to;
	*to = 0;

	/* Put source *from just after the C string */
	*from += size;
}


/* Event enumerate_file_descriptors logging function */
static inline void trace_statedump_enumerate_file_descriptors(
		const char * lttng_param_name,
		unsigned int lttng_param_PID,
		unsigned int lttng_param_fd)
{
	unsigned int index;
	struct ltt_channel_struct *channel;
	struct ltt_trace_struct *trace;
	void *transport_data;
	char *buffer = NULL;
	size_t real_to_base = 0; /* The buffer is allocated on arch_size alignment */
	size_t *to_base = &real_to_base;
	size_t real_to = 0;
	size_t *to = &real_to;
	size_t real_len = 0;
	size_t *len = &real_len;
	size_t reserve_size;
	size_t slot_size;
	size_t align;
	const char *real_from;
	const char **from = &real_from;
	u64 tsc;
	size_t before_hdr_pad, after_hdr_pad, header_size;

	if (ltt_traces.num_active_traces == 0)
		return;

	/* For each field, calculate the field size. */
	/* size = *to_base + *to + *len */
	/* Assume that the padding for alignment starts at a
	 * sizeof(void *) address. */

	*from = (const char*)lttng_param_name;
	lttng_write_string_statedump_enumerate_file_descriptors_name(buffer, to_base, to, from, len, lttng_param_name);

	*from = (const char*)&lttng_param_PID;
	align = sizeof(unsigned int);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	*len += sizeof(unsigned int);

	*from = (const char*)&lttng_param_fd;
	align = sizeof(unsigned int);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	*len += sizeof(unsigned int);

	reserve_size = *to_base + *to + *len;
	preempt_disable();
	ltt_nesting[smp_processor_id()]++;
	index = ltt_get_index_from_facility_statedump(						event_statedump_enumerate_file_descriptors);

	list_for_each_entry_rcu(trace, &ltt_traces.head, list) {
		if (!trace->active)
			continue;

		channel = ltt_get_channel_from_index(trace, index);

		slot_size = 0;
		buffer = ltt_reserve_slot(trace, channel, &transport_data,
			reserve_size, &slot_size, &tsc,
			&before_hdr_pad, &after_hdr_pad, &header_size);
		if (!buffer)
			continue; /* buffer full */

		*to_base = *to = *len = 0;

		ltt_write_event_header(trace, channel, buffer,
			ltt_facility_statedump_5E184DBD, event_statedump_enumerate_file_descriptors,
			reserve_size, before_hdr_pad, tsc);
		*to_base += before_hdr_pad + after_hdr_pad + header_size;

		*from = (const char*)lttng_param_name;
		lttng_write_string_statedump_enumerate_file_descriptors_name(buffer, to_base, to, from, len, lttng_param_name);

		/* Flush pending memcpy */
		if (*len != 0) {
			memcpy(buffer+*to_base+*to, *from, *len);
			*to += *len;
			*len = 0;
		}

		*from = (const char*)&lttng_param_PID;
		align = sizeof(unsigned int);

		if (*len == 0) {
			*to += ltt_align(*to, align); /* align output */
		} else {
			*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
		}

		*len += sizeof(unsigned int);

		/* Flush pending memcpy */
		if (*len != 0) {
			memcpy(buffer+*to_base+*to, *from, *len);
			*to += *len;
			*len = 0;
		}

		*from = (const char*)&lttng_param_fd;
		align = sizeof(unsigned int);

		if (*len == 0) {
			*to += ltt_align(*to, align); /* align output */
		} else {
			*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
		}

		*len += sizeof(unsigned int);

		/* Flush pending memcpy */
		if (*len != 0) {
			memcpy(buffer+*to_base+*to, *from, *len);
			*to += *len;
			*len = 0;
		}

		ltt_commit_slot(channel, &transport_data, buffer, slot_size);

	}

	ltt_nesting[smp_processor_id()]--;
	preempt_enable_no_resched();
}

/* Event enumerate_vm_maps structures */

/* Event enumerate_vm_maps logging function */
static inline void trace_statedump_enumerate_vm_maps(
		unsigned int lttng_param_PID,
		const void * lttng_param_start,
		const void * lttng_param_end,
		unsigned int lttng_param_flags,
		long lttng_param_pgoff,
		long lttng_param_inode)
{
	unsigned int index;
	struct ltt_channel_struct *channel;
	struct ltt_trace_struct *trace;
	void *transport_data;
	char *buffer = NULL;
	size_t real_to_base = 0; /* The buffer is allocated on arch_size alignment */
	size_t *to_base = &real_to_base;
	size_t real_to = 0;
	size_t *to = &real_to;
	size_t real_len = 0;
	size_t *len = &real_len;
	size_t reserve_size;
	size_t slot_size;
	size_t align;
	const char *real_from;
	const char **from = &real_from;
	u64 tsc;
	size_t before_hdr_pad, after_hdr_pad, header_size;

	if (ltt_traces.num_active_traces == 0)
		return;

	/* For each field, calculate the field size. */
	/* size = *to_base + *to + *len */
	/* Assume that the padding for alignment starts at a
	 * sizeof(void *) address. */

	*from = (const char*)&lttng_param_PID;
	align = sizeof(unsigned int);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	*len += sizeof(unsigned int);

	*from = (const char*)&lttng_param_start;
	align = sizeof(const void *);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	*len += sizeof(const void *);

	*from = (const char*)&lttng_param_end;
	align = sizeof(const void *);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	*len += sizeof(const void *);

	*from = (const char*)&lttng_param_flags;
	align = sizeof(unsigned int);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	*len += sizeof(unsigned int);

	*from = (const char*)&lttng_param_pgoff;
	align = sizeof(long);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	*len += sizeof(long);

	*from = (const char*)&lttng_param_inode;
	align = sizeof(long);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	*len += sizeof(long);

	reserve_size = *to_base + *to + *len;
	preempt_disable();
	ltt_nesting[smp_processor_id()]++;
	index = ltt_get_index_from_facility_statedump(						event_statedump_enumerate_vm_maps);

	list_for_each_entry_rcu(trace, &ltt_traces.head, list) {
		if (!trace->active)
			continue;

		channel = ltt_get_channel_from_index(trace, index);

		slot_size = 0;
		buffer = ltt_reserve_slot(trace, channel, &transport_data,
			reserve_size, &slot_size, &tsc,
			&before_hdr_pad, &after_hdr_pad, &header_size);
		if (!buffer)
			continue; /* buffer full */

		*to_base = *to = *len = 0;

		ltt_write_event_header(trace, channel, buffer,
			ltt_facility_statedump_5E184DBD, event_statedump_enumerate_vm_maps,
			reserve_size, before_hdr_pad, tsc);
		*to_base += before_hdr_pad + after_hdr_pad + header_size;

		*from = (const char*)&lttng_param_PID;
		align = sizeof(unsigned int);

		if (*len == 0) {
			*to += ltt_align(*to, align); /* align output */
		} else {
			*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
		}

		*len += sizeof(unsigned int);

		/* Flush pending memcpy */
		if (*len != 0) {
			memcpy(buffer+*to_base+*to, *from, *len);
			*to += *len;
			*len = 0;
		}

		*from = (const char*)&lttng_param_start;
		align = sizeof(const void *);

		if (*len == 0) {
			*to += ltt_align(*to, align); /* align output */
		} else {
			*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
		}

		*len += sizeof(const void *);

		/* Flush pending memcpy */
		if (*len != 0) {
			memcpy(buffer+*to_base+*to, *from, *len);
			*to += *len;
			*len = 0;
		}

		*from = (const char*)&lttng_param_end;
		align = sizeof(const void *);

		if (*len == 0) {
			*to += ltt_align(*to, align); /* align output */
		} else {
			*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
		}

		*len += sizeof(const void *);

		/* Flush pending memcpy */
		if (*len != 0) {
			memcpy(buffer+*to_base+*to, *from, *len);
			*to += *len;
			*len = 0;
		}

		*from = (const char*)&lttng_param_flags;
		align = sizeof(unsigned int);

		if (*len == 0) {
			*to += ltt_align(*to, align); /* align output */
		} else {
			*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
		}

		*len += sizeof(unsigned int);

		/* Flush pending memcpy */
		if (*len != 0) {
			memcpy(buffer+*to_base+*to, *from, *len);
			*to += *len;
			*len = 0;
		}

		*from = (const char*)&lttng_param_pgoff;
		align = sizeof(long);

		if (*len == 0) {
			*to += ltt_align(*to, align); /* align output */
		} else {
			*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
		}

		*len += sizeof(long);

		/* Flush pending memcpy */
		if (*len != 0) {
			memcpy(buffer+*to_base+*to, *from, *len);
			*to += *len;
			*len = 0;
		}

		*from = (const char*)&lttng_param_inode;
		align = sizeof(long);

		if (*len == 0) {
			*to += ltt_align(*to, align); /* align output */
		} else {
			*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
		}

		*len += sizeof(long);

		/* Flush pending memcpy */
		if (*len != 0) {
			memcpy(buffer+*to_base+*to, *from, *len);
			*to += *len;
			*len = 0;
		}

		ltt_commit_slot(channel, &transport_data, buffer, slot_size);

	}

	ltt_nesting[smp_processor_id()]--;
	preempt_enable_no_resched();
}

/* Event enumerate_modules structures */
static inline void lttng_write_string_statedump_enumerate_modules_name(
		char *buffer,
		size_t *to_base,
		size_t *to,
		const char **from,
		size_t *len,
		const char * obj)
{
	size_t size;
	size_t align;

	/* Flush pending memcpy */
	if (*len != 0) {
		if (buffer != NULL)
			memcpy(buffer+*to_base+*to, *from, *len);
	}
	*to += *len;
	*len = 0;

	align = sizeof(char);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	/* Contains variable sized fields : must explode the structure */

	size = strlen(obj) + 1; /* Include final NULL char. */
	if (buffer != NULL)
		memcpy(buffer+*to_base+*to, obj, size);
	*to += size;

	/* Realign the *to_base on arch size, set *to to 0 */
	*to += ltt_align(*to, sizeof(void *));
	*to_base = *to_base+*to;
	*to = 0;

	/* Put source *from just after the C string */
	*from += size;
}


/* Event enumerate_modules logging function */
static inline void trace_statedump_enumerate_modules(
		const char * lttng_param_name,
		unsigned int lttng_param_state,
		unsigned int lttng_param_ref)
{
	unsigned int index;
	struct ltt_channel_struct *channel;
	struct ltt_trace_struct *trace;
	void *transport_data;
	char *buffer = NULL;
	size_t real_to_base = 0; /* The buffer is allocated on arch_size alignment */
	size_t *to_base = &real_to_base;
	size_t real_to = 0;
	size_t *to = &real_to;
	size_t real_len = 0;
	size_t *len = &real_len;
	size_t reserve_size;
	size_t slot_size;
	size_t align;
	const char *real_from;
	const char **from = &real_from;
	u64 tsc;
	size_t before_hdr_pad, after_hdr_pad, header_size;

	if (ltt_traces.num_active_traces == 0)
		return;

	/* For each field, calculate the field size. */
	/* size = *to_base + *to + *len */
	/* Assume that the padding for alignment starts at a
	 * sizeof(void *) address. */

	*from = (const char*)lttng_param_name;
	lttng_write_string_statedump_enumerate_modules_name(buffer, to_base, to, from, len, lttng_param_name);

	*from = (const char*)&lttng_param_state;
	align = sizeof(unsigned int);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	*len += sizeof(unsigned int);

	*from = (const char*)&lttng_param_ref;
	align = sizeof(unsigned int);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	*len += sizeof(unsigned int);

	reserve_size = *to_base + *to + *len;
	preempt_disable();
	ltt_nesting[smp_processor_id()]++;
	index = ltt_get_index_from_facility_statedump(						event_statedump_enumerate_modules);

	list_for_each_entry_rcu(trace, &ltt_traces.head, list) {
		if (!trace->active)
			continue;

		channel = ltt_get_channel_from_index(trace, index);

		slot_size = 0;
		buffer = ltt_reserve_slot(trace, channel, &transport_data,
			reserve_size, &slot_size, &tsc,
			&before_hdr_pad, &after_hdr_pad, &header_size);
		if (!buffer)
			continue; /* buffer full */

		*to_base = *to = *len = 0;

		ltt_write_event_header(trace, channel, buffer,
			ltt_facility_statedump_5E184DBD, event_statedump_enumerate_modules,
			reserve_size, before_hdr_pad, tsc);
		*to_base += before_hdr_pad + after_hdr_pad + header_size;

		*from = (const char*)lttng_param_name;
		lttng_write_string_statedump_enumerate_modules_name(buffer, to_base, to, from, len, lttng_param_name);

		/* Flush pending memcpy */
		if (*len != 0) {
			memcpy(buffer+*to_base+*to, *from, *len);
			*to += *len;
			*len = 0;
		}

		*from = (const char*)&lttng_param_state;
		align = sizeof(unsigned int);

		if (*len == 0) {
			*to += ltt_align(*to, align); /* align output */
		} else {
			*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
		}

		*len += sizeof(unsigned int);

		/* Flush pending memcpy */
		if (*len != 0) {
			memcpy(buffer+*to_base+*to, *from, *len);
			*to += *len;
			*len = 0;
		}

		*from = (const char*)&lttng_param_ref;
		align = sizeof(unsigned int);

		if (*len == 0) {
			*to += ltt_align(*to, align); /* align output */
		} else {
			*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
		}

		*len += sizeof(unsigned int);

		/* Flush pending memcpy */
		if (*len != 0) {
			memcpy(buffer+*to_base+*to, *from, *len);
			*to += *len;
			*len = 0;
		}

		ltt_commit_slot(channel, &transport_data, buffer, slot_size);

	}

	ltt_nesting[smp_processor_id()]--;
	preempt_enable_no_resched();
}

/* Event enumerate_interrupts structures */
static inline void lttng_write_string_statedump_enumerate_interrupts_name(
		char *buffer,
		size_t *to_base,
		size_t *to,
		const char **from,
		size_t *len,
		const char * obj)
{
	size_t size;
	size_t align;

	/* Flush pending memcpy */
	if (*len != 0) {
		if (buffer != NULL)
			memcpy(buffer+*to_base+*to, *from, *len);
	}
	*to += *len;
	*len = 0;

	align = sizeof(char);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	/* Contains variable sized fields : must explode the structure */

	size = strlen(obj) + 1; /* Include final NULL char. */
	if (buffer != NULL)
		memcpy(buffer+*to_base+*to, obj, size);
	*to += size;

	/* Realign the *to_base on arch size, set *to to 0 */
	*to += ltt_align(*to, sizeof(void *));
	*to_base = *to_base+*to;
	*to = 0;

	/* Put source *from just after the C string */
	*from += size;
}

static inline void lttng_write_string_statedump_enumerate_interrupts_action(
		char *buffer,
		size_t *to_base,
		size_t *to,
		const char **from,
		size_t *len,
		const char * obj)
{
	size_t size;
	size_t align;

	/* Flush pending memcpy */
	if (*len != 0) {
		if (buffer != NULL)
			memcpy(buffer+*to_base+*to, *from, *len);
	}
	*to += *len;
	*len = 0;

	align = sizeof(char);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	/* Contains variable sized fields : must explode the structure */

	size = strlen(obj) + 1; /* Include final NULL char. */
	if (buffer != NULL)
		memcpy(buffer+*to_base+*to, obj, size);
	*to += size;

	/* Realign the *to_base on arch size, set *to to 0 */
	*to += ltt_align(*to, sizeof(void *));
	*to_base = *to_base+*to;
	*to = 0;

	/* Put source *from just after the C string */
	*from += size;
}


/* Event enumerate_interrupts logging function */
static inline void trace_statedump_enumerate_interrupts(
		const char * lttng_param_name,
		const char * lttng_param_action,
		unsigned int lttng_param_num)
{
	unsigned int index;
	struct ltt_channel_struct *channel;
	struct ltt_trace_struct *trace;
	void *transport_data;
	char *buffer = NULL;
	size_t real_to_base = 0; /* The buffer is allocated on arch_size alignment */
	size_t *to_base = &real_to_base;
	size_t real_to = 0;
	size_t *to = &real_to;
	size_t real_len = 0;
	size_t *len = &real_len;
	size_t reserve_size;
	size_t slot_size;
	size_t align;
	const char *real_from;
	const char **from = &real_from;
	u64 tsc;
	size_t before_hdr_pad, after_hdr_pad, header_size;

	if (ltt_traces.num_active_traces == 0)
		return;

	/* For each field, calculate the field size. */
	/* size = *to_base + *to + *len */
	/* Assume that the padding for alignment starts at a
	 * sizeof(void *) address. */

	*from = (const char*)lttng_param_name;
	lttng_write_string_statedump_enumerate_interrupts_name(buffer, to_base, to, from, len, lttng_param_name);

	*from = (const char*)lttng_param_action;
	lttng_write_string_statedump_enumerate_interrupts_action(buffer, to_base, to, from, len, lttng_param_action);

	*from = (const char*)&lttng_param_num;
	align = sizeof(unsigned int);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	*len += sizeof(unsigned int);

	reserve_size = *to_base + *to + *len;
	preempt_disable();
	ltt_nesting[smp_processor_id()]++;
	index = ltt_get_index_from_facility_statedump(						event_statedump_enumerate_interrupts);

	list_for_each_entry_rcu(trace, &ltt_traces.head, list) {
		if (!trace->active)
			continue;

		channel = ltt_get_channel_from_index(trace, index);

		slot_size = 0;
		buffer = ltt_reserve_slot(trace, channel, &transport_data,
			reserve_size, &slot_size, &tsc,
			&before_hdr_pad, &after_hdr_pad, &header_size);
		if (!buffer)
			continue; /* buffer full */

		*to_base = *to = *len = 0;

		ltt_write_event_header(trace, channel, buffer,
			ltt_facility_statedump_5E184DBD, event_statedump_enumerate_interrupts,
			reserve_size, before_hdr_pad, tsc);
		*to_base += before_hdr_pad + after_hdr_pad + header_size;

		*from = (const char*)lttng_param_name;
		lttng_write_string_statedump_enumerate_interrupts_name(buffer, to_base, to, from, len, lttng_param_name);

		/* Flush pending memcpy */
		if (*len != 0) {
			memcpy(buffer+*to_base+*to, *from, *len);
			*to += *len;
			*len = 0;
		}

		*from = (const char*)lttng_param_action;
		lttng_write_string_statedump_enumerate_interrupts_action(buffer, to_base, to, from, len, lttng_param_action);

		/* Flush pending memcpy */
		if (*len != 0) {
			memcpy(buffer+*to_base+*to, *from, *len);
			*to += *len;
			*len = 0;
		}

		*from = (const char*)&lttng_param_num;
		align = sizeof(unsigned int);

		if (*len == 0) {
			*to += ltt_align(*to, align); /* align output */
		} else {
			*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
		}

		*len += sizeof(unsigned int);

		/* Flush pending memcpy */
		if (*len != 0) {
			memcpy(buffer+*to_base+*to, *from, *len);
			*to += *len;
			*len = 0;
		}

		ltt_commit_slot(channel, &transport_data, buffer, slot_size);

	}

	ltt_nesting[smp_processor_id()]--;
	preempt_enable_no_resched();
}

/* Event enumerate_process_state structures */
static inline void lttng_write_string_statedump_enumerate_process_state_name(
		char *buffer,
		size_t *to_base,
		size_t *to,
		const char **from,
		size_t *len,
		const char * obj)
{
	size_t size;
	size_t align;

	/* Flush pending memcpy */
	if (*len != 0) {
		if (buffer != NULL)
			memcpy(buffer+*to_base+*to, *from, *len);
	}
	*to += *len;
	*len = 0;

	align = sizeof(char);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	/* Contains variable sized fields : must explode the structure */

	size = strlen(obj) + 1; /* Include final NULL char. */
	if (buffer != NULL)
		memcpy(buffer+*to_base+*to, obj, size);
	*to += size;

	/* Realign the *to_base on arch size, set *to to 0 */
	*to += ltt_align(*to, sizeof(void *));
	*to_base = *to_base+*to;
	*to = 0;

	/* Put source *from just after the C string */
	*from += size;
}


/* Event enumerate_process_state logging function */
static inline void trace_statedump_enumerate_process_state(
		unsigned int lttng_param_pid,
		unsigned int lttng_param_parent_pid,
		const char * lttng_param_name,
		enum lttng_thread_type lttng_param_type,
		enum lttng_execution_mode lttng_param_mode,
		enum lttng_execution_submode lttng_param_submode,
		enum lttng_process_status lttng_param_status,
		unsigned int lttng_param_tgid)
{
	unsigned int index;
	struct ltt_channel_struct *channel;
	struct ltt_trace_struct *trace;
	void *transport_data;
	char *buffer = NULL;
	size_t real_to_base = 0; /* The buffer is allocated on arch_size alignment */
	size_t *to_base = &real_to_base;
	size_t real_to = 0;
	size_t *to = &real_to;
	size_t real_len = 0;
	size_t *len = &real_len;
	size_t reserve_size;
	size_t slot_size;
	size_t align;
	const char *real_from;
	const char **from = &real_from;
	u64 tsc;
	size_t before_hdr_pad, after_hdr_pad, header_size;

	if (ltt_traces.num_active_traces == 0)
		return;

	/* For each field, calculate the field size. */
	/* size = *to_base + *to + *len */
	/* Assume that the padding for alignment starts at a
	 * sizeof(void *) address. */

	*from = (const char*)&lttng_param_pid;
	align = sizeof(unsigned int);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	*len += sizeof(unsigned int);

	*from = (const char*)&lttng_param_parent_pid;
	align = sizeof(unsigned int);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	*len += sizeof(unsigned int);

	*from = (const char*)lttng_param_name;
	lttng_write_string_statedump_enumerate_process_state_name(buffer, to_base, to, from, len, lttng_param_name);

	*from = (const char*)&lttng_param_type;
	align = sizeof(enum lttng_thread_type);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	*len += sizeof(enum lttng_thread_type);

	*from = (const char*)&lttng_param_mode;
	align = sizeof(enum lttng_execution_mode);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	*len += sizeof(enum lttng_execution_mode);

	*from = (const char*)&lttng_param_submode;
	align = sizeof(enum lttng_execution_submode);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	*len += sizeof(enum lttng_execution_submode);

	*from = (const char*)&lttng_param_status;
	align = sizeof(enum lttng_process_status);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	*len += sizeof(enum lttng_process_status);

	*from = (const char*)&lttng_param_tgid;
	align = sizeof(unsigned int);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	*len += sizeof(unsigned int);

	reserve_size = *to_base + *to + *len;
	preempt_disable();
	ltt_nesting[smp_processor_id()]++;
	index = ltt_get_index_from_facility_statedump(						event_statedump_enumerate_process_state);

	list_for_each_entry_rcu(trace, &ltt_traces.head, list) {
		if (!trace->active)
			continue;

		channel = ltt_get_channel_from_index(trace, index);

		slot_size = 0;
		buffer = ltt_reserve_slot(trace, channel, &transport_data,
			reserve_size, &slot_size, &tsc,
			&before_hdr_pad, &after_hdr_pad, &header_size);
		if (!buffer)
			continue; /* buffer full */

		*to_base = *to = *len = 0;

		ltt_write_event_header(trace, channel, buffer,
			ltt_facility_statedump_5E184DBD, event_statedump_enumerate_process_state,
			reserve_size, before_hdr_pad, tsc);
		*to_base += before_hdr_pad + after_hdr_pad + header_size;

		*from = (const char*)&lttng_param_pid;
		align = sizeof(unsigned int);

		if (*len == 0) {
			*to += ltt_align(*to, align); /* align output */
		} else {
			*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
		}

		*len += sizeof(unsigned int);

		/* Flush pending memcpy */
		if (*len != 0) {
			memcpy(buffer+*to_base+*to, *from, *len);
			*to += *len;
			*len = 0;
		}

		*from = (const char*)&lttng_param_parent_pid;
		align = sizeof(unsigned int);

		if (*len == 0) {
			*to += ltt_align(*to, align); /* align output */
		} else {
			*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
		}

		*len += sizeof(unsigned int);

		/* Flush pending memcpy */
		if (*len != 0) {
			memcpy(buffer+*to_base+*to, *from, *len);
			*to += *len;
			*len = 0;
		}

		*from = (const char*)lttng_param_name;
		lttng_write_string_statedump_enumerate_process_state_name(buffer, to_base, to, from, len, lttng_param_name);

		/* Flush pending memcpy */
		if (*len != 0) {
			memcpy(buffer+*to_base+*to, *from, *len);
			*to += *len;
			*len = 0;
		}

		*from = (const char*)&lttng_param_type;
		align = sizeof(enum lttng_thread_type);

		if (*len == 0) {
			*to += ltt_align(*to, align); /* align output */
		} else {
			*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
		}

		*len += sizeof(enum lttng_thread_type);

		/* Flush pending memcpy */
		if (*len != 0) {
			memcpy(buffer+*to_base+*to, *from, *len);
			*to += *len;
			*len = 0;
		}

		*from = (const char*)&lttng_param_mode;
		align = sizeof(enum lttng_execution_mode);

		if (*len == 0) {
			*to += ltt_align(*to, align); /* align output */
		} else {
			*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
		}

		*len += sizeof(enum lttng_execution_mode);

		/* Flush pending memcpy */
		if (*len != 0) {
			memcpy(buffer+*to_base+*to, *from, *len);
			*to += *len;
			*len = 0;
		}

		*from = (const char*)&lttng_param_submode;
		align = sizeof(enum lttng_execution_submode);

		if (*len == 0) {
			*to += ltt_align(*to, align); /* align output */
		} else {
			*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
		}

		*len += sizeof(enum lttng_execution_submode);

		/* Flush pending memcpy */
		if (*len != 0) {
			memcpy(buffer+*to_base+*to, *from, *len);
			*to += *len;
			*len = 0;
		}

		*from = (const char*)&lttng_param_status;
		align = sizeof(enum lttng_process_status);

		if (*len == 0) {
			*to += ltt_align(*to, align); /* align output */
		} else {
			*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
		}

		*len += sizeof(enum lttng_process_status);

		/* Flush pending memcpy */
		if (*len != 0) {
			memcpy(buffer+*to_base+*to, *from, *len);
			*to += *len;
			*len = 0;
		}

		*from = (const char*)&lttng_param_tgid;
		align = sizeof(unsigned int);

		if (*len == 0) {
			*to += ltt_align(*to, align); /* align output */
		} else {
			*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
		}

		*len += sizeof(unsigned int);

		/* Flush pending memcpy */
		if (*len != 0) {
			memcpy(buffer+*to_base+*to, *from, *len);
			*to += *len;
			*len = 0;
		}

		ltt_commit_slot(channel, &transport_data, buffer, slot_size);

	}

	ltt_nesting[smp_processor_id()]--;
	preempt_enable_no_resched();
}

/* Event enumerate_network_ip_interface structures */
static inline void lttng_write_string_statedump_enumerate_network_ip_interface_name(
		char *buffer,
		size_t *to_base,
		size_t *to,
		const char **from,
		size_t *len,
		const char * obj)
{
	size_t size;
	size_t align;

	/* Flush pending memcpy */
	if (*len != 0) {
		if (buffer != NULL)
			memcpy(buffer+*to_base+*to, *from, *len);
	}
	*to += *len;
	*len = 0;

	align = sizeof(char);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	/* Contains variable sized fields : must explode the structure */

	size = strlen(obj) + 1; /* Include final NULL char. */
	if (buffer != NULL)
		memcpy(buffer+*to_base+*to, obj, size);
	*to += size;

	/* Realign the *to_base on arch size, set *to to 0 */
	*to += ltt_align(*to, sizeof(void *));
	*to_base = *to_base+*to;
	*to = 0;

	/* Put source *from just after the C string */
	*from += size;
}


/* Event enumerate_network_ip_interface logging function */
static inline void trace_statedump_enumerate_network_ip_interface(
		const char * lttng_param_name,
		unsigned int lttng_param_addr,
		enum lttng_interface_state lttng_param_status)
{
	unsigned int index;
	struct ltt_channel_struct *channel;
	struct ltt_trace_struct *trace;
	void *transport_data;
	char *buffer = NULL;
	size_t real_to_base = 0; /* The buffer is allocated on arch_size alignment */
	size_t *to_base = &real_to_base;
	size_t real_to = 0;
	size_t *to = &real_to;
	size_t real_len = 0;
	size_t *len = &real_len;
	size_t reserve_size;
	size_t slot_size;
	size_t align;
	const char *real_from;
	const char **from = &real_from;
	u64 tsc;
	size_t before_hdr_pad, after_hdr_pad, header_size;

	if (ltt_traces.num_active_traces == 0)
		return;

	/* For each field, calculate the field size. */
	/* size = *to_base + *to + *len */
	/* Assume that the padding for alignment starts at a
	 * sizeof(void *) address. */

	*from = (const char*)lttng_param_name;
	lttng_write_string_statedump_enumerate_network_ip_interface_name(buffer, to_base, to, from, len, lttng_param_name);

	*from = (const char*)&lttng_param_addr;
	align = sizeof(unsigned int);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	*len += sizeof(unsigned int);

	*from = (const char*)&lttng_param_status;
	align = sizeof(enum lttng_interface_state);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	*len += sizeof(enum lttng_interface_state);

	reserve_size = *to_base + *to + *len;
	preempt_disable();
	ltt_nesting[smp_processor_id()]++;
	index = ltt_get_index_from_facility_statedump(						event_statedump_enumerate_network_ip_interface);

	list_for_each_entry_rcu(trace, &ltt_traces.head, list) {
		if (!trace->active)
			continue;

		channel = ltt_get_channel_from_index(trace, index);

		slot_size = 0;
		buffer = ltt_reserve_slot(trace, channel, &transport_data,
			reserve_size, &slot_size, &tsc,
			&before_hdr_pad, &after_hdr_pad, &header_size);
		if (!buffer)
			continue; /* buffer full */

		*to_base = *to = *len = 0;

		ltt_write_event_header(trace, channel, buffer,
			ltt_facility_statedump_5E184DBD, event_statedump_enumerate_network_ip_interface,
			reserve_size, before_hdr_pad, tsc);
		*to_base += before_hdr_pad + after_hdr_pad + header_size;

		*from = (const char*)lttng_param_name;
		lttng_write_string_statedump_enumerate_network_ip_interface_name(buffer, to_base, to, from, len, lttng_param_name);

		/* Flush pending memcpy */
		if (*len != 0) {
			memcpy(buffer+*to_base+*to, *from, *len);
			*to += *len;
			*len = 0;
		}

		*from = (const char*)&lttng_param_addr;
		align = sizeof(unsigned int);

		if (*len == 0) {
			*to += ltt_align(*to, align); /* align output */
		} else {
			*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
		}

		*len += sizeof(unsigned int);

		/* Flush pending memcpy */
		if (*len != 0) {
			memcpy(buffer+*to_base+*to, *from, *len);
			*to += *len;
			*len = 0;
		}

		*from = (const char*)&lttng_param_status;
		align = sizeof(enum lttng_interface_state);

		if (*len == 0) {
			*to += ltt_align(*to, align); /* align output */
		} else {
			*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
		}

		*len += sizeof(enum lttng_interface_state);

		/* Flush pending memcpy */
		if (*len != 0) {
			memcpy(buffer+*to_base+*to, *from, *len);
			*to += *len;
			*len = 0;
		}

		ltt_commit_slot(channel, &transport_data, buffer, slot_size);

	}

	ltt_nesting[smp_processor_id()]--;
	preempt_enable_no_resched();
}

/* Event statedump_end structures */

/* Event statedump_end logging function */
static inline void trace_statedump_statedump_end(
		void)
{
	unsigned int index;
	struct ltt_channel_struct *channel;
	struct ltt_trace_struct *trace;
	void *transport_data;
	char *buffer = NULL;
	size_t real_to_base = 0; /* The buffer is allocated on arch_size alignment */
	size_t *to_base = &real_to_base;
	size_t real_to = 0;
	size_t *to = &real_to;
	size_t real_len = 0;
	size_t *len = &real_len;
	size_t reserve_size;
	size_t slot_size;
	u64 tsc;
	size_t before_hdr_pad, after_hdr_pad, header_size;

	if (ltt_traces.num_active_traces == 0)
		return;

	/* For each field, calculate the field size. */
	/* size = *to_base + *to + *len */
	/* Assume that the padding for alignment starts at a
	 * sizeof(void *) address. */

	reserve_size = *to_base + *to + *len;
	preempt_disable();
	ltt_nesting[smp_processor_id()]++;
	index = ltt_get_index_from_facility_statedump(						event_statedump_statedump_end);

	list_for_each_entry_rcu(trace, &ltt_traces.head, list) {
		if (!trace->active)
			continue;

		channel = ltt_get_channel_from_index(trace, index);

		slot_size = 0;
		buffer = ltt_reserve_slot(trace, channel, &transport_data,
			reserve_size, &slot_size, &tsc,
			&before_hdr_pad, &after_hdr_pad, &header_size);
		if (!buffer)
			continue; /* buffer full */

		*to_base = *to = *len = 0;

		ltt_write_event_header(trace, channel, buffer,
			ltt_facility_statedump_5E184DBD, event_statedump_statedump_end,
			reserve_size, before_hdr_pad, tsc);
		*to_base += before_hdr_pad + after_hdr_pad + header_size;

		ltt_commit_slot(channel, &transport_data, buffer, slot_size);

	}

	ltt_nesting[smp_processor_id()]--;
	preempt_enable_no_resched();
}

#endif //_LTT_FACILITY_STATEDUMP_H_
