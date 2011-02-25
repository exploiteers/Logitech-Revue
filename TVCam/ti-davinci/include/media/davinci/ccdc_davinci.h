/*
 * Copyright (C) 2006-2008 Texas Instruments Inc
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
#ifndef _CCDC_DAVINCI_H
#define _CCDC_DAVINCI_H
#include <media/davinci/ccdc_common.h>

/* Define to enable/disable video port */
#define VIDEO_PORT_ENABLE	(1)


/* enum for No of pixel per line to be avg. in Black Clamping*/
enum sample_length {
	_1PIXELS = 0,
	_2PIXELS,
	_4PIXELS,
	_8PIXELS,
	_16PIXELS
};

#define FP_NUM_BYTES					(4)
/* Define for extra pixel/line and extra lines/frame */
#define NUM_EXTRAPIXELS	    8
#define NUM_EXTRALINES		8

/* settings for commonly used video formats */
#define VPFE_WIN_NTSC    {0, 0, 720, 480}
#define VPFE_WIN_PAL     {0, 0, 720, 576}
/* ntsc square pixel */
#define VPFE_WIN_NTSC_SP {0, 0, 640, 480}
/* pal square pixel */
#define VPFE_WIN_PAL_SP  {0, 0, 768, 576}
#define VPFE_WIN_CIF     {0, 0, 352, 288}
#define VPFE_WIN_QCIF    {0, 0, 176, 144}
#define VPFE_WIN_QVGA    {0, 0, 320, 240}
#define VPFE_WIN_SIF     {0, 0, 352, 240}

#define VPFE_WIN_VGA	{0, 0, (640 + NUM_EXTRAPIXELS),\
		(480 + NUM_EXTRALINES)}
#define VPFE_WIN_SVGA 	{0, 0, (800 + NUM_EXTRAPIXELS),\
		(600 + NUM_EXTRALINES)}
#define VPFE_WIN_XGA	{0, 0, (1024 + NUM_EXTRAPIXELS),\
		(768 + NUM_EXTRALINES)}
#define VPFE_WIN_480p	{0, 0, (720 + NUM_EXTRAPIXELS),\
		(480 + NUM_EXTRALINES)}
#define VPFE_WIN_576p	{0, 0, (720 + NUM_EXTRAPIXELS),\
		(576 + NUM_EXTRALINES)}
#define VPFE_WIN_720p 	{0, 0, (1280 + NUM_EXTRAPIXELS),\
		(720 + NUM_EXTRALINES)}
#define VPFE_WIN_1080p 	{0, 0, 1920, 1080}

/* enum for No of lines in Black Clamping */
enum sample_line {
	_1LINES = 0,
	_2LINES,
	_4LINES,
	_8LINES,
	_16LINES
};

enum hw_frame {
	CCDC_RAW,
	CCDC_YCBCR
};

/* enum for Alaw gama width */
enum gama_width {
	BITS_15_6 = 0,
	BITS_14_5,
	BITS_13_4,
	BITS_12_3,
	BITS_11_2,
	BITS_10_1,
	BITS_09_0
};

enum data_size {
	_16BITS = 0,
	_15BITS,
	_14BITS,
	_13BITS,
	_12BITS,
	_11BITS,
	_10BITS,
	_8BITS
};

struct ccdc_imgwin {
	unsigned int top;
	unsigned int left;
	unsigned int width;
	unsigned int height;
};

/* structure for ALaw */
struct a_law {
	/* Enable/disable A-Law */
	unsigned char b_alaw_enable;
	/*Gama Width Input */
	enum gama_width gama_wd;
};

/* structure for Black Clamping */
struct black_clamp {
	/* only if bClampEnable is TRUE */
	unsigned char b_clamp_enable;
	/* only if bClampEnable is TRUE */
	enum sample_length sample_pixel;
	/* only if bClampEnable is TRUE */
	enum sample_line sample_ln;
	/* only if bClampEnable is TRUE */
	unsigned short start_pixel;
	/* only if bClampEnable is FALSE */
	unsigned short sgain;
	unsigned short dc_sub;
};

/* structure for Black Level Compensation */
struct black_compensation {
	/* Constant value to subtract from Red component */
	char r_comp;
	/* Constant value to subtract from Gr component */
	char gr_comp;
	/* Constant value to subtract from Blue component */
	char b_comp;
	/* Constant value to subtract from Gb component */
	char gb_comp;
};

/* structure for fault pixel correction */
struct fault_pixel {
	/*Enable or Disable fault pixel correction */
	unsigned char fpc_enable;
	/*Number of fault pixel */
	unsigned short fp_num;
	/*Address of fault pixel table */
	unsigned int fpc_table_addr;

};

/* Structure for CCDC configuration parameters for raw capture mode passed
 * by application
 */
