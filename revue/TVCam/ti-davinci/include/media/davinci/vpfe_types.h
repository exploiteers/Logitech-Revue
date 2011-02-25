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
 *
 * File for common vpss data types
 */
#ifndef _VPFE_TYPES_H
#define _VPFE_TYPES_H

#ifdef __KERNEL__

enum ccdc_hw_if_type {
	/* BT656 - 8 bit */
	CCDC_BT656,
	/* BT1120 - 16 bit */
	CCDC_BT1120,
	/* Raw Bayer */
	CCDC_RAW_BAYER,
	/* YCbCr - 8 bit with external sync */
	CCDC_YCBCR_SYNC_8,
	/* YCbCr - 16 bit with external sync */
	CCDC_YCBCR_SYNC_16,
	/* BT656 - 10 bit */
	CCDC_BT656_10BIT
};

enum vpfe_sync_pol {
	VPFE_SYNC_POSITIVE = 0,
	VPFE_SYNC_NEGATIVE
};

/* Pixel format to be used across vpfe driver */
enum vpfe_pix_formats {
	VPFE_BAYER_8BIT_PACK,
	VPFE_BAYER_8BIT_PACK_ALAW,
	VPFE_BAYER_8BIT_PACK_DPCM,
	VPFE_BAYER_12BIT_PACK,
	/* 16 bit Bayer */
	VPFE_BAYER,
	VPFE_UYVY,
	VPFE_YUYV,
	VPFE_RGB565,
	VPFE_RGB888,
	/* YUV 420 */
	VPFE_YUV420,
	/* YUV 420, Y data */
	VPFE_420_Y,
	/* YUV 420, C data */
	VPFE_420_C
};

/* interface description */
struct ccdc_hw_if_param {
	enum ccdc_hw_if_type if_type;
	enum vpfe_sync_pol hdpol;
	enum vpfe_sync_pol vdpol;
};

#endif
#endif
