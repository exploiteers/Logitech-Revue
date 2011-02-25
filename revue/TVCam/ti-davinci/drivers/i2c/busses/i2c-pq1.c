/*
 * The combined i2c adapter and algorithm driver for 8xx processors.
 *
 * Copyright (c) 1999 Dan Malek (dmalek@jlc.net).
 *
 * moved into proper i2c interface;
 * Brad Parker (brad@heeltoe.com)
 *
 * (C) 2007 Montavista Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/stddef.h>
#include <linux/dma-mapping.h>
#include <linux/i2c.h>
#include <linux/device.h>
#include <asm/commproc.h>
#include <asm/io.h>
#include <asm/mpc8xx.h>
#include <asm/of_device.h>
#include <asm/of_platform.h>
#include <asm/mpc8xx.h>
#include <asm/time.h>
#include <asm/fs_pd.h>


#define CPM_MAX_READ    513
#define BD_SC_NAK		((ushort)0x0004)	/* NAK - did not respond */
#define BD_SC_OV		((ushort)0x0002)	/* OV - receive overrun */
#define CPM_CR_CLOSE_RXBD	((ushort)0x0007)

/* Try to define this if you have an older CPU (earlier than rev D4) */
#undef	I2C_CHIP_ERRATA

struct m8xx_i2c {
	struct of_device *ofdev;
	struct i2c_adapter adap;
	wait_queue_head_t queue;

	i2c8xx_t *regs;
	iic_t *pram;
	cpm8xx_t *cp;
	uint dp_base;
	int irq;
	int reloc;

	u_char temp[CPM_MAX_READ];
};

static irqreturn_t i2c_pq1_cpm_irq(int irq, void *dev_id, struct pt_regs *rg)
{
	struct m8xx_i2c *i2c = (struct m8xx_i2c *) dev_id;
	i2c8xx_t *regs = i2c->regs;

#ifdef I2C_CHIP_ERRATA
	/* Chip errata, clear enable. This is not needed on rev D4 CPUs
	 * This should probably be removed and replaced by I2C_CHIP_ERRATA stuff
	 * Someone with a buggy CPU needs to confirm that
	 */
	out_8(&regs->i2c_i2mod, in_8(&regs->i2c_i2mod) | ~1);
#endif
	/* Clear interrupt. */
	out_8(&regs->i2c_i2cer, 0xff);

	/* Get 'me going again. */
	wake_up_interruptible(&i2c->queue);

	return IRQ_HANDLED;
}

static void i2c_pq1_cpm_reset_params(struct m8xx_i2c *i2c)
{
	iic_t *pram = i2c->pram;

	pram->iic_tbase = i2c->dp_base;
	pram->iic_rbase = i2c->dp_base + sizeof(cbd_t) * 2;

	pram->iic_tfcr = SMC_EB;
	pram->iic_rfcr = SMC_EB;

	pram->iic_mrblr = CPM_MAX_READ;

	pram->iic_rstate = 0;
	pram->iic_rdp = 0;
	pram->iic_rbptr = pram->iic_rbase;
	pram->iic_rbc = 0;
	pram->iic_rxtmp = 0;
	pram->iic_tstate = 0;
	pram->iic_tdp = 0;
	pram->iic_tbptr = pram->iic_tbase;
	pram->iic_tbc = 0;
	pram->iic_txtmp = 0;
}

static void i2c_pq1_force_close(struct i2c_adapter *adap)
{
	struct m8xx_i2c *i2c = i2c_get_adapdata(adap);
	i2c8xx_t *regs = i2c->regs;

	if (i2c->reloc == 0) {	/* micro code disabled */
		cpm8xx_t *cp = i2c->cp;
		u16 v =
		    mk_cr_cmd(CPM_CR_CH_I2C, CPM_CR_CLOSE_RXBD) | CPM_CR_FLG;

		dev_dbg(&adap->dev, "force_close\n");

		out_be16(&cp->cp_cpcr, v);
		wait_event_timeout(i2c->queue,
				   !(in_be16(&cp->cp_cpcr) & CPM_CR_FLG),
				   HZ * 5);
	}
	out_8(&regs->i2c_i2cmr, 0x00);	/* Disable all interrupts */
	out_8(&regs->i2c_i2cer, 0xff);
}

/* Read from I2C...
 * abyte = address byte, with r/w flag already set
 */
