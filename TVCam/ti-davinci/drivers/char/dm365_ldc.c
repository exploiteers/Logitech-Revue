
/*
 * Copyright (C) 2008 Texas Instruments Inc
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option)any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

/* dm365_ldc.c file */
#include <linux/config.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/completion.h>
#include <asm/uaccess.h>
#include <asm/semaphore.h>
#include <asm/arch/cpu.h>
#include <linux/platform_device.h>

#include <asm/arch/dm365_ldc.h>
#include <asm/arch/edma.h>
#include <asm/arch/vpss.h>

#define DRIVERNAME  "DaVinciLDC"

struct completion ldc_complete;
static int master_ch = 0;
/* global variable of type cdev to register driver to the kernel */
static struct cdev cdev;

struct device *ldc_dev;

/* ldc_device_s structure */
static struct ldc_device_s ldcdevice;
static struct ldc_device_s *ldcdev = &ldcdevice;

/* Functions */
int ldc_open(struct inode *inode, struct file *filp)
{
	struct ldc_fh *fh;

	dev_dbg(ldc_dev, __FUNCTION__ "E\n");
	if (filp->f_flags & O_NONBLOCK) {
		dev_err
		    (ldc_dev,
		     "ldc_open: device cannot be opened in non-blocked mode\n");
		return -EBUSY;
	}
	/* allocate memory for a the file handle */
	if ((fh = kmalloc(sizeof(struct ldc_fh), GFP_KERNEL)) == NULL) {
		return -ENOMEM;
	}

	down(&ldcdev->ldc_sem);

	ldcdev->users++;
	up(&ldcdev->ldc_sem);

	dev_dbg(ldc_dev, __FUNCTION__ "L\n");
	return 0;
}

int ldc_release(struct inode *inode, struct file *filp)
{
	/* get the configuratin from private_date member of file */
	struct ldc_fh *fh;

	dev_dbg(ldc_dev, __FUNCTION__ "E\n");

	down(&ldcdev->ldc_sem);
	fh = (struct ldc_fh *) filp->private_data;
	ldcdev->users--;
	kfree(filp->private_data);
	/* Assign null to private_data member of file and params 
	   member of device */
	filp->private_data = NULL;
	up(&ldcdev->ldc_sem);
	dev_dbg(ldc_dev, __FUNCTION__ "L\n");
	return 0;
}

static void ldc_platform_release(struct device *device)
{
	/* This is called when the reference count goes to zero */
}

static int __init ldc_probe(struct device *device)
{
	ldc_dev = device;
	return 0;
}

static int ldc_remove(struct device *device)
{
	return 0;
}

static int ldc_set_affine(struct device *device, 
													struct ldc_affine_param_s *arg)
{
	int ret = 0, flags;
	struct ldc_affine_param_s *aff = &ldcdevice.ldc_affine_param;
	dev_dbg(ldc_dev, __FUNCTION__ "E\n");

	if (ISNULL((void *)arg)) {
		dev_err(ldc_dev, "frame config arg ptr is null\n");
		return 1;
	}

	memcpy(aff, arg, sizeof(struct ldc_affine_param_s));

	spin_lock_irqsave(&ldcdev->lock, flags);
	ldc_regw(aff->affine_a, DAVINCI_LDC_AB);
	ldc_regm(aff->affine_b << LDC_AFFINE_SHIFT, 
					 0x3FFF << LDC_AFFINE_SHIFT,
					 DAVINCI_LDC_AB);
	ldc_regw(aff->affine_c, DAVINCI_LDC_CD);
	ldc_regm(aff->affine_d << LDC_AFFINE_SHIFT, 
					 0x3FFF << LDC_AFFINE_SHIFT,
					 DAVINCI_LDC_CD);
	ldc_regw(aff->affine_e, DAVINCI_LDC_EF);
	ldc_regm(aff->affine_f << LDC_AFFINE_SHIFT, 
					 0x3FFF << LDC_AFFINE_SHIFT,
					 DAVINCI_LDC_EF);
	spin_unlock_irqrestore(&ldcdev->lock, flags);

	dev_dbg(ldc_dev, __FUNCTION__ "L\n");
	return ret;
}

static void ldc_get_affine(struct device *device, 
													struct ldc_affine_param_s *arg)
{
	u32 ret, flags;
	dev_dbg(ldc_dev, __FUNCTION__ "E\n");

