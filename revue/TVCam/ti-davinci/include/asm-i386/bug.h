#ifndef _I386_BUG_H
#define _I386_BUG_H


/*
 * Tell the user there is some problem.
 * The offending file and line are encoded after the "officially
 * undefined" opcode for parsing in the trap handler.
 */

#ifdef CONFIG_BUG
#define HAVE_ARCH_BUG
#ifdef CONFIG_DEBUG_BUGVERBOSE

#ifdef CONFIG_LATENCY_TRACE
  extern void stop_trace(void);
#else
# define stop_trace()			do { } while (0)
#endif

#define BUG()				\
do {					\
stop_trace();				\
printk("BUG at %s:%d!\n", __FILE__, __LINE__); \
 __asm__ __volatile__(	"ud2\n"		\
			"\t.word %c0\n"	\
			"\t.long %c1\n"	\
			 : : "i" (__LINE__), "i" (__FILE__)); \
} while (0)
#else
#define BUG() __asm__ __volatile__("ud2\n")
#endif
#endif

#include <asm-generic/bug.h>
#endif
