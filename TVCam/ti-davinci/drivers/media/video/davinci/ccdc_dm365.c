/*
 * Copyright (C) 2008-2009 Texas Instruments Inc
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
 *
 * This is the isif hardware module for DM365.
 */
#include <linux/delay.h>
#include <asm/arch/mux.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <media/davinci/ccdc_dm365.h>
#include <media/davinci/ccdc_hw_if.h>
#include <asm/arch/vpss.h>

static struct ccdc_bayer_config ccdc_hw_params_bayer = {
	.pix_fmt = CCDC_PIXFMT_RAW,
	.frm_fmt = CCDC_FRMFMT_PROGRESSIVE,
	.win = CCDC_WIN_VGA,
	.fid_pol = CCDC_PINPOL_POSITIVE,
	.vd_pol = CCDC_PINPOL_POSITIVE,
	.hd_pol = CCDC_PINPOL_POSITIVE,
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
	},
	.cfa_pat = CCDC_CFA_PAT_MOSAIC,
	.data_size = CCDC_12_BITS,
	.data_msb = CCDC_BIT_MSB_11,
	/* Data lines are connected to */
	.data_shift = CCDC_NO_SHIFT,
	.test_pat_gen = 0
};

static struct ccdc_ycbcr_config ccdc_hw_params_ycbcr = {
	.pix_fmt = CCDC_PIXFMT_YCBCR_8BIT,
	.frm_fmt = CCDC_FRMFMT_INTERLACED,
	.win = CCDC_WIN_PAL,
	.fid_pol = CCDC_PINPOL_POSITIVE,
	.vd_pol = CCDC_PINPOL_POSITIVE,
	.hd_pol = CCDC_PINPOL_POSITIVE,
	.pix_order = CCDC_PIXORDER_CBYCRY,
	.buf_type = CCDC_BUFTYPE_FLD_INTERLEAVED,
	.test_pat_gen = 0
};

static struct v4l2_queryctrl ccdc_control_info[CCDC_MAX_CONTROLS] = {
	{
		.id = CCDC_CID_R_GAIN,
		.name = "R/Ye WB Gain",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.minimum = 0,
		.maximum = 4095,
		.step = 1,
		.default_value = 512
	},
	{
		.id = CCDC_CID_GR_GAIN,
		.name = "Gr/Cy WB Gain",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.minimum = 0,
		.maximum = 4095,
		.step = 1,
		.default_value = 512
	},
	{
		.id = CCDC_CID_GB_GAIN,
		.name = "Gb/G WB Gain",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.minimum = 0,
		.maximum = 4095,
		.step = 1,
		.default_value = 512
	},
	{
		.id = CCDC_CID_B_GAIN,
		.name = "B/Mg WB Gain",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.minimum = 0,
		.maximum = 4095,
		.step = 1,
		.default_value = 512
	},
	{
		.id = CCDC_CID_OFFSET,
		.name = "Offset",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.minimum = 0,
		.maximum = 4095,
		.step = 1,
		.default_value = 0
	}
};

/* holds module configuation paramaters */
static struct ccdc_module_params ccdc_hw_module_params_bayer;

static enum ccdc_data_pack data_pack = CCDC_DATA_PACK8;

/* Defauts for module configuation paramaters */
static struct ccdc_module_params ccdc_module_defaults = {
	.linearize = {
		.en = 0,
		.corr_shft = CCDC_NO_SHIFT,
		.scale_fact = {1, 0},
	},
	.df_csc = {
		.df_or_csc = 0,
		.csc = {
			.en = 0
		},
	},
	.dfc = {
		.en = 0
	},
	.bclamp = {
		.en = 0
	},
	.gain_offset = {
		.gain = {
			.r_ye = {1, 0},
			.gr_cy = {1, 0},
			.gb_g = {1, 0},
			.b_mg = {1, 0},
		},
	},
	.culling = {
		.hcpat_odd = 0xff,
		.hcpat_even = 0xff,
		.vcpat = 0xff
	},
	.compress = {
		.alg = CCDC_ALAW,
	},
};

static struct ccdc_std_info ccdc_video_config;

static enum ccdc_hw_if_type ccdc_if_type;
static unsigned long isif_base_reg;
static unsigned long isp5_base_reg;

/* register access routines */
static inline u32 ccdc_read(u32 offset)
{
	return davinci_readl(isif_base_reg + offset);
}

static inline u32 ccdc_write(u32 val, u32 offset)
{
	davinci_writel(val, isif_base_reg + offset);
	return val;
}

/* register access routines */
static inline u32 isp5_read(u32 offset)
{
	return davinci_readl(isp5_base_reg + offset);
}

static inline u32 isp5_write(u32 val, u32 offset)
{
	davinci_writel(val, isp5_base_reg + offset);
	return val;
}

static inline u32 ccdc_merge(u32 mask, u32 val, u32 offset)
{
	u32 addr = isif_base_reg + offset;
	u32 new_val = (davinci_readl(addr) & ~mask) | (val & mask);

	davinci_writel(new_val, addr);
	return new_val;
}

static void ccdc_disable_all_modules(void)
{
	/* disable BC */
	ccdc_write(0, CLAMPCFG);
	/* disable vdfc */
	ccdc_write(0, DFCCTL);
	/* disable CSC */
	ccdc_write(0, CSCCTL);
	/* disable linearization */
	ccdc_write(0, LINCFG0);
	/* disable other modules here as they are supported */
}

static void ccdc_enable(int en)
{
	if (!en) {
		/* Before disable isif, disable all ISIF modules */
		ccdc_disable_all_modules();
		/* wait for next VD. Assume lowest scan rate is 12 Hz. So
		 * 100 msec delay is good enough
		 */
	}
	msleep(100);
	ccdc_merge(0x1, en, SYNCEN);
}

static void ccdc_output_to_sdram(int en)
{
	ccdc_merge((1 << 1), (en << 1), SYNCEN);
}

static void ccdc_config_culling(struct ccdc_cul *cul)
{
	u32 val;

	/* Horizontal pattern */
	val = (cul->hcpat_even) << CULL_PAT_EVEN_LINE_SHIFT;
	val |= cul->hcpat_odd;
	ccdc_write(val, CULH);

	/* vertical pattern */
	ccdc_write(cul->vcpat, CULV);

	/* LPF */
	ccdc_merge((CCDC_LPF_MASK << CCDC_LPF_SHIFT),
		   (cul->en_lpf << CCDC_LPF_SHIFT), MODESET);
}

static void ccdc_config_gain_offset(struct ccdc_gain_offsets_adj
				    *gain_offset)
{
	u32 val;

	val = ((gain_offset->gain_sdram_en & 1) << GAIN_SDRAM_EN_SHIFT);
	val |= ((gain_offset->gain_ipipe_en & 1) << GAIN_IPIPE_EN_SHIFT);
	val |= ((gain_offset->gain_h3a_en & 1) << GAIN_H3A_EN_SHIFT);
	val |= ((gain_offset->offset_sdram_en & 1) << OFST_SDRAM_EN_SHIFT);
	val |= ((gain_offset->offset_ipipe_en & 1) << OFST_IPIPE_EN_SHIFT);
	val |= ((gain_offset->offset_h3a_en & 1) << OFST_H3A_EN_SHIFT);
	ccdc_merge(GAIN_OFFSET_EN_MASK, val, CGAMMAWD);

	val = ((gain_offset->gain.r_ye.integer & GAIN_INTEGER_MASK)
	       << GAIN_INTEGER_SHIFT);
	val |= (gain_offset->gain.r_ye.decimal & GAIN_DECIMAL_MASK);
	ccdc_write(val, CRGAIN);

	val = ((gain_offset->gain.gr_cy.integer & GAIN_INTEGER_MASK)
	       << GAIN_INTEGER_SHIFT);
	val |= (gain_offset->gain.gr_cy.decimal & GAIN_DECIMAL_MASK);
	ccdc_write(val, CGRGAIN);

	val = ((gain_offset->gain.gb_g.integer & GAIN_INTEGER_MASK)
	       << GAIN_INTEGER_SHIFT);
	val |= (gain_offset->gain.gb_g.decimal & GAIN_DECIMAL_MASK);
	ccdc_write(val, CGBGAIN);

	val = ((gain_offset->gain.b_mg.integer & GAIN_INTEGER_MASK)
	       << GAIN_INTEGER_SHIFT);
	val |= (gain_offset->gain.b_mg.decimal & GAIN_DECIMAL_MASK);
	ccdc_write(val, CBGAIN);

	ccdc_write((gain_offset->offset & OFFSET_MASK), COFSTA);
}

