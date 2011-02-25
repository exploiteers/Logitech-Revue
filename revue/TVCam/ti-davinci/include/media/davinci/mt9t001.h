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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _MT9T001_H
#define _MT9T001_H

/* Kernel Header files */
#ifdef __KERNEL__
#include <linux/videodev.h>
#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/device.h>
#include <media/davinci/vid_decoder_if.h>
#endif				/* __KERNEL__ */

#define V4L2_STD_MT9T001_VGA_30FPS	((v4l2_std_id)(0x0000001000000000ULL))
#define V4L2_STD_MT9T001_VGA_60FPS	((v4l2_std_id)(0x0000002000000000ULL))
#define V4L2_STD_MT9T001_SVGA_30FPS	((v4l2_std_id)(0x0000004000000000ULL))
#define V4L2_STD_MT9T001_SVGA_60FPS	((v4l2_std_id)(0x0000008000000000ULL))
#define V4L2_STD_MT9T001_XGA_30FPS	((v4l2_std_id)(0x0000010000000000ULL))
#define V4L2_STD_MT9T001_480p_30FPS	((v4l2_std_id)(0x0000020000000000ULL))
#define V4L2_STD_MT9T001_480p_60FPS	((v4l2_std_id)(0x0000040000000000ULL))
#define V4L2_STD_MT9T001_576p_25FPS	((v4l2_std_id)(0x0000080000000000ULL))
#define V4L2_STD_MT9T001_576p_50FPS	((v4l2_std_id)(0x0000100000000000ULL))
#define V4L2_STD_MT9T001_720p_24FPS	((v4l2_std_id)(0x0000200000000000ULL))
#define V4L2_STD_MT9T001_720p_30FPS	((v4l2_std_id)(0x0000400000000000ULL))
#define V4L2_STD_MT9T001_1080p_18FPS	((v4l2_std_id)(0x0000800000000000ULL))
#define V4L2_STD_MT9T001_SXGA_25FPS	((v4l2_std_id)(0x0001000000000000ULL))
#define V4L2_STD_MT9T001_UXGA_20FPS	((v4l2_std_id)(0x0002000000000000ULL))
#define V4L2_STD_MT9T001_QXGA_12FPS 	((v4l2_std_id)(0x0004000000000000ULL))

#define V4L2_STD_MT9T001_STD_ALL  (V4L2_STD_MT9T001_VGA_30FPS\
	| V4L2_STD_MT9T001_VGA_60FPS | V4L2_STD_MT9T001_SVGA_30FPS\
	| V4L2_STD_MT9T001_SVGA_60FPS | V4L2_STD_MT9T001_XGA_30FPS\
	| V4L2_STD_MT9T001_480p_30FPS | V4L2_STD_MT9T001_480p_60FPS\
	| V4L2_STD_MT9T001_576p_25FPS | V4L2_STD_MT9T001_576p_50FPS\
	| V4L2_STD_MT9T001_720p_24FPS | V4L2_STD_MT9T001_720p_30FPS\
	| V4L2_STD_MT9T001_1080p_18FPS | V4L2_STD_MT9T001_SXGA_25FPS\
	| V4L2_STD_MT9T001_UXGA_20FPS | V4L2_STD_MT9T001_QXGA_12FPS)

/* Structure containing video standard dependent settings */
struct mt9t001_format_params {
	unsigned short col_size;	/* width */
	unsigned short row_size;	/* Height */
	unsigned short h_blank;
	unsigned short v_blank;
	unsigned int shutter_width;
	unsigned short row_addr_mode;
	unsigned short col_addr_mode;
	unsigned short black_level;
	unsigned short pixel_clk_control;
	unsigned short row_start;
	unsigned short col_start;
};

