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

/**************************************************************************
 * FILE PURPOSE :   HAL code for LCD LIDD controller.
 **************************************************************************
 *
 *  FILE DESCRIPTION  :
 *  HAL code for LCD LIDD controller.
 *
 *************************************************************************/

#include <linux/slab.h>
#include "hd44780_hal.h"
#include "lidd_hal.h"

struct ti_lidd_regs {
	unsigned int config;
	unsigned int addr;
	unsigned int data;
};

struct ti_lcd_cntl_regs {
	unsigned int revision;
	unsigned int cntl;
	unsigned int status;
	unsigned int lidd_cntl;
	struct ti_lidd_regs cs[2];
	unsigned int raster_cntl;
	unsigned int raster_timing_0;
	unsigned int raster_timing_1;
	unsigned int raster_timing_2;
	unsigned int subpanel_disp;
	unsigned int reserved;
	unsigned int dma_cntl;
	unsigned int frame0_base_addr;
	unsigned int frame0_ceiling;
	unsigned int frame1_base_addr;
	unsigned int frame1_ceiling;
};

struct lidd_hal_obj {
	unsigned char num_lcd_inst;
	void *lcd_inst[2];
	struct ti_lcd_cntl_regs *regs;
	unsigned int active_inst;
	unsigned int disp_row;
	unsigned int disp_col;
	unsigned int line_wrap;
	unsigned int cursor_state;
	unsigned int cursor_shift;
	unsigned int row;
	unsigned int col;

};

static int ti_lidd_p_set_to_xy(struct lidd_hal_obj *, int, int);
static int ti_lidd_p_update_properties(struct lidd_hal_obj *);
static int ti_lidd_p_lwrap_cursor_move(struct lidd_hal_obj *,
				       unsigned int direction);
static int ti_lidd_p_wr_or_rd(struct lidd_hal_obj *, unsigned int cmd,
			      unsigned int data);

/*-----------------------------------------------------------------------------
 * Updates the properties of the LCD.
 *
 * Returns 0 on success or -1 on failure.
 *---------------------------------------------------------------------------*/
int ti_lidd_p_update_properties(struct lidd_hal_obj *p)
{
	unsigned int passive_inst = !p->active_inst;

	if (p->num_lcd_inst > 1)
		hd47780_ioctl(p->lcd_inst[passive_inst],
			      LIDD_CURSOR_STATE, p->cursor_state);

	hd47780_ioctl(p->lcd_inst[p->active_inst], LIDD_CURSOR_STATE,
		      p->cursor_state);

	return 0;
}

/*---------------------------------------------------------
 * Sets the cursor at the specified location.
 * x is the column, y is the row.
 *
 * Returns 0 on success otherwise -1.
 *-------------------------------------------------------*/
int ti_lidd_p_set_to_xy(struct lidd_hal_obj *p, int x, int y)
{
	int update = 0;

	if (p->line_wrap) {
		/* wrap the row and col if required. */
		p->col = x = x % p->disp_col;
		p->row = y = y % p->disp_row;
	}

	/* Figure out if this causes a moving to lcd 0 to lcd 1. */
	if ((p->num_lcd_inst == 2) && (y > p->disp_row / 2 - 1)) {
		y = y % 2;

		if (p->active_inst == 0)
			update = 1;

		p->active_inst = 1;
	} else {
		/* Figure out if this causes a moving to lcd 1 to lcd 0. */
		if (p->active_inst == 1)
			update = 1;

		p->active_inst = 0;
	}

	if (update)
		ti_lidd_p_update_properties(p);

	hd47780_ioctl(p->lcd_inst[p->active_inst], LIDD_GOTO_XY, x << 8 | y);

	return 0;
}

/*-----------------------------------------------------------------------------
 * Wraps a line i.e. when the last of column of the row is reached, the cursor
 * is moved to the next row, first col. When the last row and last col is
 *  reached the cursor is moved to the first row.
 *
 * Returns: 0 if now wrap was done. 1, if a wrap was carried out.
 *---------------------------------------------------------------------------*/
int ti_lidd_p_lwrap_cursor_move(struct lidd_hal_obj *p, unsigned int direction)
{
	int ret_val = 0;

	if (direction) {	/* move right. */
		p->col = (++p->col) % (p->disp_col);
		if (!p->col) {
			/* line wrapped. */
			p->row = (++p->row) % p->disp_row;

			/* Indicate the lcd of the line wrapping. */
			ti_lidd_p_set_to_xy(p, p->col, p->row);

			ret_val = 1;
		}
	} else {		/* move left. */

		if (!p->col) {
			/* line wrapped. */
			p->col = p->disp_col - 1;
			p->row = (!p->row) ? p->disp_row - 1 : --p->row;

			/* Indicate the lcd of the line wrapping. */
			ti_lidd_p_set_to_xy(p, p->col, p->row);

			ret_val = 1;
		} else {
			p->col--;
		}
	}
	return ret_val;
}

