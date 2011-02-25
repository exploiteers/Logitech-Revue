#ifdef __KERNEL__
#ifndef _ASM_KGDB_H_
#define _ASM_KGDB_H_

#include <asm-generic/kgdb.h>

#ifndef __ASSEMBLY__
#if (_MIPS_ISA == _MIPS_ISA_MIPS1) || (_MIPS_ISA == _MIPS_ISA_MIPS2) || (_MIPS_ISA == _MIPS_ISA_MIPS32)

typedef u32 gdb_reg_t;

#elif (_MIPS_ISA == _MIPS_ISA_MIPS3) || (_MIPS_ISA == _MIPS_ISA_MIPS4) || (_MIPS_ISA == _MIPS_ISA_MIPS64)

#ifdef CONFIG_32BIT
typedef u32 gdb_reg_t;
#else /* CONFIG_CPU_32BIT */
typedef u64 gdb_reg_t;
#endif
#else
#error "Need to set typedef for gdb_reg_t"
#endif /* _MIPS_ISA */

#define BUFMAX			2048
#define NUMREGBYTES		(90*sizeof(gdb_reg_t))
#define NUMCRITREGBYTES		(12*sizeof(gdb_reg_t))
#define BREAK_INSTR_SIZE	4
#define BREAKPOINT()		__asm__ __volatile__(		\
					".globl breakinst\n\t"	\
					".set\tnoreorder\n\t"	\
					"nop\n"			\
					"breakinst:\tbreak\n\t"	\
					"nop\n\t"		\
					".set\treorder")
#define CACHE_FLUSH_IS_SAFE	0

extern int kgdb_early_setup;

#endif				/* !__ASSEMBLY__ */
#endif				/* _ASM_KGDB_H_ */
#endif				/* __KERNEL__ */