static void ccdc_reset(void)
{
	int i;

	/* disable CCDC */
	printk(KERN_DEBUG "\nstarting ccdc_reset...");
	/* Enable clock to ISIF, IPIPEIF and BL */
	vpss_enable_clock(VPSS_CCDC_CLOCK, 1);
	vpss_enable_clock(VPSS_IPIPEIF_CLOCK, 1);
	vpss_enable_clock(VPSS_BL_CLOCK, 1);

	/* set all registers to default value */
	for (i = 0; i <= 0x1f8; i += 4)
		ccdc_write(0, i);

	/* no culling support */
	ccdc_write(0xffff, CULH);
	ccdc_write(0xff, CULV);

	/* Set default offset and gain */
	ccdc_config_gain_offset(&ccdc_module_defaults.gain_offset);

	isp5_write(0x00, ISP5_CCDCMUX);
	printk(KERN_DEBUG "\nEnd of ccdc_reset...");
	/* Enable BL for ISIF */
	vpss_bl1_src_select(VPSS_BL1_SRC_SEL_ISIF);
}

static int ccdc_initialize(void)
{
	davinci_cfg_reg(DM365_VIN_CAM_WEN, PINMUX_RESV);
	davinci_cfg_reg(DM365_VIN_CAM_VD, PINMUX_RESV);
	davinci_cfg_reg(DM365_VIN_CAM_HD, PINMUX_RESV);
	davinci_cfg_reg(DM365_VIN_YIN_EN, PINMUX_RESV);
	ccdc_reset();
	return 0;
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
	struct ccdc_gain_offsets_adj *gain_offset =
	    &ccdc_hw_module_params_bayer.gain_offset;

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

	if (ctrl->value > control->maximum) {
		printk(KERN_ERR "ccdc_queryctrl : Invalid control value\n");
		return -EINVAL;
	}

	if ((ctrl->id == CCDC_CID_R_GAIN) ||
	    (ctrl->id == CCDC_CID_GR_GAIN) ||
	    (ctrl->id == CCDC_CID_GB_GAIN) ||
	    (ctrl->id == CCDC_CID_B_GAIN)) {
		/* Gain adjustment */
		if ((gain_offset->gain_sdram_en != 1) &&
		    (gain_offset->gain_ipipe_en != 1) &&
		    (gain_offset->gain_h3a_en != 1)) {
			printk(KERN_ERR
			       "Gain control disabled in module params\n");
			return -EINVAL;
		}
	} else {
		/* offset adjustment */
		if ((gain_offset->offset_sdram_en != 1) &&
		    (gain_offset->offset_ipipe_en != 1) &&
		    (gain_offset->offset_h3a_en != 1)) {
			printk(KERN_ERR
			       "Offset control disabled in module params\n");
			return -EINVAL;
		}
	}

	switch (ctrl->id) {
	case CCDC_CID_R_GAIN:
		gain_offset->gain.r_ye.integer =
		    (ctrl->value >> GAIN_INTEGER_SHIFT) & GAIN_INTEGER_MASK;
		gain_offset->gain.r_ye.decimal = ctrl->value &
						GAIN_DECIMAL_MASK;
		break;
	case CCDC_CID_GR_GAIN:
		gain_offset->gain.gr_cy.integer =
		    (ctrl->value >> GAIN_INTEGER_SHIFT) & GAIN_INTEGER_MASK;
		gain_offset->gain.gr_cy.decimal = ctrl->value &
						GAIN_DECIMAL_MASK;
		break;
	case CCDC_CID_GB_GAIN:
		gain_offset->gain.gb_g.integer =
		    (ctrl->value >> GAIN_INTEGER_SHIFT) & GAIN_INTEGER_MASK;
		gain_offset->gain.gb_g.decimal = ctrl->value &
						GAIN_DECIMAL_MASK;
		break;

	case CCDC_CID_B_GAIN:
		gain_offset->gain.b_mg.integer =
		    (ctrl->value >> GAIN_INTEGER_SHIFT) & GAIN_INTEGER_MASK;
		gain_offset->gain.b_mg.decimal = ctrl->value &
						GAIN_DECIMAL_MASK;
		break;
	default:
		/* offset */
		gain_offset->offset = ctrl->value & OFFSET_MASK;
	}
	/* set it in hardware */
	ccdc_config_gain_offset(gain_offset);
	return 0;
}

static int ccdc_getcontrol(struct v4l2_control *ctrl)
{
	int i;
	struct v4l2_queryctrl *control = NULL;
	struct ccdc_gain_offsets_adj *gain_offset =
	    &ccdc_hw_module_params_bayer.gain_offset;

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

	if ((ctrl->id == CCDC_CID_R_GAIN) ||
	    (ctrl->id == CCDC_CID_GR_GAIN) ||
	    (ctrl->id == CCDC_CID_GB_GAIN) ||
	    (ctrl->id == CCDC_CID_B_GAIN)) {
		/* Gain adjustment */
		if ((gain_offset->gain_sdram_en != 1) &&
		    (gain_offset->gain_ipipe_en != 1) &&
		    (gain_offset->gain_h3a_en != 1)) {
			printk(KERN_ERR
			       "Gain control disabled in module params\n");
			return -EINVAL;
		}
	} else {
		/* offset adjustment */
		if ((gain_offset->offset_sdram_en != 1) &&
		    (gain_offset->offset_ipipe_en != 1) &&
		    (gain_offset->offset_h3a_en != 1)) {
			printk(KERN_ERR
			       "Offset control disabled in module params\n");
			return -EINVAL;
		}
	}

	switch (ctrl->id) {
	case CCDC_CID_R_GAIN:
		ctrl->value =
		    gain_offset->gain.r_ye.integer << GAIN_INTEGER_SHIFT;
		ctrl->value |= (gain_offset->gain.r_ye.decimal &
					GAIN_DECIMAL_MASK);
		break;
	case CCDC_CID_GR_GAIN:
		ctrl->value =
		    gain_offset->gain.gr_cy.integer << GAIN_INTEGER_SHIFT;
		ctrl->value |=
		    (gain_offset->gain.gr_cy.decimal & GAIN_DECIMAL_MASK);
		break;
	case CCDC_CID_GB_GAIN:
		ctrl->value =
		    gain_offset->gain.gb_g.integer << GAIN_INTEGER_SHIFT;
		ctrl->value |= (gain_offset->gain.gb_g.decimal &
					GAIN_DECIMAL_MASK);
		break;
	case CCDC_CID_B_GAIN:
		ctrl->value =
		    gain_offset->gain.b_mg.integer << GAIN_INTEGER_SHIFT;
		ctrl->value |= (gain_offset->gain.b_mg.decimal &
					GAIN_DECIMAL_MASK);
		break;
	default:
		/* offset */
		ctrl->value = gain_offset->offset & OFFSET_MASK;
	}
	return 0;
}

/* This function will configure the window size to be capture in CCDC reg */
static void ccdc_setwin(struct ccdc_cropwin *image_win,
			enum ccdc_frmfmt frm_fmt, int ppc, int mode)
{
	int horz_start, horz_nr_pixels;
	int vert_start, vert_nr_lines;
	int mid_img = 0;
	printk(KERN_DEBUG "\nStarting ccdc_setwin...");
	/* configure horizonal and vertical starts and sizes */
	horz_start = image_win->left << (ppc - 1);
	horz_nr_pixels = ((image_win->width) << (ppc - 1)) - 1;

	/* Writing the horizontal info into the registers */
	ccdc_write(horz_start & START_PX_HOR_MASK, SPH);
	ccdc_write(horz_nr_pixels & NUM_PX_HOR_MASK, LNH);
	vert_start = image_win->top;

	if (frm_fmt == CCDC_FRMFMT_INTERLACED) {
		vert_nr_lines = (image_win->height >> 1) - 1;
		vert_start >>= 1;
		/* To account for VD since line 0 doesn't have any data */
		vert_start += 1;
	} else {
		/* To account for VD since line 0 doesn't have any data */
		vert_start += 1;
		vert_nr_lines = image_win->height - 1;
		/* configure VDINT0 and VDINT1 */
		mid_img = vert_start + (image_win->height / 2);
		ccdc_write(mid_img, VDINT1);
	}
	if (!mode)
		ccdc_write(0, VDINT0);
	else
		ccdc_write(vert_nr_lines, VDINT0);
	ccdc_write(vert_start & START_VER_ONE_MASK, SLV0);
	ccdc_write(vert_start & START_VER_TWO_MASK, SLV1);
	ccdc_write(vert_nr_lines & NUM_LINES_VER, LNV);
}

