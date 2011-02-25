/*
 * Copyright (C) 2005-2009 Texas Instruments Inc
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
#include <linux/delay.h>
#include <asm/arch/mux.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <media/davinci/ccdc_dm355.h>
#include <media/davinci/ccdc_hw_if.h>
#include <asm/arch/vpss.h>

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
	.med_filt_thres = 0,
	.mfilt1 = NO_MEDIAN_FILTER1,
	.mfilt2 = NO_MEDIAN_FILTER2,
	.ccdc_offset = 0,
	.gain = {
		.r_ye = 256,
		.gb_g = 256,
		.gr_cy = 256,
		.b_mg = 256
	},
	.lpf_enable = 0,
	.datasft = 2,
	.alaw = {
		.b_alaw_enable = 0,
		.gama_wd = 2
	},
	.blk_clamp = {
		.b_clamp_enable = 0,
		.sample_pixel = 1,
		.start_pixel = 0,
		.dc_sub = 25
	},
	.blk_comp = {
		.b_comp = 0,
		.gb_comp = 0,
		.gr_comp = 0,
		.r_comp = 0
	},
	.vertical_dft = {
		.ver_dft_en = 0
	},
	.lens_sh_corr = {
		.lsc_enable = 0
	},
	.data_formatter_r = {
		.fmt_enable = 0
	},
	.color_space_con = {
		.csc_enable = 0
	},
	.col_pat_field0 = {
		.olop = CCDC_GREEN_BLUE,
		.olep = CCDC_BLUE,
		.elop = CCDC_RED,
		.elep = CCDC_GREEN_RED
	},
	.col_pat_field1 = {
		.olop = CCDC_GREEN_BLUE,
		.olep = CCDC_BLUE,
		.elop = CCDC_RED,
		.elep = CCDC_GREEN_RED
	}
};

static struct ccdc_config_params_raw ccdc_hw_params_raw_temp;

/*Object for CCDC ycbcr mode */
static struct ccdc_params_ycbcr ccdc_hw_params_ycbcr = {
	.win = VPFE_WIN_PAL,
	.pix_fmt = CCDC_PIXFMT_YCBCR_8BIT,
	.frm_fmt = CCDC_FRMFMT_INTERLACED,
	.fid_pol = CCDC_PINPOL_POSITIVE,
	.vd_pol = CCDC_PINPOL_POSITIVE,
	.hd_pol = CCDC_PINPOL_POSITIVE,
	.bt656_enable = 1,
	.pix_order = CCDC_PIXORDER_CBYCRY,
	.buf_type = CCDC_BUFTYPE_FLD_INTERLEAVED
};

static struct v4l2_queryctrl ccdc_control_info[CCDC_MAX_CONTROLS] = {
	{
		.id = CCDC_CID_R_GAIN,
		.name = "R/Ye WB Gain",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.minimum = 0,
		.maximum = 2047,
		.step = 1,
		.default_value = 256
	},
	{
		.id = CCDC_CID_GR_GAIN,
		.name = "Gr/Cy WB Gain",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.minimum = 0,
		.maximum = 2047,
		.step = 1,
		.default_value = 256
	},
	{
		.id = CCDC_CID_GB_GAIN,
		.name = "Gb/G WB Gain",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.minimum = 0,
		.maximum = 2047,
		.step = 1,
		.default_value = 256
	},
	{
		.id = CCDC_CID_B_GAIN,
		.name = "B/Mg WB Gain",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.minimum = 0,
		.maximum = 2047,
		.step = 1,
		.default_value = 256
	},
	{
		.id = CCDC_CID_OFFSET,
		.name = "Offset",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.minimum = 0,
		.maximum = 1023,
		.step = 1,
		.default_value = 0
	}
};

static struct ccdc_std_info ccdc_video_config;
static enum ccdc_hw_if_type ccdc_if_type;
static unsigned long ccdc_base_addr;
static unsigned long vpss_base_addr;

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
static inline u32 regr_bl(u32 offset)
{
    return davinci_readl(vpss_base_addr + offset);
}

static inline u32 regw_bl(u32 val, u32 offset)
{
    davinci_writel(val, vpss_base_addr + offset);
    return val;
}

static void ccdc_enable(int en)
{
	unsigned int temp;
	temp = regr(SYNCEN);
	temp &= (~0x1);
	temp |= (en & 0x01);
	regw(temp, SYNCEN);
}

static void ccdc_enable_output_to_sdram(int en)
{
	unsigned int temp;
	temp = regr(SYNCEN);
	temp &= (~(0x1 << 1));
	temp |= (en & 0x01) << 1;
	regw(temp, SYNCEN);
}

static void ccdc_config_gain_offset(void)
{
	/* configure gain */
	regw(ccdc_hw_params_raw.gain.r_ye, RYEGAIN);

	regw(ccdc_hw_params_raw.gain.gr_cy, GRCYGAIN);

	regw(ccdc_hw_params_raw.gain.gb_g, GBGGAIN);

	regw(ccdc_hw_params_raw.gain.b_mg, BMGGAIN);

	/* configure offset */
	regw(ccdc_hw_params_raw.ccdc_offset, OFFSET);
}

/* Query control. Only applicable for Bayer capture */
static int ccdc_queryctrl(struct v4l2_queryctrl *qctrl)
{
	int i, id;
	struct v4l2_queryctrl *control = NULL;

	printk(KERN_DEBUG "ccdc_queryctrl: start\n");
	if (NULL == qctrl) {
		printk(KERN_ERR "ccdc_queryctrl : invalid user ptr\n");
		return -EINVAL;
	}

	if (ccdc_if_type != CCDC_RAW_BAYER) {
		printk(KERN_ERR
		       "ccdc_queryctrl : Not doing Raw Bayer Capture\n");
		return -EINVAL;
	}
	id = qctrl->id;
	memset(qctrl, 0, sizeof(struct v4l2_queryctrl));
	for (i = 0; i < CCDC_MAX_CONTROLS; i++) {
		control = &ccdc_control_info[i];
		if (control->id == id)
			break;
	}
	if (i == CCDC_MAX_CONTROLS) {
		printk(KERN_ERR "ccdc_queryctrl : Invalid control ID\n");
		return -EINVAL;
	}
	memcpy(qctrl, control, sizeof(struct v4l2_queryctrl));
	printk(KERN_DEBUG "ccdc_queryctrl: end\n");
	return 0;
}

static int ccdc_setcontrol(struct v4l2_control *ctrl)
{
	int i;
	struct v4l2_queryctrl *control = NULL;
	struct ccdc_gain *gain =
	    &ccdc_hw_params_raw.gain;

	if (NULL == ctrl) {
		printk(KERN_ERR "ccdc_setcontrol: invalid user ptr\n");
		return -EINVAL;
	}

	if (ccdc_if_type != CCDC_RAW_BAYER) {
		printk(KERN_ERR
		       "ccdc_setcontrol: Not doing Raw Bayer Capture\n");
		return -EINVAL;
	}

	for (i = 0; i < CCDC_MAX_CONTROLS; i++) {
		control = &ccdc_control_info[i];
		if (control->id == ctrl->id)
			break;
	}

	if (i == CCDC_MAX_CONTROLS) {
		printk(KERN_ERR "ccdc_queryctrl : Invalid control ID, 0x%x\n",
		       control->id);
		return -EINVAL;
	}

	if (ctrl->value > control->maximum) {
		printk(KERN_ERR "ccdc_queryctrl : Invalid control value\n");
		return -EINVAL;
	}

	switch (ctrl->id) {
	case CCDC_CID_R_GAIN:
		gain->r_ye = ctrl->value & CCDC_GAIN_MASK;
		break;
	case CCDC_CID_GR_GAIN:
		gain->gr_cy = ctrl->value & CCDC_GAIN_MASK;
		break;
	case CCDC_CID_GB_GAIN:
		gain->gb_g = ctrl->value  & CCDC_GAIN_MASK;
		break;

	case CCDC_CID_B_GAIN:
		gain->b_mg = ctrl->value  & CCDC_GAIN_MASK;
		break;
	default:
		ccdc_hw_params_raw.ccdc_offset = ctrl->value & CCDC_OFFSET_MASK;
	}

	/* set it in hardware */
	ccdc_config_gain_offset();
	return 0;
}

