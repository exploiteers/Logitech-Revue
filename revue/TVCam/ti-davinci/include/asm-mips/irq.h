/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1994 by Waldorf GMBH, written by Ralf Baechle
 * Copyright (C) 1995, 96, 97, 98, 99, 2000, 01, 02, 03 by Ralf Baechle
 */
#ifndef _ASM_IRQ_H
#define _ASM_IRQ_H

#include <linux/linkage.h>

#include <asm/mipsmtregs.h>

#include <irq.h>

#ifdef CONFIG_I8259
static inline int irq_canonicalize(int irq)
{
	return ((irq == 2) ? 9 : irq);
}
#else
#define irq_canonicalize(irq) (irq)	/* Sane hardware, sane code ... */
#endif

struct pt_regs;

extern asmlinkage unsigned int do_IRQ(unsigned int irq, struct pt_regs *regs);

#ifdef CONFIG_MIPS_MT_SMTC
/*
 * Clear interrupt mask handling "backstop" if irq_hwmask
 * entry so indicates. This implies that the ack() or end()
 * functions will take over re-enabling the low-level mask.
 * Otherwise it will be done on return from exception.
 */
#define __DO_IRQ_SMTC_HOOK(irq, regs)					\
do {								\
	IRQ_AFFINITY_HOOK(irq, regs);					\
	if (irq_hwmask[irq] & 0x0000ff00)			\
		write_c0_tccontext(read_c0_tccontext() &	\
			~(irq_hwmask[irq] & 0x0000ff00));	\
} while (0)

#define __NO_AFFINITY_IRQ_SMTC_HOOK(irq)			\
do {								\
	if (irq_hwmask[irq] & 0x0000ff00)			\
		write_c0_tccontext(read_c0_tccontext() &	\
			   ~(irq_hwmask[irq] & 0x0000ff00));	\
} while (0)

#else

#define __DO_IRQ_SMTC_HOOK(irq, regs)						\
do {									\
	IRQ_AFFINITY_HOOK(irq, regs);						\
} while (0)
#define __NO_AFFINITY_IRQ_SMTC_HOOK(irq) do { } while (0)
#endif /* CONFIG_MIPS_MT_SMTC */

/*
 * do_IRQ handles all normal device IRQ's (the special
 * SMP cross-CPU interrupts have their own specific
 * handlers).
 *
 * Ideally there should be away to get this into kernel/irq/handle.c to
 * avoid the overhead of a call for just a tiny function ...
 */
#define do_IRQ(irq, regs)						\
do {									\
	irq_enter();							\
	__DO_IRQ_SMTC_HOOK(irq, regs);					\
	__do_IRQ((irq), (regs));					\
	irq_exit();							\
} while (0)

#ifdef CONFIG_MIPS_MT_SMTC_IRQAFF
/*
 * To avoid inefficient and in some cases pathological re-checking of
 * IRQ affinity, we have this variant that skips the affinity check.
 */

#define do_IRQ_no_affinity(irq, regs)						\
do {									\
	irq_enter();							\
	__NO_AFFINITY_IRQ_SMTC_HOOK(irq);				\
	generic_handle_irq(irq, regs);					\
	irq_exit();							\
} while (0)

#endif /* CONFIG_MIPS_MT_SMTC_IRQAFF */

extern void arch_init_irq(void);
extern void spurious_interrupt(struct pt_regs *regs);

#ifdef CONFIG_MIPS_MT_SMTC
struct irqaction;

extern unsigned long irq_hwmask[];
extern int setup_irq_smtc(unsigned int irq, struct irqaction * new,
                          unsigned long hwmask);
#endif /* CONFIG_MIPS_MT_SMTC */

#ifdef CONFIG_MIPS_MT_SMTC_IRQAFF
#include <linux/cpumask.h>

extern void plat_set_irq_affinity(unsigned int irq, cpumask_t affinity);
extern void smtc_forward_irq(unsigned int irq, struct pt_regs *regs);

/*
 * IRQ affinity hook invoked at the beginning of interrupt dispatch
 * if option is enabled.
 *
 * Up through Linux 2.6.22 (at least) cpumask operations are very
 * inefficient on MIPS.  Initial prototypes of SMTC IRQ affinity
 * used a "fast path" per-IRQ-descriptor cache of affinity information
 * to reduce latency.  As there is a project afoot to optimize the
 * cpumask implementations, this version is optimistically assuming
 * that cpumask.h macro overhead is reasonable during interrupt dispatch.
 */
#define IRQ_AFFINITY_HOOK(irq, regs)					\
do {									\
    if (!cpu_isset(smp_processor_id(), irq_desc[irq].affinity)) {	\
	smtc_forward_irq(irq, regs);					\
	irq_exit();							\
	return;								\
    }									\
} while (0)

#else /* Not doing SMTC affinity */

#define IRQ_AFFINITY_HOOK(irq, regs) do { } while (0)

#endif /* CONFIG_MIPS_MT_SMTC_IRQAFF */

#ifdef CONFIG_SMP
#define ARCH_HAS_IRQ_PER_CPU
#endif

#endif /* _ASM_IRQ_H */
