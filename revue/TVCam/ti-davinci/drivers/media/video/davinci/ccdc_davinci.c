/*
 * Copyright (C) 2006-2009 Texas Instruments Inc
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

#include <linux/device.h>
#include <asm/page.h>
#include <asm/uaccess.h>
#include <media/davinci/ccdc_davinci.h>
#include <media/davinci/ccdc_hw_if.h>

/*Object for CCDC raw mode */
static struct ccdc_params_raw ccdc_hw_params_raw = {
	.pix_fmt = CCDC_PIXFMT_RAW,
	.frm_fmt = CCDC_FRMFMT_PROGRESSIVE,
	.win = VPFE_WIN_VGA,
	.fid_pol = CCDC_PINPOL_POSITIVE,
	.vd_pol = CCDC_PINPOL_POSITIVE,
	.hd_pol = CCDC_PINPOL_POSITIVE,
	.image_invert_enable = 0,
	.data_sz = _10BITS,
	.alaw = {
		.b_alaw_enable = 0
	},
	.blk_clamp = {
		.b_clamp_enable = 0,
		.dc_sub = 0
	},
	.blk_comp = {0, 0, 0, 0},
	.fault_pxl = {
		.fpc_enable = 0
	},
};

/*Object for CCDC ycbcr mode */
static struct ccdc_params_ycbcr ccdc_hw_params_ycbcr = {
	.pix_fmt = CCDC_PIXFMT_YCBCR_8BIT,
	.frm_fmt = CCDC_FRMFMT_INTERLACED,
	.win = VPFE_WIN_PAL,
	.fid_pol = CCDC_PINPOL_POSITIVE,
	.vd_pol = CCDC_PINPOL_POSITIVE,
	.hd_pol = CCDC_PINPOL_POSITIVE,
	.bt656_enable = 1,
	.pix_order = CCDC_PIXORDER_CBYCRY,
	.buf_type = CCDC_BUFTYPE_FLD_INTERLEAVED
};

static struct ccdc_config_params_raw ccdc_hw_params_raw_temp;
static enum ccdc_hw_if_type ccdc_if_type;
static struct ccdc_std_info ccdc_video_config;
static unsigned long ccdc_base_addr;
static unsigned long vpss_sb_base_addr;

/* register access routines */
static inline u32 regr(u32 offset)
{
    return davinci_readl(ccdc_base_addr + offset);
}

static inline u32 regw(u32 val, u32 offset)
{
    davinci_writel(val, ccdc_base_addr + offset);
    return val;
}

/* register access routines */
static inline u32 regr_sb(u32 offset)
{
    return davinci_readl(vpss_sb_base_addr + offset);
}

static inline u32 regw_sb(u32 val, u32 offset)
{
    davinci_writel(val, vpss_sb_base_addr + offset);
    return val;
}

static void ccdc_enable(int flag)
{
	regw(flag, PCR);
}

static void ccdc_enable_vport(int flag)
{
	if (flag)
		/* enable video port */
		regw(ENABLE_VIDEO_PORT, FMTCFG);
	else
		regw(DISABLE_VIDEO_PORT, FMTCFG);
}

/*
 * ======== ccdc_setwin  ========
 * This function will configure the window size
 * to be capture in CCDC reg
 */
void ccdc_setwin(struct ccdc_imgwin *image_win,
		enum ccdc_frmfmt frm_fmt,
		int ppc)
{
	int horz_start, horz_nr_pixels;
	int vert_start, vert_nr_lines;
	int val = 0, mid_img = 0;
	printk(KERN_DEBUG "\nStarting ccdc_setwin...");
	/* configure horizonal and vertical starts and sizes */
	/* Here, (ppc-1) will be different for raw and yuv modes */
	horz_start = image_win->left << (ppc - 1);
	horz_nr_pixels = (image_win->width << (ppc - 1)) - 1;
	regw((horz_start << CCDC_HORZ_INFO_SPH_SHIFT) | horz_nr_pixels,
	     HORZ_INFO);

	vert_start = image_win->top;

	if (frm_fmt == CCDC_FRMFMT_INTERLACED) {
		vert_nr_lines = (image_win->height >> 1) - 1;
		vert_start >>= 1;
		/* Since first line doesn't have any data */
		vert_start += 1;
		/* configure VDINT0 */
		val = (vert_start << CCDC_VDINT_VDINT0_SHIFT);
	} else {
		/* Since first line doesn't have any data */
		vert_start += 1;
		vert_nr_lines = image_win->height - 1;
		/* configure VDINT0 and VDINT1 */
		/* VDINT1 will be at half of image height */
		mid_img = vert_start + (image_win->height / 2);
		val = (vert_start << CCDC_VDINT_VDINT0_SHIFT) |
		    (mid_img & CCDC_VDINT_VDINT1_MASK);
	}
	regw(val, VDINT);
	regw((vert_start << CCDC_VERT_START_SLV0_SHIFT) | vert_start,
	     VERT_START);
	regw(vert_nr_lines, VERT_LINES);
	printk(KERN_DEBUG "\nEnd of ccdc_setwin...");
}

