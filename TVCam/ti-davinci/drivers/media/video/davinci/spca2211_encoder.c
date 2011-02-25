/*
 * Copyright (C) 2007 Texas Instruments Inc
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
/* spca2211_encoder.c. This is just a place holder for hardcoding all supported
   modes timing. spca2211 timing signals are programmed by the encoder manager
   based on this data.
 */

/* Kernel Specific header files */

#include <linux/kernel.h>
#include <linux/config.h>
#include <linux/init.h>
#include <linux/ctype.h>
#include <linux/delay.h>
#include <media/davinci/vid_encoder_if.h>
#include <media/davinci/spca2211_encoder.h>

/* Function prototypes */
static int spca2211_encoder_initialize(struct vid_encoder_device *enc, int flag);
static int spca2211_encoder_deinitialize(struct vid_encoder_device *enc);

static int spca2211_encoder_setmode(struct vid_enc_mode_info *mode_info,
				   struct vid_encoder_device *enc);
static int spca2211_encoder_getmode(struct vid_enc_mode_info *mode_info,
				   struct vid_encoder_device *enc);

static int spca2211_encoder_setoutput(char *output,
				     struct vid_encoder_device *enc);
static int spca2211_encoder_getoutput(char *output,
				     struct vid_encoder_device *enc);

static int spca2211_encoder_enumoutput(int index,
				      char *output,
				      struct vid_encoder_device *enc);

