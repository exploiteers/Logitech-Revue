/*
 * drivers/i2c/busses/i2c-davinci-slave.c
 * AdvanceV I2C adapter driver for slave mode based on TI DAVINCI I2C adapter driver.
 * Modified by Renwei Yan on Nov. 2009
 *
 * Copyright (C) 2006 Texas Instruments.
 * Copyright (C) 2007 MontaVista Software Inc.
 *
 * Updated by Vinod & Sudhakar Feb 2005
 *
 * ----------------------------------------------------------------------------
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * ----------------------------------------------------------------------------
 *
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/clk.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/err.h>
#include <linux/interrupt.h>  //support tasklet
#include <linux/platform_device.h>
#include <linux/io.h>

#include <asm/hardware.h>
#include <asm/mach-types.h>

#include <asm/arch/i2c.h>

//#define DAVINCI_I2C_MAX_BUFFER_LEN	128

/* Hack to enable zero length transfers and smbus quick until clean fix
 * is available
 */
#define DAVINCI_HACK

/* ----- global defines ----------------------------------------------- */
#define DAVINCI_I2C_TIMEOUT	(1*HZ)

/*Davinci I2C registers*/
#define DAVINCI_I2C_OAR_REG		0x00
#define DAVINCI_I2C_IMR_REG		0x04
#define DAVINCI_I2C_STR_REG		0x08
#define DAVINCI_I2C_CLKL_REG	0x0c
#define DAVINCI_I2C_CLKH_REG	0x10
#define DAVINCI_I2C_CNT_REG		0x14
#define DAVINCI_I2C_DRR_REG		0x18
#define DAVINCI_I2C_SAR_REG		0x1c
#define DAVINCI_I2C_DXR_REG		0x20
#define DAVINCI_I2C_MDR_REG		0x24
#define DAVINCI_I2C_IVR_REG		0x28
#define DAVINCI_I2C_EMDR_REG	0x2c
#define DAVINCI_I2C_PSC_REG		0x30

/*Interrupt mask register ICIMR 0x04 fields*/
#define DAVINCI_I2C_IMR_AAS		(1 << 6)
#define DAVINCI_I2C_IMR_SCD		(1 << 5)
#define DAVINCI_I2C_IMR_XRDY	(1 << 4)
#define DAVINCI_I2C_IMR_RRDY	(1 << 3)
#define DAVINCI_I2C_IMR_ARDY	(1 << 2)
#define DAVINCI_I2C_IMR_NACK	(1 << 1)
#define DAVINCI_I2C_IMR_AL		(1 << 0)

/*Interrupt status register ICSTR 0x08 fields*/
#define DAVINCI_I2C_STR_SDIR	(1 << 14)
#define DAVINCI_I2C_STR_NACKSNT	(1 << 13)
#define DAVINCI_I2C_STR_BB		(1 << 12)
#define DAVINCI_I2C_STR_RSFULL	(1 << 11)
#define DAVINCI_I2C_STR_XSMT	(1 << 10)
#define DAVINCI_I2C_STR_AAS		(1 << 9)
#define DAVINCI_I2C_STR_SCD		(1 << 5)
#define DAVINCI_I2C_STR_ICXRDY	(1 << 4)
#define DAVINCI_I2C_STR_ICRRDY	(1 << 3)
#define DAVINCI_I2C_STR_ARDY	(1 << 2)
#define DAVINCI_I2C_STR_NACK	(1 << 1)
#define DAVINCI_I2C_STR_AL		(1 << 0)

/*Mode register ICMDR 0x24 fields*/
#define DAVINCI_I2C_MDR_NACK	(1 << 15)
#define DAVINCI_I2C_MDR_STT		(1 << 13)
#define DAVINCI_I2C_MDR_STP		(1 << 11)
#define DAVINCI_I2C_MDR_MST		(1 << 10)
#define DAVINCI_I2C_MDR_TRX		(1 << 9)
#define DAVINCI_I2C_MDR_XA		(1 << 8)
#define DAVINCI_I2C_MDR_RM		(1 << 7)
#define DAVINCI_I2C_MDR_IRS		(1 << 5)

/*Interrupt vector register ICIVR 0x28 fields*/
#define DAVINCI_I2C_IVR_AAS		0x07
#define DAVINCI_I2C_IVR_SCD		0x06
#define DAVINCI_I2C_IVR_XRDY	0x05
#define DAVINCI_I2C_IVR_RRDR	0x04
#define DAVINCI_I2C_IVR_ARDY	0x03
#define DAVINCI_I2C_IVR_NACK	0x02
#define DAVINCI_I2C_IVR_AL		0x01


#define MOD_REG_BIT(val, mask, set) do { \
	if (set) { \
		val |= mask; \
	} else { \
		val &= ~mask; \
	} \
} while (0)

// add Rx ready
#define I2C_DAVINCI_INTR_ALL    (DAVINCI_I2C_IMR_AAS | \
				 DAVINCI_I2C_IMR_SCD | \
				 DAVINCI_I2C_IMR_ARDY | \
				 DAVINCI_I2C_IMR_NACK | \
                 		 DAVINCI_I2C_IMR_RRDY | \
				 DAVINCI_I2C_IMR_AL)

/*Own i2c slave address of DaVinci SoC*/
#define DAVINCI_I2C_OWN_ADDRESS	0x44   //Provided by Logitech

/*
 * Choose 12Mhz as the targeted I2C clock frequency after the prescaler.
 */
#define I2C_PRESCALED_CLOCK	12000000UL