/* Structure for gain settings */
struct mt9t001_rgb_gain {
	unsigned char green1_analog_gain;
	unsigned char red_analog_gain;
	unsigned char blue_analog_gain;
	unsigned char green2_analog_gain;
	unsigned char green1_digital_gain;
	unsigned char red_digital_gain;
	unsigned char blue_digital_gain;
	unsigned char green2_digital_gain;
};

/* Structure for black level calibration setttings*/
struct mt9t001_black_level_calibration {
	unsigned char manual_override;
	unsigned char disable_calibration;
	unsigned char recalculate_black_level;
	unsigned char lock_red_blue_calibration;
	unsigned char lock_green_calibration;
	unsigned char low_coarse_thrld;
	unsigned char high_coarse_thrld;
	unsigned char low_target_thrld;
	unsigned char high_target_thrld;
	unsigned short green1_offset;
	unsigned short green2_offset;
	unsigned short red_offset;
	unsigned short blue_offset;
};

/* Structure for MT9T001 configuration setttings passed by application*/
struct mt9t001_params {
	v4l2_std_id std;
	int inputidx;
	struct mt9t001_format_params format;
	struct mt9t001_rgb_gain rgb_gain;
	struct mt9t001_black_level_calibration black_calib;
};

#ifdef __KERNEL__

#define MT9T001_NUM_CHANNELS	1

#define GENERATE_MASK(bits, pos) ((((0xFFFFFFFF) << (32-bits)) \
		>> (32-bits)) << pos)

/* Definitions to access the various sensor registers */
#define MT9T001_CHIP_VERSION				(0x00)
#define MT9T001_ROW_START				(0x01)
#define MT9T001_COL_START				(0x02)
#define MT9T001_HEIGHT					(0x03)
#define MT9T001_WIDTH					(0x04)
#define MT9T001_HBLANK					(0x05)
#define MT9T001_VBLANK					(0x06)
#define MT9T001_OUTPUT_CTRL				(0x07)
#define MT9T001_SHUTTER_WIDTH_UPPER			(0x08)
#define MT9T001_SHUTTER_WIDTH				(0x09)
#define MT9T001_PIXEL_CLK_CTRL				(0x0A)
#define MT9T001_RESTART					(0x0B)
#define MT9T001_SHUTTER_DELAY				(0x0C)
#define MT9T001_RESET					(0x0D)
#define MT9T001_READ_MODE1				(0x1E)
#define MT9T001_READ_MODE2				(0x20)
#define MT9T001_READ_MODE3				(0x21)
#define MT9T001_ROW_ADDR_MODE				(0x22)
#define MT9T001_COL_ADDR_MODE				(0x23)
#define MT9T001_RESERVED_27_REG				(0x27)
#define MT9T001_GREEN1_GAIN				(0x2B)
#define MT9T001_BLUE_GAIN				(0x2C)
#define MT9T001_RED_GAIN				(0x2D)
#define MT9T001_GREEN2_GAIN				(0x2E)
#define MT9T001_GLOBAL_GAIN				(0x35)
#define MT9T001_BLACK_LEVEL				(0x49)
#define MT9T001_ROW_BLK_DEF_OFFSET			(0x4B)
#define MT9T001_RESERVED_4E_REG				(0x4e)
#define MT9T001_RESERVED_50_REG				(0x50)
#define MT9T001_RESERVED_51_REG				(0x51)
#define MT9T001_RESERVED_52_REG				(0x52)
#define MT9T001_RESERVED_53_REG				(0x53)
#define MT9T001_CAL_COARSE				(0x5D)
#define MT9T001_CAL_TARGET				(0x5F)
#define MT9T001_GREEN1_OFFSET				(0x60)
#define MT9T001_GREEN2_OFFSET				(0x61)
#define MT9T001_BLK_LVL_CALIB				(0x62)
#define MT9T001_RED_OFFSET				(0x63)
#define MT9T001_BLUE_OFFSET				(0x64)
#define MT9T001_CHIP_ENABLE_SYNC			(0xF8)
#define MT9T001_CHIP_VERSION_END			(0xFF)

