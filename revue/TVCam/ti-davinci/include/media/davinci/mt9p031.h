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
 */

#ifndef MT9P031_H
#define MT9P031_H

#ifdef __KERNEL__
#include <linux/videodev.h>
#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/device.h>
#include <media/davinci/vid_decoder_if.h>
#endif

#define V4L2_STD_MT9P031_VGA_30FPS	((v4l2_std_id)(0x0000001000000000ULL))
#define V4L2_STD_MT9P031_VGA_60FPS	((v4l2_std_id)(0x0000002000000000ULL))
#define V4L2_STD_MT9P031_SVGA_60FPS	((v4l2_std_id)(0x0000004000000000ULL))
#define V4L2_STD_MT9P031_XGA_60FPS	((v4l2_std_id)(0x0000008000000000ULL))
#define V4L2_STD_MT9P031_SXGA_30FPS	((v4l2_std_id)(0x0000010000000000ULL))
#define V4L2_STD_MT9P031_UXGA_30FPS	((v4l2_std_id)(0x0000020000000000ULL))
#define V4L2_STD_MT9P031_QXGA_15FPS 	((v4l2_std_id)(0x0000040000000000ULL))
#define V4L2_STD_MT9P031_480p_30FPS	((v4l2_std_id)(0x0000080000000000ULL))
#define V4L2_STD_MT9P031_480p_60FPS	((v4l2_std_id)(0x0000100000000000ULL))
#define V4L2_STD_MT9P031_576p_25FPS	((v4l2_std_id)(0x0000200000000000ULL))
#define V4L2_STD_MT9P031_576p_50FPS	((v4l2_std_id)(0x0000400000000000ULL))
#define V4L2_STD_MT9P031_720p_60FPS	((v4l2_std_id)(0x0000800000000000ULL))
#define V4L2_STD_MT9P031_720p_50FPS	((v4l2_std_id)(0x0001000000000000ULL))
#define V4L2_STD_MT9P031_1080p_30FPS	((v4l2_std_id)(0x0002000000000000ULL))
#define V4L2_STD_MT9P031_1080p_25FPS	((v4l2_std_id)(0x0004000000000000ULL))
#define V4L2_STD_MT9P031_1944p_14FPS	((v4l2_std_id)(0x0008000000000000ULL))
#define V4L2_STD_MT9P031_720p_30FPS	((v4l2_std_id)(0x0010000000000000ULL))
#define V4L2_STD_MT9P031_960p_30FPS	((v4l2_std_id)(0x0020000000000000ULL))


#define V4L2_STD_MT9P031_STD_ALL  (V4L2_STD_MT9P031_VGA_60FPS\
	| V4L2_STD_MT9P031_VGA_30FPS | V4L2_STD_MT9P031_SVGA_60FPS\
	| V4L2_STD_MT9P031_XGA_60FPS | V4L2_STD_MT9P031_SXGA_30FPS\
	| V4L2_STD_MT9P031_UXGA_30FPS | V4L2_STD_MT9P031_QXGA_15FPS\
	| V4L2_STD_MT9P031_480p_30FPS | V4L2_STD_MT9P031_480p_60FPS\
	| V4L2_STD_MT9P031_576p_25FPS | V4L2_STD_MT9P031_576p_50FPS\
	| V4L2_STD_MT9P031_720p_60FPS | V4L2_STD_MT9P031_720p_50FPS\
	| V4L2_STD_MT9P031_1080p_30FPS | V4L2_STD_MT9P031_1080p_25FPS\
	| V4L2_STD_MT9P031_1944p_14FPS | V4L2_STD_MT9P031_720p_30FPS\
	| V4L2_STD_MT9P031_960p_30FPS)

/* Structure containing video standard dependent settings */
struct mt9p031_format_params {
	unsigned short col_size;
	unsigned short row_size;
	unsigned short h_blank;
	unsigned short v_blank;
	unsigned int shutter_width;
	unsigned short row_addr_mode;
	unsigned short col_addr_mode;
	unsigned short black_level;
	unsigned short pixel_clk_control;
	unsigned short row_start;
	unsigned short col_start;
	unsigned short read_mode2_config;
	unsigned short output_config;
	unsigned char pll_m;
	unsigned char pll_n;
	unsigned char pll_p1;
	unsigned char reserved;
};

