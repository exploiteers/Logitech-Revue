/*
 * NAND driver for TI DaVinci based boards.
 *
 * Copyright (C) 2007 Sergey Kubushyn <ksi@koi8.net>
 *
 * Based on Linux DaVinci NAND driver by TI. Original copyright follows:
 */

/*
 *
 * linux/drivers/mtd/nand/nand_davinci.c
 *
 * NAND Flash Driver
 *
 * Copyright (C) 2006 Texas Instruments.
 *
 * ----------------------------------------------------------------------------
 *
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
 *
 *  Overview:
 *   This is a device driver for the NAND flash device found on the
 *   DaVinci board which utilizes the Samsung k9k2g08 part.
 *
 Modifications:
 ver. 1.0: Feb 2005, Vinod/Sudhakar
 -
 *
 */

#include <common.h>

#if (CONFIG_COMMANDS & CFG_CMD_NAND)
#if !defined(CFG_NAND_LEGACY)

#include "soc.h"
#include <nand.h>
#include <asm/arch/nand_defs.h>
#include <asm/arch/emif_defs.h>

extern struct nand_chip nand_dev_desc[CFG_MAX_NAND_DEVICE];

static void nand_davinci_hwcontrol(struct mtd_info *mtd, int cmd)
{
	struct		nand_chip *this = mtd->priv;
	u_int32_t	IO_ADDR_W = (u_int32_t)this->IO_ADDR_W;

	IO_ADDR_W &= ~(MASK_ALE|MASK_CLE);

	switch (cmd) {
		case NAND_CTL_SETCLE:
			IO_ADDR_W |= MASK_CLE;
			break;
		case NAND_CTL_SETALE:
			IO_ADDR_W |= MASK_ALE;
			break;
	}

	this->IO_ADDR_W = (void *)IO_ADDR_W;
}

static int nand_davinci_dev_ready(struct mtd_info *mtd)
{
	emifregs	emif_addr;

	emif_addr = (emifregs)CSL_EMIF_1_REGS;

	return(emif_addr->NANDFSR & 0x1);
}

static int nand_davinci_waitfunc(struct mtd_info *mtd, struct nand_chip *this, int state)
{
	volatile unsigned int status;
	while(!nand_davinci_dev_ready(mtd)) {;}
	do {
	*NAND_CE0CLE = NAND_STATUS;
	status = *NAND_CE0DATA;
	}
	while(!(status & 0x40));
	 
	return(status);
}

int board_nand_init(struct nand_chip *nand)
{
	nand->IO_ADDR_R   = (void  __iomem *)NAND_CE0DATA;
	nand->IO_ADDR_W   = (void  __iomem *)NAND_CE0DATA;
	nand->chip_delay  = 0;
	nand->options     = 0;
	nand->eccmode     = NAND_ECC_SOFT;

	/* Set address of hardware control function */
	nand->hwcontrol = nand_davinci_hwcontrol;

	nand->dev_ready = nand_davinci_dev_ready;
	nand->waitfunc = nand_davinci_waitfunc;

	return 0;
}

#else
#error "U-Boot legacy NAND support not available for DaVinci chips"
#endif
#endif	/* CFG_USE_NAND */