// interrupt service routine mode
#define	IRS_SLAVE_IDLE  		0
#define	IRS_SLAVE_RECEIVE 		1
#define	IRS_SLAVE_TRANSMIT		2
#define	IRS_SLAVE_XSMT_RECOVER	3

// define structs
struct i2c_davinci_client
{
	int (*interrupt)(int rw_mode, char* buf, int num, int index);
	//int nInter;
	struct list_head list;
	unsigned int	uData;
};

struct davinci_i2c_dev {
	struct device		*dev;
	void __iomem		*base;
	struct completion	cmd_complete;
	struct clk			*clk;
	int					cmd_err;
	u8					*buf;
	size_t				buf_len;
	int					irq;
	// vars added for slave mode operation
	int					rw_mode;
	u8					terminate;
	//u8			recv_buf[DAVINCI_I2C_MAX_BUFFER_LEN];  // local recv buffer 
	//size_t	recv_buf_len;
	//u8		xmit_buf[DAVINCI_I2C_MAX_BUFFER_LEN]; //transmit buffer
	//size_t	xmit_buf_len; // the data length in the transmit buffer
	//size_t	xmit_buf_idx; // transmit buffer index
	size_t				buf_idx; // buffer index
	struct i2c_adapter	adapter;
};

// pointer to hold the i2c_davinci_dev
static struct davinci_i2c_dev* i2c_davinci_dev = 0;

static inline void davinci_i2c_write_reg(struct davinci_i2c_dev *i2c_dev,
					 int reg, u16 val)
{
	__raw_writew(val, i2c_dev->base + reg);
}

static inline u16 davinci_i2c_read_reg(struct davinci_i2c_dev *i2c_dev, int reg)
{
	return __raw_readw(i2c_dev->base + reg);
}

// ADV_I2C_SLAVE_MODE_ENHANCEMENT
// DECLARE_TASKLET(name, funcion, data);
// tasklet is part of linux/interrupt.h
static struct i2c_davinci_client i2c_davinci_interrupt_client = {
	.interrupt = NULL,
        .uData = 0,
};

void i2c_davinci_input_event(unsigned long);
DECLARE_TASKLET(i2c_davinci_tasklet, i2c_davinci_input_event, 0);
static LIST_HEAD(clients);

/*Adding client to handle events from i2c masters*/
int i2c_davinci_add_client(struct i2c_davinci_client *client)
{
	if(client->interrupt)
		list_add_tail(&client->list, &clients);
	else
		return -1;

	return 0;
}

//#define ADV_I2C_SLAVE_TEST

/*Called when event occurs*/
void i2c_davinci_input_event(unsigned long event)
{
    struct list_head   *item;
    struct i2c_davinci_client* client;
    list_for_each(item, &clients)
    {
	client = list_entry(item, struct i2c_davinci_client, list);
	if (client) {
	    if (client->interrupt) {
	        // callback to the interrupt function provided by applicaiton
			if (i2c_davinci_dev->rw_mode == IRS_SLAVE_TRANSMIT) {
			    // in slave-transmit mode, 
#ifdef ADV_I2C_SLAVE_TEST
				// test purpose
			    if ( (i2c_davinci_dev->buf_len == 0) && (i2c_davinci_dev->buf_idx == 0) ) {
					// test: copy recv buf data to xmit buf
					i2c_davinci_dev->buf = i2c_davinci_dev->recv_buf;
					i2c_davinci_dev->buf_len = 4; //i2c_davinci_dev->recv_buf_len;
			    }
#endif //ADV_I2C_SLAVE_TEST
			    client->interrupt(i2c_davinci_dev->rw_mode, i2c_davinci_dev->buf, i2c_davinci_dev->buf_len, i2c_davinci_dev->buf_idx );
				//printk(KERN_INFO "i2c_davinci_input_event: slave-transmit mode num=%d, index=%d\n", i2c_davinci_dev->buf_len, i2c_davinci_dev->buf_idx);
			}
			else {
			    // in slave-receive mode
			    client->interrupt(i2c_davinci_dev->rw_mode, i2c_davinci_dev->buf, i2c_davinci_dev->buf_len, i2c_davinci_dev->buf_len);
			} // end of rw_mode
	    } // end of clinet->interrupt
	} // end of (clinet)
    } // end of list_for_each
} // end of the function

