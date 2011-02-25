/* ltt-probe-locking.c
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
#include <ltt/ltt-facility-locking.h>

#define KERNEL_HARDIRQS_ON_FORMAT "%lu"
void probe_kernel_hardirqs_on(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	unsigned long ip;

	/* Assign args */
	va_start(ap, format);
	ip = va_arg(ap, typeof(ip));

	/* Call tracer */
	trace_locking_hardirqs_on(ip);
	
	va_end(ap);
}

#define KERNEL_HARDIRQS_OFF_FORMAT "%lu"
void probe_kernel_hardirqs_off(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	unsigned long ip;

	/* Assign args */
	va_start(ap, format);
	ip = va_arg(ap, typeof(ip));

	/* Call tracer */
	trace_locking_hardirqs_off(ip);
	
	va_end(ap);
}

#define KERNEL_SOFTIRQS_ON_FORMAT "%lu"
void probe_kernel_softirqs_on(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	unsigned long ip;

	/* Assign args */
	va_start(ap, format);
	ip = va_arg(ap, typeof(ip));

	/* Call tracer */
	trace_locking_softirqs_on(ip);
	
	va_end(ap);
}

#define KERNEL_SOFTIRQS_OFF_FORMAT "%lu"
void probe_kernel_softirqs_off(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	unsigned long ip;

	/* Assign args */
	va_start(ap, format);
	ip = va_arg(ap, typeof(ip));

	/* Call tracer */
	trace_locking_softirqs_off(ip);
	
	va_end(ap);
}

#define KERNEL_LOCK_ACQUIRE_FORMAT "%lu %u struct lockdep_map %p %d"
void probe_kernel_lock_acquire(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	unsigned long ip;
	unsigned int subclass;
	void *lock;
	int trylock;

	/* Assign args */
	va_start(ap, format);
	ip = va_arg(ap, typeof(ip));
	subclass = va_arg(ap, typeof(subclass));
	lock = va_arg(ap, typeof(lock));
	trylock = va_arg(ap, typeof(trylock));

	/* Call tracer */
	trace_locking_lock_acquire(ip, subclass, lock, trylock);
	
	va_end(ap);
}

#define KERNEL_LOCK_RELEASE_FORMAT "%lu struct lockdep_map %p %d"
void probe_kernel_lock_release(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	unsigned long ip;
	void *lock;
	int nested;

	/* Assign args */
	va_start(ap, format);
	ip = va_arg(ap, typeof(ip));
	lock = va_arg(ap, typeof(lock));
	nested = va_arg(ap, typeof(nested));

	/* Call tracer */
	trace_locking_lock_release(ip, lock, nested);
	
	va_end(ap);
}


static int __init probe_init(void)
{
	int result;
	result = marker_set_probe("kernel_hardirqs_on",
			KERNEL_HARDIRQS_ON_FORMAT,
			probe_kernel_hardirqs_on);
	if (!result)
		goto cleanup;
	result = marker_set_probe("kernel_hardirqs_off",
			KERNEL_HARDIRQS_OFF_FORMAT,
			probe_kernel_hardirqs_off);
	if (!result)
		goto cleanup;
	result = marker_set_probe("kernel_softirqs_on",
			KERNEL_SOFTIRQS_ON_FORMAT,
			probe_kernel_softirqs_on);
	if (!result)
		goto cleanup;
	result = marker_set_probe("kernel_softirqs_off",
			KERNEL_SOFTIRQS_OFF_FORMAT,
			probe_kernel_softirqs_off);
	if (!result)
		goto cleanup;
	result = marker_set_probe("kernel_lock_acquire",
			KERNEL_LOCK_ACQUIRE_FORMAT,
			probe_kernel_lock_acquire);
	if (!result)
		goto cleanup;
	result = marker_set_probe("kernel_lock_release",
			KERNEL_LOCK_RELEASE_FORMAT,
			probe_kernel_lock_release);
	if (!result)
		goto cleanup;

	return 0;

cleanup:
	marker_remove_probe(probe_kernel_hardirqs_on);
	marker_remove_probe(probe_kernel_hardirqs_off);
	marker_remove_probe(probe_kernel_softirqs_on);
	marker_remove_probe(probe_kernel_softirqs_off);
	marker_remove_probe(probe_kernel_lock_acquire);
	marker_remove_probe(probe_kernel_lock_release);
	return -EPERM;
}

static void __exit probe_fini(void)
{
	marker_remove_probe(probe_kernel_hardirqs_on);
	marker_remove_probe(probe_kernel_hardirqs_off);
	marker_remove_probe(probe_kernel_softirqs_on);
	marker_remove_probe(probe_kernel_softirqs_off);
	marker_remove_probe(probe_kernel_lock_acquire);
	marker_remove_probe(probe_kernel_lock_release);
}


module_init(probe_init);
module_exit(probe_fini);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mathieu Desnoyers");
MODULE_DESCRIPTION("Locking Probe");
