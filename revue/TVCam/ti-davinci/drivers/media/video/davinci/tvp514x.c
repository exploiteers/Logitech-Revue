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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */
/* tvp514x.c : Common driver module for TVP5146 & TVP5147 */
#include <linux/config.h>
#include <linux/init.h>
#include <linux/ctype.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <media/davinci/tvp514x.h>
#include <asm/arch/video_evm.h>
#include <linux/uaccess.h>

/* Prototypes */
static int tvp514x_initialize(void *dec, int flag);
static int tvp514x_deinitialize(void *dec);
static int tvp514x_setcontrol(struct v4l2_control *ctrl, void *dec);
static int tvp514x_getcontrol(struct v4l2_control *ctrl, void *dec);
static int tvp514x_querycontrol(struct v4l2_queryctrl *queryctrl, void *dec);
static int tvp514x_setstd(v4l2_std_id *id, void *dec);
static int tvp514x_getstd(v4l2_std_id *id, void *dec);
static int tvp514x_enumstd(struct v4l2_standard *std, void *dec);
static int tvp514x_querystd(v4l2_std_id *id, void *dec);
static int tvp514x_setinput(int *index, void *dec);
static int tvp514x_getinput(int *index, void *dec);
static int tvp514x_enuminput(struct v4l2_input *input, void *dec);
static int tvp514x_setformat(struct v4l2_format *fmt, void *dec);
static int tvp514x_tryformat(struct v4l2_format *fmt, void *dec);
static int tvp514x_getformat(struct v4l2_format *fmt, void *dec);
static int tvp514x_setparams(void *params, void *dec);
static int tvp514x_getparams(void *params, void *dec);
static int tvp514x_i2c_read_reg(struct i2c_client *client, u8 reg, u8 *val);
static int tvp514x_i2c_write_reg(struct i2c_client *client, u8 reg, u8 val);
static int tvp514x_i2c_attach_client(struct i2c_client *client,
				     struct i2c_driver *driver,
				     struct i2c_adapter *adap, int addr);
static int tvp514x_i2c_detach_client(struct i2c_client *client);
static int tvp514xA_i2c_probe_adapter(struct i2c_adapter *adap);
static int tvp514xB_i2c_probe_adapter(struct i2c_adapter *adap);
static int tvp514x_i2c_init(void);
static void tvp514x_i2c_cleanup(void);
static int tvp514x_get_sliced_vbi_cap(struct v4l2_sliced_vbi_cap *cap,
				      void *dec);
static int tvp514x_read_vbi_data(struct v4l2_sliced_vbi_data *data, void *dec);

static struct v4l2_standard tvp514x_standards[TVP514X_MAX_NO_STANDARDS] = {
	{
		.index = 0,
		.id = V4L2_STD_525_60,
		.name = "NTSC",
		.frameperiod = {1001, 30000},
		.framelines = 525
	},
	{
		.index = 1,
		.id = V4L2_STD_625_50,
		.name = "PAL",
		.frameperiod = {1, 25},
		.framelines = 625
	},
	{
		.index = 2,
		.id = VPFE_STD_AUTO,
		.name = "auto detect",
		.frameperiod = {1, 1},
		.framelines = 1
	},
	{
		.index = 3,
		.id = VPFE_STD_525_60_SQP,
		.name = "NTSC-SQP",
		.frameperiod = {1001, 30000},
		.framelines = 525
	},
	{
		.index = 4,
		.id = VPFE_STD_625_50_SQP,
		.name = "PAL-SQP",
		.frameperiod = {1, 25},
		.framelines = 625
	},
	{
		.index = 5,
		.id = VPFE_STD_AUTO_SQP,
		.name = "auto detect sqp pixel",
		.frameperiod = {1, 1},
		.framelines = 1
	}
};

static enum tvp514x_mode
    tvp514x_modes[TVP514X_MAX_NO_STANDARDS][TVP514X_MAX_NO_MODES] = {
		{TVP514X_MODE_NTSC, TVP514X_MODE_NTSC_443, 0xFF},
		{TVP514X_MODE_PAL, TVP514X_MODE_PAL_M, TVP514X_MODE_PAL_CN},
		{TVP514X_MODE_AUTO, 0xFF, 0xFF},
		{TVP514X_MODE_NTSC_SQP, TVP514X_MODE_NTSC_443_SQP, 0xFF},
		{TVP514X_MODE_PAL_SQP, TVP514X_MODE_PAL_M_SQP,
			TVP514X_MODE_PAL_CN_SQP},
		{TVP514X_MODE_AUTO_SQP, 0xFF, 0xFF}
};

static struct tvp514x_control_info
    tvp514x_control_information[TVP514X_MAX_NO_CONTROLS] = {
	{
		.register_address = TVP514X_BRIGHTNESS,
		.query_control = {
			.id = V4L2_CID_BRIGHTNESS,
			.name = "BRIGHTNESS",
			.type = V4L2_CTRL_TYPE_INTEGER,
			.minimum = 0,
			.maximum = 255,
			.step = 1,
			.default_value = 128
		}
	},
	{
		.register_address = TVP514X_CONTRAST,
		.query_control = {
			.id = V4L2_CID_CONTRAST,
			.name = "CONTRAST",
			.type = V4L2_CTRL_TYPE_INTEGER,
			.minimum = 0,
			.maximum = 255,
			.step = 1,
			.default_value = 128
		}
	},
	{
		.register_address = TVP514X_SATURATION,
		.query_control = {
			.id = V4L2_CID_SATURATION,
			.name = "SATURATION",
			.type = V4L2_CTRL_TYPE_INTEGER,
			.minimum = 0,
			.maximum = 255,
			.step = 1,
			.default_value = 128
		}
	},
	{
		.register_address = TVP514X_HUE,
		.query_control = {
			.id = V4L2_CID_HUE,
			.name = "HUE",
			.type = V4L2_CTRL_TYPE_INTEGER,
			.minimum = -128,
			.maximum = 127,
			.step = 1,
			.default_value = 0
		}
	},
	{
		.register_address = TVP514X_AFE_GAIN_CTRL,
		.query_control = {
			.id = V4L2_CID_AUTOGAIN,
			.name = "Automatic Gain Control",
			.type = V4L2_CTRL_TYPE_BOOLEAN,
			.minimum = 0,
			.maximum = 1,
			.step = 1,
			.default_value = 1
		}
	}
};

static struct tvp514x_config tvp514x_configuration[TVP514X_MAX_CHANNELS] = {
	{
		.sliced_cap = {
			.service_set = (V4L2_SLICED_CAPTION_525 |
					V4L2_SLICED_WSS_625 |
					V4L2_SLICED_CGMS_525),
		},
		.num_services = 0
	},
	{
		.sliced_cap = {
			.service_set = (V4L2_SLICED_CAPTION_525 |
					V4L2_SLICED_WSS_625 |
					V4L2_SLICED_CGMS_525),
		},
		.num_services = 0
	}
};

static struct tvp514x_service_data_reg
	tvp514x_services_regs[TVP514X_VBI_NUM_SERVICES] = {
	{
		.service = V4L2_SLICED_WSS_625,
		.field[0].addr = {0x20, 0x05, 0x80},
		.field[1].addr = {0x24, 0x05, 0x80},
		.bytestoread = 2
	},
	{
		.service = V4L2_SLICED_CAPTION_525,
		.field[0].addr = {0x1C, 0x05, 0x80},
		.field[1].addr = {0x1E, 0x05, 0x80},
		.bytestoread = 2
	},
	{
		.service = V4L2_SLICED_CGMS_525,
		.field[0].addr = {0x20, 0x05, 0x80},
		.field[1].addr = {0x24, 0x05, 0x80},
		.bytestoread = 3
	},
};