int i2c_davinci_interrupt_callback(int rw_mode, char* buf, int num, int index)
{
    int i;
    //u16 w;

    //printk(KERN_INFO "i2c_davinci_interrupt_callback: num=%d, index=%d\n", num, index);
    
    if ( rw_mode == IRS_SLAVE_TRANSMIT) {
		// in slave-transmit
       if ( num > 0 ) {
            // slave app already put the data (num > 0), then set following and start send.
			if (index == 0 ) {
	        	//printk(KERN_INFO "i2c_davinci_interrupt_callback: slave-transmit mode num=%d, index=%d\n", num, index);
				if (num > 0) {
					// has data to transmit to Master         
					// write 1st one byte to Data Transmit Register (ICDXR)
    	   	    	//printk(KERN_INFO "callback send xmit_buf[0]=0x%x to Master\n", buf[0]);
    	        	i2c_davinci_dev->buf_idx ++;  //must do so to update index first
    	        	i2c_davinci_dev->buf_len --;  //must do so to update index first
			    	davinci_i2c_write_reg(i2c_davinci_dev, DAVINCI_I2C_DXR_REG, buf[0]);
					// this will trigger IVR_ICXRDY interrupt to send rest of the data
				}
				else {
					// design assumption:
					// no data in xmit_buf to transmit to Master
					// The application is responsible to use write() function 
					// to put data into xmit_buf to transmit. 
    	        	// Otherwise, this driver will be in slave-transmit mode forever.   
				}
				return i2c_davinci_dev->buf_idx;
			}  // end if (index==0)
			else {
				// should never be in this case
    		}
		    return index;  // as the index
	    } // end if (num > 0)
    	else {
			// num == 0: slave app has not put the data, 
    	    // then do nothing and just stay in the rw_mode == IRS_SLAVE_TRANSMIT state and 
			// let the slave app later provide the data in xfer and then send.
			// should never be in this case
		} // end num==0
    } // end rw_mode == IRS_SLAVE_TRANSMIT in slave-transmit mode
    else {
		// should never be in this case
		// in slave-receive mode
		printk(KERN_INFO "i2c_davinci_interrupt_callback: slave-receive mode\n");
        for (i=0; i<num; i++) {
		    printk(KERN_INFO "[%d]=0x%x ", i, buf[i]);
        }
        printk(KERN_INFO "\n");
		if (i2c_davinci_dev->buf && (i2c_davinci_dev->buf_idx > 0) ) {
			// notify user app that data has received
			complete(&i2c_davinci_dev->cmd_complete);
		}
    }
    return index;
}
//ADV_I2C_SLAVE_MODE_ENHANCEMENT_END

/*
 * This functions configures I2C and brings I2C out of reset.
 * This function is called during I2C init function. This function
 * also gets called if I2C encounters any errors.
 */
static int i2c_davinci_init(struct davinci_i2c_dev *dev)
{
	struct davinci_i2c_platform_data *pdata = dev->dev->platform_data;
	u16 psc;
	u32 clk;
	u32 input_clock = clk_get_rate(dev->clk);
	u16 w;
	u32 div, d;

	/*During linux boot up this function get called */
	//dev_warn(dev->dev, "i2c_davinci_init() to set it in slave mode\n");

	/* put I2C into reset */
	w = davinci_i2c_read_reg(dev, DAVINCI_I2C_MDR_REG);
	MOD_REG_BIT(w, DAVINCI_I2C_MDR_IRS, 0);
	davinci_i2c_write_reg(dev, DAVINCI_I2C_MDR_REG, w);

	/* NOTE: I2C Clock divider programming info
	 * As per I2C specs the following formulas provide prescaler
	 * and low/high divider values
	 * input clk --> PSC Div -----------> ICCL/H Div --> output clock
	 *                       module clk
	 *
	 * output clk = module clk / (PSC + 1) [ (ICCL + d) + (ICCH + d) ]
	 *
	 * Thus,
	 * (ICCL + ICCH) = clk = (input clk / ((psc +1) * output clk)) - 2d;
	 *
	 * where if PSC == 0, d = 7,
	 *       if PSC == 1, d = 6
	 *       if PSC > 1 , d = 5
	 */
	psc = (input_clock + (I2C_PRESCALED_CLOCK - 1)) /
	      I2C_PRESCALED_CLOCK - 1;
	if (psc == 0)
		d = 7;
	else if (psc == 1)
		d = 6;
	else
		d = 5;

	div = 2 * (psc + 1) * pdata->bus_freq * 1000;
	clk = (input_clock + div - 1)/div;
	if (clk >= d)
		clk -= d;
	else
		clk = 0;

	davinci_i2c_write_reg(dev, DAVINCI_I2C_PSC_REG, psc);
	davinci_i2c_write_reg(dev, DAVINCI_I2C_CLKH_REG, clk);
	davinci_i2c_write_reg(dev, DAVINCI_I2C_CLKL_REG, clk);

	dev_dbg(dev->dev, "CLK  = %d\n", clk);
	dev_dbg(dev->dev, "PSC  = %d\n",
		davinci_i2c_read_reg(dev, DAVINCI_I2C_PSC_REG));
	dev_dbg(dev->dev, "CLKL = %d\n",
		davinci_i2c_read_reg(dev, DAVINCI_I2C_CLKL_REG));
	dev_dbg(dev->dev, "CLKH = %d\n",
		davinci_i2c_read_reg(dev, DAVINCI_I2C_CLKH_REG));

//ADV_I2C_SLAVE_MODE_ENHANCEMENT
	// Read status register	
	w = davinci_i2c_read_reg(dev, DAVINCI_I2C_STR_REG);
	//dev_warn(dev->dev, "STR register bit 3 ICRRDY=0x%x\n", w);

	/* Set own address*/
	davinci_i2c_write_reg(dev, DAVINCI_I2C_OAR_REG, DAVINCI_I2C_OWN_ADDRESS);

	/* Set slave mode */ 
	w = davinci_i2c_read_reg(dev, DAVINCI_I2C_MDR_REG);
	MOD_REG_BIT(w, DAVINCI_I2C_MDR_MST, 0);  
	//dev_warn(dev->dev, "i2c_davinci_init() DAVINCI_I2C_MDR_REG=0x%x\n", w);
//ADV_I2C_SLAVE_MODE_ENHANCEMENT
	
	/* Enable interrupts */
	davinci_i2c_write_reg(dev, DAVINCI_I2C_IMR_REG, I2C_DAVINCI_INTR_ALL);

	/* Take the I2C module out of reset: */
	w = davinci_i2c_read_reg(dev, DAVINCI_I2C_MDR_REG);
	MOD_REG_BIT(w, DAVINCI_I2C_MDR_IRS, 1);
	davinci_i2c_write_reg(dev, DAVINCI_I2C_MDR_REG, w);

	return 0;
}

