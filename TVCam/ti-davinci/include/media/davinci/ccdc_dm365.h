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
 * ccdc header file for DM365 ISIF
 */

#ifndef _CCDC_DM365_H
#define _CCDC_DM365_H
#include <media/davinci/ccdc_common.h>

/* SOC specific controls for Bayer capture. The CIDs
 * listed here should match with that in davinci_vpfe.h
 */
/* White balance on Bayer RGB. U11Q8 */
#define CCDC_CID_R_GAIN		(V4L2_CID_PRIVATE_BASE + 0)
#define CCDC_CID_GR_GAIN	(V4L2_CID_PRIVATE_BASE + 1)
#define CCDC_CID_GB_GAIN 	(V4L2_CID_PRIVATE_BASE + 2)
#define CCDC_CID_B_GAIN 	(V4L2_CID_PRIVATE_BASE + 3)
/* Offsets */
#define CCDC_CID_OFFSET 	(V4L2_CID_PRIVATE_BASE + 4)
#define CCDC_CID_MAX		(V4L2_CID_PRIVATE_BASE + 5)

/*
 * ccdc float type S8Q8/U8Q8
 */
struct ccdc_float_8 {
	/* 8 bit integer part */
	unsigned char integer;
	/* 8 bit decimal part */
	unsigned char decimal;
};

/*
 * brief ccdc float type U16Q16/S16Q16
 */
struct ccdc_float_16 {
	/* 16 bit integer part */
	unsigned short integer;
	/* 16 bit decimal part */
	unsigned short decimal;
};

/*
 * ccdc image(target) window parameters
 */
struct ccdc_cropwin {
	/* horzontal offset of the top left corner in pixels */
	unsigned int left;
	/* vertical offset of the top left corner in pixels */
	unsigned int top;
	/* width in pixels of the rectangle */
	unsigned int width;
	/* height in lines of the rectangle */
	unsigned int height;
};

/************************************************************************
 *   Vertical Defect Correction parameters
 ***********************************************************************/

/*
 * vertical defect correction methods
 */
enum ccdc_vdfc_corr_mode {
	/* Defect level subtraction. Just fed through if saturating */
	CCDC_VDFC_NORMAL,
	/* Defect level subtraction. Horizontal interpolation ((i-2)+(i+2))/2
	 * if data saturating
	 */
	CCDC_VDFC_HORZ_INTERPOL_IF_SAT,
	/* Horizontal interpolation (((i-2)+(i+2))/2) */
	CCDC_VDFC_HORZ_INTERPOL
};

/*
 * Max Size of the Vertical Defect Correction table
 */
#define CCDC_VDFC_TABLE_SIZE 8

/*
 * Values used for shifting up the vdfc defect level
 */
enum ccdc_vdfc_shift {
	/* No Shift */
	CCDC_VDFC_NO_SHIFT,
	/* Shift by 1 bit */
	CCDC_VDFC_SHIFT_1,
	/* Shift by 2 bit */
	CCDC_VDFC_SHIFT_2,
	/* Shift by 3 bit */
	CCDC_VDFC_SHIFT_3,
	/* Shift by 4 bit */
	CCDC_VDFC_SHIFT_4
};

/*
 * Defect Correction (DFC) table entry
 */
struct ccdc_vdfc_entry {
	/* vertical position of defect */
	unsigned short pos_vert;
	/* horizontal position of defect */
	unsigned short pos_horz;
	/* Defect level of Vertical line defect position. This is subtracted
	 * from the data at the defect position
	 */
	unsigned char level_at_pos;
	/* Defect level of the pixels upper than the vertical line defect.
	 * This is subtracted from the data
	 */
	unsigned char level_up_pixels;
	/* Defect level of the pixels lower than the vertical line defect.
	 * This is subtracted from the data
	 */
	unsigned char level_low_pixels;
};

/*
 * Structure for Defect Correction (DFC) parameter
 */
struct ccdc_dfc {
	/* enable vertical defect correction */
	unsigned char en;
	/* Correction methods */
	enum ccdc_vdfc_corr_mode corr_mode;
	/* 0 - whole line corrected, 1 - not
	 * pixels upper than the defect
	 */
	unsigned char corr_whole_line;
	/* defect level shift value. level_at_pos, level_upper_pos,
	 * and level_lower_pos can be shifted up by this value
	 */
	enum ccdc_vdfc_shift def_level_shift;
	/* defect saturation level */
	unsigned short def_sat_level;
	/* number of vertical defects. Max is CCDC_VDFC_TABLE_SIZE */
	short num_vdefects;
	/* VDFC table ptr */
	struct ccdc_vdfc_entry table[CCDC_VDFC_TABLE_SIZE];
};