/* Macros for default register values */
#define MT9T001_ROW_START_DEFAULT			(0x0014)
#define MT9T001_COLUMN_START_DEFAULT			(0x0020)
#define MT9T001_ROW_SIZE_DEFAULT			(0x05FF)
#define MT9T001_COLUMN_SIZE_DEFAULT			(0x07FF)
#define MT9T001_HOR_BLANK_DEFAULT			(0x008E)
#define MT9T001_VER_BLANK_DEFAULT			(0x0019)
#define MT9T001_OUTPUT_CTRL_DEFAULT			(0x0002)
#define MT9T001_SHUTTER_WIDTH_UPPER_DEFAULT		(0x0000)
#define MT9T001_SHUTTER_WIDTH_DEFAULT			(0x0619)
#define MT9T001_PIXEL_CLK_CTRL_DEFAULT			(0x0000)
#define MT9T001_FRAME_RESTART_DEFAULT			(0x0000)
#define MT9T001_SHUTTER_DELAY_DEFAULT			(0x0000)
#define MT9T001_READ_MODE1_DEFAULT			(0xC040)
#define MT9T001_READ_MODE2_DEFAULT			(0x0000)
#define MT9T001_READ_MODE3_DEFAULT			(0x0000)
#define MT9T001_ROW_ADDR_MODE_DEFAULT			(0x0000)
#define MT9T001_COL_ADDR_MODE_DEFAULT			(0x0000)
#define MT9T001_GREEN1_GAIN_DEFAULT			(0x0008)
#define MT9T001_BLUE_GAIN_DEFAULT			(0x0008)
#define MT9T001_RED_GAIN_DEFAULT			(0x0008)
#define MT9T001_GREEN2_GAIN_DEFAULT			(0x0008)
#define MT9T001_GLOBAL_GAIN_DEFAULT			(0x0008)
#define MT9T001_BLACK_LEVEL_DEFAULT			(0x00a8)
#define MT9T001_CAL_COARSE_DEFAULT			(0x2D13)
#define MT9T001_CAL_TARGET_DEFAULT			(0x2D13)
#define MT9T001_GREEN1_OFFSET_DEFAULT			(0x0020)
#define MT9T001_GREEN2_OFFSET_DEFAULT			(0x0020)
#define MT9T001_RED_OFFSET_DEFAULT			(0x0020)
#define MT9T001_BLUE_OFFSET_DEFAULT			(0x0020)
#define MT9T001_BLK_LVL_CALIB_DEFAULT			(0x0000)
#define MT9T001_CHIP_ENABLE_SYNC_DEFAULT		(0x0001)

/* Define Shift and Mask for gain register*/
#define MT9T001_ANALOG_GAIN_SHIFT			0x0000
#define MT9T001_DIGITAL_GAIN_SHIFT			8

#define MT9T001_ANALOG_GAIN_MASK			GENERATE_MASK(7, 0)
#define MT9T001_DIGITAL_GAIN_MASK			GENERATE_MASK(7, 0)

/* Define Shift and Mask for black level caliberation register*/
#define MT9T001_MANUAL_OVERRIDE_MASK		GENERATE_MASK(1, 0)
#define MT9T001_DISABLE_CALLIBERATION_SHIFT	1

#define MT9T001_DISABLE_CALLIBERATION_MASK	GENERATE_MASK(1, 0)
#define MT9T001_RECAL_BLACK_LEVEL_SHIFT		12

#define MT9T001_RECAL_BLACK_LEVEL_MASK		GENERATE_MASK(1, 0)
#define MT9T001_LOCK_RB_CALIBRATION_SHIFT	13

#define MT9T001_LOCK_RB_CALLIBERATION_MASK	GENERATE_MASK(1, 0)
#define MT9T001_LOCK_GREEN_CALIBRATION_SHIFT	14

#define MT9T001_LOCK_GREEN_CALLIBERATION_MASK	GENERATE_MASK(1, 0)