	if (ISNULL((void *)arg)) {
		dev_err(ldc_dev, "frame config arg ptr is null\n");
		return;
	}

	memset(arg, 0, sizeof(struct ldc_affine_param_s));
	spin_lock_irqsave(&ldcdev->lock, flags);
	ret = ldc_regr(DAVINCI_LDC_AB);

	arg->affine_a = ret & 0x3FFF;
	arg->affine_b = (ret >> 16) & 0x3FFF;
	
	ret = ldc_regr(DAVINCI_LDC_CD);
	arg->affine_c = ret & 0xFFFF;
	arg->affine_d = (ret >> 16) & 0x3FFF;

	ret = ldc_regr(DAVINCI_LDC_EF);
	spin_unlock_irqrestore(&ldcdev->lock, flags);
	arg->affine_e = ret & 0x3FFF;
	arg->affine_f = (ret >> 16) & 0xFFFF;

	dev_dbg(ldc_dev, __FUNCTION__ "L\n");
}

static int ldc_check_ob(struct ldc_config_s *cfg)
{
	int ret = 1;
	u8 obw = cfg->obw;
	u8 obh = cfg->obh;
	int bmode = cfg->bmode;
	
	if (ISNULL((void *)cfg)) {
		dev_err(ldc_dev, "frame config arg ptr is null\n");
		return -EFAULT;
	}

	if (obh & 0x1) {
		dev_dbg
		    (ldc_dev,
		     "OBH must be even\n");
		goto out;
	}

	switch (cfg->mode) {
	case LDC_MODE_YC422:
		if (obw % 16) {
			dev_dbg
		    (ldc_dev,
		     "OBW must be multiple of 16\n");
			goto out;
		}
		break;
	case LDC_MODE_YC420:
		if (obw % 32) {
			dev_dbg
		    (ldc_dev,
		     "OBW must be multiple of 16\n");
			goto out;
		}
		break;
	case LDC_MODE_BAYER_CAC:
		if (bmode == LDC_BMODE_U12 && (obw % 16)) {
			dev_dbg
		    (ldc_dev,
		     "OBW must be multiple of 16\n");
			goto out;
		} else if (bmode == LDC_BMODE_P12 && (obw % 64)) {
			dev_dbg
		    (ldc_dev,
		     "OBW must be multiple of 64\n");
			goto out;
		} else if ((bmode == LDC_BMODE_P8 || bmode == LDC_BMODE_A8) 
							 && (obw % 32)) {
			dev_dbg
		    (ldc_dev,
		     "OBW must be multiple of 32\n");
			goto out;
		}
		break;
	default:
		dev_dbg
		    (ldc_dev,
		     "Invalid mode!");
	}
	ret = 0;
	
 out:
	return ret;
}

static int ldc_check_tile(struct ldc_tile_param_s *tile)
{
	int ret = 1;
	u16 width = tile->frm_params.width;
	u16 height = tile->frm_params.height;
	struct ldc_config_s *ldccfg = &ldcdev->ldc_config;

	if ((tile->initX & 0x1) || (tile->initY & 0x1)) {
		dev_dbg(ldc_dev, "tile init position must be even\n");
		goto out;
	}

	if ((0 == width) || (0 == height)) {
		dev_dbg(ldc_dev, 
						"tile dimensions must be non-zero\n");
		goto out;
	}

	if ((width % ldccfg->obw) ||
			(height % ldccfg->obh)) {
		dev_dbg(ldc_dev, 
						"tile dimensions must be multiples of obw, obh\n");
		goto out;
	}
	
	if ((width < ldccfg->obw) || 
			(height < ldccfg->obh)) {
		dev_dbg(ldc_dev, 
						"tile dimensions must be greater than obw, obh\n");
		goto out;
	}

	if ((width >= (1 << 14)) || 
			(height >= (1 << 14))) {
		dev_dbg(ldc_dev, 
						"tile dimensions must be less than 2^14\n");
		goto out;
	}
	ret = 0;
 out:
	return ret;
}


static void ldc_set_tile(struct device *device, 
												struct ldc_tile_param_s *tile)
{
	u32 flags, base;

	dev_dbg(ldc_dev, __FUNCTION__ "E\n");

	if (ISNULL((void *)tile)) {
		dev_err(ldc_dev, "frame config arg ptr is null\n");
		return;
	}
	