static struct tvp514x_sliced_reg
	tvp514x_sliced_regs[TVP514X_VBI_NUM_SERVICES] = {
	{
		.service = V4L2_SLICED_CAPTION_525,
		.std = V4L2_STD_525_60,
		.line_addr_value = 0x15,
		.line_start = 19,
		.line_end = 23,
		.field[0] = {
			.fifo_line_addr = {0x00, 0x06, 0x80},
			.fifo_mode_value = 0x01
		},
		.field[1] = {
			.fifo_line_addr = {0x02, 0x06, 0x80},
			.fifo_mode_value = 0x09
		},
		.service_line = { {21, 21}, {21, 284} }
	},
	{
		.service = V4L2_SLICED_WSS_625,
		.std = V4L2_STD_625_50,
		.line_addr_value = 0x17,
		.line_start = 21,
		.line_end = 25,
		.field[0] = {
			.fifo_line_addr = {0x04, 0x06, 0x80},
			.fifo_mode_value = 0x02
		},
		.field[1] = {
			.fifo_line_addr = {0x04, 0x06, 0x80},
			.fifo_mode_value = 0x02
		},
		.service_line = { {23, 23}, {0, 0} }
	},
	{
		.service = V4L2_SLICED_CGMS_525,
		.std = V4L2_STD_525_60,
		.line_addr_value = 0x14,
		.line_start = 18,
		.line_end = 22,
		.field[0] = {
			.fifo_line_addr = {0x08, 0x06, 0x80},
			.fifo_mode_value = 0x02
		},
		.field[1] = {
			.fifo_line_addr = {0x06, 0x06, 0x80},
			.fifo_mode_value = 0x02
		},
		.service_line = { {20, 20}, {20, 283} }
	}
};

static struct tvp514x_channel tvp514x_channel_info[TVP514X_MAX_CHANNELS];
/* Global variables */
static struct param_ops params_ops = {
	.setparams = tvp514x_setparams,
	.getparams = tvp514x_getparams
};
static struct control_ops controls_ops = {
	.count = TVP514X_MAX_NO_CONTROLS,
	.queryctrl = tvp514x_querycontrol,
	.setcontrol = tvp514x_setcontrol,
	.getcontrol = tvp514x_getcontrol
};

/* Assume each channel has different number of inputs */
static struct input_ops chan0_inputs_ops = {
	.count = 0,
	.enuminput = tvp514x_enuminput,
	.setinput = tvp514x_setinput,
	.getinput = tvp514x_getinput
};

static struct input_ops chan1_inputs_ops = {
	.count = 0,
	.enuminput = tvp514x_enuminput,
	.setinput = tvp514x_setinput,
	.getinput = tvp514x_getinput
};

static struct standard_ops standards_ops = {
	.count = TVP514X_MAX_NO_STANDARDS,
	.setstd = tvp514x_setstd,
	.getstd = tvp514x_getstd,
	.enumstd = tvp514x_enumstd,
	.querystd = tvp514x_querystd,
};
static struct format_ops formats_ops = {
	.count = 0,
	.enumformat = NULL,
	.setformat = tvp514x_setformat,
	.getformat = tvp514x_getformat,
	.tryformat = tvp514x_tryformat,
};

static struct device *tvp514x_i2c_dev[TVP514X_MAX_CHANNELS];

static unsigned int tvp514x_num_channels;

static struct decoder_device dec_dev[TVP514X_MAX_CHANNELS] = {
	{
		.name = "TVP514X",
		.if_type = INTERFACE_TYPE_BT656,
		.channel_id = 0,
		.capabilities =
			V4L2_CAP_SLICED_VBI_CAPTURE | V4L2_CAP_VBI_CAPTURE,
		.initialize = tvp514x_initialize,
		.std_ops = &standards_ops,
		.ctrl_ops = &controls_ops,
		.input_ops = &chan0_inputs_ops,
		.fmt_ops = &formats_ops,
		.params_ops = &params_ops,
		.deinitialize = tvp514x_deinitialize,
		.get_sliced_vbi_cap = tvp514x_get_sliced_vbi_cap,
		.read_vbi_data = tvp514x_read_vbi_data
	},
	{
		.name = "TVP514X",
		.if_type = INTERFACE_TYPE_BT656,
		.channel_id = 1,
		.capabilities =
			V4L2_CAP_SLICED_VBI_CAPTURE | V4L2_CAP_VBI_CAPTURE,
		.initialize = tvp514x_initialize,
		.std_ops = &standards_ops,
		.ctrl_ops = &controls_ops,
		.input_ops = &chan1_inputs_ops,
		.fmt_ops = &formats_ops,
		.params_ops = &params_ops,
		.deinitialize = tvp514x_deinitialize,
		.get_sliced_vbi_cap = tvp514x_get_sliced_vbi_cap,
		.read_vbi_data = tvp514x_read_vbi_data
	}
};

static u8 tvp514x_after_reset_reg[][2] = {
	{0xE8, 0x02},
	{0xE9, 0x00},
	{0xEA, 0x80},
	{0xE0, 0x01},
	{0xE8, 0x60},
	{0xE9, 0x00},
	{0xEA, 0xB0},
	{0xE0, 0x01},
	{0xE8, 0x16},
	{0xE9, 0x00},
	{0xEA, 0xA0},
	{0xE0, 0x16},
	{0xE8, 0x60},
	{0xE9, 0x00},
	{0xEA, 0xB0},
	{0xE0, 0x00}
};

/* tvp514x_init :
 * This function called by vpif driver to initialize the decoder with default
 * values
 */