static struct spca2211_encoder_config spca2211_encoder_configuration = {
	.no_of_outputs = SPCA2211_ENCODER_MAX_NO_OUTPUTS,
	.output[0] = {
		      .output_name = VID_ENC_OUTPUT_SPCA2211,
		      .no_of_standard = SPCA2211_ENCODER_GRAPHICS_NUM_STD,
              .standards[0] = {
                       .name = VID_ENC_STD_HOLD,
                       .std = 1,
                       .if_type = VID_ENC_IF_HOLD8,
                       .interlaced = 0,
                       .xres = 1024 * 2,
                       .yres = 1,
                       .fps = {30, 1},
                       .left_margin = 40, // This is based on minimum BASEPX requirement of 20
                       .right_margin = 2, // HINTVL = xres + left_margin + right_margin - 1 = 1024 + 40 + 2 - 1 = 1065
                       .upper_margin = 1,
                       .lower_margin = 4, // VINTVL = yres + upper_margin + lower_margin - 1 = 6
                       .hsync_len = 10,
                       .vsync_len = 1,
                       .flags = 0},	/* hsync -ve, vsync -ve */
/**************************** 640 * 480 ***********************************/
              .standards[1] = {
                       .name = VID_ENC_STD_640x480_30,
                       .std = 1,
                       .if_type = VID_ENC_IF_YCC8,
                       .interlaced = 0,
                       .xres = 648, // 640 + fixup for 2211
                       .yres = 484, // 480 + fixup for 2211
                       .fps = {30, 1},
                       .left_margin = 210, // 210
                       .right_margin = 0,
                       .upper_margin = 41,
                       .lower_margin = 0,
                       .hsync_len = 211, // 211
                       .vsync_len = 41,
                       .flags = 0},	/* hsync -ve, vsync -ve */
             .standards[2] = {
                       .name = VID_ENC_STD_640x480_25,
                       .std = 1,
                       .if_type = VID_ENC_IF_YCC8,
                       .interlaced = 0,
                       .xres = 648, // 640 + fixup for 2211
                       .yres = 484, // 480 + fixup for 2211
                       .fps = {30, 1},
                       .left_margin = 240, // 210
                       .right_margin = 0,
                       .upper_margin = 61,
                       .lower_margin = 61,
                       .hsync_len = 211, // 211
                       .vsync_len = 41,
                       .flags = 0},	/* hsync -ve, vsync -ve */
             .standards[3] = {
                       .name = VID_ENC_STD_640x480_24,
                       .std = 1,
                       .if_type = VID_ENC_IF_YCC8,
                       .interlaced = 0,
                       .xres = 648, // 640 + fixup for 2211
                       .yres = 484, // 480 + fixup for 2211
                       .fps = {30, 1},
                       .left_margin = 210, // 210
                       .right_margin = 0,
                       .upper_margin = 41,
                       .lower_margin = 0,
                       .hsync_len = 211, // 211
                       .vsync_len = 41,
                       .flags = 0},	/* hsync -ve, vsync -ve */
             .standards[4] = {
                       .name = VID_ENC_STD_640x480_20,
                       .std = 1,
                       .if_type = VID_ENC_IF_YCC8,
                       .interlaced = 0,
                       .xres = 804, // 640 + fixup for 2211
                       .yres = 544, // 480 + fixup for 2211
                       .fps = {30, 1},
                       .left_margin = 210, // 210
                       .right_margin = 0,
                       .upper_margin = 61,
                       .lower_margin = 61,
                       .hsync_len = 511, // 211
                       .vsync_len = 41,
                       .flags = 0},	/* hsync -ve, vsync -ve */
             .standards[5] = {
                       .name = VID_ENC_STD_640x480_15,
                       .std = 1,
                       .if_type = VID_ENC_IF_YCC8,
                       .interlaced = 0,
                       .xres = 936, // 640 + fixup for 2211
                       .yres = 668, // 480 + fixup for 2211
                       .fps = {30, 1},
                       .left_margin = 210, // 210
                       .right_margin = 0,
                       .upper_margin = 61,
                       .lower_margin = 61,
                       .hsync_len = 511, // 211
                       .vsync_len = 41,
                       .flags = 0},	/* hsync -ve, vsync -ve */
             .standards[6] = {
                       .name = VID_ENC_STD_640x480_10,
                       .std = 1,
                       .if_type = VID_ENC_IF_YCC8,
                       .interlaced = 0,
                       .xres = 992, // 640 + fixup for 2211
                       .yres = 992, // 480 + fixup for 2211
                       .fps = {30, 1},
                       .left_margin = 210, // 210
                       .right_margin = 0,
                       .upper_margin = 61,
                       .lower_margin = 61,
                       .hsync_len = 511, // 211
                       .vsync_len = 41,
                       .flags = 0},	/* hsync -ve, vsync -ve */
             .standards[7] = {
                       .name = VID_ENC_STD_640x480_5,
                       .std = 1,
                       .if_type = VID_ENC_IF_YCC8,
                       .interlaced = 0,
                       .xres = 1680, // 640 + fixup for 2211
                       .yres = 1280, // 480 + fixup for 2211
                       .fps = {30, 1},
                       .left_margin = 210, // 210
                       .right_margin = 0,
                       .upper_margin = 61,
                       .lower_margin = 61,
                       .hsync_len = 511, // 211
                       .vsync_len = 41,
                       .flags = 0},	/* hsync -ve, vsync -ve */

/**************************** 160 *120 ***********************************/
              .standards[8] = {
                       .name = VID_ENC_STD_160x120_30,
                       .std = 1,
                       .if_type = VID_ENC_IF_YCC8,
                       .interlaced = 0,
                       .xres = 176 + 480,
                       .yres = 124,
                       .fps = {30, 1},
                       .left_margin = 210, // 210
                       .right_margin = 0,
                       .upper_margin = 41,
                       .lower_margin = 360,
                       .hsync_len = 211, // 211
                       .vsync_len = 41,
                       .flags = 0},	/* hsync -ve, vsync -ve */
              .standards[9] = {
                       .name = VID_ENC_STD_160x120_25,
                       .std = 1,
                       .if_type = VID_ENC_IF_YCC8,
                       .interlaced = 0,
                       .xres = 276 + 480,
                       .yres = 160,
                       .fps = {30, 1},
                       .left_margin = 210, // 210
                       .right_margin = 0,
                       .upper_margin = 41,
                       .lower_margin = 360,
                       .hsync_len = 211, // 211
                       .vsync_len = 41,
                       .flags = 0},	/* hsync -ve, vsync -ve */
              .standards[10] = {
                       .name = VID_ENC_STD_160x120_24,
                       .std = 1,
                       .if_type = VID_ENC_IF_YCC8,
                       .interlaced = 0,
                       .xres = 176 + 480,
                       .yres = 124,
                       .fps = {30, 1},
                       .left_margin = 210, // 210
                       .right_margin = 0,
                       .upper_margin = 41,
                       .lower_margin = 360,
                       .hsync_len = 211, // 211
                       .vsync_len = 41,
                       .flags = 0},	/* hsync -ve, vsync -ve */
              .standards[11] = {
                       .name = VID_ENC_STD_160x120_20,
                       .std = 1,
                       .if_type = VID_ENC_IF_YCC8,
                       .interlaced = 0,
                       .xres = 396 + 480,
                       .yres = 218,
                       .fps = {30, 1},
                       .left_margin = 210, // 210
                       .right_margin = 0,
                       .upper_margin = 41,
                       .lower_margin = 360,
                       .hsync_len = 211, // 211
                       .vsync_len = 41,
                       .flags = 0},	/* hsync -ve, vsync -ve */
              .standards[12] = {
                       .name = VID_ENC_STD_160x120_15,
                       .std = 1,
                       .if_type = VID_ENC_IF_YCC8,
                       .interlaced = 0,
                       .xres = 592 + 480,
                       .yres = 312,
                       .fps = {30, 1},
                       .left_margin = 210, // 210
                       .right_margin = 0,
                       .upper_margin = 41,
                       .lower_margin = 360,
                       .hsync_len = 211, // 211
                       .vsync_len = 41,
                       .flags = 0},	/* hsync -ve, vsync -ve */
              .standards[13] = {
                       .name = VID_ENC_STD_160x120_10,
                       .std = 1,
                       .if_type = VID_ENC_IF_YCC8,
                       .interlaced = 0,
                       .xres = 896 + 480,
                       .yres = 442,
                       .fps = {30, 1},
                       .left_margin = 210, // 210
                       .right_margin = 0,
                       .upper_margin = 41,
                       .lower_margin = 360,
                       .hsync_len = 211, // 211
                       .vsync_len = 41,
                       .flags = 0},	/* hsync -ve, vsync -ve */
              .standards[14] = {
                       .name = VID_ENC_STD_160x120_5,
                       .std = 1,
                       .if_type = VID_ENC_IF_YCC8,
                       .interlaced = 0,
                       .xres = 1296 + 480,
                       .yres = 942,
                       .fps = {30, 1},
                       .left_margin = 210, // 210
                       .right_margin = 0,
                       .upper_margin = 41,
                       .lower_margin = 360,
                       .hsync_len = 211, // 211
                       .vsync_len = 41,
                       .flags = 0},	/* hsync -ve, vsync -ve */

/**************************** 320 *176 ***********************************/
              .standards[15] = {
                       .name = VID_ENC_STD_320x176_30,
                       .std = 1,
                       .if_type = VID_ENC_IF_YCC8,
                       .interlaced = 0,
                       .xres = 336 + 328,
                       .yres = 180,
                       .fps = {30, 1},
                       .left_margin = 210, // 210
                       .right_margin = 0,//328,
                       .upper_margin = 41,
                       .lower_margin = 308,
                       .hsync_len = 211, // 211
                       .vsync_len = 41,
                       .flags = 0},	/* hsync -ve, vsync -ve */
              .standards[16] = {
                       .name = VID_ENC_STD_320x176_25,
                       .std = 1,
                       .if_type = VID_ENC_IF_YCC8,
                       .interlaced = 0,
                       .xres = 416 + 328,
                       .yres = 218,
                       .fps = {30, 1},
                       .left_margin = 210, // 210
                       .right_margin = 0,
                       .upper_margin = 41,
                       .lower_margin = 308,
                       .hsync_len = 211, // 211
                       .vsync_len = 41,
                       .flags = 0},	/* hsync -ve, vsync -ve */
              .standards[17] = {
                       .name = VID_ENC_STD_320x176_24,
                       .std = 1,
                       .if_type = VID_ENC_IF_YCC8,
                       .interlaced = 0,
                       .xres = 416 + 328,
                       .yres = 240,
                       .fps = {30, 1},
                       .left_margin = 210, // 210
                       .right_margin = 0,
                       .upper_margin = 41,
                       .lower_margin = 308,
                       .hsync_len = 211, // 211
                       .vsync_len = 41,
                       .flags = 0},	/* hsync -ve, vsync -ve */
              .standards[18] = {
                       .name = VID_ENC_STD_320x176_20,
                       .std = 1,
                       .if_type = VID_ENC_IF_YCC8,
                       .interlaced = 0,
                       .xres = 536 + 328,
                       .yres = 280,
                       .fps = {30, 1},
                       .left_margin = 210, // 210
                       .right_margin = 0,
                       .upper_margin = 41,
                       .lower_margin = 308,
                       .hsync_len = 211, // 211
                       .vsync_len = 41,
                       .flags = 0},	/* hsync -ve, vsync -ve */
              .standards[19] = {
                       .name = VID_ENC_STD_320x176_15,
                       .std = 1,
                       .if_type = VID_ENC_IF_YCC8,
                       .interlaced = 0,
                       .xres = 724 + 328,
                       .yres = 380,
                       .fps = {30, 1},
                       .left_margin = 210, // 210
                       .right_margin = 0,
                       .upper_margin = 41,
                       .lower_margin = 308,
                       .hsync_len = 211, // 211
                       .vsync_len = 41,
                       .flags = 0},	/* hsync -ve, vsync -ve */
              .standards[20] = {
                       .name = VID_ENC_STD_320x176_10,
                       .std = 1,
                       .if_type = VID_ENC_IF_YCC8,
                       .interlaced = 0,
                       .xres = 924 + 328,
                       .yres = 580,
                       .fps = {30, 1},
                       .left_margin = 210, // 210
                       .right_margin = 0,
                       .upper_margin = 41,
                       .lower_margin = 308,
                       .hsync_len = 211, // 211
                       .vsync_len = 41,
                       .flags = 0},	/* hsync -ve, vsync -ve */
              .standards[21] = {
                       .name = VID_ENC_STD_320x176_5,
                       .std = 1,
                       .if_type = VID_ENC_IF_YCC8,
                       .interlaced = 0,
                       .xres = 1486 + 328,
                       .yres = 992,
                       .fps = {30, 1},
                       .left_margin = 210, // 210
                       .right_margin = 0,
                       .upper_margin = 41,
                       .lower_margin = 308,
                       .hsync_len = 211, // 211
                       .vsync_len = 41,
                       .flags = 0},	/* hsync -ve, vsync -ve */
  
/**************************** 320 *240 ***********************************/
              .standards[22] = {
                       .name = VID_ENC_STD_320x240_30,
                       .std = 1,
                       .if_type = VID_ENC_IF_YCC8,
                       .interlaced = 0,
                       .xres = 336 + 328,
                       .yres = 244,
                       .fps = {30, 1},
                       .left_margin = 210, // 210
                       .right_margin = 0,//328,
                       .upper_margin = 41,
                       .lower_margin = 244,
                       .hsync_len = 211, // 211
                       .vsync_len = 41,
                       .flags = 0},	/* hsync -ve, vsync -ve */
             .standards[23] = {
                       .name = VID_ENC_STD_320x240_25,
                       .std = 1,
                       .if_type = VID_ENC_IF_YCC8,
                       .interlaced = 0,
                       .xres = 482 + 328,
                       .yres = 244,
                       .fps = {30, 1},
                       .left_margin = 210, // 210
                       .right_margin = 0,//328,
                       .upper_margin = 41,
                       .lower_margin = 244,
                       .hsync_len = 211, // 211
                       .vsync_len = 41,
                       .flags = 0},	/* hsync -ve, vsync -ve */

             .standards[24] = {
                       .name = VID_ENC_STD_320x240_24,
                       .std = 1,
                       .if_type = VID_ENC_IF_YCC8,
                       .interlaced = 0,
                       .xres = 476 + 328,
                       .yres = 244,
                       .fps = {30, 1},
                       .left_margin = 210, // 210
                       .right_margin = 0,//328,
                       .upper_margin = 41,
                       .lower_margin = 244,
                       .hsync_len = 211, // 211
                       .vsync_len = 41,
                       .flags = 0},	/* hsync -ve, vsync -ve */

             .standards[25] = {
                       .name = VID_ENC_STD_320x240_20,
                       .std = 1,
                       .if_type = VID_ENC_IF_YCC8,
                       .interlaced = 0,
                       .xres = 686 + 328,
                       .yres = 268,
                       .fps = {30, 1},
                       .left_margin = 210, // 210
                       .right_margin = 0,//328,
                       .upper_margin = 41,
                       .lower_margin = 244,
                       .hsync_len = 211, // 211
                       .vsync_len = 41,
                       .flags = 0},	/* hsync -ve, vsync -ve */

             .standards[26] = {
                       .name = VID_ENC_STD_320x240_15,
                       .std = 1,
                       .if_type = VID_ENC_IF_YCC8,
                       .interlaced = 0,
                       .xres = 686 + 328,
                       .yres = 454,
                       .fps = {30, 1},
                       .left_margin = 210, // 210
                       .right_margin = 0,//328,
                       .upper_margin = 41,
                       .lower_margin = 244,
                       .hsync_len = 211, // 211 
                       .vsync_len = 41,
                       .flags = 0},	/* hsync -ve, vsync -ve */

             .standards[27] = {
                       .name = VID_ENC_STD_320x240_10,
                       .std = 1,
                       .if_type = VID_ENC_IF_YCC8,
                       .interlaced = 0,
                       .xres = 998 + 328,
                       .yres = 586,
                       .fps = {30, 1},
                       .left_margin = 210, // 210
                       .right_margin = 0,//328,
                       .upper_margin = 41,
                       .lower_margin = 244,
                       .hsync_len = 211, // 211
                       .vsync_len = 41,
                       .flags = 0},	/* hsync -ve, vsync -ve */

             .standards[28] = {
                       .name = VID_ENC_STD_320x240_5,
                       .std = 1,
                       .if_type = VID_ENC_IF_YCC8,
                       .interlaced = 0,
                       .xres = 1480 + 328,
                       .yres = 1076,
                       .fps = {30, 1},
                       .left_margin = 210, // 210
                       .right_margin = 0,//328,
                       .upper_margin = 41,
                       .lower_margin = 244,
                       .hsync_len = 211, // 211
                       .vsync_len = 41,
                       .flags = 0},	/* hsync -ve, vsync -ve */

/**************************** 352 *288 ***********************************/
              .standards[29] = {
                       .name = VID_ENC_STD_352x288_30,
                       .std = 1,
                       .if_type = VID_ENC_IF_YCC8,
                       .interlaced = 0,
                       .xres = 368 + 276,
                       .yres = 292,
                       .fps = {30, 1},
                       .left_margin = 210, // 210
                       .right_margin = 0,//296,
                       .upper_margin = 41,
                       .lower_margin = 196,
                       .hsync_len = 211, // 211
                       .vsync_len = 41,
                       .flags = 0},	/* hsync -ve, vsync -ve */
            .standards[30] = {
                       .name = VID_ENC_STD_352x288_25,
                       .std = 1,
                       .if_type = VID_ENC_IF_YCC8,
                       .interlaced = 0,
                       .xres = 516 + 296,
                       .yres = 292,
                       .fps = {30, 1},
                       .left_margin = 210, // 210
                       .right_margin = 0,
                       .upper_margin = 41,
                       .lower_margin = 196,
                       .hsync_len = 211, // 211
                       .vsync_len = 41,
                       .flags = 0},	/* hsync -ve, vsync -ve */
            .standards[31] = {
                       .name = VID_ENC_STD_352x288_24,
                       .std = 1,
                       .if_type = VID_ENC_IF_YCC8,
                       .interlaced = 0,
                       .xres = 368 + 296,
                       .yres = 292,
                       .fps = {30, 1},
                       .left_margin = 210, // 210
                       .right_margin = 0,
                       .upper_margin = 41,
                       .lower_margin = 196,
                       .hsync_len = 211, // 211
                       .vsync_len = 41,
                       .flags = 0},	/* hsync -ve, vsync -ve */
            .standards[32] = {
                       .name = VID_ENC_STD_352x288_20,
                       .std = 1,
                       .if_type = VID_ENC_IF_YCC8,
                       .interlaced = 0,
                       .xres = 568 + 296,
                       .yres = 392,
                       .fps = {30, 1},
                       .left_margin = 210, // 210
                       .right_margin = 0,
                       .upper_margin = 41,
                       .lower_margin = 196,
                       .hsync_len = 211, // 211
                       .vsync_len = 41,
                       .flags = 0},	/* hsync -ve, vsync -ve */
            .standards[33] = {
                       .name = VID_ENC_STD_352x288_15,
                       .std = 1,
                       .if_type = VID_ENC_IF_YCC8,
                       .interlaced = 0,
                       .xres = 768 + 296,
                       .yres = 472,
                       .fps = {30, 1},
                       .left_margin = 210, // 210
                       .right_margin = 0,
                       .upper_margin = 41,
                       .lower_margin = 196,
                       .hsync_len = 211, // 211
                       .vsync_len = 41,
                       .flags = 0},	/* hsync -ve, vsync -ve */
            .standards[34] = {
                       .name = VID_ENC_STD_352x288_10,
                       .std = 1,
                       .if_type = VID_ENC_IF_YCC8,
                       .interlaced = 0,
                       .xres = 978 + 296,
                       .yres = 672,
                       .fps = {30, 1},
                       .left_margin = 210, // 210
                       .right_margin = 0,
                       .upper_margin = 41,
                       .lower_margin = 196,
                       .hsync_len = 211, // 211
                       .vsync_len = 41,
                       .flags = 0},	/* hsync -ve, vsync -ve */
            .standards[35] = {
                       .name = VID_ENC_STD_352x288_5,
                       .std = 1,
                       .if_type = VID_ENC_IF_YCC8,
                       .interlaced = 0,
                       .xres = 1494 + 296,
                       .yres = 1092,
                       .fps = {30, 1},
                       .left_margin = 210, // 210
                       .right_margin = 0,
                       .upper_margin = 41,
                       .lower_margin = 196,
                       .hsync_len = 211, // 211
                       .vsync_len = 41,
                       .flags = 0},	/* hsync -ve, vsync -ve */

/**************************** 640 *360 ***********************************/
              .standards[36] = {
                       .name = VID_ENC_STD_640x360_30,
                       .std = 1,
                       .if_type = VID_ENC_IF_YCC8,
                       .interlaced = 0,
                       .xres = 648, // 640 + fixup for 2211
                       .yres = 364, // 360 + fixup for 2211
                       .fps = {30, 1},
                       .left_margin = 210, // 210
                       .right_margin = 0,
                       .upper_margin = 41,
                       .lower_margin = 124,
                       .hsync_len = 211, // 211
                       .vsync_len = 41,
                       .flags = 0},	/* hsync -ve, vsync -ve */
             .standards[37] = {
                       .name = VID_ENC_STD_640x360_25,
                       .std = 1,
                       .if_type = VID_ENC_IF_YCC8,
                       .interlaced = 0,
                       .xres = 816, // 640 + fixup for 2211
                       .yres = 364, // 360 + fixup for 2211
                       .fps = {30, 1},
                       .left_margin = 210, // 210
                       .right_margin = 0,
                       .upper_margin = 41,
                       .lower_margin = 124,
                       .hsync_len = 211, // 211
                       .vsync_len = 41,
                       .flags = 0},	/* hsync -ve, vsync -ve */
             .standards[38] = {
                       .name = VID_ENC_STD_640x360_24,
                       .std = 1,
                       .if_type = VID_ENC_IF_YCC8,
                       .interlaced = 0,
                       .xres = 848, // 640 + fixup for 2211
                       .yres = 364, // 360 + fixup for 2211
                       .fps = {30, 1},
                       .left_margin = 210, // 210
                       .right_margin = 0,
                       .upper_margin = 41,
                       .lower_margin = 124,
                       .hsync_len = 211, // 211
                       .vsync_len = 41,
                       .flags = 0},	/* hsync -ve, vsync -ve */
             .standards[39] = {
                       .name = VID_ENC_STD_640x360_20,
                       .std = 1,
                       .if_type = VID_ENC_IF_YCC8,
                       .interlaced = 0,
                       .xres = 848, // 640 + fixup for 2211
                       .yres = 472, // 360 + fixup for 2211
                       .fps = {30, 1},
                       .left_margin = 210, // 210
                       .right_margin = 0,
                       .upper_margin = 41,
                       .lower_margin = 124,
                       .hsync_len = 211, // 211
                       .vsync_len = 41,
                       .flags = 0},	/* hsync -ve, vsync -ve */
             .standards[40] = {
                       .name = VID_ENC_STD_640x360_15,
                       .std = 1,
                       .if_type = VID_ENC_IF_YCC8,
                       .interlaced = 0,
                       .xres = 848, // 640 + fixup for 2211
                       .yres = 684, // 360 + fixup for 2211
                       .fps = {30, 1},
                       .left_margin = 210, // 210
                       .right_margin = 0,
                       .upper_margin = 41,
                       .lower_margin = 124,
                       .hsync_len = 211, // 211
                       .vsync_len = 41,
                       .flags = 0},	/* hsync -ve, vsync -ve */
             .standards[41] = {
                       .name = VID_ENC_STD_640x360_10,
                       .std = 1,
                       .if_type = VID_ENC_IF_YCC8,
                       .interlaced = 0,
                       .xres = 1048, // 640 + fixup for 2211
                       .yres = 902, // 360 + fixup for 2211
                       .fps = {30, 1},
                       .left_margin = 210, // 210
                       .right_margin = 0,
                       .upper_margin = 41,
                       .lower_margin = 124,
                       .hsync_len = 211, // 211
                       .vsync_len = 41,
                       .flags = 0},	/* hsync -ve, vsync -ve */
             .standards[42] = {
                       .name = VID_ENC_STD_640x360_5,
                       .std = 1,
                       .if_type = VID_ENC_IF_YCC8,
                       .interlaced = 0,
                       .xres = 1612, // 640 + fixup for 2211
                       .yres = 1340, // 360 + fixup for 2211
                       .fps = {30, 1},
                       .left_margin = 210, // 210
                       .right_margin = 0,
                       .upper_margin = 41,
                       .lower_margin = 124,
                       .hsync_len = 211, // 211
                       .vsync_len = 41,
                       .flags = 0},	/* hsync -ve, vsync -ve */

/**************************** 1280 *720 ***********************************/
		      .standards[43] = {
				       .name = VID_ENC_STD_1280x480_30,
				       .std = 1,
				       .if_type = VID_ENC_IF_YCC8,
				       .interlaced = 0,
				       //.xres = 1288, // 1280 + fixup for 2211
				       //.yres = 724, // 720 + fixup for 2211
                       .xres = 1288 , // 1280 + fixup for 2211
                       .yres = 484, // 720 + fixup for 2211
				       .fps = {2, 1},
				       .left_margin = 425, // 210
				       .right_margin = 0,
				       .upper_margin = 41,
				       .lower_margin = 0,
				       .hsync_len = 211, // 211
				       .vsync_len = 41,
				       .flags = 0},	/* hsync -ve, vsync -ve */
	      .standards[44] = {
				       .name = VID_ENC_STD_1280x480_25,
				       .std = 1,
				       .if_type = VID_ENC_IF_YCC8,
				       .interlaced = 0,
				       //.xres = 1288, // 1280 + fixup for 2211
				       //.yres = 724, // 720 + fixup for 2211
                       .xres = 1288 + 420, // 1280 + fixup for 2211
                       .yres = 484, // 720 + fixup for 2211
				       .fps = {2, 1},
				       .left_margin = 70*5, // 210
				       .right_margin = 0,
				       .upper_margin = 41,
				       .lower_margin = 0,
				       .hsync_len = 211, // 211
				       .vsync_len = 41,
				       .flags = 0},	/* hsync -ve, vsync -ve */
	      .standards[45] = {
				       .name = VID_ENC_STD_1280x480_24,
				       .std = 1,
				       .if_type = VID_ENC_IF_YCC8,
				       .interlaced = 0,
				       //.xres = 1288, // 1280 + fixup for 2211
				       //.yres = 724, // 720 + fixup for 2211
                       .xres = 1288 + 428, // 1280 + fixup for 2211
                       .yres = 484, // 720 + fixup for 2211
				       .fps = {2, 1},
				       .left_margin = 420, // 210
				       .right_margin = 0,
				       .upper_margin = 41,
				       .lower_margin = 0,
				       .hsync_len = 211, // 211
				       .vsync_len = 41,
				       .flags = 0},	/* hsync -ve, vsync -ve */
	      .standards[46] = {
				       .name = VID_ENC_STD_1280x480_20,
				       .std = 1,
				       .if_type = VID_ENC_IF_YCC8,
				       .interlaced = 0,
				       //.xres = 1288, // 1280 + fixup for 2211
				       //.yres = 724, // 720 + fixup for 2211
                       .xres = 1288 + 385, // 1280 + fixup for 2211
                       .yres = 484, // 720 + fixup for 2211
				       .fps = {2, 1},
				       .left_margin = 900, // 210
				       .right_margin = 0,
				       .upper_margin = 41,
				       .lower_margin = 0,
				       .hsync_len = 211, // 211
				       .vsync_len = 41,
				       .flags = 0},	/* hsync -ve, vsync -ve */
	      .standards[47] = {
				       .name = VID_ENC_STD_1280x480_15,
				       .std = 1,
				       .if_type = VID_ENC_IF_YCC8,
				       .interlaced = 0,
				       //.xres = 1288, // 1280 + fixup for 2211
				       //.yres = 724, // 720 + fixup for 2211
                       .xres = 1288, // 1280 + fixup for 2211
                       .yres = 484, // 720 + fixup for 2211
				       .fps = {2, 1},
				       .left_margin = 400,//540, // 210
				       .right_margin = 0,
				       .upper_margin = 41,
				       .lower_margin = 0,
				       .hsync_len = 211, // 211
				       .vsync_len = 41,
				       .flags = 0},	/* hsync -ve, vsync -ve */
	      .standards[48] = {
				       .name = VID_ENC_STD_1280x720_10,
				       .std = 1,
				       .if_type = VID_ENC_IF_YCC8,
				       .interlaced = 0,
				       //.xres = 1288, // 1280 + fixup for 2211
				       //.yres = 724, // 720 + fixup for 2211
                       .xres = 1492, // 1280 + fixup for 2211
                       .yres = 742, // 720 + fixup for 2211
				       .fps = {2, 1},
				       .left_margin = 210, // 210
				       .right_margin = 0,
				       .upper_margin = 41,
				       .lower_margin = 0,
				       .hsync_len = 211, // 211
				       .vsync_len = 41,
				       .flags = 0},	/* hsync -ve, vsync -ve */
	      .standards[49] = {
				       .name = VID_ENC_STD_1280x720_5,
				       .std = 1,
				       .if_type = VID_ENC_IF_YCC8,
				       .interlaced = 0,
				       //.xres = 1288, // 1280 + fixup for 2211
				       //.yres = 724, // 720 + fixup for 2211
                       .xres = 1758,//1792, // 1280 + fixup for 2211
                       .yres = 1312, // 720 + fixup for 2211
				       .fps = {2, 1},
				       .left_margin = 210, // 210
				       .right_margin = 0,
				       .upper_margin = 41,
				       .lower_margin = 0,
				       .hsync_len = 211, // 211
				       .vsync_len = 41,
				       .flags = 0},	/* hsync -ve, vsync -ve */
/************************************************************************/

              .standards[50] = {
                       .name = VID_ENC_STD_800x600,
                       .std = 1,
                       .if_type = VID_ENC_IF_YCC8,
                       .interlaced = 0,
                       .xres = 808,
                       .yres = 604,
                       .fps = {30, 1},
                       .left_margin = 210, // 210
                       .right_margin = 0,
                       .upper_margin = 41,
                       .lower_margin = 0,
                       .hsync_len = 211, // 211
                       .vsync_len = 41,
                       .flags = 0},	/* hsync -ve, vsync -ve */
              .standards[51] = {
                       .name = VID_ENC_STD_960x720,
                       .std = 1,
                       .if_type = VID_ENC_IF_YCC8,
                       .interlaced = 0,
                       .xres = 968, // 1280 + fixup for 2211
                       .yres = 724, // 720 + fixup for 2211
                       .fps = {2, 1},
                       .left_margin = 210, // 210
                       .right_margin = 0,
                       .upper_margin = 41,
                       .lower_margin = 0,
                       .hsync_len = 211, // 211
                       .vsync_len = 41,
                       .flags = 0},	/* hsync -ve, vsync -ve */

		      .standards[52] = {
				       .name = VID_ENC_STD_640x400,
				       .std = 1,
				       .if_type = VID_ENC_IF_YCC8,
				       .interlaced = 0,
				       .xres = 648, // 640 + fixup for 2211
				       .yres = 404, // 400 + fixup for 2211
				       .fps = {30, 1},
                       .left_margin = 210, // 210
                       .right_margin = 0,
                       .upper_margin = 41,
                       .lower_margin = 0,
                       .hsync_len = 211, // 211
                       .vsync_len = 41,
                       .flags = 0},	/* hsync -ve, vsync -ve */
		      .standards[53] = {
				       .name = VID_ENC_STD_640x350,
				       .std = 1,
				       .if_type = VID_ENC_IF_YCC8,
				       .interlaced = 0,
                       .xres = 648, // 640 + fixup for 2211
                       .yres = 354, // 350 + fixup for 2211
				       .fps = {30, 1},
                       .left_margin = 210, // 210
                       .right_margin = 0,
                       .upper_margin = 41,
                       .lower_margin = 0,
                       .hsync_len = 211, // 211
                       .vsync_len = 41,
				       .flags = 0},	/* hsync -ve, vsync -ve */
		      .standards[54] = {	/* This is programmed by the user application. We just save
					   the received timing information */
				       .name = VID_ENC_STD_NON_STANDARD,
				       .std = 0,
				       .if_type = VID_ENC_IF_YCC8,
				       .interlaced = 0,
				       .xres = 0,
				       .yres = 0,
				       .fps = {0, 0},
				       .left_margin = 0,
				       .right_margin = 0,
				       .upper_margin = 0,
				       .lower_margin = 0,
				       .hsync_len = 0,
				       .vsync_len = 0,
				       .flags = 0},
		      },
};