	spin_lock_irqsave(&ldcdev->lock, flags);

	/* read/write frame base */
	base = (virt_to_phys((void*) tile->frm_params.readbase)
					- DAVINCI_DDR_BASE) >> 5;
	ldc_regw(base, DAVINCI_LDC_RD_BASE);

	base = (virt_to_phys((void*) tile->frm_params.writebase)
					- DAVINCI_DDR_BASE) >> 5;
	ldc_regw(base, DAVINCI_LDC_WR_BASE);

	/* 420C read/write base */
	if (ldcdev->ldc_config.mode == LDC_MODE_YC420) {
		base = (virt_to_phys((void*) tile->frm_params.readbase420c)
					- DAVINCI_DDR_BASE) >> 5;
		ldc_regw(base,  DAVINCI_LDC_420C_RD_BASE);

		base = (virt_to_phys((void*) tile->frm_params.writebase420c)
					- DAVINCI_DDR_BASE) >> 5;
		ldc_regw(base, DAVINCI_LDC_420C_WR_BASE);
	}

	/* read/write offset in 32-B cache line size */
	ldc_regw((tile->frm_params.readofst + 31) >> 5, 
					 DAVINCI_LDC_RD_OFST);
	ldc_regw((tile->frm_params.writeofst + 31) >> 5, 
					 DAVINCI_LDC_WR_OFST);

	/* initial pixel position */
	if (ldcdev->ldc_config.mode == LDC_SINGLE_PASS) {
		ldc_regw(0, DAVINCI_LDC_INITXY);
	} else {
		ldc_regw(tile->initX, DAVINCI_LDC_INITXY);
		ldc_regm(tile->initY << 16, 
						 0x3FFF << 16,
						 DAVINCI_LDC_INITXY);
	}

	/* input frame size */
	ldc_regw(tile->frm_params.width, 
					 DAVINCI_LDC_FRAME_SZ);
	ldc_regm(tile->frm_params.height << 16, 
					 0x3FFFF << 16,
					 DAVINCI_LDC_FRAME_SZ);
	spin_unlock_irqrestore(&ldcdev->lock, flags);

	dev_dbg(ldc_dev, __FUNCTION__ "L\n");
	return;
}

static void ldc_calc_multitiles(struct ldc_tile_param_s *arg)
{
	u16 resX, resY;
	u16 skipw, skiph;
	struct ldc_tile_param_s *tile;
	struct ldc_frame_param_s *frm = &arg->frm_params;
	u32 yuv420 = ((ldcdev->ldc_config.mode == LDC_MODE_YC420)?
								1 : 0);
	
	dev_dbg(ldc_dev, __FUNCTION__ "E\n");

	resX = frm->width;
	resY = frm->height;
	skipw = ldcdev->ldc_config.skipWidth;
	skiph = ldcdev->ldc_config.skipHeight;

	/* tile I */
	tile = &ldcdevice.ldc_tiles[0];

	tile->initX = tile->initY = 0;
	tile->frm_params.width = (resX >> 1) - (skipw >> 1);
	tile->frm_params.height = (resY >> 1) - (skiph >> 1);
	tile->frm_params.readbase = 0;
	tile->frm_params.readofst = 0;
	tile->frm_params.writebase = 0;
	tile->frm_params.writeofst = 0;
	if (yuv420) {
		tile->frm_params.readbase420c = 0;
		tile->frm_params.writebase420c = 0;
	}
	/* tile II */
	tile = &ldcdevice.ldc_tiles[1];
	
	tile->initX = (resX >> 1) - (skipw >> 1);
	tile->initY = 0;
	tile->frm_params.width = skipw;
	tile->frm_params.height = (resY >> 1) - (skiph >> 1);
	tile->frm_params.readbase = 0;
	tile->frm_params.readofst = 0;
	tile->frm_params.writebase = 0;
	tile->frm_params.writeofst = 0;
	if (yuv420) {
		tile->frm_params.readbase420c = 0;
		tile->frm_params.writebase420c = 0;
	}
	/* tile III */
	tile = &ldcdevice.ldc_tiles[2];
	
	tile->initX = ldcdevice.ldc_tiles[0].frm_params.width + skipw;
	tile->initY = 0;
	tile->frm_params.width = resX - skipw - 
		ldcdevice.ldc_tiles[0].frm_params.width;
	tile->frm_params.height = resY;
	tile->frm_params.readbase = 0;
	tile->frm_params.readofst = 0;
	tile->frm_params.writebase = 0;
	tile->frm_params.writeofst = 0;
	if (yuv420) {
		tile->frm_params.readbase420c = 0;
		tile->frm_params.writebase420c = 0;
	}
	/* tile IV */
	tile = &ldcdevice.ldc_tiles[3];
	
	tile->initX = ldcdevice.ldc_tiles[0].frm_params.width;
	tile->initY = (resY >> 1) - (skiph >> 1);
	tile->frm_params.width = skipw;
	tile->frm_params.height = resY - skiph - 
		ldcdevice.ldc_tiles[1].frm_params.height;
	tile->frm_params.readbase = 0;
	tile->frm_params.readofst = 0;
	tile->frm_params.writebase = 0;
	tile->frm_params.writeofst = 0;
	if (yuv420) {
		tile->frm_params.readbase420c = 0;
		tile->frm_params.writebase420c = 0;
	}

	/* tile V */
	tile = &ldcdevice.ldc_tiles[4];
	
	tile->initX = ldcdevice.ldc_tiles[0].frm_params.width;
	tile->initY = (resY >> 1) - (skiph >> 1);
	tile->frm_params.width = skipw;
	tile->frm_params.height = skiph;
	tile->frm_params.readbase = 0;
	tile->frm_params.readofst = 0;
	tile->frm_params.writebase = 0;
	tile->frm_params.writeofst = 0;
	if (yuv420) {
		tile->frm_params.readbase420c = 0;
		tile->frm_params.writebase420c = 0;
	}
	dev_dbg(ldc_dev, __FUNCTION__ "L\n");

}