/************************************************************************
*   Digital/Black clamp or DC Subtract parameters
************************************************************************/
/*
 * Horizontal Black Clamp modes
 */
enum ccdc_horz_bc_mode {
	/* Horizontal clamp disabled. Only vertical clamp
	 * value is subtracted
	 */
	CCDC_HORZ_BC_DISABLE,
	/* Horizontal clamp value is calculated and subtracted
	 * from image data along with vertical clamp value
	 */
	CCDC_HORZ_BC_CLAMP_CALC_ENABLED,
	/* Horizontal clamp value calculated from previous image
	 * is subtracted from image data along with vertical clamp
	 * value. How the horizontal clamp value for the first image
	 * is calculated in this case ???
	 */
	CCDC_HORZ_BC_CLAMP_NOT_UPDATED
};

/*
 * Base window selection for Horizontal Black Clamp calculations
 */
enum ccdc_horz_bc_base_win_sel {
	/* Select Most left window for bc calculation */
	CCDC_SEL_MOST_LEFT_WIN,

	/* Select Most right window for bc calculation */
	CCDC_SEL_MOST_RIGHT_WIN,
};

/* Size of window in horizontal direction for horizontal bc */
enum ccdc_horz_bc_sz_h {
	CCDC_HORZ_BC_SZ_H_2PIXELS,
	CCDC_HORZ_BC_SZ_H_4PIXELS,
	CCDC_HORZ_BC_SZ_H_8PIXELS,
	CCDC_HORZ_BC_SZ_H_16PIXELS
};

/* Size of window in vertcal direction for vertical bc */
enum ccdc_horz_bc_sz_v {
	CCDC_HORZ_BC_SZ_H_32PIXELS,
	CCDC_HORZ_BC_SZ_H_64PIXELS,
	CCDC_HORZ_BC_SZ_H_128PIXELS,
	CCDC_HORZ_BC_SZ_H_256PIXELS
};

/*
 * Structure for Horizontal Black Clamp config params
 */
struct ccdc_horz_bclamp {
	/* horizontal clamp mode */
	enum ccdc_horz_bc_mode mode;
	/* pixel value limit enable.
	 *  0 - limit disabled
	 *  1 - pixel value limited to 1023
	 */
	unsigned char clamp_pix_limit;
	/* Select most left or right window for clamp val
	 * calculation
	 */
	enum ccdc_horz_bc_base_win_sel base_win_sel_calc;
	/* Window count per color for calculation. range 1-32 */
	unsigned char win_count_calc;
	/* Window start position - horizontal for calculation. 0 - 8191 */
	unsigned short win_start_h_calc;
	/* Window start position - vertical for calculation 0 - 8191 */
	unsigned short win_start_v_calc;
	/* Width of the sample window in pixels for calculation */
	enum ccdc_horz_bc_sz_h win_h_sz_calc;
	/* Height of the sample window in pixels for calculation */
	enum ccdc_horz_bc_sz_v win_v_sz_calc;
};

/*
 * Black Clamp vertical reset values
 */
enum ccdc_vert_bc_reset_val_sel {
	/* Reset value used is the clamp value calculated
	 */
	CCDC_VERT_BC_USE_HORZ_CLAMP_VAL,
	/* Reset value used is reset_clamp_val configured */
	CCDC_VERT_BC_USE_CONFIG_CLAMP_VAL,
	/* No update, previous image value is used */
	CCDC_VERT_BC_NO_UPDATE
};

enum ccdc_vert_bc_sz_h {
	CCDC_VERT_BC_SZ_H_2PIXELS,
	CCDC_VERT_BC_SZ_H_4PIXELS,
	CCDC_VERT_BC_SZ_H_8PIXELS,
	CCDC_VERT_BC_SZ_H_16PIXELS,
	CCDC_VERT_BC_SZ_H_32PIXELS,
	CCDC_VERT_BC_SZ_H_64PIXELS
};

/*
 * Structure for Vetical Black Clamp configuration params
 */
struct ccdc_vert_bclamp {
	/* Reset value selection for vertical clamp calculation */
	enum ccdc_vert_bc_reset_val_sel reset_val_sel;
	/* U12 value if reset_sel = CCDC_BC_VERT_USE_CONFIG_CLAMP_VAL */
	unsigned short reset_clamp_val;
	/* U8Q8. Line average coefficient used in vertical clamp
	 * calculation
	 */
	unsigned char line_ave_coef;
	/* Width in pixels of the optical black region used for calculation. */
	enum ccdc_vert_bc_sz_h ob_h_sz_calc;
	/* Height of the optical black region for calculation */
	unsigned short ob_v_sz_calc;
	/* Optical black region start position - horizontal. 0 - 8191 */
	unsigned short ob_start_h;
	/* Optical black region start position - vertical 0 - 8191 */
	unsigned short ob_start_v;
};

