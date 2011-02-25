/*
 * linux/include/linux/ltt-core.h
 *
 * Copyright (C) 2005,2006 Mathieu Desnoyers (mathieu.desnoyers@polymtl.ca)
 *
 * This contains the core definitions for the Linux Trace Toolkit.
 */

#ifndef LTT_CORE_H
#define LTT_CORE_H

#include <linux/list.h>

#ifdef CONFIG_LTT

/* All modifications of ltt_traces must be done by ltt-tracer.c, while holding
 * the semaphore. Only reading of this information can be done elsewhere, with
 * the RCU mechanism : the preemption must be disabled while reading the
 * list. */
struct ltt_traces {
	struct list_head head;		/* Traces list */
	unsigned int num_active_traces;	/* Number of active traces */
} ____cacheline_aligned;

extern struct ltt_traces ltt_traces;


/* Keep track of traps nesting inside LTT */
extern volatile unsigned int ltt_nesting[];

#endif //CONFIG_LTT

#endif //LTT_CORE_H
