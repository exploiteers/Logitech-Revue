/*
 * Copyright (C) 2008 Texas Instruments Inc
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * ccdc_common.h file
 **************************************************************************/
#ifndef _CCDC_COMMON_H
#define _CCDC_COMMON_H
enum ccdc_pixfmt {
	CCDC_PIXFMT_RAW,
	CCDC_PIXFMT_YCBCR_16BIT,
	CCDC_PIXFMT_YCBCR_8BIT
};

enum ccdc_frmfmt {
	CCDC_FRMFMT_PROGRESSIVE,
	CCDC_FRMFMT_INTERLACED
};

/* PIXEL ORDER IN MEMORY from LSB to MSB */
/* only applicable for 8-bit input mode  */
enum ccdc_pixorder {
	CCDC_PIXORDER_YCBYCR,
	CCDC_PIXORDER_CBYCRY,
};

enum ccdc_buftype {
	CCDC_BUFTYPE_FLD_INTERLEAVED,
	CCDC_BUFTYPE_FLD_SEPARATED
};

enum ccdc_pinpol {
	CCDC_PINPOL_POSITIVE,
	CCDC_PINPOL_NEGATIVE
};

#define CCDC_MAX_NAME   30
#define CCDC_PIXELASPECT_NTSC       {11, 10}
#define CCDC_PIXELASPECT_PAL        {54, 59}
#define CCDC_PIXELASPECT_NTSC_SP        {1, 1}
#define CCDC_PIXELASPECT_PAL_SP         {1, 1}
#define CCDC_PIXELASPECT_DEFAULT        {1, 1}

#ifdef __KERNEL__
#include <linux/types.h>

struct ccdc_pix_aspect {
	u32 numerator;
	u32 denominator;
};

/* This structure will store size parameters as per the mode selected by
 * the user
 */
struct ccdc_channel_config_params {
	/* Name of the */
	char *name;
	/* Indicates width of the image for this mode */
	u16 width;
	/* Indicates height of the image for this mode */
	u16 height;
	u8 fps;
	/* Indicates whether this is interlaced or progressive format */
	u8 frm_fmt;
	/* hd or sd */
	u8 hd_sd;
	/* pix aspect of input */
	struct ccdc_pix_aspect pixaspect;
};

/*
 * structure for accessing standard info from ccdc device
 */
struct ccdc_std_info {
	u8 channel_id;
	/* name of the standard */
	char name[CCDC_MAX_NAME];
	/* active pixels */
	u32 activepixels;
	/* active lines */
	u32 activelines;
	/* frame per second */
	u16 fps;
	/* frame format. interlaced or progressive */
	u8 frame_format;
	/* hd or sd */
	u8 hd_sd;
	/* pixel aspect */
	struct ccdc_pix_aspect pixaspect;
};

int ccdc_get_mode_info(struct ccdc_std_info *std_info);

#endif
#endif