/*
 * Structure for Black Clamp configuration params
 */
struct ccdc_black_clamp {
	/* this offset value is added irrespective of the clamp
	 * enable status. S13
	 */
	unsigned short dc_offset;
	/* Enable black/digital clamp value to be subtracted
	 * from the image data
	 */
	unsigned char en;
	/* black clamp mode. same/separate clamp for 4 colors
	 * 0 - disable - same clamp value for all colors
	 * 1 - clamp value calculated separately for all colors
	 */
	unsigned char bc_mode_color;
	/* Vrtical start position for bc subtraction */
	unsigned short vert_start_sub;
	/* Black clamp for horizontal direction */
	struct ccdc_horz_bclamp horz;
	/* Black clamp for vertical direction */
	struct ccdc_vert_bclamp vert;
};

/*************************************************************************
** Color Space Convertion (CSC)
*************************************************************************/
/*
 * Number of Coefficient values used for CSC
 */
#define CCDC_CSC_NUM_COEFF 16

/*************************************************************************
**  Color Space Conversion parameters
*************************************************************************/
/*
 * Structure used for CSC config params
 */
struct ccdc_color_space_conv {
	/* Enable color space conversion */
	unsigned char en;
	/* csc coeffient table. S8Q5, M00 at index 0, M01 at index 1, and
	 * so forth
	 */
	struct ccdc_float_8 coeff[CCDC_CSC_NUM_COEFF];
};

/*
 * CCDC image data size
 */
enum ccdc_data_size {
	/* 8 bits */
	CCDC_8_BITS,
	/* 9 bits */
	CCDC_9_BITS,
	/* 10 bits */
	CCDC_10_BITS,
	/* 11 bits */
	CCDC_11_BITS,
	/* 12 bits */
	CCDC_12_BITS,
	/* 13 bits */
	CCDC_13_BITS,
	/* 14 bits */
	CCDC_14_BITS,
	/* 15 bits */
	CCDC_15_BITS,
	/* 16 bits */
	CCDC_16_BITS
};

/*
 * CCDC image data shift to right
 */
enum ccdc_datasft {
	/* No Shift */
	CCDC_NO_SHIFT,
	/* 1 bit Shift */
	CCDC_1BIT_SHIFT,
	/* 2 bit Shift */
	CCDC_2BIT_SHIFT,
	/* 3 bit Shift */
	CCDC_3BIT_SHIFT,
	/* 4 bit Shift */
	CCDC_4BIT_SHIFT,
	/* 5 bit Shift */
	CCDC_5BIT_SHIFT,
	/* 6 bit Shift */
	CCDC_6BIT_SHIFT
};

/*
 * MSB of image data connected to sensor port
 */
enum ccdc_data_msb {
	/* MSB b15 */
	CCDC_BIT_MSB_15,
	/* MSB b14 */
	CCDC_BIT_MSB_14,
	/* MSB b13 */
	CCDC_BIT_MSB_13,
	/* MSB b12 */
	CCDC_BIT_MSB_12,
	/* MSB b11 */
	CCDC_BIT_MSB_11,
	/* MSB b10 */
	CCDC_BIT_MSB_10,
	/* MSB b9 */
	CCDC_BIT_MSB_9,
	/* MSB b8 */
	CCDC_BIT_MSB_8,
	/* MSB b7 */
	CCDC_BIT_MSB_7
};

/*************************************************************************
**  Black  Compensation parameters
*************************************************************************/
/*
 * Structure used for Black Compensation
 */
struct ccdc_black_comp {
	/* Comp for Red */
	char r_comp;
	/* Comp for Gr */
	char gr_comp;
	/* Comp for Blue */
	char b_comp;
	/* Comp for Gb */
	char gb_comp;
};

/*************************************************************************
**  Gain parameters
*************************************************************************/
/*
 * Structure for Gain parameters
 */
struct ccdc_gain {
	/* Gain for Red or ye */
	struct ccdc_float_16 r_ye;
	/* Gain for Gr or cy */
	struct ccdc_float_16 gr_cy;
	/* Gain for Gb or g */
	struct ccdc_float_16 gb_g;
	/* Gain for Blue or mg */
	struct ccdc_float_16 b_mg;
};

/*
 * Predicator types for DPCM compression
 */
enum ccdc_dpcm_predictor {
	/* Choose Predictor1 for DPCM compression */
	CCDC_DPCM_PRED1,
	/* Choose Predictor2 for DPCM compression */
	CCDC_DPCM_PRED2
};

#define CCDC_LINEAR_TAB_SIZE 192
/*************************************************************************
**  Linearization parameters
*************************************************************************/
/*
 * Structure for Sensor data linearization
 */