/*-----------------------------------------------------------------------------
 * Writes or reads a character to/from LCD.
 *
 * Returns 0 on success or -1 on failure.
 *---------------------------------------------------------------------------*/
int ti_lidd_p_wr_or_rd(struct lidd_hal_obj *p, unsigned int cmd,
		       unsigned int data)
{

	/* Write the character on the active hd47780 instance. */
	hd47780_ioctl(p->lcd_inst[p->active_inst], cmd, data);

	/* Update the rows and columns for the next read or write,
	   only if enabled for line wrap. */

	if (p->line_wrap)
		ti_lidd_p_lwrap_cursor_move(p, p->cursor_shift);

	return 0;
}

struct lidd_hal_obj *ti_lidd_hal_init(struct ti_lidd_info *p_lidd_info)
{
	static int g_lcd_inst;
	struct lidd_hal_obj *p = NULL;
	struct ti_lcd_cntl_regs *p_regs = NULL;

	int i;

	/* Initialize the controller, if not done already. */

	p = kzalloc(sizeof(struct lidd_hal_obj), GFP_KERNEL);
	if (!p)
		return NULL;

	p->regs = (struct ti_lcd_cntl_regs *)p_lidd_info->base_addr;

	if (g_lcd_inst == 2) {
		kfree(p);
		return NULL;
	}
	if (g_lcd_inst == 0) {
		p_regs = p->regs;
		p_regs->cntl &= ~1;	/* Sets it in the HD47780 mode. */
		p_regs->cntl &= ~(LCD_CLK_DIVISOR_MASK);
		p_regs->cntl |= LCD_CLK_DIVISOR(0x7f);	/* modify the code here
					for instance where mclk is required. */
		p_regs->cs[0].config |= LCD_RS_SETUP(LCD_LIDD_P0_RS_SETUP_CYC)
		    | LCD_RS_WIDTH(LCD_LIDD_P0_RS_DUR_CYC)
		    | LCD_RS_HOLD(LCD_LIDD_P0_RS_HOLD_CYC);
		p_regs->cs[1].config |= LCD_RS_SETUP(LCD_LIDD_P1_RS_SETUP_CYC)
		    | LCD_RS_WIDTH(LCD_LIDD_P1_RS_DUR_CYC)
		    | LCD_RS_HOLD(LCD_LIDD_P1_RS_HOLD_CYC);
		p_regs->cs[0].config |= LCD_WS_SETUP(LCD_LIDD_P0_WS_SETUP_CYC)
		    | LCD_WS_WIDTH(LCD_LIDD_P0_WS_DUR_CYC)
		    | LCD_WS_HOLD(LCD_LIDD_P0_WS_HOLD_CYC);
		p_regs->cs[1].config |= LCD_WS_SETUP(LCD_LIDD_P1_WS_SETUP_CYC)
		    | LCD_WS_WIDTH(LCD_LIDD_P1_WS_DUR_CYC)
		    | LCD_WS_HOLD(LCD_LIDD_P1_WS_HOLD_CYC);
	}

	p->active_inst = 0;
	p->num_lcd_inst = p_lidd_info->num_lcd;
	p->disp_row = p_lidd_info->disp_row;
	p->disp_col = p_lidd_info->disp_col;
	p->cursor_state = p_lidd_info->cursor_show;
	p->line_wrap = p_lidd_info->line_wrap;
	p->row = 0;
	p->col = 0;
	p->cursor_shift = RIGHT;

	if (p_lidd_info->lcd_type == 4) {
		p_regs->lidd_cntl &= 0xfffffff8;
		p_regs->lidd_cntl |= 0x4;

		for (i = 0; i < p->num_lcd_inst; i++) {
			p->lcd_inst[i] =
				hd47780_init((unsigned int)(&p_regs->cs[i +
							g_lcd_inst].addr),
					     (unsigned int)(&p_regs->cs[i +
							g_lcd_inst].data),
					  (p->disp_row / p->num_lcd_inst),
					  (p->disp_col / p->num_lcd_inst));

			if (!p->lcd_inst[i])
				goto hd47780_init_error;

			hd47780_ioctl(p->lcd_inst[i], LIDD_CURSOR_SHIFT,
				      p->cursor_shift);
			hd47780_ioctl(p->lcd_inst[i], LIDD_DISPLAY, ON);
			hd47780_ioctl(p->lcd_inst[i], LIDD_CURSOR_STATE,
				      (p->cursor_state && i == 0) ? ON : OFF);
			hd47780_ioctl(p->lcd_inst[i], LIDD_DISPLAY_MOVE, OFF);
			hd47780_ioctl(p->lcd_inst[i], LIDD_BLINK,
				      (p_lidd_info->cursor_blink
				       && i == 0) ? ON : OFF);
			hd47780_ioctl(p->lcd_inst[i], LIDD_CLEAR_SCREEN, 0);

		}
	}

	g_lcd_inst += p->num_lcd_inst;

