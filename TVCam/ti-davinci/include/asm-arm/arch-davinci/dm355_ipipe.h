/*
 * Copyright (C) 2005-2008 Texas Instruments Inc
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
 * Feature description
 * ===================
 *
 * VPFE hardware setup
 *
 * case 1: Capture to SDRAM with out IPIPE
 * ****************************************
 *
 *            parallel
 *                port
 *
 * Image sensor/       ________
 * Yuv decoder    ---->| CCDC |--> SDRAM
 *                     |______|
 *
 * case 2: Capture to SDRAM with IPIPE Preview modules in Continuous
 *          (On the Fly mode)
 *
 * Image sensor/       ________    ____________________
 * Yuv decoder    ---->| CCDC |--> | Previewer modules |--> SDRAM
 *                     |______|    |___________________|
 *
 * case 3: Capture to SDRAM with IPIPE Preview modules  & Resizer
 *         in continuous (On the Fly mode)
 *
 * Image sensor/       ________    _____________   ___________
 * Yuv decoder    ---->| CCDC |--> | Previewer  |->| Resizer  |-> SDRAM
 *                     |______|    |____________|  |__________|
 *
 * case 4: Capture to SDRAM with IPIPE Resizer
 *         in continuous (On the Fly mode)
 *
 * Image sensor/       ________    ___________
 * Yuv decoder    ---->| CCDC |--> | Resizer  |-> SDRAM
 *                     |______|    |__________|
 *
 * case 5: Read from SDRAM and do preview and/or Resize
 *         in Single shot mode
 *
 *                   _____________   ___________
 *    SDRAM   ----> | Previewer  |->| Resizer  |-> SDRAM
 *                  |____________|  |__________|
 *
 *
 * Previewer allows fine tuning of the input image using different
 * tuning modules in IPIPE. Some examples :- Noise filter, Defect
 * pixel correction etc. It essentially operate on Bayer Raw data
 * or YUV raw data. To do image tuning, application call,
 * PREV_QUERY_CAP, and then call PREV_SET_PARAM to set parameter
 * for a module.
 *
 *
 * Resizer allows upscaling or downscaling a image to a desired
 * resolution. There are 2 resizer modules. both operating on the
 * same input image, but can have different output resolution.
 * Resizer 0 has a maximum line size of 1344 pixels. So
 * input and output size is limited to this size.
 * Resizer 1 has a maximum line size of 640 pixels
 */

#ifndef DM355_IPIPE_H
#define DM355_IPIPE_H

struct ipipe_float_u16q16 {
	unsigned short integer;
	unsigned short decimal;
};

struct ipipe_float_s16q16 {
	short integer;
	unsigned short decimal;
};


/**********************************************************************
**      Previewer API Structures
**********************************************************************/

/* Previewer module IDs used in PREV_SET/GET_PARAM IOCTL. Some
 * modules can be also be updated during IPIPE operation. They are
 * marked as control ID
 */
/* Prefilter. */
#define PREV_PRE_FILTER		1
/* Defect Pixel Correction */
#define PREV_DFC		2
/* Noise Filter */
#define PREV_NF			3
/* White Balance. Also a control ID */
#define PREV_WB			4
/* RGB to RBG Blend */
#define PREV_RGB2RGB		5
/* Gamma Correction */
#define PREV_GAMMA		6
/* RGB to YCbCr module */
#define PREV_RGB2YUV		7
/* Luminance Adjustment module. Also a control ID */
#define PREV_LUM_ADJ		8
/* YUV 422 conversion module */
#define PREV_YUV422_CONV	9
/* Edge Enhancement */
#define PREV_YEE		10
/* False Color Suppression */
#define PREV_FCS		11
/* Last module ID */
#define PREV_MAX_MODULES	11


struct ipipe_win {
	/* vertical start line */
	unsigned int vst;
	/* horizontal start pixel */
	unsigned int hst;
	/* width */
	unsigned int width;
	/* height */
	unsigned int height;
};

enum ipipe_pf_type {
	PREV_PF_AVG4PIX = 0,
	PREV_PF_AVG2MEDPIX = 1
};