static int tvp514x_initialize(void *dec, int flag)
{
	int err = 0, i;
	unsigned char ch_id, output;
	int index;
	struct i2c_client *i2c_client;
	v4l2_std_id std;
	struct decoder_device *dec_dev = (struct decoder_device *)dec;

	if (NULL == dec) {
		printk(KERN_ERR "dec:NULL pointer");
		return -EINVAL;
	}
	ch_id = dec_dev->channel_id;
	if (tvp514x_channel_info[ch_id].i2c_dev.i2c_registration & 0x01) {
		printk(KERN_ERR "tvp514x driver is already initialized..\n");
		return -EINVAL;
	}
	i2c_client = &tvp514x_channel_info[ch_id].i2c_dev.client;

	/* Register tvp514x I2C client */
	err = i2c_add_driver(&tvp514x_channel_info[ch_id].i2c_dev.driver);
	if (err) {
		printk(KERN_ERR "Failed to register TVP514X I2C client.\n");
		return -EINVAL;
	}
	try_module_get(THIS_MODULE);
	tvp514x_channel_info[ch_id].i2c_dev.i2c_registration |= 1;
	tvp514x_channel_info[ch_id].dec_device = (struct decoder_device *)dec;
	if (DECODER_I2C_BIND_FLAG == flag) {
		/* check that decoder is set with default values once or not,
		 * if yes return, if not continue */
		if (tvp514x_channel_info[ch_id].i2c_dev.i2c_registration & 0x02)
			return err;
	}

	if (tvp514x_set_input_mux(ch_id)) {
		dev_err(tvp514x_i2c_dev[ch_id],
			"Failed to set input mux for the channel\n");
		return -EINVAL;
	}

	dev_dbg(tvp514x_i2c_dev[ch_id],
		"Starting default settings tvp514x..\n");

	/* Reset TVP514X */
	err = tvp514x_i2c_write_reg(i2c_client, TVP514X_OPERATION_MODE,
				    TVP514X_OPERATION_MODE_RESET);

	/*Put _tvp514x in normal power mode */
	err |= tvp514x_i2c_write_reg(i2c_client, TVP514X_OPERATION_MODE,
				     TVP514X_OPERATION_MODE_DEFAULT);

	for (i = 0; i < 16; i++) {
		err |= tvp514x_i2c_write_reg(i2c_client,
					     tvp514x_after_reset_reg[i][0],
					     tvp514x_after_reset_reg[i]
					     [1]);
	}
	err |= tvp514x_i2c_write_reg(i2c_client, TVP514X_AFE_GAIN_CTRL,
				     TVP514X_AFE_GAIN_CTRL_DEFAULT);
	err |= tvp514x_i2c_write_reg(i2c_client, TVP514X_COLOR_KILLER,
				     TVP514X_COLOR_KILLER_DEFAULT);
	err |= tvp514x_i2c_write_reg(i2c_client, TVP514X_LUMA_CONTROL1,
				     TVP514X_LUMA_CONTROL1_DEFAULT);
	err |= tvp514x_i2c_write_reg(i2c_client, TVP514X_LUMA_CONTROL2,
				     TVP514X_LUMA_CONTROL2_DEFAULT);
	err |= tvp514x_i2c_write_reg(i2c_client, TVP514X_LUMA_CONTROL3,
				     TVP514X_LUMA_CONTROL3_DEFAULT);
	err |= tvp514x_i2c_write_reg(i2c_client, TVP514X_BRIGHTNESS,
				     TVP514X_BRIGHTNESS_DEFAULT);
	err |= tvp514x_i2c_write_reg(i2c_client, TVP514X_CONTRAST,
				     TVP514X_CONTRAST_DEFAULT);
	err |= tvp514x_i2c_write_reg(i2c_client, TVP514X_SATURATION,
				     TVP514X_SATURATION_DEFAULT);
	err |= tvp514x_i2c_write_reg(i2c_client, TVP514X_HUE,
				     TVP514X_HUE_DEFAULT);
	err |= tvp514x_i2c_write_reg(i2c_client, TVP514X_CHROMA_CONTROL1,
				     TVP514X_CHROMA_CONTROL1_DEFAULT);
	err |= tvp514x_i2c_write_reg(i2c_client, TVP514X_CHROMA_CONTROL2,
				     TVP514X_CHROMA_CONTROL2_DEFAULT);

	/* Configuration for interface mode */
	if (dec_dev->if_type & INTERFACE_TYPE_BT656) {
		dev_dbg(tvp514x_i2c_dev[ch_id],
			"configuring embedded sync as per BT656 ..\n");
		err |= tvp514x_i2c_write_reg(i2c_client, TVP514X_OUTPUT5,
					     TVP514X_OUTPUT5_DEFAULT);
		err |= tvp514x_i2c_write_reg(i2c_client, TVP514X_OUTPUT6,
					     TVP514X_OUTPUT6_DEFAULT);
		err |= tvp514x_i2c_write_reg(i2c_client, TVP514X_OUTPUT1,
					     TVP514X_OUTPUT1_DEFAULT);
		err |= tvp514x_i2c_write_reg(i2c_client, TVP514X_OUTPUT2,
					     TVP514X_OUTPUT2_DEFAULT);
		err |= tvp514x_i2c_write_reg(i2c_client, TVP514X_OUTPUT3,
					     TVP514X_OUTPUT3_DEFAULT);
		err |= tvp514x_i2c_write_reg(i2c_client, TVP514X_OUTPUT4,
					     TVP514X_OUTPUT4_DEFAULT);
	} else if (dec_dev->if_type & INTERFACE_TYPE_YCBCR_SYNC_8) {

		output = 0x43;
		err |= tvp514x_i2c_write_reg(i2c_client, TVP514X_OUTPUT1,
					     output);
		output = 0x11;
		err |= tvp514x_i2c_write_reg(i2c_client, TVP514X_OUTPUT2,
					     output);
		output = 0x0;
		err |= tvp514x_i2c_write_reg(i2c_client, TVP514X_OUTPUT3,
					     output);
		output = 0xAF;
		err |= tvp514x_i2c_write_reg(i2c_client, TVP514X_OUTPUT4,
					     output);
		output = 0x4;
		err |= tvp514x_i2c_write_reg(i2c_client, TVP514X_OUTPUT5,
					     output);
		output = 0x1E;
		err |= tvp514x_i2c_write_reg(i2c_client, TVP514X_OUTPUT6,
					     output);
		dev_dbg(tvp514x_i2c_dev[ch_id],
			"configuring seperate sync in 8 bit mode ..\n");
	} else if (dec_dev->if_type & INTERFACE_TYPE_YCBCR_SYNC_16) {
		dev_dbg(tvp514x_i2c_dev[ch_id],
			"configuring seperate sync in 16 bit mode ..\n");
		output = 0x41;
		err |= tvp514x_i2c_write_reg(i2c_client, TVP514X_OUTPUT1,
					     output);
		output = 0x11;
		err |= tvp514x_i2c_write_reg(i2c_client, TVP514X_OUTPUT2,
					     output);
		output = 0x0;
		err |= tvp514x_i2c_write_reg(i2c_client, TVP514X_OUTPUT3,
					     output);
		output = 0xAF;
		err |= tvp514x_i2c_write_reg(i2c_client, TVP514X_OUTPUT4,
					     output);
	}

	if (err < 0) {
		err = -EINVAL;
		dev_err(tvp514x_i2c_dev[ch_id],
			"tvp514x: register initialization failed\n");
		tvp514x_deinitialize(dec);
		return err;
	}
	/* Call setinput for setting default input */
	index = 0;
	while (index < tvp514x_configuration[ch_id].no_of_inputs) {
		err = tvp514x_setinput(&index, dec);
		if (!err) {
			/* call set standard to set default standard */
			std = tvp514x_configuration[ch_id].input[index].def_std;
			err |= tvp514x_setstd(&std, dec);
			err = tvp514x_querystd(&std, dec);
			if (!err)
				break;
		}
		index++;
	}

	if (index == tvp514x_configuration[ch_id].no_of_inputs) {
		tvp514x_deinitialize(dec);
		dev_err(tvp514x_i2c_dev[ch_id],
			"tvp514x: no input connected\n");
		return -EINVAL;
	}

	tvp514x_channel_info[ch_id].params.inputidx = index;
	tvp514x_channel_info[ch_id].i2c_dev.i2c_registration |= 0x2;
	return err;
}
static int tvp514x_deinitialize(void *dec)
{
	unsigned char ch_id;
	if (NULL == dec) {
		printk(KERN_ERR "NULL Pointer\n");
		return -EINVAL;
	}
	ch_id = ((struct decoder_device *)dec)->channel_id;
	dev_dbg(tvp514x_i2c_dev[ch_id], "Tvp5147 ch deinitialization"
		" called\n");
	if (tvp514x_channel_info[ch_id].i2c_dev.i2c_registration & 0x01) {
		i2c_del_driver(&tvp514x_channel_info[ch_id].i2c_dev.driver);
		module_put(THIS_MODULE);
		tvp514x_channel_info[ch_id].i2c_dev.client.adapter = NULL;
		tvp514x_channel_info[ch_id].i2c_dev.i2c_registration &= ~(0x01);
		tvp514x_channel_info[ch_id].dec_device = NULL;
	}
	return 0;
}

/* tvp514x_setcontrol :
 * Function to set the control parameters
 */
static int tvp514x_setcontrol(struct v4l2_control *ctrl, void *dec)
{
	int err = 0;
	int value;
	unsigned char ch_id;
	int i = 0;
	int input_idx;
	struct tvp514x_control_info *control = NULL;
	if (NULL == dec) {
		printk(KERN_ERR "NULL Pointer\n");
		return -EINVAL;
	}

	ch_id = ((struct decoder_device *)dec)->channel_id;
	if (tvp514x_set_input_mux(ch_id)) {
		dev_err(tvp514x_i2c_dev[ch_id],
			"Failed to set input mux for the channel\n");
		return -EINVAL;
	}

	input_idx = tvp514x_channel_info[ch_id].params.inputidx;

	/* check for null pointer */
	if (ctrl == NULL) {
		dev_err(tvp514x_i2c_dev[ch_id], "NULL pointer\n");
		return -EINVAL;
	}
	dev_dbg(tvp514x_i2c_dev[ch_id], "Starting setctrl...\n");
	value = (__s32) ctrl->value;
	for (i = 0; i < tvp514x_configuration[ch_id].input[input_idx].
	     no_of_controls; i++) {
		control = &tvp514x_configuration[ch_id].input[input_idx].
		    controls[i];
		if ((control->query_control).id == ctrl->id)
			break;
	}
	if (i == tvp514x_configuration[ch_id].input[input_idx].no_of_controls)
		return -EINVAL;

	if (V4L2_CID_AUTOGAIN == ctrl->id) {
		if (value == 1)
			value = 0xF;
		else if (value == 0)
			value = 0xC;
		else
			return -EINVAL;
	} else {
		if (((control->query_control).minimum > value)
		    || ((control->query_control).maximum < value))
			return -EINVAL;
	}
	err = tvp514x_i2c_write_reg(&tvp514x_channel_info[ch_id].i2c_dev.
				    client, control->register_address, value);
	if (err < 0) {
		dev_err(tvp514x_i2c_dev[ch_id],
			"TVP514X set control fails...\n");
		return err;
	}
	dev_dbg(tvp514x_i2c_dev[ch_id], "End of setcontrol...\n");
	return err;
}

/* tvp514x_getcontrol :
 * Function to get the control parameters
 */
