/*
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

#ifndef _LCD_H_
#define _LCD_H_
#define DEFAULT_ROWS 2		/*HANTRONIX HDM24216-2*/
#define DEFAULT_COLS 24		/*HANTRONIX HDM24216-2*/
#define MAX_ROWS 4
#define MAX_COLS 48

struct lcd_pos_arg {
	u32 row;
	u32 column;
};

#endif