/* Struct for configuring PREV_PRE_FILTER module */
struct prev_prefilter {
	/* 0 - disable, 1 - enable */
	unsigned char en;
	/* 0 - constant gain, 1 - adaptive gain control */
	unsigned char en_adapt_prefilter;
	/* 0 - Only pre-filter, 1 - Dot Reduction applied after pre-filter */
	unsigned char en_adapt_dotred;
	/* 0 - average of 4 pixels, 1 - average of 2 median pixels */
	enum ipipe_pf_type aver_meth_gs1;
	/* 0 - average of 4 pixels, 1 - average of 2 median pixels */
	enum ipipe_pf_type aver_meth_gs2;
	/* constant gain val if en_adapt_gc = 0,
	 * slope in gain function if en_adapt_gc = 1
	 */
	unsigned int pre_gain;
	/* down shift value in adaptive filter algorithm */
	unsigned int pre_shf;
	/* ThrG in adaptive filter algorithm */
	unsigned int pre_thr_g;
	/* ThrB in Dot Reduction */
	unsigned int pre_thr_b;
	/* Thr1 in the adaptive pre-filter algorithm */
	unsigned int pre_thr_1;
};

/* Copy method selection for vertical correction
 * Used when ipipe_dfc_corr_meth is PREV_DPC_CTORB_AFTER_HINT
 */
enum ipipe_dfc_copy_meth {
	/* Copy from top */
	IPIPE_DFC_COPY_FROM_TOP = 0,
	/* Copy from bottom */
	IPIPE_DFC_COPY_FROM_BOTTOM
};

enum ipipe_dfc_corr_meth {
	/* horizontal interpolation */
	IPIPE_DFC_H_INTP = 0,
	/* Copy from left */
	IPIPE_DFC_CL,
	/* Copy from right */
	IPIPE_DFC_CR,
	/* 4 directional interpolation */
	IPIPE_DFC_4_DIR_INTP,
	/* Vertical interpolation after horizontal interpolation */
	IPIPE_DFC_V_INTP_AFTER_H_INTP,
	/* Vertical interpolation after copy from left */
	IPIPE_DFC_V_INTP_AFTER_CL,
	/* Vertical interpolation after copy from right */
	IPIPE_DFC_V_INTP_AFTER_CR,
	/* copy from top or bottom after horizontal interpolation */
	IPIPE_DFC_CTORB_AFTER_HINT
};

struct ipipe_dfc_entry {
	/* Horizontal position */
	unsigned short horz_pos;
	/* vertical position */
	unsigned short vert_pos;
	/* correction method */
	enum ipipe_dfc_corr_meth method;
};

/* Struct for configuring DPC module */
struct prev_dfc {
	/* 0 - disable, 1 - enable */
	unsigned char en;
	/* Used when ipipe_dfc_corr_meth is PREV_DPC_CTORB_AFTER_HINT */
	enum ipipe_dfc_copy_meth vert_copy_method;
	/* number of entries in the correction table */
	unsigned short dfc_size;
	/* DFC entries table */
	struct ipipe_dfc_entry *table;
};


/* Threshold values table size */
#define IPIPE_NF_THR_TABLE_SIZE 32
/* Intensity values table size */
#define IPIPE_NF_STR_TABLE_SIZE 32

/* NF, sampling method for green pixels */
enum ipipe_nf_sampl_meth {
	/* Same as R or B */
	IPIPE_NF_BOX = 0,
	/* Diamond mode */
	IPIPE_NF_DIAMOND = 1
};

/* Struct for configuring NF module */
struct prev_nf {
	/* 0 - disable, 1 - enable */
	unsigned char en;
	/* Sampling method for green pixels */
	enum ipipe_nf_sampl_meth gr_sample_meth;
	/* Down shift value in LUT reference address */
	unsigned char shft_val;
	/* Spread value in NF algorithm */
	unsigned char spread_val;
	/* Threshold values table */
	unsigned short thr[IPIPE_NF_THR_TABLE_SIZE];
	/* intensity values table */
	unsigned char str[IPIPE_NF_STR_TABLE_SIZE];
};


/* Struct for configuring WB module */
struct prev_wb {
	/* Digital gain (U10Q8). For brightness */
	struct ipipe_float_u16q16 dgn;
	/* Gain (U10Q7) for Red */
	struct ipipe_float_u16q16 gain_r;
	/* Gain (U10Q7) for Gr */
	struct ipipe_float_u16q16 gain_gr;
	/* Gain (U10Q7) for Gb */
	struct ipipe_float_u16q16 gain_gb;
	/* Gain (U10Q7) for Blue */
	struct ipipe_float_u16q16 gain_b;
};