static void ccdc_readregs(void)
{
	unsigned int val = 0;

	val = regr(ALAW);
	printk(KERN_NOTICE "\nReading 0x%x to ALAW...\n", val);
	val = regr(CLAMP);
	printk(KERN_NOTICE "\nReading 0x%x to CLAMP...\n", val);
	val = regr(DCSUB);
	printk(KERN_NOTICE "\nReading 0x%x to DCSUB...\n", val);
	val = regr(BLKCMP);
	printk(KERN_NOTICE "\nReading 0x%x to BLKCMP...\n", val);
	val = regr(FPC_ADDR);
	printk(KERN_NOTICE "\nReading 0x%x to FPC_ADDR...\n", val);
	val = regr(FPC);
	printk(KERN_NOTICE "\nReading 0x%x to FPC...\n", val);
	val = regr(FMTCFG);
	printk(KERN_NOTICE "\nReading 0x%x to FMTCFG...\n", val);
	val = regr(COLPTN);
	printk(KERN_NOTICE "\nReading 0x%x to COLPTN...\n", val);
	val = regr(FMT_HORZ);
	printk(KERN_NOTICE "\nReading 0x%x to FMT_HORZ...\n", val);
	val = regr(FMT_VERT);
	printk(KERN_NOTICE "\nReading 0x%x to FMT_VERT...\n", val);
	val = regr(HSIZE_OFF);
	printk(KERN_NOTICE "\nReading 0x%x to HSIZE_OFF...\n", val);
	val = regr(SDOFST);
	printk(KERN_NOTICE "\nReading 0x%x to SDOFST...\n", val);
	val = regr(VP_OUT);
	printk(KERN_NOTICE "\nReading 0x%x to VP_OUT...\n", val);
	val = regr(SYN_MODE);
	printk(KERN_NOTICE "\nReading 0x%x to SYN_MODE...\n", val);
	val = regr(HORZ_INFO);
	printk(KERN_NOTICE "\nReading 0x%x to HORZ_INFO...\n", val);
	val = regr(VERT_START);
	printk(KERN_NOTICE "\nReading 0x%x to VERT_START...\n", val);
	val = regr(VERT_LINES);
	printk(KERN_NOTICE "\nReading 0x%x to VERT_LINES...\n", val);
}

static int validate_ccdc_param(struct ccdc_config_params_raw *ccdcparam)
{
	if ((ccdc_hw_params_raw.frm_fmt != CCDC_FRMFMT_INTERLACED)
	    && (ccdcparam->image_invert_enable == 1)) {
		printk(KERN_ERR "\nImage invert not supported");
		return -1;
	}
	if (ccdc_hw_params_raw.alaw.b_alaw_enable) {
		if ((ccdcparam->alaw.gama_wd > BITS_09_0)
		    || (ccdcparam->alaw.gama_wd < BITS_15_6)
		    || (ccdcparam->alaw.gama_wd < ccdcparam->data_sz)) {
			printk(KERN_ERR "\nInvalid data line select");
			return -1;
		}
	}
	return 0;
}

static int ccdc_update_ycbcr_params(void *arg)
{
	memcpy(&ccdc_hw_params_ycbcr,
	       (struct ccdc_params_ycbcr *)arg,
	       sizeof(struct ccdc_params_ycbcr));
	return 0;
}