static int i2c_pq1_read(struct i2c_adapter *adap, u_char abyte,
			char *buf, int count)
{
	struct m8xx_i2c *i2c = i2c_get_adapdata(adap);
	iic_t *pram = i2c->pram;
	i2c8xx_t *regs = i2c->regs;
	cbd_t *tbdf, *rbdf;
	u_char *tb;
	int ret = 0;

	if (count >= CPM_MAX_READ)
		return -EINVAL;

	/* check for and use a microcode relocation patch */
	if (i2c->reloc)
		i2c_pq1_cpm_reset_params(i2c);

	tbdf = (cbd_t *) cpm_dpram_addr(pram->iic_tbase);
	rbdf = (cbd_t *) cpm_dpram_addr(pram->iic_rbase);

	/* To read, we need an empty buffer of the proper length.
	 * All that is used is the first byte for address, the remainder
	 * is just used for timing (and doesn't really have to exist).
	 */
	tb = i2c->temp;
	tb = (u_char *) (((uint) tb + 15) & ~15);
	tb[0] = abyte;		/* Device address byte w/rw flag */

	dev_dbg(&adap->dev, "i2c_pq1_read(abyte=0x%x)\n", abyte);

	tbdf->cbd_bufaddr = dma_map_single(&i2c->ofdev->dev,
						tb, count, DMA_TO_DEVICE);

	tbdf->cbd_datlen = count + 1;
	tbdf->cbd_sc = BD_SC_READY | BD_SC_LAST | BD_SC_WRAP | BD_IIC_START;

	pram->iic_mrblr = count + 1;	/* prevent excessive read, +1
					   is needed otherwise will the
					   RXB interrupt come too early */

	rbdf->cbd_datlen = 0;
	rbdf->cbd_bufaddr = dma_map_single(&i2c->ofdev->dev,
						buf, count, DMA_FROM_DEVICE);
	rbdf->cbd_sc = BD_SC_EMPTY | BD_SC_WRAP | BD_SC_INTRPT;

	/* Chip bug, set enable here */
	out_8(&regs->i2c_i2cmr, 0x13);	/* Enable some interupts */
	out_8(&regs->i2c_i2cer, 0xff);
	out_8(&regs->i2c_i2mod, in_8(&regs->i2c_i2mod) | 1);	/* Enable */
	out_8(&regs->i2c_i2com, in_8(&regs->i2c_i2com) | 0x80);	/* Begin transmission */

	/* Wait for IIC transfer */
	ret = wait_event_interruptible_timeout(i2c->queue, 0, 1 * HZ);

	dma_unmap_single(i2c->ofdev->dev, tb, 1, DMA_TO_DEVICE);
	dma_unmap_single(i2c->ofdev->dev, buf, count, DMA_FROM_DEVICE);

	if (ret < 0) {
		i2c_pq1_force_close(adap);
		dev_dbg(&adap->dev, "I2C read: timeout!\n");
		return -EIO;
	}
#ifdef I2C_CHIP_ERRATA
	/* Chip errata, clear enable. This is not needed on rev D4 CPUs.
	   Disabling I2C too early may cause too short stop condition */
	udelay(4);
	out_8(&regs->i2c_i2mod, in_8(&regs->i2c_i2mod) | ~1);
#endif

	dev_dbg(&adap->dev, "tx sc %04x, rx sc %04x\n",
		tbdf->cbd_sc, rbdf->cbd_sc);

	if (tbdf->cbd_sc & BD_SC_READY) {
		dev_dbg(&adap->dev, "I2C read; complete but tbuf ready\n");
		i2c_pq1_force_close(adap);
		dev_dbg(&adap->dev, "tx sc %04x, rx sc %04x\n",
			tbdf->cbd_sc, rbdf->cbd_sc);
	}

	if (tbdf->cbd_sc & BD_SC_NAK) {
		dev_dbg(&adap->dev, "I2C read; no ack\n");
		return -EREMOTEIO;
	}

	if (rbdf->cbd_sc & BD_SC_EMPTY) {
		/* force_close(adap); */
		dev_dbg(&adap->dev,
			"I2C read; complete but rbuf empty\n");
		dev_dbg(&adap->dev, "tx sc %04x, rx sc %04x\n",
			tbdf->cbd_sc, rbdf->cbd_sc);
		return -EREMOTEIO;
	}

	if (rbdf->cbd_sc & BD_SC_OV) {
		dev_dbg(&adap->dev, "I2C read; Overrun\n");
		return -EREMOTEIO;
	}

	dev_dbg(&adap->dev, "read %d bytes\n", rbdf->cbd_datlen);

	if (rbdf->cbd_datlen < count) {
		dev_dbg(&adap->dev,
			"I2C read; short, wanted %d got %d\n", count,
			rbdf->cbd_datlen);
		return 0;
	}

	return count;
}