static void ldc_access_lut(struct ldc_config_s *cfg, 
													 int set)
{
	int i;

	if (ISNULL((void *)cfg)) {
		dev_err(ldc_dev, "ldc cfg ptr is null\n");
		return;
	}
	
	ldc_regw(0, DAVINCI_LDC_LUT_ADDR);
	if (set)
		for (i = 0; i<256; i++)
			ldc_regw(cfg->lut[i], DAVINCI_LDC_LUT_WDATA);
	else
		for (i = 0; i<256; i++)
			cfg->lut[i] = ldc_regr(DAVINCI_LDC_LUT_RDATA);

}

static void ldc_get_config(struct device *device, 
													struct ldc_config_s *arg)
{
	u32 ret, flags;

	dev_dbg(ldc_dev, __FUNCTION__ "E\n");

	if (ISNULL((void *)arg)) {
		dev_err(ldc_dev, "ldc cfg ptr is null\n");
		return;
	}
	
	memset(arg, 0, sizeof(struct ldc_config_s));

	spin_lock_irqsave(&ldcdev->lock, flags);

	/* lens center */
	ret = ldc_regr(DAVINCI_LDC_CENTER);
	arg->lens_cntr_x = ret & 0x3FFF;
	arg->lens_cntr_y = (ret >> 16) & 0x3FFF;

	/* set LUT */
	ldc_access_lut(arg, 0);

	/* LDC_CONFIG */
	ret = ldc_regr(DAVINCI_LDC_CONFIG);
	arg->initc = (ret >> 4) & 0x3;
	arg->yint_type = (ret >> 6) & 0x1;
	arg->rth = (ret >> 16) & 0x3FFF;

	/* LDC_KHV */
	ret = ldc_regr(DAVINCI_LDC_KHV);
	arg->khl = ret & 0xFF;
	arg->khr = (ret >> 8) & 0xFF;
	arg->kvu = (ret >> 16) & 0xFF;
	arg->khr = (ret >> 24) & 0xFF;

	/* LDC_BLOCK */
	ret = ldc_regr(DAVINCI_LDC_BLOCK);
	arg->obw = ret & 0xFF;
	arg->obh = (ret >> 8) & 0xFF;
	arg->pixpad = (ret >> 16) & 0xFF;
	
	/* LDC_PCR */
	ret = ldc_regr(DAVINCI_LDC_PCR);
	arg->mode = (ret >> 3) & 0x3;
	arg->bmode = (ret >> 5) & 0x3;

