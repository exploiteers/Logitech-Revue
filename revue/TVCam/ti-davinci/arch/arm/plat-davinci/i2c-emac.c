/*
 * arch/arm/plat-davinci/i2c-emac.c
 *
 * Read MAC address from I2C-attached EEPROM
 *
 * Author: Texas Instruments
 * Copyright (C) 2006 Texas Instruments, Inc.
 * Copyright (C) 2008 MontaVista Software, Inc. <source@mvista.com>
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/errno.h>

#include <asm/arch/i2c-client.h>

/* Get Ethernet address from kernel boot params */
static unsigned char cpmac_eth_string[20] = "deadbeaf";

/*
 * This function gets the Ethernet MAC address from EEPROM
 * input buffer to be of at least 20 bytes in length.
 */
int davinci_get_macaddr(char *ptr)
{
#ifndef CONFIG_I2C_DAVINCI
	printk(KERN_INFO "DaVinci EMAC: unable to read MAC address from "
	       "EEPROM, no I2C support in kernel.\n");
#else
/**
	Appropho modify 2009/03/26 in order to get EMAC from bootparams.
 */
//#ifndef CONFIG_MACH_DAVINCI_DM365_IPNC
#if 0 // force it to read MAC from bootparams.
	char data[2] = { 0x7f, 0 };
	char temp[20];
	int  ret, i = 0;

	if (ptr == NULL)
		return -EFAULT;

	ret = davinci_i2c_write(2, data, 0x50);
	if (!ret)
		davinci_i2c_write(2, data, 0x50);
	davinci_i2c_read(8, temp, 0x50);

	/*
	 * Check whether MAC address is available in ERPROM; else try to
	 * to get it from bootparams for now.  From Delta EVM MAC address
	 * should be available from I2C EEPROM.
	 */
	if (temp[0] != 0xFF || temp[1] != 0xFF || temp[2] != 0xFF ||
	    temp[3] != 0xFF || temp[4] != 0xFF || temp[5] != 0xFF ||
	    temp[6] != 0xFF) {
		ptr[0] = (temp[0] & 0xF0) >> 4;
		ptr[1] = (temp[0] & 0x0F);
		ptr[2] = ':';
		ptr[3] = (temp[1] & 0xF0) >> 4;
		ptr[4] = (temp[1] & 0x0F);
		ptr[5] = ':';
		ptr[6] = (temp[2] & 0xF0) >> 4;
		ptr[7] = (temp[2] & 0x0F);
		ptr[8] = ':';
		ptr[9] = (temp[3] & 0xF0) >> 4;
		ptr[10] = (temp[3] & 0x0F);
		ptr[11] = ':';
		ptr[12] = (temp[4] & 0xF0) >> 4;
		ptr[13] = (temp[4] & 0x0F);
		ptr[14] = ':';
		ptr[15] = (temp[5] & 0xF0) >> 4;
		ptr[16] = (temp[5] & 0x0F);

		for (i = 0; i < 17; i++) {
			if (ptr[i] == ':')
				continue;
			else if (ptr[i] <= 9)
				ptr[i] = ptr[i] + '0';
			else
				ptr[i] = ptr[i] + 'a' - 10;
		}
	} else
#endif
#endif
	{
		strcpy(ptr, cpmac_eth_string);
	}
	return 0;
}
EXPORT_SYMBOL(davinci_get_macaddr);

static int davinci_cpmac_eth_setup(char *str)
{
	/* The first char passed from the bootloader is '=', so ignore it */
	strcpy(cpmac_eth_string, str + 1);

	printk("TI DaVinci EMAC: kernel boot params Ethernet address: %s\n",
		cpmac_eth_string);

	return 1;
}
#ifdef MODULE
static char *cmdline = NULL;
module_param(cmdline, charp, 0);
MODULE_LICENSE("GPL");

static void __init parse_options (char *line)
{
	char *next = line;

	if (line == NULL || !*line)
		return;
 	if ((next = strstr(line,"eth")) != NULL)
	{
		//next = strndup(next+3,18);
		memcpy(next,next+3,18);
		next[18] = '\0';
		if (!davinci_cpmac_eth_setup(next))
		printk (KERN_INFO "Unknown option '%s'\n", line);
	}
}

int __init init_module (void)
{
	parse_options(cmdline);
	return 0;
}

void cleanup_module (void)
{
}
#else
__setup("eth", davinci_cpmac_eth_setup);
#endif
