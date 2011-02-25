/*
 * i2c.c - driver for Blackfin on-chip TWI/I2C
 *
 * Copyright (c) 2006-2008 Analog Devices Inc.
 *
 * Licensed under the GPL-2 or later.
 */

#include <common.h>

#ifdef CONFIG_HARD_I2C

#include <asm/blackfin.h>
#include <i2c.h>
#include <asm/io.h>
#include <asm/mach-common/bits/twi.h>

/* Two-Wire Interface		(0xFFC01400 - 0xFFC014FF) */
#ifdef TWI0_CLKDIV
#define bfin_read_TWI_CLKDIV()               bfin_read_TWI0_CLKDIV()
#define bfin_write_TWI_CLKDIV(val)           bfin_write_TWI0_CLKDIV(val)
#define bfin_read_TWI_CONTROL()              bfin_read_TWI0_CONTROL()
#define bfin_write_TWI_CONTROL(val)          bfin_write_TWI0_CONTROL(val)
#define bfin_read_TWI_SLAVE_CTL()            bfin_read_TWI0_SLAVE_CTL()
#define bfin_write_TWI_SLAVE_CTL(val)        bfin_write_TWI0_SLAVE_CTL(val)
#define bfin_read_TWI_SLAVE_STAT()           bfin_read_TWI0_SLAVE_STAT()
#define bfin_write_TWI_SLAVE_STAT(val)       bfin_write_TWI0_SLAVE_STAT(val)
#define bfin_read_TWI_SLAVE_ADDR()           bfin_read_TWI0_SLAVE_ADDR()
#define bfin_write_TWI_SLAVE_ADDR(val)       bfin_write_TWI0_SLAVE_ADDR(val)
#define bfin_read_TWI_MASTER_CTL()           bfin_read_TWI0_MASTER_CTL()
#define bfin_write_TWI_MASTER_CTL(val)       bfin_write_TWI0_MASTER_CTL(val)
#define bfin_read_TWI_MASTER_STAT()          bfin_read_TWI0_MASTER_STAT()
#define bfin_write_TWI_MASTER_STAT(val)      bfin_write_TWI0_MASTER_STAT(val)
#define bfin_read_TWI_MASTER_ADDR()          bfin_read_TWI0_MASTER_ADDR()
#define bfin_write_TWI_MASTER_ADDR(val)      bfin_write_TWI0_MASTER_ADDR(val)
#define bfin_read_TWI_INT_STAT()             bfin_read_TWI0_INT_STAT()
#define bfin_write_TWI_INT_STAT(val)         bfin_write_TWI0_INT_STAT(val)
#define bfin_read_TWI_INT_MASK()             bfin_read_TWI0_INT_MASK()
#define bfin_write_TWI_INT_MASK(val)         bfin_write_TWI0_INT_MASK(val)
#define bfin_read_TWI_FIFO_CTL()             bfin_read_TWI0_FIFO_CTL()
#define bfin_write_TWI_FIFO_CTL(val)         bfin_write_TWI0_FIFO_CTL(val)
#define bfin_read_TWI_FIFO_STAT()            bfin_read_TWI0_FIFO_STAT()
#define bfin_write_TWI_FIFO_STAT(val)        bfin_write_TWI0_FIFO_STAT(val)
#define bfin_read_TWI_XMT_DATA8()            bfin_read_TWI0_XMT_DATA8()
#define bfin_write_TWI_XMT_DATA8(val)        bfin_write_TWI0_XMT_DATA8(val)
#define bfin_read_TWI_XMT_DATA_16()          bfin_read_TWI0_XMT_DATA16()
#define bfin_write_TWI_XMT_DATA16(val)       bfin_write_TWI0_XMT_DATA16(val)
#define bfin_read_TWI_RCV_DATA8()            bfin_read_TWI0_RCV_DATA8()
#define bfin_write_TWI_RCV_DATA8(val)        bfin_write_TWI0_RCV_DATA8(val)
#define bfin_read_TWI_RCV_DATA16()           bfin_read_TWI0_RCV_DATA16()
#define bfin_write_TWI_RCV_DATA16(val)       bfin_write_TWI0_RCV_DATA16(val)
#endif