static struct spca2211_encoder_channel spca2211_encoder_channel_info = {
	.params.outindex = 0,
	.params.mode = VID_ENC_STD_640x480,
	.enc_device = NULL
};

static struct vid_enc_output_ops outputs_ops = {
	.count = SPCA2211_ENCODER_MAX_NO_OUTPUTS,
	.enumoutput = spca2211_encoder_enumoutput,
	.setoutput = spca2211_encoder_setoutput,
	.getoutput = spca2211_encoder_getoutput
};

static struct vid_enc_mode_ops modes_ops = {
	.setmode = spca2211_encoder_setmode,
	.getmode = spca2211_encoder_getmode,
};

static struct vid_encoder_device spca2211_encoder_dev = {
	.name = "SPCA2211_ENCODER",
	.capabilities = 0,
	.initialize = spca2211_encoder_initialize,
	.mode_ops = &modes_ops,
	.ctrl_ops = NULL,
	.output_ops = &outputs_ops,
	.params_ops = NULL,
	.misc_ops = NULL,
	.deinitialize = spca2211_encoder_deinitialize,
};

/* This function is called by the encoder manager to initialize spca2211 encoder driver.
 */
static int spca2211_encoder_initialize(struct vid_encoder_device *enc, int flag)
{
	int err = 0, outindex;
	char *std, *output;
	if (NULL == enc) {
		printk(KERN_ERR "enc:NULL Pointer\n");
		return -EINVAL;
	}
	spca2211_encoder_channel_info.enc_device = (struct encoder_device *)enc;

	/* call set standard */
	std = spca2211_encoder_channel_info.params.mode;
	outindex = spca2211_encoder_channel_info.params.outindex;
	output = spca2211_encoder_configuration.output[outindex].output_name;
	err |= spca2211_encoder_setoutput(output, enc);
	if (err < 0) {
		err = -EINVAL;
		printk(KERN_ERR "Error occured in setoutput\n");
		spca2211_encoder_deinitialize(enc);
		return err;
	}
	printk(KERN_DEBUG "spca2211 Encoder initialized\n");
	return err;
}