static int tvp514x_getcontrol(struct v4l2_control *ctrl, void *dec)
{
	int err = 0;
	unsigned char ch_id;
	int i = 0;
	struct tvp514x_control_info *control = NULL;
	int input_idx;
	u8 val;
	if (NULL == dec) {
		printk(KERN_ERR "NULL Pointer\n");
		return -EINVAL;
	}

	ch_id = ((struct decoder_device *)dec)->channel_id;
	if (tvp514x_set_input_mux(ch_id)) {
		dev_err(tvp514x_i2c_dev[ch_id],
			"Failed to set input mux for the channel\n");
		return -EINVAL;
	}

	dev_dbg(tvp514x_i2c_dev[ch_id], "Starting getctrl of TVP514X...\n");
	input_idx = tvp514x_channel_info[ch_id].params.inputidx;

	/* check for null pointer */
	if (ctrl == NULL) {
		dev_err(tvp514x_i2c_dev[ch_id], "NULL pointer\n");
		return -EINVAL;
	}
	for (i = 0; i < tvp514x_configuration[ch_id].input[input_idx].
	     no_of_controls; i++) {
		control = &tvp514x_configuration[ch_id].input[input_idx].
		    controls[i];
		if ((control->query_control).id == ctrl->id)
			break;
	}
	if (i == tvp514x_configuration[ch_id].input[input_idx].no_of_controls) {
		dev_err(tvp514x_i2c_dev[ch_id], "Invalid id...\n");
		return -EINVAL;
	}
	err = tvp514x_i2c_read_reg(&tvp514x_channel_info[ch_id].i2c_dev.
				   client, control->register_address, &val);
	ctrl->value = (int)val;
	if (err < 0) {
		dev_err(tvp514x_i2c_dev[ch_id],
			"TVP514X get control fails...\n");
		return err;
	}
	if (V4L2_CID_AUTOGAIN == ctrl->id) {
		if ((ctrl->value & 0x3) == 3)
			ctrl->value = 1;
		else
			ctrl->value = 0;
	}
	dev_dbg(tvp514x_i2c_dev[ch_id], "End of tvp514x_getcontrol...\n");
	return err;
}

/* This function is used to write the vbi data to the decoder device */
static int tvp514x_read_vbi_data(struct v4l2_sliced_vbi_data *data, void *dec)
{
	int err = 0;
	unsigned char ch_id;
	int i = 0, j, k;
	unsigned char value;
	u8 num_services;

	if (NULL == dec) {
		printk(KERN_ERR "tvp514x_write_vbi_data:NULL Pointer\n");
		return -EINVAL;
	}
	ch_id = ((struct decoder_device *)dec)->channel_id;
	dev_dbg(tvp514x_i2c_dev[ch_id], "tvp514x_write_vbi_data:"
		"Start of tvp514x_write_vbi_data..\n");
	if (data == NULL) {
		dev_err(tvp514x_i2c_dev[ch_id], "tvp514x_write_vbi_data:"
			"NULL pointer.\n");
		return -EINVAL;
	}
	num_services = tvp514x_configuration[ch_id].num_services;
	for (i = 0; i < num_services; i++) {
		if (0 == data[i].id)
			continue;
		if ((data[i].id | tvp514x_channel_info[ch_id].
		     params.fmt.fmt.sliced.service_set) !=
		    tvp514x_channel_info[ch_id].
		    params.fmt.fmt.sliced.service_set) {
			return -EINVAL;
		}

		for (j = 0; j < TVP514X_VBI_NUM_SERVICES; j++) {
			if (!(tvp514x_services_regs[j].service & data[i].id))
				continue;
			for (k = 0; k < 3; k++)
				tvp514x_i2c_write_reg(&tvp514x_channel_info
					[ch_id].i2c_dev.client,
					TVP514X_VBUS_ADDRESS_ACCESS0
					+ k,
					tvp514x_services_regs[j].
					field[data[i].field].
					addr[k]);

			for (k = 0; k <
			     tvp514x_services_regs[j].bytestoread; k++) {
				tvp514x_i2c_read_reg(&tvp514x_channel_info
					[ch_id].i2c_dev.client,
					TVP514X_VBUS_DATA_ACCESS_AUTO_INCR,
					&value);
				data[i].data[k] = value;
			}
		}
	}
	dev_dbg(tvp514x_i2c_dev[ch_id], "</tvp514x_write_vbi_data>\n");
	return err;
}

/* This function is used to get the sliced vbi services supported
   by the decoder device */
static int tvp514x_get_sliced_vbi_cap(struct v4l2_sliced_vbi_cap *cap,
				      void *dec)
{
	unsigned char ch_id;

	if (NULL == dec) {
		printk(KERN_ERR "NULL Pointer\n");
		return -EINVAL;
	}
	ch_id = ((struct decoder_device *)dec)->channel_id;
	dev_dbg(tvp514x_i2c_dev[ch_id], "Start of "
		"tvp514x_get_sliced_vbi_cap\n");
	if (cap == NULL) {
		dev_err(tvp514x_i2c_dev[ch_id],
			"tvp514x_get_sliced_vbi_cap:" "NULL pointer\n");
		return -EINVAL;
	}
	*cap = tvp514x_configuration[ch_id].sliced_cap;
	return 0;
}

/* tvp514x_querycontrol :
 * Function to query control parameters
 */
static int tvp514x_querycontrol(struct v4l2_queryctrl *ctrl, void *dec)
{
	int err = 0;
	int id;
	unsigned char ch_id;
	int i = 0;
	struct tvp514x_control_info *control = NULL;
	int input_idx;
	if (NULL == dec) {
		printk(KERN_ERR "NULL Pointer\n");
		return -EINVAL;
	}
	ch_id = ((struct decoder_device *)dec)->channel_id;
	dev_dbg(tvp514x_i2c_dev[ch_id], "Starting tvp514x_queryctrl...\n");
	input_idx = tvp514x_channel_info[ch_id].params.inputidx;
	if (ctrl == NULL) {
		dev_err(tvp514x_i2c_dev[ch_id], "NULL Pointer\n");
		return -EINVAL;
	}
	id = ctrl->id;
	memset(ctrl, 0, sizeof(struct v4l2_queryctrl));
	ctrl->id = id;
	for (i = 0; i < tvp514x_configuration[ch_id].input[input_idx].
	     no_of_controls; i++) {
		control = &tvp514x_configuration[ch_id].input[input_idx].
		    controls[i];
		if ((control->query_control).id == ctrl->id)
			break;
	}
	if (i == tvp514x_configuration[ch_id].input[input_idx].no_of_controls) {
		dev_err(tvp514x_i2c_dev[ch_id], "Invalid id...\n");
		return -EINVAL;
	}
	memcpy(ctrl, &control->query_control, sizeof(struct v4l2_queryctrl));
	dev_dbg(tvp514x_i2c_dev[ch_id], "End of tvp514x_querycontrol...\n");
	return err;
}

static int tvp514x_raw_vbi_setformat(struct v4l2_vbi_format *fmt)
{
	/* TBD */
	return 0;
}

