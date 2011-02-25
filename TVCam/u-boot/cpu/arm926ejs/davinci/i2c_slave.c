/*
 * TI DaVinci (TMS320DM365) I2C Slave driver.
 *
 * Copyright (C) 2009 AdvanceV Corp.
 *
 * --------------------------------------------------------
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

#include <common.h>

#ifdef CONFIG_DRIVER_DAVINCI_I2C

#include <i2c_slave.h>
#include <asm/arch/hardware.h>
#include <asm/arch/i2c_defs.h>

#define CHECK_NACK() \
	do {\
		if (tmp & (I2C_TIMEOUT | I2C_STAT_NACK)) {\
			REG(I2C_CON) = 0;\
			return(1);\
		}\
	} while (0)

#define MOD_REG_BIT(val, mask, set) do { \
	if (set) { \
		val |= mask; \
	} else { \
		val &= ~mask; \
	} \
} while (0)

/*ADV_I2C_SLAVE_MODE_ENHANCEMENT_START*/
#define I2C_SLAVE_RECV_MAX_TIMEOUT	0x7FFF0000
#define I2C_SLAVE_XMIT_MAX_TIMEOUT	0x00000010

/* state control in slave mode */
typedef enum i2c_state {
	IDLE_MODE = 0,
	SLAVE_IDLE_MODE,
	SLAVE_RECV_MODE,
	SLAVE_XMIT_MODE
} i2c_state_t;


static int is_slave_mode(void)
{	/* if chip address matches to its own address, then, it operates in slave mode */
	return (  0 == (REG(I2C_CON) & I2C_CON_MST) );
}

static int is_own_slave(u_int8_t chip)
{	/* if chip address matches to its own address, then, it operates in slave mode */
	return ( REG(I2C_OA) == chip );
}

/*ADV_I2C_SLAVE_MODE_ENHANCEMENT_END*/

// return I2C_STAT value
static int poll_i2c_slave_irq(int mask, u_int32_t max_timeout)
{
	int	stat;
	u_int32_t timeout;
	int irq = 0;

#if 0
/* special debug. Need clean up later */
	if (I2C_SLAVE_XMIT_MAX_TIMEOUT == max_timeout) 
		printf("%s(): xmit max_timeout=0x%x\n", __FUNCTION__, max_timeout);
/* end special debug. Need clean up later */
#endif

	for (timeout = 0; timeout < max_timeout; timeout++) {
#ifdef DELAY_1MS
		udelay(1000);
#else
		udelay(1); /* us */
#endif
		stat = REG(I2C_STAT);
		if (stat & mask) {
			irq = 1;
			break;
		}
	}

	if (irq)
		return(stat);

	printf("%s(): time out return. timeout=0x%x, stat=0x%x\n", __FUNCTION__, timeout, stat);
	REG(I2C_STAT) = 0xffff;
	return(stat | I2C_TIMEOUT);
}


void slave_flush_rx(void)
{
	int	dummy;

	while (1) {
		if (!(REG(I2C_STAT) & I2C_STAT_RRDY))
			break;  /* exit since no data is received */

		/* there is a data received */
		dummy = REG(I2C_DRR);
		/* just set RRDY bit to 1 to manually clear receive-data-ready interrupt flag bit */
		REG(I2C_STAT) = I2C_STAT_RRDY;
		udelay(1000);
	}
}


void i2c_slave_init(int speed, int ownaddr)
{
	u_int32_t	div, psc;

	if (REG(I2C_CON) & I2C_CON_EN) {
		REG(I2C_CON) = 0;
		udelay (50000);
	}

	/* only useful when it is in master mode*/
	psc = 2;
	div = (CFG_HZ_CLOCK / ((psc + 1) * speed)) - 10;	/* SCLL + SCLH */
	REG(I2C_PSC) = psc;			/* 27MHz / (2 + 1) = 9MHz */
	REG(I2C_SCLL) = (div * 50) / 100;	/* 50% Duty */
	REG(I2C_SCLH) = div - REG(I2C_SCLL);

	REG(I2C_OA) = ownaddr;
	REG(I2C_CNT) = 0;

	/* Interrupts must be enabled (except transmit) or I2C module won't work */
	REG(I2C_IE) = I2C_IE_AAS_IE | I2C_IE_SCD_IE | I2C_IE_RRDY_IE |
		I2C_IE_ARDY_IE | I2C_IE_NACK_IE | I2C_IE_AL_IE;

	/* Now enable I2C controller (get it out of reset) */
	REG(I2C_CON) = I2C_CON_EN;

	udelay(1000);

#ifndef FAST_BOOT
	printf("%s(): Excuted\n", __FUNCTION__);
#endif

}

