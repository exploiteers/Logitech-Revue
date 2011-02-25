/*
 * linux/include/asm-powerpc/ltt.h
 *
 * Copyright (C) 2005, Mathieu Desnoyers
 *
 * POWERPC definitions for tracing system
 */

#ifndef _ASM_POWERPC_LTT_H
#define _ASM_POWERPC_LTT_H

#define LTT_HAS_TSC

/* Current arch type */
#define LTT_ARCH_TYPE LTT_ARCH_TYPE_POWERPC

/* Current variant type */
#define LTT_ARCH_VARIANT LTT_ARCH_VARIANT_NONE

#include <linux/ltt-core.h>
#include <asm/timex.h>
#include <asm/time.h>
#include <asm/processor.h>

u64 ltt_heartbeat_read_synthetic_tsc(void);

static inline u32 ltt_get_timestamp32(void)
{
	return get_tbl();
}

static inline u64 ltt_get_timestamp64(void)
{
	return get_tb();
}

static inline unsigned int ltt_frequency(void)
{
	return tb_ticks_per_sec;
}

static inline u32 ltt_freq_scale(void)
{
	return 1;
}

#endif //_ASM_POWERPC_LTT_H
