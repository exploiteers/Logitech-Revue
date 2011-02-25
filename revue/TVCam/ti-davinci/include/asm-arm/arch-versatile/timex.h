/*
 *  linux/include/asm-arm/arch-versatile/timex.h
 *
 *  Versatile architecture timex specifications
 *
 *  Copyright (C) 2003 ARM Limited
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

/*
 * get_cycles is connected to a 1Mhz clock.
 */
#define CLOCK_TICK_RATE		(1000000)

extern u64 versatile_get_cycles(void);
#define mach_read_cycles() versatile_get_cycles()