#define MT9T001_LOW_COARSE_THELD_MASK		GENERATE_MASK(7, 0)
#define MT9T001_HIGH_COARSE_THELD_SHIFT		8

#define MT9T001_HIGH_COARSE_THELD_MASK		GENERATE_MASK(7, 0)

#define MT9T001_LOW_TARGET_THELD_MASK		GENERATE_MASK(7, 0)
#define MT9T001_HIGH_TARGET_THELD_SHIFT		8

#define MT9T001_HIGH_TARGET_THELD_MASK		GENERATE_MASK(7, 8)

#define MT9T001_SHUTTER_WIDTH_LOWER_MASK	GENERATE_MASK(16, 0)

#define MT9T001_SHUTTER_WIDTH_UPPER_SHIFT	16

#define MT9T001_SHUTTER_WIDTH_UPPER_MASK	GENERATE_MASK(16, 0)

#define MT9T001_ROW_START_MASK			GENERATE_MASK(11, 0)

#define MT9T001_COL_START_MASK			GENERATE_MASK(12, 0)

#define		MT9T001_GREEN1_OFFSET_MASK		GENERATE_MASK(9, 0)

#define		MT9T001_GREEN2_OFFSET_MASK		GENERATE_MASK(9, 0)

#define		MT9T001_RED_OFFSET_MASK			GENERATE_MASK(9, 0)

#define		MT9T001_BLUE_OFFSET_MASK		GENERATE_MASK(9, 0)

#define MT9T001_N0_CHANGE_MODE			(0x0003)
#define MT9T001_NORMAL_OPERATION_MODE		(0x0002)
#define MT9T001_RESET_ENABLE			(0x0001)
#define MT9T001_RESET_DISABLE			(0x0000)

/* Pixel clock control register values */
#define MT9T001_PIX_CLK_RISING			(0x8000)
#define MT9T001_PIX_CLK_FALLING			(0x0000)

#define MT9T001_RAW_INPUT			(0x00)

/* decoder standard related strctures */
#define MT9T001_MAX_NO_INPUTS			(1)
#define MT9T001_MAX_NO_STANDARDS		(15)
#define MT9T001_MAX_NO_CONTROLS			(1)

#define MT9T001_STANDARD_INFO_SIZE		(MT9T001_MAX_NO_STANDARDS)

struct mt9t001_control_info {
	int register_address;
	struct v4l2_queryctrl query_control;
};

struct mt9t001_config {
	int no_of_inputs;
	struct {
		int input_type;
		struct v4l2_input input_info;
		int no_of_standard;
		struct v4l2_standard *standard;
		struct mt9t001_format_params *format;
		int no_of_controls;
		struct mt9t001_control_info *controls;
	} input[MT9T001_MAX_NO_INPUTS];
};

/*i2c adress for PCA9543A*/
#define PCA9543A_I2C_ADDR			(0x73)
#define PCA9543A_I2C_CONFIG			(0)
#define PCA9543A_REGVAL				(0x01)

#define MT9T001_I2C_CONFIG			(1)
#define ECP_I2C_CONFIG				(2)
	unsigned char manual_override;
	unsigned char disable_calibration;
	unsigned char recalculate_black_level;
	unsigned char lock_red_blue_calibration;
	unsigned char lock_green_calibration;
#define ECP_I2C_ADDR				(0x25)
#define ECP_REGADDR				(0x08)
#define ECP_REGVAL				(0x80)

/* MT9T001 Device structure */
struct mt9t001_channel {
	struct {
		struct i2c_client client;
		struct i2c_driver driver;
		u32 i2c_addr;
		int i2c_registration;
	} i2c_dev;
	struct decoder_device *dec_device;
	struct mt9t001_params params;
};

#endif				/* End of #ifdef __KERNEL__ */
#endif				/* End of #ifndef _MT9T001_H */
