/*
 * Copyright (C) 2007-2008 Texas Instruments Inc
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
 */
/* tvp514x.h file. Header file for tvp5146 and tvp5147 driver modules */

#ifndef TVP514X_H
#define TVP514X_H

#ifdef __KERNEL__

/* Kernel Header files */
#include <linux/i2c.h>
#include <linux/device.h>
#include <linux/videodev.h>
#include <linux/videodev2.h>
#include <media/davinci/vid_decoder_if.h>

#endif				/* __KERNEL__ */

#define VPFE_STD_625_50_SQP	((v4l2_std_id)0x0000000100000000ULL)
#define VPFE_STD_525_60_SQP	((v4l2_std_id)0x0000000200000000ULL)
#define VPFE_STD_AUTO		((v4l2_std_id)0x0000000400000000ULL)
#define VPFE_STD_AUTO_SQP	((v4l2_std_id)0x0000000800000000ULL)

#define V4L2_STD_TVP514X_ALL	(V4L2_STD_525_60 | V4L2_STD_625_50 |\
	VPFE_STD_AUTO | VPFE_STD_625_50_SQP |\
	VPFE_STD_525_60_SQP | VPFE_STD_AUTO_SQP)

struct tvp514x_params {
	v4l2_std_id std;
	int inputidx;
	struct v4l2_format fmt;
};

#ifdef __KERNEL__

#define TVP514X_MAX_CHANNELS		2

#define TVP514X_MAX_VBI_SERVICES	2	/* This gives maximum
						   number of VBI service
						   supported */

enum tvp514x_mode {
	TVP514X_MODE_INV = -1,
	TVP514X_MODE_AUTO = 0,	/* autoswitch mode (default)   */
	TVP514X_MODE_NTSC = 1,	/* (M, J) NTSC      525-line   */
	TVP514X_MODE_PAL = 2,	/* (B, D, G, H, I, N) PAL      */
	TVP514X_MODE_PAL_M = 3,	/* (M) PAL          525-line   */
	TVP514X_MODE_PAL_CN = 4,	/* (Combination-N) PAL         */
	TVP514X_MODE_NTSC_443 = 5,	/* NTSC 4.43        525-line   */
	TVP514X_MODE_SECAM = 6,	/* SECAM                       */
	TVP514X_MODE_PAL_60 = 7,	/* PAL 60          525-line    */
	TVP514X_MODE_AUTO_SQP = 8,	/* autoswitch mode (default)   */
	TVP514X_MODE_NTSC_SQP = 9,	/* (M, J) NTSC      525-line   */
	TVP514X_MODE_PAL_SQP = 0xA,	/* (B, D, G, H, I, N) PAL      */
	TVP514X_MODE_PAL_M_SQP = 0xB,	/* (M) PAL          525-line   */
	TVP514X_MODE_PAL_CN_SQP = 0xC,	/* (Combination-N) PAL         */
	TVP514X_MODE_NTSC_443_SQP = 0xD,/* NTSC 4.43 525-line          */
	TVP514X_MODE_SECAM_SQP = 0xE,	/* SECAM                       */
	TVP514X_MODE_PAL_60_SQP = 0xF,	/* PAL 60          525-line    */
};

/* decoder standard related strctures */
#define TVP514X_MAX_NO_STANDARDS        (6)
#define TVP514X_MAX_NO_CONTROLS         (5)
/* for AUTO mode, stdinfo structure will not be filled */
#define TVP514X_STANDARD_INFO_SIZE      (TVP514X_MAX_NO_STANDARDS - 2)

#define TVP514X_MAX_NO_MODES		(3)
#define ISNULL(val) ((val == NULL) ? 1:0)

struct tvp514x_control_info {
	int register_address;
	struct v4l2_queryctrl query_control;
};

struct tvp514x_input {
	int input_type;
	u8 lock_mask;
	struct v4l2_input input_info;
	int no_of_standard;
	struct v4l2_standard *standard;
	v4l2_std_id def_std;
	 enum tvp514x_mode(*mode)[TVP514X_MAX_NO_MODES];
	int no_of_controls;
	struct tvp514x_control_info *controls;
};

struct tvp514x_config {
	int no_of_inputs;
	struct tvp514x_input *input;
	struct v4l2_sliced_vbi_cap sliced_cap;
	u8 num_services;
};

struct tvp514x_channel {
	struct {
		struct i2c_client client;
		struct i2c_driver driver;
		u32 i2c_addr;
		int i2c_registration;
	} i2c_dev;
	struct decoder_device *dec_device;
	struct tvp514x_params params;
};

struct tvp514x_service_data_reg {
	u32 service;
	struct {
		u8 addr[3];
	} field[2];
	u8 bytestoread;
};