/* Structure for gain settings */
struct mt9p031_rgb_gain {
	/* Green1 Analog gain */
	unsigned char gr1_analog_gain;
	/*
	 * Green1 Analog multiplier,
	 * 0 - no additional gain,
	 * 1 - 2X additional gain
	 */
	unsigned char gr1_analog_mult;
	/* Green1 digital gain */
	unsigned char gr1_digital_gain;
	/* Blue analog gain */
	unsigned char b_analog_gain;
	/*
	 * Blue analog multiplier.
	 * 0 - no additional gain,
	 * 1 - 2X additional gain
	 */
	unsigned char b_analog_mult;
	/* Blue digital gain */
	unsigned char b_digital_gain;
	/* Red analog gain */
	unsigned char r_analog_gain;
	/*
	 * Red analog multiplier
	 * 0 - no additional gain,
	 * 1 - 2X additional gain
	 */
	unsigned char r_analog_mult;
	/* Red digital gain */
	unsigned char r_digital_gain;
	/* Green2 analog gain */
	unsigned char gr2_analog_gain;
	/*
	 * Green1 Analog multiplier,
	 * 0 - no additional gain,
	 * 1 - 2X additional gain
	 */
	unsigned char gr2_analog_mult;
	/* Green1 digital gain */
	unsigned char gr2_digital_gain;
};

/* Test pattern modes */
enum mt9p031_tst_pat_mode {
	MT9P031_COLOR_FIELD,
	MT9P031_HORZ_GRAD,
	MT9P031_VERT_GRAD,
	MT9P031_DIAGONAL,
	MT9P031_CLASSIC,
	MT9P031_WALKING_ONES,
	MT9P031_MONO_HORZ_BARS,
	MT9P031_MONO_VERT_BARS
};

/* Test pattern generation */
struct mt9p031_test_pat {
	/*
	 * Enable test pattern input
	 * 0 - disable, 1 - enable
	 */
	unsigned char en;
	/* pattern mode */
	enum mt9p031_tst_pat_mode mode;
	/* pixel value for green, 0 - 4095 */
	unsigned short val_gr;
	/* pixel value for Red, 0 - 4095 */
	unsigned short val_r;
	/* pixel value for Blue, 0 - 4095 */
	unsigned short val_b;
	/*
	 * Bar width when mode is monochrom horizontal
	 * or vertical bars, 0 - 4095. Must be odd
	 */
	unsigned short bar_width;
};

struct mt9p031_misc_cfg {
	/* Enable mirror row. reverse the image horizontally */
	unsigned char mirror_row;
	/* Enable mirror col. reverse the image vertically */
	unsigned char mirror_col;
	/* show dark rows */
	unsigned char show_dk_rows;
	/* show dark columns */
	unsigned char show_dk_cols;
};

/* Structure for MT9P031 configuration setttings passed by application*/
struct mt9p031_params {
	v4l2_std_id std;
	int inputidx;
	struct mt9p031_format_params format;
	/* Set this for individual gains */
	struct mt9p031_rgb_gain rgb_gain;
	/* Test pattern */
	struct mt9p031_test_pat test_pat;
	/* Misc configuration */
	struct mt9p031_misc_cfg misc_cfg;
};

#ifdef __KERNEL__

#define MT9P031_NUM_CHANNELS                    1

