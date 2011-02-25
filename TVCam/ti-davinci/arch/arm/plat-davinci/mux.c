/*
 * Utility to set the DaVINCI PINMUX registers from a table
 *
 * Author: Vladimir Barinov, MontaVista Software, Inc.
 * Copyright (C) 2007-2008 MontaVista Software, Inc. <source@mvista.com>
 *
 * Based on linux/arch/arm/plat-omap/mux.c:
 * Copyright (C) 2003 - 2005 Nokia Corporation
 * Written by Tony Lindgren <tony.lindgren@nokia.com>
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/io.h>

#include <asm/arch/mux.h>
#include <asm/arch/hardware.h>

static unsigned long *pinmux_in_use;
static const struct pin_config *pin_table;
static unsigned pin_table_sz;
const short *(*get_pin_list)(unsigned ctlr, unsigned id);

void __init davinci_mux_register(const struct pin_config *pins, unsigned size,
				 const short *(*get_pins)(unsigned, unsigned),
				 unsigned long *in_use)
{
	pin_table = pins;
	pin_table_sz = size;
	get_pin_list = get_pins;
	pinmux_in_use = in_use;
}

static int __init_or_module davinci_find_conflict(unsigned long mux_reg,
						  unsigned long mode)
{
	int i;

	for (i = 0; i < pin_table_sz; i++) {
		if (pin_table[i].mux_reg == mux_reg &&
		    pin_table[i].mode << pin_table[i].mask_offset == mode)
			return i;
	}
	return -1;
}

/*
 * Sets the DAVINCI MUX register based on the table
 */
int __init_or_module davinci_cfg_reg(unsigned index, int action)
{
	static DEFINE_SPINLOCK(mux_spin_lock);
	const struct pin_config *cfg;
	unsigned long flags, reg_orig = 0, reg = 0;
	const char *conflict_name = "???";
	int conflict, warn = 0;

	if (pin_table == NULL)
		BUG();

	if (index >= pin_table_sz) {
		printk(KERN_ERR "Invalid PINMUX index: %u, max %u\n",
		       index, pin_table_sz);
		dump_stack();
		return -ENODEV;
	}

	/* Check the PinMux register in question */
	cfg = pin_table + index;
	//trl_();trvi_(action);trvs(cfg->name);
	if (cfg->mux_reg) {
		unsigned long mask = cfg->mask << cfg->mask_offset;
		unsigned long mode = cfg->mode << cfg->mask_offset;

		spin_lock_irqsave(&mux_spin_lock, flags);
		reg_orig = davinci_cfg_readl(cfg->mux_reg);
		reg = (reg_orig & ~mask) | mode;

		if ((reg_orig & mask) != mode)
			warn = 1;

		/* Check if the pin is reserved */
		if (pinmux_in_use[cfg->reg_index] & mask) {
			conflict = davinci_find_conflict(cfg->mux_reg,
							 reg_orig & mask);
			if (conflict >= 0)
				conflict_name = pin_table[conflict].name;
			if (action == PINMUX_FREE) {
				if (warn) {
					printk(KERN_ERR "%s unreserve failed. "
						"Pin used by %s\n",
						cfg->name, conflict_name);
					spin_unlock_irqrestore(&mux_spin_lock,
							       flags);
					return -EBUSY;
				}
				pinmux_in_use[cfg->reg_index] &= ~mask;
				spin_unlock_irqrestore(&mux_spin_lock, flags);
				return 0;
			}

			if (warn) {
				printk(KERN_ERR "Pin %s already used for %s.\n",
					cfg->name, conflict_name);
				spin_unlock_irqrestore(&mux_spin_lock, flags);
				return -EBUSY;
			}
		} else	{
			if (action == PINMUX_FREE) {
				spin_unlock_irqrestore(&mux_spin_lock, flags);
				return 0;
			}
			pinmux_in_use[cfg->reg_index] |= mask;
		}
		davinci_cfg_writel(reg, cfg->mux_reg);
		spin_unlock_irqrestore(&mux_spin_lock, flags);
	}

#ifdef CONFIG_DAVINCI_MUX_WARNINGS
	if (warn)
		printk(KERN_WARNING "MUX: initialized %s\n", cfg->name);
#endif

#ifdef CONFIG_DAVINCI_MUX_DEBUG
	if (cfg->debug || warn) {
		printk(KERN_INFO "MUX: setting register PINMUX%u (%#08lx) "
		       "for %s: %#08lx -> %#08lx\n", cfg->reg_index,
		       cfg->mux_reg, cfg->name, reg_orig, reg);
	}
#endif
	return 0;
}
EXPORT_SYMBOL(davinci_cfg_reg);

int davinci_pinmux_setup(unsigned ctlr, unsigned id, int action)
{
	const short *pins = NULL;
	int i, error = 0;

	if (get_pin_list != NULL)
		pins = get_pin_list(ctlr, id);

	if (pins != NULL) {
		for (i = 0; pins[i] >= 0; i++) {
			error = davinci_cfg_reg(pins[i], action);
			if (error)
				break;
		}
		/* Free the previously reserved pins if an error occured */
		if (error)
			for (i--; i >= 0; i--)
				davinci_cfg_reg(pins[i], PINMUX_FREE);
	}
	return error;
}