static int ccdc_update_raw_params(void *arg)
{
	unsigned int *fpc_virtaddr = NULL;
	unsigned int *fpc_physaddr = NULL;
	struct ccdc_params_raw *ccd_params = &ccdc_hw_params_raw;
	struct ccdc_config_params_raw *raw_params =
			(struct ccdc_config_params_raw *) arg;
	ccd_params->image_invert_enable = raw_params->image_invert_enable;

	printk(KERN_DEBUG "\nimage_invert_enable = %d",
	       ccd_params->image_invert_enable);

	ccd_params->data_sz = raw_params->data_sz;
	printk(KERN_DEBUG "\ndata_sz = %d", ccd_params->data_sz);

	ccd_params->alaw.b_alaw_enable = raw_params->alaw.b_alaw_enable;
	printk(KERN_DEBUG "\nALaw Enable = %d", ccd_params->alaw.b_alaw_enable);
	/* copy A-Law configurations to vpfe_device, from arg
	 * passed by application */
	if (ccd_params->alaw.b_alaw_enable) {
		ccd_params->alaw.gama_wd = raw_params->alaw.gama_wd;
		printk(KERN_DEBUG "\nALaw Gama width = %d",
		       ccd_params->alaw.gama_wd);
	}

	/* copy Optical Balck Clamping configurations to
	 * vpfe_device,from arg passed by application */
	ccd_params->blk_clamp.b_clamp_enable
	    = raw_params->blk_clamp.b_clamp_enable;
	printk(KERN_DEBUG "\nb_clamp_enable = %d",
	       ccd_params->blk_clamp.b_clamp_enable);
	if (ccd_params->blk_clamp.b_clamp_enable) {
		/*gain */
		ccd_params->blk_clamp.sgain = raw_params->blk_clamp.sgain;
		printk(KERN_DEBUG "\nblk_clamp.sgain = %d",
		       ccd_params->blk_clamp.sgain);
		/*Start pixel */
		ccd_params->blk_clamp.start_pixel
		    = raw_params->blk_clamp.start_pixel;
		printk(KERN_DEBUG "\nblk_clamp.start_pixel = %d",
		       ccd_params->blk_clamp.start_pixel);
		/*No of line to be avg */
		ccd_params->blk_clamp.sample_ln
		    = raw_params->blk_clamp.sample_ln;
		printk(KERN_DEBUG "\nblk_clamp.sample_ln = %d",
		       ccd_params->blk_clamp.sample_ln);
		/*No of pixel/line to be avg */
		ccd_params->blk_clamp.sample_pixel
		    = raw_params->blk_clamp.sample_pixel;
		printk(KERN_DEBUG "\nblk_clamp.sample_pixel  = %d",
		       ccd_params->blk_clamp.sample_pixel);
	} else {		/* configure DCSub */

		ccd_params->blk_clamp.dc_sub = raw_params->blk_clamp.dc_sub;
		printk(KERN_DEBUG "\nblk_clamp.dc_sub  = %d",
		       ccd_params->blk_clamp.dc_sub);
	}

	/* copy BalckLevel Compansation configurations to
	 * vpfe_device,from arg passed by application
	 */
	ccd_params->blk_comp.r_comp = raw_params->blk_comp.r_comp;
	ccd_params->blk_comp.gr_comp = raw_params->blk_comp.gr_comp;
	ccd_params->blk_comp.b_comp = raw_params->blk_comp.b_comp;
	ccd_params->blk_comp.gb_comp = raw_params->blk_comp.gb_comp;
	printk(KERN_DEBUG "\nblk_comp.r_comp   = %d",
	       ccd_params->blk_comp.r_comp);
	printk(KERN_DEBUG "\nblk_comp.gr_comp  = %d",
	       ccd_params->blk_comp.gr_comp);
	printk(KERN_DEBUG "\nblk_comp.b_comp   = %d",
	       ccd_params->blk_comp.b_comp);
	printk(KERN_DEBUG "\nblk_comp.gb_comp  = %d",
	       ccd_params->blk_comp.gb_comp);

	/* copy FPC configurations to vpfe_device,from
	 * arg passed by application
	 */
	ccd_params->fault_pxl.fpc_enable = raw_params->fault_pxl.fpc_enable;
	printk(KERN_DEBUG "\nfault_pxl.fpc_enable  = %d",
	       ccd_params->fault_pxl.fpc_enable);

	if (ccd_params->fault_pxl.fpc_enable) {
		fpc_physaddr =
		    (unsigned int *)ccd_params->fault_pxl.fpc_table_addr;

		fpc_virtaddr = (unsigned int *)
		    phys_to_virt((unsigned long)
				 fpc_physaddr);

		/* Allocate memory for FPC table if current
		 * FPC table buffer is not big enough to
		 * accomodate FPC Number requested
		 */
		if (raw_params->fault_pxl.fp_num !=
		    ccd_params->fault_pxl.fp_num) {
			if (fpc_physaddr != NULL) {
				free_pages((unsigned long)
					   fpc_physaddr,
					   get_order
					   (ccd_params->
					    fault_pxl.fp_num * FP_NUM_BYTES));

			}

			/* Allocate memory for FPC table */
			fpc_virtaddr = (unsigned int *)
			    __get_free_pages(GFP_KERNEL |
					     GFP_DMA,
					     get_order
					     (raw_params->
					      fault_pxl.fp_num * FP_NUM_BYTES));

			if (fpc_virtaddr == NULL) {
				printk(KERN_ERR
				       "\n Unable to allocate memory for FPC");
				return -1;
			}
			fpc_physaddr =
			    (unsigned int *)virt_to_phys((void *)fpc_virtaddr);
		}

		/* Copy number of fault pixels and FPC table */
		ccd_params->fault_pxl.fp_num = raw_params->fault_pxl.fp_num;
		copy_from_user((void *)fpc_virtaddr,
			       (void *)raw_params->
			       fault_pxl.fpc_table_addr,
			       (unsigned long)ccd_params->
			       fault_pxl.fp_num * FP_NUM_BYTES);

		ccd_params->fault_pxl.fpc_table_addr =
		    (unsigned int)fpc_physaddr;
	}
	return 0;
}

static int ccdc_deinitialize(void)
{
	unsigned int *fpc_physaddr = NULL, *fpc_virtaddr = NULL;
	fpc_physaddr = (unsigned int *)
	    ccdc_hw_params_raw.fault_pxl.fpc_table_addr;

	if (fpc_physaddr != NULL) {
		fpc_virtaddr = (unsigned int *)
		    phys_to_virt((unsigned long)fpc_physaddr);
		free_pages((unsigned long)fpc_virtaddr,
			   get_order(ccdc_hw_params_raw.fault_pxl.
				     fp_num * FP_NUM_BYTES));
	}
	return 0;
}

