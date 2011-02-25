/* ltt-probe-list.c
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
#include <ltt/ltt-facility-select-statedump.h>
#include <ltt/ltt-facility-statedump.h>

#define LIST_MODULES_FORMAT "%s enum module_state %d %lu"
void probe_list_modules(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	const char *name;
	enum module_state state;
	unsigned int ref;

	/* Assign args */
	va_start(ap, format);
	name = va_arg(ap, typeof(name));
	state = va_arg(ap, typeof(state));
	ref = va_arg(ap, typeof(ref));

	/* Call tracer */
	trace_statedump_enumerate_modules(name, state, ref);
	
	va_end(ap);
}

static int __init probe_init(void)
{
	int result;
	result = marker_set_probe("list_modules",
			LIST_MODULES_FORMAT,
			probe_list_modules);
	if (!result)
		goto cleanup;

	return 0;

cleanup:
	marker_remove_probe(probe_list_modules);
	return -EPERM;
}

static void __exit probe_fini(void)
{
	marker_remove_probe(probe_list_modules);
}


module_init(probe_init);
module_exit(probe_fini);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mathieu Desnoyers");
MODULE_DESCRIPTION("List Probe");

