/*
 * Copyright 2006-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <linux/kernel.h>
#include <asm/arch/mxcfb.h>

const struct fb_videomode mxcfb_modedb[] = {
	[0] = {
	       /* 240x320 @ 60 Hz */
	       "Sharp-QVGA", 60, 240, 320, 185925, 9, 16, 7, 9, 1, 1,
	       FB_SYNC_HOR_HIGH_ACT | FB_SYNC_SHARP_MODE |
	       FB_SYNC_CLK_INVERT | FB_SYNC_DATA_INVERT | FB_SYNC_CLK_IDLE_EN,
	       FB_VMODE_NONINTERLACED,
	       0,
	       },
	[1] = {
	       /* 640x480 @ 60 Hz */
	       "NEC-VGA", 60, 640, 480, 38255, 144, 0, 34, 40, 1, 1,
	       FB_SYNC_VERT_HIGH_ACT | FB_SYNC_OE_ACT_HIGH,
	       FB_VMODE_NONINTERLACED,
	       0,
	       },
	[2] = {
	       /* NTSC TV output */
	       "TV-NTSC", 60, 640, 480, 37538,
	       38, 858 - 640 - 38 - 3,
	       36, 518 - 480 - 36 - 1,
	       3, 1,
	       0,
	       FB_VMODE_NONINTERLACED,
	       0,
	       },
	[3] = {
	       /* PAL TV output */
	       "TV-PAL", 50, 640, 480, 37538,
	       38, 960 - 640 - 38 - 32,
	       32, 555 - 480 - 32 - 3,
	       32, 3,
	       0,
	       FB_VMODE_NONINTERLACED,
	       0,
	       },
	[4] = {
	       /* TV output VGA mode, 640x480 @ 65 Hz */
	       "TV-VGA", 60, 640, 480, 40574, 35, 45, 9, 1, 46, 5,
	       0, FB_VMODE_NONINTERLACED, 0,
	       },
};

int mxcfb_modedb_sz = ARRAY_SIZE(mxcfb_modedb);
EXPORT_SYMBOL(mxcfb_modedb);
EXPORT_SYMBOL(mxcfb_modedb_sz);