/* Function to de-initialize the encoder */
static int spca2211_encoder_deinitialize(struct vid_encoder_device *enc)
{
	if (NULL == enc) {
		printk(KERN_ERR "NULL Pointer\n");
		return -EINVAL;
	}
	spca2211_encoder_channel_info.enc_device = NULL;
	printk(KERN_DEBUG "spca2211 Encoder de-initialized\n");
	return 0;
}

/* Following function is used to set the mode*/
static int spca2211_encoder_setmode(struct vid_enc_mode_info *mode_info,
				   struct vid_encoder_device *enc)
{
	int err = 0, outindex, i;
	char *mode;
	struct vid_enc_mode_info *my_mode_info = NULL;

	if ((NULL == enc) || (NULL == mode_info)) {
		printk(KERN_ERR "NULL Pointer\n");
		return -EINVAL;
	}

	if (NULL == (mode = mode_info->name)) {
		printk(KERN_ERR "NULL Pointer\n");
		return -EINVAL;
	}
	//printk(KERN_DEBUG "Start of spca2211_encoder_setmode..\n");
	outindex = spca2211_encoder_channel_info.params.outindex;

	if (mode_info->std) {
		char *mymode = NULL;
		/* This is a standard mode */
		for (i = 0;
		     i <
		     spca2211_encoder_configuration.output[outindex].
		     no_of_standard; i++) {
			if (!strcmp
			    (spca2211_encoder_configuration.output[outindex].
			     standards[i].name, mode)) {
				mymode =
				    spca2211_encoder_configuration.
				    output[outindex].standards[i].name;
				break;
			}
		}
		if ((i ==
		     spca2211_encoder_configuration.output[outindex].
		     no_of_standard) || (NULL == mymode)) {
			printk(KERN_ERR "Invalid id...\n");
			return -EINVAL;
		}
		/* Store the standard in global object of spca2211_encoder */
		spca2211_encoder_channel_info.params.mode = mymode;
		return 0;
	} else {
		/* Non- Standard mode. Check if we support it. If so
		   save the timing info and return */
		for (i = 0; i < SPCA2211_ENCODER_GRAPHICS_NUM_STD; i++) {
			if (!strcmp
			    (spca2211_encoder_configuration.output[outindex].
			     standards[i].name, VID_ENC_STD_NON_STANDARD)) {
				my_mode_info =
				    &spca2211_encoder_configuration.
				    output[outindex].standards[i];
				break;
			}
		}
		if (my_mode_info) {
			/* We support. So save timing info and return success
			   interface type is same as what is currently is active
			 */
			my_mode_info->interlaced = mode_info->interlaced;
			my_mode_info->xres = mode_info->xres;
			my_mode_info->yres = mode_info->yres;
			my_mode_info->fps = mode_info->fps;
			my_mode_info->left_margin = mode_info->left_margin;
			my_mode_info->right_margin = mode_info->right_margin;
			my_mode_info->upper_margin = mode_info->upper_margin;
			my_mode_info->lower_margin = mode_info->lower_margin;
			my_mode_info->hsync_len = mode_info->hsync_len;
			my_mode_info->vsync_len = mode_info->vsync_len;
			my_mode_info->flags = mode_info->flags;
			/* If we need to configure something in the encoder module, we need to
			   do this here */
			return 0;
		}
		printk(KERN_ERR "Mode not supported..\n");
		return -EINVAL;
	}
	//printk(KERN_DEBUG "</spca2211_encoder_setmode>\n");
	return err;
}