#ifdef DEBUG_I2C
#define PRINTD(fmt,args...)	do {	\
	DECLARE_GLOBAL_DATA_PTR;	\
	if (gd->have_console)		\
		printf(fmt ,##args);	\
	} while (0)
#else
#define PRINTD(fmt,args...)
#endif

#ifndef CONFIG_TWICLK_KHZ
#define CONFIG_TWICLK_KHZ	50
#endif

/* All transfers are described by this data structure */
struct i2c_msg {
	u16 addr;		/* slave address */
	u16 flags;
#define I2C_M_STOP		0x2
#define I2C_M_RD		0x1
	u16 len;		/* msg length */
	u8 *buf;		/* pointer to msg data */
};

/**
 * i2c_reset: - reset the host controller
 */
static void i2c_reset(void)
{
	/* Disable TWI */
	bfin_write_TWI_CONTROL(0);
	SSYNC();

	/* Set TWI internal clock as 10MHz */
	bfin_write_TWI_CONTROL(((get_sclk() / 1024 / 1024 + 5) / 10) & 0x7F);

	/* Set Twi interface clock as specified */
	if (CONFIG_TWICLK_KHZ > 400)
		bfin_write_TWI_CLKDIV(((5 * 1024 / 400) << 8) | ((5 * 1024 /
						400) & 0xFF));
	else
		bfin_write_TWI_CLKDIV(((5 * 1024 /
					CONFIG_TWICLK_KHZ) << 8) | ((5 * 1024 /
						CONFIG_TWICLK_KHZ)
						& 0xFF));

	/* Enable TWI */
	bfin_write_TWI_CONTROL(bfin_read_TWI_CONTROL() | TWI_ENA);
	SSYNC();
}

int wait_for_completion(struct i2c_msg *msg, int timeout_count)
{
	unsigned short twi_int_stat;
	unsigned short mast_stat;
	int i;

	for (i = 0; i < timeout_count; i++) {
		twi_int_stat = bfin_read_TWI_INT_STAT();
		mast_stat = bfin_read_TWI_MASTER_STAT();

		if (XMTSERV & twi_int_stat) {
			/* Transmit next data */
			if (msg->len > 0) {
				bfin_write_TWI_XMT_DATA8(*(msg->buf++));
				msg->len--;
			} else if (msg->flags & I2C_M_STOP)
				bfin_write_TWI_MASTER_CTL
				    (bfin_read_TWI_MASTER_CTL() | STOP);
			SSYNC();
			/* Clear status */
			bfin_write_TWI_INT_STAT(XMTSERV);
			SSYNC();
			i = 0;
		}
		if (RCVSERV & twi_int_stat) {
			if (msg->len > 0) {
				/* Receive next data */
				*(msg->buf++) = bfin_read_TWI_RCV_DATA8();
				msg->len--;
			} else if (msg->flags & I2C_M_STOP) {
				bfin_write_TWI_MASTER_CTL
				    (bfin_read_TWI_MASTER_CTL() | STOP);
				SSYNC();
			}
			/* Clear interrupt source */
			bfin_write_TWI_INT_STAT(RCVSERV);
			SSYNC();
			i = 0;
		}
		if (MERR & twi_int_stat) {
			bfin_write_TWI_INT_STAT(MERR);
			bfin_write_TWI_INT_MASK(0);
			bfin_write_TWI_MASTER_STAT(0x3e);
			bfin_write_TWI_MASTER_CTL(0);
			SSYNC();
			/*
			 * if both err and complete int stats are set,
			 * return proper results.
			 */
			if (MCOMP & twi_int_stat) {
				bfin_write_TWI_INT_STAT(MCOMP);
				bfin_write_TWI_INT_MASK(0);
				bfin_write_TWI_MASTER_CTL(0);
				SSYNC();
				/*
				 * If it is a quick transfer,
				 * only address bug no data, not an err.
				 */
				if (msg->len == 0 && mast_stat & BUFRDERR)
					return 0;
				/*
				 * If address not acknowledged return -3,
				 * else return 0.
				 */
				else if (!(mast_stat & ANAK))
					return 0;
				else
					return -3;
			}
			return -1;
		}
		if (MCOMP & twi_int_stat) {
			bfin_write_TWI_INT_STAT(MCOMP);
			SSYNC();
			bfin_write_TWI_INT_MASK(0);
			bfin_write_TWI_MASTER_CTL(0);
			SSYNC();
			return 0;
		}
	}
	if (msg->flags & I2C_M_RD)
		return -4;
	else
		return -2;
}