struct ccdc_linearize {
	/* Enable or Disable linearization of data */
	unsigned char en;
	/* Shift value applied */
	enum ccdc_datasft corr_shft;
	/* scale factor applied U11Q10 */
	struct ccdc_float_16 scale_fact;
	/* Size of the linear table */
	unsigned short table[CCDC_LINEAR_TAB_SIZE];
};

enum ccdc_cfa_pattern {
	CCDC_CFA_PAT_MOSAIC,
	CCDC_CFA_PAT_STRIPE
};

enum ccdc_colpats {
	CCDC_RED = 0,
	CCDC_GREEN_RED = 1,
	CCDC_GREEN_BLUE = 2,
	CCDC_BLUE = 3
};

struct ccdc_col_pat {
	enum ccdc_colpats olop;
	enum ccdc_colpats olep;
	enum ccdc_colpats elop;
	enum ccdc_colpats elep;
};

/*************************************************************************
**  CCDC Raw configuration parameters
*************************************************************************/
/*
 * Structure for raw image processing configuration
 */
struct ccdc_bayer_config {
	/* ccdc pixel format */
	enum ccdc_pixfmt pix_fmt;
	/* ccdc frame format */
	enum ccdc_frmfmt frm_fmt;
	/* CCDC crop window */
	struct ccdc_cropwin win;
	/* field polarity */
	enum ccdc_pinpol fid_pol;
	/* interface VD polarity */
	enum ccdc_pinpol vd_pol;
	/* interface HD polarity */
	enum ccdc_pinpol hd_pol;
	/* buffer type. Applicable for interlaced mode */
	enum ccdc_buftype buf_type;
	/* color pattern for field 0 */
	struct ccdc_col_pat col_pat_field0;
	/* color pattern for field 1 */
	struct ccdc_col_pat col_pat_field1;
	/* cfa pattern */
	enum ccdc_cfa_pattern cfa_pat;
	/* data size from 8 to 16 bits */
	enum ccdc_data_size data_size;
	/* Data shift applied before storing to SDRAM */
	enum ccdc_datasft data_shift;
	/* Data MSB position */
	enum ccdc_data_msb data_msb;
	/* Enable horizontal flip */
	unsigned char horz_flip_en;
	/* Enable image invert vertically */
	unsigned char image_invert_en;
	/* enable input test pattern generation */
	unsigned char test_pat_gen;
};

struct ccdc_ycbcr_config {
	/* ccdc pixel format */
	enum ccdc_pixfmt pix_fmt;
	/* ccdc frame format */
	enum ccdc_frmfmt frm_fmt;
	/* CCDC crop window */
	struct ccdc_cropwin win;
	/* field polarity */
	enum ccdc_pinpol fid_pol;
	/* interface VD polarity */
	enum ccdc_pinpol vd_pol;
	/* interface HD polarity */
	enum ccdc_pinpol hd_pol;
	/* ccdc pix order. Only used for ycbcr capture */
	enum ccdc_pixorder pix_order;
	/* ccdc buffer type. Only used for ycbcr capture */
	enum ccdc_buftype buf_type;
	/* enable input test pattern generation */
	unsigned char test_pat_gen;
};

enum ccdc_fmt_mode {
	CCDC_SPLIT,
	CCDC_COMBINE
};

enum ccdc_lnum {
	CCDC_1LINE,
	CCDC_2LINES,
	CCDC_3LINES,
	CCDC_4LINES
};

enum ccdc_line {
	CCDC_1STLINE,
	CCDC_2NDLINE,
	CCDC_3RDLINE,
	CCDC_4THLINE
};

struct ccdc_fmtplen {
	/* number of program entries for SET0, range 1 - 16
	 * when fmtmode is CCDC_SPLIT, 1 - 8 when fmtmode is
	 * CCDC_COMBINE
	 */
	unsigned short plen0;
	/* number of program entries for SET1, range 1 - 16
	 * when fmtmode is CCDC_SPLIT, 1 - 8 when fmtmode is
	 * CCDC_COMBINE
	 */
	unsigned short plen1;
	/* number of program entries for SET2, range 1 - 16
	 * when fmtmode is CCDC_SPLIT, 1 - 8 when fmtmode is
	 * CCDC_COMBINE
	 */
	unsigned short plen2;
	/* number of program entries for SET3, range 1 - 16
	 * when fmtmode is CCDC_SPLIT, 1 - 8 when fmtmode is
	 * CCDC_COMBINE
	 */
	unsigned short plen3;
};

struct ccdc_fmt_cfg {
	/* Split or combine or line alternate */
	enum ccdc_fmt_mode fmtmode;
	/* enable or disable line alternating mode */
	unsigned char ln_alter_en;
	/* Split/combine line number */
	enum ccdc_lnum lnum;
	/* Address increment Range 1 - 16 */
	unsigned int addrinc;
};

