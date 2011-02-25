#ifndef _LTT_FACILITY_CORE_H_
#define _LTT_FACILITY_CORE_H_

#include <linux/types.h>
#include <ltt/ltt-facility-id-core.h>
#include <ltt/ltt-tracer.h>

/* Named types */

/* Event facility_load structures */
static inline void lttng_write_string_core_facility_load_name(
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


/* Event facility_load logging function */
static inline void trace_core_facility_load(
		const char * lttng_param_name,
		unsigned int lttng_param_checksum,
		unsigned int lttng_param_id,
		unsigned int lttng_param_int_size,
		unsigned int lttng_param_long_size,
		unsigned int lttng_param_pointer_size,
		unsigned int lttng_param_size_t_size,
		unsigned int lttng_param_has_alignment)
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
	lttng_write_string_core_facility_load_name(buffer, to_base, to, from, len, lttng_param_name);

	*from = (const char*)&lttng_param_checksum;
	align = sizeof(unsigned int);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	*len += sizeof(unsigned int);

	*from = (const char*)&lttng_param_id;
	align = sizeof(unsigned int);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	*len += sizeof(unsigned int);

	*from = (const char*)&lttng_param_int_size;
	align = sizeof(unsigned int);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	*len += sizeof(unsigned int);

	*from = (const char*)&lttng_param_long_size;
	align = sizeof(unsigned int);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	*len += sizeof(unsigned int);

	*from = (const char*)&lttng_param_pointer_size;
	align = sizeof(unsigned int);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	*len += sizeof(unsigned int);

	*from = (const char*)&lttng_param_size_t_size;
	align = sizeof(unsigned int);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	*len += sizeof(unsigned int);

	*from = (const char*)&lttng_param_has_alignment;
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
	index = ltt_get_index_from_facility_core(						event_core_facility_load);

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
			ltt_facility_core_1A8DE486, event_core_facility_load,
			reserve_size, before_hdr_pad, tsc);
		*to_base += before_hdr_pad + after_hdr_pad + header_size;

		*from = (const char*)lttng_param_name;
		lttng_write_string_core_facility_load_name(buffer, to_base, to, from, len, lttng_param_name);

		/* Flush pending memcpy */
		if (*len != 0) {
			memcpy(buffer+*to_base+*to, *from, *len);
			*to += *len;
			*len = 0;
		}

		*from = (const char*)&lttng_param_checksum;
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

		*from = (const char*)&lttng_param_id;
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

		*from = (const char*)&lttng_param_int_size;
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

		*from = (const char*)&lttng_param_long_size;
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

		*from = (const char*)&lttng_param_pointer_size;
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

		*from = (const char*)&lttng_param_size_t_size;
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

		*from = (const char*)&lttng_param_has_alignment;
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

/* Event facility_unload structures */

/* Event facility_unload logging function */
static inline void trace_core_facility_unload(
		unsigned int lttng_param_id)
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

	*from = (const char*)&lttng_param_id;
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
	index = ltt_get_index_from_facility_core(						event_core_facility_unload);

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
			ltt_facility_core_1A8DE486, event_core_facility_unload,
			reserve_size, before_hdr_pad, tsc);
		*to_base += before_hdr_pad + after_hdr_pad + header_size;

		*from = (const char*)&lttng_param_id;
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

/* Event time_heartbeat structures */

/* Event time_heartbeat logging function */
static inline void trace_core_time_heartbeat(
unsigned int tracefile_index)
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
	index = tracefile_index;

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
			ltt_facility_core_1A8DE486, event_core_time_heartbeat,
			reserve_size, before_hdr_pad, tsc);
		*to_base += before_hdr_pad + after_hdr_pad + header_size;

		ltt_commit_slot(channel, &transport_data, buffer, slot_size);

	}

	ltt_nesting[smp_processor_id()]--;
	preempt_enable_no_resched();
}

/* Event state_dump_facility_load structures */
static inline void lttng_write_string_core_state_dump_facility_load_name(
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


/* Event state_dump_facility_load logging function */
static inline void trace_core_state_dump_facility_load(
		struct ltt_trace_struct *dest_trace,
		const char * lttng_param_name,
		unsigned int lttng_param_checksum,
		unsigned int lttng_param_id,
		unsigned int lttng_param_int_size,
		unsigned int lttng_param_long_size,
		unsigned int lttng_param_pointer_size,
		unsigned int lttng_param_size_t_size,
		unsigned int lttng_param_has_alignment)
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
	lttng_write_string_core_state_dump_facility_load_name(buffer, to_base, to, from, len, lttng_param_name);

	*from = (const char*)&lttng_param_checksum;
	align = sizeof(unsigned int);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	*len += sizeof(unsigned int);

	*from = (const char*)&lttng_param_id;
	align = sizeof(unsigned int);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	*len += sizeof(unsigned int);

	*from = (const char*)&lttng_param_int_size;
	align = sizeof(unsigned int);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	*len += sizeof(unsigned int);

	*from = (const char*)&lttng_param_long_size;
	align = sizeof(unsigned int);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	*len += sizeof(unsigned int);

	*from = (const char*)&lttng_param_pointer_size;
	align = sizeof(unsigned int);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	*len += sizeof(unsigned int);

	*from = (const char*)&lttng_param_size_t_size;
	align = sizeof(unsigned int);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	*len += sizeof(unsigned int);

	*from = (const char*)&lttng_param_has_alignment;
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
	index = ltt_get_index_from_facility_core(						event_core_state_dump_facility_load);

	list_for_each_entry_rcu(trace, &ltt_traces.head, list) {
		if (!trace->active)
			continue;

		if (dest_trace != trace)
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
			ltt_facility_core_1A8DE486, event_core_state_dump_facility_load,
			reserve_size, before_hdr_pad, tsc);
		*to_base += before_hdr_pad + after_hdr_pad + header_size;

		*from = (const char*)lttng_param_name;
		lttng_write_string_core_state_dump_facility_load_name(buffer, to_base, to, from, len, lttng_param_name);

		/* Flush pending memcpy */
		if (*len != 0) {
			memcpy(buffer+*to_base+*to, *from, *len);
			*to += *len;
			*len = 0;
		}

		*from = (const char*)&lttng_param_checksum;
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

		*from = (const char*)&lttng_param_id;
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

		*from = (const char*)&lttng_param_int_size;
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

		*from = (const char*)&lttng_param_long_size;
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

		*from = (const char*)&lttng_param_pointer_size;
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

		*from = (const char*)&lttng_param_size_t_size;
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

		*from = (const char*)&lttng_param_has_alignment;
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

#endif //_LTT_FACILITY_CORE_H_
