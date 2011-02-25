/*
 * (C) Copyright 2003 - 2007
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
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
 *
 * Based on the MPC5xxx code.
 */

#include <common.h>

DECLARE_GLOBAL_DATA_PTR;

#ifdef CONFIG_HARD_I2C

#include <mpc512x.h>
#include <i2c.h>

#define immr ((immap_t *)CFG_IMMR)

/* by default set I2C bus 0 active */
static unsigned int bus_num = 0;

#define I2C_TIMEOUT	100
#define I2C_RETRIES	3

struct mpc512x_i2c_tap {
	int scl2tap;
	int tap2tap;
};

static int  mpc_reg_in(volatile u32 *reg);
static void mpc_reg_out(volatile u32 *reg, int val, int mask);
static int  wait_for_bb(void);
static int  wait_for_pin(int *status);
static int  do_address(uchar chip, char rdwr_flag);
static int  send_bytes(uchar chip, char *buf, int len);
static int  receive_bytes(uchar chip, char *buf, int len);
static int  mpc_get_fdr(int);

static int mpc_reg_in (volatile u32 *reg)
{
	int ret = *reg >> 24;
	__asm__ __volatile__ ("eieio");
	return ret;
}

static void mpc_reg_out (volatile u32 *reg, int val, int mask)
{
	int tmp;

	if (!mask) {
		*reg = val << 24;
	} else {
		tmp = mpc_reg_in (reg);
		*reg = ((tmp & ~mask) | (val & mask)) << 24;
	}
	__asm__ __volatile__ ("eieio");

	return;
}

static int wait_for_bb (void)
{
	i2c512x_dev_t *regs = &immr->i2c.dev[bus_num];
	int timeout = I2C_TIMEOUT;
	int status;

	status = mpc_reg_in (&regs->msr);

	while (timeout-- && (status & I2C_BB)) {
		volatile int temp;
		mpc_reg_out (&regs->mcr, I2C_STA, I2C_STA);
		temp = mpc_reg_in (&regs->mdr);
		mpc_reg_out (&regs->mcr, 0, I2C_STA);
		mpc_reg_out (&regs->mcr, 0, 0);
		mpc_reg_out (&regs->mcr, I2C_EN, 0);

		udelay (1000);
		status = mpc_reg_in (&regs->msr);
	}

	return (status & I2C_BB);
}

static int wait_for_pin (int *status)
{
	i2c512x_dev_t *regs = &immr->i2c.dev[bus_num];
	int timeout = I2C_TIMEOUT;

	*status = mpc_reg_in (&regs->msr);

	while (timeout-- && !(*status & I2C_IF)) {
		udelay (1000);
		*status = mpc_reg_in (&regs->msr);
	}

	if (!(*status & I2C_IF)) {
		return -1;
	}

	mpc_reg_out (&regs->msr, 0, I2C_IF);

	return 0;
}

static int do_address (uchar chip, char rdwr_flag)
{
	i2c512x_dev_t *regs = &immr->i2c.dev[bus_num];
	int status;

	chip <<= 1;

	if (rdwr_flag) {
		chip |= 1;
	}

	mpc_reg_out (&regs->mcr, I2C_TX, I2C_TX);
	mpc_reg_out (&regs->mdr, chip, 0);

	if (wait_for_pin (&status)) {
		return -2;
	}

	if (status & I2C_RXAK) {
		return -3;
	}

	return 0;
}

static int send_bytes (uchar chip, char *buf, int len)
{
	i2c512x_dev_t *regs = &immr->i2c.dev[bus_num];
	int wrcount;
	int status;

	for (wrcount = 0; wrcount < len; ++wrcount) {

		mpc_reg_out (&regs->mdr, buf[wrcount], 0);

		if (wait_for_pin (&status)) {
			break;
		}

		if (status & I2C_RXAK) {
			break;
		}

	}

	return !(wrcount == len);
}

static int receive_bytes (uchar chip, char *buf, int len)
{
	i2c512x_dev_t *regs = &immr->i2c.dev[bus_num];
	int dummy   = 1;
	int rdcount = 0;
	int status;
	int i;

	mpc_reg_out (&regs->mcr, 0, I2C_TX);

	for (i = 0; i < len; ++i) {
		buf[rdcount] = mpc_reg_in (&regs->mdr);

		if (dummy) {
			dummy = 0;
		} else {
			rdcount++;
		}

		if (wait_for_pin (&status)) {
			return -4;
		}
	}

	mpc_reg_out (&regs->mcr, I2C_TXAK, I2C_TXAK);
	buf[rdcount++] = mpc_reg_in (&regs->mdr);

	if (wait_for_pin (&status)) {
		return -5;
	}

	mpc_reg_out (&regs->mcr, 0, I2C_TXAK);

	return 0;
}

/**************** I2C API ****************/

void i2c_init (int speed, int saddr)
{
	int i;
	for(i = 0; i < I2C_BUS_CNT; i++){
		i2c512x_dev_t *regs = &immr->i2c.dev[i];
		mpc_reg_out (&regs->mcr, 0, 0);

		/* Set clock */
		mpc_reg_out (&regs->mfdr, mpc_get_fdr (speed), 0);
		mpc_reg_out (&regs->madr, saddr << 1, 0);

		/* Enable module */
		mpc_reg_out (&regs->mcr, I2C_EN, I2C_INIT_MASK);
		mpc_reg_out (&regs->msr, 0, I2C_IF);
	}

	/* Disable interrupts */
	immr->i2c.icr = 0;
	/* Turn off filters */
	immr->i2c.mifr = 0;
	return;
}