/*
 * ======== ccdc_reset  ========
 *
 * This function will reset all CCDc reg
 */
void ccdc_reset(void)
{
	int i;

	/* disable CCDC */
	ccdc_enable(0);
	/* set all registers to default value */
	for (i = 0; i <= 0x94; i += 4) {
		regw(0, i);
	}
	regw(0, PCR);
	regw(0, SYN_MODE);
	regw(0, HD_VD_WID);
	regw(0, PIX_LINES);
	regw(0, HORZ_INFO);
	regw(0, VERT_START);
	regw(0, VERT_LINES);
	regw(0xffff00ff, CULLING);
	regw(0, HSIZE_OFF);
	regw(0, SDOFST);
	regw(0, SDR_ADDR);
	regw(0, VDINT);
	regw(0, REC656IF);
	regw(0, CCDCFG);
	regw(0, FMTCFG);
	regw(0, VP_OUT);
}

static int ccdc_initialize(void)
{
	ccdc_reset();
	if (ccdc_if_type == CCDC_RAW_BAYER)
		ccdc_enable_vport(1);
	return 0;
}

static u32 ccdc_sbl_reset(void)
{
	int sb_reset;
	sb_reset = regr_sb(SBL_PCR_VPSS);
	regw_sb((sb_reset & SBL_PCR_CCDC_WBL_O), SBL_PCR_VPSS);
	return sb_reset;
}

/* Parameter operations */
static int ccdc_setparams(void *params)
{
	int x;
	if (ccdc_if_type == CCDC_RAW_BAYER) {
		x = copy_from_user(&ccdc_hw_params_raw_temp,
				   (struct ccdc_config_params_raw *)params,
				   sizeof(struct ccdc_config_params_raw));
		if (x) {
			printk(KERN_ERR "ccdc_setparams: error in copying"
				   "ccdc params, %d\n", x);
			return -EFAULT;
		}

		if (!validate_ccdc_param(&ccdc_hw_params_raw_temp)) {
			if (!ccdc_update_raw_params(&ccdc_hw_params_raw_temp))
				return 0;
		}
	} else {
		return (ccdc_update_ycbcr_params(params));
	}
	return -EINVAL;
}

/*
 * ======== ccdc_config_ycbcr  ========
 * This function will configure CCDC for YCbCr parameters
 */
void ccdc_config_ycbcr(void)
{
	u32 syn_mode;
	unsigned int val;
	struct ccdc_params_ycbcr *params = &ccdc_hw_params_ycbcr;

	/* first reset the CCDC                                          */
	/* all registers have default values after reset                 */
	/* This is important since we assume default values to be set in */
	/* a lot of registers that we didn't touch                       */
	printk(KERN_DEBUG "\nStarting ccdc_config_ycbcr...");
	ccdc_reset();

	/* configure pixel format */
	syn_mode = (params->pix_fmt & 0x3) << 12;

	/* configure video frame format */
	syn_mode |= (params->frm_fmt & 0x1) << 7;

	/* setup BT.656 sync mode */
	if (params->bt656_enable) {
		regw(3, REC656IF);

		/* configure the FID, VD, HD pin polarity */
		/* fld,hd pol positive, vd negative, 8-bit pack mode */
		syn_mode |= 0x00000F04;
	} else {		/* y/c external sync mode */
		syn_mode |= ((params->fid_pol & 0x1) << 4);
		syn_mode |= ((params->hd_pol & 0x1) << 3);
		syn_mode |= ((params->vd_pol & 0x1) << 2);
	}

	/* configure video window */
	ccdc_setwin(&params->win, params->frm_fmt, 2);

	/* configure the order of y cb cr in SD-RAM */
	regw((params->pix_order << 11) | 0x8000, CCDCFG);

	/* configure the horizontal line offset */
	/* this is done by rounding up width to a multiple of 16 pixels */
	/* and multiply by two to account for y:cb:cr 4:2:2 data */
	regw(((params->win.width * 2) + 31) & 0xffffffe0, HSIZE_OFF);

	/* configure the memory line offset */
	if (params->buf_type == CCDC_BUFTYPE_FLD_INTERLEAVED) {
		/* two fields are interleaved in memory */
		regw(0x00000249, SDOFST);
	}
	/* enable output to SDRAM */
	syn_mode |= (0x1 << 17);
	/* enable internal timing generator */
	syn_mode |= (0x1 << 16);

	syn_mode |= CCDC_DATA_PACK_ENABLE;
	regw(syn_mode, SYN_MODE);

	val = (unsigned int)ccdc_sbl_reset();
	printk(KERN_DEBUG "\nReading 0x%x from SBL...\n", val);

	printk(KERN_DEBUG "\nEnd of ccdc_config_ycbcr...\n");
	ccdc_readregs();
}

/*
 * ======== ccdc_config_raw  ========
 *
 * This function will configure CCDC for Raw mode parameters
 */