/* Write to I2C...
 * addr = address byte, with r/w flag already set
 */
static int i2c_pq1_write(struct i2c_adapter *adap, u_char abyte,
			 char *buf, int count)
{
	struct m8xx_i2c *i2c = i2c_get_adapdata(adap);
	iic_t *pram = i2c->pram;
	i2c8xx_t *regs = i2c->regs;
	cbd_t *tbdf;
	u_char *tb;
	int ret = 0;

	/* check for and use a microcode relocation patch */
	if (i2c->reloc) {
		i2c_pq1_cpm_reset_params(i2c);
	}

	tb = i2c->temp;
	tb = (u_char *) (((uint) tb + 15) & ~15);
	*tb = abyte;		/* Device address byte w/rw flag */

	dev_dbg(&adap->dev, "i2c_pq1_write(abyte=0x%x)\n", abyte);

	/* set up 2 descriptors */
	tbdf = (cbd_t *) cpm_dpram_addr(pram->iic_tbase);

	tbdf[0].cbd_bufaddr = dma_map_single(&i2c->ofdev->dev,
						tb, 1, DMA_TO_DEVICE);
	tbdf[0].cbd_datlen = 1;
	tbdf[0].cbd_sc = BD_SC_READY | BD_IIC_START;

	tbdf[1].cbd_bufaddr = dma_map_single(&i2c->ofdev->dev,
						buf, count, DMA_TO_DEVICE);
	tbdf[1].cbd_datlen = count;
	tbdf[1].cbd_sc = BD_SC_READY | BD_SC_INTRPT | BD_SC_LAST | BD_SC_WRAP;

	/* Chip bug, set enable here */
	out_8(&regs->i2c_i2cmr, 0x13);	/* Enable some interupts */
	out_8(&regs->i2c_i2cer, 0xff);
	out_8(&regs->i2c_i2mod, in_8(&regs->i2c_i2mod) | 1);	/* Enable */
	out_8(&regs->i2c_i2com, in_8(&regs->i2c_i2com) | 0x80);	/* Begin transmission */

	/* Wait for IIC transfer */
	ret = wait_event_interruptible_timeout(i2c->queue, 0, 1 * HZ);

	dma_unmap_single(i2c->ofdev->dev, tb, 1, DMA_TO_DEVICE);
	dma_unmap_single(i2c->ofdev->dev, buf, count, DMA_TO_DEVICE);

	if (ret < 0) {
		i2c_pq1_force_close(adap);
		dev_dbg(&adap->dev, "I2C write: timeout!\n");
		return -EIO;
	}
#ifdef I2C_CHIP_ERRATA
	/* Chip errata, clear enable. This is not needed on rev D4 CPUs.
	 * Disabling I2C too early may cause too short stop condition.
	 */
	udelay(4);
	out_8(&regs->i2c_i2mod, in_8(&regs->i2c_i2mod) | ~1);
#endif
	dev_dbg(&adap->dev, "tx0 sc %04x, tx1 sc %04x\n",
		tbdf[0].cbd_sc, tbdf[1].cbd_sc);

	if (tbdf->cbd_sc & BD_SC_NAK) {
		dev_dbg(&adap->dev, "I2C write; no ack\n");
		return 0;
	}

	if (tbdf->cbd_sc & BD_SC_READY) {
		dev_dbg(&adap->dev,
			"I2C write; complete but tbuf ready\n");
		return 0;
	}

	return count;
}

