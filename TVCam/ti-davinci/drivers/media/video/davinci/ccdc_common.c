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
 * common functions across all CCDC nodules
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <media/davinci/ccdc_common.h>

#define CCDC_CH0_MAX_MODES	(39)

#define CCDC_SD_PARAMS \
	{"NTSC", 720, 480, 30, 0, 0, CCDC_PIXELASPECT_NTSC }, \
	{"PAL", 720, 576, 25, 0, 0, CCDC_PIXELASPECT_PAL}

#define CCDC_HD_PARAMS	\
	{"720P-60", 1280, 720, 60, 1, 1, CCDC_PIXELASPECT_DEFAULT}, \
	{"1080I-30", 1920, 1080, 30, 0, 1, CCDC_PIXELASPECT_DEFAULT}, \
	{"1080I-25", 1920, 1080, 25, 0, 1, CCDC_PIXELASPECT_DEFAULT}

#define CCDC_720P_PARAMS \
	{"720P-25", 1280, 720, 25, 1, 1, CCDC_PIXELASPECT_DEFAULT}, \
	{"720P-30", 1280, 720, 30, 1, 1, CCDC_PIXELASPECT_DEFAULT}, \
	{"720P-50", 1280, 720, 50, 1, 1, CCDC_PIXELASPECT_DEFAULT}

#define CCDC_1080P_PARAMS  \
	{"1080P-24", 1920, 1080, 24, 1, 1, CCDC_PIXELASPECT_DEFAULT}, \
	{"1080P-25", 1920, 1080, 25, 1, 1, CCDC_PIXELASPECT_DEFAULT}, \
	{"1080P-30", 1920, 1080, 30, 1, 1, CCDC_PIXELASPECT_DEFAULT}

#define CCDC_ED_PARAMS	\
	{"480P-60", 720, 480, 60, 1, 0, CCDC_PIXELASPECT_NTSC}, \
	{"576P-50", 720, 576, 50, 1, 0, CCDC_PIXELASPECT_PAL}

#define CCDC_MT9T001_PARAMS \
	{"VGA-30", 640, 480, 30, 1, 0, CCDC_PIXELASPECT_DEFAULT}, \
	{"VGA-60", 640, 480, 60, 1, 0, CCDC_PIXELASPECT_DEFAULT}, \
	{"SVGA-30", 800, 600, 30, 1, 0, CCDC_PIXELASPECT_DEFAULT}, \
	{"SVGA-60", 800, 600, 60, 1, 0, CCDC_PIXELASPECT_DEFAULT}, \
	{"XGA-30", 1024, 768, 30, 1, 0, CCDC_PIXELASPECT_DEFAULT}, \
	{"480P-MT-30", 720, 480, 30, 1, 0, CCDC_PIXELASPECT_DEFAULT}, \
	{"480P-MT-60", 720, 480, 60, 1, 0, CCDC_PIXELASPECT_DEFAULT}, \
	{"576P-MT-25", 720, 576, 25, 1, 0, CCDC_PIXELASPECT_DEFAULT}, \
	{"576P-MT-50", 720, 576, 50, 1, 0, CCDC_PIXELASPECT_DEFAULT}, \
	{"720P-MT-24", 1280, 720, 24, 1, 0, CCDC_PIXELASPECT_DEFAULT}, \
	{"720P-MT-30", 1280, 720, 30, 1, 0, CCDC_PIXELASPECT_DEFAULT}, \
	{"1080P-MT-18", 1920, 1080, 18, 1, 0, CCDC_PIXELASPECT_DEFAULT}, \
	{"SXGA-25", 1280, 1024, 25, 1, 0, CCDC_PIXELASPECT_DEFAULT}, \
	{"UXGA-20", 1600, 1200, 20, 1, 0, CCDC_PIXELASPECT_DEFAULT}, \
	{"QXGA-12", 2048, 1536, 12, 1, 0, CCDC_PIXELASPECT_DEFAULT}

#define CCDC_MT9P031_PARAMS \
	{"SXGA-MP-30", 1280, 1024, 30, 1, 0, CCDC_PIXELASPECT_DEFAULT}, \
	{"UXGA-MP-30", 1600, 1200, 30, 1, 0, CCDC_PIXELASPECT_DEFAULT}, \
	{"QXGA-MP-15", 2048, 1536, 15, 1, 0, CCDC_PIXELASPECT_DEFAULT}, \
	{"720P-MP-50", 1280, 720, 50, 1, 0, CCDC_PIXELASPECT_DEFAULT}, \
	{"720P-MP-60", 1280, 720, 60, 1, 0, CCDC_PIXELASPECT_DEFAULT}, \
	{"720P-MP-25", 1280, 720, 25, 1, 0, CCDC_PIXELASPECT_DEFAULT}, \
	{"1080P-MP-30", 1920, 1080, 30, 1, 0, CCDC_PIXELASPECT_DEFAULT}, \
	{"1080P-MP-25", 1920, 1080, 25, 1, 0, CCDC_PIXELASPECT_DEFAULT}, \
	{"1944P-MP-14", 2592, 1944, 14, 1, 0, CCDC_PIXELASPECT_DEFAULT},\
	{"720P-MP-30", 1280, 720, 30, 1, 0, CCDC_PIXELASPECT_DEFAULT},\
	{"960P-MP-30", 1280, 960, 30, 1, 0, CCDC_PIXELASPECT_DEFAULT}

static struct ccdc_channel_config_params ch0_params[CCDC_CH0_MAX_MODES] = {
	CCDC_SD_PARAMS, CCDC_HD_PARAMS, CCDC_MT9T001_PARAMS, CCDC_720P_PARAMS,
	CCDC_1080P_PARAMS, CCDC_ED_PARAMS, CCDC_MT9P031_PARAMS
};

int ccdc_get_mode_info(struct ccdc_std_info *std_info)
{
	int index, found = -1;

	if (!std_info)
		return found;

	printk(KERN_DEBUG "ccdc_get_mode_info:%s\n", std_info->name);
	/* loop on the number of mode supported per channel */
	for (index = 0; index < CCDC_CH0_MAX_MODES; index++) {

		/* If the mode is found, set the parameter in VPIF register */
		if (0 == strcmp(ch0_params[index].name, std_info->name)) {
			std_info->channel_id = 0;
			std_info->activelines = ch0_params[index].height;
			std_info->activepixels = ch0_params[index].width;
			std_info->fps = ch0_params[index].fps;
			std_info->frame_format = ch0_params[index].frm_fmt;
			std_info->hd_sd = ch0_params[index].hd_sd;
			std_info->pixaspect = ch0_params[index].pixaspect;
			found = 1;
			break;
		}
	}
	return found;
}
EXPORT_SYMBOL(ccdc_get_mode_info);

MODULE_LICENSE("GPL");