void ccdc_config_raw(void)
{

	struct ccdc_params_raw *params = &ccdc_hw_params_raw;
	unsigned int syn_mode = 0;
	unsigned int val;
	printk(KERN_DEBUG "\nStarting ccdc_config_raw...");
	/*      Reset CCDC */
	ccdc_reset();
	/* Disable latching function registers on VSYNC  */
	regw(CCDC_LATCH_ON_VSYNC_DISABLE, CCDCFG);

	/*      Configure the vertical sync polarity(SYN_MODE.VDPOL) */
	syn_mode = (params->vd_pol & CCDC_VD_POL_MASK) << CCDC_VD_POL_SHIFT;

	/*      Configure the horizontal sync polarity (SYN_MODE.HDPOL) */
	syn_mode |= (params->hd_pol & CCDC_HD_POL_MASK) << CCDC_HD_POL_SHIFT;

	/*      Configure frame id polarity (SYN_MODE.FLDPOL) */
	syn_mode |= (params->fid_pol & CCDC_FID_POL_MASK) << CCDC_FID_POL_SHIFT;

	/* Configure frame format(progressive or interlace) */
	syn_mode |= (params->frm_fmt & CCDC_FRM_FMT_MASK) << CCDC_FRM_FMT_SHIFT;

	/* Configure the data size(SYNMODE.DATSIZ) */
	syn_mode |= (params->data_sz & CCDC_DATA_SZ_MASK) << CCDC_DATA_SZ_SHIFT;

	/* Configure pixel format (Input mode) */
	syn_mode |= (params->pix_fmt & CCDC_PIX_FMT_MASK) << CCDC_PIX_FMT_SHIFT;

	/* Configure VP2SDR bit of syn_mode = 0 */
	syn_mode &= CCDC_VP2SDR_DISABLE;

	/* Enable write enable bit */
	syn_mode |= CCDC_WEN_ENABLE;

	/* Disable output to resizer */
	syn_mode &= CCDC_SDR2RSZ_DISABLE;

	/* enable internal timing generator */
	syn_mode |= CCDC_VDHDEN_ENABLE;

	/* Enable and configure aLaw register if needed */
	if (params->alaw.b_alaw_enable) {
		val = (params->alaw.gama_wd & CCDC_ALAW_GAMA_WD_MASK);
		val |= CCDC_ALAW_ENABLE;	/*set enable bit of alaw */
		regw(val, ALAW);

		printk(KERN_DEBUG "\nWriting 0x%x to ALAW...\n", val);
	}

	/* configure video window */
	ccdc_setwin(&params->win, params->frm_fmt, PPC_RAW);

	if (params->blk_clamp.b_clamp_enable) {
		val = (params->blk_clamp.sgain) & CCDC_BLK_SGAIN_MASK;	/*gain */
		val |= (params->blk_clamp.start_pixel & CCDC_BLK_ST_PXL_MASK)
		    << CCDC_BLK_ST_PXL_SHIFT;	/*Start pixel */
		val |= (params->blk_clamp.sample_ln & CCDC_BLK_SAMPLE_LINE_MASK)
		    << CCDC_BLK_SAMPLE_LINE_SHIFT;	/*No of line to be avg */
		val |=
		    (params->blk_clamp.sample_pixel & CCDC_BLK_SAMPLE_LN_MASK)
		    << CCDC_BLK_SAMPLE_LN_SHIFT;	/*No of pixel/line to be avg */
		val |= CCDC_BLK_CLAMP_ENABLE;	/*Enable the Black clamping */
		regw(val, CLAMP);

		printk(KERN_DEBUG "\nWriting 0x%x to CLAMP...\n", val);
		regw(DCSUB_DEFAULT_VAL, DCSUB);	/*If Black clamping is enable
						   then make dcsub 0 */
		printk(KERN_DEBUG "\nWriting 0x00000000 to DCSUB...\n");

	} else {
		/* configure DCSub */
		val = (params->blk_clamp.dc_sub) & CCDC_BLK_DC_SUB_MASK;
		regw(val, DCSUB);

		printk(KERN_DEBUG "\nWriting 0x%x to DCSUB...\n", val);
		regw(CLAMP_DEFAULT_VAL, CLAMP);

		printk(KERN_DEBUG "\nWriting 0x0000 to CLAMP...\n");
	}

	/*      Configure Black level compensation */
	val = (params->blk_comp.b_comp & CCDC_BLK_COMP_MASK);
	val |= (params->blk_comp.gb_comp & CCDC_BLK_COMP_MASK)
	    << CCDC_BLK_COMP_GB_COMP_SHIFT;
	val |= (params->blk_comp.gr_comp & CCDC_BLK_COMP_MASK)
	    << CCDC_BLK_COMP_GR_COMP_SHIFT;
	val |= (params->blk_comp.r_comp & CCDC_BLK_COMP_MASK)
	    << CCDC_BLK_COMP_R_COMP_SHIFT;

	regw(val, BLKCMP);

	printk(KERN_DEBUG "\nWriting 0x%x to BLKCMP...\n", val);
	printk(KERN_DEBUG "\nbelow 	regw(val, BLKCMP)...");
	/* Initially disable FPC */
	val = CCDC_FPC_DISABLE;
	regw(val, FPC);
	/* Configure Fault pixel if needed */
	if (params->fault_pxl.fpc_enable) {
		regw(params->fault_pxl.fpc_table_addr, FPC_ADDR);

		printk(KERN_DEBUG "\nWriting 0x%x to FPC_ADDR...\n",
		       (params->fault_pxl.fpc_table_addr));
		/* Write the FPC params with FPC disable */
		val = params->fault_pxl.fp_num & CCDC_FPC_FPC_NUM_MASK;
		regw(val, FPC);

		printk(KERN_DEBUG "\nWriting 0x%x to FPC...\n", val);
		/* read the FPC register */
		val = regr(FPC);
		val |= CCDC_FPC_ENABLE;
		regw(val, FPC);

		printk(KERN_DEBUG "\nWriting 0x%x to FPC...\n", val);
	}
	/* If data size is 8 bit then pack the data */
	if ((params->data_sz == _8BITS) || params->alaw.b_alaw_enable) {
		syn_mode |= CCDC_DATA_PACK_ENABLE;
	}
#if VIDEO_PORT_ENABLE
	val = ENABLE_VIDEO_PORT;	/* enable video port */
#else
	val = DISABLE_VIDEO_PORT;	/* disable video port */
#endif

	if (params->data_sz == _8BITS)
		val |= (_10BITS & CCDC_FMTCFG_VPIN_MASK)
		    << CCDC_FMTCFG_VPIN_SHIFT;
	else
		val |= (params->data_sz & CCDC_FMTCFG_VPIN_MASK)
		    << CCDC_FMTCFG_VPIN_SHIFT;

	/* Write value in FMTCFG */
	regw(val, FMTCFG);

	printk(KERN_DEBUG "\nWriting 0x%x to FMTCFG...\n", val);

	/* Configure the color pattern according to mt9t001 sensor */
	regw(CCDC_COLPTN_VAL, COLPTN);

	printk(KERN_DEBUG "\nWriting 0xBB11BB11 to COLPTN...\n");
	/* Configure Data formatter(Video port) pixel selection
	 * (FMT_HORZ, FMT_VERT)
	 */
	val = 0;
	val |= ((params->win.left) & CCDC_FMT_HORZ_FMTSPH_MASK)
	    << CCDC_FMT_HORZ_FMTSPH_SHIFT;
	val |= (((params->win.width)) & CCDC_FMT_HORZ_FMTLNH_MASK);
	regw(val, FMT_HORZ);

	printk(KERN_DEBUG "\nWriting 0x%x to FMT_HORZ...\n", val);
	val = 0;
	val |= (params->win.top & CCDC_FMT_VERT_FMTSLV_MASK)
	    << CCDC_FMT_VERT_FMTSLV_SHIFT;
	if (params->frm_fmt == CCDC_FRMFMT_PROGRESSIVE)
		val |= (params->win.height) & CCDC_FMT_VERT_FMTLNV_MASK;
	else
		val |= (params->win.height >> 1) & CCDC_FMT_VERT_FMTLNV_MASK;

	printk(KERN_DEBUG "\nparams->win.height  0x%x ...\n",
	       params->win.height);
	regw(val, FMT_VERT);

	printk(KERN_DEBUG "\nWriting 0x%x to FMT_VERT...\n", val);

	printk(KERN_DEBUG "\nbelow regw(val, FMT_VERT)...");

	/* Configure Horizontal offset register */
	/* If pack 8 is enabled then 1 pixel will take 1 byte */
	if ((params->data_sz == _8BITS) || params->alaw.b_alaw_enable) {
		regw(((params->win.width) + CCDC_32BYTE_ALIGN_VAL)
		     & CCDC_HSIZE_OFF_MASK, HSIZE_OFF);

	} else {		/* else one pixel will take 2 byte */

		regw(((params->win.width * TWO_BYTES_PER_PIXEL)
		      + CCDC_32BYTE_ALIGN_VAL)
		     & CCDC_HSIZE_OFF_MASK, HSIZE_OFF);

	}

	/* Set value for SDOFST */

	if (params->frm_fmt == CCDC_FRMFMT_INTERLACED) {
		if (params->image_invert_enable) {
			/* For intelace inverse mode */
			regw(INTERLACED_IMAGE_INVERT, SDOFST);
			printk(KERN_DEBUG "\nWriting 0x4B6D to SDOFST...\n");
		}

		else {
			/* For intelace non inverse mode */
			regw(INTERLACED_NO_IMAGE_INVERT, SDOFST);
			printk(KERN_DEBUG "\nWriting 0x0249 to SDOFST...\n");
		}
	} else if (params->frm_fmt == CCDC_FRMFMT_PROGRESSIVE) {
		regw(PROGRESSIVE_NO_IMAGE_INVERT, SDOFST);
		printk(KERN_DEBUG "\nWriting 0x0000 to SDOFST...\n");
	}

	/* Configure video port pixel selection (VPOUT) */
	/* Here -1 is to make the height value less than FMT_VERT.FMTLNV */
	if (params->frm_fmt == CCDC_FRMFMT_PROGRESSIVE) {
		val = (((params->win.height - 1) & CCDC_VP_OUT_VERT_NUM_MASK))
		    << CCDC_VP_OUT_VERT_NUM_SHIFT;
	} else {
		val =
		    ((((params->win.
			height >> CCDC_INTERLACED_HEIGHT_SHIFT) -
		       1) & CCDC_VP_OUT_VERT_NUM_MASK))
		    << CCDC_VP_OUT_VERT_NUM_SHIFT;
	}

	val |= ((((params->win.width))) & CCDC_VP_OUT_HORZ_NUM_MASK)
	    << CCDC_VP_OUT_HORZ_NUM_SHIFT;
	val |= (params->win.left) & CCDC_VP_OUT_HORZ_ST_MASK;
	regw(val, VP_OUT);

	printk(KERN_DEBUG "\nWriting 0x%x to VP_OUT...\n", val);
	regw(syn_mode, SYN_MODE);
	printk(KERN_DEBUG "\nWriting 0x%x to SYN_MODE...\n", syn_mode);

	val = (unsigned int)ccdc_sbl_reset();
	printk(KERN_DEBUG "\nReading 0x%x from SBL...\n", val);

	printk(KERN_DEBUG "\nend of ccdc_config_raw...");
	ccdc_readregs();
}