static int ccdc_getcontrol(struct v4l2_control *ctrl)
{
	int i;
	struct v4l2_queryctrl *control = NULL;

	if (NULL == ctrl) {
		printk(KERN_ERR "ccdc_setcontrol: invalid user ptr\n");
		return -EINVAL;
	}

	if (ccdc_if_type != CCDC_RAW_BAYER) {
		printk(KERN_ERR
		       "ccdc_setcontrol: Not doing Raw Bayer Capture\n");
		return -EINVAL;
	}

	for (i = 0; i < CCDC_MAX_CONTROLS; i++) {
		control = &ccdc_control_info[i];
		if (control->id == ctrl->id)
			break;
	}

	if (i == CCDC_MAX_CONTROLS) {
		printk(KERN_ERR "ccdc_queryctrl : Invalid control ID\n");
		return -EINVAL;
	}

	switch (ctrl->id) {
	case CCDC_CID_R_GAIN:
		ctrl->value = ccdc_hw_params_raw.gain.r_ye;
		break;
	case CCDC_CID_GR_GAIN:
		ctrl->value = ccdc_hw_params_raw.gain.gr_cy;
		break;
	case CCDC_CID_GB_GAIN:
		ctrl->value = ccdc_hw_params_raw.gain.gb_g;
		break;
	case CCDC_CID_B_GAIN:
		ctrl->value = ccdc_hw_params_raw.gain.b_mg;
		break;
	default:
		/* offset */
		ctrl->value = ccdc_hw_params_raw.ccdc_offset;
	}
	/* set it in hardware */
	return 0;
}

static void ccdc_reset(void)
{
	int i, clkctrl;
	/* disable CCDC */
	printk(KERN_DEBUG "\nstarting ccdc_reset...");
	ccdc_enable(0);
	/* set all registers to default value */
	for (i = 0; i <= 0x15c; i += 4)
		regw(0, i);
	/* no culling support */
	regw(0xffff, CULH);
	regw(0xff, CULV);
	/* Set default Gain and Offset */
	ccdc_hw_params_raw.gain.r_ye = 256;
	ccdc_hw_params_raw.gain.gb_g = 256;
	ccdc_hw_params_raw.gain.gr_cy = 256;
	ccdc_hw_params_raw.gain.b_mg = 256;
	ccdc_hw_params_raw.ccdc_offset = 0;
	ccdc_config_gain_offset();
	/* up to 12 bit sensor */
	regw(0x0FFF, OUTCLIP);
	vpss_dfc_memory_sel(VPSS_DFC_SEL_IPIPE);
	regw_bl(0x00, CCDCMUX);	/*CCDC input Mux select directly from sensor */
	clkctrl = regr_bl(CLKCTRL);
	clkctrl &= 0x3f;
	clkctrl |= 0x40;
	regw_bl(clkctrl, CLKCTRL);

	printk(KERN_DEBUG "\nEnd of ccdc_reset...");
}

static int ccdc_initialize(void)
{
	davinci_cfg_reg(DM355_VIN_PCLK, PINMUX_RESV);
	davinci_cfg_reg(DM355_VIN_CAM_WEN, PINMUX_RESV);
	davinci_cfg_reg(DM355_VIN_CAM_VD, PINMUX_RESV);
	davinci_cfg_reg(DM355_VIN_CAM_HD, PINMUX_RESV);
	davinci_cfg_reg(DM355_VIN_YIN_EN, PINMUX_RESV);
	davinci_cfg_reg(DM355_VIN_CINL_EN, PINMUX_RESV);
	davinci_cfg_reg(DM355_VIN_CINH_EN, PINMUX_RESV);
	ccdc_reset();
	return 0;
}

/*
 * ======== ccdc_setwin  ========
 *
 * This function will configure the window size to
 * be capture in CCDC reg
 */
static void ccdc_setwin(struct ccdc_imgwin *image_win,
			enum ccdc_frmfmt frm_fmt, int ppc, int mode)
{
	int horz_start, horz_nr_pixels;
	int vert_start, vert_nr_lines;
	int mid_img = 0;
	printk(KERN_DEBUG "\nStarting ccdc_setwin...");
	/* configure horizonal and vertical starts and sizes */
	horz_start = image_win->left << (ppc - 1);
	horz_nr_pixels = ((image_win->width) << (ppc - 1)) - 1;

	/*Writing the horizontal info into the registers */
	regw(horz_start & START_PX_HOR_MASK, SPH);
	regw(horz_nr_pixels & NUM_PX_HOR_MASK, NPH);
	vert_start = image_win->top;

	if (frm_fmt == CCDC_FRMFMT_INTERLACED) {
		vert_nr_lines = (image_win->height >> 1) - 1;
		vert_start >>= 1;
		/* Since first line doesn't have any data */
		vert_start += 1;
	} else {
		/* Since first line doesn't have any data */
		vert_start += 1;
		vert_nr_lines = image_win->height - 1;
		/* configure VDINT0 and VDINT1 */
		mid_img = vert_start + (image_win->height / 2);
		regw(mid_img, VDINT1);
	}
	if (!mode)
		regw(0, VDINT0);
	else
		regw(vert_nr_lines, VDINT0);

	regw(vert_start & START_VER_ONE_MASK, SLV0);
	regw(vert_start & START_VER_TWO_MASK, SLV1);
	regw(vert_nr_lines & NUM_LINES_VER, NLV);
	printk(KERN_DEBUG "\nEnd of ccdc_setwin...");
}