/**
 * i2c_transfer: - Transfer one byte over the i2c bus
 *
 * This function can tranfer a byte over the i2c bus in both directions.
 * It is used by the public API functions.
 *
 * @return:	 0: transfer successful
 *		-1: transfer fail
 *		-2: transmit timeout
 *		-3: ACK missing
 *		-4: receive timeout
 *		-5: controller not ready
 */
int i2c_transfer(struct i2c_msg *msg)
{
	int ret = 0;
	int timeout_count = 10000;
	int len = msg->len;

	if (!(bfin_read_TWI_CONTROL() & TWI_ENA)) {
		ret = -5;
		goto transfer_error;
	}

	while (bfin_read_TWI_MASTER_STAT() & BUSBUSY)
		continue;

	/* Set Transmit device address */
	bfin_write_TWI_MASTER_ADDR(msg->addr);

	/*
	 * FIFO Initiation.
	 * Data in FIFO should be discarded before start a new operation.
	 */
	bfin_write_TWI_FIFO_CTL(0x3);
	SSYNC();
	bfin_write_TWI_FIFO_CTL(0);
	SSYNC();

	if (!(msg->flags & I2C_M_RD)) {
		/* Transmit first data */
		if (msg->len > 0) {
			PRINTD("1 in i2c_transfer: buf=%d, len=%d\n", *msg->buf,
			       len);
			bfin_write_TWI_XMT_DATA8(*(msg->buf++));
			msg->len--;
			SSYNC();
		}
	}

	/* clear int stat */
	bfin_write_TWI_INT_STAT(MERR | MCOMP | XMTSERV | RCVSERV);

	/* Interrupt mask . Enable XMT, RCV interrupt */
	bfin_write_TWI_INT_MASK(MCOMP | MERR |
			((msg->flags & I2C_M_RD) ? RCVSERV : XMTSERV));
	SSYNC();

	if (len > 0 && len <= 255)
		bfin_write_TWI_MASTER_CTL((len << 6));
	else if (msg->len > 255) {
		bfin_write_TWI_MASTER_CTL((0xff << 6));
		msg->flags &= I2C_M_STOP;
	} else
		bfin_write_TWI_MASTER_CTL(0);

	/* Master enable */
	bfin_write_TWI_MASTER_CTL(bfin_read_TWI_MASTER_CTL() | MEN |
			((msg->flags & I2C_M_RD)
			 ? MDIR : 0) | ((CONFIG_TWICLK_KHZ >
					 100) ? FAST : 0));
	SSYNC();

	ret = wait_for_completion(msg, timeout_count);
	PRINTD("3 in i2c_transfer: ret=%d\n", ret);

 transfer_error:
	switch (ret) {
	case 1:
		PRINTD(("i2c_transfer: error: transfer fail\n"));
		break;
	case 2:
		PRINTD(("i2c_transfer: error: transmit timeout\n"));
		break;
	case 3:
		PRINTD(("i2c_transfer: error: ACK missing\n"));
		break;
	case 4:
		PRINTD(("i2c_transfer: error: receive timeout\n"));
		break;
	case 5:
		PRINTD(("i2c_transfer: error: controller not ready\n"));
		i2c_reset();
		break;
	default:
		break;
	}
	return ret;

}

/* ---------------------------------------------------------------------*/
/* API Functions							*/
/* ---------------------------------------------------------------------*/

void i2c_init(int speed, int slaveaddr)
{
	i2c_reset();
}

/**
 * i2c_probe: - Test if a chip answers for a given i2c address
 *
 * @chip:	address of the chip which is searched for
 * @return:	0 if a chip was found, -1 otherwhise
 */

