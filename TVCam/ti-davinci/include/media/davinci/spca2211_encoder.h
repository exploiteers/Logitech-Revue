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

#ifndef SPCA2211_ENCODER_H
#define SPCA2211_ENCODER_H

#ifdef __KERNEL__
/* Kernel Header files */
#include <linux/i2c.h>
#include <linux/device.h>
#endif

#ifdef __KERNEL__
/* encoder standard related strctures */
#define SPCA2211_ENCODER_MAX_NO_OUTPUTS		    (1)
#define SPCA2211_ENCODER_GRAPHICS_NUM_STD		    (60)//(13)

struct spca2211_encoder_params {
	int outindex;
	char *mode;
};

struct spca2211_encoder_config {
	int no_of_outputs;
	struct {
		char *output_name;
		int no_of_standard;
		struct vid_enc_mode_info
		 standards[SPCA2211_ENCODER_GRAPHICS_NUM_STD];
	} output[SPCA2211_ENCODER_MAX_NO_OUTPUTS];
};

struct spca2211_encoder_channel {
	struct encoder_device *enc_device;
	struct spca2211_encoder_params params;
};

#endif				/* End of #ifdef __KERNEL__ */

#endif				/* End of #ifndef SPCA2211_ENCODER_H */
