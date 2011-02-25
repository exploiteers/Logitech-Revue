/*
 * marker.h
 *
 * Code markup for dynamic and static tracing. i386 architecture optimisations.
 *
 * (C) Copyright 2006 Mathieu Desnoyers <mathieu.desnoyers@polymtl.ca>
 *
 * This file is released under the GPLv2.
 * See the file COPYING for more details.
 */


struct __mark_marker_c {
	const char *name;
	marker_probe_func **call;
	const char *format;
} __attribute__((packed));

struct __mark_marker {
	struct __mark_marker_c *cmark;
	volatile char *enable;
} __attribute__((packed));

#ifdef CONFIG_MARKERS
#define MARK(name, format, args...) \
	do { \
		static marker_probe_func *__mark_call_##name = \
					__mark_empty_function; \
		static const struct __mark_marker_c __mark_c_##name \
			__attribute__((section(".markers.c"))) = \
			{ #name, &__mark_call_##name, format } ; \
		char condition; \
		asm volatile(	".section .markers, \"a\";\n\t" \
					".long %1, 0f;\n\t" \
					".previous;\n\t" \
					".align 2\n\t" \
					"0:\n\t" \
					"movb $0,%0;\n\t" \
				: "=r" (condition) \
				: "m" (__mark_c_##name)); \
		__mark_check_format(format, ## args); \
		if (unlikely(condition)) { \
			preempt_disable(); \
			(*__mark_call_##name)(format, ## args); \
			preempt_enable_no_resched(); \
		} \
	} while (0)

/* Offset of the immediate value from the start of the movb instruction, in
 * bytes. */
#define MARK_ENABLE_IMMEDIATE_OFFSET 1

#endif