/* Struct for configuring RGB2RGB blending module */
struct prev_rgb2rgb {
	/* Matrix coefficient for RR S12Q8 */
	struct ipipe_float_s16q16 coef_rr;
	/* Matrix coefficient for GR S12Q8 */
	struct ipipe_float_s16q16 coef_gr;
	/* Matrix coefficient for BR S12Q8 */
	struct ipipe_float_s16q16 coef_br;
	/* Matrix coefficient for RG S12Q8 */
	struct ipipe_float_s16q16 coef_rg;
	/* Matrix coefficient for GG S12Q8 */
	struct ipipe_float_s16q16 coef_gg;
	/* Matrix coefficient for BG S12Q8 */
	struct ipipe_float_s16q16 coef_bg;
	/* Matrix coefficient for RB S12Q8 */
	struct ipipe_float_s16q16 coef_rb;
	/* Matrix coefficient for GB S12Q8 */
	struct ipipe_float_s16q16 coef_gb;
	/* Matrix coefficient for BB S12Q8 */
	struct ipipe_float_s16q16 coef_bb;
	/* Output offset for R S14Q0 */
	int out_ofst_r;
	/* Output offset for G S14Q0 */
	int out_ofst_g;
	/* Output offset for B S14Q0 */
	int out_ofst_b;
};

enum ipipe_gamma_tbl_size {
	IPIPE_GAMMA_TBL_SZ_128	= 0,
	IPIPE_GAMMA_TBL_SZ_256	= 1,
	IPIPE_GAMMA_TBL_SZ_512	= 3
};

enum ipipe_gamma_tbl_sel {
	IPIPE_GAMMA_TBL_RAM = 0,
	IPIPE_GAMMA_TBL_ROM = 1
};

struct ipipe_gamma_entry {
	/* 10 bit slope */
	unsigned short slope;
	/* 10 bit offset */
	unsigned short offset;
};

/* Struct for configuring Gamma correction module */
struct prev_gamma {
	/* 0 - Enable Gamma correction for Red
	 * 1 - bypass Gamma correction. Data is divided by 16
	 */
	unsigned char bypass_r;
	/* 0 - Enable Gamma correction for Blue
	 * 1 - bypass Gamma correction. Data is divided by 16
	 */
	unsigned char bypass_b;
	/* 0 - Enable Gamma correction for Green
	 * 1 - bypass Gamma correction. Data is divided by 16
	 */
	unsigned char bypass_g;
	/* PREV_GAMMA_TBL_RAM or PREV_GAMMA_TBL_ROM */
	enum ipipe_gamma_tbl_sel tbl_sel;
	/* Table size for RAM gamma table. */
	enum ipipe_gamma_tbl_size tbl_size;
	/* R table */
	struct ipipe_gamma_entry *table_r;
	/* Blue table */
	struct ipipe_gamma_entry *table_b;
	/* Green table */
	struct ipipe_gamma_entry *table_g;
	/* rgb all table */
	struct ipipe_gamma_entry *table_rgb_all;
};


/* Struct for configuring rgb2ycbcr module */
struct prev_rgb2yuv {
	/* Matrix coefficient for RY S10Q8 */
	struct ipipe_float_s16q16 coef_ry;
	/* Matrix coefficient for GY S10Q8 */
	struct ipipe_float_s16q16 coef_gy;
	/* Matrix coefficient for BY S10Q8 */
	struct ipipe_float_s16q16 coef_by;
	/* Matrix coefficient for RCb S10Q8 */
	struct ipipe_float_s16q16 coef_rcb;
	/* Matrix coefficient for GCb S10Q8 */
	struct ipipe_float_s16q16 coef_gcb;
	/* Matrix coefficient for BCb S10Q8 */
	struct ipipe_float_s16q16 coef_bcb;
	/* Matrix coefficient for RCr S10Q8 */
	struct ipipe_float_s16q16 coef_rcr;
	/* Matrix coefficient for GCr S10Q8 */
	struct ipipe_float_s16q16 coef_gcr;
	/* Matrix coefficient for BCr S10Q8 */
	struct ipipe_float_s16q16 coef_bcr;
	/* Output offset for R S9Q0 */
	int out_ofst_y;
	/* Output offset for Cb S9Q0 */
	int out_ofst_cb;
	/* Output offset for Cr S9Q0 */
	int out_ofst_cr;
};

/* Struct for configuring Luminance Adjustment module */
struct prev_lum_adj {
	/* Brightness adjustments */
	unsigned char brightness;
	/* contrast adjustments */
	unsigned char contast;
};

/* Chrominance position. Applicable only for YCbCr input
 * Applied after edge enhancement
 */
enum ipipe_yuv422_chr_pos {
	/* Cositing, same position with luminance */
	IPIPE_YUV422_CHR_POS_COSITE = 0,
	/* Centering, In the middle of luminance */
	IPIPE_YUV422_CHR_POS_CENTRE
};

