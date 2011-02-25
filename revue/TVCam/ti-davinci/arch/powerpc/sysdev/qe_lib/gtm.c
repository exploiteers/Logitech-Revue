/*
 * QE General-Purpose Timers Module
 *
 * Copyright (c) Freescale Semicondutor, Inc. 2006.
 *               Shlomi Gridish <gridish@freescale.com>
 *               Jerry Huang <Chang-Ming.Huang@freescale.com>
 * Copyright (c) MontaVista Software, Inc. 2008.
 *               Anton Vorontsov <avorontsov@ru.mvista.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/io.h>
#include <linux/spinlock.h>
#include <asm/of_platform.h>
#include <asm/immap_qe.h>
#include <asm/qe.h>

struct gtm_timer {
	unsigned int irq;
	bool requested;

	u8 __iomem *gtcfr;
	u16 __iomem *gtmdr;
	u16 __iomem *gtpsr;
	u16 __iomem *gtcnr;
	u16 __iomem *gtrfr;
	u16 __iomem *gtevr;
};

static struct gtm_timer timers[4];
static struct qe_timers __iomem *qet;
static spinlock_t gtm_lock = __SPIN_LOCK_UNLOCKED(gtm_lock);

static int __init qe_init_gtm(void)
{
	struct device_node *gtm;
	int i;

	gtm = of_find_compatible_node(NULL, NULL, "fsl,qe-gtm");
	if (!gtm)
		return -ENODEV;

	for (i = 0; i < 3; i++) {
		int ret;
		struct resource irq;

		ret = of_irq_to_resource(gtm, i, &irq);
		if (ret == NO_IRQ) {
			pr_err("%s: not enough interrupts specified\n",
			       gtm->full_name);
			of_node_put(gtm);
			return -EINVAL;
		}
		timers[i].irq = irq.start;
	}

	qet = of_iomap(gtm, 0);
	of_node_put(gtm);
	if (!qet) {
		pr_err("%s: unable to iomap registers\n", gtm->full_name);
		return -EINVAL;
	}

	/*
	 * Yeah, I don't like this either, but timers' registers a bit messed,
	 * so we have to provide shortcuts to write timer independent code.
	 */
	timers[0].gtcfr = &qet->gtcfr1;
	timers[0].gtmdr = &qet->gtmdr1;
	timers[0].gtpsr = &qet->gtpsr1;
	timers[0].gtcnr = &qet->gtcnr1;
	timers[0].gtrfr = &qet->gtrfr1;
	timers[0].gtevr = &qet->gtevr1;

	timers[1].gtcfr = &qet->gtcfr1;
	timers[1].gtmdr = &qet->gtmdr2;
	timers[1].gtpsr = &qet->gtpsr2;
	timers[1].gtcnr = &qet->gtcnr2;
	timers[1].gtrfr = &qet->gtrfr2;
	timers[1].gtevr = &qet->gtevr2;

	timers[2].gtcfr = &qet->gtcfr2;
	timers[2].gtmdr = &qet->gtmdr3;
	timers[2].gtpsr = &qet->gtpsr3;
	timers[2].gtcnr = &qet->gtcnr3;
	timers[2].gtrfr = &qet->gtrfr3;
	timers[2].gtevr = &qet->gtevr3;

	timers[3].gtcfr = &qet->gtcfr2;
	timers[3].gtmdr = &qet->gtmdr4;
	timers[3].gtpsr = &qet->gtpsr4;
	timers[3].gtcnr = &qet->gtcnr4;
	timers[3].gtrfr = &qet->gtrfr4;
	timers[3].gtevr = &qet->gtevr4;

	return 0;
}
arch_initcall(qe_init_gtm);

int qe_get_timer(int width, unsigned int *irq)
{
	int i;

	BUG_ON(!irq);
	if (!qet)
		return -ENODEV;
	if (width != 16)
		return -ENOSYS;

	spin_lock_irq(&gtm_lock);

	for (i = 0; i < 3; i++) {
		if (!timers[i].requested) {
			timers[i].requested = true;
			*irq = timers[i].irq;
			spin_unlock_irq(&gtm_lock);
			return i;
		}
	}

	spin_unlock_irq(&gtm_lock);

	return 0;
}
EXPORT_SYMBOL(qe_get_timer);

void qe_put_timer(int num)
{
	spin_lock_irq(&gtm_lock);

	timers[num].requested = false;

	spin_unlock_irq(&gtm_lock);
}
EXPORT_SYMBOL(qe_put_timer);

int qe_reset_ref_timer_16(int num, unsigned int hz, u16 ref)
{
	struct gtm_timer *tmr = &timers[num];
	unsigned long flags;
	unsigned int prescaler;
	u8 psr;
	u8 sps;

	prescaler = qe_get_brg_clk() / hz;

	/*
	 * We have two 8 bit prescalers -- primary and secondary (psr, sps),
	 * so total prescale value is (psr + 1) * (sps + 1).
	 */
	if (prescaler > 256 * 256) {
		return -EINVAL;
	} else if (prescaler > 256) {
		psr = 256 - 1;
		sps = prescaler / 256 - 1;
	} else {
		psr = prescaler - 1;
		sps = 1 - 1;
	}

	spin_lock_irqsave(&gtm_lock, flags);

	/*
	 * Properly reset timers: stop, reset, set up prescalers, reference
	 * value and clear event register.
	 */
	clrsetbits_8(tmr->gtcfr, ~(QE_GTCFR_STP(num) | QE_GTCFR_RST(num)),
				 QE_GTCFR_STP(num) | QE_GTCFR_RST(num));

	setbits8(tmr->gtcfr, QE_GTCFR_STP(num));

	out_be16(tmr->gtpsr, psr);
	setbits16(tmr->gtmdr, QE_GTMDR_SPS(sps) | QE_GTMDR_ICLK_QERF |
			      QE_GTMDR_ORI);
	out_be16(tmr->gtcnr, 0);
	out_be16(tmr->gtrfr, ref);
	out_be16(tmr->gtevr, 0xFFFF);

	/* Let it be. */
	clrbits8(tmr->gtcfr, QE_GTCFR_STP(num));

	spin_unlock_irqrestore(&gtm_lock, flags);

	return 0;
}
EXPORT_SYMBOL(qe_reset_ref_timer_16);

void qe_stop_timer(int num)
{
	struct gtm_timer *tmr = &timers[num];
	unsigned long flags;

	spin_lock_irqsave(&gtm_lock, flags);

	setbits8(tmr->gtcfr, QE_GTCFR_STP(num));

	spin_unlock_irqrestore(&gtm_lock, flags);
}
EXPORT_SYMBOL(qe_stop_timer);
