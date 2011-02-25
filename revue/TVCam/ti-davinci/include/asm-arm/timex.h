/*
 *  linux/include/asm-arm/timex.h
 *
 *  Copyright (C) 1997,1998 Russell King
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  Architecture Specific TIME specifications
 */
#ifndef _ASMARM_TIMEX_H
#define _ASMARM_TIMEX_H

#include <asm/arch/timex.h>

typedef unsigned long cycles_t;

#ifndef mach_read_cycles
 #define mach_read_cycles() (0)
#endif

#ifdef CONFIG_LATENCY_TIMING
 #define mach_cycles_to_usecs(d) \
   (((d) * ((1000000LL << 32) / CLOCK_TICK_RATE)) >> 32)
 #define mach_usecs_to_cycles(d) \
   (((d) * (((long long)CLOCK_TICK_RATE << 32) / 1000000)) >> 32)
#endif

static inline cycles_t get_cycles (void)
{
	return mach_read_cycles();
}

#endif