static int ccdc_configure(int mode)
{
	if (ccdc_if_type == CCDC_RAW_BAYER) {
		printk(KERN_INFO "calling ccdc_config_raw()\n");
		ccdc_config_raw();
	} else {
		printk(KERN_INFO "calling ccdc_config_ycbcr()\n");
		ccdc_config_ycbcr();
	}
	return 0;
}

static int ccdc_tryformat(struct v4l2_pix_format *fmt)
{
	return 0;
}

static int ccdc_set_buftype(enum ccdc_buftype buf_type)
{
	if (ccdc_if_type == CCDC_RAW_BAYER)
		ccdc_hw_params_raw.buf_type = buf_type;
	else
		ccdc_hw_params_ycbcr.buf_type = buf_type;
	return 0;
}

static int ccdc_get_buftype(enum ccdc_buftype *buf_type)
{
	if (ccdc_if_type == CCDC_RAW_BAYER)
		*buf_type = ccdc_hw_params_raw.buf_type;
	else
		*buf_type = ccdc_hw_params_ycbcr.buf_type;
	return 0;
}

static int ccdc_set_pixel_format(unsigned int pixfmt)
{
	if (ccdc_if_type == CCDC_RAW_BAYER) {
		if (pixfmt == V4L2_PIX_FMT_SBGGR8)
			ccdc_hw_params_raw.pix_fmt = CCDC_PIXFMT_RAW;
		else
			return -1;
	} else {
		if (pixfmt == V4L2_PIX_FMT_YUYV)
			ccdc_hw_params_ycbcr.pix_order = CCDC_PIXORDER_YCBYCR;
		else if (pixfmt == V4L2_PIX_FMT_UYVY)
			ccdc_hw_params_ycbcr.pix_order = CCDC_PIXORDER_CBYCRY;
		else
			return -1;
	}
	return 0;
}
static int ccdc_get_pixel_format(unsigned int *pixfmt)
{
	if (ccdc_if_type == CCDC_RAW_BAYER) {
		*pixfmt = V4L2_PIX_FMT_SBGGR8;
	} else {
		if (ccdc_hw_params_ycbcr.pix_order == CCDC_PIXORDER_YCBYCR)
			*pixfmt = V4L2_PIX_FMT_YUYV;
		else
			*pixfmt = V4L2_PIX_FMT_UYVY;
	}
	return 0;
}

