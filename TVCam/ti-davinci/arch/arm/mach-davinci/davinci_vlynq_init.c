/*******************************************************************************
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * ----------------------------------------------------------------------------
*******************************************************************************/
#include <linux/module.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <asm/mach-types.h>
#include <asm/arch/i2c-client.h>
#include <asm/arch/clock.h>

static int __init vlynq_platform_init(void)
{
	struct clk *vlynq_clock;

	printk(KERN_INFO "Enabling VLYNQ clock\n");
	vlynq_clock = clk_get(NULL, "VLYNQ_CLK");
	if (IS_ERR(vlynq_clock))
		return -1;

	clk_enable(vlynq_clock);

	if (machine_is_davinci_dm6467_evm()) {
		printk(KERN_INFO "Enable VLYNQ Switch using I2C...\n");
		davinci_i2c_expander_op(VSCALE_ON_DM646X, 0);
	}

	return 0;
}

MODULE_AUTHOR("Texas Instruments India");
MODULE_LICENSE("GPL");

late_initcall(vlynq_platform_init);