/* Definitions to access the various sensor registers */
#define MT9P031_CHIP_VERSION			0x00
#define MT9P031_ROW_START			0x01
#define MT9P031_COL_START			0x02
#define MT9P031_HEIGHT				0x03
#define MT9P031_WIDTH				0x04
#define MT9P031_HBLANK				0x05
#define MT9P031_VBLANK				0x06
#define MT9P031_OUTPUT_CTRL			0x07
#define MT9P031_SHUTTER_WIDTH_UPPER		0x08
#define MT9P031_SHUTTER_WIDTH			0x09
#define MT9P031_PIXEL_CLK_CTRL			0x0A
#define MT9P031_RESTART				0x0B
#define MT9P031_SHUTTER_DELAY			0x0C
#define MT9P031_RESET				0x0D
#define MT9P031_PLL_CTRL			0x10
#define MT9P031_PLL_CFG1			0x11
#define MT9P031_PLL_CFG2			0x12
#define MT9P031_READ_MODE1			0x1E
#define MT9P031_READ_MODE2			0x20
#define MT9P031_ROW_ADDR_MODE			0x22
#define MT9P031_COL_ADDR_MODE			0x23
#define MT9P031_RESERVED_27_REG			0x27
#define MT9P031_GREEN1_GAIN			0x2B
#define MT9P031_BLUE_GAIN			0x2C
#define MT9P031_RED_GAIN			0x2D
#define MT9P031_GREEN2_GAIN			0x2E
#define MT9P031_GLOBAL_GAIN			0x35
#define MT9P031_ROW_BLK_TARGET			0x49
#define MT9P031_ROW_BLK_DEF_OFFSET		0x4B
#define MT9P031_RESERVED_4E_REG			0x4e
#define MT9P031_RESERVED_50_REG			0x50
#define MT9P031_RESERVED_51_REG			0x51
#define MT9P031_RESERVED_52_REG			0x52
#define MT9P031_RESERVED_53_REG			0x53
#define MT9P031_BLC_SAMPLE_SIZE			0x5B
#define MT9P031_BLC_TUNE_1			0x5C
#define MT9P031_BLC_DELTA_THRESHOLDS		0x5D
#define MT9P031_BLC_TUNE_2			0x5E
#define MT9P031_BLC_TARGET_THRESHOLDS		0x5F
#define MT9P031_GREEN1_OFFSET			0x60
#define MT9P031_GREEN2_OFFSET			0x61
#define MT9P031_BLK_LVL_CALIB			0x62
#define MT9P031_RED_OFFSET			0x63
#define MT9P031_BLUE_OFFSET			0x64
#define MT9P031_TST_PAT_CTRL			0xA0
#define MT9P031_TST_PAT_GR			0xA1
#define MT9P031_TST_PAT_R			0xA2
#define MT9P031_TST_PAT_B			0xA3
#define MT9P031_TST_PAT_BAR_WIDTH		0xA4
#define MT9P031_CHIP_VERSION_END		0xFF

/* Macros for default register values */
#define MT9P031_ROW_START_DEFAULT		0x0036
#define MT9P031_COLUMN_START_DEFAULT		0x0010
#define MT9P031_ROW_SIZE_DEFAULT		0x0797
#define MT9P031_COLUMN_SIZE_DEFAULT		0x0A1F
#define MT9P031_HOR_BLANK_DEFAULT		0x0000
#define MT9P031_VER_BLANK_DEFAULT		0x0019
#define MT9P031_OUTPUT_CTRL_DEFAULT		0x1F82
#define MT9P031_SHUTTER_WIDTH_UPPER_DEFAULT	0x0000
#define MT9P031_SHUTTER_WIDTH_DEFAULT		0x0797
#define MT9P031_PIXEL_CLK_CTRL_DEFAULT		0x0000
#define MT9P031_FRAME_RESTART_DEFAULT		0x0000
#define MT9P031_SHUTTER_DELAY_DEFAULT		0x0000
#define MT9P031_PLL_CTRL_DEFAULT		0x0050
#define MT9P031_PLL_CTRL_POWER_UP		0x0051
#define MT9P031_PLL_CTRL_USE_PLL_CLK		0x0053
#define MT9P031_PLL_CFG1_96MHZ			0x4001
#define MT9P031_PLL_CFG1_DEFAULT		0x1001
#define MT9P031_PLL_CFG2_DEFAULT		0x0003
#define MT9P031_PLL_CFG2_96MHZ			0x0007
#define MT9P031_READ_MODE1_DEFAULT		0x4006
#define MT9P031_READ_MODE2_DEFAULT		0x0040
#define MT9P031_ROW_ADDR_MODE_DEFAULT		0x0000
#define MT9P031_COL_ADDR_MODE_DEFAULT		0x0000
#define MT9P031_GREEN1_GAIN_DEFAULT		0x0008
#define MT9P031_BLUE_GAIN_DEFAULT		0x0008
#define MT9P031_RED_GAIN_DEFAULT		0x0008
#define MT9P031_GREEN2_GAIN_DEFAULT		0x0008
#define MT9P031_GLOBAL_GAIN_DEFAULT		0x0008
#define MT9P031_ROW_BLK_TARGET_DEFAULT		0x00A8
#define MT9P031_ROW_BLK_DEF_OFFSET_DEFAULT	0x0000
#define MT9P031_BLC_SAMPLE_SIZE_DEFAULT		0x0001
#define MT9P031_BLC_TUNE_1_DEFAULT		0x005A
#define MT9P031_BLC_DELTA_THRESHOLDS_DEFAULT	0x2D13
#define MT9P031_BLC_TUNE_2_DEFAULT		0x41FF
#define MT9P031_BLC_TARGET_THRESHOLDS_DEFAULT	0x231D
#define MT9P031_GREEN1_OFFSET_DEFAULT		0x0020
#define MT9P031_GREEN2_OFFSET_DEFAULT		0x0020
#define MT9P031_BLK_LVL_CALIB_DEFAULT		0x0000
#define MT9P031_RED_OFFSET_DEFAULT		0x0020
#define MT9P031_BLUE_OFFSET_DEFAULT		0x0020
#define MT9P031_PLL_CONFIG1_DEFAULT		0x4808
#define MT9P031_PLL_CONFIG2_DEFAULT		0x0003
#define MT9P031_PLL_DISABLE			0x0050
#define MT9P031_PLL_USE_EXTERNAL		0x0051
#define MT9P031_PLL_ENABLE			0x0053
#define MT9P031_TST_PAT_CTRL_DEFAULT		0x0000
#define MT9P031_TST_PAT_GR_DEFAULT		0x0000
#define MT9P031_TST_PAT_R_DEFAULT		0x0000
#define MT9P031_TST_PAT_B_DEFAULT		0x0000
#define MT9P031_TST_PAT_BAR_WIDTH_DEFAULT	0x0000