struct ccdc_fmt_addr_ptr {
	/* Initial address */
	unsigned int init_addr;
	/* output line number */
	enum ccdc_line out_line;
};

struct ccdc_fmtpgm_ap {
	/* program address pointer */
	unsigned char pgm_aptr;
	/* program address increment or decrement */
	unsigned char pgmupdt;
};

struct ccdc_data_formatter {
	/* Enable/Disable data formatter */
	unsigned char en;
	/* data formatter configuration */
	struct ccdc_fmt_cfg cfg;
	/* Formatter program entries length */
	struct ccdc_fmtplen plen;
	/* first pixel in a line fed to formatter */
	unsigned short fmtrlen;
	/* HD interval for output line. Only valid when split line */
	unsigned short fmthcnt;
	/* formatter address pointers */
	struct ccdc_fmt_addr_ptr fmtaddr_ptr[16];
	/* program enable/disable */
	unsigned char pgm_en[32];
	/* program address pointers */
	struct ccdc_fmtpgm_ap fmtpgm_ap[32];
};

struct ccdc_df_csc {
	/* Color Space Conversion confguration, 0 - csc, 1 - df */
	unsigned int df_or_csc;
	/* csc configuration valid if df_or_csc is 0 */
	struct ccdc_color_space_conv csc;
	/* data formatter configuration valid if df_or_csc is 1 */
	struct ccdc_data_formatter df;
	/* start pixel in a line at the input */
	unsigned int start_pix;
	/* number of pixels in input line */
	unsigned int num_pixels;
	/* start line at the input */
	unsigned int start_line;
	/* number of lines at the input */
	unsigned int num_lines;
};

struct ccdc_gain_offsets_adj {
	/* Gain adjustment per color */
	struct ccdc_gain gain;
	/* Offset adjustment */
	unsigned short offset;
	/* Enable or Disable Gain adjustment for SDRAM data */
	unsigned char gain_sdram_en;
	/* Enable or Disable Gain adjustment for IPIPE data */
	unsigned char gain_ipipe_en;
	/* Enable or Disable Gain adjustment for H3A data */
	unsigned char gain_h3a_en;
	/* Enable or Disable Gain adjustment for SDRAM data */
	unsigned char offset_sdram_en;
	/* Enable or Disable Gain adjustment for IPIPE data */
	unsigned char offset_ipipe_en;
	/* Enable or Disable Gain adjustment for H3A data */
	unsigned char offset_h3a_en;
};

struct ccdc_cul {
	/* Horizontal Cull pattern for odd lines */
	unsigned char hcpat_odd;
	/* Horizontal Cull pattern for even lines */
	unsigned char hcpat_even;
	/* Vertical Cull pattern */
	unsigned char vcpat;
	/* Enable or disable lpf. Apply when cull is enabled */
	unsigned char en_lpf;
};

enum ccdc_compress_alg {
	CCDC_ALAW,
	CCDC_DPCM,
	CCDC_NO_COMPRESSION
};

struct ccdc_compress {
	/* Enable or diable A-Law or DPCM compression. */
	enum ccdc_compress_alg alg;
	/* Predictor for DPCM compression */
	enum ccdc_dpcm_predictor pred;
};

struct ccdc_module_params {
	/* Linearization parameters for image sensor data input */
	struct ccdc_linearize linearize;
	/* Data formatter or CSC */
	struct ccdc_df_csc df_csc;
	/* Defect Pixel Correction (DFC) confguration */
	struct ccdc_dfc dfc;
	/* Black/Digital Clamp configuration */
	struct ccdc_black_clamp bclamp;
	/* Gain, offset adjustments */
	struct ccdc_gain_offsets_adj gain_offset;
	/* Culling */
	struct ccdc_cul culling;
	/* A-Law and DPCM compression options */
	struct ccdc_compress compress;
	/* horizontal offset for Gain/LSC/DFC */
	unsigned short horz_offset;
	/* vertical offset for Gain/LSC/DFC */
	unsigned short vert_offset;
};

struct ccdc_param {
	/* 0 - raw bayer , 1 - raw ycbcr */
	unsigned char if_type;
	/* configuration param */
	union cfg_t {
		struct ccdc_bayer_config bayer;
		struct ccdc_ycbcr_config ycbcr;
	} cfg;
	/* Raw bayer path modules configuration params */
	struct ccdc_module_params *module_params;
};

#ifdef __KERNEL__

#define ISNULL(val) 	((val == NULL) ? 1 : 0)