static int i2c_pq1_cpm_xfer(struct i2c_adapter *adap, struct i2c_msg *msgs, int num)
{
	struct i2c_msg *pmsg;
	int i, ret;
	u_char addr;

	for (i = 0; i < num; i++) {
		pmsg = &msgs[i];

		dev_dbg(&adap->dev, "i2c-xfer: %d addr=0x%x flags=0x%x len=%d\n buf=%lx\n",
			i, pmsg->addr, pmsg->flags, pmsg->len,
			(unsigned long)pmsg->buf);

		addr = pmsg->addr << 1;
		if (pmsg->flags & I2C_M_RD)
			addr |= 1;

		if (pmsg->flags & I2C_M_RD) {
			/* read bytes into buffer */
			ret = i2c_pq1_read(adap, addr, pmsg->buf, pmsg->len);
			dev_dbg(&adap->dev, "i2c-xfer: read %d bytes\n", ret);
			if (ret < pmsg->len) {
				return (ret < 0) ? ret : -EREMOTEIO;
			}
		} else {
			/* write bytes from buffer */
			ret = i2c_pq1_write(adap, addr, pmsg->buf, pmsg->len);
			dev_dbg(&adap->dev, "i2c-xfer: wrote %d\n", ret);
			if (ret < pmsg->len) {
				return (ret < 0) ? ret : -EREMOTEIO;
			}
		}
	}

	return num;
}

static int i2c_pq1_cpm_init(struct m8xx_i2c *i2c)
{
	iic_t *pram = i2c->pram;
	i2c8xx_t *regs = i2c->regs;
	unsigned char brg;
	int ret;

	/* Initialize the parameter ram.
	 * We need to make sure many things are initialized to zero,
	 * especially in the case of a microcode patch.
	 */
	pram->iic_rstate = 0;
	pram->iic_rdp = 0;
	pram->iic_rbptr = 0;
	pram->iic_rbc = 0;
	pram->iic_rxtmp = 0;
	pram->iic_tstate = 0;
	pram->iic_tdp = 0;
	pram->iic_tbptr = 0;
	pram->iic_tbc = 0;
	pram->iic_txtmp = 0;

	/* Set up the IIC parameters in the parameter ram. */
	pram->iic_tbase = i2c->dp_base;
	pram->iic_rbase = i2c->dp_base + sizeof(cbd_t) * 2;

	pram->iic_tfcr = SMC_EB;
	pram->iic_rfcr = SMC_EB;

	/* Set maximum receive size. */
	pram->iic_mrblr = CPM_MAX_READ;

	/* Initialize Tx/Rx parameters. */
	if (i2c->reloc == 0) {
		cpm8xx_t *cp = i2c->cp;
		int ret;

		u16 v = mk_cr_cmd(CPM_CR_CH_I2C, CPM_CR_INIT_TRX) | CPM_CR_FLG;

		out_be16(&cp->cp_cpcr, v);
		ret = wait_event_timeout(i2c->queue,
					 !(in_be16(&cp->cp_cpcr) & CPM_CR_FLG),
					 HZ * 1);
		if (!ret)
			return -EIO;

	} else {
		pram->iic_rbptr = pram->iic_rbase;
		pram->iic_tbptr = pram->iic_tbase;
		pram->iic_rstate = 0;
		pram->iic_tstate = 0;
	}

	/* Select an arbitrary address. Just make sure it is unique. */
	out_8(&regs->i2c_i2add, 0xfe);

	/* Make clock run at 60 kHz. */
	brg = ppc_proc_freq / (32 * 2 * 60000) - 3;
	out_8(&regs->i2c_i2brg, brg);

	out_8(&regs->i2c_i2mod, 0x00);
	out_8(&regs->i2c_i2com, 0x01);	/* Master mode */

	/* Disable interrupts. */
	out_8(&regs->i2c_i2cmr, 0);
	out_8(&regs->i2c_i2cer, 0xff);

	/* Install interrupt handler. */
	ret = request_irq(i2c->irq, i2c_pq1_cpm_irq, 0, "8xx_i2c", i2c);
	if (ret)
		return -EIO;

	return 0;
}

static void i2c_pq1_shutdown(struct m8xx_i2c *i2c)
{
	i2c8xx_t *regs = i2c->regs;

	out_8(&regs->i2c_i2mod, in_8(&regs->i2c_i2mod) | ~1);
	out_8(&regs->i2c_i2cmr, 0);
	out_8(&regs->i2c_i2cer, 0xff);
}

