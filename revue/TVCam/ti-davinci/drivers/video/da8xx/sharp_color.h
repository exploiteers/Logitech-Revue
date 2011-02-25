/*
 *  Copyright (C) 2005 Texas Instruments, Inc.  All rights reserved.
 *
 *  This program is free software; you can distribute it and/or modify it
 *  under the terms of the GNU General Public License (Version 2) as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place - Suite 330, Boston MA 02111-1307, USA.
 */

/*******************************************************************************
 * FILE PURPOSE:    Graphical LCD Configuration file
 *******************************************************************************
 * FILE NAME:       sharp_color.h
 *
 * DESCRIPTION:     Configuration file for Sharp LQ057Q3DC02 Color GLCD
 *
 * AUTHOR:			hai@ti.com
 *
 * (C) Copyright 2005, Texas Instruments, Inc
 ******************************************************************************/

#define TI_GLCD_SHARP_COLOR_MIN_CONTRAST     0
#define TI_GLCD_SHARP_COLOR_MAX_CONTRAST    31

static const struct display_panel disp_panel = {
	QVGA,
	16,
	16,
    QVGA_WIDTH,  /* width          */
    QVGA_HEIGHT, /* height         */
    COLOR_ACTIVE,
};

static const struct lcd_ctrl_config lcd_cfg = {
	&disp_panel, /* p_disp_panel   */
	.hfp			= 8,   /* hfp            */
	.hbp			= 6,   /* hbp            */
	.hsw			= 0,   /* hsw            */
	.vfp			= 2,    /* vfp            */
	.vbp			= 2,    /* vbp            */
	.vsw			= 0,    /* vsw            */
	.ac_bias		= 255,  /* ac bias        */
	.ac_bias_intrpt		= 0,    /* ac bias intrpt */
	.dma_burst_sz		= 16,   /* dma_burst_sz   */
	.bpp			= 16,   /* bpp            */
	.fdd			= 255,  /* fdd            */
	.pxl_clk		= 0x1e,    /* pxl_clk        */
	.tft_alt_mode		= 0,    /* tft_alt_mode   */
	.stn_565_mode		= 0,    /* stn_565_mode   */
	.mono_8bit_mode		= 0,    /* mono_8bit_mode    */
	.invert_pxl_clock	= 1,    /* invert_pxl_clock  */
	.invert_line_clock	= 1,    /* invert_line_clock */
	.invert_frm_clock	= 1,    /* invert_frm_clock  */
	.sync_edge		= 0,    /* sync_edge         */
	.sync_ctrl		= 1,    /* sync_ctrl         */
	.raster_order		= 0,    /* raster_order      */
};
