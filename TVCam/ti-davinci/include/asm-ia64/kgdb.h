#ifdef __KERNEL__
#ifndef _ASM_KGDB_H_
#define _ASM_KGDB_H_

/*
 * Copyright (C) 2001-2004 Amit S. Kale
 */

#include <linux/threads.h>
#include <asm-generic/kgdb.h>

/************************************************************************/
/* BUFMAX defines the maximum number of characters in inbound/outbound buffers*/
/* at least NUMREGBYTES*2 are needed for register packets */
/* Longer buffer is needed to list all threads */
#define BUFMAX			1024

/* Number of bytes of registers.  We set this to 0 so that certain GDB
 * packets will fail, forcing the use of others, which are more friendly
 * on ia64. */
#define NUMREGBYTES		0

#define NUMCRITREGBYTES		(70*8)
#define JMP_REGS_ALIGNMENT	__attribute__ ((aligned (16)))

#define BREAKNUM		0x00003333300LL
#define KGDBBREAKNUM		0x6665UL
#define BREAKPOINT()		asm volatile ("break.m 0x6665")
#define BREAK_INSTR_SIZE	16
#define CACHE_FLUSH_IS_SAFE	1

struct pt_regs;
extern volatile int kgdb_hwbreak_sstep[NR_CPUS];
extern void smp_send_nmi_allbutself(void);
extern void kgdb_wait_ipi(struct pt_regs *);
#endif				/* _ASM_KGDB_H_ */
#endif				/* __KERNEL__ */
