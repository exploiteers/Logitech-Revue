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
/**************************************************************************
 *
 *  FILE DESCRIPTION  :
 *  IOCTL commands for LCD LIDD controller.
 *
 *************************************************************************/

#ifndef _LIDD_CMD_H_
#define _LIDD_CMD_H_

#define LIDD_CLEAR_SCREEN   1
#define LIDD_CURSOR_HOME    2
#define LIDD_GOTO_XY        3
#define LIDD_DISPLAY        4
#define LIDD_BLINK          5
#define LIDD_CURSOR_STATE   6
#define LIDD_DISPLAY_SHIFT  7
#define LIDD_CURSOR_SHIFT   8
#define LIDD_CURSOR_MOVE    9
#define LIDD_DISPLAY_MOVE   10
#define LIDD_WR_CHAR        11
#define LIDD_RD_CHAR        12
#define LIDD_LINE_WRAP      13
#define LIDD_RD_CMD         14

#define RIGHT                1
#define LEFT                 0
#define ON                   1
#define OFF                  0

#endif				/* _LIDD_CMD_H_ */