static int i2c_pq1_res_init(struct m8xx_i2c *i2c)
{
	volatile cpm8xx_t *cp;
	struct resource r;
	struct of_device *ofdev = i2c->ofdev;
	struct device_node *np = ofdev->node;

	/* Pointer to Communication Processor
	 */
	cp = i2c->cp = (cpm8xx_t *)immr_map(im_cpm);

	i2c->irq = irq_of_parse_and_map(np, 0);
  	if (i2c->irq < 0)
		return -EINVAL;

	if (of_address_to_resource(np, 0, &r))
		return -EINVAL;

	i2c->regs = ioremap(r.start, r.end - r.start + 1);
	if (i2c->regs == NULL)
		return -EINVAL;

	if (of_address_to_resource(np, 1, &r)) {
		iounmap(i2c->regs);
		return -EINVAL;
	}

	i2c->pram = ioremap(r.start, r.end - r.start + 1);
	if (i2c->pram == NULL) {
		iounmap(i2c->regs);
		return -EINVAL;
	}

	/* Check for and use a microcode relocation patch. */
	if ((i2c->reloc = i2c->pram->iic_rpbase))
		i2c->pram = (iic_t *)&cp->cp_dpmem[i2c->pram->iic_rpbase];

	/* Allocate space for two transmit and two receive buffer
	 * descriptors in the DP ram.
	 */
	i2c->dp_base = cpm_dpalloc(sizeof(cbd_t) * 4, 8);

	return 0;
}

static void i2c_pq1_ret_free(struct m8xx_i2c *i2c)
{
	iounmap(i2c->regs);
	iounmap(i2c->pram);
}

static u32 i2c_pq1_func(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL;
}

static struct i2c_algorithm i2c_pq1_algo = {
	.master_xfer = i2c_pq1_cpm_xfer,
	.functionality = i2c_pq1_func,
};

static const struct i2c_adapter pq1_ops = {
	.owner		= THIS_MODULE,
	.name		= "i2c-pq1",
	.id		= I2C_HW_MPC8XX_EPON,
	.algo		= &i2c_pq1_algo,
	.class		= I2C_CLASS_HWMON,
	.timeout	= 1,
	.retries	= 1
};

static int i2c_pq1_probe(struct of_device* ofdev, const struct of_device_id *match)
{
	int ret;
	struct m8xx_i2c *i2c;

	if (!(i2c = kzalloc(sizeof(*i2c), GFP_KERNEL))) {
		return -ENOMEM;
	}
	i2c->ofdev = ofdev;
	init_waitqueue_head(&i2c->queue);

	ret = i2c_pq1_res_init(i2c);
        if (ret != 0) {
		return ret;
	}

        /* initialise the i2c controller */
	ret = i2c_pq1_cpm_init(i2c);
        if (ret != 0) {
		i2c_pq1_ret_free(i2c);
		return ret;
	}

	dev_set_drvdata(&ofdev->dev, i2c);
	i2c->adap = pq1_ops;
	i2c_set_adapdata(&i2c->adap, i2c);
	i2c->adap.dev.parent = &ofdev->dev;

	if ((ret = i2c_add_adapter(&i2c->adap)) < 0) {
		printk(KERN_ERR "i2c-pq1: Unable to register with I2C\n");
		i2c_pq1_ret_free(i2c);
		free_irq(i2c->irq, NULL);
		kfree(i2c);
	}

	return ret;
}

static int i2c_pq1_remove(struct of_device* ofdev)
{
	struct m8xx_i2c *i2c = dev_get_drvdata(&ofdev->dev);

	i2c_pq1_shutdown(i2c);
	i2c_pq1_ret_free(i2c);
	free_irq(i2c->irq, NULL);
	dev_set_drvdata(&ofdev->dev, NULL);
	kfree(i2c);

	return 0;
}

static struct of_device_id i2c_pq1_match[] = {
	{
		.type = "i2c",
		.compatible = "fsl,i2c-cpm",
	},
	{},
};

MODULE_DEVICE_TABLE(of, i2c_pq1_match);

static struct of_platform_driver i2c_pq1_driver = {
	.name		= "fsl-i2c-cpm",
	.match_table	= i2c_pq1_match,
	.probe		= i2c_pq1_probe,
	.remove		= i2c_pq1_remove,
};

static int __init i2c_pq1_init(void)
{
	return of_register_platform_driver(&i2c_pq1_driver);
}

static void __exit i2c_pq1_exit(void)
{
	of_unregister_platform_driver(&i2c_pq1_driver);
}

module_init(i2c_pq1_init);
module_exit(i2c_pq1_exit);

MODULE_AUTHOR("Dan Malek <dmalek@jlc.net>");
MODULE_DESCRIPTION("I2C-Bus adapter routines for MPC8xx boards");
