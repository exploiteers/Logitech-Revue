/*
 *  include/asm-arm/arch-davinci/i2c-client.h
 *
 * Copyright (C) 2006 Texas Instruments Inc
 * Copyright (C) 2008 MontaVista Software, Inc. <source@mvista.com>
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
/* i2c-client.h */
#ifndef _DAVINCI_I2C_CLIENT_H_
#define _DAVINCI_I2C_CLIENT_H_

enum i2c_expander_pins {
        USB_DRVVBUS = 0,
        VDDIMX_EN = 1,
        VLYNQ_ON = 2,
        CF_RESET = 3,
        WLAN_RESET = 5,
        ATA_SEL = 6,
        CF_SEL = 7,
	/* DM646X expanders */
	ATA_SEL_DM646X = 0,
	ATA_PWD_DM646X = 1,
	VSCALE_ON_DM646X = 2,
	VLYNQ_RESET_DM646X = 3,
	CIR_DEMOD_DM646X = 4,
	CIR_MOD_DM646X = 5,
	I2C_INT_DM646X = 6,
	USB_FB_DM646X = 7,
	/* DA8xx UI board expander */
	MUX_MODE_DA8XX = 6,
	SPI_MODE_DA8XX = 7,
};

struct davinci_i2c_expander {
	u16 address;
	u8 init_data;
	const char *name;
	void (*setup)(void);
	int (*validate)(enum i2c_expander_pins pin);
	int (*special)(enum i2c_expander_pins pin, unsigned val);
};

/*
 * The board code will set this to point to its expander pin setup function
 * to be called upon registering the I2C client.
 */
extern struct davinci_i2c_expander davinci_i2c_expander;

int davinci_i2c_expander_op(enum i2c_expander_pins pin, unsigned val);
int davinci_i2c_write(u8 size, u8 *val, u16 client_addr);
int davinci_i2c_read(u8 size, u8 *val, u16 client_addr);

#endif