static void ccdc_config_bclamp(struct ccdc_black_clamp *bc)
{
	u32 val;

	/* DC Offset is always added to image data irrespective of bc enable
	 * status
	 */
	val = bc->dc_offset & CCDC_BC_DCOFFSET_MASK;
	ccdc_write(val, CLDCOFST);

	if (bc->en) {
		val = (bc->bc_mode_color & CCDC_BC_MODE_COLOR_MASK)
		    << CCDC_BC_MODE_COLOR_SHIFT;

		/* Enable BC */
		val |= 1;

		/****** horizontal clamp caculation paramaters ******/
		val |= (bc->horz.mode & CCDC_HORZ_BC_MODE_MASK)
		    << CCDC_HORZ_BC_MODE_SHIFT;

		ccdc_write(val, CLAMPCFG);

		if (bc->horz.mode != CCDC_HORZ_BC_DISABLE) {
			/* Window count for calculation */
			val =
			    (bc->horz.
			     win_count_calc & CCDC_HORZ_BC_WIN_COUNT_MASK);
			/* Base window selection */
			val |=
			    ((bc->horz.
			      base_win_sel_calc & 1) <<
			     CCDC_HORZ_BC_WIN_SEL_SHIFT);
			/* pixel limit */
			val |=
			    ((bc->horz.
			      clamp_pix_limit & 1) <<
			     CCDC_HORZ_BC_PIX_LIMIT_SHIFT);
			/* Horizontal size of window */
			val |=
			    ((bc->horz.
			      win_h_sz_calc & CCDC_HORZ_BC_WIN_H_SIZE_MASK)
			     << CCDC_HORZ_BC_WIN_H_SIZE_SHIFT);
			/* vertical size of the window */
			val |=
			    ((bc->horz.
			      win_v_sz_calc & CCDC_HORZ_BC_WIN_V_SIZE_MASK)
			     << CCDC_HORZ_BC_WIN_V_SIZE_SHIFT);

			ccdc_write(val, CLHWIN0);

			/* Horizontal start position of the window */
			val =
			    (bc->horz.
			     win_start_h_calc & CCDC_HORZ_BC_WIN_START_H_MASK);
			ccdc_write(val, CLHWIN1);

			/* Vertical start position of the window */
			val =
			    (bc->horz.
			     win_start_v_calc & CCDC_HORZ_BC_WIN_START_V_MASK);
			ccdc_write(val, CLHWIN2);
		}

		/****** vertical clamp caculation paramaters ******/

		/* OB H Valid */
		val = (bc->vert.ob_h_sz_calc & CCDC_VERT_BC_OB_H_SZ_MASK);

		/* Reset clamp value sel for previous line */
		val |= ((bc->vert.reset_val_sel & CCDC_VERT_BC_RST_VAL_SEL_MASK)
			<< CCDC_VERT_BC_RST_VAL_SEL_SHIFT);

		/* Line average coefficient */
		val |=
		    (bc->vert.
		     line_ave_coef << CCDC_VERT_BC_LINE_AVE_COEF_SHIFT);

		ccdc_write(val, CLVWIN0);

		/* Confifigured reset value */
		if (bc->vert.reset_val_sel ==
		    CCDC_VERT_BC_USE_CONFIG_CLAMP_VAL) {
			val =
			    (bc->vert.
			     reset_clamp_val & CCDC_VERT_BC_RST_VAL_MASK);
			ccdc_write(val, CLVRV);
		}

		/* Optical Black horizontal start position */
		val = (bc->vert.ob_start_h & CCDC_VERT_BC_OB_START_HORZ_MASK);
		ccdc_write(val, CLVWIN1);

		/* Optical Black vertical start position */
		val = (bc->vert.ob_start_v & CCDC_VERT_BC_OB_START_VERT_MASK);
		ccdc_write(val, CLVWIN2);

		val = (bc->vert.ob_v_sz_calc & CCDC_VERT_BC_OB_VERT_SZ_MASK);
		ccdc_write(val, CLVWIN3);

		/* Vertical start position for BC subtraction */
		val = (bc->vert_start_sub & CCDC_BC_VERT_START_SUB_V_MASK);
		ccdc_write(val, CLSV);
	}
}

static void ccdc_config_dfc(struct ccdc_dfc *vdfc)
{
	u32 val;
	int i;

	if (!vdfc->en)
		return;

	/* initially reset en bit */
	val = 0 << CCDC_VDFC_EN_SHIFT;

	/* Correction mode */
	val |= ((vdfc->corr_mode & CCDC_VDFC_CORR_MOD_MASK)
		<< CCDC_VDFC_CORR_MOD_SHIFT);

	/* Correct whole line or partial */
	if (vdfc->corr_whole_line)
		val |= 1 << CCDC_VDFC_CORR_WHOLE_LN_SHIFT;

	/* level shift value */
	val |= (vdfc->def_level_shift & CCDC_VDFC_LEVEL_SHFT_MASK)
	    << CCDC_VDFC_LEVEL_SHFT_SHIFT;

	ccdc_write(val, DFCCTL);

	/* Defect saturation level */
	val = vdfc->def_sat_level & CCDC_VDFC_SAT_LEVEL_MASK;
	ccdc_write(val, VDFSATLV);

	ccdc_write(vdfc->table[0].pos_vert & CCDC_VDFC_POS_MASK, DFCMEM0);
	ccdc_write(vdfc->table[0].pos_horz & CCDC_VDFC_POS_MASK, DFCMEM1);
	if (vdfc->corr_mode == CCDC_VDFC_NORMAL ||
	    vdfc->corr_mode == CCDC_VDFC_HORZ_INTERPOL_IF_SAT) {
		ccdc_write(vdfc->table[0].level_at_pos, DFCMEM2);
		ccdc_write(vdfc->table[0].level_up_pixels, DFCMEM3);
		ccdc_write(vdfc->table[0].level_low_pixels, DFCMEM4);
	}

	val = ccdc_read(DFCMEMCTL);
	/* set DFCMARST and set DFCMWR */
	val |= 1 << CCDC_DFCMEMCTL_DFCMARST_SHIFT;
	val |= 1;
	ccdc_write(val, DFCMEMCTL);

	do {
		val = ccdc_read(DFCMEMCTL);
	} while (val & 0x01);

	for (i = 1; i < vdfc->num_vdefects; i++) {
		ccdc_write(vdfc->table[i].pos_vert & CCDC_VDFC_POS_MASK,
			   DFCMEM0);
		ccdc_write(vdfc->table[i].pos_horz & CCDC_VDFC_POS_MASK,
			   DFCMEM1);
		if (vdfc->corr_mode == CCDC_VDFC_NORMAL ||
		    vdfc->corr_mode == CCDC_VDFC_HORZ_INTERPOL_IF_SAT) {
			ccdc_write(vdfc->table[i].level_at_pos, DFCMEM2);
			ccdc_write(vdfc->table[i].level_up_pixels, DFCMEM3);
			ccdc_write(vdfc->table[i].level_low_pixels, DFCMEM4);
		}
		val = ccdc_read(DFCMEMCTL);
		/* clear DFCMARST and set DFCMWR */
		val &= ~(1 << CCDC_DFCMEMCTL_DFCMARST_SHIFT);
		val |= 1;
		ccdc_write(val, DFCMEMCTL);
		do {
			val = ccdc_read(DFCMEMCTL);
		} while (val & 0x01);
	}
	if (vdfc->num_vdefects < CCDC_VDFC_TABLE_SIZE) {
		/* Extra cycle needed */
		ccdc_write(0, DFCMEM0);
		ccdc_write(0x1FFF, DFCMEM1);
		val = 1;
		ccdc_write(val, DFCMEMCTL);
	}

	/* enable VDFC */
	ccdc_merge((1 << CCDC_VDFC_EN_SHIFT), (1 << CCDC_VDFC_EN_SHIFT),
		   DFCCTL);

	ccdc_merge((1 << CCDC_VDFC_EN_SHIFT), (0 << CCDC_VDFC_EN_SHIFT),
		   DFCCTL);

	ccdc_write(0x6, DFCMEMCTL);
	for (i = 0 ; i < vdfc->num_vdefects; i++) {
		do {
			val = ccdc_read(DFCMEMCTL);
		} while (val & 0x2);
		val = ccdc_read(DFCMEM0);
		val = ccdc_read(DFCMEM1);
		val = ccdc_read(DFCMEM2);
		val = ccdc_read(DFCMEM3);
		val = ccdc_read(DFCMEM4);
		ccdc_write(0x2, DFCMEMCTL);
	}
}

