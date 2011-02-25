/*
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __DAVINCI_SPI_H_
#define __DAVINCI_SPI_H_

#include <linux/clk.h>
#include <asm/arch-davinci/edma.h>

#define DAVINCI_SPI_INTERN_CS		0xFF

enum {
	DAVINCI_SPI_VERSION_1, /* Original on most Davinci's */
	DAVINCI_SPI_VERSION_2, /* New one on DA8xx */
};

struct davinci_spi_platform_data {
	u8	version;
	u16	num_chipselect;
	u8	*chip_sel;
	char	*clk_name;
	struct clk *clk_info;
};

#endif				/* __DAVINCI_SPI_H_ */