int i2c_probe(uchar chip)
{
	struct i2c_msg msg;
	u8 probebuf;

	i2c_reset();

	probebuf = 0;
	msg.addr = chip;
	msg.flags = 0;
	msg.len = 1;
	msg.buf = &probebuf;
	if (i2c_transfer(&msg))
		return -1;

	msg.addr = chip;
	msg.flags = I2C_M_RD;
	msg.len = 1;
	msg.buf = &probebuf;
	if (i2c_transfer(&msg))
		return -1;

	return 0;
}

/**
 *   i2c_read: - Read multiple bytes from an i2c device
 *
 *   chip:    I2C chip address, range 0..127
 *   addr:    Memory (register) address within the chip
 *   alen:    Number of bytes to use for addr (typically 1, 2 for larger
 *		memories, 0 for register type devices with only one
 *		register)
 *   buffer:  Where to read/write the data
 *   len:     How many bytes to read/write
 *
 *   Returns: 0 on success, not 0 on failure
 */

int i2c_read(uchar chip, uint addr, int alen, uchar * buffer, int len)
{
	struct i2c_msg msg;
	u8 addr_bytes[3];	/* lowest...highest byte of data address */

	PRINTD("i2c_read: chip=0x%x, addr=0x%x, alen=0x%x, len=0x%x\n", chip,
			addr, alen, len);

	if (alen > 0) {
		addr_bytes[0] = (u8) ((addr >> 0) & 0x000000FF);
		addr_bytes[1] = (u8) ((addr >> 8) & 0x000000FF);
		addr_bytes[2] = (u8) ((addr >> 16) & 0x000000FF);
		msg.addr = chip;
		msg.flags = 0;
		msg.len = alen;
		msg.buf = addr_bytes;
		if (i2c_transfer(&msg))
			return -1;
	}

	/* start read sequence */
	PRINTD(("i2c_read: start read sequence\n"));
	msg.addr = chip;
	msg.flags = I2C_M_RD;
	msg.len = len;
	msg.buf = buffer;
	if (i2c_transfer(&msg))
		return -1;

	return 0;
}

/**
 *   i2c_write: -  Write multiple bytes to an i2c device
 *
 *   chip:    I2C chip address, range 0..127
 *   addr:    Memory (register) address within the chip
 *   alen:    Number of bytes to use for addr (typically 1, 2 for larger
 *		memories, 0 for register type devices with only one
 *		register)
 *   buffer:  Where to read/write the data
 *   len:     How many bytes to read/write
 *
 *   Returns: 0 on success, not 0 on failure
 */

int i2c_write(uchar chip, uint addr, int alen, uchar * buffer, int len)
{
	struct i2c_msg msg;
	u8 addr_bytes[3];	/* lowest...highest byte of data address */

	PRINTD
		("i2c_write: chip=0x%x, addr=0x%x, alen=0x%x, len=0x%x, buf0=0x%x\n",
		 chip, addr, alen, len, buffer[0]);

	/* chip address write */
	if (alen > 0) {
		addr_bytes[0] = (u8) ((addr >> 0) & 0x000000FF);
		addr_bytes[1] = (u8) ((addr >> 8) & 0x000000FF);
		addr_bytes[2] = (u8) ((addr >> 16) & 0x000000FF);
		msg.addr = chip;
		msg.flags = 0;
		msg.len = alen;
		msg.buf = addr_bytes;
		if (i2c_transfer(&msg))
			return -1;
	}

	/* start read sequence */
	PRINTD(("i2c_write: start write sequence\n"));
	msg.addr = chip;
	msg.flags = 0;
	msg.len = len;
	msg.buf = buffer;
	if (i2c_transfer(&msg))
		return -1;

	return 0;

}

uchar i2c_reg_read(uchar chip, uchar reg)
{
	uchar buf;

	PRINTD("i2c_reg_read: chip=0x%02x, reg=0x%02x\n", chip, reg);
	i2c_read(chip, reg, 0, &buf, 1);
	return (buf);
}

void i2c_reg_write(uchar chip, uchar reg, uchar val)
{
	PRINTD("i2c_reg_write: chip=0x%02x, reg=0x%02x, val=0x%02x\n", chip,
			reg, val);
	i2c_write(chip, reg, 0, &val, 1);
}

#endif /* CONFIG_HARD_I2C */