/* Struct for configuring rgb2ycbcr module */
struct prev_yuv422_conv {
	/* Minimum Luminance value */
	unsigned char lum_min;
	/* Max Luminance value */
	unsigned char lum_max;
	/* Minimum Chrominance value */
	unsigned char chrom_min;
	/* Max Chrominance value */
	unsigned char chrom_max;
	/* 1 - enable LPF for chrminance, 0 - disable */
	unsigned char en_chrom_lpf;
	/* chroma position */
	enum ipipe_yuv422_chr_pos chrom_pos;
};

/* Used to shift input data based on the image sensor
 * interface requirement
 */
enum ipipeif_data_shift {
	IPIPEIF_BITS15_2	= 0,
	IPIPEIF_BITS14_1	= 1,
	IPIPEIF_BITS13_0	= 2,
	IPIPEIF_BITS12_0	= 3,
	IPIPEIF_BITS11_0	= 4,
	IPIPEIF_BITS10_0	= 5,
	IPIPEIF_BITS9_0		= 6
};

/* Set ALAW & PACK8 status based on this */
enum ipipe_pix_formats {
	IPIPE_BAYER_8BIT_PACK,
	IPIPE_BAYER_8BIT_PACK_ALAW,
	/* 16 bit */
	IPIPE_BAYER,
	IPIPE_UYVY,
	IPIPE_RGB565,
	IPIPE_RGB888,
	IPIPE_YUV420P
};

enum ipipeif_clkdiv {
	IPIPEIF_DIVIDE_HALF		= 0,
	IPIPEIF_DIVIDE_THIRD		= 1,
	IPIPEIF_DIVIDE_FOURTH		= 2,
	IPIPEIF_DIVIDE_FIFTH		= 3,
	IPIPEIF_DIVIDE_SIXTH		= 4,
	IPIPEIF_DIVIDE_EIGHTH		= 5,
	IPIPEIF_DIVIDE_SIXTEENTH	= 6,
	IPIPEIF_DIVIDE_THIRTY		= 7
};

enum ipipeif_decimation {
	IPIPEIF_DECIMATION_OFF	= 0,
	IPIPEIF_DECIMATION_ON	= 1
};

enum ipipe_dpaths_bypass_t {
	IPIPE_BYPASS_OFF = 0,
	IPIPE_BYPASS_ON  = 1
};

enum ipipe_colpat_t {
	IPIPE_RED = 0,
	IPIPE_GREEN_RED  = 1,
	IPIPE_GREEN_BLUE = 2,
	IPIPE_BLUE = 3
};

/* Structure for configuring Single Shot mode in the previewer
 * channel
 */

struct prev_ss_input_spec {
	/* width of the image in SDRAM.*/
	unsigned int image_width;
	/* height of the image in SDRAM */
	unsigned int image_height;
	/* vertical start position of the image
	 * data to IPIPE
	 */
	unsigned int vst;
	/* horizontal start position of the image
	 * data to IPIPE
	 */
	unsigned int hst;
	/* Global frame HD rate */
	unsigned int ppln;
	/* Global frame VD rate */
	unsigned int lpfr;
	/* clock divide to bring down the pixel clock*/
	enum ipipeif_clkdiv clk_div;
	/* Shift data as per image sensor capture format
	 * only applicable for RAW Bayer inputs
	 */
	enum ipipeif_data_shift data_shift;
	/* Enable decimation 1 - enable, 0 - disable
	 * This is used for bringing down the line size
	 * to that supported by IPIPE. DM355 IPIPE
	 * can process only 1344 pixels per line.
	 */
	enum ipipeif_decimation dec_en;
	/* used when en_dec = 1. Resize ratio for decimation
	 * when frame size is greater than what hw can handle.
	 * 16 to 112. IPIPE input width is calculated as follows.
	 * width = image_width * 16/ipipeif_rsz. For example
	 * if image_width is 1920 and user want to scale it down
	 * to 1280, use ipipeif_rsz = 24. 1920*16/24 = 1280
	 */
	unsigned char rsz;
	/* Enable/Disable avg filter at IPIPEIF.  1 - enable, 0 - disable */
	unsigned char avg_filter_en;
	/* Gain. 1 - 1023. divided by 512. So can be from 1/512 to 1/1023 */
	unsigned short gain;
	/* Input pixels formats */
	enum ipipe_pix_formats pix_fmt;
	/* Color pattern for odd line, odd pixel */
	enum ipipe_colpat_t colp_olop;
	/* Color pattern for odd line, even pixel */
	enum ipipe_colpat_t colp_olep;
	/* Color pattern for even line, odd pixel */
	enum ipipe_colpat_t colp_elop;
	/* Color pattern for even line, even pixel */
	enum ipipe_colpat_t colp_elep;
};

