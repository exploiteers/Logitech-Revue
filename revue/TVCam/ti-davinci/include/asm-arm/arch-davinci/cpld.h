/*
 * Copyright (C) 2008-2009 Texas Instruments Inc
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

#ifndef _CPLD_H
#define _CPLD_H

#define DM365_CPLD_BASE_ADDR	(0x04000000)
#define DM365_CPLD_ADDR_SIZE	(0x10000)

#define DM365_CPLD_REGISTER3	(0x18)
#define DM365_CPLD_REGISTER4	(0x1C)
#define DM365_CPLD_REGISTER5	(0x0408)

/* For routing GPIO40 to Sensor Reset line */
#define DM365_CPLD_REGISTER16	(0x1000)
#define DM365_CPLD_REGISTER18	(0x1010)

/* Control TVP5146 reset line */
#define DM365_CPLD_REGISTER19	(0x1018)
#define DM365_TVP7002_SEL	(0x01010101)
#define DM365_SENSOR_SEL	(0x02020202)
#define DM365_TVP5146_SEL	(0x05050505)
#define DM365_VIDEO_MUX_MASK	(0x07070707)
#define DM365_IMAGER_RST_MASK	(0x40404040)
#define DM365_TVP5146_RST_MASK	(0x01010101)

#define DM365_CCD_POWER_MASK (0x08080808)

u32 cpld_read(u32 offset);
u32 cpld_write(u32 val, u32 offset);

#endif