	spin_unlock_irqrestore(&ldcdev->lock, flags);
	dev_dbg(ldc_dev, __FUNCTION__ "L\n");
	
}

static int ldc_set_config(struct device *device, 
													struct ldc_config_s *arg)
{
	int ret = 0;
	struct ldc_config_s *ldccfg = &ldcdev->ldc_config;
	u32 flags, cfgw = 0;

	dev_dbg(ldc_dev, __FUNCTION__ "E\n");

	if (ISNULL((void *)arg)) {
		dev_err(ldc_dev, "ldc cfg ptr is null\n");
		return 1;
	}

	memcpy(ldccfg, arg, sizeof(struct ldc_config_s));

	if (ldccfg->pass > LDC_MULTI_PASS)
		return 1;

	if (ldc_check_ob(ldccfg))
		return 1;

	spin_lock_irqsave(&ldcdev->lock, flags);
	/* lens center */
	cfgw = (ldccfg->lens_cntr_x & 0x3FFF);
	cfgw |= ((ldccfg->lens_cntr_y & 0x3FFF) << 16);
	ldc_regw(cfgw, DAVINCI_LDC_CENTER);

	/* init LUT */
	ldc_access_lut(ldccfg, 1);

	/* LDC_CONFIG register */
	cfgw = (ldccfg->tbits & 0xF);
	cfgw |= ((ldccfg->initc & 0x3) << 4);
	cfgw |= ((ldccfg->yint_type & 0x1) << 6);
	cfgw |= ((ldccfg->rth & 0x3FFF) << 16);
	ldc_regw(cfgw, DAVINCI_LDC_CONFIG);

	/* LDC_KHV */
	cfgw = (ldccfg->khl & 0xFF);
	cfgw |= ((ldccfg->khr & 0xFF) << 8);
	cfgw |= ((ldccfg->kvu & 0xFF) << 16);
	cfgw |= ((ldccfg->kvl & 0xFF) << 24);
	ldc_regw(cfgw, DAVINCI_LDC_KHV);

	/* LDC_BLOCK */
	cfgw = (ldccfg->obw & 0xFF);
	cfgw |= ((ldccfg->obh & 0xFF) << 8);
	cfgw |= ((ldccfg->pixpad & 0xF) << 16);
	ldc_regw(cfgw, DAVINCI_LDC_BLOCK);
	
	/* LDC_PCR */
	cfgw = ((ldccfg->mode & 0x3) << 3);
	cfgw |= ((ldccfg->bmode & 0x3) << 5);
	ldc_regw(cfgw, DAVINCI_LDC_PCR);
	
	spin_unlock_irqrestore(&ldcdev->lock, flags);
	dev_dbg(ldc_dev, __FUNCTION__ "L\n");
	return ret;
}

int complete_cnt;

static irqreturn_t ldc_isr(int irq, void *device_id,
				  struct pt_regs *regs)
{

	if (LDC_MULTI_PASS == ldcdev->ldc_config.pass) {
		complete_cnt++;
		if (complete_cnt == 4) {
			/* DMA tile 5 to the output */
		} else {
			/* set next tile */
			ldc_set_tile(ldc_dev, &ldcdev->ldc_tiles[complete_cnt]);
			/* kick off LDC */
			ldc_regm(1, 1, DAVINCI_LDC_PCR);
		}
	} else 
		complete(&ldc_complete);

	return IRQ_HANDLED;
}

static void ldc_dma_isr(int lch, u16 ch_status, void *data)
{
	davinci_stop_dma(lch);
	complete_cnt = 0;
  complete(&ldc_complete);
}