struct prev_ss_output_spec {
	/* output pixel format */
	enum ipipe_pix_formats pix_fmt;
};

struct prev_single_shot_config {
	/* Bypass image processing. RAW -> RAW */
	enum ipipe_dpaths_bypass_t bypass;
	/* Input specification for the image data */
	struct prev_ss_input_spec input;
	/* Output specification for the image data */
	struct prev_ss_output_spec output;
};

struct prev_cont_input_spec {
	/* 1 - enable, 0 - disable df subtraction */
	unsigned char en_df_sub;
	/* Enable decimation 1 - enable, 0 - disable
	 * This is used for bringing down the line size
	 * to that supported by IPIPE. DM355 IPIPE
	 * can process only 1344 pixels per line.
	 */
	enum ipipeif_decimation dec_en;
	/* used when en_dec = 1. Resize ratio for decimation
	 * when frame size is greater than what hw can handle.
	 * 16 to 112. IPIPE input width is calculated as follows.
	 * width = image_width * 16/ipipeif_rsz. For example
	 * if image_width is 1920 and user want to scale it down
	 * to 1280, use ipipeif_rsz = 24. 1920*16/24 = 1280
	 */
	unsigned char rsz;
	/* Enable/Disable avg filter at IPIPEIF. 1 - enable, 0 - disable */
	unsigned char avg_filter_en;
	/* Gain applied at IPIPEIF. 1 - 1023. divided by 512.So can be from
	 * 1/512 to 1/1023.
	 */
	unsigned short gain;
	/* Color pattern for odd line, odd pixel */
	enum ipipe_colpat_t colp_olop;
	/* Color pattern for odd line, even pixel */
	enum ipipe_colpat_t colp_olep;
	/* Color pattern for even line, odd pixel */
	enum ipipe_colpat_t colp_elop;
	/* Color pattern for even line, even pixel */
	enum ipipe_colpat_t colp_elep;
};

/* Structure for configuring Continuous mode in the previewer
 * channel . In continuous mode, only following parameters are
 * available for configuration from user. Rest are configured
 * through S_CROP and S_FMT IOCTLs in CCDC driver. In this mode
 * data to IPIPEIF comes from CCDC
 */
struct prev_continuous_config {
	/* Bypass image processing. RAW -> RAW */
	enum ipipe_dpaths_bypass_t bypass;
	/* Input specification for the image data */
	struct prev_cont_input_spec input;
};


/*******************************************************************
**  Resizer API structures
*******************************************************************/


/* Struct for configuring YUV Edge Enhancement module */
struct prev_yee {
	/* 1 - enable enhancement, 0 - disable */
	unsigned char en;
	/* 1 - enable Median NR, 0 - diable */
	unsigned char en_emf;
	/* HPF Shift length */
	unsigned char hpf_shft;
	/* HPF Coefficient 00, S10Q0 */
	short hpf_coef_00;
	/* HPF Coefficient 01, S10Q0 */
	short hpf_coef_01;
	/* HPF Coefficient 02, S10Q0 */
	short hpf_coef_02;
	/* HPF Coefficient 10, S10Q0 */
	short hpf_coef_10;
	/* HPF Coefficient 11, S10Q0 */
	short hpf_coef_11;
	/* HPF Coefficient 12, S10Q0 */
	short hpf_coef_12;
	/* HPF Coefficient 20, S10Q0 */
	short hpf_coef_20;
	/* HPF Coefficient 21, S10Q0 */
	short hpf_coef_21;
	/* HPF Coefficient 22, S10Q0 */
	short hpf_coef_22;
	/* Ptr to EE table. Must have 1024 entries */
	short *table;
};

enum ipipe_fcs_type {
	/* Y */
	IPIPE_FCS_Y = 0,
	/* Horizontal HPF */
	IPIPE_FCS_HPF_HORZ,
	/* Vertical HPF */
	IPIPE_FCS_HPF_VERT,
	/* 2D HPF */
	IPIPE_FCS_2D_HPF,
	/* 2D HPF from Y Edge Enhancement module */
	IPIPE_FCS_2D_HPF_YEE
};