struct tvp514x_sliced_reg {
	u32 service;
	u8 line_addr_value;
	u16 line_start, line_end;
	struct {
		u8 fifo_line_addr[3];
		u8 fifo_mode_value;
	} field[2];
	v4l2_std_id std;
	struct {
		u8 index;
		u16 value;
	} service_line[2];
};

/* Defines for TVP514X register address */

#define TVP514X_INPUT_SEL					(0x00)
#define TVP514X_AFE_GAIN_CTRL					(0x01)
#define TVP514X_VIDEO_STD					(0x02)
#define TVP514X_OPERATION_MODE					(0x03)
#define TVP514X_AUTOSWT_MASK					(0x04)
#define TVP514X_COLOR_KILLER					(0x05)
#define TVP514X_LUMA_CONTROL1					(0x06)
#define TVP514X_LUMA_CONTROL2					(0x07)
#define TVP514X_LUMA_CONTROL3					(0x08)
#define TVP514X_BRIGHTNESS					(0x09)
#define TVP514X_CONTRAST					(0x0A)
#define TVP514X_SATURATION					(0x0B)
#define TVP514X_HUE						(0x0C)
#define TVP514X_CHROMA_CONTROL1					(0x0D)
#define TVP514X_CHROMA_CONTROL2					(0x0E)
#define TVP514X_OUTPUT1						(0x33)
#define TVP514X_OUTPUT2						(0x34)
#define TVP514X_OUTPUT3						(0x35)
#define TVP514X_OUTPUT4						(0x36)
#define TVP514X_OUTPUT5						(0x37)
#define TVP514X_OUTPUT6						(0x38)
#define TVP514X_CLEAR_LOST_LOCK					(0x39)
#define TVP514X_STATUS1						(0x3A)
#define TVP514X_VID_STD_STATUS					(0x3F)
#define TVP514X_FIFO_OUTPUT_CTRL				(0xC0)

/* masks */

#define TVP514X_LOST_LOCK_MASK					(0x10)
/* mask to enable autoswitch for all standards*/

#define TVP514X_AUTOSWITCH_MASK					(0x7F)
#define TVP514X_COMPOSITE_INPUT					(0x05)
#define TVP514X_SVIDEO_INPUT					(0x46)

/* DEFAULTS */

#define TVP514X_OPERATION_MODE_RESET				(0x1)
#define TVP514X_OPERATION_MODE_DEFAULT				(0x0)
#define TVP514X_AFE_GAIN_CTRL_DEFAULT				(0x0F)
#define TVP514X_COLOR_KILLER_DEFAULT				(0x10)
#define TVP514X_LUMA_CONTROL1_DEFAULT				(0x10)
#define TVP514X_LUMA_CONTROL2_DEFAULT				(0x00)
#define TVP514X_LUMA_CONTROL3_DEFAULT				(0x02)
#define TVP514X_BRIGHTNESS_DEFAULT				(0x80)
#define TVP514X_CONTRAST_DEFAULT				(0x80)
#define TVP514X_SATURATION_DEFAULT				(0x80)
#define TVP514X_HUE_DEFAULT					(0x00)
#define TVP514X_CHROMA_CONTROL1_DEFAULT				(0x00)
#define TVP514X_CHROMA_CONTROL2_DEFAULT				(0x0E)
#define TVP514X_OUTPUT1_DEFAULT					(0x40)
#define TVP514X_OUTPUT2_DEFAULT					(0x11)
#define TVP514X_OUTPUT3_DEFAULT					(0xFF)
#define TVP514X_OUTPUT4_DEFAULT					(0xFF)
#define TVP514X_OUTPUT5_DEFAULT					(0xFF)
#define TVP514X_OUTPUT6_DEFAULT					(0xFF)
#define TVP514X_FIFO_OUTPUT_CTRL_DEFAULT			(0x01)

#define TVP514X_VBUS_ADDRESS_ACCESS0				(0xE8)
#define TVP514X_VBUS_ADDRESS_ACCESS1				(0xE9)
#define TVP514X_VBUS_ADDRESS_ACCESS2				(0xEA)
#define TVP514X_VBUS_DATA_ACCESS				(0xE0)
#define TVP514X_VBUS_DATA_ACCESS_AUTO_INCR			(0xE1)

#define TVP514X_LINE_ADDRESS_START				(0x80)
#define TVP514X_LINE_ADDRESS_MIDDLE				(0x06)
#define TVP514X_LINE_ADDRESS_END				(0x00)

#define TVP514X_LINE_ADDRESS_DEFAULT				(0x00)
#define TVP514X_LINE_MODE_DEFAULT				(0xFF)

#define TVP514X_VDP_LINE_START					(0xD6)
#define TVP514X_VDP_LINE_STOP					(0xD7)

#define TVP514X_VBI_NUM_SERVICES				(3)
#define TVP514X_SLICED_BUF_SIZE					(128)

#endif				/* __KERNEL__ */

#endif				/* TVP514X_H */
