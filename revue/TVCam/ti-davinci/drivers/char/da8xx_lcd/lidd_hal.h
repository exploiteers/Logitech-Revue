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
 * FILE PURPOSE :   HAL header for LCD LIDD controller.
 **************************************************************************
 * FILE NAME    :   lidd_hal.h
 *
 * DESCRIPTION  :
 *  HAL code for LCD LIDD controller.
 *
 *************************************************************************/
#ifndef _LIDD_HAL_H_
#define _LIDD_HAL_H_

#include "lidd_cmd.h"

/* LCD controller configuration */

#define LCD_LIDD_P0_RS_SETUP_CYC     1
#define LCD_LIDD_P0_RS_HOLD_CYC      1
#define LCD_LIDD_P0_RS_DUR_CYC       4

#define LCD_LIDD_P1_RS_SETUP_CYC     1
#define LCD_LIDD_P1_RS_HOLD_CYC      1
#define LCD_LIDD_P1_RS_DUR_CYC       4

#define LCD_LIDD_P0_WS_SETUP_CYC     1
#define LCD_LIDD_P0_WS_HOLD_CYC      1
#define LCD_LIDD_P0_WS_DUR_CYC       4

#define LCD_LIDD_P1_WS_SETUP_CYC     1
#define LCD_LIDD_P1_WS_HOLD_CYC      1
#define LCD_LIDD_P1_WS_DUR_CYC       4

#define LCD_CLK_DIVISOR(x)          ((x) << 8)
#define LCD_CLK_DIVISOR_MASK        LCD_CLK_DIVISOR(0xFF)
#define LCD_MODE_SEL_MASK           0x01

#define LCD_CS1_E1_POL_INV          0x80
#define LCD_CS0_E0_POL_INV          0x40
#define LCD_WE_RW_POL_INV           0x20
#define LCD_OE_E_POL_INV            0x10
#define LCD_ALE_POL_INV             0x08
#define LCD_LIDD_MODE(x)            ((x) << 0)

#define LCD_HITACHI_MODE            0x04
#define LCD_MPU80_ASYNC_MODE        0x03
#define LCD_MPU80_SYNC_MODE         0x02
#define LCD_MPU68_ASYNC_MODE        0x01
#define LCD_MPU68_SYNC_MODE         0x00

#define LCD_LIDD_MODE_MASK          LCD_LIDD_MODE(0x07)

#define LCD_WS_SETUP(x)             ((x) << 27)
#define LCD_WS_WIDTH(x)             ((x) << 21)
#define LCD_WS_HOLD(x)              ((x) << 17)
#define LCD_RS_SETUP(x)             ((x) << 12)
#define LCD_RS_WIDTH(x)             ((x) << 6)
#define LCD_RS_HOLD(x)              ((x) << 2)
#define LCD_LIDD_DELAY(x)           ((x) << 0)

#define LCD_WS_SETUP_MASK           LCD_WS_SETUP(0x1F)
#define LCD_WS_WIDTH_MASK           LCD_WS_WIDTH(0x3F)
#define LCD_WS_HOLD_MASK            LCD_WS_HOLD(0x0F)
#define LCD_RS_SETUP_MASK           LCD_RS_SETUP(0x1F)
#define LCD_RS_WIDTH_MASK           LCD_RS_WIDTH(0x3F)
#define LCD_RS_HOLD_MASK            LCD_RS_HOLD(0x0F)

#define LCD_LIDD_DELAY_MASK         LCD_LIDD_DELAY(0x03)

struct ti_lidd_info {
	unsigned int base_addr;
	unsigned int disp_row;	/* total number of row. */
	unsigned int disp_col;	/* total number of col. */
	unsigned int line_wrap;	/* whether to wrap the line. */
	unsigned int cursor_blink;
	unsigned int cursor_show;
	unsigned int lcd_type;	/* 0 = Sync MPU68,
				   1 = Async MPU68,
				   2 = Sync MPU80,
				   3 = Aync MPU80,
				   4 = Hitachi (Async) */
	unsigned int num_lcd;	/* num of hd44780 or equivalnet lcd.
				   The valid values are 1 or 2. */
};

/**********************************************************************
 * Returns: NULL in case of error, otherwise a handle to be used in sub-
 * sequent calls.
 *********************************************************************/
struct lidd_hal_obj *ti_lidd_hal_init(struct ti_lidd_info *);

/**********************************************************************
 * Returns: -1 for error otherwise 0 for success.
 *********************************************************************/
int ti_lidd_hal_open(struct lidd_hal_obj *);
int ti_lidd_hal_close(struct lidd_hal_obj *);
int ti_lidd_hal_cleanup(struct lidd_hal_obj *);

/***********************************************************************
 * Returns: -1 for error, otherwise the number of bytes that were
 *          actually written.
 *
 *          The write begins at the current address location.
 *
 * Note: Here, the character array is not assumed to be NULL terminted.
 *
 **********************************************************************/
int ti_lidd_hal_write(struct lidd_hal_obj *, char *, unsigned int size);

/***********************************************************************
 * Returns: -1 for error, otherwise the number of bytes that were
 *          read.
 *
 *          The read begins at the current address location.
 *
 * Note: Here, the character array is not assumed to be NULL terminted.
 *
 **********************************************************************/
int ti_lidd_hal_read(struct lidd_hal_obj *, char *, unsigned int size);

/**********************************************************************
 * Returns: -1 for error, other 0 for success.
 *
 *     cmd                                  val
 *
 *     TI_LIDD_CLEAR_SCREEN                 none
 *     TI_LIDD_CURSOR_HOME                  none
 *     TI_LIDD_DISPLAY                      0 - off, 1 - on
 *     TI_LIDD_GOTO_XY                      [row - 2 bytes][col - 2 bytes]
 *     TI_LIDD_BLINK                        0 - off, 1 - on
 *     TI_LIDD_CURSOR_STATE                 0 - not visible, 1 - visible.
 *     TI_LIDD_SHIFT                        1 - Right shift, 0 - left shift.
 *     TI_LIDD_CURSOR_SHIFT                 1 - Right, 0 - left
 *     TI_LIDD_WR_CHAR                      character.
 *     TI_LIDD_RD_CHAR                      place holder for character.
 *     TI_LIDD_CURSOR_MOVE                  1 - Right, 0 - Left
 *     TI_LIDD_DISPLAY_MOVE                 1 - Right, 0 - Left.
 *     TI_LIDD_LINE_WRAP                    0 - off, 1 - on.
 *
 *********************************************************************/
int ti_lidd_hal_ioctl(struct lidd_hal_obj *, unsigned int cmd,
		      unsigned int val);

#endif				/* _LIDD_HAL_H_ */
