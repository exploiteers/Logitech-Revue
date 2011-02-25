/*
 * Malta Platform-specific hooks for SMP operation
 */

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/cpumask.h>
#include <linux/interrupt.h>

#include <asm/atomic.h>
#include <asm/cpu.h>
#include <asm/processor.h>
#include <asm/system.h>
#include <asm/hardirq.h>
#include <asm/mmu_context.h>
#include <asm/smp.h>
#ifdef CONFIG_MIPS_MT_SMTC
#include <asm/smtc_ipi.h>
#endif /* CONFIG_MIPS_MT_SMTC */

#ifdef CONFIG_MIPS_MT_SMTC_IRQAFF
/*
 * IRQ affinity hook
 */


void plat_set_irq_affinity(unsigned int irq, cpumask_t affinity)
{
	cpumask_t tmask = affinity;
	int cpu = 0;
	void smtc_set_irq_affinity(unsigned int irq, cpumask_t aff);

	/*
	 * On the legacy Malta development board, all I/O interrupts
	 * are routed through the 8259 and combined in a single signal
	 * to the CPU daughterboard, and on the CoreFPGA2/3 34K models,
	 * that signal is brought to IP2 of both VPEs. To avoid racing
	 * concurrent interrupt service events, IP2 is enabled only on
	 * one VPE, by convention VPE0.  So long as no bits are ever
	 * cleared in the affinity mask, there will never be any
	 * interrupt forwarding.  But as soon as a program or operator
	 * sets affinity for one of the related IRQs, we need to make
	 * sure that we don't ever try to forward across the VPE boundry,
	 * at least not until we engineer a system where the interrupt
	 * _ack() or _end() function can somehow know that it corresponds
	 * to an interrupt taken on another VPE, and perform the appropriate
	 * restoration of Status.IM state using MFTR/MTTR instead of the
	 * normal local behavior. We also ensure that no attempt will
	 * be made to forward to an offline "CPU".
	 */

	for_each_cpu_mask(cpu, affinity) {
		if ((cpu_data[cpu].vpe_id != 0) || !cpu_online(cpu))
			cpu_clear(cpu, tmask);
	}
	irq_desc[irq].affinity = tmask;

	if (cpus_empty(tmask))
		/*
		 * We could restore a default mask here, but the
		 * runtime code can anyway deal with the null set
		 */
		printk(KERN_WARNING
			"IRQ affinity leaves no legal CPU for IRQ %d\n", irq);

	/* Do any generic SMTC IRQ affinity setup */
	smtc_set_irq_affinity(irq, tmask);
}
#endif /* CONFIG_MIPS_MT_SMTC_IRQAFF */

/* VPE/SMP Prototype implements platform interfaces directly */
#if !defined(CONFIG_MIPS_MT_SMP)

/*
 * Cause the specified action to be performed on a targeted "CPU"
 */

void core_send_ipi(int cpu, unsigned int action)
{
/* "CPU" may be TC of same VPE, VPE of same CPU, or different CPU */
#ifdef CONFIG_MIPS_MT_SMTC
	smtc_send_ipi(cpu, LINUX_SMP_IPI, action);
#endif /* CONFIG_MIPS_MT_SMTC */
}

/*
 * Platform "CPU" startup hook
 */

void prom_boot_secondary(int cpu, struct task_struct *idle)
{
#ifdef CONFIG_MIPS_MT_SMTC
	smtc_boot_secondary(cpu, idle);
#endif /* CONFIG_MIPS_MT_SMTC */
}

/*
 * Post-config but pre-boot cleanup entry point
 */

void prom_init_secondary(void)
{
#ifdef CONFIG_MIPS_MT_SMTC
        void smtc_init_secondary(void);
	int myvpe;

	/* Don't enable Malta I/O interrupts (IP2) for secondary VPEs */
	myvpe = read_c0_tcbind() & TCBIND_CURVPE;
	if (myvpe != 0) {
		/* Ideally, this should be done only once per VPE, but... */
		clear_c0_status(STATUSF_IP2);
		set_c0_status(STATUSF_IP0 | STATUSF_IP1 | STATUSF_IP3
				| STATUSF_IP4 | STATUSF_IP5 | STATUSF_IP6
				| STATUSF_IP7);
	}

        smtc_init_secondary();
#endif /* CONFIG_MIPS_MT_SMTC */
}

/*
 * Platform SMP pre-initialization
 *
 * As noted above, we can assume a single CPU for now
 * but it may be multithreaded.
 */

void plat_smp_setup(void)
{
	if (read_c0_config3() & (1<<2))
		mipsmt_build_cpu_map(0);
}

void __init plat_prepare_cpus(unsigned int max_cpus)
{
	if (read_c0_config3() & (1<<2))
		mipsmt_prepare_cpus();
}

/*
 * SMP initialization finalization entry point
 */

void prom_smp_finish(void)
{
#ifdef CONFIG_MIPS_MT_SMTC
	smtc_smp_finish();
#endif /* CONFIG_MIPS_MT_SMTC */
}

/*
 * Hook for after all CPUs are online
 */

void prom_cpus_done(void)
{
#ifdef CONFIG_HIGH_RES_TIMERS
	extern void sync_c0_count_master(void);
	sync_c0_count_master();
#endif
}

#endif /* CONFIG_MIPS32R2_MT_SMP */