/* Following function is used to get currently selected mode.*/
static int spca2211_encoder_getmode(struct vid_enc_mode_info *mode_info,
				   struct vid_encoder_device *enc)
{
	int err = 0, i, outindex;
	if ((NULL == enc) || (NULL == mode_info)) {
		printk(KERN_ERR "NULL Pointer\n");
		return -EINVAL;
	}
	//printk(KERN_DEBUG "<spca2211_encoder_getmode>\n");
	outindex = spca2211_encoder_channel_info.params.outindex;
	for (i = 0; i < SPCA2211_ENCODER_GRAPHICS_NUM_STD; i++) {
		if (!strcmp(spca2211_encoder_channel_info.params.mode,
			    spca2211_encoder_configuration.output[outindex].
			    standards[i].name)) {
			memcpy(mode_info,
			       &spca2211_encoder_configuration.output[outindex].
			       standards[i], sizeof(struct vid_enc_mode_info));
			break;
		}
	}
	if (i == SPCA2211_ENCODER_GRAPHICS_NUM_STD) {
		printk(KERN_ERR "Wiered. No mode info\n");
		return -EINVAL;
	}
	//printk(KERN_DEBUG "</spca2211_encoder_getmode>\n");
	return err;
}

/* For spca2211, we have only one output, called LCD, we
   always set this to this at init
*/
static int spca2211_encoder_setoutput(char *output,
				     struct vid_encoder_device *enc)
{
	int err = 0;
	struct vid_enc_mode_info *my_mode_info;
	//printk(KERN_DEBUG "<spca2211_encoder_setoutput>\n");
	if (NULL == enc) {
		printk(KERN_ERR "enc:NULL Pointer\n");
		return -EINVAL;
	}

