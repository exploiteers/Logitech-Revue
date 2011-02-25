/*
 * Copyright 2004-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */
#ifndef __ASM_ARCH_MXC_I2C_H__
#define __ASM_ARCH_MXC_I2C_H__

/*!
 * @defgroup MXCI2C Inter-IC (I2C) Driver
 */

/*!
 * @file arch-mxc/mxc_i2c.h
 *
 * @brief This file contains the I2C chip level configuration details.
 *
 * It also contains the API function that other drivers can use to read/write
 * to the I2C device registers.
 *
 * @ingroup MXCI2C
 */

struct mxc_i2c_platform_data {
	u32 i2c_clk;
};

/*!
 * This defines the string used to identify MXC I2C Bus drivers
 */
#define MXC_ADAPTER_NAME        "MXC I2C Adapter"

#define MXC_I2C_FLAG_READ	0x01	/* if set, is read; else is write */
#define MXC_I2C_FLAG_POLLING	0x02	/* if set, is polling mode; else is interrupt mode */

#endif				/* __ASM_ARCH_MXC_I2C_H__ */