static int ccdc_set_image_window(struct v4l2_rect *win)
{
	if (ccdc_if_type == CCDC_RAW_BAYER) {
		ccdc_hw_params_raw.win.top = win->top;
		ccdc_hw_params_raw.win.left = win->left;
		ccdc_hw_params_raw.win.width = win->width;
		ccdc_hw_params_raw.win.height = win->height;
	} else {
		ccdc_hw_params_ycbcr.win.top = win->top;
		ccdc_hw_params_ycbcr.win.left = win->left;
		ccdc_hw_params_ycbcr.win.width = win->width;
		ccdc_hw_params_ycbcr.win.height = win->height;
	}
	return 0;
}
static int ccdc_get_image_window(struct v4l2_rect *win)
{
	if (ccdc_if_type == CCDC_RAW_BAYER) {
		win->top = ccdc_hw_params_raw.win.top;
		win->left = ccdc_hw_params_raw.win.left;
		win->width = ccdc_hw_params_raw.win.width;
		win->height = ccdc_hw_params_raw.win.height;
	} else {
		win->top = ccdc_hw_params_ycbcr.win.top;
		win->left = ccdc_hw_params_ycbcr.win.left;
		win->width = ccdc_hw_params_ycbcr.win.width;
		win->height = ccdc_hw_params_ycbcr.win.height;
	}
	return 0;
}

