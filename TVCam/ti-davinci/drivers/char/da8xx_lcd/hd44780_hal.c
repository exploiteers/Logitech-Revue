/*
 * Copyright (C) 2008 MontaVista Software Inc.
 * Copyright (C) 2008 Texas Instruments Inc
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option)any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

/*
 * A single HD47780 can be set in the following modes.
 *
 *  8 x 1, 16 x 1, 16 x 2, 16 x 4, 20 x 1, 20 x 2, 20 X 4, 24 x 1, 24 x 2
 *
 * Different display set up affects the way we deal with the data location
 * in the intenal data buffer (i.e. address in the data buffer).
 *
 */

#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/io.h>
#include "hd44780_hal.h"
#include "lidd_cmd.h"

struct hd47780 {
	unsigned int cntl_reg;
	unsigned int data_reg;
	unsigned char disp_row;
	unsigned char disp_col;
	unsigned char disp_cntl;
	unsigned char entry_mode;
};

static int hd47780_p_set_to_xy(struct hd47780 *p, int x, int y);

static inline void lidd_udelay(unsigned int delay, unsigned int addr)
{
	unsigned int i, val;
	for (i = 0; i < delay; i++) {
		val = readl(addr);
		if (!(val & HD47780_BUSY))
			break;
		udelay(1);
	}
}

int hd47780_p_set_to_xy(struct hd47780 *p, int x, int y)
{
	unsigned int cursor_offset = x + (y % 2) * HD47780_ROW_SIZE;
	if (p->disp_row == 1)
		cursor_offset += (x % 8) * (HD47780_ROW_SIZE - 8);
						/* reqd for 1x16 */
	if ((y % 4) >= 2)
		cursor_offset += p->disp_col;

	/* load this offset into the lcd. */
	writel(cursor_offset | 0x80, p->cntl_reg);
	lidd_udelay(50, p->cntl_reg);

	return 0;
}

struct hd47780 *hd47780_init(unsigned int cntl_reg,
			     unsigned int data_reg,
			     unsigned char disp_row, unsigned char disp_col)
{
	struct hd47780 *p_hd = NULL;
	char func_set = 0x20;

	p_hd = kzalloc(sizeof(struct hd47780), GFP_KERNEL);
	if (!p_hd)
		return NULL;
	p_hd->cntl_reg = cntl_reg;
	p_hd->data_reg = data_reg;
	p_hd->disp_row = disp_row;
	p_hd->disp_col = disp_col;

	if (!(p_hd->disp_row % 2))
		func_set |= 0x08;

	lidd_udelay(200, p_hd->cntl_reg);

	if (readl(p_hd->cntl_reg) & HD47780_BUSY) {
		kfree(p_hd);
		return 0;
	}

	writel(func_set | 0x10, p_hd->cntl_reg);
	lidd_udelay(50, p_hd->cntl_reg);

	return p_hd;
}

int hd47780_cleanup(struct hd47780 *p_obj)
{
	kfree(p_obj);
	return 0;
}

int hd47780_ioctl(struct hd47780 *p_obj, unsigned int cmd, unsigned int val)
{
	switch (cmd) {
	case LIDD_CLEAR_SCREEN:
		writel(HD47780_CLR_SCR, p_obj->cntl_reg);
		lidd_udelay(2000, p_obj->cntl_reg);
		break;

	case LIDD_CURSOR_HOME:
		writel(HD47780_DRAM_0, p_obj->cntl_reg);
		lidd_udelay(2000, p_obj->cntl_reg);
		break;

	case LIDD_GOTO_XY:
		hd47780_p_set_to_xy(p_obj, (val & 0xff00) >> 8, (val & 0xff));
		break;

	case LIDD_DISPLAY:
		if (val)
			p_obj->disp_cntl |= HD47780_DISP_ON;
		else
			p_obj->disp_cntl &= ~HD47780_DISP_ON;
		writel(p_obj->disp_cntl | HD47780_SET_DCB, p_obj->cntl_reg);
		lidd_udelay(50, p_obj->cntl_reg);
		break;

	case LIDD_BLINK:	/* Blink ON/OFF */
		if (val)
			p_obj->disp_cntl |= HD47780_BLINK_ON;
		else
			p_obj->disp_cntl &= ~HD47780_BLINK_ON;
		writel(p_obj->disp_cntl | HD47780_SET_DCB, p_obj->cntl_reg);
		lidd_udelay(50, p_obj->cntl_reg);
		break;

	case LIDD_CURSOR_STATE:	/* Cursor State ON/OFF */
		if (val)
			p_obj->disp_cntl |= HD47780_CURSOR_ON;
		else
			p_obj->disp_cntl &= ~HD47780_CURSOR_ON;
		writel(p_obj->disp_cntl | HD47780_SET_DCB, p_obj->cntl_reg);
		lidd_udelay(50, p_obj->cntl_reg);
		break;

	case LIDD_DISPLAY_SHIFT:	/* controls whether display
					   will be shifted when
					   characters are read/written */
		if (val)
			p_obj->entry_mode |= HD47780_SHIFT_ON;
		else
			p_obj->entry_mode &= ~HD47780_SHIFT_ON;
		writel(p_obj->entry_mode | HD47780_SET_IDS, p_obj->cntl_reg);
		lidd_udelay(50, p_obj->cntl_reg);
		break;

	case LIDD_CURSOR_SHIFT:
		if (val)
			p_obj->entry_mode |= HD47780_CSR_INC;
		else
			p_obj->entry_mode &= ~HD47780_CSR_INC;
		writel(p_obj->entry_mode | HD47780_SET_IDS, p_obj->cntl_reg);
		lidd_udelay(50, p_obj->cntl_reg);
		break;

	case LIDD_CURSOR_MOVE:
		if (val)
			val = HD47780_RGHT_DIR;
		writel(val | HD47780_SET_SCRL, p_obj->cntl_reg);
		lidd_udelay(50, p_obj->cntl_reg);
		break;

	case LIDD_DISPLAY_MOVE:	/* moves the display LEFT/RIGHT */
		if (val)
			val = HD47780_RGHT_DIR;
		writel(val | HD47780_DISP_MOV, p_obj->cntl_reg);
		lidd_udelay(50, p_obj->cntl_reg);
		break;

	case LIDD_WR_CHAR:
		writeb((char)val, p_obj->data_reg);
		lidd_udelay(50, p_obj->cntl_reg);
		break;

	case LIDD_RD_CHAR:
		*((unsigned char *)val) = readb(p_obj->data_reg);
		lidd_udelay(50, p_obj->cntl_reg);
		break;

	case LIDD_RD_CMD:
		*((unsigned int *)val) = readl(p_obj->cntl_reg);
		break;

	default:
		break;

	}

	return 0;
}
