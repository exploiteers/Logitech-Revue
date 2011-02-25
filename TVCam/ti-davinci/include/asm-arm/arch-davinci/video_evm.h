/*
 *
 * Copyright (C) 2008 Texas Instruments	Inc
 *
 * This	program	is free	software; you can redistribute it and/or modify
 * it under the	terms of the GNU General Public	License	as published by
 * the Free Software Foundation; either	version	2 of the License, or
 * (at your option)any	later version.
 *
 * This	program	is distributed in the hope that	it will	be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not,write to the	Free Software
 * Foundation, Inc., 59	Temple Place, Suite 330, Boston, MA  02111-1307	USA
 */
#ifndef _VIDEO_EVM_H
#define _VIDEO_EVM_H

#define TVP514X_EVM_MAX_CHANNELS	2
#define TVP514X_EVM_MAX_INPUTS		10
#define TVP514X_MAX_CHAR		40

struct tvp514x_evm_input {
	/* Name of the input connector */
	char name[TVP514X_MAX_CHAR];
	/* register value to select the above connector */
	unsigned char input_sel;
	/* register value for lock mask */
	unsigned char lock_mask;
};

struct tvp514x_evm_chan {
	unsigned char i2c_addr;
	unsigned char num_inputs;
	struct tvp514x_evm_input input[TVP514X_EVM_MAX_INPUTS];
};

struct tvp514x_evm_config {
	unsigned char num_channels;
	struct tvp514x_evm_chan channels[TVP514X_EVM_MAX_CHANNELS];
};

/* return configuration for a channel. */
struct tvp514x_evm_chan *tvp514x_get_evm_chan_config(unsigned char chan);

/* type = 0 for TVP5146, = 1 for Image sensor */
int tvp514x_set_input_mux(unsigned char channel);
int mt9xxx_set_input_mux(unsigned char channel);
int tvp7002_set_input_mux(unsigned char channel);

#endif /* _VIDEO_EVM_H */