/*
 * Low level master read/write transaction. This function is called
 * from i2c_davinci_xfer.
 */
static int
i2c_davinci_xfer_msg(struct i2c_adapter *adap, struct i2c_msg *msg, int stop)
{
	struct davinci_i2c_dev *dev = i2c_get_adapdata(adap);
	int r;
	u16 w;
	unsigned long timeout = DAVINCI_I2C_TIMEOUT;
#ifdef DAVINCI_HACK
	u8 zero_byte = 0;
#endif

#ifndef DAVINCI_HACK
	if (msg->len == 0)
		return -EINVAL;

	dev->buf = msg->buf;
	dev->buf_len = msg->len;
#else
	if (msg->len == 0) {
		dev->buf = &zero_byte;
		dev->buf_len = 1;
	} else {
		dev->buf = msg->buf;
		dev->buf_len = msg->len;  // as user provided buffer length
	}
#endif
	//dev_warn(dev->dev, "\n  i2c_davinci_xfer_msg(slave): msg->len=%d\n", msg->len);

	//init_completion(&dev->cmd_complete);
	INIT_COMPLETION(dev->cmd_complete);
	dev->cmd_err = 0;

	dev->buf_idx = 0;

	if (msg->flags & I2C_M_RD) {
		// read as slave-receive
		//dev_warn(dev->dev, "i2c_davinci_xfer_msg(slave read): buf[0]=%d, len=%d\n", *(msg->buf), msg->len);
	
		// check if has data received
		w = davinci_i2c_read_reg(dev, DAVINCI_I2C_STR_REG);
		if ( w & DAVINCI_I2C_STR_ICRRDY ) {
			// slave already receive data in recv-data-register.
			// It should not be in this case usually.

			// decrease length first (must)
            dev->buf_len--;
			dev->buf_idx++;
			// then read 1st date byte into data buffer array
			dev->buf[0] = davinci_i2c_read_reg(dev, DAVINCI_I2C_DRR_REG);
		}
		/* else { // master has not initiate write yet }*/
        #if 0
        else {
            /* ToDo add check to see if driver opened O_NONBLOCK */
            //if (file->f_flags & O_NONBLOCK) 
            {
                return -EAGAIN;
            }
        }
        #endif

		// slave-receive mode wait without timeout
		//r = wait_for_completion_interruptible(&dev->cmd_complete);
		r = wait_for_completion_interruptible_timeout(&dev->cmd_complete, timeout*20);
		#if 1
		if (r == 0) /* && dev->buf_idx == 0)*/ {
			// r == 0 --> interrupt timeout

			i2c_davinci_init(dev);
			return -EAGAIN;
		}
		#endif
	} // end of slave-receive
	else {
		// write as slave-transmit
		// check if it is in slave-transmit mode, mean already get AAS interrupt
		//dev_warn(dev->dev, "i2c_davinci_xfer_msg(slave transmit) rw_mode=%d: buf[0]=%d, len=%d\n", dev->rw_mode, *(dev->buf), dev->buf_len);
		if (dev->rw_mode == IRS_SLAVE_TRANSMIT) {
			// initiate slave transmit 1st byte
			i2c_davinci_interrupt_callback(dev->rw_mode, dev->buf, dev->buf_len, dev->buf_idx);
		}
		else {
			timeout = timeout * dev->buf_len;
			// master has not init master-read yet
			// suppose not in this case
		}
	
		// then with isr to complete transmit data with timeout
		r = wait_for_completion_interruptible_timeout(&dev->cmd_complete, timeout);
		if (r == 0) {
			// interrupt timeout

			i2c_davinci_init(dev);
			return -ETIMEDOUT;
		}
	}

	msg->len = dev->buf_idx;
	
	/* no error */
	if (likely(!dev->cmd_err)) {
			//dev_warn(dev->dev, "i2c_davinci_xfer_msg(slave) after interrupt:msg->buf[0]=0x%x, msg->len=%d, buf_idx=%d\n", *(msg->buf), msg->len, dev->buf_idx);
	}

	return msg->len; //success
}

/*
 * Prepare controller for a transaction and call i2c_davinci_xfer_msg
 */
static int
i2c_davinci_xfer(struct i2c_adapter *adap, struct i2c_msg msgs[], int num)
{
	struct davinci_i2c_dev *dev = i2c_get_adapdata(adap);
	int i;
	int ret = 0;

	//dev_warn(dev->dev, "%s: msgs: %d\n", __FUNCTION__, num);

	for (i = 0; i < num; i++) {
		ret = i2c_davinci_xfer_msg(adap, &msgs[i], (i == (num - 1)));
		if (ret < 0)
			return ret;
	}

	dev_dbg(dev->dev, "%s:%d ret: %d\n", __FUNCTION__, __LINE__, ret);

	return num; 
}

