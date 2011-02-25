/*
 * (C) Copyright 2009 AdvanceV Corp.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef _I2C_SLAVE_H_
#define _I2C_SLAVE_H_

/*
 * The implementation is for TI Davinci I2C slave driver
 * It is in i2c_slave.c
 */
#define I2C_SLAVE_MAX_BUFFER_LEN 64

/*
 * Initialization, must be called once on start up, may be called
 * repeatedly to change its own addresses and perform i2c software reset
 */
void i2c_slave_init(int speed, int ownaddr);

/*
 * i2c software reset
 */
void i2c_slave_reset(int ownaddr);

/*
 * Probe the given I2C chip address.  Returns 0 if a chip responded (is a slave),
 * not 0 on failure.
 */
int i2c_slave_probe(uchar chip);

/*
 * Read/Write interface:
 *   chip:    I2C chip address, range 0..127
 *   addr:    Memory (register) address within the chip
 *            Note: Davinci only has register within the chip
 *   alen:    Number of bytes to use for addr (typically 1, 2 for larger
 *              memories, 0 for register type devices (Davinci is in this case) 
 *              with only one register)
 *   buffer:  Where to read/write the data provided by the caller
 *   len:     How many bytes to read/write. 
 *            As input: it is the max buffer length
 *            As output: it is the bytes actually read/write.
 *
 *   Returns: 0 on success, not 0 on failure
 */
int i2c_slave_read(uchar *buffer, int *len);
int i2c_slave_write(uchar *buffer, int *len);

/*
 * Utility routines to read/write davinci i2c 32-bits registers.
 * Refer to i2c_defs.h for the registers
 * For example, to read I2C_STAT
 * unsigned int tmp = i2c_slave_regA_read(I2C_STAT);
 * To set it is own address
 * i2c_slave_reg_write(I2C_OWN, CFG_I2C_SLAVE);
 */
unsigned int i2c_slave_reg_read (unsigned int reg);
void  i2c_slave_reg_write(unsigned int reg, unsigned int val);

#endif	/* _I2C_SLAVE_H_ */