static int ldc_doioctl(struct inode *inode, struct file *file,
											 unsigned int cmd, unsigned long arg)
{
	int ret = -EINVAL;

	if (ISNULL((void *)arg)) {
		dev_err(ldc_dev, "arg ptr is null\n");
		ret = -EFAULT;
		goto out;
	}

	dev_dbg(ldc_dev, __FUNCTION__ "E\n");

	switch (cmd) {
	case LDC_SET_FRMDATA:
		{
			dev_dbg(ldc_dev, "LDC_SET_FRMDATA:\n");
			if (LDC_SINGLE_PASS == ldcdev->ldc_config.pass){
				struct ldc_tile_param_s *tile;
				tile = &ldcdevice.ldc_tiles[0];
				memcpy(&tile->frm_params, (void*)arg,
							 sizeof(struct ldc_frame_param_s));
				if (ldc_check_tile(tile))
					goto out;
				ldc_set_tile(ldc_dev, tile);
			}
			else {
				struct ldc_tile_param_s tile;
				u32 i;
				memcpy(&tile.frm_params, (void*)arg,
							 sizeof(struct ldc_frame_param_s));

				/* compute tiles and stored in ldcdevice */
				ldc_calc_multitiles(&tile);

				/* check tiles 0 to 3 */
				for (i = 0; i < 4; i++) {
					if (ldc_check_tile(&ldcdev->ldc_tiles[i])) {
						dev_err(ldc_dev, "tile [%d] frame params bad\n", i);
						goto out;
					}
				}
					
				/* load tile 1 frame data */
				ldc_set_tile(ldc_dev, &ldcdev->ldc_tiles[0]);
				/* reset count */
				complete_cnt = 0;
			}
		}
		break;
	case LDC_GET_FRMDATA:
		{
			dev_dbg(ldc_dev, "LDC_GET_FRMDATA:\n");
			memcpy((void*)arg, 
						 ldcdev->ldc_tiles, 
						 sizeof(struct ldc_tile_param_s));
		}
		break;
	case LDC_SET_CONFIG:
		{
			
			dev_dbg(ldc_dev, "LDC_SET_CONFIG:\n");

			if (ldc_set_config(ldc_dev,
												 (struct ldc_config_s*) arg))
				goto out;
			/* need DMA channel */
			if (LDC_MULTI_PASS == ldcdev->ldc_config.pass) {
				int tcc = EDMA_TCC_ANY;
				ret = davinci_request_dma(EDMA_DMA_CHANNEL_ANY, DRIVERNAME,
                       ldc_dma_isr, ldc_dev, &master_ch,
                       &tcc, EVENTQ_0);
			}
		}
		break;
	case LDC_GET_CONFIG:
		{
			dev_dbg(ldc_dev, "LDC_GET_CONFIG:\n");
			ldc_get_config(ldc_dev, (struct ldc_config_s *)arg);
		}
		break;
	case LDC_SET_AFFINE:
		{
			dev_dbg(ldc_dev, "LDC_SET_AFFINE:\n");
			if (ldc_set_affine(ldc_dev, 
												 (struct ldc_affine_param_s *)arg));
				return -EFAULT;
		}
		break;
	case LDC_GET_AFFINE:
		{
			dev_dbg(ldc_dev, "LDC_GET_AFFINE:\n");
			ldc_get_affine(ldc_dev, 
										 (struct ldc_affine_param_s *)arg);
		}
		break;
	case LDC_START:
		{
			dev_dbg(ldc_dev, "LDC_START:\n");
			if (vpss_request_irq(VPSS_LDC_INT, ldc_isr, SA_INTERRUPT,
													 DRIVERNAME, (void*) NULL)) {
				ret = -EFAULT;
				goto out;
			}

			/* kick off LDC */
			ldc_regm(1, 1, DAVINCI_LDC_PCR);

			/* Waiting for LDC to complete */
			wait_for_completion_interruptible(&ldc_complete);
			vpss_free_irq(VPSS_LDC_INT, (void*) NULL);
		}
		break;
	default:
		dev_err(ldc_dev, "previewer_ioctl: Invalid Command Value\n");
		goto out;
	}
	ret = 0;
 out:	
	dev_dbg(ldc_dev, __FUNCTION__ "L\n");
	return ret;
}

