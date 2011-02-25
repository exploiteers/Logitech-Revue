/*
 * Header file for DaVinci MMC platform data.
 *
 * (c) 2007-2008 MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#ifndef DAVINCI_MMC_PLAT_H
#define DAVINCI_MMC_PLAT_H

struct davinci_mmc_platform_data {
	const char *mmc_clk;	/* MMC clock name for clk_get() */
	unsigned int rw_threshold;
	unsigned char use_4bit_mode;
	unsigned char use_8bit_mode;
	unsigned int max_frq;
	unsigned char pio_set_dmatrig;
	int (*get_ro) (int);
};

#endif	/* ifndef DAVINCI_MMC_PLAT_H */
