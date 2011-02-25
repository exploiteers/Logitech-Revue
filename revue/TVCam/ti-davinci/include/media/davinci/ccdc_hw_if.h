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
 * ccdc interface for accessing ccdc APIs from VPFE
 */
#ifndef _CCDC_HW_IF_H
#define _CCDC_HW_IF_H

#ifdef __KERNEL__
#include <linux/videodev.h>
#include <linux/videodev2.h>
#include <media/davinci/ccdc_common.h>
#include <media/davinci/vpfe_types.h>

/*
 * Maximum number of capture channels supported by CCDC
 */
#define CCDC_CAPTURE_NUM_CHANNELS 1

/*
 * ccdc device interface
 */
struct ccdc_hw_interface {

	/* CCDC device name */
	char *name;

	/* Pointer to initialize function to initialize ccdc device */
	int (*initialize) (void);
	/* Set of functions pointers for control related functions.
	 * Use queryctrl of decoder interface to check if it is a decoder
	 * control id. If not passed to ccdc to process it
	 */
	void (*enable) (int en);
	/*
	 * Pointer to function to enable or disable ccdc
	 */
	u32 (*reset) (void);
	/* reset sbl. only for 6446 */
	void (*enable_out_to_sdram) (int en);
	/* Pointer to function to set hw frame type */
	int (*set_hw_if_type) (enum ccdc_hw_if_type iface);
	/* get interface parameters */
	int (*get_hw_if_params) (struct ccdc_hw_if_param *param);
	/* Set/Get parameters in CCDC */
	/* Pointer to function to set parameters. Used
	 * for implementing VPFE_S_CCDC_PARAMS
	 */
	int (*setparams) (void *params);
	/* Pointer to function to get parameter. Used
	 * for implementing VPFE_G_CCDC_PARAMS
	 */
	int (*getparams) (void *params);
	/* Pointer to function to configure ccdc */
	int (*configure) (int mode);

	/* Set of functions pointers for format related functions */
	/* Pointer to function to try format */
	int (*tryformat) (struct v4l2_pix_format *argp);
	/* Pointer to function to set buffer type */
	int (*set_buftype) (enum ccdc_buftype buf_type);
	/* Pointer to function to get buffer type */
	int (*get_buftype) (enum ccdc_buftype *buf_type);
	/* Pointer to function to set frame format */
	int (*set_frame_format) (enum ccdc_frmfmt frm_fmt);
	/* Pointer to function to get frame format */
	int (*get_frame_format) (enum ccdc_frmfmt *frm_fmt);
	/* Pointer to function to set buffer type */
	int (*get_pixelformat) (unsigned int *pixfmt);
	/* Pointer to function to get pixel format. Uses V4L2 type */
	int (*set_pixelformat) (unsigned int pixfmt);
	/* Pointer to function to set image window */
	int (*set_image_window) (struct v4l2_rect *win);
	/* Pointer to function to set image window */
	int (*get_image_window) (struct v4l2_rect *win);
	/* Pointer to function to set line length */
	int (*set_line_length) (unsigned int len);
	/* Pointer to function to get line length */
	int (*get_line_length) (unsigned int *len);
	/* Pointer to function to set crop window */
	int (*setcropwindow) (struct v4l2_rect *win);
	/* Pointer to function to get crop window */
	int (*getcropwindow) (struct v4l2_rect *win);

	/* Query SoC control IDs */
	int (*queryctrl)(struct	v4l2_queryctrl *qctrl);
	/* Set SoC control */
	int (*setcontrol)(struct v4l2_control *ctrl);
	/* Get SoC control */
	int (*getcontrol)(struct v4l2_control *ctrl);
	/* Pointer to function to set current standard info */
	int (*setstd) (char *std);
	/* Pointer to function to get current standard info */
	int (*getstd_info) (struct ccdc_std_info *std_info);

	/* Pointer to function to set frame buffer address */
	void (*setfbaddr) (unsigned long addr);
	/* Pointer to function to get field id */
	int (*getfid) (void);
	/* Pointer to deinitialize function */
	int (*deinitialize) (void);
};

/*
 * CCDC modules implement this API which is used by VPFE to
 * access the operations on ccdc device
 */
struct ccdc_hw_interface *ccdc_get_hw_interface(void);

#endif
#endif