// support ioctl 
// set i2c to slave mode (default) if user wants.
// set its 7-bit own address
// adap, cmd, arg 
static int i2c_davinci_algo_control(struct i2c_adapter *adap, unsigned int cmd, unsigned long arg)
{
	struct davinci_i2c_dev *dev = i2c_get_adapdata(adap);
	int r = 0;
	u32 stat;
	u16 w;

	switch (cmd) {
		case I2C_OWN_ADDRESS:
		//dev_warn(dev->dev, "i2c_davinci_algo_control(I2C_OWN_ADDRESS) cmd=0x%x, arg=0x%x\n", cmd, (unsigned int)arg);
		if (arg> 0x7f) {
			dev_warn(dev->dev, "ioctl(I2C_SLAVE_MODE) set XA as 10-bit addressing\n");
			// have to set 10-bit address
			w = davinci_i2c_read_reg(dev, DAVINCI_I2C_MDR_REG);
			MOD_REG_BIT(w, DAVINCI_I2C_MDR_XA, 1);
			davinci_i2c_write_reg(dev, DAVINCI_I2C_MDR_REG, w);
		}

		/* Set own address*/
		davinci_i2c_write_reg(dev, DAVINCI_I2C_OAR_REG, (u16) arg);
		break;

		case I2C_SLAVE_MODE:
		//dev_warn(dev->dev, "i2c_davinci_algo_control(I2C_SLAVE_MODE) cmd=0x%x, arg=0x%x\n", cmd, (unsigned int)arg);

		//init_completion(&dev->cmd_complete);
		INIT_COMPLETION(dev->cmd_complete);
		dev->cmd_err = 0;

		// init recv buffer and xmit buffer
		dev->buf = NULL;
		dev->rw_mode = IRS_SLAVE_IDLE;
		dev->buf_len = 0;
		dev->buf_idx = 0;
		dev->terminate = 0;

		// init kernel mode bottom-half callback funtion
		if ( NULL == i2c_davinci_interrupt_client.interrupt ) {
			i2c_davinci_interrupt_client.interrupt = i2c_davinci_interrupt_callback;
			if ( (r = i2c_davinci_add_client(&i2c_davinci_interrupt_client)) < 0)
				dev_err(dev->dev, "Fail to add interrupt callback function 0x%x\n", (unsigned int)arg);
		}

		/* take I2C module into soft reset: */
		w = davinci_i2c_read_reg(dev, DAVINCI_I2C_MDR_REG);
		MOD_REG_BIT(w, DAVINCI_I2C_MDR_IRS, 0);
		davinci_i2c_write_reg(dev, DAVINCI_I2C_MDR_REG, w);

		/* Clear any pending interrupts by reading the IVR */
		stat = davinci_i2c_read_reg(dev, DAVINCI_I2C_IVR_REG);

		/* clear status register */
		davinci_i2c_write_reg(dev, DAVINCI_I2C_STR_REG, 0xFFFF);

		/* Set slave or master mode */ 
		w = davinci_i2c_read_reg(dev, DAVINCI_I2C_MDR_REG);
		MOD_REG_BIT(w, DAVINCI_I2C_MDR_MST, arg);
		davinci_i2c_write_reg(dev, DAVINCI_I2C_MDR_REG, w);
		
		if (arg) {
			/* set master-receive mode */
			w = davinci_i2c_read_reg(dev, DAVINCI_I2C_MDR_REG);
    	   	MOD_REG_BIT(w, DAVINCI_I2C_MDR_TRX, 0);
			davinci_i2c_write_reg(dev, DAVINCI_I2C_MDR_REG, w);
			//w = davinci_i2c_read_reg(dev, DAVINCI_I2C_MDR_REG);
		}

		/* Enable interrupts */
		davinci_i2c_write_reg(dev, DAVINCI_I2C_IMR_REG, I2C_DAVINCI_INTR_ALL);

		/* Take the I2C module out of reset: */
		w = davinci_i2c_read_reg(dev, DAVINCI_I2C_MDR_REG);
		MOD_REG_BIT(w, DAVINCI_I2C_MDR_IRS, 1);
		davinci_i2c_write_reg(dev, DAVINCI_I2C_MDR_REG, w);
		break;

		/* Register callback function from user space */
		case I2C_SLAVE_CALLBACK:
		dev_dbg(dev->dev, "i2c_davinci_algo_control(I2C_SLAVE_CALLBACK): try recovery cmd=0x%x, arg=0x%x\n", cmd, (int)arg);
		// used to release I2C bus on slave side
		if ( dev->rw_mode == IRS_SLAVE_TRANSMIT) {
			// initiate dummy write
			dev->rw_mode = IRS_SLAVE_XSMT_RECOVER;

			init_completion(&dev->cmd_complete);
			dev->cmd_err = 0;

			dev->buf_idx++;
			dev->buf_len = (int)arg - 1; 
			davinci_i2c_write_reg(i2c_davinci_dev, DAVINCI_I2C_DXR_REG, 0);
			// then with isr to complete receive and transmit data
			r = wait_for_completion_interruptible(&dev->cmd_complete);
	
			/* no error */
			if (likely(!dev->cmd_err)) {
				dev_warn(dev->dev, "i2c_davinci_ALGO_CONTROL(I2C_SLAVE_CALLBACK) try recovery  after interrupt\n");
			}
		}
		break;

		default:
		dev_warn(dev->dev, "i2c_davinci_algo_control cmd=0x%x, arg=0x%x not supported!!!\n", cmd, (unsigned int)arg);
	}
	return r;
}

static u32 i2c_davinci_func(struct i2c_adapter *adap)
{
#ifndef DAVINCI_HACK
	return I2C_FUNC_I2C | (I2C_FUNC_SMBUS_EMUL & ~I2C_FUNC_SMBUS_QUICK);
#else
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL;
#endif
}

/*
 * Interrupt service routine. This gets called whenever an I2C interrupt
 * occurs.
 */