static int mpc_get_fdr (int speed)
{
	static int fdr = -1;

	if (fdr == -1) {
		ulong best_speed = 0;
		ulong divider;
		ulong ips, scl;
		ulong bestmatch = 0xffffffffUL;
		int best_i = 0, best_j = 0, i, j;
		int SCL_Tap[] = { 9, 10, 12, 15, 5, 6, 7, 8};
		struct mpc512x_i2c_tap scltap[] = {
			{4, 1},
			{4, 2},
			{6, 4},
			{6, 8},
			{14, 16},
			{30, 32},
			{62, 64},
			{126, 128}
		};

		ips = gd->ips_clk;
		for (i = 7; i >= 0; i--) {
			for (j = 7; j >= 0; j--) {
				scl = 2 * (scltap[j].scl2tap +
					   (SCL_Tap[i] - 1) * scltap[j].tap2tap
					   + 2);
				if (ips <= speed*scl) {
					if ((speed*scl - ips) < bestmatch) {
						bestmatch = speed*scl - ips;
						best_i = i;
						best_j = j;
						best_speed = ips/scl;
					}
				}
			}
		}
		divider = (best_i & 3) | ((best_i & 4) << 3) | (best_j << 2);
		if (gd->flags & GD_FLG_RELOC) {
			fdr = divider;
		} else {
			debug("%ld kHz, \n", best_speed / 1000);
			return divider;
		}
	}

	return fdr;
}

int i2c_probe (uchar chip)
{
	i2c512x_dev_t *regs = &immr->i2c.dev[bus_num];
	int i;

	for (i = 0; i < I2C_RETRIES; i++) {
		mpc_reg_out (&regs->mcr, I2C_STA, I2C_STA);

		if (! do_address (chip, 0)) {
			mpc_reg_out (&regs->mcr, 0, I2C_STA);
			udelay (500);
			break;
		}

		mpc_reg_out (&regs->mcr, 0, I2C_STA);
		udelay (500);
	}

	return (i == I2C_RETRIES);
}

int i2c_read (uchar chip, uint addr, int alen, uchar *buf, int len)
{
	char xaddr[4];
	i2c512x_dev_t *regs = &immr->i2c.dev[bus_num];
	int ret = -1;

	xaddr[0] = (addr >> 24) & 0xFF;
	xaddr[1] = (addr >> 16) & 0xFF;
	xaddr[2] = (addr >>  8) & 0xFF;
	xaddr[3] =  addr	& 0xFF;

	if (wait_for_bb ()) {
		printf ("i2c_read: bus is busy\n");
		goto Done;
	}

	mpc_reg_out (&regs->mcr, I2C_STA, I2C_STA);
	if (do_address (chip, 0)) {
		printf ("i2c_read: failed to address chip\n");
		goto Done;
	}

	if (send_bytes (chip, &xaddr[4-alen], alen)) {
		printf ("i2c_read: send_bytes failed\n");
		goto Done;
	}

	mpc_reg_out (&regs->mcr, I2C_RSTA, I2C_RSTA);
	if (do_address (chip, 1)) {
		printf ("i2c_read: failed to address chip\n");
		goto Done;
	}

	if (receive_bytes (chip, (char *)buf, len)) {
		printf ("i2c_read: receive_bytes failed\n");
		goto Done;
	}

	ret = 0;
Done:
	mpc_reg_out (&regs->mcr, 0, I2C_STA);
	return ret;
}

int i2c_write (uchar chip, uint addr, int alen, uchar *buf, int len)
{
	char xaddr[4];
	i2c512x_dev_t *regs = &immr->i2c.dev[bus_num];
	int ret = -1;

	xaddr[0] = (addr >> 24) & 0xFF;
	xaddr[1] = (addr >> 16) & 0xFF;
	xaddr[2] = (addr >>  8) & 0xFF;
	xaddr[3] =  addr	& 0xFF;

	if (wait_for_bb ()) {
		printf ("i2c_write: bus is busy\n");
		goto Done;
	}

	mpc_reg_out (&regs->mcr, I2C_STA, I2C_STA);
	if (do_address (chip, 0)) {
		printf ("i2c_write: failed to address chip\n");
		goto Done;
	}

	if (send_bytes (chip, &xaddr[4-alen], alen)) {
		printf ("i2c_write: send_bytes failed\n");
		goto Done;
	}

	if (send_bytes (chip, (char *)buf, len)) {
		printf ("i2c_write: send_bytes failed\n");
		goto Done;
	}

	ret = 0;
Done:
	mpc_reg_out (&regs->mcr, 0, I2C_STA);
	return ret;
}

uchar i2c_reg_read (uchar chip, uchar reg)
{
	uchar buf;

	i2c_read (chip, reg, 1, &buf, 1);

	return buf;
}

void i2c_reg_write (uchar chip, uchar reg, uchar val)
{
	i2c_write (chip, reg, 1, &val, 1);

	return;
}


int i2c_set_bus_num (unsigned int bus)
{
	if (bus >= I2C_BUS_CNT) {
		return -1;
	}
	bus_num = bus;

	return 0;
}

unsigned int i2c_get_bus_num (void)
{
	return bus_num;
}

/* TODO */
unsigned int i2c_get_bus_speed (void)
{
	return -1;
}

int i2c_set_bus_speed (unsigned int speed)
{
	if (speed != CFG_I2C_SPEED)
		return -1;

	return 0;
}

#endif	/* CONFIG_HARD_I2C */
