/*
 * TI Common Platform Interrupt Controller (cp_intc) driver
 *
 * Author: Steve Chen <schen@mvista.com>
 * Copyright (C) 2008, MontaVista Software, Inc. <source@mvista.com>
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */

#include <linux/init.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <asm/arch/hardware.h>
#include <asm/arch/common.h>
#include <asm/hardware/cp_intc.h>

static void __iomem *cp_intc_base;

static inline unsigned int cp_intc_read(unsigned offset)
{
	return __raw_readl(cp_intc_base + offset);
}

static inline void cp_intc_write(unsigned long value, unsigned offset)
{
	__raw_writel(value, cp_intc_base + offset);
}

static void cp_intc_ack_irq(unsigned int irq)
{
	cp_intc_write(irq, CP_INTC_SYS_STAT_IDX_CLR);
}

/* Disable interrupt */
static void cp_intc_mask_irq(unsigned int irq)
{
	/* XXX don't know why we need to disable nIRQ here... */
	cp_intc_write(1, CP_INTC_HOST_ENABLE_IDX_CLR);
	cp_intc_write(irq, CP_INTC_SYS_ENABLE_IDX_CLR);
	cp_intc_write(1, CP_INTC_HOST_ENABLE_IDX_SET);
}

/* Enable interrupt */
static void cp_intc_unmask_irq(unsigned int irq)
{
	cp_intc_write(irq, CP_INTC_SYS_ENABLE_IDX_SET);
}

static int cp_intc_set_irq_type(unsigned int irq, unsigned int flow_type)
{
	unsigned reg		= irq >> 5;
	unsigned mask		= irq & 0x1f;
	unsigned pol_mask	= cp_intc_read(CP_INTC_SYS_POLARITY(reg));
	unsigned type_mask	= cp_intc_read(CP_INTC_SYS_TYPE(reg));

	switch (flow_type) {
	case IRQ_TYPE_EDGE_RISING:
		pol_mask  |= mask;
		type_mask |= mask;
		break;
	case IRQ_TYPE_EDGE_FALLING:
		pol_mask  &= ~mask;
		type_mask |= mask;
		break;
	case IRQ_TYPE_LEVEL_HIGH:
		pol_mask  |= mask;
		type_mask &= ~mask;
		break;
	case IRQ_TYPE_LEVEL_LOW:
		pol_mask  &= ~mask;
		type_mask &= ~mask;
		break;
	default:
		return -1;
	}

	cp_intc_write(pol_mask,  CP_INTC_SYS_POLARITY(reg));
	cp_intc_write(type_mask, CP_INTC_SYS_TYPE(reg));
	return 0;
}

static struct irq_chip cp_intc_irq_chip = {
	.name		= "cp_intc",
	.ack		= cp_intc_ack_irq,
	.mask		= cp_intc_mask_irq,
	.unmask		= cp_intc_unmask_irq,
	.set_type	= cp_intc_set_irq_type,
};

void __init cp_intc_init(void __iomem *base, unsigned short num_irq,
			 u8 *irq_prio)
{
	unsigned num_reg	= (num_irq + 0x1f) >> 5;
	int i;

	cp_intc_base = base;

	cp_intc_write(0, CP_INTC_GLOBAL_ENABLE);

	/* Disable all host interrupts */
	cp_intc_write(0, CP_INTC_HOST_ENABLE(0));

	/* Disable system interrupts */
	for (i = 0; i < num_reg; i++)
		cp_intc_write(~0, CP_INTC_SYS_ENABLE_CLR(i));

	/* Set to normal mode, no nesting, no priority hold */
	cp_intc_write(0, CP_INTC_CTRL);
	cp_intc_write(0, CP_INTC_HOST_CTRL);

	/* Clear system interrupt status */
	for (i = 0; i < num_reg; i++)
		cp_intc_write(~0, CP_INTC_SYS_STAT_CLR(i));

	/* Enable nIRQ (what about nFIQ?) */
	cp_intc_write(1, CP_INTC_HOST_ENABLE_IDX_SET);

	/*
	 * Priority is determined by host channel: lower channel number has
	 * higher priority i.e. channel 0 has highest priority and channel 31
	 * had the lowest priority.
	 */
	num_reg = (num_irq + 3) >> 2; /* 4 channels per register */
	if (irq_prio) {
		unsigned j, k;
		u32 val;

		for (k = i = 0; i < num_reg; i++) {
			for (val = j = 0; j < 4; j++, k++) {
				val >>= 8;
				if (k < num_irq)
					val |= irq_prio[k] << 24;
			}

			cp_intc_write(val, CP_INTC_CHAN_MAP(i));
		}
	} else {
		/*
		 * Default everything to channel 15 if priority not specified.
		 * Note that channel 0-1 are mapped to nFIQ and channels 2-31
		 * are mapped to nIRQ.
		 */
		for (i = 0; i < num_reg; i++)
			cp_intc_write(0x0f0f0f0f, CP_INTC_CHAN_MAP(i));
	}

	/* Set up genirq dispatching for cp_intc */
	for (i = 0; i < num_irq; i++) {
		set_irq_chip(i, &cp_intc_irq_chip);
		set_irq_flags(i, IRQF_VALID | IRQF_PROBE);
		set_irq_handler(i, handle_edge_irq);
	}

	/* Enable global interrupt */
	cp_intc_write(1, CP_INTC_GLOBAL_ENABLE);
}