static int tvp514x_sliced_vbi_setformat(struct v4l2_sliced_vbi_format *fmt,
					void *dec)
{
	int ch_id, i, j, k, index;
	u16 val;
	u8 num_services = 0;
	ch_id = ((struct decoder_device *)dec)->channel_id;
	if ((fmt->service_set |
	     tvp514x_configuration[ch_id].sliced_cap.service_set) !=
	    tvp514x_configuration[ch_id].sliced_cap.service_set) {
		dev_err(tvp514x_i2c_dev[ch_id], "tvp514x_setformat:"
			"Invalid service\n");
		return -EINVAL;
	}
	memset(fmt->service_lines, 0, 2 * 24 * 2);
	for (i = 0; i < TVP514X_VBI_NUM_SERVICES; i++) {
		if ((fmt->service_set & tvp514x_sliced_regs[i].service) &&
		    !(tvp514x_sliced_regs[i].std &
		      tvp514x_channel_info[ch_id].params.std)) {
			dev_err(tvp514x_i2c_dev[ch_id], "tvp514x_setformat:"
				"Invalid service for this standard\n");
			return -EINVAL;
		}

		if (tvp514x_sliced_regs[i].std &
		    tvp514x_channel_info[ch_id].params.std) {
			for (j = 0; j < 2; j++) {
				for (k = 0; k < 3; k++)
					tvp514x_i2c_write_reg
					    (&tvp514x_channel_info[ch_id].
					     i2c_dev.client,
					     TVP514X_VBUS_ADDRESS_ACCESS0 + k,
					     tvp514x_sliced_regs[i].field[j].
					     fifo_line_addr[k]);

				if (fmt->service_set &
				    tvp514x_sliced_regs[i].service) {
					num_services++;
					tvp514x_i2c_write_reg
					    (&tvp514x_channel_info[ch_id].
					     i2c_dev.client,
					     TVP514X_VBUS_DATA_ACCESS_AUTO_INCR,
					     tvp514x_sliced_regs[i].
					     line_addr_value);
					tvp514x_i2c_write_reg
					    (&tvp514x_channel_info[ch_id].
					     i2c_dev.client,
					     TVP514X_VBUS_DATA_ACCESS_AUTO_INCR,
					     tvp514x_sliced_regs[i].field[j].
					     fifo_mode_value);
					tvp514x_i2c_write_reg
					    (&tvp514x_channel_info[ch_id].
					     i2c_dev.client,
					     TVP514X_VDP_LINE_START,
					     tvp514x_sliced_regs[i].line_start);
					tvp514x_i2c_write_reg
					    (&tvp514x_channel_info[ch_id].
					     i2c_dev.client,
					     TVP514X_VDP_LINE_STOP,
					     tvp514x_sliced_regs[i].line_end);
					for (k = 0; k < 2; k++) {
						index = tvp514x_sliced_regs[i].
						    service_line[k].index;
						val = tvp514x_sliced_regs[i].
						    service_line[k].value;
						fmt->service_lines[k][index] =
						    val;
					}
				} else {
					tvp514x_i2c_write_reg
					    (&tvp514x_channel_info[ch_id].
					     i2c_dev.client,
					     TVP514X_VBUS_DATA_ACCESS_AUTO_INCR,
					     TVP514X_LINE_ADDRESS_DEFAULT);
					tvp514x_i2c_write_reg
					    (&tvp514x_channel_info[ch_id].
					     i2c_dev.client,
					     TVP514X_VBUS_DATA_ACCESS_AUTO_INCR,
					     TVP514X_LINE_MODE_DEFAULT);
				}
			}
		}
	}
	fmt->io_size = (TVP514X_SLICED_BUF_SIZE * num_services) / 2;
	tvp514x_configuration[ch_id].num_services = num_services / 2;
	return num_services / 2;
}

/*  tvp514x_setformat :
 * This function is used to set sliced vbi services
 */
static int tvp514x_setformat(struct v4l2_format *fmtp, void *dec)
{
	int err = 0, ch_id;
	struct v4l2_sliced_vbi_format fmta;
	struct v4l2_vbi_format fmtb;
	if (NULL == dec || NULL == fmtp) {
		printk(KERN_ERR "tvp514x_setformat:NULL Pointer\n");
		return -EINVAL;
	}
	if ((V4L2_BUF_TYPE_VIDEO_CAPTURE != fmtp->type) &&
	    (V4L2_BUF_TYPE_VBI_CAPTURE != fmtp->type) &&
	    (V4L2_BUF_TYPE_SLICED_VBI_CAPTURE != fmtp->type))
		return -EINVAL;
	ch_id = ((struct decoder_device *)dec)->channel_id;

	if (V4L2_BUF_TYPE_SLICED_VBI_CAPTURE == fmtp->type) {
		fmta = fmtp->fmt.sliced;
		err = tvp514x_sliced_vbi_setformat(&fmta, dec);
	} else {
		fmtb = fmtp->fmt.vbi;
		err = tvp514x_raw_vbi_setformat(&fmtb);
	}

	tvp514x_channel_info[ch_id].params.fmt = *fmtp;
	dev_dbg(tvp514x_i2c_dev[ch_id],
		"tvp514x_setformat:End of" " tvp514x_querycontrol...\n");
	return err;
}

/* tvp514x_getformat:
 * This function is used to set sliced vbi services
 */
static int tvp514x_getformat(struct v4l2_format *fmtp, void *dec)
{
	int err = 0;
	int ch_id;

	if (NULL == dec || NULL == fmtp) {
		printk(KERN_ERR "NULL Pointer\n");
		return -EINVAL;
	}
	ch_id = ((struct decoder_device *)dec)->channel_id;
	dev_dbg(tvp514x_i2c_dev[ch_id], "tvp514x_getformat:"
		"Starting getformat function.\n");

	/* Read sliced vbi format */
	fmtp->fmt.sliced = tvp514x_channel_info[ch_id].params.fmt.fmt.sliced;
	dev_dbg(tvp514x_i2c_dev[ch_id], "tvp514x_getformat:"
		"End of getformat function.\n");
	return err;
}

/* tvp514x_tryformat:
 * This function is used to set sliced vbi services
 */
static int tvp514x_tryformat(struct v4l2_format *fmtp, void *dec)
{
	int err = 0;
	int ch_id;
	int i = 0;
	struct v4l2_sliced_vbi_format *fmt;
	int num_services = 0;

	if (NULL == dec) {
		printk(KERN_ERR "NULL Pointer\n");
		return -EINVAL;
	}

	ch_id = ((struct decoder_device *)dec)->channel_id;
	dev_dbg(tvp514x_i2c_dev[ch_id], "tvp514x_tryformat:"
		"Start of tvp514x_tryformat..\n");
	if (fmtp == NULL) {
		dev_err(tvp514x_i2c_dev[ch_id], "tvp514x_tryformat:"
			"NULL pointer\n");
		return -EINVAL;
	}
	fmt = &(fmtp->fmt.sliced);
	if ((fmt->service_set |
	     tvp514x_configuration[ch_id].sliced_cap.service_set) !=
	    tvp514x_configuration[ch_id].sliced_cap.service_set) {
		fmt->service_set =
		    tvp514x_configuration[ch_id].sliced_cap.service_set;
		dev_err(tvp514x_i2c_dev[ch_id], "Invalid service\n");
		return -EINVAL;
	}
	for (i = 0; i < TVP514X_VBI_NUM_SERVICES; i++) {
		if (((tvp514x_sliced_regs[i].service & fmt->service_set) ==
		     tvp514x_sliced_regs[i].service) &&
		    (tvp514x_channel_info[ch_id].params.std
		     != tvp514x_sliced_regs[i].std)) {
			dev_err(tvp514x_i2c_dev[ch_id],
				"service not supported for the standard\n");
			return -EINVAL;
		}
	}
	num_services = 0;
	for (i = 0; i < TVP514X_VBI_NUM_SERVICES; i++) {
		if (tvp514x_sliced_regs[i].service & fmt->service_set)
			num_services++;
	}
	fmt->io_size = num_services * TVP514X_SLICED_BUF_SIZE;
	dev_dbg(tvp514x_i2c_dev[ch_id], "</tvp514x_tryformat>\n");
	return err;
}

/* tvp514x_setstd :
 * This function is used to configure TVP514X for video standard passed
 * by application
 */