static void ccdc_config_csc(struct ccdc_df_csc *df_csc)
{
	u32 val1 = 0, val2 = 0, i;

	if (!df_csc->csc.en) {
		ccdc_write(0, CSCCTL);
		return;
	}
	for (i = 0; i < CCDC_CSC_NUM_COEFF; i++) {
		if ((i % 2) == 0) {
			/* CSCM - LSB */
			val1 =
				(df_csc->csc.coeff[i].integer &
				CCDC_CSC_COEF_INTEG_MASK)
				<< CCDC_CSC_COEF_INTEG_SHIFT;
			val1 |=
				(df_csc->csc.coeff[i].decimal &
			CCDC_CSC_COEF_DECIMAL_MASK);
		} else {

			/* CSCM - MSB */
			val2 =
				(df_csc->csc.coeff[i].integer &
				CCDC_CSC_COEF_INTEG_MASK)
				<< CCDC_CSC_COEF_INTEG_SHIFT;
			val2 |=
				(df_csc->csc.coeff[i].decimal &
				CCDC_CSC_COEF_DECIMAL_MASK);
			val2 <<= CCDC_CSCM_MSB_SHIFT;
			val2 |= val1;
			ccdc_write(val2, (CSCM0 + ((i-1) << 1)));
		}
	}

	/* program the active area */
	ccdc_write(df_csc->start_pix & CCDC_DF_CSC_SPH_MASK, FMTSPH);
	/* one extra pixel as required for CSC. Actually number of
	 * pixel - 1 should be configured in this register. So we
	 * need to subtract 1 before writing to FMTSPH, but we will
	 * not do this since csc requires one extra pixel
	 */
	ccdc_write((df_csc->num_pixels) & CCDC_DF_CSC_SPH_MASK, FMTLNH);
	ccdc_write(df_csc->start_line & CCDC_DF_CSC_SPH_MASK, FMTSLV);
	/* one extra line as required for CSC. See reason documented for
	 * num_pixels
	 */
	ccdc_write((df_csc->num_lines) & CCDC_DF_CSC_SPH_MASK, FMTLNV);

	/* Enable CSC */
	ccdc_write(1, CSCCTL);
}

static void ccdc_config_linearization(struct ccdc_linearize *linearize)
{
	u32 val, i;
	if (!linearize->en) {
		ccdc_write(0, LINCFG0);
		return;
	}

	/* shift value for correction */
	val = (linearize->corr_shft & CCDC_LIN_CORRSFT_MASK)
	    << CCDC_LIN_CORRSFT_SHIFT;
	/* enable */
	val |= 1;
	ccdc_write(val, LINCFG0);

	/* Scale factor */
	val = (linearize->scale_fact.integer & 1)
	    << CCDC_LIN_SCALE_FACT_INTEG_SHIFT;
	val |= (linearize->scale_fact.decimal &
				CCDC_LIN_SCALE_FACT_DECIMAL_MASK);
	ccdc_write(val, LINCFG1);

	for (i = 0; i < CCDC_LINEAR_TAB_SIZE; i++) {
		val = linearize->table[i] & CCDC_LIN_ENTRY_MASK;
		if (i%2)
			davinci_writel(val,
				(CCDC_LINEAR_LUT1_ADDR + ((i >> 1) << 2)));
		else
			davinci_writel(val,
				(CCDC_LINEAR_LUT0_ADDR + ((i >> 1) << 2)));
	}
}

static int ccdc_config_raw(int mode)
{
	struct ccdc_bayer_config *params = &ccdc_hw_params_bayer;
	struct ccdc_module_params *module_params = &ccdc_hw_module_params_bayer;
	u32 val;

	printk(KERN_DEBUG "\nStarting ccdc_config_raw..\n");
	ccdc_reset();

	/********* Configure CCDCFG register *******************************/

	/*Set CCD Not to swap input since input is RAW data */
	val = CCDC_YCINSWP_RAW;

	/*set FID detection function to Latch at V-Sync */
	val |= CCDC_CCDCFG_FIDMD_LATCH_VSYNC;

	/*set WENLOG - ccdc valid area */
	val |= CCDC_CCDCFG_WENLOG_AND;

	/*set TRGSEL */
	val |= CCDC_CCDCFG_TRGSEL_WEN;

	/*set EXTRG */
	val |= CCDC_CCDCFG_EXTRG_DISABLE;


	/* Packed to 8 or 16 bits */
	val |= (data_pack & CCDC_DATA_PACK_MASK);

	ccdc_write(val, CCDCFG);
	printk(KERN_DEBUG "Writing 0x%x to ...CCDCFG \n", val);

	/********* Configure MODESET register *******************************/

	/*Set VDHD direction to input */
	val = CCDC_VDHDOUT_INPUT;

	/*      Configure the vertical sync polarity(MODESET.VDPOL) */
	val |= (params->vd_pol & CCDC_VD_POL_MASK) << CCDC_VD_POL_SHIFT;

	/*      Configure the horizontal sync polarity (MODESET.HDPOL) */
	val |= (params->hd_pol & CCDC_HD_POL_MASK) << CCDC_HD_POL_SHIFT;

	/*      Configure frame id polarity (MODESET.FLDPOL) */
	val |= (params->fid_pol & CCDC_FID_POL_MASK) << CCDC_FID_POL_SHIFT;

	/*      Configure data polarity */
	val |= (CCDC_DATAPOL_NORMAL & CCDC_DATAPOL_MASK) << CCDC_DATAPOL_SHIFT;

	/*      Configure External WEN Selection */
	val |= (CCDC_EXWEN_DISABLE & CCDC_EXWEN_MASK) << CCDC_EXWEN_SHIFT;

	/* Configure frame format(progressive or interlace) */
	val |= (params->frm_fmt & CCDC_FRM_FMT_MASK) << CCDC_FRM_FMT_SHIFT;

	/* Configure pixel format (Input mode) */
	val |= (params->pix_fmt & CCDC_INPUT_MASK) << CCDC_INPUT_SHIFT;

	/*Configure the data shift */
	val |= (params->data_shift & CCDC_DATASFT_MASK)
	    << CCDC_DATASFT_SHIFT;

	ccdc_write(val, MODESET);
	printk(KERN_DEBUG "Writing 0x%x to MODESET...\n", val);

	/********* Configure GAMMAWD register *******************************/
	/* CFA pattern setting */
	val =
	    (params->cfa_pat & CCDC_GAMMAWD_CFA_MASK) << CCDC_GAMMAWD_CFA_SHIFT;

	/* Gamma msb */
	if (module_params->compress.alg == CCDC_ALAW)
		val |= CCDC_ALAW_ENABLE;
	val |= (params->data_msb & CCDC_ALAW_GAMA_WD_MASK)
	    << CCDC_ALAW_GAMA_WD_SHIFT;

	ccdc_write(val, CGAMMAWD);

	/********* Configure DPCM compression settings *******************/
	if (module_params->compress.alg == CCDC_DPCM) {
		val =  1 << CCDC_DPCM_EN_SHIFT;
		val |= (module_params->compress.pred & CCDC_DPCM_PREDICTOR_MASK)
		    << CCDC_DPCM_PREDICTOR_SHIFT;
	}

	ccdc_write(val, MISC);
	/********* Configure Gain & Offset *******************************/

	ccdc_config_gain_offset(&module_params->gain_offset);

	/********* Configure Color pattern *******************************/
	val = (params->col_pat_field0.olop);
	val |= (params->col_pat_field0.olep << 2);
	val |= (params->col_pat_field0.elop << 4);
	val |= (params->col_pat_field0.elep << 6);
	val |= (params->col_pat_field1.olop << 8);
	val |= (params->col_pat_field1.olep << 10);
	val |= (params->col_pat_field1.elop << 12);
	val |= (params->col_pat_field1.elep << 14);
	ccdc_write(val, CCOLP);
	printk(KERN_DEBUG "Writing %x to CCOLP ...\n", val);

	/********* Configure HSIZE register  *****************************/
	val =
	    (params->
	     horz_flip_en & CCDC_HSIZE_FLIP_MASK) << CCDC_HSIZE_FLIP_SHIFT;

	/* calculate line offset in 32 bytes based on pack value */
	if (data_pack == CCDC_PACK_8BIT)
		val |= (((params->win.width + 31) >> 5) & CCDC_LINEOFST_MASK);
	else if (data_pack == CCDC_PACK_12BIT)
		val |= ((((params->win.width +
			   (params->win.width >> 2)) +
			  31) >> 5) & CCDC_LINEOFST_MASK);
	else
		val |=
		    ((((params->win.width * 2) +
		       31) >> 5) & CCDC_LINEOFST_MASK);
	ccdc_write(val, HSIZE);

	/********* Configure SDOFST register  *****************************/
	if (params->frm_fmt == CCDC_FRMFMT_INTERLACED) {
		if (params->image_invert_en) {
			/* For interlace inverse mode */
			ccdc_write(0x4B6D, SDOFST);
			printk(KERN_DEBUG "Writing 0x4B6D to SDOFST...\n");
		} else {
			/* For interlace non inverse mode */
			ccdc_write(0x0B6D, SDOFST);
			printk(KERN_DEBUG "Writing 0x0B6D to SDOFST...\n");
		}
	} else if (params->frm_fmt == CCDC_FRMFMT_PROGRESSIVE) {
		if (params->image_invert_en) {
			/* For progessive inverse mode */
			ccdc_write(0x4000, SDOFST);
			printk(KERN_DEBUG "Writing 0x4000 to SDOFST...\n");
		} else {
			/* For progessive non inverse mode */
			ccdc_write(0x0000, SDOFST);
			printk(KERN_DEBUG "Writing 0x0000 to SDOFST...\n");
		}
	}

	/********* Configure video window ********************************/
	ccdc_setwin(&params->win, params->frm_fmt, 1, mode);

	/********* Configure Black Clamp *********************************/
	ccdc_config_bclamp(&module_params->bclamp);

	/********* Configure Vertical Defection Pixel Correction **********/
	ccdc_config_dfc(&module_params->dfc);

	if (!module_params->df_csc.df_or_csc)
		/********* Configure Color Space Conversion *******************/
		ccdc_config_csc(&module_params->df_csc);

	/********* Configure Linearization table  *************************/
	ccdc_config_linearization(&module_params->linearize);

	/********* Configure Culling **************************************/
	ccdc_config_culling(&module_params->culling);

	/******* Configure Horizontal and vertical offsets(DFC,LSC,Gain) **/
	val = module_params->horz_offset & CCDC_DATA_H_OFFSET_MASK;
	ccdc_write(val, DATAHOFST);

	val = module_params->vert_offset & CCDC_DATA_V_OFFSET_MASK;
	ccdc_write(val, DATAVOFST);

	/* Setup test pattern if enabled */
	if (params->test_pat_gen) {
		/* configure pattern register  */
		val = (CCDC_PG_EN | CCDC_SEL_PG_SRC);

		/* Use the HD/VD pol settings from user */
		val |=
		    ((params->
		      hd_pol & CCDC_HD_POL_MASK) << CCDC_PG_HD_POL_SHIFT);
		val |=
		    ((params->
		      vd_pol & CCDC_VD_POL_MASK) << CCDC_PG_VD_POL_SHIFT);

		isp5_write(val, ISP5_CCDCMUX);

		/* Set hlpfr */
		val = ((ccdc_video_config.activelines >> 1) - 1) << 16;
		val |= (ccdc_video_config.activepixels - 1);
		isp5_write(val, ISP5_PG_FRAME_SIZE);
	} else
	printk(KERN_DEBUG "\nEnd of ccdc_config_ycbcr...\n");
	return 0;
}

