#ifndef _ASM_X86_64_LTT_H
#define _ASM_X86_64_LTT_H
/*
+ * linux/include/asm-x86_64/ltt.h
+ *
+ * x86_64 time and TSC definitions for ltt
+ */

#include <asm/timex.h>
#include <asm/processor.h>

#define LTT_ARCH_TYPE LTT_ARCH_TYPE_X86_64
#define LTT_ARCH_VARIANT LTT_ARCH_VARIANT_NONE

#define LTTNG_LOGICAL_SHIFT 13

extern atomic_t lttng_logical_clock;

/* The shift overflow doesn't matter
 * We use the xtime seq_lock to protect 64 bits clock and
 * 32 bits ltt logical clock coherency.
 *
 * try 5 times. If it still fails, we are cleary in a NMI nested over
 * the seq_lock. Return 0 -> error.
 *
 * 0 is considered an erroneous value.
 */

static inline u32 ltt_timestamp_no_tsc_32(void)
{
	unsigned long seq;
	unsigned long try = 5;
	u32 ret;

	do {
		seq = read_seqbegin(&xtime_lock);
		ret = (jiffies << LTTNG_LOGICAL_SHIFT) 
			| (atomic_add_return(1, &lttng_logical_clock));
	} while (read_seqretry(&xtime_lock, seq) && (--try) > 0);

	if (try == 0)
		return 0;
	else
		return ret;
}


static inline u64 ltt_timestamp_no_tsc(void)
{
	unsigned long seq;
	unsigned long try = 5;
	u64 ret;

	do {
		seq = read_seqbegin(&xtime_lock);
		ret = (jiffies_64 << LTTNG_LOGICAL_SHIFT) 
			| (atomic_add_return(1, &lttng_logical_clock));
	} while (read_seqretry(&xtime_lock, seq) && (--try) > 0);

	if (try == 0)
		return 0;
	else
		return ret;
}

#ifdef CONFIG_LTT_SYNTHETIC_TSC
u64 ltt_heartbeat_read_synthetic_tsc(void);
#endif //CONFIG_LTT_SYNTHETIC_TSC

static inline u32 ltt_get_timestamp32(void)
{
#ifndef CONFIG_X86_TSC
	if (!cpu_has_tsc)
		return ltt_timestamp_no_tsc32();
#endif

#if defined(CONFIG_X86_GENERIC) || defined(CONFIG_X86_TSC)
	return get_cycles(); /* only need the 32 LSB */
#else
	return ltt_timestamp_no_tsc32();
#endif
}

static inline u64 ltt_get_timestamp64(void)
{
#ifndef CONFIG_X86_TSC
	if (!cpu_has_tsc)
		return ltt_timestamp_no_tsc64();
#endif

#if defined(CONFIG_X86_GENERIC) || defined(CONFIG_X86_TSC)
#ifdef CONFIG_LTT_SYNTHETIC_TSC
	return ltt_heartbeat_read_synthetic_tsc();
#else
	return get_cycles();
#endif //CONFIG_LTT_SYNTHETIC_TSC
#else
	return ltt_timestamp_no_tsc64();
#endif
}

/* this has to be called with the write seqlock held */
static inline void ltt_reset_timestamp(void)
{
#ifndef CONFIG_X86_TSC
	if (!cpu_has_tsc) {
		atomic_set(&lttng_logical_clock, 0);
		return;
	}
#endif

#if defined(CONFIG_X86_GENERIC) || defined(CONFIG_X86_TSC)
	return;
#else
	atomic_set(&lttng_logical_clock, 0);
	return;
#endif
}

static inline unsigned int ltt_frequency(void)
{
#ifndef CONFIG_X86_TSC
	if (!cpu_has_tsc)
  	return HZ << LTTNG_LOGICAL_SHIFT;
#endif

#if defined(CONFIG_X86_GENERIC) || defined(CONFIG_X86_TSC)
	return cpu_khz;
#else
	return HZ << LTTNG_LOGICAL_SHIFT;
#endif
}

static inline u32 ltt_freq_scale(void)
{
#ifndef CONFIG_X86_TSC
	if (!cpu_has_tsc)
  	return 1;
#endif

#if defined(CONFIG_X86_GENERIC) || defined(CONFIG_X86_TSC)
	return 1000;
#else
	return 1;
#endif

}

#endif //_ASM_X86_64_LTT_H
