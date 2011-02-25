/*
 * include/linux/m48t35.h
 *
 * Definitions for the platform data of m48t35 RTC chip driver.
 *
 * Copyright (c) 2008 MontaVista Software, Inc.
 * Copyright (c) 2007 Wind River Systems, Inc.
 *
 * Author: Randy Vinson <rvinson@mvista.com>
 *
 * Derived from m48t59.c by Mark Zhan <rongkai.zhan@windriver.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _LINUX_RTC_M48T35_H_
#define _LINUX_RTC_M48T35_H_

/*
 * M48T35 Register Offset
 */
#define M48T35_YEAR		0x7fff
#define M48T35_MONTH		0x7ffe
#define M48T35_MDAY		0x7ffd	/* Day of Month */
#define M48T35_WDAY		0x7ffc	/* Day of Week */
#define M48T35_WDAY_CB			0x20	/* Century Bit */
#define M48T35_WDAY_CEB			0x10	/* Century Enable Bit */
#define M48T35_HOUR		0x7ffb
#define M48T35_MIN		0x7ffa
#define M48T35_SEC		0x7ff9
#define M48T35_CNTL		0x7ff8
#define M48T35_CNTL_READ		0x40
#define M48T35_CNTL_WRITE		0x80

#define M48T35_NVRAM_SIZE	0x7ff0

struct m48t35_plat_data {
	/* The method to access M48T35 registers,
	 * NOTE: The 'ofs' should be 0x00~0x1fff
	 */
	void (*write_byte)(struct device *dev, u32 ofs, u8 val);
	unsigned char (*read_byte)(struct device *dev, u32 ofs);
};

#endif /* _LINUX_RTC_M48T35_H_ */
