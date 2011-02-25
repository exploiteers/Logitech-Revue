/*
 * DaVinci timer subsystem
 *
 * Author: Kevin Hilman, MontaVista Software, Inc.
 * Copyright (C) 2007-2008 MontaVista Software, Inc. <source@mvista.com>
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */
#include <linux/kernel.h>
#include <linux/clockchips.h>
#include <linux/irq.h>

#include <asm/arch/time.h>

static struct clock_event_device clockevent_davinci;

static int tid_system;
static int tid_freerun;

struct timer_s {
	char *name;
	unsigned int id;
	unsigned long period;
	unsigned long opts;
	unsigned long reg_base;
	unsigned long tim_reg;
	unsigned long prd_reg;
	unsigned long cmp_reg;
	unsigned long enamode_shift;
	struct irqaction irqaction;
	struct irqaction cmpaction;
};

/* values for 'opts' field of struct timer_s */
#define TIMER_OPTS_DISABLED   0x00
#define TIMER_OPTS_ONESHOT    0x01
#define TIMER_OPTS_PERIODIC   0x02

static void timer32_config(struct timer_s *t)
{
	u32 tcr = davinci_readl(t->reg_base + TCR);

	/* Disable timer */
	tcr &= ~(TCR_ENAMODE_MASK << t->enamode_shift);
	davinci_writel(tcr, t->reg_base + TCR);

	/* Reset counter to zero, set new period */
	davinci_writel(0, t->tim_reg);
	davinci_writel(t->period, t->prd_reg);
	if (t->cmp_reg)
		davinci_writel(t->period, t->cmp_reg);

	/* Set enable mode */
	if (t->opts & TIMER_OPTS_ONESHOT)
		tcr |= TCR_ENAMODE_ONESHOT << t->enamode_shift;
	else if (t->opts & TIMER_OPTS_PERIODIC)
		tcr |= TCR_ENAMODE_PERIODIC << t->enamode_shift;

	davinci_writel(tcr, t->reg_base + TCR);
}

static inline u32 timer32_read(struct timer_s *t)
{
	return davinci_readl(t->tim_reg);
}

static irqreturn_t timer_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	clockevent_davinci.event_handler(regs);
	return IRQ_HANDLED;
}

/* Called when 32-bit counter wraps */
static irqreturn_t freerun_interrupt(int irq, void *dev_id,
				     struct pt_regs *regs)
{
	return IRQ_HANDLED;
}

static irqreturn_t cmp_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	struct timer_s *t = dev_id;

	/* We have to emulate the periodic mode for the clockevents layer */
	if (t->opts & TIMER_OPTS_PERIODIC) {
		unsigned long tim, cmp = davinci_readl(t->cmp_reg);

		cmp += t->period;
		davinci_writel(cmp, t->cmp_reg);

		/*
		 * The interrupts do happen  to be disabled by the kernel for
		 * a long periods of time, thus the timer can go far ahead of
		 * the last set compare value...
		 */
		tim = davinci_readl(t->tim_reg);
		if (time_after(tim, cmp))
			davinci_writel(tim + t->period, t->cmp_reg);
	}

	clockevent_davinci.event_handler(regs);
	return IRQ_HANDLED;
}

static struct timer_s davinci_system_timer = {
	.name		= "clockevent",
	.opts		= TIMER_OPTS_DISABLED,
	.irqaction	= {
		.flags		= IRQF_DISABLED | IRQF_TIMER,
		.handler	= timer_interrupt
	}
};

static struct timer_s davinci_freerun_timer = {
	.name		= "free-run counter",
	.period 	= ~0,
	.opts		= TIMER_OPTS_PERIODIC,
	.irqaction	= {
		.flags		= IRQF_DISABLED | IRQF_TIMER,
		.handler	= freerun_interrupt
	},
	.cmpaction	= {
		.name		= "timer compare reg 0",
		.flags		= IRQF_DISABLED | IRQF_TIMER,
		.handler	= cmp_interrupt
	}
};
static struct timer_s *timers[NUM_TIMERS];

static void __init timer_init(int num_timers64, u32 *bases,
			      int *timer_irqs,	int *cmp_irqs)
{
	int i;

	/* Global init of each 64-bit timer as a whole */
	for (i = 0; i < num_timers64; i++) {
		u32 tgcr, base = bases[i];

		/* Disabled, Internal clock source */
		davinci_writel(0, base + TCR);

		/* reset both timers, no pre-scaler for timer34 */
		davinci_writel(0, base + TGCR);

		/* Set both timers to unchained 32-bit */
		tgcr = TGCR_TIMMODE_32BIT_UNCHAINED << TGCR_TIMMODE_SHIFT;
		davinci_writel(tgcr, base + TGCR);

		/* Unreset timers */
		tgcr |= (TGCR_UNRESET << TGCR_TIM12RS_SHIFT) |
			(TGCR_UNRESET << TGCR_TIM34RS_SHIFT);
		davinci_writel(tgcr, base + TGCR);

		/* Init both counters to zero */
		davinci_writel(0, base + TIM12);
		davinci_writel(0, base + TIM34);
	}

	/* Init of each timer as a 32-bit timer */
	for (i = 0; i < NUM_TIMERS; i++) {
		struct timer_s *t = timers[i];

		if (t && t->name) {
			t->id = i;
			t->reg_base = bases[i >> 1];

			if (IS_TIMER_BOT(i)) {
				t->enamode_shift = ENAMODE12_SHIFT;
				t->tim_reg = t->reg_base + TIM12;
				t->prd_reg = t->reg_base + PRD12;
				/* Check the compare register IRQ */
				if (t->cmpaction.handler != NULL &&
				    cmp_irqs != NULL && cmp_irqs[t->id]) {
					t->cmp_reg = t->reg_base + CMP12(0);
					t->cmpaction.dev_id = (void *)t;
					setup_irq(cmp_irqs[t->id],
						  &t->cmpaction);
				}
			} else {
				t->enamode_shift = ENAMODE34_SHIFT;
				t->tim_reg = t->reg_base + TIM34;
				t->prd_reg = t->reg_base + PRD34;
			}

			/* Register interrupt */
			t->irqaction.name = t->name;
			t->irqaction.dev_id = (void *)t;
			if (t->irqaction.handler != NULL)
				setup_irq(timer_irqs[i], &t->irqaction);

			timer32_config(timers[i]);
		}
	}
}

