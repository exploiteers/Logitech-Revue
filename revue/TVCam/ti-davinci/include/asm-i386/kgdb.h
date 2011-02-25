#ifdef __KERNEL__
#ifndef _ASM_KGDB_H_
#define _ASM_KGDB_H_

#include <asm-generic/kgdb.h>

/*
 * Copyright (C) 2001-2004 Amit S. Kale
 */

/************************************************************************/
/* BUFMAX defines the maximum number of characters in inbound/outbound buffers*/
/* at least NUMREGBYTES*2 are needed for register packets */
/* Longer buffer is needed to list all threads */
#define BUFMAX			1024

/* Number of bytes of registers.  */
#define NUMREGBYTES		64
/* Number of bytes of registers we need to save for a setjmp/longjmp. */
#define NUMCRITREGBYTES		24

/*
 *  Note that this register image is in a different order than
 *  the register image that Linux produces at interrupt time.
 *
 *  Linux's register image is defined by struct pt_regs in ptrace.h.
 *  Just why GDB uses a different order is a historical mystery.
 */
enum regnames { _EAX,		/* 0 */
	_ECX,			/* 1 */
	_EDX,			/* 2 */
	_EBX,			/* 3 */
	_ESP,			/* 4 */
	_EBP,			/* 5 */
	_ESI,			/* 6 */
	_EDI,			/* 7 */
	_PC,			/* 8 also known as eip */
	_PS,			/* 9 also known as eflags */
	_CS,			/* 10 */
	_SS,			/* 11 */
	_DS,			/* 12 */
	_ES,			/* 13 */
	_FS,			/* 14 */
	_GS			/* 15 */
};

#define BREAKPOINT()		asm("   int $3");
#define BREAK_INSTR_SIZE	1
#define CACHE_FLUSH_IS_SAFE	1
#endif				/* _ASM_KGDB_H_ */
#endif				/* __KERNEL__ */
