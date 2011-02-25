/*
 *
 * Copyright (C) 2004 Texas Instruments.
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
 */
#ifndef _TYPESH_
#define _TYPESH_

typedef unsigned long 	ULONG;
typedef unsigned short 	USHORT;
typedef unsigned long   BOOL;
typedef unsigned int	WORD;
typedef char            CHAR;
typedef unsigned char   BYTE, *LPBYTE, UCHAR, *PUCHAR, PBYTE;

#define FALSE           0
#define TRUE            1

#define NULL			0

typedef unsigned short int Hwd;
typedef volatile unsigned short int vHwd;
typedef unsigned short int *  Hwdptr;
typedef volatile unsigned short int * vHwdptr;
//typedef volatile unsigned int * vHwdptr;


#endif