static int validate_ccdc_param(struct ccdc_config_params_raw *ccdcparam)
{
	if (ccdcparam->pix_fmt != 0) {
		printk(KERN_ERR
		       "Invalid value of pix_fmt, only raw supported\n");
		return -1;
	}

	if (ccdcparam->frm_fmt != 0) {
		printk(KERN_ERR
		       "Only Progressive frame format is supported\n");
		return -1;
	}

	if (ccdcparam->fid_pol != CCDC_PINPOL_POSITIVE
	    && ccdcparam->fid_pol != CCDC_PINPOL_NEGATIVE) {
		printk(KERN_ERR "Invalid value of field id polarity\n");
		return -1;
	}

	if (ccdcparam->vd_pol != CCDC_PINPOL_POSITIVE
	    && ccdcparam->vd_pol != CCDC_PINPOL_NEGATIVE) {
		printk(KERN_ERR "Invalid value of VD polarity\n");
		return -1;
	}

	if (ccdcparam->hd_pol != CCDC_PINPOL_POSITIVE
	    && ccdcparam->hd_pol != CCDC_PINPOL_NEGATIVE) {
		printk(KERN_ERR "Invalid value of HD polarity\n");
		return -1;
	}

	if (ccdcparam->datasft < NO_SHIFT || ccdcparam->datasft > _6BIT) {
		printk(KERN_ERR "Invalid value of data shift\n");
		return -1;
	}

	if (ccdcparam->mfilt1 < NO_MEDIAN_FILTER1
	    || ccdcparam->mfilt1 > MEDIAN_FILTER1) {
		printk(KERN_ERR "Invalid value of median filter1\n");
		return -1;
	}

	if (ccdcparam->mfilt2 < NO_MEDIAN_FILTER2
	    || ccdcparam->mfilt2 > MEDIAN_FILTER2) {
		printk(KERN_ERR "Invalid value of median filter2\n");
		return -1;
	}

	if (ccdcparam->ccdc_offset < 0 || ccdcparam->ccdc_offset > 1023) {
		printk(KERN_ERR "Invalid value of offset\n");
		return -1;
	}

	if ((ccdcparam->med_filt_thres < 0)
		|| (ccdcparam->med_filt_thres > 0x3FFF)) {
		printk(KERN_ERR "Invalid value of median filter thresold\n");
		return -1;
	}

	if (ccdcparam->data_sz < _16BITS || ccdcparam->data_sz > _8BITS) {
		printk(KERN_ERR "Invalid value of data size\n");
		return -1;
	}

	if (ccdcparam->alaw.b_alaw_enable) {
		if (ccdcparam->alaw.gama_wd < BITS_13_4
		    || ccdcparam->alaw.gama_wd > BITS_09_0) {
			printk(KERN_ERR "Invalid value of ALAW\n");
			return -1;
		}
	}

	if (ccdcparam->blk_clamp.b_clamp_enable) {
		if (ccdcparam->blk_clamp.sample_pixel < _1PIXELS
		    || ccdcparam->blk_clamp.sample_pixel > _16PIXELS) {
			printk(KERN_ERR "Invalid value of sample pixel\n");
			return -1;
		}
		if (ccdcparam->blk_clamp.sample_ln < _1LINES
		    || ccdcparam->blk_clamp.sample_ln > _16LINES) {
			printk(KERN_ERR "Invalid value of sample lines\n");
			return -1;
		}

	}

	if (ccdcparam->lens_sh_corr.lsc_enable) {
		printk(KERN_ERR "Lens shadding correction is not supported\n");
		return -1;
	}
	return 0;
}

static int ccdc_update_raw_params(void *arg)
{
	struct ccdc_config_params_raw *raw =
		(struct ccdc_config_params_raw *)arg;

	ccdc_hw_params_raw.pix_fmt =
		raw->pix_fmt;
	ccdc_hw_params_raw.frm_fmt =
		raw->frm_fmt;
	ccdc_hw_params_raw.win =
		raw->win;
	ccdc_hw_params_raw.fid_pol =
		raw->fid_pol;
	ccdc_hw_params_raw.vd_pol =
		raw->vd_pol;
	ccdc_hw_params_raw.hd_pol =
		raw->hd_pol;
	ccdc_hw_params_raw.buf_type =
		raw->buf_type;
	ccdc_hw_params_raw.datasft =
		raw->datasft;
	ccdc_hw_params_raw.mfilt1 =
		raw->mfilt1;
	ccdc_hw_params_raw.mfilt2 =
		raw->mfilt2;
	ccdc_hw_params_raw.lpf_enable =
		raw->lpf_enable;
	ccdc_hw_params_raw.horz_flip_enable =
		raw->horz_flip_enable;
	ccdc_hw_params_raw.ccdc_offset =
		raw->ccdc_offset;
	ccdc_hw_params_raw.med_filt_thres =
		raw->med_filt_thres;
	ccdc_hw_params_raw.image_invert_enable =
		raw->image_invert_enable;
	ccdc_hw_params_raw.data_sz =
		raw->data_sz;
	ccdc_hw_params_raw.alaw =
		raw->alaw;
	ccdc_hw_params_raw.data_offset_s =
		raw->data_offset_s;
	ccdc_hw_params_raw.blk_clamp =
		raw->blk_clamp;
	ccdc_hw_params_raw.vertical_dft =
		raw->vertical_dft;
	ccdc_hw_params_raw.lens_sh_corr =
		raw->lens_sh_corr;
	ccdc_hw_params_raw.data_formatter_r =
		raw->data_formatter_r;
	ccdc_hw_params_raw.color_space_con =
		raw->color_space_con;
	ccdc_hw_params_raw.col_pat_field0 =
		raw->col_pat_field0;
	ccdc_hw_params_raw.col_pat_field1 =
		raw->col_pat_field1;
	return 0;
}

static int ccdc_update_ycbcr_params(void *arg)
{
	if (copy_from_user(&ccdc_hw_params_ycbcr,
			   (struct ccdc_params_ycbcr *)arg,
			   sizeof(struct ccdc_params_ycbcr))) {
		printk(KERN_ERR "ccdc_update_ycbcr_params: error"
			   "in copying ccdc params\n");
		return -EFAULT;
	}
	return 0;
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
			printk(KERN_ERR "ccdc_setparams: error in copying ccdc"
				   "params, %d\n", x);
			return -EFAULT;
		}

		if (!validate_ccdc_param(&ccdc_hw_params_raw_temp)) {
			if (!ccdc_update_raw_params(&ccdc_hw_params_raw_temp))
				return 0;
		}
	} else
		return (ccdc_update_ycbcr_params(params));
	return -EINVAL;
}


/*This function will configure CCDC for YCbCr parameters*/
static void ccdc_config_ycbcr(int mode)
{
	u32 modeset;
	struct ccdc_params_ycbcr *params = &ccdc_hw_params_ycbcr;

	/* first reset the CCDC                                          */
	/* all registers have default values after reset                 */
	/* This is important since we assume default values to be set in */
	/* a lot of registers that we didn't touch                       */
	printk(KERN_DEBUG "\nStarting ccdc_config_ycbcr...");
	ccdc_reset();

	/* configure pixel format */
	modeset = (params->pix_fmt & 0x3) << 12;

	/* configure video frame format */
	modeset |= (params->frm_fmt & 0x1) << 7;

	/* setup BT.656 sync mode */
	if (params->bt656_enable) {
		regw(3, REC656IF);
		/* configure the FID, VD, HD pin polarity */
		/* fld,hd pol positive, vd negative, 8-bit pack mode */
		modeset |= 0x04;
	} else {		/* y/c external sync mode */
		modeset |= ((params->fid_pol & 0x1) << 4);
		modeset |= ((params->hd_pol & 0x1) << 3);
		modeset |= ((params->vd_pol & 0x1) << 2);
	}

	/* pack the data to 8-bit */
	modeset |= 0x1 << 11;

	regw(modeset, MODESET);

	/* configure video window */
	ccdc_setwin(&params->win, params->frm_fmt, 2, mode);
	/* configure the order of y cb cr in SD-RAM */
	regw((params->pix_order << 11) | 0x8040, CCDCFG);

	/* configure the horizontal line offset */
	/* this is done by rounding up width to a multiple of 16 pixels */
	/* and multiply by two to account for y:cb:cr 4:2:2 data */
	regw(((((params->win.width * 2) + 31) & 0xffffffe0) >> 5), HSIZE);

	/* configure the memory line offset */
	if (params->buf_type == CCDC_BUFTYPE_FLD_INTERLEAVED) {
		/* two fields are interleaved in memory */
		regw(0x00000249, SDOFST);
	}

	printk(KERN_DEBUG "\nEnd of ccdc_config_ycbcr...\n");
}


/*
 * ======== ccdc_config_raw  ========
 *
 * This function will configure CCDC for Raw mode parameters
 */