static int ccdc_validate_linearize_params(struct ccdc_linearize *linearize)
{
	int err = -EINVAL;
	int i;

	if (linearize->en) {
		if (linearize->corr_shft < CCDC_NO_SHIFT
		    || linearize->corr_shft > CCDC_6BIT_SHIFT) {
			printk(KERN_ERR "invalid corr_shft value\n");
			return err;
		}

		if (linearize->scale_fact.integer > 1 ||
		    linearize->scale_fact.decimal >
			CCDC_LIN_SCALE_FACT_DECIMAL_MASK) {
			/* U11Q10 */
			printk(KERN_ERR
			       "invalid linearize scale facror value\n");
			return err;
		}
		for (i = 0; i < CCDC_LINEAR_TAB_SIZE; i++) {
			if (linearize->table[i] > CCDC_LIN_ENTRY_MASK) {
				printk(KERN_ERR
				       "invalid linearize lut value\n");
				return err;
			}
		}
	}
	return 0;
}

static int ccdc_validate_df_csc_params(struct ccdc_df_csc *df_csc)
{
	int err = -EINVAL;
	int i, csc_df_en = 0;
	struct ccdc_color_space_conv *csc;

	if (!df_csc->df_or_csc) {
		/* csc configuration */
		csc = &df_csc->csc;
		if (csc->en) {
			csc_df_en = 1;
			for (i = 0; i < CCDC_CSC_NUM_COEFF; i++) {
				if (csc->coeff[i].integer >
					CCDC_CSC_COEF_INTEG_MASK ||
				    csc->coeff[i].decimal >
					CCDC_CSC_COEF_DECIMAL_MASK) {
					printk(KERN_ERR
					       "invalid csc coefficients \n");
					return err;
				}
			}
		}
	}

	if (df_csc->start_pix > 0x1fff) {
		printk(KERN_ERR "invalid df_csc start pix value \n");
		return err;
	}
	if (df_csc->num_pixels > 0x1fff) {
		printk(KERN_ERR "invalid df_csc num pixels value \n");
		return err;
	}
	if (df_csc->start_line > 0x1fff) {
		printk(KERN_ERR "invalid df_csc start_line value \n");
		return err;
	}
	if (df_csc->num_lines > 0x7fff) {
		printk(KERN_ERR "invalid df_csc num_lines value \n");
		return err;
	}
	return 0;
}

static int ccdc_validate_dfc_params(struct ccdc_dfc *dfc)
{
	int err = -EINVAL;
	int i;

	if (dfc->en) {
		if (dfc->corr_whole_line > 1) {
			printk(KERN_ERR "invalid corr_whole_line value \n");
			return err;
		}

		if (dfc->def_level_shift > 4) {
			printk(KERN_ERR "invalid def_level_shift value \n");
			return err;
		}

		if (dfc->def_sat_level > 4095) {
			printk(KERN_ERR "invalid def_sat_level value \n");
			return err;
		}
		if ((!dfc->num_vdefects) || (dfc->num_vdefects > 8)) {
			printk(KERN_ERR "invalid num_vdefects value \n");
			return err;
		}
		for (i = 0; i < CCDC_VDFC_TABLE_SIZE; i++) {
			if (dfc->table[i].pos_vert > 0x1fff) {
				printk(KERN_ERR "invalid pos_vert value \n");
				return err;
			}
			if (dfc->table[i].pos_horz > 0x1fff) {
				printk(KERN_ERR "invalid pos_horz value \n");
				return err;
			}
		}
	}
	return 0;
}

static int ccdc_validate_bclamp_params(struct ccdc_black_clamp *bclamp)
{
	int err = -EINVAL;

	if (bclamp->dc_offset > 0x1fff) {
		printk(KERN_ERR "invalid bclamp dc_offset value \n");
		return err;
	}

	if (bclamp->en) {
		if (bclamp->horz.clamp_pix_limit > 1) {
			printk(KERN_ERR
			       "invalid bclamp horz clamp_pix_limit value \n");
			return err;
		}

		if (bclamp->horz.win_count_calc < 1 ||
		    bclamp->horz.win_count_calc > 32) {
			printk(KERN_ERR
			       "invalid bclamp horz win_count_calc value \n");
			return err;
		}

		if (bclamp->horz.win_start_h_calc > 0x1fff) {
			printk(KERN_ERR
			       "invalid bclamp win_start_v_calc value \n");
			return err;
		}

		if (bclamp->horz.win_start_v_calc > 0x1fff) {
			printk(KERN_ERR
			       "invalid bclamp win_start_v_calc value \n");
			return err;
		}

		if (bclamp->vert.reset_clamp_val > 0xfff) {
			printk(KERN_ERR
			       "invalid bclamp reset_clamp_val value \n");
			return err;
		}

		if (bclamp->vert.ob_v_sz_calc > 0x1fff) {
			printk(KERN_ERR "invalid bclamp ob_v_sz_calc value \n");
			return err;
		}

		if (bclamp->vert.ob_start_h > 0x1fff) {
			printk(KERN_ERR "invalid bclamp ob_start_h value \n");
			return err;
		}

		if (bclamp->vert.ob_start_v > 0x1fff) {
			printk(KERN_ERR "invalid bclamp ob_start_h value \n");
			return err;
		}
	}
	return 0;
}