static irqreturn_t i2c_davinci_isr(int this_irq, void *dev_id, struct pt_regs *regs)
{
	struct davinci_i2c_dev *dev = dev_id;
	u32 stat;
	int count = 0;
	u16 w = 0;

	// read I2C interrupt Vector Register ICIVR. Used b CPU to determine which 
	// event generated the i2c interrupt. Reading ICIVR clears the interrupt flag
	while ((stat = davinci_i2c_read_reg(dev, DAVINCI_I2C_IVR_REG))) {
		//dev_dbg(dev->dev, "%s: stat=0x%x\n", __FUNCTION__, stat);
		if (count++ == 100) {
			dev_warn(dev->dev, "Too much work in one IRQ\n");
			break;
		}
		stat = stat & 0x7; // bit 2-0 has INTCode
		switch (stat) {
		//Arbitration-lost interrupt (AL)
		case DAVINCI_I2C_IVR_AL:
	        w = davinci_i2c_read_reg(dev, DAVINCI_I2C_MDR_REG);
			if ( w &  DAVINCI_I2C_MDR_MST ) {
				//in master mode 
				dev->cmd_err |= DAVINCI_I2C_STR_AL;
				dev_warn(dev->dev, "Arbitration-lost AL 1h detected\n");
				dev->buf_len = 0;
				complete(&dev->cmd_complete);
			}
			break;

		// No-acknowledgment interrupt (NACK)
		case DAVINCI_I2C_IVR_NACK:
			w = davinci_i2c_read_reg(dev, DAVINCI_I2C_STR_REG);
            dev_warn(dev->dev, "No-acknowledgment NACK 2h detected. STR=0x%x\n", w);
			// not much to do in slave-mode
			w = davinci_i2c_read_reg(dev, DAVINCI_I2C_MDR_REG);
			if ( w &  DAVINCI_I2C_MDR_MST ) {
				//in master mode                     
      			dev->cmd_err |= DAVINCI_I2C_STR_NACK;
		        dev_warn(dev->dev, "No-acknowledgment NACK 2h detected in master mode\n");
		        dev->buf_len = 0;
		        complete(&dev->cmd_complete);
			}
			break;

		// Register-access-ready interrupt (ARDY)
		// only applicable when the I2C is in the master-mode
		// so, should not get this interrupt in slave-mode
		case DAVINCI_I2C_IVR_ARDY:
			w = davinci_i2c_read_reg(dev, DAVINCI_I2C_MDR_REG);				// IMR (interrupt mask reg):
				//   set Transmit-data-ready interrupt enable bit in IMR
				w = davinci_i2c_read_reg(i2c_davinci_dev, DAVINCI_I2C_IMR_REG);
				w |= DAVINCI_I2C_IMR_XRDY;
				davinci_i2c_write_reg(i2c_davinci_dev, DAVINCI_I2C_IMR_REG, w);

            if ( w &  DAVINCI_I2C_MDR_MST ) {			
				// ICSTR status regirster to clear ARDY bit (write to 1)
				w = davinci_i2c_read_reg(dev, DAVINCI_I2C_STR_REG);
				MOD_REG_BIT(w, DAVINCI_I2C_STR_ARDY, 1);
				dev_warn(dev->dev, "Register-access-ready ARDY 3h detected\n");
				davinci_i2c_write_reg(dev, DAVINCI_I2C_STR_REG, w);
				complete(&dev->cmd_complete);
            }
			break;

		// Receive-data-ready interrupt (ICRRDY)
		case DAVINCI_I2C_IVR_RRDR:
	       	// handles slave-receive only   
	    	// Read status register	
	    	//w = davinci_i2c_read_reg(dev, DAVINCI_I2C_STR_REG);
	    	//dev_warn(dev->dev, "Receive-data-ready ICRRDY 4h detected, STR bit 3 ICRRDY=0x%x\n", w);
			// in slave-mode
    		if ( NULL != dev->buf ) {
				if ( dev->buf_len > 0 ) {
					w = dev->buf_idx;

					// read date into data buffer arrary
					dev->buf[dev->buf_idx++] = davinci_i2c_read_reg(dev, DAVINCI_I2C_DRR_REG);
					// decrease length
	               	dev->buf_len--;
					//dev_warn(dev->dev, "Recv: buf[%d]=0x%x\n", w, dev->buf[w]);
				}
				else if ( dev->buf_idx > 0) {
					dev_warn(dev->dev, "Receive-data-ready RRDY 4h: exceeds buffer length\n");
    	           	// read extra date into a temp var
					w = davinci_i2c_read_reg(dev, DAVINCI_I2C_DRR_REG);
				}
			} 
#if 0 //2010-0107 improve performance
			else {
				// user space has not provided buffer yet
				// leave the received data in data-receive-register
           	}
#endif 
			break; //end of IVR_RDR
	
		// Transmit-data-ready interrupt (ICXRDY)
		case DAVINCI_I2C_IVR_XRDY:
	        // Read status register	
			//w = davinci_i2c_read_reg(dev, DAVINCI_I2C_STR_REG);
			//dev_warn(dev->dev, "Transmit-data-ready XRDY 5h detected. STR=0x%x. bit4-ICXRDY=%x, xmit_buf_len=%d, buf_idx=%d\n", w, w&DAVINCI_I2C_STR_ICXRDY, dev->buf_len, i2c_davinci_dev->buf_idx);

			// slave-transmit mode
			if ( IRS_SLAVE_TRANSMIT == dev->rw_mode ) {
				if ( (dev->buf != NULL)  && (dev->buf_len > 0) ) {
					w = dev->buf_idx;

					// write date into i2c transmit register
					davinci_i2c_write_reg(dev, DAVINCI_I2C_DXR_REG, dev->buf[dev->buf_idx++]);
					dev->buf_len--;
					//dev_warn(dev->dev, "Xmit: buf[%d]=0x%x,\n", w, dev->buf[w]);
		     	} 
				else {
					if ( (dev->buf != NULL) && (dev->buf_idx > 0) ) {
						//no more byte data transmit, no more IVR_XRDY interrupt
						// IMR: mask out transmit interrupt
					    w = davinci_i2c_read_reg(dev, DAVINCI_I2C_IMR_REG);
					    MOD_REG_BIT(w, DAVINCI_I2C_IMR_XRDY, 0);
					    davinci_i2c_write_reg(dev, DAVINCI_I2C_IMR_REG, w);
						dev_dbg(dev->dev, "Transmit-data-ready XDRY 5h: exceeds buffer length and no more data to master\n");
					} // no more slave-tranmit
					else {
						// user space has not provided buffer yet
						// nothing to write to data-transmit-register
						dev_warn(dev->dev, "Transmit-data-ready XDRY 5h: user space has not provided buffer yet\n");
					}
				}
			} // end IRS_SLAVE_TRANSMIT mode
			else if ( IRS_SLAVE_XSMT_RECOVER == dev->rw_mode) {
				if (dev->buf_len > 0) {
					davinci_i2c_write_reg(dev, DAVINCI_I2C_DXR_REG, 0);
					dev->buf_idx++;
					dev->buf_len--;
				}
			}
			break; //end of IVR_XRDY

		case DAVINCI_I2C_IVR_SCD:
            // note: in slave-mode, STR_SCD(bit5)=0 and STR_AAS(bit9)=0
			// clear SCD bit (automatically cleared by read IVR_REG)
#if 0 //2010-0107 improve performance
			if( dev->buf_idx ) {
			    // check if it is in slave-mode  
			    w = davinci_i2c_read_reg(dev, DAVINCI_I2C_MDR_REG);
			    if ( 0 == (w & DAVINCI_I2C_MDR_MST) ) {
#endif
					// in slave-mode
					if ( IRS_SLAVE_TRANSMIT == dev->rw_mode || 
						 IRS_SLAVE_XSMT_RECOVER == dev->rw_mode  ) {
						// in slave-transmit mode 
						// IMR: mask out transmit interrupt
					    w = davinci_i2c_read_reg(dev, DAVINCI_I2C_IMR_REG);
					    MOD_REG_BIT(w, DAVINCI_I2C_IMR_XRDY, 0);
					    davinci_i2c_write_reg(dev, DAVINCI_I2C_IMR_REG, w);
					}
					else {
						// in slave-receive mode
						// Note: in slave-mode, never disable the recv intrrupt
						// otherwise, davinci i2c could not recieve data anymore
					} 

			        //dev_warn(dev->dev, "Stop-condidtion-detected.\n");
					//dev_warn(dev->dev, "Stop-detected.\n");

					dev->rw_mode = IRS_SLAVE_IDLE; 
					dev->buf = NULL;
					dev->buf_len = 0;

					// get back to xfer_msg or ioctl caller
					complete(&dev->cmd_complete);
#if 0 //2010-0107 improve performance
				} // slave-mode
			}
#endif
			break;  //end of IVR_SCD

		case DAVINCI_I2C_IVR_AAS:
			// In slave mode so it receives and match slave address

			// read I2C Interrupt Status Register STR
			w = davinci_i2c_read_reg(dev, DAVINCI_I2C_STR_REG);
	                
			// based on w & DAVINCI_I2C_STR_SDIR
			// = 0 in slave-receive mode
			// = 1 in slave-transmit mode
			// then decided to enable recieve or transmit interrupt
			if (w & DAVINCI_I2C_STR_SDIR) {
				// enter slave-transmit mode
				//dev_warn(dev->dev, "AAS-detected(XSMT) STR=0x%x, buf_idx=%d\n", w, dev->buf_idx);

				// IMR (interrupt mask reg):
				//   set Transmit-data-ready interrupt enable bit in IMR
			    w = davinci_i2c_read_reg(dev, DAVINCI_I2C_IMR_REG);
			    MOD_REG_BIT(w, DAVINCI_I2C_IMR_XRDY, 1);
			    davinci_i2c_write_reg(dev, DAVINCI_I2C_IMR_REG, w);

				if (dev->rw_mode == IRS_SLAVE_RECEIVE) {
					// in slave-recv and has not get STOP, then it is in
					// S A|R L S A|W D D D D... P pattern

					// set to xmit mode
					dev->rw_mode = IRS_SLAVE_TRANSMIT;
					//dev->buf_idx = 0;
					dev->buf = NULL;
					dev->buf_len = 0;
					// back to xfer_msg to return user space read(fd...) call first
					complete(&dev->cmd_complete);
					// let user space initiate the write
				} 
				else {                         
					// set to xmit mode
					dev->rw_mode = IRS_SLAVE_TRANSMIT;

					// note: 2211 will issue master-read immediate after master-write
					// before the return of the interrup
					if ( (dev->buf != NULL)  && (dev->buf_len) ) {
						// user space has provided data buffer for slave-transmit
						// initiate the slave-transmit
						dev->buf_idx = 0;
				        // transmit the 1st byte to master	dev->buf_len = 0;
						tasklet_hi_schedule(&i2c_davinci_tasklet);
					}
				}
			}  // end in slave-transmit mode 
			else {
				// enter slave-receive mode
 				//dev_warn(dev->dev, "AAS-detected 7h(RECV) STR=0x%x\n", w);

				// set to receive mode
				dev->rw_mode = IRS_SLAVE_RECEIVE;
#if 0 //2010-0107 improve performance				
				// IMR (interrupt mask reg):
				//   set Receive-data-ready interrupt enable bit in IMR
				w = davinci_i2c_read_reg(dev, DAVINCI_I2C_IMR_REG);
				MOD_REG_BIT(w, DAVINCI_I2C_IMR_RRDY, 1);
				davinci_i2c_write_reg(dev, DAVINCI_I2C_IMR_REG,	w);
#endif

	        }
	        break;  //end of IVR_AAS
		}/* switch */
	}/* while */

	return count ? IRQ_HANDLED : IRQ_NONE;
}