static void ccdc_config_raw(int mode)
{
	struct ccdc_params_raw *params = &ccdc_hw_params_raw;
	unsigned int mode_set = 0;
	unsigned int val = 0, val1 = 0;
	int temp1 = 0, temp2 = 0, i = 0, fmtreg_v = 0, shift_v = 0, flag = 0;
	int temp_gf = 0, temp_lcs = 0;
	printk(KERN_DEBUG "\nStarting ccdc_config_raw...");
	/*      Reset CCDC */
	ccdc_reset();

	/*
	 *      C O N F I G U R I N G  T H E  C C D C F G  R E G I S T E R
	 */

	/*Set CCD Not to swap input since input is RAW data */
	val |= CCDC_YCINSWP_RAW;

	/*set FID detection function to Latch at V-Sync */
	val |= CCDC_CCDCFG_FIDMD_LATCH_VSYNC << CCDC_CCDCFG_FIDMD_SHIFT;

	/*set WENLOG - ccdc valid area */
	val |= CCDC_CCDCFG_WENLOG_AND << CCDC_CCDCFG_WENLOG_SHIFT;

	/*set TRGSEL */
	val |= CCDC_CCDCFG_TRGSEL_WEN << CCDC_CCDCFG_TRGSEL_SHIFT;

	/*set EXTRG */
	val |= CCDC_CCDCFG_EXTRG_DISABLE << CCDC_CCDCFG_EXTRG_SHIFT;

	/* Disable latching function registers on VSYNC-busy writable
	   registers  */
	/*regw(CCDC_LATCH_ON_VSYNC_DISABLE, CCDCFG); */

	/* Enable latching function registers on VSYNC-shadowed registers */
	val |= CCDC_LATCH_ON_VSYNC_DISABLE;
	regw(val, CCDCFG);
	/*
	 *      C O N F I G U R I N G  T H E  M O D E S E T  R E G I S T E R
	 */

	/*Set VDHD direction to input */
	mode_set |=
	    (CCDC_VDHDOUT_INPUT & CCDC_VDHDOUT_MASK) << CCDC_VDHDOUT_SHIFT;

	/*Set input type to raw input */
	mode_set |=
	    (CCDC_RAW_IP_MODE & CCDC_RAW_INPUT_MASK) << CCDC_RAW_INPUT_SHIFT;

	/*      Configure the vertical sync polarity(MODESET.VDPOL) */
	mode_set = (params->vd_pol & CCDC_VD_POL_MASK) << CCDC_VD_POL_SHIFT;

	/*      Configure the horizontal sync polarity (MODESET.HDPOL) */
	mode_set |= (params->hd_pol & CCDC_HD_POL_MASK) << CCDC_HD_POL_SHIFT;

	/*      Configure frame id polarity (MODESET.FLDPOL) */
	mode_set |= (params->fid_pol & CCDC_FID_POL_MASK) << CCDC_FID_POL_SHIFT;

	/*      Configure data polarity */
	mode_set |=
	    (CCDC_DATAPOL_NORMAL & CCDC_DATAPOL_MASK) << CCDC_DATAPOL_SHIFT;

	/*      Configure External WEN Selection */
	mode_set |= (CCDC_EXWEN_DISABLE & CCDC_EXWEN_MASK) << CCDC_EXWEN_SHIFT;

	/* Configure frame format(progressive or interlace) */
	mode_set |= (params->frm_fmt & CCDC_FRM_FMT_MASK) << CCDC_FRM_FMT_SHIFT;

	/* Configure pixel format (Input mode) */
	mode_set |= (params->pix_fmt & CCDC_PIX_FMT_MASK) << CCDC_PIX_FMT_SHIFT;

	if ((params->data_sz == _8BITS) || params->alaw.b_alaw_enable) {
		mode_set |= CCDC_DATA_PACK_ENABLE;
	}

	/*Configure for LPF */
	if (params->lpf_enable) {
		mode_set |=
		    (params->lpf_enable & CCDC_LPF_MASK) << CCDC_LPF_SHIFT;
	}

	/*Configure the data shift */
	mode_set |= (params->datasft & CCDC_DATASFT_MASK) << CCDC_DATASFT_SHIFT;
	regw(mode_set, MODESET);
	printk(KERN_DEBUG "\nWriting 0x%x to MODESET...\n", mode_set);

	/*Configure the Median Filter threshold */
	regw((params->med_filt_thres) & 0x3fff, MEDFILT);

	/*
	 *      C O N F I G U R E   T H E   G A M M A W D   R E G I S T E R
	 */

	val = 8;
	val |=
	    (CCDC_CFA_MOSAIC & CCDC_GAMMAWD_CFA_MASK) << CCDC_GAMMAWD_CFA_SHIFT;

	/* Enable and configure aLaw register if needed */
	if (params->alaw.b_alaw_enable) {
		val |= (params->alaw.gama_wd & CCDC_ALAW_GAMA_WD_MASK) << 2;
		val |= CCDC_ALAW_ENABLE;	/*set enable bit of alaw */
	}

	/*Configure Median filter1 for IPIPE capture */
	val |= params->mfilt1 << CCDC_MFILT1_SHIFT;

	/*Configure Median filter2 for SDRAM capture */
	val |= params->mfilt2 << CCDC_MFILT2_SHIFT;

	regw(val, GAMMAWD);
	printk(KERN_DEBUG "\nWriting 0x%x to GAMMAWD...\n", val);

	/* configure video window */
	ccdc_setwin(&params->win, params->frm_fmt, 1, mode);

	/*
	 *      O P T I C A L   B L A C K   A V E R A G I N G
	 */
	val = 0;
	if (params->blk_clamp.b_clamp_enable) {
		val |= (params->blk_clamp.start_pixel & CCDC_BLK_ST_PXL_MASK);

		val1 |= (params->blk_clamp.sample_ln & CCDC_NUM_LINE_CALC_MASK)
		    << CCDC_NUM_LINE_CALC_SHIFT;	/*No of line
							   to be avg */
		val |=
		    (params->blk_clamp.sample_pixel & CCDC_BLK_SAMPLE_LN_MASK)
		    << CCDC_BLK_SAMPLE_LN_SHIFT;	/*No of pixel/line to be avg */
		val |= CCDC_BLK_CLAMP_ENABLE;	/*Enable the Black clamping */
		regw(val, CLAMP);

		printk(KERN_DEBUG "\nWriting 0x%x to CLAMP...\n", val);
		regw(val1, DCSUB);	/*If Black clamping is enable
					   then make dcsub 0 */
		printk(KERN_DEBUG "\nWriting 0x00000000 to DCSUB...\n");

	} else {
		/* configure DCSub */
		val = (params->blk_clamp.dc_sub) & CCDC_BLK_DC_SUB_MASK;
		regw(val, DCSUB);

		printk(KERN_DEBUG "\nWriting 0x%x to DCSUB...\n", val);
		regw(0x0000, CLAMP);

		printk(KERN_DEBUG "\nWriting 0x0000 to CLAMP...\n");
	}

	/*
	 *  C O N F I G U R E   B L A C K   L E V E L   C O M P E N S A T I O N
	 */
	val = 0;
	val = (params->blk_comp.b_comp & CCDC_BLK_COMP_MASK);
	val |= (params->blk_comp.gb_comp & CCDC_BLK_COMP_MASK)
	    << CCDC_BLK_COMP_GB_COMP_SHIFT;
	regw(val, BLKCMP1);

	val1 = 0;
	val1 |= (params->blk_comp.gr_comp & CCDC_BLK_COMP_MASK)
	    << CCDC_BLK_COMP_GR_COMP_SHIFT;
	val1 |= (params->blk_comp.r_comp & CCDC_BLK_COMP_MASK)
	    << CCDC_BLK_COMP_R_COMP_SHIFT;
	regw(val1, BLKCMP0);

	printk(KERN_DEBUG "\nWriting 0x%x to BLKCMP1...\n", val);
	printk(KERN_DEBUG "\nWriting 0x%x to BLKCMP0...\n", val1);

	/* Configure Vertical Defect Correction if needed */
	if (params->vertical_dft.ver_dft_en) {

		shift_v = 0;
		shift_v = 0 << CCDC_DFCCTL_VDFCEN_SHIFT;
		shift_v |=
		    params->vertical_dft.gen_dft_en & CCDC_DFCCTL_GDFCEN_MASK;
		shift_v |=
		    (params->vertical_dft.dft_corr_ctl.
		     vdfcsl & CCDC_DFCCTL_VDFCSL_MASK) <<
		    CCDC_DFCCTL_VDFCSL_SHIFT;
		shift_v |=
		    (params->vertical_dft.dft_corr_ctl.
		     vdfcuda & CCDC_DFCCTL_VDFCUDA_MASK) <<
		    CCDC_DFCCTL_VDFCUDA_SHIFT;
		shift_v |=
		    (params->vertical_dft.dft_corr_ctl.
		     vdflsft & CCDC_DFCCTL_VDFLSFT_MASK) <<
		    CCDC_DFCCTL_VDFLSFT_SHIFT;
		regw(shift_v, DFCCTL);
		regw(params->vertical_dft.dft_corr_vert[0], DFCMEM0);
		regw(params->vertical_dft.dft_corr_horz[0], DFCMEM1);
		regw(params->vertical_dft.dft_corr_sub1[0], DFCMEM2);
		regw(params->vertical_dft.dft_corr_sub2[0], DFCMEM3);
		regw(params->vertical_dft.dft_corr_sub3[0], DFCMEM4);

		shift_v = 0;
		shift_v = regr(DFCMEMCTL);
		shift_v |= 1 << CCDC_DFCMEMCTL_DFCMARST_SHIFT;
		shift_v |= 1;
		regw(shift_v, DFCMEMCTL);

		while (1) {
			flag = regr(DFCMEMCTL);
			if ((flag & 0x01) == 0x00)
				break;
		}
		flag = 0;
		shift_v = 0;
		shift_v = regr(DFCMEMCTL);
		shift_v |= 0 << CCDC_DFCMEMCTL_DFCMARST_SHIFT;
		regw(shift_v, DFCMEMCTL);

		for (i = 1; i < 16; i++) {
			regw(params->vertical_dft.dft_corr_vert[i], DFCMEM0);
			regw(params->vertical_dft.dft_corr_horz[i], DFCMEM1);
			regw(params->vertical_dft.dft_corr_sub1[i], DFCMEM2);
			regw(params->vertical_dft.dft_corr_sub2[i], DFCMEM3);
			regw(params->vertical_dft.dft_corr_sub3[i], DFCMEM4);

			shift_v = 0;
			shift_v = regr(DFCMEMCTL);
			shift_v |= 1;
			regw(shift_v, DFCMEMCTL);

			while (1) {
				flag = regr(DFCMEMCTL);
				if ((flag & 0x01) == 0x00)
					break;
			}
			flag = 0;
		}
		regw(params->vertical_dft.
		     saturation_ctl & CCDC_VDC_DFCVSAT_MASK, DFCVSAT);

		shift_v = 0;
		shift_v = regr(DFCCTL);
		shift_v |= 1 << CCDC_DFCCTL_VDFCEN_SHIFT;
		regw(shift_v, DFCCTL);
	}

	/* Configure Lens Shading Correction if needed */
	if (params->lens_sh_corr.lsc_enable) {
		printk(KERN_DEBUG "\nlens shading Correction entered....\n");

		/*first disable the LSC */
		regw(CCDC_LSC_DISABLE, LSCCFG1);

		/*UPDATE PROCEDURE FOR GAIN FACTOR TABLE 1 */

		/*select table 1 */
		regw(CCDC_LSC_TABLE1_SLC, LSCMEMCTL);

		/*Reset memory address */
		temp_lcs = regr(LSCMEMCTL);
		temp_lcs |= CCDC_LSC_MEMADDR_RESET;
		regw(temp_lcs, LSCMEMCTL);

		/*Update gainfactor for table 1 - u8q8 */
		temp_gf =
		    ((int)(params->lens_sh_corr.gf_table1[0].frac_no * 256))
		    & CCDC_LSC_FRAC_MASK_T1;
		temp_gf |=
		    (((int)(params->lens_sh_corr.gf_table1[0].frac_no * 256))
		     & CCDC_LSC_FRAC_MASK_T1) << 8;
		regw(temp_gf, LSCMEMD);

		while (1) {
			if ((regr(LSCMEMCTL) & 0x10) == 0)
				break;
		}

		/*set the address to incremental mode */
		temp_lcs = 0;
		temp_lcs = regr(LSCMEMCTL);
		temp_lcs |= CCDC_LSC_MEMADDR_INCR;
		regw(temp_lcs, LSCMEMCTL);

		for (i = 2; i < 255; i += 2) {
			temp_gf = 0;
			temp_gf = ((int)
				   (params->lens_sh_corr.gf_table1[0].frac_no *
				    256))
			    & CCDC_LSC_FRAC_MASK_T1;
			temp_gf |= (((int)
				     (params->lens_sh_corr.gf_table1[0].
				      frac_no * 256))
				    & CCDC_LSC_FRAC_MASK_T1) << 8;
			regw(temp_gf, LSCMEMD);

			while (1) {
				if ((regr(LSCMEMCTL) & 0x10) == 0)
					break;
			}
		}

		/*UPDATE PROCEDURE FOR GAIN FACTOR TABLE 2 */

		/*select table 2 */
		temp_lcs = 0;
		temp_lcs = regr(LSCMEMCTL);
		temp_lcs |= CCDC_LSC_TABLE2_SLC;
		regw(temp_lcs, LSCMEMCTL);

		/*Reset memory address */
		temp_lcs = 0;
		temp_lcs = regr(LSCMEMCTL);
		temp_lcs |= CCDC_LSC_MEMADDR_RESET;
		regw(temp_lcs, LSCMEMCTL);

		/*Update gainfactor for table 2 - u16q14 */
		temp_gf =
		    (params->lens_sh_corr.gf_table2[0].
		     int_no & CCDC_LSC_INT_MASK) << 14;
		temp_gf |=
		    ((int)(params->lens_sh_corr.gf_table2[0].frac_no) * 16384)
		    & CCDC_LSC_FRAC_MASK;
		regw(temp_gf, LSCMEMD);

		while (1) {
			if ((regr(LSCMEMCTL) & 0x10) == 0)
				break;
		}

		/*set the address to incremental mode */
		temp_lcs = 0;
		temp_lcs = regr(LSCMEMCTL);
		temp_lcs |= CCDC_LSC_MEMADDR_INCR;
		regw(temp_lcs, LSCMEMCTL);

		for (i = 1; i < 128; i++) {
			temp_gf = 0;
			temp_gf =
			    (params->lens_sh_corr.gf_table2[i].
			     int_no & CCDC_LSC_INT_MASK) << 14;
			temp_gf |=
			    ((int)(params->lens_sh_corr.gf_table2[0].frac_no) *
			     16384)
			    & CCDC_LSC_FRAC_MASK;
			regw(temp_gf, LSCMEMD);

			while (1) {
				if ((regr(LSCMEMCTL) & 0x10) == 0)
					break;
			}
		}

		/*UPDATE PROCEDURE FOR GAIN FACTOR TABLE 3 */

		/*select table 3 */
		temp_lcs = 0;
		temp_lcs = regr(LSCMEMCTL);
		temp_lcs |= CCDC_LSC_TABLE3_SLC;
		regw(temp_lcs, LSCMEMCTL);

		/*Reset memory address */
		temp_lcs = 0;
		temp_lcs = regr(LSCMEMCTL);
		temp_lcs |= CCDC_LSC_MEMADDR_RESET;
		regw(temp_lcs, LSCMEMCTL);

		/*Update gainfactor for table 2 - u16q14 */
		temp_gf =
		    (params->lens_sh_corr.gf_table3[0].
		     int_no & CCDC_LSC_INT_MASK) << 14;
		temp_gf |=
		    ((int)(params->lens_sh_corr.gf_table3[0].frac_no) * 16384)
		    & CCDC_LSC_FRAC_MASK;
		regw(temp_gf, LSCMEMD);

		while (1) {
			if ((regr(LSCMEMCTL) & 0x10) == 0)
				break;
		}

		/*set the address to incremental mode */
		temp_lcs = 0;
		temp_lcs = regr(LSCMEMCTL);
		temp_lcs |= CCDC_LSC_MEMADDR_INCR;
		regw(temp_lcs, LSCMEMCTL);

		for (i = 1; i < 128; i++) {
			temp_gf = 0;
			temp_gf =
			    (params->lens_sh_corr.gf_table3[i].
			     int_no & CCDC_LSC_INT_MASK) << 14;
			temp_gf |=
			    ((int)(params->lens_sh_corr.gf_table3[0].frac_no) *
			     16384)
			    & CCDC_LSC_FRAC_MASK;
			regw(temp_gf, LSCMEMD);

			while (1) {
				if ((regr(LSCMEMCTL) & 0x10) == 0)
					break;
			}
		}
		/*Configuring the optical centre of the lens */
		regw(params->lens_sh_corr.
		     lens_center_horz & CCDC_LSC_CENTRE_MASK, LSCH0);
		regw(params->lens_sh_corr.
		     lens_center_vert & CCDC_LSC_CENTRE_MASK, LSCV0);

		val = 0;
		val =
		    ((int)(params->lens_sh_corr.horz_left_coef.frac_no * 128)) &
		    0x7f;
		val |= (params->lens_sh_corr.horz_left_coef.int_no & 0x01) << 7;
		val |=
		    (((int)(params->lens_sh_corr.horz_right_coef.frac_no * 128))
		     & 0x7f) << 8;
		val |=
		    (params->lens_sh_corr.horz_right_coef.int_no & 0x01) << 15;
		regw(val, LSCKH);

		val = 0;
		val =
		    ((int)(params->lens_sh_corr.ver_up_coef.frac_no * 128)) &
		    0x7f;
		val |= (params->lens_sh_corr.ver_up_coef.int_no & 0x01) << 7;
		val |=
		    (((int)(params->lens_sh_corr.ver_low_coef.frac_no * 128)) &
		     0x7f) << 8;
		val |= (params->lens_sh_corr.ver_low_coef.int_no & 0x01) << 15;
		regw(val, LSCKV);

		/*configuring the lsc configuration register 2 */
		temp_lcs = 0;
		temp_lcs |=
		    (params->lens_sh_corr.lsc_config.
		     gf_table_scaling_fact & CCDC_LSCCFG_GFTSF_MASK) <<
		    CCDC_LSCCFG_GFTSF_SHIFT;
		temp_lcs |=
		    (params->lens_sh_corr.lsc_config.
		     gf_table_interval & CCDC_LSCCFG_GFTINV_MASK) <<
		    CCDC_LSCCFG_GFTINV_SHIFT;
		temp_lcs |=
		    (params->lens_sh_corr.lsc_config.
		     epel & CCDC_LSC_GFTABLE_SEL_MASK) <<
		    CCDC_LSC_GFTABLE_EPEL_SHIFT;
		temp_lcs |=
		    (params->lens_sh_corr.lsc_config.
		     opel & CCDC_LSC_GFTABLE_SEL_MASK) <<
		    CCDC_LSC_GFTABLE_OPEL_SHIFT;
		temp_lcs |=
		    (params->lens_sh_corr.lsc_config.
		     epol & CCDC_LSC_GFTABLE_SEL_MASK) <<
		    CCDC_LSC_GFTABLE_EPOL_SHIFT;
		temp_lcs |=
		    (params->lens_sh_corr.lsc_config.
		     opol & CCDC_LSC_GFTABLE_SEL_MASK) <<
		    CCDC_LSC_GFTABLE_OPOL_SHIFT;
		regw(temp_lcs, LSCCFG2);

		/*configuring the LSC configuration register 1 */
		temp_lcs = 0;
		temp_lcs |= CCDC_LSC_ENABLE;
		temp_lcs |= (params->lens_sh_corr.lsc_config.mode &
			     CCDC_LSC_GFMODE_MASK) << CCDC_LSC_GFMODE_SHIFT;
		regw(temp_lcs, LSCCFG1);
	}

	/* Configure data formatter if needed */
	if (params->data_formatter_r.fmt_enable
	    && (!params->color_space_con.csc_enable)) {
		printk(KERN_DEBUG
		       "\ndata formatter will be configured now....\n");

		/*Configuring the FMTPLEN */
		fmtreg_v = 0;
		fmtreg_v |=
		    (params->data_formatter_r.plen.
		     plen0 & CCDC_FMTPLEN_P0_MASK);
		fmtreg_v |=
		    ((params->data_formatter_r.plen.
		      plen1 & CCDC_FMTPLEN_P1_MASK)
		     << CCDC_FMTPLEN_P1_SHIFT);
		fmtreg_v |=
		    ((params->data_formatter_r.plen.
		      plen2 & CCDC_FMTPLEN_P2_MASK)
		     << CCDC_FMTPLEN_P2_SHIFT);
		fmtreg_v |=
		    ((params->data_formatter_r.plen.
		      plen3 & CCDC_FMTPLEN_P3_MASK)
		     << CCDC_FMTPLEN_P3_SHIFT);
		regw(fmtreg_v, FMTPLEN);

		/*Configurring the FMTSPH */
		regw((params->data_formatter_r.fmtsph & CCDC_FMTSPH_MASK),
		     FMTSPH);

		/*Configurring the FMTLNH */
		regw((params->data_formatter_r.fmtlnh & CCDC_FMTLNH_MASK),
		     FMTLNH);

		/*Configurring the FMTSLV */
		regw((params->data_formatter_r.fmtslv & CCDC_FMTSLV_MASK),
		     FMTSLV);

		/*Configurring the FMTLNV */
		regw((params->data_formatter_r.fmtlnv & CCDC_FMTLNV_MASK),
		     FMTLNV);

		/*Configurring the FMTRLEN */
		regw((params->data_formatter_r.fmtrlen & CCDC_FMTRLEN_MASK),
		     FMTRLEN);

		/*Configurring the FMTHCNT */
		regw((params->data_formatter_r.fmthcnt & CCDC_FMTHCNT_MASK),
		     FMTHCNT);

		/*Configuring the FMTADDR_PTR */
		for (i = 0; i < 8; i++) {
			fmtreg_v = 0;

			if (params->data_formatter_r.addr_ptr[i].init >
			    (params->data_formatter_r.fmtrlen - 1)) {
				printk(KERN_DEBUG "\nInvalid init parameter for"
					   "FMTADDR_PTR....\n");
				return;
			}

			fmtreg_v =
			    (params->data_formatter_r.addr_ptr[i].
			     init & CCDC_ADP_INIT_MASK);
			fmtreg_v |=
			    ((params->data_formatter_r.addr_ptr[i].
			      line & CCDC_ADP_LINE_MASK) <<
			     CCDC_ADP_LINE_SHIFT);
			regw(fmtreg_v, FMT_ADDR_PTR(i));
		}

		/*Configuring the FMTPGM_VF0 */
		fmtreg_v = 0;
		for (i = 0; i < 16; i++) {
			fmtreg_v |= params->data_formatter_r.pgm_en[i] << i;
		}
		regw(fmtreg_v, FMTPGM_VF0);

		/*Configuring the FMTPGM_VF1 */
		fmtreg_v = 0;
		for (i = 16; i < 32; i++)
			fmtreg_v |=
			    params->data_formatter_r.pgm_en[i] << (i - 16);
		regw(fmtreg_v, FMTPGM_VF1);

		/*Configuring the FMTPGM_AP0 */
		fmtreg_v = 0;
		shift_v = 0;
		for (i = 0; i < 4; i++) {
			fmtreg_v |=
			    ((params->data_formatter_r.pgm_ap[i].
			      pgm_aptr & CCDC_FMTPGN_APTR_MASK) << shift_v);
			fmtreg_v |=
			    (params->data_formatter_r.pgm_ap[i].
			     pgmupdt << (shift_v + 3));
			shift_v += 4;
		}
		regw(fmtreg_v, FMTPGM_AP0);

		/*Configuring the FMTPGM_AP1 */
		fmtreg_v = 0;
		shift_v = 0;
		for (i = 4; i < 8; i++) {
			fmtreg_v |=
			    ((params->data_formatter_r.pgm_ap[i].
			      pgm_aptr & CCDC_FMTPGN_APTR_MASK) << shift_v);
			fmtreg_v |=
			    (params->data_formatter_r.pgm_ap[i].
			     pgmupdt << (shift_v + 3));
			shift_v += 4;
		}
		regw(fmtreg_v, FMTPGM_AP1);

		/*Configuring the FMTPGM_AP2 */
		fmtreg_v = 0;
		shift_v = 0;
		for (i = 8; i < 12; i++) {
			fmtreg_v |=
			    ((params->data_formatter_r.pgm_ap[i].
			      pgm_aptr & CCDC_FMTPGN_APTR_MASK) << shift_v);
			fmtreg_v |=
			    (params->data_formatter_r.pgm_ap[i].
			     pgmupdt << (shift_v + 3));
			shift_v += 4;
		}
		regw(fmtreg_v, FMTPGM_AP2);

		/*Configuring the FMTPGM_AP3 */
		fmtreg_v = 0;
		shift_v = 0;
		for (i = 12; i < 16; i++) {
			fmtreg_v |=
			    ((params->data_formatter_r.pgm_ap[i].
			      pgm_aptr & CCDC_FMTPGN_APTR_MASK) << shift_v);
			fmtreg_v |=
			    (params->data_formatter_r.pgm_ap[i].
			     pgmupdt << (shift_v + 3));
			shift_v += 4;
		}
		regw(fmtreg_v, FMTPGM_AP3);

		/*Configuring the FMTPGM_AP4 */
		fmtreg_v = 0;
		shift_v = 0;
		for (i = 16; i < 20; i++) {
			fmtreg_v |=
			    ((params->data_formatter_r.pgm_ap[i].
			      pgm_aptr & CCDC_FMTPGN_APTR_MASK) << shift_v);
			fmtreg_v |=
			    (params->data_formatter_r.pgm_ap[i].
			     pgmupdt << (shift_v + 3));
			shift_v += 4;
		}
		regw(fmtreg_v, FMTPGM_AP4);

		/*Configuring the FMTPGM_AP5 */
		fmtreg_v = 0;
		shift_v = 0;
		for (i = 20; i < 24; i++) {
			fmtreg_v |=
			    ((params->data_formatter_r.pgm_ap[i].
			      pgm_aptr & CCDC_FMTPGN_APTR_MASK) << shift_v);
			fmtreg_v |=
			    (params->data_formatter_r.pgm_ap[i].
			     pgmupdt << (shift_v + 3));
			shift_v += 4;
		}
		regw(fmtreg_v, FMTPGM_AP5);

		/*Configuring the FMTPGM_AP6 */
		fmtreg_v = 0;
		shift_v = 0;
		for (i = 24; i < 28; i++) {
			fmtreg_v |=
			    ((params->data_formatter_r.pgm_ap[i].
			      pgm_aptr & CCDC_FMTPGN_APTR_MASK) << shift_v);
			fmtreg_v |=
			    (params->data_formatter_r.pgm_ap[i].
			     pgmupdt << (shift_v + 3));
			shift_v += 4;
		}
		regw(fmtreg_v, FMTPGM_AP6);

		/*Configuring the FMTPGM_AP7 */
		fmtreg_v = 0;
		shift_v = 0;
		for (i = 28; i < 32; i++) {
			fmtreg_v |=
			    ((params->data_formatter_r.pgm_ap[i].
			      pgm_aptr & CCDC_FMTPGN_APTR_MASK) << shift_v);
			fmtreg_v |=
			    (params->data_formatter_r.pgm_ap[i].
			     pgmupdt << (shift_v + 3));
			shift_v += 4;
		}
		regw(fmtreg_v, FMTPGM_AP7);

		/*Configuring the FMTCFG register */
		fmtreg_v = 0;
		fmtreg_v = CCDC_DF_ENABLE;
		fmtreg_v |=
		    ((params->data_formatter_r.cfg.
		      mode & CCDC_FMTCFG_FMTMODE_MASK)
		     << CCDC_FMTCFG_FMTMODE_SHIFT);
		fmtreg_v |=
		    ((params->data_formatter_r.cfg.
		      lnum & CCDC_FMTCFG_LNUM_MASK)
		     << CCDC_FMTCFG_LNUM_SHIFT);
		fmtreg_v |=
		    ((params->data_formatter_r.cfg.
		      addrinc & CCDC_FMTCFG_ADDRINC_MASK)
		     << CCDC_FMTCFG_ADDRINC_SHIFT);
		regw(fmtreg_v, FMTCFG);

	} else if (params->data_formatter_r.fmt_enable)
		printk(KERN_DEBUG
		       "\nCSC and Data Formatter Enabled at same time....\n");

	/*
	 *      C O N F I G U R E   C O L O R   S P A C E   C O N V E R T E R
	 */

	if ((params->color_space_con.csc_enable)
	    && (!params->data_formatter_r.fmt_enable)) {
		printk(KERN_DEBUG "\nconfiguring the CSC Now....\n");

		/*Enable the CSC sub-module */
		regw(CCDC_CSC_ENABLE, CSCCTL);

		/*Converting the co-eff as per the format of the register */
		for (i = 0; i < 16; i++) {
			temp1 = params->color_space_con.csc_dec_coeff[i];
			/*Masking the data for 3 bits */
			temp1 &= CCDC_CSC_COEFF_DEC_MASK;
			/*Recovering the fractional part and converting to
			   binary of 5 bits */
			temp2 =
			    (int)(params->color_space_con.csc_frac_coeff[i] *
				  (32 / 10));
			temp2 &= CCDC_CSC_COEFF_FRAC_MASK;
			/*shifting the decimal to the MSB */
			temp1 = temp1 << CCDC_CSC_DEC_SHIFT;
			temp1 |= temp2;	/*appending the fraction at LSB */
			params->color_space_con.csc_dec_coeff[i] = temp1;
		}
		regw(params->color_space_con.csc_dec_coeff[0], CSCM0);
		regw(params->color_space_con.
		     csc_dec_coeff[1] << CCDC_CSC_COEFF_SHIFT, CSCM0);
		regw(params->color_space_con.csc_dec_coeff[2], CSCM1);
		regw(params->color_space_con.
		     csc_dec_coeff[3] << CCDC_CSC_COEFF_SHIFT, CSCM1);
		regw(params->color_space_con.csc_dec_coeff[4], CSCM2);
		regw(params->color_space_con.
		     csc_dec_coeff[5] << CCDC_CSC_COEFF_SHIFT, CSCM2);
		regw(params->color_space_con.csc_dec_coeff[6], CSCM3);
		regw(params->color_space_con.
		     csc_dec_coeff[7] << CCDC_CSC_COEFF_SHIFT, CSCM3);
		regw(params->color_space_con.csc_dec_coeff[8], CSCM4);
		regw(params->color_space_con.
		     csc_dec_coeff[9] << CCDC_CSC_COEFF_SHIFT, CSCM4);
		regw(params->color_space_con.csc_dec_coeff[10], CSCM5);
		regw(params->color_space_con.
		     csc_dec_coeff[11] << CCDC_CSC_COEFF_SHIFT, CSCM5);
		regw(params->color_space_con.csc_dec_coeff[12], CSCM6);
		regw(params->color_space_con.
		     csc_dec_coeff[13] << CCDC_CSC_COEFF_SHIFT, CSCM6);
		regw(params->color_space_con.csc_dec_coeff[14], CSCM7);
		regw(params->color_space_con.
		     csc_dec_coeff[15] << CCDC_CSC_COEFF_SHIFT, CSCM7);

	} else if (params->color_space_con.csc_enable) {
		printk(KERN_DEBUG
		       "\nCSC and Data Formatter Enabled at same time....\n");
	}

	/* Configure the Gain  & offset control */
	ccdc_config_gain_offset();

	/*
	 *      C O N F I G U R E  C O L O R  P A T T E R N  A S
	 *      P E R  N N 1 2 8 6 A  S E N S O R
	 */

	val = (params->col_pat_field0.olop);
	val |= (params->col_pat_field0.olep << 2);
	val |= (params->col_pat_field0.elop << 4);
	val |= (params->col_pat_field0.elep << 6);
	val |= (params->col_pat_field1.olop << 8);
	val |= (params->col_pat_field1.olep << 10);
	val |= (params->col_pat_field1.elop << 12);
	val |= (params->col_pat_field1.elep << 14);
	regw(val, COLPTN);

	printk(KERN_DEBUG "\nWriting %x to COLPTN...\n", val);

	/*
	 *      C O N F I G U R I N G  T H E  H S I Z E  R E G I S T E R
	 */
	val = 0;
	val |=
	    (params->data_offset_s.
	     horz_offset & CCDC_DATAOFST_MASK) << CCDC_DATAOFST_H_SHIFT;
	val |=
	    (params->data_offset_s.
	     vert_offset & CCDC_DATAOFST_MASK) << CCDC_DATAOFST_V_SHIFT;
	regw(val, DATAOFST);

	/*
	 *      C O N F I G U R I N G  T H E  H S I Z E  R E G I S T E R
	 */
	val = 0;
	val |=
	    (params->
	     horz_flip_enable & CCDC_HSIZE_FLIP_MASK) << CCDC_HSIZE_FLIP_SHIFT;

	/* If pack 8 is enable then 1 pixel will take 1 byte */
	if ((params->data_sz == _8BITS) || params->alaw.b_alaw_enable) {
		val |= (((params->win.width) + 31) >> 5) & 0x0fff;

		printk(KERN_DEBUG "\nWriting 0x%x to HSIZE...\n",
		       (((params->win.width) + 31) >> 5) & 0x0fff);
	} else {		/* else one pixel will take 2 byte */

		val |= (((params->win.width * 2) + 31) >> 5) & 0x0fff;

		printk(KERN_DEBUG "\nWriting 0x%x to HSIZE...\n",
		       (((params->win.width * 2) + 31) >> 5) & 0x0fff);
	}
	regw(val, HSIZE);

	/*
	 *      C O N F I G U R E   S D O F S T  R E G I S T E R
	 */

	if (params->frm_fmt == CCDC_FRMFMT_INTERLACED) {
		if (params->image_invert_enable) {
			/* For interlace inverse mode */
			regw(0x4B6D, SDOFST);
			printk(KERN_DEBUG "\nWriting 0x4B6D to SDOFST...\n");
		}

		else {
			/* For interlace non inverse mode */
			regw(0x0B6D, SDOFST);
			printk(KERN_DEBUG "\nWriting 0x0B6D to SDOFST...\n");
		}
	} else if (params->frm_fmt == CCDC_FRMFMT_PROGRESSIVE) {
		if (params->image_invert_enable) {
			/* For progessive inverse mode */
			regw(0x4000, SDOFST);
			printk(KERN_DEBUG "\nWriting 0x4000 to SDOFST...\n");
		}

		else {
			/* For progessive non inverse mode */
			regw(0x0000, SDOFST);
			printk(KERN_DEBUG "\nWriting 0x0000 to SDOFST...\n");
		}

	}
	printk(KERN_DEBUG "\nend of ccdc_config_raw...");
}