static int ccdc_validate_gain_ofst_params(struct ccdc_gain_offsets_adj
					  *gain_offset)
{
	int err = -EINVAL;

	if (gain_offset->gain_sdram_en ||
	    gain_offset->gain_ipipe_en ||
	    gain_offset->gain_h3a_en) {
		if ((gain_offset->gain.r_ye.integer > 7) ||
		    (gain_offset->gain.r_ye.decimal > 0x1ff)) {
			printk(KERN_ERR "invalid  gain r_ye\n");
			return err;
		}
		if ((gain_offset->gain.gr_cy.integer > 7) ||
		    (gain_offset->gain.gr_cy.decimal > 0x1ff)) {
			printk(KERN_ERR "invalid  gain gr_cy\n");
			return err;
		}
		if ((gain_offset->gain.gb_g.integer > 7) ||
		    (gain_offset->gain.gb_g.decimal > 0x1ff)) {
			printk(KERN_ERR "invalid  gain gb_g\n");
			return err;
		}
		if ((gain_offset->gain.b_mg.integer > 7) ||
		    (gain_offset->gain.b_mg.decimal > 0x1ff)) {
			printk(KERN_ERR "invalid  gain b_mg\n");
			return err;
		}
	}
	if (gain_offset->offset_sdram_en ||
	    gain_offset->offset_ipipe_en ||
	    gain_offset->offset_h3a_en) {
		if (gain_offset->offset > 0xfff) {
			printk(KERN_ERR "invalid  gain b_mg\n");
			return err;
		}
	}
	return 0;
}

static int ccdc_validate_bayer_cfg_params(struct ccdc_param *param)
{
	int err = 0;
	struct ccdc_bayer_config *cfg = &param->cfg.bayer;
	struct ccdc_module_params *module_params;

	err = -EINVAL;
	/* validate and save config data */
	if (cfg->pix_fmt != CCDC_PIXFMT_RAW) {
		printk(KERN_ERR "Invalid pix format\n");
		return err;
	}

	if (cfg->frm_fmt != CCDC_FRMFMT_PROGRESSIVE) {
		printk(KERN_ERR "Invalid frame format\n");
		return err;
	}

	if (cfg->fid_pol != CCDC_PINPOL_POSITIVE &&
	    cfg->fid_pol != CCDC_PINPOL_NEGATIVE) {
		printk(KERN_ERR "Invalid value of field id polarity\n");
		return err;
	}

	if (cfg->vd_pol != CCDC_PINPOL_POSITIVE &&
	    cfg->vd_pol != CCDC_PINPOL_NEGATIVE) {
		printk(KERN_ERR "Invalid value of VD polarity\n");
		return err;
	}

	if (cfg->hd_pol != CCDC_PINPOL_POSITIVE &&
	    cfg->hd_pol != CCDC_PINPOL_NEGATIVE) {
		printk(KERN_ERR "Invalid value of HD polarity\n");
		return err;
	}

	if (cfg->data_size > CCDC_16_BITS || cfg->data_size < CCDC_8_BITS) {
		printk(KERN_ERR "Invalid value of data size\n");
		return err;
	}

	if (cfg->data_shift < CCDC_NO_SHIFT ||
	    cfg->data_shift > CCDC_4BIT_SHIFT) {
		printk(KERN_ERR "Invalid value of data shift\n");
		return err;
	}

	if (cfg->test_pat_gen > 1) {
		printk(KERN_ERR "Invalid value of test_pat_gen\n");
		return err;
	}

	err = 0;
	if (ISNULL(param->module_params)) {
		printk(KERN_ERR "Invalid user ptr for module params\n");
		return err;
	} else {
		module_params = kmalloc(sizeof(struct ccdc_module_params),
					GFP_KERNEL);
		if (ISNULL(module_params)) {
			printk(KERN_ERR "memory allocation failure\n");
			return -EFAULT;
		}
		if (copy_from_user(module_params,
				   param->module_params,
				   sizeof(struct ccdc_module_params))) {
			printk(KERN_ERR
			       "error in copying ccdc params to kernel\n");
			kfree(module_params);
			return -EFAULT;
		}

		err |=
		    ccdc_validate_linearize_params(&module_params->linearize);
		err |= ccdc_validate_df_csc_params(&module_params->df_csc);
		err |= ccdc_validate_dfc_params(&module_params->dfc);
		err |= ccdc_validate_bclamp_params(&module_params->bclamp);
		err |=
		    ccdc_validate_gain_ofst_params(&module_params->gain_offset);
		if (err) {
			kfree(module_params);
			return err;
		} else {
			memcpy(&ccdc_hw_module_params_bayer,
			       module_params,
			       sizeof(struct ccdc_module_params));
		}

		kfree(module_params);
	}
	memcpy(&ccdc_hw_params_bayer, cfg, sizeof(struct ccdc_bayer_config));
	return err;
}

static int ccdc_validate_ycbcr_cfg_params(struct ccdc_param *param)
{
	int err = -EINVAL;
	struct ccdc_ycbcr_config *cfg = &param->cfg.ycbcr;

	if (cfg->pix_fmt != CCDC_PIXFMT_YCBCR_8BIT &&
	    cfg->pix_fmt != CCDC_PIXFMT_YCBCR_16BIT) {
		printk(KERN_ERR "Invalid pix format\n");
		return err;
	}

	if (cfg->frm_fmt != CCDC_FRMFMT_PROGRESSIVE &&
	    cfg->frm_fmt != CCDC_FRMFMT_INTERLACED) {
		printk(KERN_ERR "Invalid frame format\n");
		return err;
	}

	if (cfg->fid_pol != CCDC_PINPOL_POSITIVE &&
	    cfg->fid_pol != CCDC_PINPOL_NEGATIVE) {
		printk(KERN_ERR "Invalid value of field id polarity\n");
		return err;
	}

	if (cfg->vd_pol != CCDC_PINPOL_POSITIVE &&
	    cfg->vd_pol != CCDC_PINPOL_NEGATIVE) {
		printk(KERN_ERR "Invalid value of VD polarity\n");
		return err;
	}

	if (cfg->hd_pol != CCDC_PINPOL_POSITIVE &&
	    cfg->hd_pol != CCDC_PINPOL_NEGATIVE) {
		printk(KERN_ERR "Invalid value of HD polarity\n");
		return err;
	}

	if (cfg->pix_order != CCDC_PIXORDER_YCBYCR &&
	    cfg->pix_order != CCDC_PIXORDER_CBYCRY) {
		printk(KERN_ERR "Invalid value of pixel order\n");
		return err;
	}

	if (cfg->buf_type != CCDC_BUFTYPE_FLD_INTERLEAVED &&
	    cfg->buf_type != CCDC_BUFTYPE_FLD_SEPARATED) {
		printk(KERN_ERR "Invalid value of buffer type\n");
		return err;
	}

	if (cfg->test_pat_gen > 1) {
		printk(KERN_ERR "Invalid value of test_pat_gen\n");
		return err;
	}
	memcpy(&ccdc_hw_params_ycbcr, cfg, sizeof(struct ccdc_ycbcr_config));
	return 0;
}