struct ccdc_config_params_raw {
	/* pixel format */
	enum ccdc_pixfmt pix_fmt;
	/* progressive or interlaced frame */
	enum ccdc_frmfmt frm_fmt;
	/* video window */
	struct ccdc_imgwin win;
	/* field id polarity */
	enum ccdc_pinpol fid_pol;
	/* vertical sync polarity */
	enum ccdc_pinpol vd_pol;
	/* horizontal sync polarity */
	enum ccdc_pinpol hd_pol;
	/* enable to store the image in inverse
	 * order in memory(bottom to top)
	 */
	unsigned char image_invert_enable;
	/* data size value from 8 to 16 bits */
	enum data_size data_sz;
	/* Structure for Optional A-Law */
	struct a_law alaw;
	/* Structure for Optical Black Clamp */
	struct black_clamp blk_clamp;
	/* Structure for Black Compensation */
	struct black_compensation blk_comp;
	/* Structure for Fault Pixel Module Configuration */
	struct fault_pixel fault_pxl;
};

struct ccdc_params_ycbcr {
	/* pixel format */
	enum ccdc_pixfmt pix_fmt;
	/* progressive or interlaced frame */
	enum ccdc_frmfmt frm_fmt;
	/* video window */
	struct ccdc_imgwin win;
	/* field id polarity */
	enum ccdc_pinpol fid_pol;
	/* vertical sync polarity */
	enum ccdc_pinpol vd_pol;
	/* horizontal sync polarity */
	enum ccdc_pinpol hd_pol;
	/* enable BT.656 embedded sync mode */
	int bt656_enable;
	/* cb:y:cr:y or y:cb:y:cr in memory */
	enum ccdc_pixorder pix_order;
	/* interleaved or separated fields  */
	enum ccdc_buftype buf_type;
};

#ifdef __KERNEL__
#include <linux/io.h>
/* Structure for CCDC configuration parameters for raw capture mode */
struct ccdc_params_raw {
	/* pixel format */
	enum ccdc_pixfmt pix_fmt;
	/* progressive or interlaced frame */
	enum ccdc_frmfmt frm_fmt;
	/* video window */
	struct ccdc_imgwin win;
	/* field id polarity */
	enum ccdc_pinpol fid_pol;
	/* vertical sync polarity */
	enum ccdc_pinpol vd_pol;
	/* horizontal sync polarity */
	enum ccdc_pinpol hd_pol;
	/* interleaved or separated fields */
	enum ccdc_buftype buf_type;
	/* enable to store the image in inverse order in memory
	 * (bottom to top)
	 */
	unsigned char image_invert_enable;
	/* data size value from 8 to 16 bits */
	enum data_size data_sz;
	/* Structure for Optional A-Law */
	struct a_law alaw;
	/* Structure for Optical Black Clamp */
	struct black_clamp blk_clamp;
	/* Structure for Black Compensation */
	struct black_compensation blk_comp;
	/* Structure for Fault Pixel Module Configuration */
	struct fault_pixel fault_pxl;
};

/**************************************************************************\
* Register OFFSET Definitions
\**************************************************************************/
#define PID                             (0x0)
#define PCR                             (0x4)
#define SYN_MODE                        (0x8)
#define HD_VD_WID                       (0xc)
#define PIX_LINES                       (0x10)
#define HORZ_INFO                       (0x14)
#define VERT_START                      (0x18)
#define VERT_LINES                      (0x1c)
#define CULLING                         (0x20)
#define HSIZE_OFF                       (0x24)
#define SDOFST                          (0x28)
#define SDR_ADDR                        (0x2c)
#define CLAMP                           (0x30)
#define DCSUB                           (0x34)
#define COLPTN                          (0x38)
#define BLKCMP                          (0x3c)
#define FPC                             (0x40)
#define FPC_ADDR                        (0x44)
#define VDINT                           (0x48)
#define ALAW                            (0x4c)
#define REC656IF                        (0x50)
#define CCDCFG                          (0x54)
#define FMTCFG                          (0x58)
#define FMT_HORZ                        (0x5c)
#define FMT_VERT                        (0x60)
#define FMT_ADDR0                       (0x64)
#define FMT_ADDR1                       (0x68)
#define FMT_ADDR2                       (0x6c)
#define FMT_ADDR3                       (0x70)
#define FMT_ADDR4                       (0x74)
#define FMT_ADDR5                       (0x78)
#define FMT_ADDR6                       (0x7c)
#define FMT_ADDR7                       (0x80)
#define PRGEVEN_0                       (0x84)
#define PRGEVEN_1                       (0x88)
#define PRGODD_0                        (0x8c)
#define PRGODD_1                        (0x90)
#define VP_OUT                          (0x94)

#define CCDC_BASE_ADDR 					0x01c70400
#define VPSS_SB_BASE_ADDR 				0x01c73400