static struct i2c_algorithm i2c_davinci_algo = {
	.master_xfer	= i2c_davinci_xfer,
	.algo_control = i2c_davinci_algo_control,
	.functionality	= i2c_davinci_func,
};

static int davinci_i2c_probe(struct platform_device *pdev)
{
	struct davinci_i2c_dev *dev;
	struct i2c_adapter *adap;
	struct resource *mem, *irq, *ioarea;
	int r;

	if (!pdev->dev.platform_data)
		return -ENODEV;

	/* NOTE: driver uses the static register mapping */
	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem) {
		dev_err(&pdev->dev, "no mem resource?\n");
		return -ENODEV;
	}

	irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!irq) {
		dev_err(&pdev->dev, "no irq resource?\n");
		return -ENODEV;
	}

	ioarea = request_mem_region(mem->start, (mem->end - mem->start) + 1,
				    pdev->name);
	if (!ioarea) {
		dev_err(&pdev->dev, "I2C region already claimed\n");
		return -EBUSY;
	}

	dev = kzalloc(sizeof(struct davinci_i2c_dev), GFP_KERNEL);
	if (!dev) {
		r = -ENOMEM;
		goto err_release_region;
	}

	// save to the static var for slave mode interrupt callback use
	i2c_davinci_dev = dev;
	
	dev->dev = get_device(&pdev->dev);
	dev->irq = irq->start;
	platform_set_drvdata(pdev, dev);

	dev->clk = clk_get(&pdev->dev, "I2CCLK");
	if (IS_ERR(dev->clk)) {
		r = -ENODEV;
		goto err_free_mem;
	}

	r = clk_enable(dev->clk);
	if (r < 0)
		goto err_free_clk;

	dev->base = (void __iomem *)IO_ADDRESS(mem->start);
	/* init dev */
	dev->cmd_err = 0;
	dev->buf = NULL;
	dev->rw_mode = IRS_SLAVE_IDLE;
	dev->buf_len = 0;
	dev->buf_idx = 0;
	dev->terminate = 0;
    init_completion(&dev->cmd_complete);
	
	i2c_davinci_init(dev);

	
	r = request_irq(dev->irq, i2c_davinci_isr, 0, pdev->name, dev);
	if (r) {
		dev_err(&pdev->dev, "failure requesting irq %i\n", dev->irq);
		goto err_unuse_clocks;
	}

	adap = &dev->adapter;
	i2c_set_adapdata(adap, dev);
	adap->owner = THIS_MODULE;
	adap->class = I2C_CLASS_HWMON;
	strlcpy(adap->name, "DaVinci I2C adapter (slave mode)", sizeof(adap->name));
	adap->algo = &i2c_davinci_algo;
	adap->dev.parent = &pdev->dev;

	/* FIXME */
	adap->timeout = 1;
	adap->retries = 1;

	adap->nr = pdev->id;
	r = i2c_add_adapter(adap);
	if (r) {
		dev_err(&pdev->dev, "failure adding adapter\n");
		goto err_free_irq;
	}

	return 0;