/* Struct for configuring FCS module */
struct prev_fcs {
	/* 1 - enable FCS, 0 - disable */
	unsigned char en;
	/* Selection of HOF for color suppression */
	enum ipipe_fcs_type type;
	/* Y Shift value for HPF */
	unsigned char hpf_shft_y;
	/* C Shift value for Gain function */
	unsigned char gain_shft_c;
	/* Threshold of the suppression Gain. Used in gain function */
	unsigned short thr;
	/* Intensity of the color suppression. Used in gain function */
	unsigned short sgn;
	/* Lower limit of the chroma gain. Used in gain function */
	unsigned short lth;
};

/* Interpolation types used for horizontal rescale */
enum rsz_h_intp_t {
	RSZ_H_INTP_CUBIC = 0,
	RSZ_H_INTP_LINEAR
};

/* Horizontal LPF intensity selection */
enum rsz_h_lpf_lse_t {
	RSZ_H_LPF_LSE_INTERN = 0,
	RSZ_H_LPF_LSE_USER_VAL
};

/* Structure for configuring resizer in single shot mode.
 * This structure is used when operation mode of the
 * resizer is single shot. The related IOCTL is
 * RSZ_S_CONFIG & RSZ_G_CONFIG. When chained, data to
 * resizer comes from previewer. When not chained, only
 * UYVY data input is allowed for resizer operation.
 * To operate on RAW Bayer data from CCDC, chain resizer
 * with previewer by setting chain field in the
 * rsz_channel_config structure.
 */

struct rsz_ss_input_spec {
	/* width of the image in SDRAM.*/
	unsigned int image_width;
	/* height of the image in SDRAM */
	unsigned int image_height;
	/* vertical start position of the image data to IPIPE */
	unsigned int vst;
	/* horizontal start position of the image data to IPIPE */
	unsigned int hst;
	/* Global frame HD rate */
	unsigned int ppln;
	/* Global frame VD rate */
	unsigned int lpfr;
	/* clock divide to bring down the pixel clock*/
	enum ipipeif_clkdiv clk_div;
	/* Enable decimation 1 - enable, 0 - disable
	 * This is used for bringing down the line size
	 * to that supported by IPIPE. DM355 IPIPE
	 * can process only 1344 pixels per line.
	 */
	enum ipipeif_decimation dec_en;
	/* used when en_dec = 1. Resize ratio for decimation
	 * when frame size is greater than what hw can handle.
	 * 16 to 112. IPIPE input width is calculated as follows.
	 * width = image_width * 16/ipipeif_rsz. For example
	 * if image_width is 1920 and user want to scale it down
	 * to 1280, use ipipeif_rsz = 24. 1920*16/24 = 1280
	 */
	unsigned char rsz;
	/* Enable/Disable avg filter at IPIPEIF. 1 - enable, 0 - disable */
	unsigned char avg_filter_en;
	/* Input pixels formats */
	enum ipipe_pix_formats pix_fmt;
};

struct rsz_ss_output_spec {
	/* enable the resizer output */
	unsigned char enable;
	/* output pixel format. Has to be UYVY */
	enum ipipe_pix_formats pix_fmt;
	/* width in pixels. must be multiple of 16. */
	unsigned int width;
	/* height in lines */
	unsigned int height;
	/* line start offset. */
	unsigned int vst;
	/* pixel start offset.*/
	unsigned int hst;
	/* horizontal rescale interpolation types */
	enum rsz_h_intp_t h_intp_type;
	/* horizontal hpf intesity value selection choices */
	enum rsz_h_lpf_lse_t h_lpf_lse_sel;
	/* User provided intensity value. Used when h_lpf_lse_sel
	 * is RSZ_H_LPF_LSE_USER_VAL
	 */
	unsigned char lpf_user_val;
};

struct rsz_single_shot_config {
	/* input spec of the image data (UYVY). non-chained
	 * mode. Only valid when not chained. For chained
	 * operation, previewer settings are used
	 */
	struct rsz_ss_input_spec input;
	/* output spec of the image data coming out of resizer - 0(UYVY). */
	struct rsz_ss_output_spec output1;
	/* output spec of the image data coming out of resizer - 1(UYVY). */
	struct rsz_ss_output_spec output2;
	/* Image is flipped vertically, 0 - disable, 1 - enable */
	unsigned char en_flip_vert;
	/* Image is flipped horizontally, 0 - disable, 1 - enable */
	unsigned char en_flip_horz;
	/* 0 , chroma sample at odd pixel, 1 - even pixel */
	unsigned char chroma_sample_even;
	/* Enable Vertical anti aliasing filter 0 - disable, 1 - enable */
	unsigned char en_vaaf;
};