/* Define Shift and Mask for PLL register*/
#define	MT9P031_PLL_M_SHIFT			8
#define	MT9P031_PLL_N_MASK			0x003F
#define	MT9P031_PLL_P1_MASK			0x001F
#define	MT9P031_OUTPUT_CTRL_MASK 		0x1F8E
/* Define Shift and Mask for gain register*/
#define MT9P031_ANALOG_GAIN_SHIFT		0
#define MT9P031_ANALOG_MULT_SHIFT		6
#define MT9P031_DIGITAL_GAIN_SHIFT		8

#define MT9P031_ANALOG_GAIN_MASK		0x3F
#define MT9P031_ANALOG_MULT_MASK		0x1
#define MT9P031_DIGITAL_GAIN_MASK		0x3F

#define MT9P031_SHUTTER_WIDTH_LOWER_MASK	0xFFFF
#define MT9P031_SHUTTER_WIDTH_UPPER_SHIFT	16
#define MT9P031_ONE_BIT_MASK			0x1
#define MT9P031_TST_PAT_VAL_MASK		0xFFF
#define MT9P031_TST_PAT_BAR_WIDTH_MASK		0xFFF
#define MT9P031_N0_CHANGE_MODE			0x0003
#define MT9P031_NORMAL_OPERATION_MODE		0x0002
#define MT9P031_RESET_ENABLE			0x0001
#define MT9P031_RESET_DISABLE			0x0000
#define MT9P031_MIRROR_ROW_SHIFT		15
#define MT9P031_MIRROR_COL_SHIFT		14
#define MT9P031_SHOW_DK_COLS_SHIFT		12
#define MT9P031_SHOW_DK_ROWS_SHIFT		11
/* Pixel clock control register values */
#define MT9P031_PIX_CLK_RISING			0x8000
#define MT9P031_PIX_CLK_FALLING			0x0000

#define MT9P031_RAW_INPUT			0x00

/* decoder standard related strctures */
#define MT9P031_MAX_NO_INPUTS			1
#define MT9P031_MAX_NO_STANDARDS		11
#define MT9P031_MAX_NO_CONTROLS			2

#define MT9P031_STANDARD_INFO_SIZE	(MT9P031_MAX_NO_STANDARDS)

struct mt9p031_control_info {
	int register_address;
	struct v4l2_queryctrl query_control;
};

struct mt9p031_config {
	int no_of_inputs;
	struct {
		int input_type;
		struct v4l2_input input_info;
		int no_of_standard;
		struct v4l2_standard *standard;
		struct mt9p031_format_params *format;
		int no_of_controls;
		struct mt9p031_control_info *controls;
	} input[MT9P031_MAX_NO_INPUTS];
};

/* MT9P031 Device structure */
struct mt9p031_channel {
	struct {
		struct i2c_client client;
		struct i2c_driver driver;
		u32 i2c_addr;
		int i2c_registration;
	} i2c_dev;
	struct decoder_device *dec_device;
	struct mt9p031_params params;
};

#endif				/* End of #ifdef __KERNEL__ */
#endif				/* End of #ifndef MT9P031_H */
