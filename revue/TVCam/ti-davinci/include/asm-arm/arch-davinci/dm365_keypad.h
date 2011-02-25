/*
 * include/asm-arm/arch-davinci/dm365_keypad.h
 *
 * Copyright (C) 2008-2009 Texas Instruments, Inc.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __DM365_KEYPAD_H__
#define __DM365_KEYPAD_H__

struct dm365_kp_platform_data {
	int *keymap;
	unsigned int keymapsize;
	unsigned int rep:1;
};

u32 keypad_read(u32 offset);
u32 keypad_write(u32 val, u32 offset);

#define dm365_keypad_write(val, addr)	davinci_writel(val, addr)
#define dm365_keypad_read(addr)		davinci_readl(addr)

/* Keypad registers */

#define KEYPAD_BASE			DM365_KEYSCAN_BASE
#define DM365_KEYPAD_KEYCTRL		(KEYPAD_BASE + 0x0000)
#define DM365_KEYPAD_INTENA		(KEYPAD_BASE + 0x0004)
#define DM365_KEYPAD_INTFLAG		(KEYPAD_BASE + 0x0008)
#define DM365_KEYPAD_INTCLR		(KEYPAD_BASE + 0x000c)
#define DM365_KEYPAD_STRBWIDTH		(KEYPAD_BASE + 0x0010)
#define DM365_KEYPAD_INTERVAL		(KEYPAD_BASE + 0x0014)
#define DM365_KEYPAD_CONTTIME		(KEYPAD_BASE + 0x0018)
#define DM365_KEYPAD_CURRENTST		(KEYPAD_BASE + 0x001c)
#define DM365_KEYPAD_PREVSTATE		(KEYPAD_BASE + 0x0020)
#define DM365_KEYPAD_EMUCTRL		(KEYPAD_BASE + 0x0024)
#define DM365_KEYPAD_IODFTCTRL		(KEYPAD_BASE + 0x002c)

/*Key Control Register (KEYCTRL)*/

#define DM365_KEYPAD_KEYEN		0x00000001
#define DM365_KEYPAD_PREVMODE		0x00000002
#define DM365_KEYPAD_CHATOFF		0x00000004
#define DM365_KEYPAD_AUTODET		0x00000008
#define DM365_KEYPAD_SCANMODE		0x00000010
#define DM365_KEYPAD_OUTTYPE		0x00000020
#define DM365_KEYPAD_4X4		0x00000040

/*Masks for the interrupts*/

#define DM365_KEYPAD_INT_CONT           0x00000008
#define DM365_KEYPAD_INT_OFF           	0x00000004
#define DM365_KEYPAD_INT_ON             0x00000002
#define DM365_KEYPAD_INT_CHANGE         0x00000001
#define DM365_KEYPAD_INT_ALL		0x0000000f

/*Masks for the various keys on the DM365*/

#define	KEY_DM365_KEY2		0
#define	KEY_DM365_LEFT		1
#define	KEY_DM365_EXIT		2
#define	KEY_DM365_DOWN		3
#define	KEY_DM365_ENTER		4
#define	KEY_DM365_UP		5
#define	KEY_DM365_KEY1		6
#define	KEY_DM365_RIGHT		7
#define	KEY_DM365_MENU		8
#define	KEY_DM365_REC		9
#define	KEY_DM365_REW		10
#define	KEY_DM365_SKIPMINUS	11
#define	KEY_DM365_STOP		12
#define	KEY_DM365_FF		13
#define	KEY_DM365_SKIPPLUL	14
#define	KEY_DM365_PLAYPAUSE	15

#endif
