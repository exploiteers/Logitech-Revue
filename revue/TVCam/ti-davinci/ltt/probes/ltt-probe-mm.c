/* ltt-probe-mm.c
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
#include <ltt/ltt-facility-select-default.h>
#include <ltt/ltt-facility-memory.h>

#define MM_FILEMAP_WAIT_START_FORMAT "%p"
void probe_mm_filemap_wait_start(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	void *address;

	/* Assign args */
	va_start(ap, format);
	address = va_arg(ap, typeof(address));

	/* Call tracer */
	trace_memory_page_wait_start(address);
	
	va_end(ap);
}

#define MM_FILEMAP_WAIT_END_FORMAT "%p"
void probe_mm_filemap_wait_end(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	void *address;

	/* Assign args */
	va_start(ap, format);
	address = va_arg(ap, typeof(address));

	/* Call tracer */
	trace_memory_page_wait_end(address);
	
	va_end(ap);
}

#define MM_PAGE_FREE_FORMAT "%u %p"
void probe_mm_page_free(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	unsigned int order;
	void *address;

	/* Assign args */
	va_start(ap, format);
	order = va_arg(ap, typeof(order));
	address = va_arg(ap, typeof(address));

	/* Call tracer */
	trace_memory_page_free(order, address);
	
	va_end(ap);
}

#define MM_PAGE_ALLOC_FORMAT "%u %p"
void probe_mm_page_alloc(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	unsigned int order;
	void *address;

	/* Assign args */
	va_start(ap, format);
	order = va_arg(ap, typeof(order));
	address = va_arg(ap, typeof(address));

	/* Call tracer */
	trace_memory_page_alloc(order, address);
	
	va_end(ap);
}

#define MM_SWAP_IN_FORMAT "%lu"
void probe_mm_swap_in(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	unsigned long address;

	/* Assign args */
	va_start(ap, format);
	address = va_arg(ap, typeof(address));

	/* Call tracer */
	trace_memory_swap_in((void*)address);
	
	va_end(ap);
}

#define MM_SWAP_OUT_FORMAT "%p"
void probe_mm_swap_out(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	void *address;

	/* Assign args */
	va_start(ap, format);
	address = va_arg(ap, typeof(address));

	/* Call tracer */
	trace_memory_swap_out(address);
	
	va_end(ap);
}

static int __init probe_init(void)
{
	int result;
	result = marker_set_probe("mm_filemap_wait_start",
			MM_FILEMAP_WAIT_START_FORMAT,
			probe_mm_filemap_wait_start);
	if (!result)
		goto cleanup;
	result = marker_set_probe("mm_filemap_wait_end",
			MM_FILEMAP_WAIT_END_FORMAT,
			probe_mm_filemap_wait_end);
	if (!result)
		goto cleanup;
	result = marker_set_probe("mm_page_free",
			MM_PAGE_FREE_FORMAT,
			probe_mm_page_free);
	if (!result)
		goto cleanup;
	result = marker_set_probe("mm_page_alloc",
			MM_PAGE_ALLOC_FORMAT,
			probe_mm_page_alloc);
	if (!result)
		goto cleanup;
	result = marker_set_probe("mm_swap_in",
			MM_SWAP_IN_FORMAT,
			probe_mm_swap_in);
	if (!result)
		goto cleanup;
	result = marker_set_probe("mm_swap_out",
			MM_SWAP_OUT_FORMAT,
			probe_mm_swap_out);
	if (!result)
		goto cleanup;

	return 0;

cleanup:
	marker_remove_probe(probe_mm_filemap_wait_start);
	marker_remove_probe(probe_mm_filemap_wait_end);
	marker_remove_probe(probe_mm_page_free);
	marker_remove_probe(probe_mm_page_alloc);
	marker_remove_probe(probe_mm_swap_in);
	marker_remove_probe(probe_mm_swap_out);
	return -EPERM;
}

static void __exit probe_fini(void)
{
	marker_remove_probe(probe_mm_filemap_wait_start);
	marker_remove_probe(probe_mm_filemap_wait_end);
	marker_remove_probe(probe_mm_page_free);
	marker_remove_probe(probe_mm_page_alloc);
	marker_remove_probe(probe_mm_swap_in);
	marker_remove_probe(probe_mm_swap_out);
}


module_init(probe_init);
module_exit(probe_fini);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mathieu Desnoyers");
MODULE_DESCRIPTION("MM Probe");