static int tvp514x_setstd(v4l2_std_id *id, void *dec)
{
	int err = 0;
	unsigned char output1;
	unsigned char ch_id;
	int i = 0;
	struct v4l2_standard *standard;
	int input_idx;
	struct v4l2_sliced_vbi_format fmt;

	enum tvp514x_mode mode = TVP514X_MODE_INV;
	v4l2_std_id std;
	if (NULL == dec) {
		printk(KERN_ERR "NULL Pointer\n");
		return -EINVAL;
	}

	ch_id = ((struct decoder_device *)dec)->channel_id;
	if (tvp514x_set_input_mux(ch_id)) {
		dev_err(tvp514x_i2c_dev[ch_id],
			"Failed to set input mux for the channel\n");
		return -EINVAL;
	}

	dev_dbg(tvp514x_i2c_dev[ch_id], "Start of tvp514x_setstd..\n");
	input_idx = tvp514x_channel_info[ch_id].params.inputidx;
	if (id == NULL) {
		dev_err(tvp514x_i2c_dev[ch_id], "NULL pointer.\n");
		return -EINVAL;
	}
	std = *id;
	for (i = 0; i < tvp514x_configuration[ch_id].input[input_idx].
	     no_of_standard; i++) {
		standard = &(tvp514x_configuration[ch_id].input[input_idx].
			     standard[i]);
		if (standard->id & std)
			break;
	}
	if (i == tvp514x_configuration[ch_id].input[input_idx].no_of_standard) {
		dev_err(tvp514x_i2c_dev[ch_id], "Invalid id...\n");
		return -EINVAL;
	}
	mode = tvp514x_configuration[ch_id].input[input_idx].mode[i][0];

	/* for square pixel */
	err |= tvp514x_i2c_read_reg(&tvp514x_channel_info[ch_id].i2c_dev.
				    client, TVP514X_OUTPUT1, &output1);
	if (err < 0)
		return err;
	output1 |= ((mode & 0x8) << 4);
	err |= tvp514x_i2c_write_reg(&tvp514x_channel_info[ch_id].i2c_dev.
				     client, TVP514X_OUTPUT1, output1);

	/* setup the video standard */
	err |= tvp514x_i2c_write_reg(&tvp514x_channel_info[ch_id].i2c_dev.
				     client, TVP514X_VIDEO_STD, (mode & 0x07));

	/* if autoswitch mode, enable all modes for autoswitch */
	if ((mode & 0x07) == TVP514X_MODE_AUTO) {
		err |= tvp514x_i2c_write_reg(&tvp514x_channel_info[ch_id].
					     i2c_dev.client,
					     TVP514X_AUTOSWT_MASK,
					     TVP514X_AUTOSWITCH_MASK);
	}
	if (err < 0) {
		dev_err(tvp514x_i2c_dev[ch_id], "Set standard failed\n");
		return err;
	}
	tvp514x_channel_info[ch_id].params.std = *id;
	if (*id == VPFE_STD_AUTO || *id == VPFE_STD_AUTO_SQP)
		err = tvp514x_querystd(id, dec);
	/* disable all vbi services */
	fmt.service_set = 0;
	tvp514x_sliced_vbi_setformat(&fmt, dec);

	dev_dbg(tvp514x_i2c_dev[ch_id], "End of tvp514x set standard...\n");
	return err;
}

/* tvp514x_getstd :
 * Function to get the video standard
 */
static int tvp514x_getstd(v4l2_std_id *id, void *dec)
{
	int err = 0;
	int ch_id;
	if (NULL == dec) {
		printk(KERN_ERR "NULL Pointer\n");
		return -EINVAL;
	}
	ch_id = ((struct decoder_device *)dec)->channel_id;
	dev_dbg(tvp514x_i2c_dev[ch_id], "Starting getstd function..\n");
	if (id == NULL) {
		dev_err(tvp514x_i2c_dev[ch_id], "NULL Pointer\n");
		return -EINVAL;
	}
	*id = tvp514x_channel_info[ch_id].params.std;
	dev_dbg(tvp514x_i2c_dev[ch_id], "End of tvp514x_getstd function\n");
	return err;
}

/* tvp514x_enumstd :
 * Function to enumerate standards supported
 */
static int tvp514x_enumstd(struct v4l2_standard *std, void *dec)
{
	int index, index1;
	int err = 0;
	int ch_id;
	int input_idx, sumstd = 0;
	if (NULL == dec) {
		printk(KERN_ERR "NULL Pointer\n");
		return -EINVAL;
	}
	ch_id = ((struct decoder_device *)dec)->channel_id;
	if (std == NULL) {
		printk(KERN_ERR "NULL Pointer\n");
		return -EINVAL;
	}
	index = std->index;
	index1 = index;
	/* Check for valid value of index */
	for (input_idx = 0;
	     input_idx < tvp514x_configuration[ch_id].no_of_inputs;
	     input_idx++) {
		sumstd +=
		    tvp514x_configuration[ch_id].input[input_idx].
		    no_of_standard;
		if (index < sumstd) {
			sumstd -= tvp514x_configuration[ch_id].
			    input[input_idx].no_of_standard;
			break;
		}
	}
	if (input_idx == tvp514x_configuration[ch_id].no_of_inputs)
		return -EINVAL;
	index -= sumstd;
	memset(std, 0, sizeof(*std));
	memcpy(std, &tvp514x_configuration[ch_id].input[input_idx].
	       standard[index], sizeof(struct v4l2_standard));
	std->index = index1;
	return err;
}

/* tvp514x_querystd :
 *
 * Function to return standard detected by decoder
 */
static int tvp514x_querystd(v4l2_std_id *id, void *dec)
{
	int err = 0;

	unsigned char std;
	int ch_id;
	unsigned char output1 = 0;
	int i = 0, j = 0;
	unsigned char lock_status = 0xFF;
	int input_idx, flag = 1;
	struct v4l2_sliced_vbi_format fmt;

	enum tvp514x_mode mode;
	u8 lock_mask;
	if (NULL == dec) {
		printk(KERN_ERR "NULL Pointer\n");
		return -EINVAL;
	}

	ch_id = ((struct decoder_device *)dec)->channel_id;
	if (tvp514x_set_input_mux(ch_id)) {
		dev_err(tvp514x_i2c_dev[ch_id],
			"Failed to set input mux for the channel\n");
		return -EINVAL;
	}

	dev_dbg(tvp514x_i2c_dev[ch_id],
		"Start of tvp514x standard detection.\n");
	input_idx = tvp514x_channel_info[ch_id].params.inputidx;
	if (id == NULL) {
		dev_err(tvp514x_i2c_dev[ch_id], "NULL Pointer\n");
		return -EINVAL;
	}
	err = tvp514x_i2c_read_reg(&tvp514x_channel_info[ch_id].i2c_dev.
				   client, TVP514X_VIDEO_STD, &std);
	if (err < 0) {
		dev_err(tvp514x_i2c_dev[ch_id], "Standard detection failed\n");
		return err;
	}

	std &= 0x7;
	if (std == TVP514X_MODE_AUTO)
		err |= tvp514x_i2c_read_reg(&tvp514x_channel_info[ch_id].
					    i2c_dev.client,
					    TVP514X_VID_STD_STATUS, &std);
	if (err < 0) {
		dev_err(tvp514x_i2c_dev[ch_id], "Standard detection failed\n");
		return err;
	}

	/* to keep standard without square pixel */
	std &= 0x7;

	/* for square pixel */
	err |=
	    tvp514x_i2c_read_reg(&tvp514x_channel_info[ch_id].i2c_dev.
				 client, TVP514X_OUTPUT1, &output1);
	if (err < 0) {
		dev_err(tvp514x_i2c_dev[ch_id],
			"Setting square pixel failed.\n");
		return err;
	}
	mode = std | ((output1 & 0x80) >> 4);	/* square pixel status */

	/* check lock status */
	err |= tvp514x_i2c_read_reg(&tvp514x_channel_info[ch_id].i2c_dev.
				    client, TVP514X_STATUS1, &lock_status);
	if (err < 0)
		return err;
	err = -EAGAIN;
	lock_mask = tvp514x_configuration[ch_id].input[input_idx].lock_mask;
	if (lock_mask != (lock_status & lock_mask))
		return err;
	for (i = 0; i < tvp514x_configuration[ch_id].input[input_idx].
	     no_of_standard; i++) {
		for (j = 0; j < TVP514X_MAX_NO_MODES; j++) {
			if (mode == tvp514x_configuration[ch_id].
			    input[input_idx].mode[i][j]) {
				flag = 0;
				break;
			}
		}
		if (!flag)
			break;
	}

	if ((i == tvp514x_configuration[ch_id].input[input_idx].
	     no_of_standard) && (TVP514X_MAX_NO_MODES == j)) {
		dev_err(tvp514x_i2c_dev[ch_id],
			"tvp514x_querystd:Invalid std\n");
		return -EINVAL;
	}
	*id = tvp514x_configuration[ch_id].input[input_idx].standard[i].id;
	tvp514x_channel_info[ch_id].params.std = *id;

	/* disable all vbi services */
	fmt.service_set = 0;
	tvp514x_sliced_vbi_setformat(&fmt, dec);

	dev_dbg(tvp514x_i2c_dev[ch_id], "End of detection...\n");
	return 0;
}

/* tvp514x_setinput:
 * Function to set the input
 */
