/*
 * TI DaVinci clock configuration code
 *
 * Copyright (C) 2006 Texas Instruments.
 * Copyright (C) 2008 MontaVista Software, Inc. <source@mvista.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/platform_device.h>

#include <asm/hardware.h>

#include <asm/arch/clock.h>
#include <asm/arch/mux.h>

static LIST_HEAD(clocks);
static DEFINE_MUTEX(clocks_mutex);
static DEFINE_SPINLOCK(clockfw_lock);

void (*davinci_module_setup)(unsigned ctlr, unsigned id, int enable);

static unsigned long *psc_bases;
static unsigned psc_num;

void __init davinci_psc_register(unsigned long *bases, unsigned num)
{
	psc_bases = bases;
	psc_num = num;
}

/* Enable or disable a PSC domain */
int davinci_psc_config(unsigned domain, unsigned ctlr, unsigned id, int enable)
{
	u32 epcpr, ptcmd, ptstat, pdstat, pdctl1, mdstat, mdctl;
	u32 state = enable ? 0x00000003 : 0x00000002;
	unsigned long psc_base;
	int error;

	if (ctlr >= psc_num)
		return -EINVAL;

	psc_base = psc_bases[ctlr];

	mdctl = davinci_readl(psc_base + MDCTL(id));
	mdctl &= ~0x0000001F;
	mdctl |= state;
	davinci_writel(mdctl, psc_base + MDCTL(id));

	pdstat = davinci_readl(psc_base + PDSTAT);
	if (!(pdstat & 0x00000001)) {
		pdctl1 = davinci_readl(psc_base + PDCTL1);
		pdctl1 |= 0x1;
		davinci_writel(pdctl1, psc_base + PDCTL1);

		ptcmd = 1 << domain;
		davinci_writel(ptcmd, psc_base + PTCMD);

		do {
			epcpr = davinci_readl(psc_base + EPCPR);
		} while (!((epcpr >> domain) & 1));

		pdctl1 = davinci_readl(psc_base + PDCTL1);
		pdctl1 |= 0x100;
		davinci_writel(pdctl1, psc_base + PDCTL1);

		do {
			ptstat = davinci_readl(psc_base + PTSTAT);
		} while ((ptstat >> domain) & 1);
	} else {
		ptcmd = 1 << domain;
		davinci_writel(ptcmd, psc_base + PTCMD);

		do {
			ptstat = davinci_readl(psc_base + PTSTAT);
		} while ((ptstat >> domain) & 1);
	}

	do {
		mdstat = davinci_readl(psc_base + MDSTAT(id));
	} while ((mdstat & 0x0000001F) != state);

	error = davinci_pinmux_setup(ctlr, id,
				     enable ? PINMUX_RESV : PINMUX_FREE);

	if (!error && davinci_module_setup != NULL)
		davinci_module_setup(ctlr, id, enable);

	return error;
}

/*
 * Returns a clock. Note that we first try to use device ID on the bus
 * and clock name. If this fails, we try to use clock name only.
 */
struct clk *clk_get(struct device *dev, const char *id)
{
	struct clk *p, *clk = ERR_PTR(-ENOENT);
	int idno;

	if (dev == NULL || dev->bus != &platform_bus_type)
		idno = -1;
	else
		idno = to_platform_device(dev)->id;

	mutex_lock(&clocks_mutex);

	list_for_each_entry(p, &clocks, node)
		if (p->id == idno &&
		    strcmp(id, p->name) == 0 && try_module_get(p->owner)) {
			clk = p;
			goto found;
		}

	list_for_each_entry(p, &clocks, node)
		if (strcmp(id, p->name) == 0 && try_module_get(p->owner)) {
			clk = p;
			break;
		}

found:
	mutex_unlock(&clocks_mutex);

	return clk;
}
EXPORT_SYMBOL(clk_get);

void clk_put(struct clk *clk)
{
	if (clk && !IS_ERR(clk))
		module_put(clk->owner);
}
EXPORT_SYMBOL(clk_put);

static int __clk_enable(struct clk *clk)
{
	if (clk->flags & ALWAYS_ENABLED)
		return 0;

	return davinci_psc_config(DAVINCI_GPSC_ARMDOMAIN,
				  clk->ctlr, clk->lpsc,	1);
}