struct rsz_cont_input_spec {
	/* Enable decimation 1 - enable, 0 - disable
	 * This is used for bringing down the line size
	 * to that supported by IPIPE. DM355 IPIPE
	 * can process only 1344 pixels per line.
	 */
	enum ipipeif_decimation dec_en;
	/* used when en_dec = 1. Resize ratio for decimation
	 * when frame size is greater than what hw can handle.
	 * 16 to 112. IPIPE input width is calculated as follow.
	 * width = image_width * 16/ipipeif_rsz. For example
	 * if image_width is 1920 and user want to scale it down
	 * to 1280, use ipipeif_rsz = 24. 1920*16/24 = 1280
	 */
	unsigned char rsz;
	/* Enable/Disable avg filter at IPIPEIF.
	 * 1 - enable, 0 - disable
	 */
	unsigned char avg_filter_en;
	/* 1 - 1023. divided by 512. So can be from 1/512 to 1/1023 */
	unsigned short gain;
};

struct rsz_continuous_config {
	/* input spec of the image data (UYVY). non-chained
	 * mode. Only valid when not chained. For chained
	 * operation, previewer settings are used
	 */
	struct rsz_cont_input_spec input;
	/* enable or disable resizer 0 output. 1 - enable,
	 *   0 - disable
	 */
	unsigned char en_output1;
	/* output spec of the image data coming out of resizer - 1(UYVY). */
	struct rsz_ss_output_spec output2;
	/* Image is flipped vertically, 0 - disable, 1 - enable */
	unsigned char en_flip_vert;
	/* Image is flipped horizontally, 0 - disable, 1 - enable */
	unsigned char en_flip_horz;
	/* 0 , chroma sample at odd pixel, 1 - even pixel */
	unsigned char chroma_sample_even;
	/* Enable Vertical anti aliasing filter 0 - disable, 1 - enable */
	unsigned char en_vaaf;
};

#ifdef __KERNEL__
#include <asm/arch/imp_common.h>
enum ipipeif_clock {
	PIXCEL_CLK	= 0,
	SDRAM_CLK	= 1
};

enum ipipeif_ialaw {
	ALAW_OFF	= 0,
	ALAW_ON		= 1
};

enum ipipeif_pack_mode {
	SIXTEEN_BIT	= 0,
	EIGHT_BIT	= 1
};

enum ipipeif_avg_filter {
	AVG_OFF		= 0,
	AVG_ON		= 1
};

/* data paths */
enum ipipe_data_paths {
	/* Bayer RAW input to YCbCr output */
	IPIPE_RAW2YUV = 0,
	/* Bayer Raw to Bayer output */
	IPIPE_RAW2RAW,
	/* Bayer Raw to Boxcar output */
	IPIPE_RAW2BOX,
	/* YUV Raw to YUV Raw output */
	IPIPE_YUV2YUV
};

enum ipipeif_input_source {
	CCDC		= 0,
	SDRAM_RAW	= 1,
	CCDC_DARKFM	= 2,
	SDRAM_YUV	= 3
};

enum ipipe_oper_mode {
	CONTINUOUS  = 0,
	ONE_SHOT    = 1
};

/*ipipeif structures*/
struct ipipeif {
	/*IPPEIF config register*/
	enum ipipeif_data_shift data_shift;
	enum ipipeif_clock clock_select;
	enum ipipeif_ialaw ialaw;
	enum ipipeif_pack_mode pack_mode;
	enum ipipeif_avg_filter avg_filter;
	enum ipipeif_clkdiv clk_div;
	enum ipipeif_input_source source;
	enum ipipeif_decimation decimation;
	enum ipipe_oper_mode mode;

	unsigned int glob_hor_size;
	unsigned int glob_ver_size;
	unsigned int hnum;
	unsigned int vnum;
	unsigned int adofs;
	unsigned char rsz;
	unsigned int gain;
};

enum enable_disable_t {
	DISABLE =  0,
	ENABLE =   1
};

/* Resizer Rescale Parameters*/
struct ipipe_rsz_rescale_param {
	enum ipipe_oper_mode rsz_mode;
	unsigned int rsz_i_vst;
	unsigned int rsz_i_vsz;
	unsigned int rsz_i_hst;
	unsigned int rsz_o_vsz;
	unsigned int rsz_o_hsz;
	unsigned int rsz_o_hst;
	unsigned int rsz_v_phs;
	unsigned int rsz_v_dif;
	unsigned int rsz_h_phs;
	unsigned int rsz_h_dif;
	enum rsz_h_intp_t rsz_h_typ;
	enum rsz_h_lpf_lse_t rsz_h_lse_sel;
	unsigned char rsz_h_lpf;
};

