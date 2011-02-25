/*
 * Copyright (C) 1999 Cort Dougan <cort@cs.nmt.edu>
 */
#ifndef _ASM_POWERPC_HW_IRQ_H
#define _ASM_POWERPC_HW_IRQ_H

#ifdef __KERNEL__

#include <linux/errno.h>
#include <asm/ptrace.h>
#include <asm/processor.h>

extern void timer_interrupt(struct pt_regs *);

#ifdef CONFIG_PPC_ISERIES

extern unsigned long raw_local_get_flags(void);
extern unsigned long raw_local_irq_disable(void);
extern void raw_local_irq_restore(unsigned long);

#define raw_local_irq_enable()	raw_local_irq_restore(1)
#define raw_local_save_flags(flags)	((flags) = raw_local_get_flags())
#define raw_local_irq_save(flags)	((flags) = raw_local_irq_disable())

#define raw_irqs_disabled()			(raw_local_get_flags() == 0)
#define raw_irqs_disabled_flags(flags)	((flags) == 0)

#else

#if defined(CONFIG_BOOKE)
#define SET_MSR_EE(x)	mtmsr(x)
#define raw_local_irq_restore(flags)	__asm__ __volatile__("wrtee %0" : : "r" (flags) : "memory")
#elif defined(__powerpc64__)
#define SET_MSR_EE(x)	__mtmsrd(x, 1)
#define raw_local_irq_restore(flags) do { \
	__asm__ __volatile__("": : :"memory"); \
	__mtmsrd((flags), 1); \
} while(0)
#else
#define SET_MSR_EE(x)	mtmsr(x)
#define raw_local_irq_restore(flags)	mtmsr(flags)
#endif

static inline void raw_local_irq_disable(void)
{
#ifdef CONFIG_BOOKE
	__asm__ __volatile__("wrteei 0": : :"memory");
#else
	unsigned long msr;
	__asm__ __volatile__("": : :"memory");
	msr = mfmsr();
	SET_MSR_EE(msr & ~MSR_EE);
#endif
}

static inline void raw_local_irq_enable(void)
{
#ifdef CONFIG_BOOKE
	__asm__ __volatile__("wrteei 1": : :"memory");
#else
	unsigned long msr;
	__asm__ __volatile__("": : :"memory");
	msr = mfmsr();
	SET_MSR_EE(msr | MSR_EE);
#endif
}

static inline void raw_local_irq_save_ptr(unsigned long *flags)
{
	unsigned long msr;
	msr = mfmsr();
	*flags = msr;
#ifdef CONFIG_BOOKE
	__asm__ __volatile__("wrteei 0": : :"memory");
#else
	SET_MSR_EE(msr & ~MSR_EE);
#endif
	__asm__ __volatile__("": : :"memory");
}

#define raw_local_save_flags(flags)		((flags) = mfmsr())
#define raw_local_irq_save(flags)		raw_local_irq_save_ptr(&flags)
#define raw_irqs_disabled()			((mfmsr() & MSR_EE) == 0)
#define raw_irqs_disabled_flags(flags)	((flags & MSR_EE) == 0)

#endif /* CONFIG_PPC_ISERIES */

#include <linux/irqflags.h>

#define mask_irq(irq)						\
	({							\
	 	irq_desc_t *desc = get_irq_desc(irq);		\
		if (desc->chip && desc->chip->disable)	\
			desc->chip->disable(irq);		\
	})
#define unmask_irq(irq)						\
	({							\
	 	irq_desc_t *desc = get_irq_desc(irq);		\
		if (desc->chip && desc->chip->enable)	\
			desc->chip->enable(irq);		\
	})
#define ack_irq(irq)						\
	({							\
	 	irq_desc_t *desc = get_irq_desc(irq);		\
		if (desc->chip && desc->chip->ack)	\
			desc->chip->ack(irq);		\
	})

/*
 * interrupt-retrigger: should we handle this via lost interrupts and IPIs
 * or should we not care like we do now ? --BenH.
 */
struct hw_interrupt_type;

#endif	/* __KERNEL__ */
#endif	/* _ASM_POWERPC_HW_IRQ_H */