err_free_irq:
	free_irq(dev->irq, dev);
err_unuse_clocks:
	clk_disable(dev->clk);
err_free_clk:
	clk_put(dev->clk);
	dev->clk = NULL;
err_free_mem:
	platform_set_drvdata(pdev, NULL);
	put_device(&pdev->dev);
	kfree(dev);
err_release_region:
	release_mem_region(mem->start, (mem->end - mem->start) + 1);

	return r;
}

static int davinci_i2c_remove(struct platform_device *pdev)
{
	struct davinci_i2c_dev *dev = platform_get_drvdata(pdev);
	struct resource *mem;

	platform_set_drvdata(pdev, NULL);
	i2c_del_adapter(&dev->adapter);
	put_device(&pdev->dev);

	clk_disable(dev->clk);
	clk_put(dev->clk);
	dev->clk = NULL;

	davinci_i2c_write_reg(dev, DAVINCI_I2C_MDR_REG, 0);
	free_irq(IRQ_I2C, dev);
	kfree(dev);

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	release_mem_region(mem->start, (mem->end - mem->start) + 1);
	return 0;
}

static struct platform_driver davinci_i2c_driver = {
	.probe		= davinci_i2c_probe,
	.remove		= davinci_i2c_remove,
	.driver		= {
		.name	= "i2c_davinci",
		.owner	= THIS_MODULE,
	},
};

/* I2C may be needed to bring up other drivers */
static int __init davinci_i2c_init_driver(void)
{
	return platform_driver_register(&davinci_i2c_driver);
}
subsys_initcall(davinci_i2c_init_driver);

static void __exit davinci_i2c_exit_driver(void)
{
	platform_driver_unregister(&davinci_i2c_driver);
}
module_exit(davinci_i2c_exit_driver);

MODULE_AUTHOR("AdvanceV Corp.");
MODULE_DESCRIPTION("TI DaVinci I2C bus adapter (slave mode)");
MODULE_LICENSE("GPL");