enum ipipe_rsz_rgb_t {
	OUTPUT_32BIT = 0,
	OUTPUT_16BIT
};

enum ipipe_rsz_rgb_msk_t {
	NOMASK = 0,
	MASKLAST2
};

/* Resizer RGB Conversion Parameters */
struct ipipe_rsz_resize2rgb {
	enum enable_disable_t rsz_rgb_en;
	enum ipipe_rsz_rgb_t rsz_rgb_typ;
	enum ipipe_rsz_rgb_msk_t rsz_rgb_msk0;
	enum ipipe_rsz_rgb_msk_t rsz_rgb_msk1;
	unsigned int rsz_rgb_alpha_val;
};

/* Resizer External Memory Parameters */
struct ipipe_ext_mem_param {
	unsigned int rsz_sdr_bad_h;
	unsigned int rsz_sdr_bad_l;
	unsigned int rsz_sdr_sad_h;
	unsigned int rsz_sdr_sad_l;
	unsigned int rsz_sdr_oft;
	unsigned int rsz_sdr_ptr_s;
	unsigned int rsz_sdr_ptr_e;
};

struct ipipe_params {
	struct ipipeif ipipeif_param;
	enum ipipe_oper_mode ipipe_mode;
	/*input/output datapath register*/
	enum ipipe_data_paths ipipe_dpaths_fmt;
	enum ipipe_dpaths_bypass_t ipipe_dpaths_bypass;

	/*color pattern register*/
	enum ipipe_colpat_t ipipe_colpat_elep;
	enum ipipe_colpat_t ipipe_colpat_elop;
	enum ipipe_colpat_t ipipe_colpat_olep;
	enum ipipe_colpat_t ipipe_colpat_olop;

	/*horizontal/vertical start, horizontal/vertical size*/
	unsigned int ipipe_vst;
	unsigned int ipipe_vsz;
	unsigned int ipipe_hst;
	unsigned int ipipe_hsz;

	enum enable_disable_t rsz_seq_seq;
	enum enable_disable_t rsz_seq_tmm;
	enum enable_disable_t rsz_seq_hrv;
	enum enable_disable_t rsz_seq_vrv;
	enum enable_disable_t rsz_seq_crv;
	enum enable_disable_t rsz_aal;

	struct ipipe_rsz_rescale_param rsz_rsc_param[2];
	struct ipipe_rsz_resize2rgb rsz2rgb[2];
	struct ipipe_ext_mem_param ext_mem_param[2];
	enum enable_disable_t rsz_en[2];
};

void ipipe_enable_reg_write(void);
int ipipe_hw_setup(struct ipipe_params *config);
int ipipe_default_raw2raw(struct ipipe_params *parameter);
int ipipe_default_bypass_resizer(struct ipipe_params *parameter);
int ipipe_default_bypass_ycbcr(struct ipipe_params *parameter);
int ipipe_set_dfc_regs(struct prev_dfc *dfc);
int ipipe_set_d2f_nf_regs(struct prev_nf *noise_filter);
int ipipe_set_pf_regs(struct prev_prefilter *pre_filter);
int ipipe_set_wb_regs(struct prev_wb *wb);
int ipipe_set_rgb2ycbcr_regs(struct prev_rgb2yuv *yuv);
int ipipe_set_lum_adj_regs(struct prev_lum_adj *lum_adj);
int ipipe_set_yuv422_conv_regs(struct prev_yuv422_conv *conv);
int ipipe_set_rgb2rgb_regs(struct prev_rgb2rgb *rgb);
int ipipe_set_gamma_regs(struct prev_gamma *gamma);
int ipipe_set_ee_regs(struct prev_yee *ee);
int ipipe_set_fcs_regs(struct prev_fcs *fcs);
int ipipe_set_rsz_regs(struct ipipe_params *param_resize);
int ipipe_set_aal_regs(struct ipipe_params *param_resize);
int ipipe_set_output_size(struct ipipe_params *params);
int ipipe_set_output_offsets(int resizer, struct ipipe_params *params);
int ipipe_set_rsz_structs(struct ipipe_params *params);
int ipipe_set_resizer_address(struct ipipe_params *params,
						int resize_no,
						unsigned int address);
int ipipe_set_ipipeif_address(struct ipipe_params *params,
						unsigned int address);
void ipipe_hw_dump_config(void);
int rsz_enable(int rsz_id, int enable);

#endif
#endif
