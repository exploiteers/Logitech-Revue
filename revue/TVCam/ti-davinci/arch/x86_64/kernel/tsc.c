#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/clocksource.h>
#include <linux/time.h>
#include <linux/acpi.h>
#include <linux/cpufreq.h>

#include <asm/timex.h>

#define NS_SCALE	10 /* 2^10, carefully chosen */
#define US_SCALE	32 /* 2^32, arbitralrily chosen */

static int notsc __initdata = 0;

unsigned int cpu_khz;					/* TSC clocks / usec, not used here */
EXPORT_SYMBOL(cpu_khz);

static unsigned int cyc2ns_scale __read_mostly;

void set_cyc2ns_scale(unsigned long khz)
{
	cyc2ns_scale = (NSEC_PER_MSEC << NS_SCALE) / khz;
}

static inline unsigned long long cycles_2_ns(unsigned long long cyc)
{
	return (cyc * cyc2ns_scale) >> NS_SCALE;
}

unsigned long long sched_clock(void)
{
	unsigned long a = 0;

	/* Could do CPU core sync here. Opteron can execute rdtsc speculatively,
	   which means it is not completely exact and may not be monotonous between
	   CPUs. But the errors should be too small to matter for scheduling
	   purposes. */

	rdtscll(a);
	return cycles_2_ns(a);
}

static int tsc_unstable;

static inline int check_tsc_unstable(void)
{
	return tsc_unstable;
}

void mark_tsc_unstable(void)
{
	tsc_unstable = 1;
}
EXPORT_SYMBOL_GPL(mark_tsc_unstable);

#ifdef CONFIG_CPU_FREQ

/* Frequency scaling support. Adjust the TSC based timer when the cpu frequency
   changes.

   RED-PEN: On SMP we assume all CPUs run with the same frequency.  It's
   not that important because current Opteron setups do not support
   scaling on SMP anyroads.

   Should fix up last_tsc too. Currently gettimeofday in the
   first tick after the change will be slightly wrong. */

#include <linux/workqueue.h>

static unsigned int cpufreq_delayed_issched = 0;
static unsigned int cpufreq_init = 0;
static struct work_struct cpufreq_delayed_get_work;

static void handle_cpufreq_delayed_get(void *v)
{
	unsigned int cpu;
	for_each_online_cpu(cpu) {
		cpufreq_get(cpu);
	}
	cpufreq_delayed_issched = 0;
}

static unsigned int  ref_freq = 0;
static unsigned long loops_per_jiffy_ref = 0;

static unsigned long cpu_khz_ref = 0;

static int time_cpufreq_notifier(struct notifier_block *nb, unsigned long val,
				 void *data)
{
        struct cpufreq_freqs *freq = data;
	unsigned long *lpj, dummy;

	if (cpu_has(&cpu_data[freq->cpu], X86_FEATURE_CONSTANT_TSC))
		return 0;

	lpj = &dummy;
	if (!(freq->flags & CPUFREQ_CONST_LOOPS))
#ifdef CONFIG_SMP
		lpj = &cpu_data[freq->cpu].loops_per_jiffy;
#else
		lpj = &boot_cpu_data.loops_per_jiffy;
#endif

	if (!ref_freq) {
		ref_freq = freq->old;
		loops_per_jiffy_ref = *lpj;
		cpu_khz_ref = cpu_khz;
	}
        if ((val == CPUFREQ_PRECHANGE  && freq->old < freq->new) ||
            (val == CPUFREQ_POSTCHANGE && freq->old > freq->new) ||
	    (val == CPUFREQ_RESUMECHANGE)) {
                *lpj =
		cpufreq_scale(loops_per_jiffy_ref, ref_freq, freq->new);

		cpu_khz = cpufreq_scale(cpu_khz_ref, ref_freq, freq->new);
		if (!(freq->flags & CPUFREQ_CONST_LOOPS))
			mark_tsc_unstable();
	}

	set_cyc2ns_scale(cpu_khz_ref);

	return 0;
}

static struct notifier_block time_cpufreq_notifier_block = {
	.notifier_call  = time_cpufreq_notifier
};

static int __init cpufreq_tsc(void)
{
	INIT_WORK(&cpufreq_delayed_get_work, handle_cpufreq_delayed_get, NULL);
	if (!cpufreq_register_notifier(&time_cpufreq_notifier_block,
				       CPUFREQ_TRANSITION_NOTIFIER))
		cpufreq_init = 1;
	return 0;
}

core_initcall(cpufreq_tsc);

#endif
/*
 * Make an educated guess if the TSC is trustworthy and synchronized
 * over all CPUs.
 */
__cpuinit int unsynchronized_tsc(void)
{
#ifdef CONFIG_SMP
	if (apic_is_clustered_box())
		return 1;
#endif
	/* Most intel systems have synchronized TSCs except for
	   multi node systems */
 	if (boot_cpu_data.x86_vendor == X86_VENDOR_INTEL) {
#ifdef CONFIG_ACPI
		/* But TSC doesn't tick in C3 so don't use it there */
		if (acpi_fadt.length > 0 && acpi_fadt.plvl3_lat < 100)
			return 1;
#endif
 		return 0;
	}

 	/* Assume multi socket systems are not synchronized */
 	return num_present_cpus() > 1;
}

int __init notsc_setup(char *s)
{
	notsc = 1;
	return 1;
}

__setup("notsc", notsc_setup);


/* clock source code: */

static int tsc_update_callback(void);

static cycle_t read_tsc(void)
{
	cycle_t ret = (cycle_t)get_cycles_sync();
	return ret;
}

static cycle_t __vsyscall_fn vread_tsc(void)
{
	cycle_t ret = (cycle_t)get_cycles_sync();
	return ret;
}

static struct clocksource clocksource_tsc = {
	.name			= "tsc",
	.rating			= 300,
	.read			= read_tsc,
	.mask			= (cycle_t)-1,
	.mult			= 0, /* to be set */
	.shift			= 22,
	.update_callback	= tsc_update_callback,
	.is_continuous		= 1,
	.vread			= vread_tsc,
};

static int tsc_update_callback(void)
{
	int change = 0;

	/* check to see if we should switch to the safe clocksource: */
	if (clocksource_tsc.rating != 50 && check_tsc_unstable()) {
		clocksource_tsc.rating = 50;
		clocksource_reselect();
		change = 1;
	}
	return change;
}

static int __init init_tsc_clocksource(void)
{
	if (!notsc) {
		clocksource_tsc.mult = clocksource_khz2mult(cpu_khz,
							clocksource_tsc.shift);
		return clocksource_register(&clocksource_tsc);
	}
	return 0;
}

module_init(init_tsc_clocksource);
