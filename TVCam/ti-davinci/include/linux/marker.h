#ifndef _LINUX_MARKER_H
#define _LINUX_MARKER_H

/*
 * marker.h
 *
 * Code markup for dynamic and static tracing.
 *
 * Example :
 *
 * MARK(subsystem_event, "%d %s", someint, somestring);
 * Where :
 * - Subsystem is the name of your subsystem.
 * - event is the name of the event to mark.
 * - "%d %s" is the formatted string for printk.
 * - someint is an integer.
 * - somestring is a char *.
 *
 * - Dynamically overridable function call based on marker mechanism
 *   from Frank Ch. Eigler <fche@redhat.com>.
 * - Thanks to Jeremy Fitzhardinge <jeremy@goop.org> for his constructive
 *   criticism about gcc optimization related issues.
 *
 * The marker mechanism supports multiple instances of the same marker.
 * Markers can be put in inline functions, inlined static functions and
 * unrolled loops.
 *
 * (C) Copyright 2006 Mathieu Desnoyers <mathieu.desnoyers@polymtl.ca>
 *
 * This file is released under the GPLv2.
 * See the file COPYING for more details.
 */

#ifndef __ASSEMBLY__

typedef void marker_probe_func(const char *fmt, ...);

#ifndef CONFIG_MARKERS_DISABLE_OPTIMIZATION
#include <asm/marker.h>
#else
#include <asm-generic/marker.h>
#endif

#define MARK_NOARGS " "
#define MARK_MAX_FORMAT_LEN	1024

#ifndef CONFIG_MARKERS
#define MARK(name, format, args...) \
		__mark_check_format(format, ## args)
#endif

static inline __attribute__ ((format (printf, 1, 2)))
void __mark_check_format(const char *fmt, ...)
{ }

extern marker_probe_func __mark_empty_function;

extern int marker_set_probe(const char *name, const char *format,
				marker_probe_func *probe);

extern int marker_remove_probe(marker_probe_func *probe);
extern int marker_list_probe(marker_probe_func *probe);

#endif
#endif
