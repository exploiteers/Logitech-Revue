/*
 * (C) Copyright 2007-2008
 * Stelian Pop <stelian.pop@leadtechdesign.com>
 * Lead Tech Design <www.leadtechdesign.com>
 *
 * (C) Copyright 2006 ATMEL Rousset, Lacressonniere Nicolas
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <asm/arch/at91sam9261.h>
#include <asm/arch/gpio.h>
#include <asm/arch/at91_pio.h>

#include <nand.h>

/*
 *	hardware specific access to control-lines
 */
#define	MASK_ALE	(1 << 22)	/* our ALE is AD22 */
#define	MASK_CLE	(1 << 21)	/* our CLE is AD21 */

static void at91sam9261ek_nand_hwcontrol(struct mtd_info *mtd,
					 int cmd, unsigned int ctrl)
{
	struct nand_chip *this = mtd->priv;

	if (ctrl & NAND_CTRL_CHANGE) {
		ulong IO_ADDR_W = (ulong) this->IO_ADDR_W;
		IO_ADDR_W &= ~(MASK_ALE | MASK_CLE);

		if (ctrl & NAND_CLE)
			IO_ADDR_W |= MASK_CLE;
		if (ctrl & NAND_ALE)
			IO_ADDR_W |= MASK_ALE;

		at91_set_gpio_value(AT91_PIN_PC14, !(ctrl & NAND_NCE));
		this->IO_ADDR_W = (void *) IO_ADDR_W;
	}

	if (cmd != NAND_CMD_NONE)
		writeb(cmd, this->IO_ADDR_W);
}

static int at91sam9261ek_nand_ready(struct mtd_info *mtd)
{
	return at91_get_gpio_value(AT91_PIN_PC15);
}

int board_nand_init(struct nand_chip *nand)
{
	nand->ecc.mode = NAND_ECC_SOFT;
#ifdef CFG_NAND_DBW_16
	nand->options = NAND_BUSWIDTH_16;
#endif
	nand->cmd_ctrl = at91sam9261ek_nand_hwcontrol;
	nand->dev_ready = at91sam9261ek_nand_ready;
	nand->chip_delay = 20;

	return 0;
}