	/* check for null pointer */
	if (output == NULL) {
		printk(KERN_ERR "output: NULL Pointer.\n");
		return -EINVAL;
	}

	/* Just check if the default output match with this output name */
	if (strcmp(spca2211_encoder_configuration.output[0].output_name, output)) {
		printk(KERN_ERR "no matching output found.\n");
		return -EINVAL;
	}
	spca2211_encoder_channel_info.params.mode
	    = spca2211_encoder_configuration.output[0].standards[0].name;

	my_mode_info = &spca2211_encoder_configuration.output[0].standards[0];
	err |= spca2211_encoder_setmode(my_mode_info, enc);
	if (err < 0) {
		printk(KERN_ERR "Error in setting default mode\n");
		return err;
	}
	//printk(KERN_DEBUG "</spca2211_encoder_setoutput>\n");
	return err;
}

/* Following function is used to get output name of current output.*/
static int spca2211_encoder_getoutput(char *output,
				     struct vid_encoder_device *enc)
{
	int err = 0, index, len;
	if (NULL == enc) {
		printk(KERN_ERR "enc:NULL Pointer\n");
		return -EINVAL;
	}
	//printk(KERN_DEBUG "<spca2211_encoder_getoutput>\n");
	/* check for null pointer */
	if (output == NULL) {
		printk(KERN_ERR "output:NULL Pointer.\n");
		return -EINVAL;
	}
	index = spca2211_encoder_channel_info.params.outindex;
	len = strlen(spca2211_encoder_configuration.output[index].output_name);
	if (len > (VID_ENC_NAME_MAX_CHARS - 1))
		len = VID_ENC_NAME_MAX_CHARS - 1;
	strncpy(output, spca2211_encoder_configuration.output[index].output_name,
		len);
	output[len] = '\0';
	//printk(KERN_DEBUG "</spca2211_encoder_getoutput>\n");
	return err;
}