static int tvp514x_setinput(int *index, void *dec)
{
	int err = 0;
	unsigned char input_sel;
	unsigned char ch_id;
	u8 status;
	if (NULL == dec) {
		printk(KERN_ERR "NULL Pointer\n");
		return -EINVAL;
	}

	ch_id = ((struct decoder_device *)dec)->channel_id;
	if (tvp514x_set_input_mux(ch_id)) {
		dev_err(tvp514x_i2c_dev[ch_id],
			"Failed to set input mux for the channel\n");
		return -EINVAL;
	}

	dev_dbg(tvp514x_i2c_dev[ch_id], "Start of set input function\n");

	/* check for null pointer */
	if (index == NULL) {
		dev_err(tvp514x_i2c_dev[ch_id], "NULL Pointer\n");
		return -EINVAL;
	}
	if ((*index >= tvp514x_configuration[ch_id].no_of_inputs)
	    || (*index < 0)) {
		dev_err(tvp514x_i2c_dev[ch_id], "Invalid Index.\n");
		return -EINVAL;
	}
	input_sel = tvp514x_configuration[ch_id].input[*index].input_type;
	err = tvp514x_i2c_write_reg(&tvp514x_channel_info[ch_id].i2c_dev.
				    client, TVP514X_INPUT_SEL, input_sel);
	if (err < 0) {
		dev_err(tvp514x_i2c_dev[ch_id], "Set input failed, err = %x\n",
			err);
		return -EINVAL;
	}
	mdelay(500);
	err = tvp514x_i2c_write_reg(&tvp514x_channel_info[ch_id].i2c_dev.
				    client, TVP514X_CLEAR_LOST_LOCK, 0x01);
	if (err < 0) {
		dev_err(tvp514x_i2c_dev[ch_id], "tvp514x_getinput:error "
			"writing  clear lost lock register\n");
		return err;
	}

	/* wait here so that if lock is lost, it can be detected */
	mdelay(500);
	err |= tvp514x_i2c_read_reg(&tvp514x_channel_info[ch_id].i2c_dev.
				    client, TVP514X_STATUS1, &status);
	if (err < 0) {
		dev_err(tvp514x_i2c_dev[ch_id], "tvp514x_setinput:error "
			"reading status register\n");
		return -EINVAL;
	}
	if (TVP514X_LOST_LOCK_MASK == (status & TVP514X_LOST_LOCK_MASK)) {
		dev_err(tvp514x_i2c_dev[ch_id],
			"tvp514x_setinput:lost lock]\n");
		return -EINVAL;
	}
	tvp514x_channel_info[ch_id].params.inputidx = *index;
	dev_dbg(tvp514x_i2c_dev[ch_id], "End of set input function\n");
	return err;
}

/* tvp514x_getinput :
 * Function to get the input
 */
static int tvp514x_getinput(int *index, void *dec)
{
	int err = 0;
	unsigned char ch_id;
	unsigned char input_sel;
	unsigned char status;
	int curr_input_idx;

	if (NULL == dec) {
		printk(KERN_ERR "NULL Pointer\n");
		return -EINVAL;
	}

	ch_id = ((struct decoder_device *)dec)->channel_id;
	curr_input_idx = tvp514x_channel_info[ch_id].params.inputidx;

	if (tvp514x_set_input_mux(ch_id)) {
		dev_err(tvp514x_i2c_dev[ch_id],
			"Failed to set input mux for the channel\n");
		return -EINVAL;
	}

	dev_dbg(tvp514x_i2c_dev[ch_id], "Start of get input function.\n");

	/* check for null pointer */
	if (index == NULL) {
		dev_err(tvp514x_i2c_dev[ch_id], "NULL Pointer\n");
		return -EINVAL;
	}
	input_sel =
	    tvp514x_configuration[ch_id].input[curr_input_idx].input_type;
	err =
	    tvp514x_i2c_write_reg(&tvp514x_channel_info[ch_id].i2c_dev.client,
				  TVP514X_INPUT_SEL, input_sel);
	mdelay(500);
	if (err < 0)
		return err;
	err = tvp514x_i2c_write_reg(&tvp514x_channel_info[ch_id].i2c_dev.
				    client, TVP514X_CLEAR_LOST_LOCK, 0x01);
	if (err < 0)
		return err;

	/* wait here so that if lock is lost, it can be detected */
	mdelay(500);
	err |= tvp514x_i2c_read_reg(&tvp514x_channel_info[ch_id].i2c_dev.
				    client, TVP514X_STATUS1, &status);
	if (err < 0) {
		dev_err(tvp514x_i2c_dev[ch_id], "tvp514x_getinput:error "
			"reading status register\n");
		return -EINVAL;
	}
	if (TVP514X_LOST_LOCK_MASK == (status & TVP514X_LOST_LOCK_MASK))
		return -EINVAL;
	*index = curr_input_idx;

	/* Store the input type in index */
	tvp514x_channel_info[ch_id].params.inputidx = *index;
	dev_dbg(tvp514x_i2c_dev[ch_id], "End of tvp514x_getinput.\n");
	return 0;
}

/* tvp514x_enuminput :
 * Function to enumerate the input
 */
static int tvp514x_enuminput(struct v4l2_input *input, void *dec)
{
	int err = 0;
	int index = 0;
	unsigned char ch_id;
	if (NULL == dec) {
		printk(KERN_ERR "NULL Pointer\n");
		return -EINVAL;
	}
	ch_id = ((struct decoder_device *)dec)->channel_id;

	/* check for null pointer */
	if (input == NULL) {
		dev_err(tvp514x_i2c_dev[ch_id], "NULL Pointer\n");
		return -EINVAL;
	}

	/* Only one input is available */
	if (input->index >= tvp514x_configuration[ch_id].no_of_inputs)
		return -EINVAL;
	index = input->index;
	memset(input, 0, sizeof(*input));
	input->index = index;
	memcpy(input,
	       &tvp514x_configuration[ch_id].input[index].input_info,
	       sizeof(struct v4l2_input));
	return err;
}

/* tvp514x_setparams :
 * Function to set the parameters for tvp514x
 */
static int tvp514x_setparams(void *params, void *dec)
{
	int err = 0;
	unsigned char ch_id;
	struct tvp514x_params tvp514xparams;
	if (NULL == dec) {
		printk(KERN_ERR "NULL Pointer\n");
		return -EINVAL;
	}

	if (copy_from_user(&tvp514xparams, (struct tvp514x_params *)params,
			   sizeof(tvp514xparams)))
		return -EFAULT;

	ch_id = ((struct decoder_device *)dec)->channel_id;
	if (tvp514x_set_input_mux(ch_id)) {
		dev_err(tvp514x_i2c_dev[ch_id],
			"Failed to set input mux for the channel\n");
		return -EINVAL;
	}

	err |= tvp514x_setinput(&(tvp514xparams.inputidx), dec);
	if (err < 0)
		return err;
	err |= tvp514x_setstd(&(tvp514xparams.std), dec);
	if (err < 0)
		return err;
	err |= tvp514x_setformat(&(tvp514xparams.fmt), dec);
	if (err < 0)
		return err;
	tvp514x_channel_info[ch_id].params = tvp514xparams;
	return err;
}

/*  tvp514x_getparams :
 *  Function to get the parameters for tvp514x
 */
static int tvp514x_getparams(void *params, void *dec)
{
	int err = 0;
	unsigned char ch_id;
	struct tvp514x_params *tvp514xparams = (struct tvp514x_params *)params;
	if (NULL == dec) {
		printk(KERN_ERR "NULL Pointer\n");
		return -EINVAL;
	}
	ch_id = ((struct decoder_device *)dec)->channel_id;

	if (copy_to_user(tvp514xparams, &(tvp514x_channel_info[ch_id].params),
			 sizeof(*tvp514xparams)))
		return -EFAULT;

	return err;
}

/* tvp514x_i2c_read_reg :This function is used to read value from
 * register for i2c client.
 */
static int tvp514x_i2c_read_reg(struct i2c_client *client, u8 reg, u8 *val)
{
	int err = 0;

	struct i2c_msg msg[1];
	unsigned char data[1];
	if (!client->adapter) {
		err = -ENODEV;
	} else {
		msg->addr = client->addr;
		msg->flags = 0;
		msg->len = 1;
		msg->buf = data;
		data[0] = reg;
		err = i2c_transfer(client->adapter, msg, 1);
		if (err >= 0) {
			msg->flags = I2C_M_RD;
			err = i2c_transfer(client->adapter, msg, 1);
			if (err >= 0)
				*val = data[0];
		}
	}
	return ((err < 0) ? err : 0);
}

/* tvp514x_i2c_write_reg :This function is used to write value into register
 * for i2c client.
 */
