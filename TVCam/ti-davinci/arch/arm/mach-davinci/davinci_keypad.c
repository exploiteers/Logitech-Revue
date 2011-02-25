/*
 * Copyright (C) 2008 - 2009 Texas Instruments	Inc
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
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
#include <asm/arch/dm365_keypad.h>

u32 keypad_read(u32 offset)
{
	if (cpu_is_davinci_dm365())
		return cpld_read(offset);
	else
		pr_info("NO keypad support\n");

	return -1;
}
EXPORT_SYMBOL(keypad_read);

u32 keypad_write(u32 val, u32 offset)
{
	if (cpu_is_davinci_dm365())
		return cpld_write(val, offset);
	else
		pr_info("NO keypad support\n");

	return -1;
}
EXPORT_SYMBOL(keypad_write);

int keypad_init(void)
{
	return 0;
}

void keypad_cleanup(void)
{
}

MODULE_LICENSE("GPL");
/* Function for module initialization and cleanup */
module_init(keypad_init);
module_exit(keypad_cleanup);