static void __clk_disable(struct clk *clk)
{
	if (clk->usecount)
		return;

	davinci_psc_config(DAVINCI_GPSC_ARMDOMAIN, clk->ctlr, clk->lpsc, 0);
}

int clk_enable(struct clk *clk)
{
	unsigned long flags;
	int ret = 0;

	if (clk == NULL || IS_ERR(clk))
		return -EINVAL;

	if (clk->usecount++ == 0) {
		spin_lock_irqsave(&clockfw_lock, flags);
		ret = __clk_enable(clk);
		spin_unlock_irqrestore(&clockfw_lock, flags);
	}

	return ret;
}
EXPORT_SYMBOL(clk_enable);

void clk_disable(struct clk *clk)
{
	unsigned long flags;

	if (clk == NULL || IS_ERR(clk))
		return;

	if (clk->usecount > 0 && !(--clk->usecount)) {
		spin_lock_irqsave(&clockfw_lock, flags);
		__clk_disable(clk);
		spin_unlock_irqrestore(&clockfw_lock, flags);
	}
}
EXPORT_SYMBOL(clk_disable);

unsigned long clk_get_rate(struct clk *clk)
{
	if (clk == NULL || IS_ERR(clk))
		return -EINVAL;

	return *(clk->rate);
}
EXPORT_SYMBOL(clk_get_rate);

long clk_round_rate(struct clk *clk, unsigned long rate)
{
	if (clk == NULL || IS_ERR(clk))
		return -EINVAL;

	return *(clk->rate);
}
EXPORT_SYMBOL(clk_round_rate);

int clk_set_rate(struct clk *clk, unsigned long rate)
{
	if (clk == NULL || IS_ERR(clk))
		return -EINVAL;

	/* changing the clk rate is not supported */
	return -EINVAL;
}
EXPORT_SYMBOL(clk_set_rate);

int clk_register(struct clk *clk)
{
	if (clk == NULL || IS_ERR(clk))
		return -EINVAL;

	mutex_lock(&clocks_mutex);
	list_add(&clk->node, &clocks);
	mutex_unlock(&clocks_mutex);

	return 0;
}
EXPORT_SYMBOL(clk_register);

void clk_unregister(struct clk *clk)
{
	if (clk == NULL || IS_ERR(clk))
		return;

	mutex_lock(&clocks_mutex);
	list_del(&clk->node);
	mutex_unlock(&clocks_mutex);
}
EXPORT_SYMBOL(clk_unregister);

int __init davinci_enable_clks(struct clk *clk_list, int num_clks)
{
	struct clk *clkp;
	int i;

	for (i = 0, clkp = clk_list; i < num_clks; i++, clkp++) {
		clk_register(clkp);

		/* Turn on clocks that have been enabled in the table above */
		if (clkp->usecount)
			clk_enable(clkp);
	}

	return 0;
}

#ifdef CONFIG_PROC_FS
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

static void *davinci_ck_start(struct seq_file *m, loff_t *pos)
{
	return *pos < 1 ? (void *)1 : NULL;
}

static void *davinci_ck_next(struct seq_file *m, void *v, loff_t *pos)
{
	++*pos;
	return NULL;
}

static void davinci_ck_stop(struct seq_file *m, void *v)
{
}

static int davinci_ck_show(struct seq_file *m, void *v)
{
	struct clk *cp;

	list_for_each_entry(cp, &clocks, node)
		seq_printf(m, "%s %d %d\n", cp->name, *(cp->rate),
				cp->usecount);

	return 0;
}

static struct seq_operations davinci_ck_op = {
	.start	= davinci_ck_start,
	.next	= davinci_ck_next,
	.stop	= davinci_ck_stop,
	.show	= davinci_ck_show
};

static int davinci_ck_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &davinci_ck_op);
}

static struct file_operations proc_davinci_ck_operations = {
	.open		= davinci_ck_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= seq_release,
};

static int __init davinci_ck_proc_init(void)
{
	struct proc_dir_entry *entry;

	entry = create_proc_entry("davinci_clocks", 0, NULL);
	if (entry)
		entry->proc_fops = &proc_davinci_ck_operations;
	return 0;

}
device_initcall(davinci_ck_proc_init);
#endif	/* CONFIG_DEBUG_PROC_FS */
