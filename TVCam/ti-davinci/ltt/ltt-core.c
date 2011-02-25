
/*
 * LTT core in-kernel infrastructure.
 *
 * Copyright 2006 - Mathieu Desnoyers mathieu.desnoyers@polymtl.ca
 *
 * Distributed under the GPL license
 */

#include <linux/ltt-core.h>
#include <linux/module.h>

/* Traces structures */
struct ltt_traces ltt_traces = {
	.head = LIST_HEAD_INIT(ltt_traces.head),
	.num_active_traces = 0
};

EXPORT_SYMBOL(ltt_traces);

volatile unsigned int ltt_nesting[NR_CPUS] = { [ 0 ... NR_CPUS-1 ] = 0 } ;

EXPORT_SYMBOL(ltt_nesting);

atomic_t lttng_logical_clock = ATOMIC_INIT(0);
EXPORT_SYMBOL(lttng_logical_clock);