	return p;

hd47780_init_error:
	while (i > 0) {
		hd47780_cleanup(p->lcd_inst[--i]);
		p->num_lcd_inst--;
	}
	kfree(p);
	return NULL;
}

int ti_lidd_hal_ioctl(struct lidd_hal_obj *p, unsigned int cmd,
		      unsigned int val)
{
	switch (cmd) {
	case LIDD_CLEAR_SCREEN:
		hd47780_ioctl(p->lcd_inst[0], cmd, val);
		if (p->num_lcd_inst > 1)
			hd47780_ioctl(p->lcd_inst[1], cmd, val);
		p->row = 0;
		p->col = 0;
		break;

	case LIDD_LINE_WRAP:
		p->line_wrap = val ? 1 : 0;
		if (p->line_wrap) {
			p->active_inst = 0;
			ti_lidd_p_update_properties(p);

			/* disable display shift */
			if (p->num_lcd_inst > 1)
				hd47780_ioctl(p->lcd_inst[1],
					      LIDD_DISPLAY_MOVE, val);
			hd47780_ioctl(p->lcd_inst[0], LIDD_DISPLAY_MOVE, val);
		}
		break;

	case LIDD_CURSOR_HOME:
		if (p->num_lcd_inst > 1) {
			hd47780_ioctl(p->lcd_inst[1], cmd, 0);
			hd47780_ioctl(p->lcd_inst[1], LIDD_CURSOR_STATE, 0);
		}
		hd47780_ioctl(p->lcd_inst[0], cmd, 0);
		p->row = 0;
		p->col = 0;
		break;

	case LIDD_DISPLAY:
		if (p->num_lcd_inst > 1)
			hd47780_ioctl(p->lcd_inst[1], cmd, val);
		hd47780_ioctl(p->lcd_inst[0], cmd, val);
		break;

	case LIDD_GOTO_XY:
		ti_lidd_p_set_to_xy(p, (val & 0xff00) >> 8, val & 0xff);
		break;

	case LIDD_BLINK:
		if (p->num_lcd_inst > 1)
			hd47780_ioctl(p->lcd_inst[1], cmd, val);
		hd47780_ioctl(p->lcd_inst[0], cmd, val);
		break;

	case LIDD_CURSOR_STATE:
		hd47780_ioctl(p->lcd_inst[p->active_inst], cmd, val);
		p->cursor_state = val ? 1 : 0;
		break;

	case LIDD_CURSOR_SHIFT:
		if (p->num_lcd_inst > 1)
			hd47780_ioctl(p->lcd_inst[1], cmd, val);
		hd47780_ioctl(p->lcd_inst[0], cmd, val);
		p->cursor_shift = (val) ? 1 : 0;
		break;

	case LIDD_CURSOR_MOVE:
		{
			if (!p->line_wrap
			    && !ti_lidd_p_lwrap_cursor_move(p, val))
				hd47780_ioctl(p->lcd_inst[p->active_inst],
					      cmd, val);
		}
		break;

	case LIDD_DISPLAY_SHIFT:
		if (p->num_lcd_inst > 1)
			hd47780_ioctl(p->lcd_inst[1], cmd, val);
		hd47780_ioctl(p->lcd_inst[0], cmd, val);
		p->line_wrap = 0;	/* p->row = 0; p->col = 0; */
		break;

	case LIDD_DISPLAY_MOVE:
		hd47780_ioctl(p->lcd_inst[p->active_inst], cmd, val);
		break;

	case LIDD_WR_CHAR:
	case LIDD_RD_CHAR:
		ti_lidd_p_wr_or_rd(p, cmd, val);
		break;

	case LIDD_RD_CMD:
		hd47780_ioctl(p->lcd_inst[p->active_inst], cmd, val);
		break;

	default:
		break;

	}

	return 0;
}

int ti_lidd_hal_open(struct lidd_hal_obj *p)
{
	return 0;
}

int ti_lidd_hal_close(struct lidd_hal_obj *p)
{
	return 0;
}

int ti_lidd_hal_write(struct lidd_hal_obj *p, char *data, unsigned int len)
{
	int i = 0;
	for (i = 0; i < len; i++) {
		/* Ignore NULLs and NLs */
		if (!data[i] || data[i] == 0xa)
			continue;
		if (ti_lidd_p_wr_or_rd(p, LIDD_WR_CHAR, data[i]))
			break;
	}
	return i;
}

int ti_lidd_hal_read(struct lidd_hal_obj *p, char *data, unsigned int len)
{
	int i = 0;
	for (i = 0; i < len; i++) {
		if (ti_lidd_p_wr_or_rd
		    (p, LIDD_RD_CHAR, (unsigned int)&(data[i])))
			break;
	}
	return i;
}

int ti_lidd_hal_cleanup(struct lidd_hal_obj *p)
{
	while (p->num_lcd_inst--)
		hd47780_cleanup(p->lcd_inst[p->num_lcd_inst]);

	kfree(p);

	return 0;
}