/***************************************************************
*	Define for various register bit mask and shifts for CCDC
****************************************************************/
#define CCDC_FID_POL_MASK			(0x01)
#define CCDC_FID_POL_SHIFT			(4)
#define CCDC_HD_POL_MASK			(0x01)
#define CCDC_HD_POL_SHIFT			(3)
#define CCDC_VD_POL_MASK			(0x01)
#define CCDC_VD_POL_SHIFT			(2)
#define CCDC_HSIZE_OFF_MASK			(0xffffffe0)
#define CCDC_32BYTE_ALIGN_VAL			(31)
#define CCDC_FRM_FMT_MASK			(0x01)
#define CCDC_FRM_FMT_SHIFT			(7  )
#define CCDC_DATA_SZ_MASK			(0x07)
#define CCDC_DATA_SZ_SHIFT			(8)
#define CCDC_PIX_FMT_MASK			(0x03)
#define CCDC_PIX_FMT_SHIFT			(12)
#define CCDC_VP2SDR_DISABLE			(0xFFFBFFFF)
#define CCDC_WEN_ENABLE				(0x01 << 17)
#define CCDC_SDR2RSZ_DISABLE			(0xFFF7FFFF)
#define CCDC_VDHDEN_ENABLE			(0x01 << 16)
#define CCDC_LPF_ENABLE				(0x01 << 14)
#define CCDC_ALAW_ENABLE			(0x01 << 3 )
#define CCDC_ALAW_GAMA_WD_MASK			(0x07)
#define CCDC_BLK_CLAMP_ENABLE			(0x01 << 31)
#define CCDC_BLK_SGAIN_MASK			(0x1F )
#define CCDC_BLK_ST_PXL_MASK			(0x7FFF)
#define CCDC_BLK_ST_PXL_SHIFT			(10)
#define CCDC_BLK_SAMPLE_LN_MASK			(0x07)
#define CCDC_BLK_SAMPLE_LN_SHIFT		(28)
#define CCDC_BLK_SAMPLE_LINE_MASK		(0x07)
#define CCDC_BLK_SAMPLE_LINE_SHIFT		(25)
#define CCDC_BLK_DC_SUB_MASK			(0x03FFF)
#define CCDC_BLK_COMP_MASK			(0x000000FF)
#define CCDC_BLK_COMP_GB_COMP_SHIFT		(8)
#define CCDC_BLK_COMP_GR_COMP_SHIFT		(16)
#define CCDC_BLK_COMP_R_COMP_SHIFT		(24)
#define CCDC_LATCH_ON_VSYNC_DISABLE		(0x01 << 15)
#define CCDC_FPC_ENABLE				(0x01 << 15)
#define CCDC_FPC_DISABLE			(0x0)
#define CCDC_FPC_FPC_NUM_MASK 			(0x7FFF)
#define CCDC_DATA_PACK_ENABLE			(0x01<<11)
#define CCDC_FMTCFG_VPIN_MASK			(0x07)
#define CCDC_FMTCFG_VPIN_SHIFT			(12)
#define CCDC_FMT_HORZ_FMTLNH_MASK		(0x1FFF)
#define CCDC_FMT_HORZ_FMTSPH_MASK		(0x1FFF)
#define CCDC_FMT_HORZ_FMTSPH_SHIFT		(16 )
#define CCDC_FMT_VERT_FMTLNV_MASK		(0x1FFF)
#define CCDC_FMT_VERT_FMTSLV_MASK		(0x1FFF)
#define CCDC_FMT_VERT_FMTSLV_SHIFT		(16 )
#define CCDC_VP_OUT_VERT_NUM_MASK		(0x3FFF)
#define CCDC_VP_OUT_VERT_NUM_SHIFT		(17)
#define CCDC_VP_OUT_HORZ_NUM_MASK		(0x1FFF)
#define CCDC_VP_OUT_HORZ_NUM_SHIFT		(4)
#define CCDC_VP_OUT_HORZ_ST_MASK		(0x000F)
#define CCDC_HORZ_INFO_SPH_SHIFT		(16)
#define CCDC_VERT_START_SLV0_SHIFT		(16)
#define CCDC_VDINT_VDINT0_SHIFT			(16)
#define CCDC_VDINT_VDINT1_MASK			(0xFFFF)

/* SBL register and mask defination */
#define SBL_PCR_VPSS				(4)
#define SBL_PCR_CCDC_WBL_O			(0xFF7FFFFF)

#define PPC_RAW					(1)
#define DCSUB_DEFAULT_VAL			(0)
#define CLAMP_DEFAULT_VAL			(0)
#define ENABLE_VIDEO_PORT			(0x00008000)
#define DISABLE_VIDEO_PORT			(0)
#define CCDC_COLPTN_VAL				(0xBB11BB11)
#define TWO_BYTES_PER_PIXEL			(2)
#define INTERLACED_IMAGE_INVERT			(0x4B6D)
#define INTERLACED_NO_IMAGE_INVERT		(0x0249)
#define PROGRESSIVE_IMAGE_INVERT		(0x4000)
#define PROGRESSIVE_NO_IMAGE_INVERT		(0)
#define CCDC_INTERLACED_HEIGHT_SHIFT		(1)

#endif
#endif				/* CCDC_DAVINCI_H */
