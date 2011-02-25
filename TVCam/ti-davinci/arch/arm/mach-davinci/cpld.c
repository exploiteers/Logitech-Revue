/*
 * Copyright (C) 2008 - 2009 Texas Instruments	Inc
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either	version 2 of the License, or
 * (at your option)any	later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not,write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <linux/kernel.h>
#include <linux/config.h>
#include <linux/init.h>
#include <linux/ctype.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <asm/arch/hardware.h>
#include <linux/io.h>
#include <asm/arch/cpld.h>

static void *__iomem cpld_base_addr;

u32 cpld_read(u32 offset)
{
	if (cpu_is_davinci_dm365()) {
		if (offset < DM365_CPLD_ADDR_SIZE)
			return __raw_readl(cpld_base_addr + offset);
		else
			printk(KERN_ERR "offset exceeds DM365 cpld address space\n");
	}
	return -1;
}
EXPORT_SYMBOL(cpld_read);

u32 cpld_write(u32 val, u32 offset)
{
	if (cpu_is_davinci_dm365()) {
		if (offset < DM365_CPLD_ADDR_SIZE) {
			__raw_writel(val, cpld_base_addr + offset);
			return val;
		} else
			printk(KERN_ERR "offset exceeds DM365 cpld address space\n");
	}
	return -1;
}
EXPORT_SYMBOL(cpld_write);

int cpld_init(void)
{
	u32 val1;

	if (cpu_is_davinci_dm365()) {
		cpld_base_addr = ioremap_nocache(DM365_CPLD_BASE_ADDR,
					 DM365_CPLD_ADDR_SIZE);
		if (!cpld_base_addr) {
			printk(KERN_ERR "Couldn't io map CPLD registers\n");
			return -ENXIO;
		}
                
                printk(KERN_INFO "enable CCD power Vout4\n");
		// enable CCD power Vout4
		val1 = cpld_read(DM365_CPLD_REGISTER5);
		val1 |= DM365_CCD_POWER_MASK;
		cpld_write(val1, DM365_CPLD_REGISTER5);

		mt9xxx_set_input_mux(0);

	}
	return 0;
}

void cpld_cleanup(void)
{
	iounmap(cpld_base_addr);
}

MODULE_LICENSE("GPL");
/* Function for module initialization and cleanup */
module_init(cpld_init);
module_exit(cpld_cleanup);
