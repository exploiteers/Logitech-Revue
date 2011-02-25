/*
 * DaVinci timer definitions
 *
 * Author: Kevin Hilman, MontaVista Software, Inc.
 * (C) 2007-2008 MontaVista Software, Inc. <source@mvista.com>
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */
#ifndef __ASM_ARCH_TIME_H
#define __ASM_ARCH_TIME_H

/* Timer register offsets */
#define PID12				0x0
#define TIM12				0x10
#define TIM34				0x14
#define PRD12				0x18
#define PRD34				0x1c
#define TCR				0x20
#define TGCR				0x24
#define WDTCR				0x28
#define CMP12(n)			(0x60 + ((n) << 2))

/* Timer register bitfields */
#define ENAMODE12_SHIFT 		6
#define ENAMODE34_SHIFT 		22
#define TCR_ENAMODE_DISABLE		0x0
#define TCR_ENAMODE_ONESHOT		0x1
#define TCR_ENAMODE_PERIODIC		0x2
#define TCR_ENAMODE_MASK		0x3

#define TGCR_TIMMODE_SHIFT		2
#define TGCR_TIMMODE_64BIT_GP		0x0
#define TGCR_TIMMODE_32BIT_UNCHAINED	0x1
#define TGCR_TIMMODE_64BIT_WDOG 	0x2
#define TGCR_TIMMODE_32BIT_CHAINED	0x3

#define TGCR_TIM12RS_SHIFT		0
#define TGCR_TIM34RS_SHIFT		1
#define TGCR_RESET			0x0
#define TGCR_UNRESET			0x1
#define TGCR_RESET_MASK 		0x3

#define WDTCR_WDEN_SHIFT		14
#define WDTCR_WDEN_DISABLE		0x0
#define WDTCR_WDEN_ENABLE		0x1
#define WDTCR_WDKEY_SHIFT		16
#define WDTCR_WDKEY_SEQ0		0xA5C6
#define WDTCR_WDKEY_SEQ1		0xDA7E

enum {
	T0_BOT,
	T0_TOP,
	T1_BOT,
	T1_TOP,
	NUM_TIMERS
};

#define IS_TIMER_TOP(id) ((id) & 0x1)
#define IS_TIMER_BOT(id) (!IS_TIMER_TOP(id))

void davinci_common_timer_init(u32 *bases, int num_timers64,
			       int timer_irqs[NUM_TIMERS], int *cmp_irqs,
			       int system, int freerun);

void davinci_watchdog_reset(u32 base);

#endif /* __ASM_ARCH_TIME_H__ */