void i2c_slave_reset(int ownaddr)
{
	/* perform software i2c reset based on davinci i2c specification */
	REG(I2C_CON) = 0;
	udelay (1000);
	
	if (0 == ownaddr)
		REG(I2C_OA) = CFG_I2C_SLAVE;
	else		
		REG(I2C_OA) = ownaddr;

	/* Interrupts must be enabled (except transmit) or I2C module won't work */
	REG(I2C_IE) = I2C_IE_AAS_IE | I2C_IE_SCD_IE | I2C_IE_RRDY_IE |
		I2C_IE_ARDY_IE | I2C_IE_NACK_IE | I2C_IE_AL_IE;

	/* Now enable I2C controller (get it out of reset) */
	REG(I2C_CON) = I2C_CON_EN;

	udelay(1000);

	printf("%s(): Excuted\n", __FUNCTION__);
}

/*
 *	return: 0 - the chip is a i2c slave
 *	        1 - the chip is a i2c master
*/
int i2c_slave_probe(u_int8_t chip)
{
	int	rc = 1;

	if ( is_own_slave(chip) ) {
		if ( !is_slave_mode() ) {
			printf("%s(): chip == REG(I2C_OA), not in slave mode\n", __FUNCTION__);
			i2c_slave_reset(chip);
		}
		rc = 0;
	}
	printf("davinci: %s(): Excuted %d\n", __FUNCTION__, rc);
	return rc;
}


int i2c_slave_read(u_int8_t *buf, int *len)
{
	int		rc = 0;
	int		i, dummy;
	u_int32_t	tmp;
	i2c_state_t	state = SLAVE_IDLE_MODE;

	/*printf("%s(): len=%d, status=0x%x\n", __FUNCTION__, *len, REG(I2C_STAT));*/

	/* check I2C_STAT(I2C_STAT_AAS) to enter slave-receive-mode from slave-idle-mode */
	tmp = poll_i2c_slave_irq(I2C_STAT_AAS, I2C_SLAVE_RECV_MAX_TIMEOUT);
	if (  !(tmp & I2C_STAT_AAS) ) {
		/* fail to have master-write to slave afer I2C_TIMEOUT time */
		printf("%s(): error: fail to get AAS.\n", __FUNCTION__);
		/* i2c_slave_reset(REG(I2C_OA));*/
		return(-1);
	}
	
	if ( tmp & I2C_STAT_SDIR) {
		printf("%s(): error: not in slave-receive mode.\n", __FUNCTION__);
		/*i2c_slave_reset(REG(I2C_OA));*/
		return (-2);
	}
				
	/* start receive bytes */
	i = 0;
	state = SLAVE_RECV_MODE;
	while ( SLAVE_RECV_MODE == state ) {
		/* get I2C_STAT(I2C_STAT_RRDY) to get data or get I2C_STAT(I2C_STAT_SCD) */
		tmp = poll_i2c_slave_irq(I2C_STAT_RRDY | I2C_STAT_SCD | I2C_STAT_AAS, I2C_SLAVE_RECV_MAX_TIMEOUT);

		if ( tmp & I2C_STAT_RRDY ) {
			/* receive a byte */
			if ( i < *len ) {
				buf[i++] = REG(I2C_DRR);
				/*printf("%s(): receive a byte buf[%d]=0x%x\n", __FUNCTION__, i-1, buf[i-1]);*/
			}
			else {
				dummy = REG(I2C_DRR);
				rc = -3;  /* got more bytes then expected */
				printf("%s(): receive more byte dummy=0x%x\n", __FUNCTION__, dummy);
			}
		}

		if (tmp & I2C_STAT_SCD ) {
			/* manully clear SCD */
			REG(I2C_STAT) |= I2C_STAT_SCD;
			/*printf("%s(): SCD stop-detected status=0x%x\n", __FUNCTION__, REG(I2C_STAT));*/
			/* stop slave-receive */
			state = SLAVE_IDLE_MODE;
		}

		if ((tmp & I2C_STAT_AAS) && (tmp & I2C_STAT_SDIR) ) {
			/*printf("%s(): Enter into slave-transmit mode status=0x%x\n", __FUNCTION__, tmp);*/
			state = SLAVE_XMIT_MODE;
		}
	}  /* end of while-loop */

	*len = i;

	/*printf("%s(): Exit len=%d, status=0x%x\n", __FUNCTION__, *len, REG(I2C_STAT));*/

	return(rc);  // 0 on success
}


