/*
 * Copyright (C) 2008 MontaVista Software Inc.
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

#ifndef __DA8XX_FB_H__
#define __DA8XX_FB_H__

#define QVGA_HEIGHT    240
#define QVGA_WIDTH     320
#define PALETTE_SIZE 256
#define LEFT_MARGIN   64
#define RIGHT_MARGIN  64
#define UPPER_MARGIN  32
#define LOWER_MARGIN  32

/* ioctls */
#define FBIOGET_CONTRAST        _IOR('F', 1, int)
#define FBIOPUT_CONTRAST        _IOW('F', 2, int)
#define FBIGET_BRIGHTNESS       _IOR('F', 3, int)
#define FBIPUT_BRIGHTNESS       _IOW('F', 3, int)
#define FBIGET_COLOR       _IOR('F', 5, int)
#define FBIPUT_COLOR       _IOW('F', 6, int)
#define FBIOPUT_SINGLE_BUFFER	_IO('F', 7)
#define FBIOPUT_DUAL_BUFFER     _IO('F', 8)
#define FBIPUT_HSYNC       _IOW('F', 9, int)
#define FBIPUT_VSYNC       _IOW('F', 10, int)

enum panel_type {
	QVGA = 0
};

enum panel_shade {
	MONOCROME = 0,
	COLOR_ACTIVE,
	COLOR_PASSIVE,
};

enum raster_load_mode {
	LOAD_DATA = 1,
	LOAD_PALETTE,
};

struct display_panel {
	enum panel_type panel_type; /* QVGA */
	int         max_bpp;
	int         min_bpp;
	unsigned short        width;
	unsigned short        height;
	enum panel_shade panel_shade;
};

struct lcd_ctrl_config {
	const struct display_panel  *p_disp_panel;
	int             hfp;              /* Horizontal front porch */
	int             hbp;              /* Horizontal back porch */
	int             hsw;              /* Horizontal Sync Pulse Width */
	int             vfp;              /* Vertical front porch */
	int             vbp;              /* Vertical back porch */
	int             vsw;              /* Vertical Sync Pulse Width */
	int             ac_bias;          /* AC Bias Pin Frequency */
	int             ac_bias_intrpt;   /* AC Bias Pin Transitions
					     per Interrupt */
	int             dma_burst_sz;     /* DMA burst size */
	int             bpp;              /* Bits per pixel */
	int             fdd;              /* FIFO DMA Request Delay */
	int             pxl_clk;          /* Pixel clock */
	unsigned char   tft_alt_mode;     /* TFT Alternative Signal Mapping
					     (Only for active) */
	unsigned char   stn_565_mode;     /* 12 Bit Per Pixel (5-6-5) Mode
					     (Only for passive) */
	unsigned char   mono_8bit_mode;   /* Mono 8-bit Mode: 1=D0-D7
					     or 0=D0-D3 */
	unsigned char   invert_pxl_clock; /* Invert pixel clock */
	unsigned char   invert_line_clock;/* Invert line clock */
	unsigned char   invert_frm_clock; /* Invert frame clock  */
	unsigned char   sync_edge;        /* Horizontal and Vertical Sync Edge:
					     0=rising 1=falling */
	unsigned char   sync_ctrl;        /* Horizontal and Vertical Sync
					     Control: 0=ignore */
	unsigned char   raster_order;     /* Raster Data Order Select:
					     1=Most-to-least 0=Least-to-most */
};

struct lcd_contrast_arg {
	unsigned char is_up;
	unsigned char cnt;
};
struct lcd_sync_arg {
	int back_porch;
	int front_porch;
	int pulse_width;
};


#endif