enum ccdc_data_pack {
	CCDC_PACK_16BIT,
	CCDC_PACK_12BIT,
	CCDC_PACK_8BIT
};

#define ISIF_IOBASE_ADDR	               	0x01c71000
#define ISP5_IOBASE_ADDR			0x01c70000
#define CCDC_WIN_PAL				{0, 0, 720, 576}
#define CCDC_WIN_VGA				{0, 0, 640, 480}

#define ISP5_CCDCMUX				0x20
#define ISP5_PG_FRAME_SIZE 			0x28

#define CCDC_MAX_CONTROLS 5
/* ISIF registers relative offsets */
#define SYNCEN					0x00
#define MODESET					0x04
#define HDW					0x08
#define VDW					0x0c
#define PPLN					0x10
#define LPFR					0x14
#define SPH					0x18
#define LNH					0x1c
#define SLV0					0x20
#define SLV1					0x24
#define LNV					0x28
#define CULH					0x2c
#define CULV					0x30
#define HSIZE					0x34
#define SDOFST					0x38
#define CADU					0x3c
#define CADL					0x40
#define LINCFG0					0x44
#define LINCFG1					0x48
#define CCOLP					0x4c
#define CRGAIN 					0x50
#define CGRGAIN					0x54
#define CGBGAIN					0x58
#define CBGAIN					0x5c
#define COFSTA					0x60
#define FLSHCFG0				0x64
#define FLSHCFG1				0x68
#define FLSHCFG2				0x6c
#define VDINT0					0x70
#define VDINT1					0x74
#define VDINT2					0x78
#define MISC 					0x7c
#define CGAMMAWD				0x80
#define REC656IF				0x84
#define CCDCFG					0x88
/*****************************************************
* Defect Correction registers
*****************************************************/
#define DFCCTL					0x8c
#define VDFSATLV				0x90
#define DFCMEMCTL				0x94
#define DFCMEM0					0x98
#define DFCMEM1					0x9c
#define DFCMEM2					0xa0
#define DFCMEM3					0xa4
#define DFCMEM4					0xa8
/****************************************************
* Black Clamp registers
****************************************************/
#define CLAMPCFG				0xac
#define CLDCOFST				0xb0
#define CLSV					0xb4
#define CLHWIN0					0xb8
#define CLHWIN1					0xbc
#define CLHWIN2					0xc0
#define CLVRV					0xc4
#define CLVWIN0					0xc8
#define CLVWIN1					0xcc
#define CLVWIN2					0xd0
#define CLVWIN3					0xd4
/****************************************************
* Lense Shading Correction
****************************************************/
#define DATAHOFST				0xd8
#define DATAVOFST				0xdc
#define LSCHVAL					0xe0
#define LSCVVAL					0xe4
#define TWODLSCCFG				0xe8
#define TWODLSCOFST				0xec
#define TWODLSCINI				0xf0
#define TWODLSCGRBU				0xf4
#define TWODLSCGRBL				0xf8
#define TWODLSCGROF				0xfc
#define TWODLSCORBU				0x100
#define TWODLSCORBL				0x104
#define TWODLSCOROF				0x108
#define TWODLSCIRQEN				0x10c
#define TWODLSCIRQST				0x110
/****************************************************
* Data formatter
****************************************************/
#define FMTCFG					0x114
#define FMTPLEN					0x118
#define FMTSPH					0x11c
#define FMTLNH					0x120
#define FMTSLV					0x124
#define FMTLNV					0x128
#define FMTRLEN					0x12c
#define FMTHCNT					0x130
#define FMTAPTR_BASE				0x134
/* Below macro for addresses FMTAPTR0 - FMTAPTR15 */
#define FMTAPTR(i)			(FMTAPTR_BASE + (i * 4))
#define FMTPGMVF0				0x174
#define FMTPGMVF1				0x178
#define FMTPGMAPU0				0x17c
#define FMTPGMAPU1				0x180
#define FMTPGMAPS0				0x184
#define FMTPGMAPS1				0x188
#define FMTPGMAPS2				0x18c
#define FMTPGMAPS3				0x190
#define FMTPGMAPS4				0x194
#define FMTPGMAPS5				0x198
#define FMTPGMAPS6				0x19c
#define FMTPGMAPS7				0x1a0
/************************************************
* Color Space Converter
************************************************/
#define CSCCTL					0x1a4
#define CSCM0					0x1a8
#define CSCM1					0x1ac
#define CSCM2					0x1b0
#define CSCM3					0x1b4
#define CSCM4					0x1b8
#define CSCM5					0x1bc
#define CSCM6					0x1c0
#define CSCM7					0x1c4
#define OBWIN0					0x1c8
#define OBWIN1					0x1cc
#define OBWIN2					0x1d0
#define OBWIN3					0x1d4
#define OBVAL0					0x1d8
#define OBVAL1					0x1dc
#define OBVAL2					0x1e0
#define OBVAL3					0x1e4
#define OBVAL4					0x1e8
#define OBVAL5					0x1ec
#define OBVAL6					0x1f0
#define OBVAL7					0x1f4
#define CLKCTL					0x1f8