static int ccdc_configure(int mode)
{
	if (ccdc_if_type == CCDC_RAW_BAYER) {
		printk(KERN_INFO "calling ccdc_config_raw()\n");
		ccdc_config_raw(mode);
	} else {
		printk(KERN_INFO "calling ccdc_config_ycbcr()\n");
		ccdc_config_ycbcr(mode);
	}
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
		if (pixfmt == V4L2_PIX_FMT_SBGGR8) {
			ccdc_hw_params_raw.alaw.b_alaw_enable = 1;
			ccdc_hw_params_raw.pix_fmt = CCDC_PIXFMT_RAW;
		} else if (pixfmt == V4L2_PIX_FMT_SBGGR16) {
			ccdc_hw_params_raw.alaw.b_alaw_enable = 0;
			ccdc_hw_params_raw.pix_fmt = CCDC_PIXFMT_RAW;
		}
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
		if (ccdc_hw_params_raw.alaw.b_alaw_enable)
			*pixfmt = V4L2_PIX_FMT_SBGGR8;
		else
			*pixfmt = V4L2_PIX_FMT_SBGGR16;
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
	*len = ((*len + 31) & ~0x1f);
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
	int fid = (regr(MODESET) >> 15) & 0x1;
	return fid;
}

/* misc operations */
static void ccdc_setfbaddr(unsigned long addr)
{
	regw((addr >> 21) & 0x007f, STADRH);
	regw((addr >> 5) & 0x0ffff, STADRL);
}