static int tvp514x_i2c_write_reg(struct i2c_client *client, u8 reg, u8 val)
{
	int err = 0;

	struct i2c_msg msg[1];
	unsigned char data[2];
	if (!client->adapter) {
		err = -ENODEV;
	} else {
		msg->addr = client->addr;
		msg->flags = 0;
		msg->len = 2;
		msg->buf = data;
		data[0] = reg;
		data[1] = val;
		err = i2c_transfer(client->adapter, msg, 1);
	}
	return ((err < 0) ? err : 0);
}

/* tvp514x_i2c_attach_client : This function is used to attach i2c client
 */
static int tvp514x_i2c_attach_client(struct i2c_client *client,
				     struct i2c_driver *driver,
				     struct i2c_adapter *adap, int addr)
{
	int err = 0;
	if (client->adapter)
		err = -EBUSY;	/* our client is already attached */
	else {
		client->addr = addr;
		client->driver = driver;
		client->adapter = adap;
		err = i2c_attach_client(client);
		if (err)
			client->adapter = NULL;
	}
	return err;
}

/* tvp514x_i2c_detach_client:
 * This function is used to detach i2c client
 */
static int tvp514x_i2c_detach_client(struct i2c_client *client)
{
	int err = 0;
	if (!client->adapter)
		return -ENODEV;	/* our client isn't attached */
	else {
		err = i2c_detach_client(client);
		client->adapter = NULL;
	}
	return err;
}

/* tvp514xA_i2c_probe_adapter : This function is used to probe i2c adapter
 */
static int tvp514xA_i2c_probe_adapter(struct i2c_adapter *adap)
{
	int err = 0;
	tvp514x_i2c_dev[0] = &(adap->dev);
	dev_dbg(tvp514x_i2c_dev[0], "Tvp514XA i2c probe adapter called...\n");
	err = tvp514x_i2c_attach_client(&tvp514x_channel_info[0].i2c_dev.
					client,
					&tvp514x_channel_info[0].i2c_dev.
					driver, adap,
					tvp514x_channel_info[0].i2c_dev.
					i2c_addr);
	dev_dbg(tvp514x_i2c_dev[0], "Tvp514XA i2c probe adapter ends...\n");
	return err;
}

/* tvp514xB_i2c_probe_adapter: This function is used to probe i2c adapter
 */
static int tvp514xB_i2c_probe_adapter(struct i2c_adapter *adap)
{
	int err = 0;
	tvp514x_i2c_dev[1] = &(adap->dev);
	dev_dbg(tvp514x_i2c_dev[1], "Tvp514XB i2c probe adapter called...\n");
	err = tvp514x_i2c_attach_client(&tvp514x_channel_info[1].i2c_dev.
					client,
					&tvp514x_channel_info[1].i2c_dev.
					driver, adap,
					tvp514x_channel_info[1].i2c_dev.
					i2c_addr);
	dev_dbg(tvp514x_i2c_dev[1], "Tvp514XB i2c probe adapter ends...\n");
	return err;
}

/* tvp514x_i2c_init : This function is used initialize TVP514X i2c client
 */
static int tvp514x_i2c_init(void)
{
	int err = 0;
	int i = 0, j = 0;
	struct tvp514x_evm_chan *evm_chan0, *evm_chan1;
	/* Take instance of driver */
	struct i2c_driver *driver;
	static char strings[TVP514X_MAX_CHANNELS][80] =
	    { "TVP514X channel0 Video Decoder I2C driver",
		"TVP514X channel1 Video Decoder I2C driver"
	};

	/*      tvp514x driver can support upto 2 channels and the
	   channel configuration is evm specific. So get the
	   required configuration from evm configuration
	 */
	evm_chan0 = tvp514x_get_evm_chan_config(0);
	evm_chan1 = tvp514x_get_evm_chan_config(1);
	if (ISNULL(evm_chan0)) {
		printk(KERN_ERR "TVP514x :Channel 0 is mandatory\n");
		return -1;
	}
	chan0_inputs_ops.count = evm_chan0->num_inputs;
	tvp514x_channel_info[0].i2c_dev.i2c_addr = evm_chan0->i2c_addr;
	tvp514x_configuration[0].no_of_inputs = evm_chan0->num_inputs;
	tvp514x_num_channels++;
	if (!(ISNULL(evm_chan1))) {
		tvp514x_num_channels++;
		chan1_inputs_ops.count = evm_chan1->num_inputs;
		tvp514x_channel_info[1].i2c_dev.i2c_addr = evm_chan1->i2c_addr;
		tvp514x_configuration[1].no_of_inputs = evm_chan1->num_inputs;
	}

	printk(KERN_NOTICE "TVP514X : nummber of channels = %d\n",
	       tvp514x_num_channels);

	for (i = 0; i < tvp514x_num_channels; i++) {
		tvp514x_configuration[i].input =
		    kmalloc((sizeof(struct tvp514x_input) *
			     tvp514x_configuration[i].no_of_inputs),
			    GFP_KERNEL);
		if (ISNULL(tvp514x_configuration[i].input)) {
			printk(KERN_ERR "Memory Allocation failure\n");
			return -1;
		}
		for (j = 0; j < tvp514x_configuration[i].no_of_inputs; j++) {
			if (0 == i) {
				tvp514x_configuration[i].input[j].input_type =
				    evm_chan0->input[j].input_sel;
				tvp514x_configuration[i].input[j].lock_mask =
				    evm_chan0->input[j].lock_mask;
				strcpy(tvp514x_configuration[i].input[j].
				       input_info.name,
				       evm_chan0->input[j].name);
			} else {
				tvp514x_configuration[i].input[j].input_type =
				    evm_chan1->input[j].input_sel;
				tvp514x_configuration[i].input[j].lock_mask =
				    evm_chan1->input[j].lock_mask;
				strcpy(tvp514x_configuration[i].input[j].
				       input_info.name,
				       evm_chan1->input[j].name);
			}
			tvp514x_configuration[i].input[j].input_info.index = j;
			tvp514x_configuration[i].input[j].input_info.type =
			    V4L2_INPUT_TYPE_CAMERA;
			tvp514x_configuration[i].input[j].input_info.std =
			    V4L2_STD_TVP514X_ALL;
			tvp514x_configuration[i].input[j].no_of_standard =
			    TVP514X_MAX_NO_STANDARDS;
			tvp514x_configuration[i].input[j].standard =
			    (struct v4l2_standard *)tvp514x_standards;
			tvp514x_configuration[i].input[j].def_std =
			    VPFE_STD_AUTO;
			tvp514x_configuration[i].input[j].mode =
			    (enum tvp514x_mode(*)[]) &tvp514x_modes;
			tvp514x_configuration[i].input[j].no_of_controls =
			    TVP514X_MAX_NO_CONTROLS;
			tvp514x_configuration[i].input[j].controls =
			    (struct tvp514x_control_info *)
			    &tvp514x_control_information;

		}

		/* Configure i2c driver for the channel */
		driver = &tvp514x_channel_info[i].i2c_dev.driver;
		driver->driver.name = strings[i];
		driver->id = I2C_DRIVERID_MISC;
		if (0 == i)
			driver->attach_adapter = tvp514xA_i2c_probe_adapter;
		else
			driver->attach_adapter = tvp514xB_i2c_probe_adapter;
		driver->detach_client = tvp514x_i2c_detach_client;
		err = vpif_register_decoder(&dec_dev[i]);
		if (err < 0) {
			for (j = i - 1; j > 0; j--)
				vpif_unregister_decoder(&dec_dev[j]);
			return err;
		}
	}
	return err;
}

/* tvp514x_i2c_cleanup : This function is used detach TVP514X i2c client
 */
static void tvp514x_i2c_cleanup(void)
{
	int i;
	for (i = 0; i < tvp514x_num_channels; i++) {
		if (tvp514x_channel_info[i].i2c_dev.i2c_registration & 0x01) {
			i2c_del_driver(&tvp514x_channel_info[i].i2c_dev.driver);
			tvp514x_channel_info[i].i2c_dev.client.adapter = NULL;
			tvp514x_channel_info[i].i2c_dev.i2c_registration = 0;
		}
		vpif_unregister_decoder(&dec_dev[i]);
		kfree(tvp514x_configuration[i].input);
	}
}

module_init(tvp514x_i2c_init);
module_exit(tvp514x_i2c_cleanup);
MODULE_LICENSE("GPL");