/* Following function is used to enumerate outputs supported by the driver.
   It fills in information in the output. */
static int spca2211_encoder_enumoutput(int index, char *output,
				      struct vid_encoder_device *enc)
{
	int err = 0;

	//printk(KERN_DEBUG "<spca2211_encoder_enumoutput>\n");
	if (NULL == enc) {
		printk(KERN_ERR "enc:NULL Pointer.\n");
		return -EINVAL;
	}
	/* check for null pointer */
	if (output == NULL) {
		printk(KERN_ERR "output:NULL Pointer.\n");
		return -EINVAL;
	}
	/* Only one output is available */
	if (index >= spca2211_encoder_configuration.no_of_outputs) {
		return -EINVAL;
	}
	strncpy(output,
		spca2211_encoder_configuration.output[index].output_name,
		VID_ENC_NAME_MAX_CHARS);
	//printk(KERN_DEBUG "</spca2211_encoder_enumoutput>\n");
	return err;
}

/* This function used to initialize the spca2211 encoder driver */
static int spca2211_encoder_init(void)
{
	int err = 0;

	err = vid_enc_register_encoder(&spca2211_encoder_dev);
	printk(KERN_NOTICE "spca2211 encoder initialized\n");
	return err;
}

/* Function used to cleanup spca2211 encoder driver */
static void spca2211_encoder_cleanup(void)
{
	vid_enc_unregister_encoder(&spca2211_encoder_dev);
}

subsys_initcall(spca2211_encoder_init);
module_exit(spca2211_encoder_cleanup);

MODULE_LICENSE("GPL");