static int ldc_ioctl(struct inode *inode, struct file *file,
			   unsigned int cmd, unsigned long arg)
{
	int ret;
	char sbuf[128];
	void *mbuf = NULL;
	void *parg = NULL;

	dev_dbg(ldc_dev, "Start of ldc ioctl\n");

	/*  Copy arguments into temp kernel buffer  */
	switch (_IOC_DIR(cmd)) {
	case _IOC_NONE:
		parg = NULL;
		break;
	case _IOC_READ:
	case _IOC_WRITE:
	case (_IOC_WRITE | _IOC_READ):
		if (_IOC_SIZE(cmd) <= sizeof(sbuf)) {
			parg = sbuf;
		} else {
			/* too big to allocate from stack */
			mbuf = kmalloc(_IOC_SIZE(cmd), GFP_KERNEL);
			if (ISNULL(mbuf))
				return -ENOMEM;
			parg = mbuf;
		}

		ret = -EFAULT;
		if (_IOC_DIR(cmd) & _IOC_WRITE)
			if (copy_from_user(parg, (void __user *)arg,
					   _IOC_SIZE(cmd)))
				goto out;
		break;
	}

	/* call driver */
	ret = ldc_doioctl(inode, file, cmd, (unsigned long)parg);
	if (ret == -ENOIOCTLCMD)
		ret = -EINVAL;

	/*  Copy results into user buffer  */
	switch (_IOC_DIR(cmd)) {
	case _IOC_READ:
	case (_IOC_WRITE | _IOC_READ):
		if (copy_to_user((void __user *)arg, parg, _IOC_SIZE(cmd)))
			ret = -EFAULT;
		break;
	}
 out:
	if (mbuf)
		kfree(mbuf);

	dev_dbg(ldc_dev, "End of ldc ioctl\n");
	return ret;
}



/* global variable of type file_operations containing function 
pointers of file operations */
static struct file_operations ldc_fops = {
	.owner = THIS_MODULE,
	.open = ldc_open,
	.release = ldc_release,
	.ioctl = ldc_ioctl,
};


/* global variable which keeps major and minor number of the driver in it */
static dev_t devnum;

static struct platform_device ldc_device = {
	.name = "davinci_ldc",
	.id = 0,
	.dev = {
		.release = ldc_platform_release,
	}
};

static struct device_driver ldc_driver = {
	.name = "davinci_ldc",
	.bus = &platform_bus_type,
	.probe = ldc_probe,
	.remove = ldc_remove
};

static struct class *ldc_class = NULL;

int __init ldc_init(void)
{
	int result;
	
	/* Register the driver in the kernel */
	/* dynmically get the major number for the driver using 
	   alloc_chrdev_region function */
	result = alloc_chrdev_region(&devnum, 0, 1, DRIVERNAME);

	/* if it fails return error */
	if (result < 0) {
		printk(KERN_ERR "DaVinciLDC: Module intialization \
                failed. could not register character device\n");
		return -ENODEV;
	}

	/* initialize cdev with file operations */
	cdev_init(&cdev, &ldc_fops);
	cdev.owner = THIS_MODULE;
	cdev.ops = &ldc_fops;

	/* add cdev to the kernel */
	result = cdev_add(&cdev, devnum, 1);

	if (result) {
		unregister_chrdev_region(devnum, 1);
		printk(KERN_ERR
		       "DaVinciLDC: Error adding DavinciLDC .. error no:%d\n",
		       result);
		return -EINVAL;
	}

	/* register driver as a platform driver */
	if (driver_register(&ldc_driver) != 0) {
		unregister_chrdev_region(devnum, 1);
		cdev_del(&cdev);
		return -EINVAL;
	}

	/* Register the drive as a platform device */
	if (platform_device_register(&ldc_device) != 0) {
		driver_unregister(&ldc_driver);
		unregister_chrdev_region(devnum, 1);
		cdev_del(&cdev);
		return -EINVAL;
	}

	ldc_class = class_create(THIS_MODULE, "davinci_ldc");

	if (!ldc_class) {
		printk(KERN_NOTICE
		       "ldc_init: error in creating device class\n");
		driver_unregister(&ldc_driver);
		platform_device_unregister(&ldc_device);
		unregister_chrdev_region(devnum, 1);
		cdev_del(&cdev);
		return -EIO;
	}

	class_device_create(ldc_class, NULL, devnum, NULL, "davinci_ldc");

	init_MUTEX(&(ldcdevice.ldc_sem));
	spin_lock_init(&ldcdev->lock);
	printk(KERN_NOTICE "davinci_ldc initialized\n");
	return 0;
}

void __exit ldc_cleanup(void)
{
	/* remove major number allocated to this driver */
	unregister_chrdev_region(devnum, 1);

	/* remove simple class device */
	class_device_destroy(ldc_class, devnum);

	/* destroy simple class */
	class_destroy(ldc_class);

	/* Remove platform driver */
	driver_unregister(&ldc_driver);

	/* remove platform device */
	platform_device_unregister(&ldc_device);

	cdev_del(&cdev);

}

module_init(ldc_init)
module_exit(ldc_cleanup)
MODULE_LICENSE("GPL");