/*
 * clocksource
 */
static cycle_t read_cycles(void)
{
	struct timer_s *t = timers[tid_freerun];

	return (cycles_t)timer32_read(t);
}

static struct clocksource clocksource_davinci = {
	.name		= "timer0_1",
	.rating		= 300,
	.read		= read_cycles,
	.mask		= CLOCKSOURCE_MASK(32),
	.shift		= 24,
	.is_continuous	= 1,
};

/*
 * Clockevent code
 */
static void davinci_set_next_event(unsigned long cycles,
				   struct clock_event_device *evt)
{
	struct timer_s *t = timers[tid_system];

	t->period = cycles;

	/*
	 * We need not (and must not) disable the timer and reprogram
	 * its mode/period when using the compare register...
	 */
	if (t->cmp_reg)
		davinci_writel(davinci_readl(t->tim_reg) + cycles, t->cmp_reg);
	else
		timer32_config(t);
}

static void davinci_set_mode(enum clock_event_mode mode,
			     struct clock_event_device *evt)
{
	struct timer_s *t = timers[tid_system];

	switch (mode) {
	case CLOCK_EVT_PERIODIC:
		t->opts = TIMER_OPTS_PERIODIC;
		davinci_set_next_event(DAVINCI_CLOCK_TICK_RATE / HZ, evt);
		break;
	case CLOCK_EVT_ONESHOT:
		t->opts = TIMER_OPTS_ONESHOT;
		break;
	case CLOCK_EVT_SHUTDOWN:
		t->opts = TIMER_OPTS_DISABLED;
		break;
	}
}

static struct clock_event_device clockevent_davinci = {
	.name		= "timer0_0",
	.capabilities	= CLOCK_CAPS_MASK,
	.shift		= 32,
	.set_next_event	= davinci_set_next_event,
	.set_mode	= davinci_set_mode,
};

static char error[] __initdata = KERN_ERR "%s: can't register clocksource!\n";

void __init davinci_common_timer_init(u32 *bases, int num_timers64,
				      int timer_irqs[NUM_TIMERS], int *cmp_irqs,
				      int system, int freerun)
{
	tid_system  = system;
	tid_freerun = freerun;

	timers[tid_system]  = &davinci_system_timer;
	timers[tid_freerun] = &davinci_freerun_timer;

	/* Init timer hardware */
	timer_init(num_timers64, bases, timer_irqs, cmp_irqs);

	/* Setup clocksource */
	clocksource_davinci.mult =
		clocksource_khz2mult(DAVINCI_CLOCK_TICK_RATE / 1000,
				     clocksource_davinci.shift);
	if (clocksource_register(&clocksource_davinci))
		printk(error, clocksource_davinci.name);

	/* Setup clockevent */
	clockevent_davinci.mult = div_sc(DAVINCI_CLOCK_TICK_RATE, NSEC_PER_SEC,
					 clockevent_davinci.shift);
	clockevent_davinci.max_delta_ns =
		clockevent_delta2ns(0xfffffffe, &clockevent_davinci);
	clockevent_davinci.min_delta_ns =
		clockevent_delta2ns(1, &clockevent_davinci);

	register_global_clockevent(&clockevent_davinci);
}

/* Reset board using the watchdog timer */
void davinci_watchdog_reset(u32 base)
{
	u32 tgcr, wdtcr;

	/* Disable, internal clock source */
	davinci_writel(0, base + TCR);

	/* Reset timer, set mode to 64-bit watchdog, and unreset */
	davinci_writel(0, base + TGCR);
	tgcr =	(TGCR_TIMMODE_64BIT_WDOG << TGCR_TIMMODE_SHIFT) |
		(TGCR_UNRESET << TGCR_TIM12RS_SHIFT) |
		(TGCR_UNRESET << TGCR_TIM34RS_SHIFT);
	davinci_writel(tgcr, base + TGCR);

	/* Clear counter and period regs */
	davinci_writel(0, base + TIM12);
	davinci_writel(0, base + TIM34);
	davinci_writel(0, base + PRD12);
	davinci_writel(0, base + PRD34);

	/* Enable periodic mode */
	davinci_writel(TCR_ENAMODE_PERIODIC << ENAMODE12_SHIFT, base + TCR);

	/* Put watchdog in pre-active state */
	wdtcr = (WDTCR_WDKEY_SEQ0 << WDTCR_WDKEY_SHIFT) |
		(WDTCR_WDEN_ENABLE << WDTCR_WDEN_SHIFT);
	davinci_writel(wdtcr, base + WDTCR);

	/* Put watchdog in active state */
	wdtcr = (WDTCR_WDKEY_SEQ1 << WDTCR_WDKEY_SHIFT) |
		(WDTCR_WDEN_ENABLE << WDTCR_WDEN_SHIFT);
	davinci_writel(wdtcr, base + WDTCR);

	/*
	 * Write an invalid value to the WDKEY field to trigger
	 * a watchdog reset.
	 */
	wdtcr = 0xDEADBEEF;
	davinci_writel(wdtcr, base + WDTCR);
}
