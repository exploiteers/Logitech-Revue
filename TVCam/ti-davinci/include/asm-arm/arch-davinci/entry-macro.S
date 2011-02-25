/*
 * Low-level IRQ helper macros for TI DaVinci-based platforms
 *
 * Author: Kevin Hilman, MontaVista Software, Inc. <source@mvista.com>
 *
 * 2007 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */
#include <asm/arch/io.h>
#include <asm/arch/irqs.h>

		.macro	disable_fiq
		.endm

		.macro	get_irqnr_and_base, irqnr, irqstat, base, tmp
#ifdef CONFIG_ARCH_DA8XX
		ldr \base, =CP_INTC_VIRT
		ldr \irqnr, [\base, #0x80] /* get irq number */
		and \irqnr, \irqnr, #0xff  /* irq is in bits 0-9 */
		mov \tmp, \irqnr, lsr #3
		and \tmp, \tmp, #0xfc
		add \tmp, \tmp, #0x280 /* get the register offset */
		ldr \irqstat, [\base, \tmp] /* get the intc status */
		cmp \irqstat, #0x0
#else
		ldr \base, =IO_ADDRESS(DAVINCI_ARM_INTC_BASE)
		ldr \tmp, [\base, #0x14]
		mov \tmp, \tmp, lsr #2
		sub \irqnr, \tmp, #1
		cmp \tmp, #0
#endif
		.endm

		.macro	irq_prio_table
		.endm
