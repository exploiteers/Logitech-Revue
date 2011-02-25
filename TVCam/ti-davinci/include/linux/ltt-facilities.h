/*
 * linux/include/linux/ltt-facilities.h
 *
 * Copyright (C) 2005 Mathieu Desnoyers (mathieu.desnoyers@polymtl.ca)
 *
 * This contains the definitions for the Linux Trace Toolkit facilities.
 *
 * Each facility must store its facility index number (returned by the register)
 * in an exported symbol named :
 *
 * ltt_facility_name_checksum
 *
 * Where name is the name of the facility, and checksum is the text that
 * corresponds to the checksum of the facility in hexadecimal notation.
 */

#ifndef _LTT_FACILITIES_H
#define _LTT_FACILITIES_H

#include <linux/config.h>
#include <linux/types.h>
#include <asm/atomic.h>
#include <linux/types.h>

#define FACNAME_LEN 32

struct user_facility_info {
	char name[FACNAME_LEN];
	unsigned int num_events;
	size_t alignment;
	u32 checksum;
	size_t int_size;
	size_t long_size;
	size_t pointer_size;
	size_t size_t_size;
};

#ifdef __KERNEL__

/* Is kernel tracing enabled */
#if defined(CONFIG_LTT)

#define LTT_FAC_PER_PROCESS 16

#define LTT_MAX_NUM_FACILITIES	256
#define LTT_RESERVED_FACILITIES	1

typedef unsigned long ltt_facility_t;

enum ltt_facility_type {
	LTT_FACILITY_TYPE_KERNEL,
	LTT_FACILITY_TYPE_USER
};

struct ltt_facility {
	const char *name;
	const unsigned int num_events;
	const u32 checksum;
	const char *symbol;
};

struct ltt_facility_info {
	char name[FACNAME_LEN];
	enum ltt_facility_type type;
	unsigned int num_events;
	size_t alignment;
	u32 checksum;
	size_t int_size;
	size_t long_size;
	size_t pointer_size;
	size_t size_t_size;
	atomic_t ref;
};

struct ltt_trace_struct;

void ltt_facility_ref(ltt_facility_t facility_id);

int ltt_facility_unregister(ltt_facility_t facility_id);


int ltt_facility_register(enum ltt_facility_type type,
		const char *name,
		const unsigned int num_events,
		u32 checksum,
		size_t int_size,
		size_t long_size,
		size_t pointer_size,
		size_t size_t_size,
		size_t alignment);

int ltt_facility_verify(enum ltt_facility_type type,
		const char *name,
		const unsigned int num_events,
		const u32 checksum,
		size_t int_size,
		size_t long_size,
		size_t pointer_size,
		size_t size_t_size,
		size_t alignment);

unsigned int ltt_facility_kernel_register(struct ltt_facility *facility);

int ltt_facility_user_access_ok(ltt_facility_t fac_id);

void ltt_facility_free_unused(void);

void ltt_facility_state_dump(struct ltt_trace_struct *trace);

#endif //__KERNEL__

#endif /* defined(CONFIG_LTT) */
#endif /* _LTT_FACILITIES_H */
