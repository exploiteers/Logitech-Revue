/*
 * Copyright (C) 2008 Texas Instruments Inc
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
/* dm6446_imp.c file. A dummy Image processor module for dm6446 */
#include <asm/arch/imp_hw_if.h>

/* This is a place holder for implementing imp interface for
   DM6446. Currently this is not implemented and returns NULL
*/
struct imp_hw_interface *imp_get_hw_if(void)
{
	return NULL;
}
EXPORT_SYMBOL(imp_get_hw_if);
