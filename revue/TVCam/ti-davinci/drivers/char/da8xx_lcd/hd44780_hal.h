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

#define _HD47780_HAL_H_

#define HD47780_CLR_SCR    0x1  /* Clears entire display */
#define HD47780_DRAM_0     0x2  /* Sets DRAM address 0 in address counter */

#define HD47780_SET_IDS    0x4  /* Cursor move direction, display Shift setup */
#define HD47780_SHIFT_ON   0x1  /* Sets display shift */
#define HD47780_CSR_INC    0x2  /* Sets cursor move direction to Increment */

#define HD47780_SET_DCB    0x8  /* ON/OFF setup for display, cursor and blink*/
#define HD47780_BLINK_ON   0x1  /* Sets blink ON */
#define HD47780_CURSOR_ON  0x2  /* Sets cursor ON */
#define HD47780_DISP_ON    0x4  /* Sets display ON */

#define HD47780_SET_SCRL  0x10  /* Cursor move and display move setup */
#define HD47780_DISP_MOV  0x18  /* Selects display move mode */
#define HD47780_RGHT_DIR   0x4  /* Selects disp. shift/cursor move to right */

#define HD47780_BUSY      0x80  /* Busy flag - internal operation in progress */

#define HD47780_ROW_SIZE  0x40  /* Bytes per row in DDRAM */
/**********************************************************************
 * Returns: NULL in case of error, otherwise a handle to be used in sub-
 * sequent calls.
 *********************************************************************/
struct hd47780 *hd47780_init(unsigned int cntl_reg,
			     unsigned int data_reg,
			     unsigned char row,
			     unsigned char col);

/**********************************************************************
 * Returns: -1 for error otherwise 0 for success.
 *********************************************************************/
int hd47780_cleanup(struct hd47780 *p);

/**********************************************************************
 * Returns: -1 for error, other 0 for success.
 *
 *     cmd                                  val
 *
 *     LIDD_CLEAR_SCREEN                 none
 *     LIDD_CURSOR_HOME                  none
 *     LIDD_DISPLAY                      0 - off, 1 - on
 *     LIDD_GOTO_XY                      [row - 2 bytes][col - 2 bytes]
 *     LIDD_BLINK                        0 - off, 1 - on
 *     LIDD_CURSOR_STATE                 0 - not visible, 1 - visible.
 *     LIDD_SHIFT                        1 - Right shift, 0 - left shift.
 *     LIDD_CURSOR_SHIFT                 1 - Right, 0 - left
 *     LIDD_WR_CHAR                      character.
 *     LIDD_RD_CHAR                      place holder for character.
 *     LIDD_CURSOR_MOVE                  1 - Right, 0 - Left
 *     LIDD_DISPLAY_MOVE                 1 - Right, 0 - Left.
 *     LIDD_LINE_WRAP                    0 - off, 1 - on.
 *
 *********************************************************************/
int hd47780_ioctl(struct hd47780 *p, unsigned int cmd, unsigned int val);