int i2c_slave_write(u_int8_t *buf, int *len)
{
	int		rc = 0;
	int		i;
	u_int32_t	tmp;
	i2c_state_t	state = SLAVE_IDLE_MODE;

	/* printf("%s() Enter: len=%d, status=0x%x\n", __FUNCTION__, *len, REG(I2C_STAT));*/

	if (*len < 0) {
		printf("%s(): bogus length %d\n", __FUNCTION__, *len);
		return(-1);
	}

	/* check I2C_STAT(I2C_STAT_AAS) to enter slave-transmit-mode from slave-idle-mode */
	tmp = poll_i2c_slave_irq(I2C_STAT_AAS | I2C_STAT_SDIR, I2C_SLAVE_XMIT_MAX_TIMEOUT);
	if (  !(tmp & I2C_STAT_AAS) || !(tmp & I2C_STAT_SDIR) ) {
		// fail to have master-read to slave afer I2C_TIMEOUT time
		printf("%s(): not in slave_transmit mode tmp=0x%x\n", __FUNCTION__, tmp);
		return(-2);
	}

	/* enable slave-transmit interrupt */
 	/* Interrupts must be enabled (except transmit) or I2C module won't work */
	tmp = REG(I2C_IE);
    MOD_REG_BIT(tmp, I2C_IE_XRDY_IE, 1);
	REG(I2C_IE) = tmp;
		
	/* start transmit bytes */
	state = SLAVE_XMIT_MODE;

	/* transmit 1st byte */
	i = 0;
	REG(I2C_DXR) = buf[i++];
	printf("%s(): send a byte buf[%d]=0x%x\n", __FUNCTION__, 0, buf[0]);

	/* transmit rest bytes */
	while ( SLAVE_XMIT_MODE == state ) {
		/* get I2C_STAT(I2C_STAT_XRDY) to get data or get I2C_STAT(I2C_STAT_SCD)*/
		tmp = poll_i2c_slave_irq(I2C_STAT_XRDY | I2C_STAT_NACK | I2C_STAT_SCD, I2C_SLAVE_XMIT_MAX_TIMEOUT);
		
		/*printf("%s(): while status=0x%x\n", __FUNCTION__, tmp);*/

		if ( (tmp & I2C_STAT_XRDY) && (i<*len) ) {
			printf("%s(): send a byte buf[%d]=0x%x\n", __FUNCTION__, i, buf[i]);
			REG(I2C_DXR) = buf[i++];
		}

		if ( tmp & I2C_STAT_SCD ) {
			/* manully clear SCD */
			REG(I2C_STAT) |= I2C_STAT_SCD;
			printf("%s(): SCD stop-detected status=0x%x\n", __FUNCTION__, REG(I2C_STAT));

			/* disable transmit interrupt */
			tmp = REG(I2C_IE);
		    MOD_REG_BIT(tmp, I2C_IE_XRDY_IE, 0);
			REG(I2C_IE) = tmp;

			/* stop slave-trnasmit */
			state = SLAVE_IDLE_MODE;
		}

		/* special handle  to exit slave-transmit mode*/
		if ( ( I2C_TIMEOUT == (tmp & I2C_TIMEOUT)) && (i>=*len) ) {
			rc = -10; /* fail */
			/* stop slave-trnasmit */
			state = SLAVE_IDLE_MODE;
		}		
	}  /* end of while-loop */

	/*printf("%s(): exit rc=%d\n", __FUNCTION__, rc);*/

	return(rc);
}

u_int32_t i2c_slave_reg_read(u_int32_t reg)
{
	u_int32_t	tmp;
	
	tmp = REG(reg);
	return(tmp);
}

void i2c_slave_reg_write(u_int32_t reg, u_int32_t val)
{
	REG(reg) = val;
}

#endif /* CONFIG_DRIVER_DAVINCI_I2C */