static int ccdc_set_line_length(unsigned int len)
{
	/* nothing */
	return 0;
}

static int ccdc_get_line_length(unsigned int *len)
{
	if (ccdc_if_type == CCDC_RAW_BAYER) {
		if ((ccdc_hw_params_raw.alaw.b_alaw_enable) ||
		    (ccdc_hw_params_raw.data_sz == _8BITS)) {
			*len = ccdc_hw_params_raw.win.width;
		} else {
			*len = ccdc_hw_params_raw.win.width * 2;
		}
	} else {
		*len = ccdc_hw_params_ycbcr.win.width * 2;
	}
	return 0;
}

static int ccdc_set_frame_format(enum ccdc_frmfmt frm_fmt)
{
	if (ccdc_if_type == CCDC_RAW_BAYER)
		ccdc_hw_params_raw.frm_fmt = frm_fmt;
	else
		ccdc_hw_params_ycbcr.frm_fmt = frm_fmt;
	return 0;
}
static int ccdc_get_frame_format(enum ccdc_frmfmt *frm_fmt)
{
	if (ccdc_if_type == CCDC_RAW_BAYER)
		*frm_fmt = ccdc_hw_params_raw.frm_fmt;
	else
		*frm_fmt = ccdc_hw_params_ycbcr.frm_fmt;
	return 0;
}

/* Standard operation */
static int ccdc_setstd(char *std)
{
	struct v4l2_rect win;
	struct ccdc_std_info std_info;

	printk(KERN_INFO "ccdc_setstd\n");

	if (NULL == std)
		return -1;

	strcpy(std_info.name, std);
	if (ccdc_get_mode_info(&std_info) < 0)
		return -1;

	win.width = std_info.activepixels;
	win.height = std_info.activelines;
	win.left = 0;
	win.top = 0;
	ccdc_set_image_window(&win);
	if (ccdc_if_type == CCDC_RAW_BAYER)
		ccdc_set_pixel_format(V4L2_PIX_FMT_SBGGR8);
	else
		ccdc_set_pixel_format(V4L2_PIX_FMT_UYVY);

	if (!std_info.frame_format)
		ccdc_set_frame_format(CCDC_FRMFMT_INTERLACED);
	else
		ccdc_set_frame_format(CCDC_FRMFMT_PROGRESSIVE);
	ccdc_set_buftype(CCDC_BUFTYPE_FLD_INTERLEAVED);
	memcpy(&ccdc_video_config, &std_info, sizeof(struct ccdc_std_info));
	return 0;
}

static int ccdc_get_std_info(struct ccdc_std_info *std_info)
{
	if (std_info) {
		memcpy(std_info, &ccdc_video_config,
		       sizeof(struct ccdc_std_info));
		return 0;
	}
	return -1;
}

static int ccdc_getfid(void)
{
	int fid = (regr(SYN_MODE) >> 15) & 0x1;
	return fid;
}

/* misc operations */
static void ccdc_setfbaddr(unsigned long addr)
{
	regw(addr & 0xffffffe0, SDR_ADDR);
}

static int ccdc_set_hw_if_type(enum ccdc_hw_if_type iface)
{
	ccdc_if_type = iface;
	return 0;
}

static struct ccdc_hw_interface davinci_ccdc_hw_if = {
	.name = "DM6446 CCDC",
	.initialize = ccdc_initialize,
	.reset = ccdc_sbl_reset,
	.enable = ccdc_enable,
	.set_hw_if_type = ccdc_set_hw_if_type,
	.setparams = ccdc_setparams,
	.configure = ccdc_configure,
	.tryformat = ccdc_tryformat,
	.set_buftype = ccdc_set_buftype,
	.get_buftype = ccdc_get_buftype,
	.set_pixelformat = ccdc_set_pixel_format,
	.get_pixelformat = ccdc_get_pixel_format,
	.set_frame_format = ccdc_set_frame_format,
	.get_frame_format = ccdc_get_frame_format,
	.set_image_window = ccdc_set_image_window,
	.get_image_window = ccdc_get_image_window,
	.set_line_length = ccdc_set_line_length,
	.get_line_length = ccdc_get_line_length,
	.setstd = ccdc_setstd,
	.getstd_info = ccdc_get_std_info,
	.setfbaddr = ccdc_setfbaddr,
	.getfid = ccdc_getfid,
	.deinitialize = ccdc_deinitialize
};

struct ccdc_hw_interface *ccdc_get_hw_interface(void)
{
	return (&davinci_ccdc_hw_if);
}
EXPORT_SYMBOL(ccdc_get_hw_interface);

static int davinci_ccdc_init(void)
{
	ccdc_base_addr = CCDC_BASE_ADDR;
	vpss_sb_base_addr = VPSS_SB_BASE_ADDR;
	return 0;
}

static void davinci_ccdc_exit(void)
{
}

subsys_initcall(davinci_ccdc_init);
module_exit(davinci_ccdc_exit);

MODULE_LICENSE("GPL");