static int ccdc_setparams(void *param)
{
	struct ccdc_param *input_param;
	int err = -EINVAL;

	if (ISNULL(param)) {
		printk(KERN_ERR "Null User ptr for param \n");
		return err;
	}

	input_param = kmalloc(sizeof(struct ccdc_param), GFP_KERNEL);
	if (ISNULL(input_param)) {
		printk(KERN_ERR "memory allocation failed\n");
		return -EFAULT;
	}
	if (copy_from_user(input_param,
			   (struct ccdc_param *)param,
			   sizeof(struct ccdc_param))) {
		printk(KERN_ERR "error in copying ccdc params to kernel\n");
		return -EFAULT;
	}
	if (!input_param->if_type) {
		if (ccdc_if_type != CCDC_RAW_BAYER) {
			printk(KERN_ERR
			       "if_type doesn't match with current"
			       " interface\n");
			return err;
		}
	} else {
		if (ccdc_if_type == CCDC_RAW_BAYER)
			printk(KERN_ERR
			       "if_type doesn't match with current"
			       " interface\n");
		return err;
	}

	err = 0;
	if (input_param->if_type == 0)
		err = ccdc_validate_bayer_cfg_params(input_param);
	else
		err = ccdc_validate_ycbcr_cfg_params(input_param);

	kfree(input_param);
	return err;
}

static int ccdc_set_buftype(enum ccdc_buftype buf_type)
{
	if (ccdc_if_type == CCDC_RAW_BAYER)
		ccdc_hw_params_bayer.buf_type = buf_type;
	else
		ccdc_hw_params_ycbcr.buf_type = buf_type;
	return 0;

}
static int ccdc_get_buftype(enum ccdc_buftype *buf_type)
{
	if (ccdc_if_type == CCDC_RAW_BAYER)
		*buf_type = ccdc_hw_params_bayer.buf_type;
	else
		*buf_type = ccdc_hw_params_ycbcr.buf_type;
	return 0;
}

static int ccdc_set_pixel_format(unsigned int pixfmt)
{
	if (ccdc_if_type == CCDC_RAW_BAYER) {
		if (pixfmt == V4L2_PIX_FMT_SBGGR8) {
			if ((ccdc_hw_module_params_bayer.compress.alg !=
					CCDC_ALAW) &&
			    (ccdc_hw_module_params_bayer.compress.alg !=
					CCDC_DPCM)) {
				printk(KERN_ERR "Either configure A-Law or"
						"DPCM\n");
				return -1;
			}
			data_pack = CCDC_PACK_8BIT;
		} else if (pixfmt == V4L2_PIX_FMT_SBGGR16) {
			if (ccdc_hw_module_params_bayer.compress.alg !=
					CCDC_NO_COMPRESSION) {
				printk(KERN_ERR "Disable compression"
						" for this pixel format\n");
				return -1;
			}
			data_pack = CCDC_PACK_16BIT;
		} else
			return -1;
		ccdc_hw_params_bayer.pix_fmt = CCDC_PIXFMT_RAW;
	} else {
		if (pixfmt == V4L2_PIX_FMT_YUYV)
			ccdc_hw_params_ycbcr.pix_order = CCDC_PIXORDER_YCBYCR;
		else if (pixfmt == V4L2_PIX_FMT_UYVY)
			ccdc_hw_params_ycbcr.pix_order = CCDC_PIXORDER_CBYCRY;
		else
			return -1;
		data_pack = CCDC_PACK_8BIT;
	}
	return 0;
}

static int ccdc_get_pixel_format(unsigned int *pixfmt)
{
	if (ccdc_if_type == CCDC_RAW_BAYER) {
		if (data_pack == CCDC_PACK_8BIT)
			*pixfmt = V4L2_PIX_FMT_SBGGR8;
		else if (data_pack == CCDC_PACK_16BIT)
			*pixfmt = V4L2_PIX_FMT_SBGGR16;
		else
			return -1;
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
		ccdc_hw_params_bayer.win.top = win->top;
		ccdc_hw_params_bayer.win.left = win->left;
		ccdc_hw_params_bayer.win.width = win->width;
		ccdc_hw_params_bayer.win.height = win->height;
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
		win->top = ccdc_hw_params_bayer.win.top;
		win->left = ccdc_hw_params_bayer.win.left;
		win->width = ccdc_hw_params_bayer.win.width;
		win->height = ccdc_hw_params_bayer.win.height;
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
		if (data_pack == CCDC_PACK_8BIT)
			*len = ((ccdc_hw_params_bayer.win.width + 31) & ~0x1F);
		else if (data_pack == CCDC_PACK_12BIT)
			*len = (((ccdc_hw_params_bayer.win.width * 2) +
				 (ccdc_hw_params_bayer.win.width >> 2) + 31) &
				 ~0x1F);
		else
			*len = (((ccdc_hw_params_bayer.win.width * 2) + 31) &
				~0x1F);
	} else
		*len = (((ccdc_hw_params_ycbcr.win.width * 2) + 31) & ~0x1F);
	return 0;
}

static int ccdc_set_frame_format(enum ccdc_frmfmt frm_fmt)
{
	if (ccdc_if_type == CCDC_RAW_BAYER)
		ccdc_hw_params_bayer.frm_fmt = frm_fmt;
	else
		ccdc_hw_params_ycbcr.frm_fmt = frm_fmt;
	return 0;
}
static int ccdc_get_frame_format(enum ccdc_frmfmt *frm_fmt)
{
	if (ccdc_if_type == CCDC_RAW_BAYER)
		*frm_fmt = ccdc_hw_params_bayer.frm_fmt;
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
	if (ccdc_if_type == CCDC_RAW_BAYER) {
		if (ccdc_hw_module_params_bayer.compress.alg == CCDC_ALAW ||
		    ccdc_hw_module_params_bayer.compress.alg == CCDC_DPCM)
			ccdc_set_pixel_format(V4L2_PIX_FMT_SBGGR8);
		else
			ccdc_set_pixel_format(V4L2_PIX_FMT_SBGGR16);
	} else
		ccdc_set_pixel_format(V4L2_PIX_FMT_UYVY);
	data_pack = CCDC_PACK_8BIT;
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
	return (ccdc_read(MODESET) >> 15) & 0x1;
}

/* misc operations */
static void ccdc_setfbaddr(unsigned long addr)
{
	ccdc_write((addr >> 21) & 0x07ff, CADU);
	ccdc_write((addr >> 5) & 0x0ffff, CADL);
}

static int ccdc_set_hw_if_type(enum ccdc_hw_if_type iface)
{
	ccdc_if_type = iface;

	switch (ccdc_if_type) {
	case CCDC_BT656:
	case CCDC_BT656_10BIT:
	case CCDC_YCBCR_SYNC_8:
		ccdc_hw_params_ycbcr.pix_fmt = CCDC_PIXFMT_YCBCR_8BIT;
		ccdc_hw_params_ycbcr.pix_order = CCDC_PIXORDER_CBYCRY;
		break;
	case CCDC_BT1120:
	case CCDC_YCBCR_SYNC_16:
		ccdc_hw_params_ycbcr.pix_fmt = CCDC_PIXFMT_YCBCR_16BIT;
		ccdc_hw_params_ycbcr.pix_order = CCDC_PIXORDER_CBYCRY;
		break;
	case CCDC_RAW_BAYER:
		ccdc_hw_params_bayer.pix_fmt = CCDC_PIXFMT_RAW;
		break;
	default:
		printk(KERN_ERR "Invalid interface type\n");
		return -1;
	}
	return 0;
}

static int ccdc_getparams(void *param)
{
	int err = -EINVAL;
	struct ccdc_param *input_param;

	if (ISNULL(param)) {
		printk(KERN_ERR "Null User ptr for param \n");
		return err;
	}

	input_param = kmalloc(sizeof(struct ccdc_param), GFP_KERNEL);
	if (ISNULL(input_param)) {
		printk(KERN_ERR "memory allocation failed\n");
		return -EFAULT;
	}

	/* to get module_params user address */
	if (copy_from_user(input_param,
			   (struct ccdc_param *)param,
			   sizeof(struct ccdc_param))) {
		printk(KERN_ERR "error in copying ccdc params to kernel\n");
		return -EFAULT;
	}

	if (ccdc_if_type == CCDC_RAW_BAYER) {
		input_param->if_type = 0;
		if (ISNULL(input_param->module_params)) {
			printk(KERN_ERR "Invalid user ptr for module_params\n");
			return err;
		}
		memcpy(&input_param->cfg.bayer,
		       &ccdc_hw_params_bayer, sizeof(struct ccdc_bayer_config));

		if (copy_to_user(input_param->module_params,
				 &ccdc_hw_module_params_bayer,
				 sizeof(struct ccdc_module_params))) {
			printk(KERN_ERR
			       "error in copying module params "
			       "to user space\n");
			return -EFAULT;
		}
	} else {
		input_param->if_type = 1;
		memcpy(&input_param->cfg.ycbcr,
		       &ccdc_hw_params_ycbcr, sizeof(struct ccdc_ycbcr_config));
	}

	if (copy_to_user(param, input_param, sizeof(struct ccdc_param))) {
		printk(KERN_ERR "error in copying ccdc params to user space\n");
		return -EFAULT;
	}
	kfree(input_param);
	return 0;
}

