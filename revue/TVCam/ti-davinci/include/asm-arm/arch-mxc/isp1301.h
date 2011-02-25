
/*
 * Copyright 2005-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */
#ifndef __ASM_ARCH_MXC_ISP1301_H__
#define __ASM_ARCH_MXC_ISP1301_H__

/* ISP1301 register addresses,all register of ISP1301
 * is one-byte length register
 */

/* ISP1301: I2C device address */
#define ISP1301_DEV_ADDR 		0x2D

/* ISP 1301 register set*/
#define ISP1301_MODE_REG1_SET		0x04
#define ISP1301_MODE_REG1_CLR		0x05

#define ISP1301_CTRL_REG1_SET		0x06
#define ISP1301_CTRL_REG1_CLR		0x07

#define ISP1301_INT_SRC_REG		0x08
#define ISP1301_INT_LAT_REG_SET		0x0a
#define ISP1301_INT_LAT_REG_CLR		0x0b
#define ISP1301_INT_FALSE_REG_SET	0x0c
#define ISP1301_INT_FALSE_REG_CLR	0x0d
#define ISP1301_INT_TRUE_REG_SET	0x0e
#define ISP1301_INT_TRUE_REG_CLR	0x0f

#define ISP1301_CTRL_REG2_SET		0x10
#define ISP1301_CTRL_REG2_CLR		0x11

#define ISP1301_MODE_REG2_SET		0x12
#define ISP1301_MODE_REG2_CLR		0x13

#define ISP1301_BCD_DEV_REG0		0x14
#define ISP1301_BCD_DEV_REG1		0x15

/* OTG Control register bit description */
#define DP_PULLUP			0x01
#define DM_PULLUP			0x02
#define DP_PULLDOWN			0x04
#define DM_PULLDOWN			0x08
#define ID_PULLDOWN			0x10
#define VBUS_DRV			0x20
#define VBUS_DISCHRG			0x40
#define VBUS_CHRG			0x80

/* Mode Control 1 register bit description */
#define SPEED_REG  			0x01
#define SUSPEND_REG			0x02
#define DAT_SE0				0x04
#define TRANSP_EN			0x08
#define BDIS_ACON_EN			0x10
#define OE_INT_EN			0x20
#define UART_EN				0x40

void isp1301_set_vbus_power(int on);
void isp1301_set_serial_dev(void);
void isp1301_set_serial_host(void);
void isp1301_init(void);
void isp1301_uninit(void);

#endif
