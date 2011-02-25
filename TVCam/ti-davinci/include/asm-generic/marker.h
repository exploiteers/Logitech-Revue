/*
 * marker.h
 *
 * Code markup for dynamic and static tracing. Generic header.
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
	const struct __mark_marker_c *cmark;
	volatile char *enable;
} __attribute__((packed));

#ifdef CONFIG_MARKERS

#define MARK(name, format, args...) \
	do { \
		static marker_probe_func *__mark_call_##name = \
					__mark_empty_function; \
		volatile static char __marker_enable_##name = 0; \
		static const struct __mark_marker_c __mark_c_##name \
			__attribute__((section(".markers.c"))) = \
			{ #name, &__mark_call_##name, format } ; \
		static const struct __mark_marker __mark_##name \
			__attribute__((section(".markers"), used)) = \
			{ &__mark_c_##name, &__marker_enable_##name } ; \
 		asm volatile ( "" : : "g" (&__mark_##name)); \
		__mark_check_format(format, ## args); \
		if (unlikely(__marker_enable_##name)) { \
			preempt_disable(); \
			(*__mark_call_##name)(format, ## args); \
			preempt_enable_no_resched(); \
		} \
	} while (0)


#define MARK_ENABLE_IMMEDIATE_OFFSET 0

#endif