/* This function will configure CCDC for YCbCr parameters. */
static int ccdc_config_ycbcr(int mode)
{
	u32 modeset = 0, ccdcfg = 0, val = 0;
	struct ccdc_ycbcr_config *params = &ccdc_hw_params_ycbcr;

	/* first reset the CCDC                                          */
	/* all registers have default values after reset                 */
	/* This is important since we assume default values to be set in */
	/* a lot of registers that we didn't touch                       */
	printk(KERN_DEBUG "\nStarting ccdc_config_ycbcr...");
	ccdc_reset();

	/* configure pixel format or input mode */
	modeset |= (params->pix_fmt & CCDC_INPUT_MASK) << CCDC_INPUT_SHIFT;

	/* configure video frame format */
	modeset |= (params->frm_fmt & CCDC_FRM_FMT_MASK) << CCDC_FRM_FMT_SHIFT;

	modeset |=
	    ((params->
	      fid_pol & CCDC_FID_POL_MASK) << CCDC_FID_POL_SHIFT);
	modeset |=
	    ((params->hd_pol & CCDC_HD_POL_MASK) << CCDC_HD_POL_SHIFT);
	modeset |=
	    ((params->vd_pol & CCDC_VD_POL_MASK) << CCDC_VD_POL_SHIFT);

	/* pack the data to 8-bit CCDCCFG */
	switch (ccdc_if_type) {
	case CCDC_BT656:
		if (params->pix_fmt != CCDC_PIXFMT_YCBCR_8BIT) {
			printk(KERN_ERR "Invalid pix_fmt(input mode)\n");
			return -1;
		}
		/* override the polarity to negative */
		/* setup BT.656, embedded sync  */
		modeset |=
			((CCDC_PINPOL_NEGATIVE & CCDC_VD_POL_MASK) <<
			CCDC_VD_POL_SHIFT);
		ccdc_write(3, REC656IF);
		ccdcfg |= CCDC_DATA_PACK8;
		ccdcfg |= CCDC_YCINSWP_YCBCR;
		break;
	case CCDC_BT656_10BIT:
		if (params->pix_fmt != CCDC_PIXFMT_YCBCR_8BIT) {
			printk(KERN_ERR "Invalid pix_fmt(input mode)\n");
			return -1;
		}
		/* setup BT.656, embedded sync  */
		ccdc_write(3, REC656IF);
		/* enable 10 bit mode in ccdcfg */
		ccdcfg |= CCDC_DATA_PACK8;
		ccdcfg |= CCDC_YCINSWP_YCBCR;
		ccdcfg |= CCDC_BW656_ENABLE;
		break;
	case CCDC_BT1120:
		if (params->pix_fmt != CCDC_PIXFMT_YCBCR_16BIT) {
			printk(KERN_ERR "Invalid pix_fmt(input mode)\n");
			return -1;
		}
		ccdc_write(3, REC656IF);
		break;

	case CCDC_YCBCR_SYNC_8:
		ccdcfg |= CCDC_DATA_PACK8;
		ccdcfg |= CCDC_YCINSWP_YCBCR;
		if (params->pix_fmt != CCDC_PIXFMT_YCBCR_8BIT) {
			printk(KERN_ERR "Invalid pix_fmt(input mode)\n");
			return -1;
		}
		break;
	case CCDC_YCBCR_SYNC_16:
		if (params->pix_fmt != CCDC_PIXFMT_YCBCR_16BIT) {
			printk(KERN_ERR "Invalid pix_fmt(input mode)\n");
			return -1;
		}
		break;
	default:
		/* should never come here */
		printk(KERN_ERR "Invalid interface type\n");
		return -1;
	}

	ccdc_write(modeset, MODESET);

	/* Set up pix order */
	ccdcfg |= (params->pix_order & CCDC_PIX_ORDER_MASK)
	    << CCDC_PIX_ORDER_SHIFT;

	ccdc_write(ccdcfg, CCDCFG);

	/* configure video window */
	if ((ccdc_if_type == CCDC_BT1120) ||
	    (ccdc_if_type == CCDC_YCBCR_SYNC_16))
		ccdc_setwin(&params->win, params->frm_fmt, 1, mode);
	else
		ccdc_setwin(&params->win, params->frm_fmt, 2, mode);

	/* configure the horizontal line offset */
	/* this is done by rounding up width to a multiple of 16 pixels */
	/* and multiply by two to account for y:cb:cr 4:2:2 data */
	ccdc_write(((((params->win.width * 2) + 31) & 0xffffffe0) >> 5), HSIZE);

	/* configure the memory line offset */
	if ((params->frm_fmt == CCDC_FRMFMT_INTERLACED) &&
	    (params->buf_type == CCDC_BUFTYPE_FLD_INTERLEAVED)) {
		/* two fields are interleaved in memory */
		ccdc_write(0x00000249, SDOFST);
	}
	/* Setup test pattern if enabled */
	if (params->test_pat_gen) {
		/* Select MMR clock */
		/* configure pattern register  */
		val |= (CCDC_PG_EN | CCDC_SEL_PG_SRC);

		/* Use the HD/VD pol settings from user */
		val |=
		    ((params->
		      hd_pol & CCDC_HD_POL_MASK) << CCDC_PG_HD_POL_SHIFT);
		val |=
		    ((params->
		      vd_pol & CCDC_VD_POL_MASK) << CCDC_PG_VD_POL_SHIFT);

		isp5_write(val, ISP5_CCDCMUX);

		/* Set hlpfr */
		val = ((ccdc_video_config.activelines >> 1) - 1) << 16;
		val |= (ccdc_video_config.activepixels - 1);
		isp5_write(val, ISP5_PG_FRAME_SIZE);
	}
	return 0;
}

static int ccdc_configure(int mode)
{
	if (ccdc_if_type == CCDC_RAW_BAYER)
		return ccdc_config_raw(mode);
	else
		return ccdc_config_ycbcr(mode);
}

static int ccdc_get_hw_if_params(struct ccdc_hw_if_param *param)
{
	if (ISNULL(param))
		return -1;

	param->if_type = ccdc_if_type;
	if (ccdc_if_type == CCDC_RAW_BAYER) {
		param->hdpol = ccdc_hw_params_bayer.hd_pol;
		param->vdpol = ccdc_hw_params_bayer.vd_pol;
	} else {
		param->hdpol = ccdc_hw_params_ycbcr.hd_pol;
		param->vdpol = ccdc_hw_params_ycbcr.vd_pol;
	}
	return 0;
}
static int ccdc_init(void)
{
	isif_base_reg = ISIF_IOBASE_ADDR;
	isp5_base_reg = ISP5_IOBASE_ADDR;
	memcpy(&ccdc_hw_module_params_bayer,
	       &ccdc_module_defaults,
	       sizeof(struct ccdc_module_params));
	return 0;
}

static int ccdc_deinitialize(void)
{
	/* copy defaults to module params */
	memcpy(&ccdc_hw_module_params_bayer,
	       &ccdc_module_defaults,
	       sizeof(struct ccdc_module_params));
	return 0;
}

static struct ccdc_hw_interface dm365_ccdc_hw_if = {
	.name = "DM365 ISIF",
	.initialize = ccdc_initialize,
	.deinitialize = ccdc_deinitialize,
	.enable = ccdc_enable,
	.enable_out_to_sdram = ccdc_output_to_sdram,
	.set_hw_if_type = ccdc_set_hw_if_type,
	.get_hw_if_params = ccdc_get_hw_if_params,
	.setparams = ccdc_setparams,
	.getparams = ccdc_getparams,
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
	return &dm365_ccdc_hw_if;
}
EXPORT_SYMBOL(ccdc_get_hw_interface);

static void ccdc_exit(void)
{
}

subsys_initcall(ccdc_init);
module_exit(ccdc_exit);

MODULE_LICENSE("GPL");