#define CCDC_LINEAR_LUT0_ADDR			0x1C7C000
#define CCDC_LINEAR_LUT1_ADDR			0x1C7C400

/* Masks & Shifts below */
#define START_PX_HOR_MASK			(0x7FFF)
#define NUM_PX_HOR_MASK				(0x7FFF)
#define START_VER_ONE_MASK			(0x7FFF)
#define START_VER_TWO_MASK			(0x7FFF)
#define NUM_LINES_VER				(0x7FFF)

/* gain - offset masks */
#define GAIN_INTEGER_MASK			(0x7)
#define GAIN_INTEGER_SHIFT			(0x9)
#define GAIN_DECIMAL_MASK			(0x1FF)
#define OFFSET_MASK			  	(0xFFF)
#define GAIN_SDRAM_EN_SHIFT			(12)
#define GAIN_IPIPE_EN_SHIFT			(13)
#define GAIN_H3A_EN_SHIFT			(14)
#define OFST_SDRAM_EN_SHIFT			(8)
#define OFST_IPIPE_EN_SHIFT			(9)
#define OFST_H3A_EN_SHIFT			(10)
#define GAIN_OFFSET_EN_MASK			(0x7700)

/* Culling */
#define CULL_PAT_EVEN_LINE_SHIFT		(8)

/* CCDCFG register */
#define CCDC_YCINSWP_RAW			(0x00 << 4)
#define CCDC_YCINSWP_YCBCR			(0x01 << 4)
#define CCDC_CCDCFG_FIDMD_LATCH_VSYNC		(0x00 << 6)
#define CCDC_CCDCFG_WENLOG_AND			(0x00 << 8)
#define CCDC_CCDCFG_TRGSEL_WEN			(0x00 << 9)
#define CCDC_CCDCFG_EXTRG_DISABLE		(0x00 << 10)
#define CCDC_LATCH_ON_VSYNC_DISABLE		(0x01 << 15)
#define CCDC_LATCH_ON_VSYNC_ENABLE		(0x00 << 15)
#define CCDC_DATA_PACK_MASK			(0x03)
#define CCDC_DATA_PACK16			(0x0)
#define CCDC_DATA_PACK12			(0x1)
#define CCDC_DATA_PACK8				(0x2)
#define CCDC_PIX_ORDER_SHIFT			(11)
#define CCDC_PIX_ORDER_MASK			(0x01)
#define CCDC_BW656_ENABLE			(0x01 << 5)

/* MODESET registers */
#define CCDC_VDHDOUT_INPUT			(0x00 << 0)
#define CCDC_INPUT_MASK				(0x03)
#define CCDC_INPUT_SHIFT			(12)
#define CCDC_RAW_INPUT_MODE			(0x00)
#define CCDC_FID_POL_MASK			(0x01)
#define CCDC_FID_POL_SHIFT			(4)
#define CCDC_HD_POL_MASK			(0x01)
#define CCDC_HD_POL_SHIFT			(3)
#define CCDC_VD_POL_MASK			(0x01)
#define CCDC_VD_POL_SHIFT			(2)
#define CCDC_DATAPOL_NORMAL			(0x00)
#define CCDC_DATAPOL_MASK			(0x01)
#define CCDC_DATAPOL_SHIFT			(6)
#define CCDC_EXWEN_DISABLE 			(0x00)
#define CCDC_EXWEN_MASK				(0x01)
#define CCDC_EXWEN_SHIFT			(5)
#define CCDC_FRM_FMT_MASK			(0x01)
#define CCDC_FRM_FMT_SHIFT			(7)
#define CCDC_DATASFT_MASK			(0x07)
#define CCDC_DATASFT_SHIFT			(8)
#define CCDC_LPF_SHIFT				(14)
#define CCDC_LPF_MASK				(0x1)

/* GAMMAWD registers */
#define CCDC_ALAW_GAMA_WD_MASK			(0xF)
#define CCDC_ALAW_GAMA_WD_SHIFT			(1)
#define CCDC_ALAW_ENABLE			(0x01)
#define CCDC_GAMMAWD_CFA_MASK			(0x01)
#define CCDC_GAMMAWD_CFA_SHIFT			(5)

/* HSIZE registers */
#define CCDC_HSIZE_FLIP_MASK			(0x01)
#define CCDC_HSIZE_FLIP_SHIFT			(12)
#define CCDC_LINEOFST_MASK			(0xFFF)