static int ccdc_set_hw_if_type(enum ccdc_hw_if_type iface)
{
	ccdc_if_type = iface;
	return 0;
}

static struct ccdc_hw_interface dm355_ccdc_hw_if = {
	.name = "DM355 CCDC",
	.initialize = ccdc_initialize,
	.enable = ccdc_enable,
	.enable_out_to_sdram = ccdc_enable_output_to_sdram,
	.set_hw_if_type = ccdc_set_hw_if_type,
	.setparams = ccdc_setparams,
	.configure = ccdc_configure,
	.set_buftype = ccdc_set_buftype,
	.get_buftype = ccdc_get_buftype,
	.set_pixelformat = ccdc_set_pixel_format,
	.get_pixelformat = ccdc_get_pixel_format,
	.set_frame_format = ccdc_set_frame_format,
	.get_frame_format = ccdc_get_frame_format,
	.set_image_window = ccdc_set_image_window,
	.get_image_window = ccdc_get_image_window,
	.get_line_length = ccdc_get_line_length,
	.queryctrl = ccdc_queryctrl,
	.setcontrol = ccdc_setcontrol,
	.getcontrol = ccdc_getcontrol,
	.setstd = ccdc_setstd,
	.getstd_info = ccdc_get_std_info,
	.setfbaddr = ccdc_setfbaddr,
	.getfid = ccdc_getfid,
};

struct ccdc_hw_interface *ccdc_get_hw_interface(void)
{
	return (&dm355_ccdc_hw_if);
}
EXPORT_SYMBOL(ccdc_get_hw_interface);

static int dm355_ccdc_init(void)
{
	ccdc_base_addr = 0x01c70600;
	vpss_base_addr = 0x01c70000;
	return 0;
}

static void dm355_ccdc_exit(void)
{
}

subsys_initcall(dm355_ccdc_init);
module_exit(dm355_ccdc_exit);

MODULE_LICENSE("GPL");