/* MISC registers */
#define CCDC_DPCM_EN_SHIFT			(12)
#define CCDC_DPCM_EN_MASK			(1)
#define CCDC_DPCM_PREDICTOR_SHIFT		(13)
#define CCDC_DPCM_PREDICTOR_MASK 		(1)

/* Black clamp related */
#define CCDC_BC_DCOFFSET_MASK			(0x1FFF)
#define CCDC_BC_MODE_COLOR_MASK			(1)
#define CCDC_BC_MODE_COLOR_SHIFT		(4)
#define CCDC_HORZ_BC_MODE_MASK			(3)
#define CCDC_HORZ_BC_MODE_SHIFT			(1)
#define CCDC_HORZ_BC_WIN_COUNT_MASK		(0x1F)
#define CCDC_HORZ_BC_WIN_SEL_SHIFT		(5)
#define CCDC_HORZ_BC_PIX_LIMIT_SHIFT		(6)
#define CCDC_HORZ_BC_WIN_H_SIZE_MASK		(3)
#define CCDC_HORZ_BC_WIN_H_SIZE_SHIFT		(8)
#define CCDC_HORZ_BC_WIN_V_SIZE_MASK		(3)
#define CCDC_HORZ_BC_WIN_V_SIZE_SHIFT		(12)
#define CCDC_HORZ_BC_WIN_START_H_MASK		(0x1FFF)
#define CCDC_HORZ_BC_WIN_START_V_MASK		(0x1FFF)
#define CCDC_VERT_BC_OB_H_SZ_MASK		(7)
#define CCDC_VERT_BC_RST_VAL_SEL_MASK		(3)
#define	CCDC_VERT_BC_RST_VAL_SEL_SHIFT		(4)
#define CCDC_VERT_BC_LINE_AVE_COEF_SHIFT	(8)
#define	CCDC_VERT_BC_OB_START_HORZ_MASK		(0x1FFF)
#define CCDC_VERT_BC_OB_START_VERT_MASK		(0x1FFF)
#define CCDC_VERT_BC_OB_VERT_SZ_MASK		(0x1FFF)
#define CCDC_VERT_BC_RST_VAL_MASK		(0xFFF)
#define CCDC_BC_VERT_START_SUB_V_MASK		(0x1FFF)

/* VDFC registers */
#define CCDC_VDFC_EN_SHIFT			(4)
#define CCDC_VDFC_CORR_MOD_MASK			(3)
#define CCDC_VDFC_CORR_MOD_SHIFT		(5)
#define CCDC_VDFC_CORR_WHOLE_LN_SHIFT		(7)
#define CCDC_VDFC_LEVEL_SHFT_MASK		(7)
#define CCDC_VDFC_LEVEL_SHFT_SHIFT		(8)
#define CCDC_VDFC_SAT_LEVEL_MASK		(0xFFF)
#define CCDC_VDFC_POS_MASK			(0x1FFF)
#define CCDC_DFCMEMCTL_DFCMARST_SHIFT		(2)

/* CSC registers */
#define CCDC_CSC_COEF_INTEG_MASK		(7)
#define CCDC_CSC_COEF_DECIMAL_MASK		(0x1f)
#define CCDC_CSC_COEF_INTEG_SHIFT		(5)
#define CCDC_CSCM_MSB_SHIFT			(8)
#define CCDC_DF_CSC_SPH_MASK			(0x1FFF)
#define CCDC_DF_CSC_LNH_MASK			(0x1FFF)
#define CCDC_DF_CSC_SLV_MASK			(0x1FFF)
#define CCDC_DF_CSC_LNV_MASK			(0x1FFF)

/* Offsets for LSC/DFC/Gain */
#define CCDC_DATA_H_OFFSET_MASK			(0x1FFF)
#define CCDC_DATA_V_OFFSET_MASK			(0x1FFF)

/* Linearization */
#define CCDC_LIN_CORRSFT_MASK			(7)
#define CCDC_LIN_CORRSFT_SHIFT			(4)
#define CCDC_LIN_SCALE_FACT_INTEG_SHIFT		(10)
#define CCDC_LIN_SCALE_FACT_DECIMAL_MASK	(0x3FF)
#define CCDC_LIN_ENTRY_MASK			(0x3FF)

#define CCDC_DF_FMTRLEN_MASK			(0x1FFF)
#define CCDC_DF_FMTHCNT_MASK			(0x1FFF)

/* Pattern registers */
#define CCDC_PG_EN				(1 << 3)
#define CCDC_SEL_PG_SRC				(3 << 4)
#define CCDC_PG_VD_POL_SHIFT			(0)
#define CCDC_PG_HD_POL_SHIFT			(1)

#endif
#endif
