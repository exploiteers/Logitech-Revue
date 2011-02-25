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
#include <linux/config.h>
#include <linux/init.h>
#include <linux/ctype.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/device.h>
#include <media/davinci/mt9t001.h>
#include <asm/arch/video_evm.h>

/*	Function prototype*/
static int mt9t001_initialize(void *dec, int flag);
static int mt9t001_deinitialize(void *dec);
static int mt9t001_setcontrol(struct v4l2_control *ctrl, void *dec);
static int mt9t001_getcontrol(struct v4l2_control *ctrl, void *dec);
static int mt9t001_querycontrol(struct v4l2_queryctrl *ctrl, void *dec);
static int mt9t001_setstd(v4l2_std_id *id, void *dec);
static int mt9t001_getstd(v4l2_std_id *id, void *dec);
static int mt9t001_querystd(v4l2_std_id *id, void *dec);
static int mt9t001_enumstd(struct v4l2_standard *std, void *dec);
static int mt9t001_setinput(int *index, void *dec);
static int mt9t001_getinput(int *index, void *dec);
static int mt9t001_enuminput(struct v4l2_input *input, void *dec);
static int mt9t001_set_format_params(struct mt9t001_format_params
				     *mt9tformats, void *dec);
static int mt9t001_setparams(void *params, void *dec);
static int mt9t001_getparams(void *params, void *dec);
static int mt9t001_i2c_read_reg(struct i2c_client *client,
				unsigned char reg, unsigned short *val,
				int configdev);
static int mt9t001_i2c_write_reg(struct i2c_client *client,
				 unsigned char reg, unsigned short val,
				 int configdev);
static int mt9t001_i2c_attach_client(struct i2c_client *client,
				     struct i2c_driver *driver,
				     struct i2c_adapter *adap, int addr);
static int mt9t001_i2c_detach_client(struct i2c_client *client);
static int mt9t001_i2c_probe_adapter(struct i2c_adapter *adap);
static int mt9t001_i2c_init(void);
static void mt9t001_i2c_cleanup(void);
static struct v4l2_standard mt9t001_standards[MT9T001_MAX_NO_STANDARDS] = {
	{
		.index = 0,
		.id = V4L2_STD_MT9T001_VGA_30FPS,
		.name = "VGA-30",
		.frameperiod = {1, 30},
		.framelines = 480
	},
	{
		.index = 1,
		.id = V4L2_STD_MT9T001_VGA_60FPS,
		.name = "VGA-60",
		.frameperiod = {1, 60},
		.framelines = 480
	},
	{
		.index = 2,
		.id = V4L2_STD_MT9T001_SVGA_30FPS,
		.name = "SVGA-30",
		.frameperiod = {1, 30},
		.framelines = 600
	},
	{
		.index = 3,
		.id = V4L2_STD_MT9T001_SVGA_60FPS,
		.name = "SVGA-60",
		.frameperiod = {1, 60},
		.framelines = 600
	},
	{
		.index = 4,
		.id = V4L2_STD_MT9T001_XGA_30FPS,
		.name = "XGA-30",
		.frameperiod = {1, 30},
		.framelines = 768
	},
	{
		.index = 5,
		.id = V4L2_STD_MT9T001_480p_30FPS,
		.name = "480P-MT-30",
		.frameperiod = {1, 30},
		.framelines = 480
	},
	{
		.index = 6,
		.id = V4L2_STD_MT9T001_480p_60FPS,
		.name = "480P-MT-60",
		.frameperiod = {1, 60},
		.framelines = 480
	},
	{
		.index = 7,
		.id = V4L2_STD_MT9T001_576p_25FPS,
		.name = "576P-MT-25",
		.frameperiod = {1, 25},
		.framelines = 576
	},
	{
		.index = 8,
		.id = V4L2_STD_MT9T001_576p_50FPS,
		.name = "576P-MT-50",
		.frameperiod = {1, 50},
		.framelines = 576
	},
	{
		.index = 9,
		.id = V4L2_STD_MT9T001_720p_24FPS,
		.name = "720P-MT-24",
		.frameperiod = {1, 24},
		.framelines = 720
	},
	{
		.index = 10,
		.id = V4L2_STD_MT9T001_720p_30FPS,
		.name = "720P-MT-30",
		.frameperiod = {1, 30},
		.framelines = 720
	},
	{
		.index = 11,
		.id = V4L2_STD_MT9T001_1080p_18FPS,
		.name = "1080P-MT-18",
		.frameperiod = {1, 18},
		.framelines = 1080
	},
	{
		.index = 12,
		.id = V4L2_STD_MT9T001_SXGA_25FPS,
		.name = "SXGA-25",
		.frameperiod = {1, 25},
		.framelines = 1024
	},
	{
		.index = 13,
		.id = V4L2_STD_MT9T001_UXGA_20FPS,
		.name = "UXGA-20",
		.frameperiod = {1, 20},
		.framelines = 1200
	},
	{
		.index = 14,
		.id = V4L2_STD_MT9T001_QXGA_12FPS,
		.name = "QXGA-12",
		.frameperiod = {1, 12},
		.framelines = 1536
	}
};

/* Parameters for  various format supported  */
/*Format  is
{
	NUMBER OF PIXELS PER LINE, NUMBER OF LINES,
	HRIZONTAL BLANKING WIDTH, VERTICAL BLANKING WIDTH,
	SHUTTER WIDTH, ROW ADDRESS MODE, COL ADDRESS MODE,
	BLACK_LEVEL,PIXEL CLOCK CONTROL,
	ROW START, COL START
}
*/
static struct mt9t001_format_params
	mt9t001_format_parameters[MT9T001_MAX_NO_STANDARDS] = {
	/* V4L2_STD_MT9T001_VGA_30FPS */
	{1979, 1467, 21, 31, 822, 0x22, 0x22, 64, 0x8000, 0, 0},
	/* V4L2_STD_MT9T001_VGA_60FPS */
	{1979, 1467, 21, 31, 582, 0x12, 0x12, 64, 0x8000, 0, 0},
	/* V4L2_STD_MT9T001_SVGA_30FPS */
	{1639, 1239, 21, 31, 1042, 0x11, 0x11, 64, 0x8000, 0, 0},
	/* V4L2_STD_MT9T001_SVGA_60FPS */
	{1639, 1239, 21, 31, 661, 0x01, 0x01, 64, 0x8000, 0, 0},
	/* V4L2_STD_MT9T001_XGA_30FPS */
	{1039, 775, 100, 283, 783, 0x00, 0x00, 64, 0x8000, 0, 0},
	/* V4L2_STD_MT9T001_480p_30FPS */
	{1471, 975, 350, 350, 898, 0x11, 0x11, 64, 0x8000, 0, 0},
	/* V4L2_STD_MT9T001_480p_60FPS */
	{1471, 975, 52, 50, 480, 0x11, 0x11, 64, 0x8000, 0, 0},
	/* V4L2_STD_MT9T001_576p_25FPS */
	{1471, 1167, 424, 450, 500, 0x11, 0x11, 64, 0x8000, 0, 0},
	/* V4L2_STD_MT9T001_576p_50FPS */
	{1471, 1167, 84, 48, 480, 0x11, 0x11, 64, 0x8000, 0, 0},
	/* V4L2_STD_MT9T001_720p_24FPS */
	{1299, 729, 300, 282, 568, 0x00, 0x00, 64, 0x8000, 0, 0},
	/* V4L2_STD_MT9T001_720p_30FPS */
	{1299, 729, 22, 220, 568, 0x00, 0x00, 64, 0x8000, 0, 0},
	/* V4L2_STD_MT9T001_1080p_18FPS */
	{1935, 1091, 24, 50, 600, 0x00, 0x00, 64, 0x8000, 0, 0},
	/* V4L2_STD_MT9T001_SXGA_25FPS */
	{1293, 1026, 186, 10, 1038, 0x00, 0x00, 64, 0x8000, 0, 0},
	/* V4L2_STD_MT9T001_UXGA_20FPS */
	{1601, 1201, 4, 10, 1214, 0x00, 0x00, 64, 0x8000, 0, 0},
	/* V4L2_STD_MT9T001_QXGA_12FPS */
	{2064, 1546, 80, 40, 1400, 0x00, 0x00, 64, 0x8000, 0, 0}
};

static struct mt9t001_control_info
	mt9t001_control_information[MT9T001_MAX_NO_CONTROLS] = {
	{
		.register_address = MT9T001_GLOBAL_GAIN,
		.query_control = {
			.id = V4L2_CID_GAIN,
			.name = "GAIN",
			.type = V4L2_CTRL_TYPE_INTEGER,
			.minimum = 0,
			.maximum = 128,
			.step = 1,
			.default_value = 8
		}
	 }
};

static struct mt9t001_config mt9t001_configuration[MT9T001_NUM_CHANNELS] = {
	{
		.no_of_inputs = MT9T001_MAX_NO_INPUTS,
		.input[0] = {
			.input_type = MT9T001_RAW_INPUT,
			.input_info = {
					.index = 0,
					.name = "RAW",
					.type = V4L2_INPUT_TYPE_CAMERA,
					.std = V4L2_STD_MT9T001_STD_ALL
			},
			.no_of_standard = MT9T001_MAX_NO_STANDARDS,
			.standard = (struct v4l2_standard *)&mt9t001_standards,
			.format = (struct mt9t001_format_params *)
				&mt9t001_format_parameters,
			.no_of_controls = MT9T001_MAX_NO_CONTROLS,
			.controls = (struct mt9t001_control_info *)
				&mt9t001_control_information
		}
	}
};

static struct mt9t001_channel mt9t001_channel_info[MT9T001_NUM_CHANNELS] = {
	{
		.params.inputidx = 0,
		.params.std = V4L2_STD_MT9T001_VGA_30FPS,
		.i2c_dev = {
			.i2c_addr = (0xBA >> 1),
			.i2c_registration = 0
		},
		.dec_device = NULL
	}
};

static struct device *mt9t001_i2c_dev[MT9T001_NUM_CHANNELS];

static struct param_ops params_ops = {
	.setparams = mt9t001_setparams,
	.getparams = mt9t001_getparams
};
static struct control_ops controls_ops = {
	.count = MT9T001_MAX_NO_CONTROLS,
	.queryctrl = mt9t001_querycontrol,
	.setcontrol = mt9t001_setcontrol,
	.getcontrol = mt9t001_getcontrol
};
static struct input_ops inputs_ops = {
	.count = MT9T001_MAX_NO_INPUTS,
	.enuminput = mt9t001_enuminput,
	.setinput = mt9t001_setinput,
	.getinput = mt9t001_getinput
};
static struct standard_ops standards_ops = {
	.count = MT9T001_MAX_NO_STANDARDS,
	.setstd = mt9t001_setstd,
	.getstd = mt9t001_getstd,
	.enumstd = mt9t001_enumstd,
	.querystd = mt9t001_querystd,
};
static struct decoder_device mt9t001_dev[MT9T001_NUM_CHANNELS] = {
	{
		.name = "MT9T001",
		.if_type = INTERFACE_TYPE_RAW,
		.channel_id = 0,
		.capabilities = 0,
		.initialize = mt9t001_initialize,
		.std_ops = &standards_ops,
		.ctrl_ops = &controls_ops,
		.input_ops = &inputs_ops,
		.fmt_ops = NULL,
		.params_ops = &params_ops,
		.deinitialize = mt9t001_deinitialize
	}
};

/* This function called by vpif driver to initialize the decoder with default
 * values
 */
static int mt9t001_initialize(void *dec, int flag)
{
	int err = 0;
	int ch_id;
	v4l2_std_id std;
	int index;
	struct i2c_client *ch_client;
	if (NULL == dec) {
		printk(KERN_ERR "dec:NULL pointer");
		return -EINVAL;
	}
	ch_id = ((struct decoder_device *)dec)->channel_id;
	ch_client = &mt9t001_channel_info[ch_id].i2c_dev.client;
	if (mt9t001_channel_info[ch_id].i2c_dev.i2c_registration & 0x01) {
		printk(KERN_ERR "MT9T001 driver is already initialized..\n");
		err = -EINVAL;
		return err;
	}

	/* Register MT9T001 I2C client */
	err |= i2c_add_driver(&mt9t001_channel_info[ch_id].i2c_dev.driver);
	if (err) {
		printk(KERN_ERR "Failed to register MT9T001 I2C client.\n");
		return -EINVAL;
	}
	mt9t001_channel_info[ch_id].i2c_dev.i2c_registration |= 0x1;
	mt9t001_channel_info[ch_id].dec_device = (struct decoder_device *)dec;
	dev_dbg(mt9t001_i2c_dev[ch_id], "Mt9t001 driver registered\n");
	if (DECODER_I2C_BIND_FLAG == flag) {
		/* check that decoder is set with default values once or not,
		 * if yes return, if not continue */
		if (mt9t001_channel_info[ch_id].i2c_dev.i2c_registration & 0x02)
			return err;
	}
	dev_dbg(mt9t001_i2c_dev[ch_id], "Starting mt9t001_init....\n");

	if (mt9xxx_set_input_mux(0) < 0)
		dev_dbg(mt9t001_i2c_dev[ch_id], "Video Mux setting failed\n");

	msleep(100);

	/*Configure the MT9T001 in normalpower up mode */
	err |=
	    mt9t001_i2c_write_reg(ch_client, MT9T001_ROW_START,
				  MT9T001_ROW_START_DEFAULT,
				  MT9T001_I2C_CONFIG);
	err |=
	    mt9t001_i2c_write_reg(ch_client, MT9T001_COL_START,
				  MT9T001_COLUMN_START_DEFAULT,
				  MT9T001_I2C_CONFIG);
	err |=
	    mt9t001_i2c_write_reg(ch_client, MT9T001_WIDTH,
				  MT9T001_ROW_SIZE_DEFAULT, MT9T001_I2C_CONFIG);
	err |=
	    mt9t001_i2c_write_reg(ch_client, MT9T001_HEIGHT,
				  MT9T001_COLUMN_SIZE_DEFAULT,
				  MT9T001_I2C_CONFIG);
	err |=
	    mt9t001_i2c_write_reg(ch_client, MT9T001_HBLANK,
				  MT9T001_HOR_BLANK_DEFAULT,
				  MT9T001_I2C_CONFIG);
	err |=
	    mt9t001_i2c_write_reg(ch_client, MT9T001_VBLANK,
				  MT9T001_VER_BLANK_DEFAULT,
				  MT9T001_I2C_CONFIG);
	err |=
	    mt9t001_i2c_write_reg(ch_client, MT9T001_OUTPUT_CTRL,
				  MT9T001_OUTPUT_CTRL_DEFAULT,
				  MT9T001_I2C_CONFIG);
	err |=
	    mt9t001_i2c_write_reg(ch_client, MT9T001_SHUTTER_WIDTH_UPPER,
			  MT9T001_SHUTTER_WIDTH_UPPER_DEFAULT,
			  MT9T001_I2C_CONFIG);
	err |=
	    mt9t001_i2c_write_reg(ch_client, MT9T001_SHUTTER_WIDTH,
				  MT9T001_SHUTTER_WIDTH_DEFAULT,
				  MT9T001_I2C_CONFIG);
	err |=
	    mt9t001_i2c_write_reg(ch_client, MT9T001_PIXEL_CLK_CTRL,
			     MT9T001_PIXEL_CLK_CTRL_DEFAULT,
			     MT9T001_I2C_CONFIG);
	err |=
	    mt9t001_i2c_write_reg(ch_client, MT9T001_RESTART,
				  MT9T001_FRAME_RESTART_DEFAULT,
				  MT9T001_I2C_CONFIG);
	err |=
	    mt9t001_i2c_write_reg(ch_client, MT9T001_SHUTTER_DELAY,
				  MT9T001_SHUTTER_DELAY_DEFAULT,
				  MT9T001_I2C_CONFIG);
	err |=
	    mt9t001_i2c_write_reg(ch_client, MT9T001_READ_MODE1,
				  MT9T001_READ_MODE1_DEFAULT,
				  MT9T001_I2C_CONFIG);
	err |=
	    mt9t001_i2c_write_reg(ch_client, MT9T001_READ_MODE2,
				  MT9T001_READ_MODE2_DEFAULT,
				  MT9T001_I2C_CONFIG);
	err |=
	    mt9t001_i2c_write_reg(ch_client, MT9T001_READ_MODE3,
				  MT9T001_READ_MODE3_DEFAULT,
				  MT9T001_I2C_CONFIG);
	err |=
	    mt9t001_i2c_write_reg(ch_client, MT9T001_ROW_ADDR_MODE,
				  MT9T001_ROW_ADDR_MODE_DEFAULT,
				  MT9T001_I2C_CONFIG);
	err |=
	    mt9t001_i2c_write_reg(ch_client, MT9T001_COL_ADDR_MODE,
				  MT9T001_COL_ADDR_MODE_DEFAULT,
				  MT9T001_I2C_CONFIG);
	err |=
	    mt9t001_i2c_write_reg(ch_client, MT9T001_GREEN1_GAIN,
				  MT9T001_GREEN1_GAIN_DEFAULT,
				  MT9T001_I2C_CONFIG);
	err |=
	    mt9t001_i2c_write_reg(ch_client, MT9T001_BLUE_GAIN,
				  MT9T001_BLUE_GAIN_DEFAULT,
				  MT9T001_I2C_CONFIG);
	err |=
	    mt9t001_i2c_write_reg(ch_client, MT9T001_RED_GAIN,
			     MT9T001_RED_GAIN_DEFAULT, MT9T001_I2C_CONFIG);
	err |=
	    mt9t001_i2c_write_reg(ch_client, MT9T001_GREEN2_GAIN,
				  MT9T001_GREEN2_GAIN_DEFAULT,
				  MT9T001_I2C_CONFIG);
	err |=
	    mt9t001_i2c_write_reg(ch_client, MT9T001_GLOBAL_GAIN,
				  MT9T001_GLOBAL_GAIN_DEFAULT,
				  MT9T001_I2C_CONFIG);
	err |=
	    mt9t001_i2c_write_reg(ch_client, MT9T001_BLACK_LEVEL,
				  MT9T001_BLACK_LEVEL_DEFAULT,
				  MT9T001_I2C_CONFIG);
	err |=
	    mt9t001_i2c_write_reg(ch_client, MT9T001_CAL_COARSE,
				  MT9T001_CAL_COARSE_DEFAULT,
				  MT9T001_I2C_CONFIG);
	err |=
	    mt9t001_i2c_write_reg(ch_client, MT9T001_CAL_TARGET,
				  MT9T001_CAL_TARGET_DEFAULT,
				  MT9T001_I2C_CONFIG);
	err |=
	    mt9t001_i2c_write_reg(ch_client, MT9T001_GREEN1_OFFSET,
				  MT9T001_GREEN1_OFFSET_DEFAULT,
				  MT9T001_I2C_CONFIG);
	err |=
	    mt9t001_i2c_write_reg(ch_client, MT9T001_GREEN2_OFFSET,
				  MT9T001_GREEN2_OFFSET_DEFAULT,
				  MT9T001_I2C_CONFIG);
	err |=
	    mt9t001_i2c_write_reg(ch_client, MT9T001_RED_OFFSET,
				  MT9T001_RED_OFFSET_DEFAULT,
				  MT9T001_I2C_CONFIG);
	err |=
	    mt9t001_i2c_write_reg(ch_client, MT9T001_BLUE_OFFSET,
				  MT9T001_BLUE_OFFSET_DEFAULT,
				  MT9T001_I2C_CONFIG);
	err |=
	    mt9t001_i2c_write_reg(ch_client, MT9T001_BLK_LVL_CALIB,
				  MT9T001_BLK_LVL_CALIB_DEFAULT,
				  MT9T001_I2C_CONFIG);
	err |=
	    mt9t001_i2c_write_reg(ch_client, MT9T001_CHIP_ENABLE_SYNC,
			     MT9T001_CHIP_ENABLE_SYNC_DEFAULT,
			     MT9T001_I2C_CONFIG);

	/* Reset the sensor to be in default power up state */
	err |=
	    mt9t001_i2c_write_reg(ch_client, MT9T001_RESET,
			     MT9T001_RESET_ENABLE, MT9T001_I2C_CONFIG);

	/* Clear the bit to resume normal operation */
	err |=
	    mt9t001_i2c_write_reg(ch_client, MT9T001_RESET,
			     MT9T001_RESET_DISABLE, MT9T001_I2C_CONFIG);

	/* delay applying changes  */
	err |=
	    mt9t001_i2c_write_reg(ch_client, MT9T001_OUTPUT_CTRL,
				  MT9T001_N0_CHANGE_MODE, MT9T001_I2C_CONFIG);
	err |=
	    mt9t001_i2c_write_reg(ch_client, MT9T001_PIXEL_CLK_CTRL,
				  MT9T001_PIX_CLK_RISING, MT9T001_I2C_CONFIG);

	/*Configure the MT9T001 in normalpower up mode */
	err |=
	    mt9t001_i2c_write_reg(ch_client, MT9T001_OUTPUT_CTRL,
				  MT9T001_NORMAL_OPERATION_MODE,
				  MT9T001_I2C_CONFIG);

	/* call set standard */
	std = mt9t001_channel_info[ch_id].params.std;
	err |= mt9t001_setstd(&std, dec);

	/* call query standard */
	err |= mt9t001_querystd(&std, dec);

	/* call get input */
	err |= mt9t001_getinput(&index, dec);

	/* Configure for default video standard */
	if (err < 0) {
		err = -EINVAL;
		mt9t001_deinitialize(dec);
		return err;
	}
	mt9t001_channel_info[ch_id].i2c_dev.i2c_registration |= 0x2;
	dev_dbg(mt9t001_i2c_dev[ch_id], "End of mt9t001_init.\n");
	return err;
}

/*
 * This function called by vpif driver to de-initialize the decoder
 */
static int mt9t001_deinitialize(void *dec)
{
	int ch_id;
	if (NULL == dec) {
		printk("NULL Pointer\n");
		return -EINVAL;
	}
	ch_id = ((struct decoder_device *)dec)->channel_id;
	if (mt9t001_channel_info[ch_id].i2c_dev.i2c_registration & 0x01) {
		i2c_del_driver(&mt9t001_channel_info[ch_id].i2c_dev.driver);
		mt9t001_channel_info[ch_id].i2c_dev.client.adapter = NULL;
		mt9t001_channel_info[ch_id].i2c_dev.i2c_registration &= ~(0x01);
		mt9t001_channel_info[ch_id].dec_device = NULL;
	}
	return 0;
}

/* Function to set the control parameters */
static int mt9t001_setcontrol(struct v4l2_control *ctrl, void *dec)
{
	int i = 0;
	int err = 0;
	int ch_id;
	int value;
	int input_idx;
	struct mt9t001_control_info *control = NULL;
	if (NULL == dec) {
		printk(KERN_ERR "NULL Pointer\n");
		return -EINVAL;
	}
	ch_id = ((struct decoder_device *)dec)->channel_id;
	dev_dbg(mt9t001_i2c_dev[ch_id], "Mt9t001 set control started.\n");
	input_idx = mt9t001_channel_info[ch_id].params.inputidx;

	/* check for null pointer */
	if (ctrl == NULL) {
		dev_err(mt9t001_i2c_dev[ch_id], "NULL pointer.\n");
		return -EINVAL;
	}
	value = (unsigned char)ctrl->value;
	for (i = 0; i < mt9t001_configuration[ch_id].input[input_idx].
	     no_of_controls; i++) {
		control = &mt9t001_configuration[ch_id].input[input_idx].
		    controls[i];
		if ((control->query_control).id == ctrl->id)
			break;
	}
	if (i == mt9t001_configuration[ch_id].input[input_idx].no_of_controls)
		return -EINVAL;
	if (((control->query_control).minimum > value)
	    || ((control->query_control).maximum < value))
		return -EINVAL;
	err = mt9t001_i2c_write_reg(&mt9t001_channel_info[ch_id].i2c_dev.
				    client, control->register_address,
				    value, MT9T001_I2C_CONFIG);
	if (err < 0) {
		dev_err(mt9t001_i2c_dev[ch_id],
			"MT9T001 set control fails...\n");
		return err;
	}
	dev_dbg(mt9t001_i2c_dev[ch_id], "Mt9t001 set control done\n");
	return err;
}

/* Function to get the control parameter */
static int mt9t001_getcontrol(struct v4l2_control *ctrl, void *dec)
{
	int input_idx;
	struct mt9t001_control_info *control = NULL;
	int ch_id;
	int i = 0;
	int err = 0;
	if (NULL == dec) {
		printk(KERN_ERR "NULL Pointer\n");
		return -EINVAL;
	}
	ch_id = ((struct decoder_device *)dec)->channel_id;
	dev_dbg(mt9t001_i2c_dev[ch_id], "Mt9t001 get control started.\n");
	input_idx = mt9t001_channel_info[ch_id].params.inputidx;

	/* check for null pointer */
	if (ctrl == NULL) {
		dev_err(mt9t001_i2c_dev[ch_id], "NULL pointer\n");
		return -EINVAL;
	}
	for (i = 0; i < mt9t001_configuration[ch_id].input[input_idx].
	     no_of_controls; i++) {
		control = &mt9t001_configuration[ch_id].input[input_idx].
		    controls[i];
		if ((control->query_control).id == ctrl->id)
			break;
	}
	if (i == mt9t001_configuration[ch_id].input[input_idx].no_of_controls) {
		dev_err(mt9t001_i2c_dev[ch_id], "Invalid id...\n");
		return -EINVAL;
	}
	err = mt9t001_i2c_read_reg(&mt9t001_channel_info[ch_id].i2c_dev.
				   client, control->register_address,
				   (unsigned short *)&(ctrl->value),
				   MT9T001_I2C_CONFIG);
	if (err < 0) {
		dev_err(mt9t001_i2c_dev[ch_id],
			"MT9T001 get control fails...\n");
		return err;
	}
	dev_dbg(mt9t001_i2c_dev[ch_id], "Mt9t001 get control done.\n");
	return err;
}

/* Function to query control parameter */
static int mt9t001_querycontrol(struct v4l2_queryctrl *ctrl, void *dec)
{
	int err = 0;
	int id;
	int ch_id;
	int i = 0;
	struct mt9t001_control_info *control = NULL;
	int input_idx;
	if (NULL == dec) {
		printk(KERN_ERR "NULL Pointer\n");
		return -EINVAL;
	}
	ch_id = ((struct decoder_device *)dec)->channel_id;
	dev_dbg(mt9t001_i2c_dev[ch_id], "Starting mt9t001_queryctrl...\n");
	input_idx = mt9t001_channel_info[ch_id].params.inputidx;
	if (NULL == ctrl) {
		dev_err(mt9t001_i2c_dev[ch_id], "NULL Pointer\n");
		return -EINVAL;
	}
	id = ctrl->id;
	memset(ctrl, 0, sizeof(struct v4l2_queryctrl));
	ctrl->id = id;
	for (i = 0; i < mt9t001_configuration[ch_id].input[input_idx].
	     no_of_controls; i++) {
		control = &mt9t001_configuration[ch_id].input[input_idx].
		    controls[i];
		if ((control->query_control).id == ctrl->id)
			break;
	}
	if (i == mt9t001_configuration[ch_id].input[input_idx].no_of_controls) {
		dev_err(mt9t001_i2c_dev[ch_id], "Invalid id...\n");
		return -EINVAL;
	}
	memcpy(ctrl, &control->query_control, sizeof(struct v4l2_queryctrl));
	dev_dbg(mt9t001_i2c_dev[ch_id], "End of mt9t001_queryctrl...\n");
	return err;
}

/* Function to set the video standard */
static int mt9t001_setstd(v4l2_std_id *id, void *dec)
{
	struct mt9t001_format_params *mt9tformats;
	int err = 0, ch_id, i = 0;
	int input_idx;
	struct v4l2_standard *standard;
	if (NULL == dec) {
		printk(KERN_ERR "NULL Pointer\n");
		return -EINVAL;
	}
	ch_id = ((struct decoder_device *)dec)->channel_id;
	dev_dbg(mt9t001_i2c_dev[ch_id], "Mt9t001 set standard started...\n");
	input_idx = mt9t001_channel_info[ch_id].params.inputidx;
	if (id == NULL) {
		dev_err(mt9t001_i2c_dev[ch_id], "NULL Pointer\n");
		return -EINVAL;
	}
	for (i = 0; i < mt9t001_configuration[ch_id].input[input_idx].
	     no_of_standard; i++) {
		standard = &mt9t001_configuration[ch_id].input[input_idx].
		    standard[i];
		if (standard->id == *id)
			break;
	}
	if (i == mt9t001_configuration[ch_id].input[input_idx].no_of_standard) {
		dev_err(mt9t001_i2c_dev[ch_id], "Invalid id...\n");
		return -EINVAL;
	}
	mt9tformats = &mt9t001_configuration[ch_id].input[input_idx].format[i];
	err = mt9t001_set_format_params(mt9tformats, dec);
	if (err < 0) {
		dev_err(mt9t001_i2c_dev[ch_id], "Set standard failed.\n");
		return err;
	}
	mt9t001_channel_info[ch_id].params.std = *id;
	dev_dbg(mt9t001_i2c_dev[ch_id], "End of mt9t001 set standard...\n");
	return err;
}

/* Function to get the video standard */
static int mt9t001_getstd(v4l2_std_id *id, void *dec)
{
	int err = 0;
	int ch_id;
	if (NULL == dec) {
		printk(KERN_ERR "NULL Pointer\n");
		return -EINVAL;
	}
	ch_id = ((struct decoder_device *)dec)->channel_id;
	dev_dbg(mt9t001_i2c_dev[ch_id], "Mt9t001 get standard started...\n");
	if (id == NULL) {
		dev_err(mt9t001_i2c_dev[ch_id], "NULL Pointer\n");
		return -EINVAL;
	}
	*id = mt9t001_channel_info[ch_id].params.std;
	dev_dbg(mt9t001_i2c_dev[ch_id], "End of getstd function.\n");
	return err;
}

/* Function querystd not supported by mt9t001 */
static int mt9t001_querystd(v4l2_std_id *id, void *dec)
{
	int err = 0;
	int ch_id;
	if (NULL == dec) {
		printk(KERN_ERR "NULL Pointer\n");
		return -EINVAL;
	}
	ch_id = ((struct decoder_device *)dec)->channel_id;
	dev_dbg(mt9t001_i2c_dev[ch_id],
		"MT9T001 does not support this ioctl...\n");
	if (NULL == id) {
		dev_err(mt9t001_i2c_dev[ch_id], "NULL Pointer\n");
		return -EINVAL;
	}
	*id = V4L2_STD_MT9T001_STD_ALL;
	return err;
}

/* Function to enumerate standards supported*/
static int mt9t001_enumstd(struct v4l2_standard *std, void *dec)
{
	u32 index, index1;
	int err = 0;
	int ch_id;
	int input_idx, sumstd = 0;
	if (NULL == dec) {
		printk(KERN_ERR "NULL Pointer\n");
		return -EINVAL;
	}
	ch_id = ((struct decoder_device *)dec)->channel_id;
	if (std == NULL) {
		printk(KERN_ERR "NULL Pointer.\n");
		return -EINVAL;
	}
	index = std->index;
	index1 = index;
	/* Check for valid value of index */
	for (input_idx = 0;
	     input_idx < mt9t001_configuration[ch_id].no_of_inputs;
	     input_idx++) {
		sumstd += mt9t001_configuration[ch_id].input[input_idx]
		    .no_of_standard;
		if (index < sumstd) {
			sumstd -= mt9t001_configuration[ch_id]
			    .input[input_idx].no_of_standard;
			break;
		}
	}
	if (input_idx == mt9t001_configuration[ch_id].no_of_inputs)
		return -EINVAL;
	index -= sumstd;

	memset(std, 0, sizeof(*std));
	memcpy(std, &mt9t001_configuration[ch_id].input[input_idx].
	       standard[index], sizeof(struct v4l2_standard));
	std->index = index1;
	return err;
}

/* Function to set the input */
static int mt9t001_setinput(int *index, void *dec)
{
	int err = 0;
	int ch_id;
	if (NULL == dec) {
		printk(KERN_ERR "NULL Pointer\n");
		return -EINVAL;
	}
	ch_id = ((struct decoder_device *)dec)->channel_id;
	dev_dbg(mt9t001_i2c_dev[ch_id], "Start of set input function.\n");

	/* check for null pointer */
	if (index == NULL) {
		dev_err(mt9t001_i2c_dev[ch_id], "NULL Pointer.\n");
		return -EINVAL;
	}
	if ((*index >= mt9t001_configuration[ch_id].no_of_inputs)
	    || (*index < 0))
		return -EINVAL;
	mt9t001_channel_info[ch_id].params.inputidx = *index;
	dev_dbg(mt9t001_i2c_dev[ch_id], "End of set input function.\n");
	return err;
}

/* Function to get the input*/
static int mt9t001_getinput(int *index, void *dec)
{
	int err = 0;
	int ch_id;
	if (NULL == dec) {
		printk(KERN_ERR "NULL Pointer.\n");
		return -EINVAL;
	}
	ch_id = ((struct decoder_device *)dec)->channel_id;
	dev_dbg(mt9t001_i2c_dev[ch_id], "Start of get input function.\n");

	/* check for null pointer */
	if (index == NULL) {
		dev_err(mt9t001_i2c_dev[ch_id], "NULL Pointer.\n");
		return -EINVAL;
	}
	*index = mt9t001_channel_info[ch_id].params.inputidx;

	/* Call setinput function to set the input */
	err = mt9t001_setinput(index, dec);
	dev_dbg(mt9t001_i2c_dev[ch_id], "End of get input function.\n");
	return err;
}

/* Function to enumerate the input */
static int mt9t001_enuminput(struct v4l2_input *input, void *dec)
{
	int err = 0;
	int index = 0;
	int ch_id;
	if (NULL == dec) {
		printk(KERN_ERR "NULL Pointer\n");
		return -EINVAL;
	}
	ch_id = ((struct decoder_device *)dec)->channel_id;

	/* check for null pointer */
	if (input == NULL) {
		printk(KERN_ERR "MT9T001: NULL Pointer.\n");
		return -EINVAL;
	}

	/* Only one input is available */
	if (input->index >= mt9t001_configuration[ch_id].no_of_inputs)
		return -EINVAL;
	index = input->index;
	memset(input, 0, sizeof(*input));
	input->index = index;
	memcpy(input, &mt9t001_configuration[ch_id].input[index].input_info,
	       sizeof(struct v4l2_input));
	return err;
}

/* Function to set the format parameters*/
static int mt9t001_set_format_params(struct mt9t001_format_params
				     *mt9tformats, void *dec)
{
	int err = 0;
	int ch_id;
	struct i2c_client *ch_client;
	if (NULL == dec) {
		printk(KERN_ERR "NULL Pointer\n");
		return -EINVAL;
	}
	ch_id = ((struct decoder_device *)dec)->channel_id;
	ch_client = &mt9t001_channel_info[ch_id].i2c_dev.client;
	dev_dbg(mt9t001_i2c_dev[ch_id], "Mt9t001 set format params started.\n");
	if (mt9tformats == NULL) {
		dev_err(mt9t001_i2c_dev[ch_id], "NULL Pointer.\n");
		return -EINVAL;
	}

	/*Write the height width and blanking information required
	   for particular format */
	err = mt9t001_i2c_write_reg(ch_client, MT9T001_HEIGHT,
			    mt9tformats->row_size, MT9T001_I2C_CONFIG);
	if (err < 0) {
		dev_err(mt9t001_i2c_dev[ch_id], "I2C write fails...height.\n");
		return err;
	}
	err = mt9t001_i2c_write_reg(ch_client, MT9T001_WIDTH,
			    mt9tformats->col_size, MT9T001_I2C_CONFIG);
	if (err < 0) {
		dev_err(mt9t001_i2c_dev[ch_id], "I2C write fails...width\n");
		return err;
	}

	/* Configure for default video standard */
	err = mt9t001_i2c_write_reg(ch_client, MT9T001_HBLANK,
			    mt9tformats->h_blank, MT9T001_I2C_CONFIG);
	if (err < 0) {
		dev_err(mt9t001_i2c_dev[ch_id], "I2C write fails...hblk\n");
		return err;
	}
	err = mt9t001_i2c_write_reg(ch_client, MT9T001_VBLANK,
			    mt9tformats->v_blank, MT9T001_I2C_CONFIG);
	if (err < 0) {
		dev_err(mt9t001_i2c_dev[ch_id], "I2C write fails...vblk\n");
		return err;
	}
	err = mt9t001_i2c_write_reg(ch_client, MT9T001_SHUTTER_WIDTH,
			    (unsigned short)(mt9tformats->
					     shutter_width &
					     MT9T001_SHUTTER_WIDTH_LOWER_MASK),
			    MT9T001_I2C_CONFIG);
	if (err < 0) {
		dev_err(mt9t001_i2c_dev[ch_id],
			"I2C write fails...shutterwidth.\n");
		return err;
	}
	err = mt9t001_i2c_write_reg(ch_client, MT9T001_SHUTTER_WIDTH_UPPER,
			  (unsigned short)(mt9tformats->
					   shutter_width >>
					   MT9T001_SHUTTER_WIDTH_UPPER_SHIFT),
			  MT9T001_I2C_CONFIG);
	if (err < 0) {
		dev_err(mt9t001_i2c_dev[ch_id], "I2C write fails...upper.\n");
		return err;
	}
	err = mt9t001_i2c_write_reg(ch_client, MT9T001_ROW_ADDR_MODE,
				    mt9tformats->row_addr_mode,
				    MT9T001_I2C_CONFIG);
	if (err < 0) {
		dev_err(mt9t001_i2c_dev[ch_id],
			"I2C write fails...addrmoderow.\n");
		return err;
	}
	err = mt9t001_i2c_write_reg(ch_client, MT9T001_COL_ADDR_MODE,
				    mt9tformats->col_addr_mode,
				    MT9T001_I2C_CONFIG);
	if (err < 0) {
		dev_err(mt9t001_i2c_dev[ch_id],
			"I2C write fails...addrmodecol.\n");
		return err;
	}
	err = mt9t001_i2c_write_reg(ch_client, MT9T001_BLACK_LEVEL,
				    mt9tformats->black_level,
				    MT9T001_I2C_CONFIG);
	if (err < 0) {
		dev_err(mt9t001_i2c_dev[ch_id],
			"I2C write fails...black_level.\n");
		return err;
	}
	err = mt9t001_i2c_write_reg(ch_client, MT9T001_ROW_START,
			    mt9tformats->row_start, MT9T001_I2C_CONFIG);
	if (err < 0) {
		dev_err(mt9t001_i2c_dev[ch_id],
			"I2C write fails...rowstart.\n");
		return err;
	}
	err = mt9t001_i2c_write_reg(ch_client, MT9T001_COL_START,
			    mt9tformats->col_start, MT9T001_I2C_CONFIG);
	if (err < 0) {
		dev_err(mt9t001_i2c_dev[ch_id], "I2C write fails...colstart\n");
		return err;
	}
	err = mt9t001_i2c_write_reg(ch_client, MT9T001_PIXEL_CLK_CTRL,
				    mt9tformats->pixel_clk_control,
				    MT9T001_I2C_CONFIG);
	if (err < 0) {
		dev_err(mt9t001_i2c_dev[ch_id], "I2C write fails...clkctrl\n");
		return err;
	}

	/* applying changes  */
	err = mt9t001_i2c_write_reg(ch_client, MT9T001_OUTPUT_CTRL,
				    MT9T001_NORMAL_OPERATION_MODE,
				    MT9T001_I2C_CONFIG);
	if (err < 0) {
		dev_err(mt9t001_i2c_dev[ch_id], "I2C write fails.outputctrl\n");
		return err;
	}
	dev_dbg(mt9t001_i2c_dev[ch_id], "End of mt9t001 set format params.\n");
	return err;
}

/* This function will configure MT9T001 for bayer pattern capture */
static int mt9t001_setparams(void *params, void *dec)
{
	int err = 0;
	unsigned short val;
	int ch_id;
	struct mt9t001_params *mt9t_params = (struct mt9t001_params *) params;
	if (NULL == dec) {
		printk(KERN_ERR "NULL Pointer\n");
		return -EINVAL;
	}
	ch_id = ((struct decoder_device *)dec)->channel_id;
	dev_dbg(mt9t001_i2c_dev[ch_id],
		"Start of mt9t001 set params function.\n");

	/* check for null pointer */
	if (mt9t_params == NULL) {
		dev_err(mt9t001_i2c_dev[ch_id], "Null pointer.\n");
		return -EINVAL;
	}
	err |= mt9t001_setinput(&(mt9t_params->inputidx), dec);
	if (err < 0) {
		dev_dbg(mt9t001_i2c_dev[ch_id],
			"Set format parameters failed.\n");
		return err;
	}
	err |= mt9t001_setstd(&(mt9t_params->std), dec);
	if (err < 0) {
		dev_dbg(mt9t001_i2c_dev[ch_id],
			"Set format parameters failed.\n");
		return err;
	}

	/* set video format related parameters */
	err = mt9t001_set_format_params(&mt9t_params->format, dec);
	if (err < 0) {
		dev_err(mt9t001_i2c_dev[ch_id],
			"Set format parameters failed.\n");
		return err;
	}

	/*Write the gain information */
	val = (unsigned short)(mt9t_params->rgb_gain.
			       green1_analog_gain & MT9T001_ANALOG_GAIN_MASK);
	val |= ((mt9t_params->rgb_gain.
		 green1_digital_gain << MT9T001_DIGITAL_GAIN_SHIFT) &
		MT9T001_DIGITAL_GAIN_MASK);
	err = mt9t001_i2c_write_reg(&mt9t001_channel_info[ch_id].i2c_dev.
				    client, MT9T001_GREEN1_GAIN, val,
				    MT9T001_I2C_CONFIG);
	if (err < 0) {
		dev_err(mt9t001_i2c_dev[ch_id], "I2C write fails....\n");
		return err;
	}
	val = (unsigned short)(mt9t_params->rgb_gain.red_analog_gain)
	    & MT9T001_ANALOG_GAIN_MASK;
	val |= (((mt9t_params->rgb_gain.red_digital_gain)
		 << MT9T001_DIGITAL_GAIN_SHIFT)
		& MT9T001_DIGITAL_GAIN_MASK);
	err = mt9t001_i2c_write_reg(&mt9t001_channel_info[ch_id].i2c_dev.
				    client, MT9T001_RED_GAIN, val,
				    MT9T001_I2C_CONFIG);
	if (err < 0) {
		dev_err(mt9t001_i2c_dev[ch_id], "I2C write fails...\n");
		return err;
	}
	val = (unsigned short)(mt9t_params->rgb_gain.blue_analog_gain)
	    & MT9T001_ANALOG_GAIN_MASK;
	val |= (((mt9t_params->rgb_gain.blue_digital_gain)
		 << MT9T001_DIGITAL_GAIN_SHIFT)
		& MT9T001_DIGITAL_GAIN_MASK);
	err = mt9t001_i2c_write_reg(&mt9t001_channel_info[ch_id].i2c_dev.
				    client, MT9T001_BLUE_GAIN, val,
				    MT9T001_I2C_CONFIG);
	if (err < 0) {
		dev_err(mt9t001_i2c_dev[ch_id], "I2C write fails...\n");
		return err;
	}
	val = (unsigned short)((mt9t_params->rgb_gain.green2_analog_gain)
			       << MT9T001_ANALOG_GAIN_SHIFT) &
	    MT9T001_ANALOG_GAIN_MASK;
	val |= (((mt9t_params->rgb_gain.
		  green2_digital_gain) << MT9T001_DIGITAL_GAIN_SHIFT) &
		MT9T001_DIGITAL_GAIN_MASK);
	err = mt9t001_i2c_write_reg(&mt9t001_channel_info[ch_id].i2c_dev.
				    client, MT9T001_GREEN2_GAIN, val,
				    MT9T001_I2C_CONFIG);
	if (err < 0) {
		dev_err(mt9t001_i2c_dev[ch_id], "I2C write fails...\n");
		return err;
	}

	/*Write the offset value in register */
	val = mt9t_params->black_calib.green1_offset
	    & MT9T001_GREEN1_OFFSET_MASK;
	err = mt9t001_i2c_write_reg(&mt9t001_channel_info[ch_id].i2c_dev.
				    client, MT9T001_GREEN1_OFFSET, val,
				    MT9T001_I2C_CONFIG);
	if (err < 0) {
		dev_err(mt9t001_i2c_dev[ch_id], "I2C write fails...\n");
		return err;
	}
	val = mt9t_params->black_calib.green2_offset
	    & MT9T001_GREEN2_OFFSET_MASK;
	err = mt9t001_i2c_write_reg(&mt9t001_channel_info[ch_id].i2c_dev.
				    client, MT9T001_GREEN2_OFFSET, val,
				    MT9T001_I2C_CONFIG);
	if (err < 0) {
		dev_err(mt9t001_i2c_dev[ch_id], "I2C write fails...\n");
		return err;
	}
	val = mt9t_params->black_calib.red_offset & MT9T001_RED_OFFSET_MASK;
	err = mt9t001_i2c_write_reg(&mt9t001_channel_info[ch_id].i2c_dev.
				    client, MT9T001_RED_OFFSET, val,
				    MT9T001_I2C_CONFIG);
	if (err < 0) {
		dev_err(mt9t001_i2c_dev[ch_id], "I2C write fails...\n");
		return err;
	}
	val = mt9t_params->black_calib.blue_offset & MT9T001_BLUE_OFFSET_MASK;
	err = mt9t001_i2c_write_reg(&mt9t001_channel_info[ch_id].i2c_dev.
				    client, MT9T001_BLUE_OFFSET, val,
				    MT9T001_I2C_CONFIG);
	if (err < 0) {
		dev_err(mt9t001_i2c_dev[ch_id], "I2C write fails...\n");
		return err;
	}

	/*Write other black caliberation information */
	val = (unsigned short)(mt9t_params->black_calib.manual_override)
	    & MT9T001_MANUAL_OVERRIDE_MASK;
	val |= ((mt9t_params->black_calib.
		 disable_calibration) << MT9T001_DISABLE_CALLIBERATION_SHIFT)
	    & MT9T001_DISABLE_CALLIBERATION_MASK;
	val |= ((mt9t_params->black_calib.
		 recalculate_black_level) << MT9T001_RECAL_BLACK_LEVEL_SHIFT)
	    & MT9T001_RECAL_BLACK_LEVEL_MASK;
	val |= ((mt9t_params->black_calib.lock_red_blue_calibration)
		<< MT9T001_LOCK_RB_CALIBRATION_SHIFT)
	    & MT9T001_LOCK_RB_CALLIBERATION_MASK;
	val |= ((mt9t_params->black_calib.lock_green_calibration)
		<< MT9T001_LOCK_GREEN_CALIBRATION_SHIFT)
	    & MT9T001_LOCK_GREEN_CALLIBERATION_MASK;
	err = mt9t001_i2c_write_reg(&mt9t001_channel_info[ch_id].i2c_dev.
				    client, MT9T001_BLK_LVL_CALIB, val,
				    MT9T001_I2C_CONFIG);
	if (err < 0) {
		dev_err(mt9t001_i2c_dev[ch_id], "I2C write fails...\n");
		return err;
	}

	/*Write Thresholds Value */
	val = (unsigned short)mt9t_params->black_calib.
	    low_coarse_thrld & MT9T001_LOW_COARSE_THELD_MASK;
	val |= (mt9t_params->black_calib.
		high_coarse_thrld << MT9T001_HIGH_COARSE_THELD_SHIFT) &
	    MT9T001_HIGH_COARSE_THELD_MASK;
	err = mt9t001_i2c_write_reg(&mt9t001_channel_info[ch_id].i2c_dev.
				    client, MT9T001_CAL_COARSE, val,
			  MT9T001_I2C_CONFIG);
	if (err < 0) {
		dev_err(mt9t001_i2c_dev[ch_id], "I2C write fails...\n");
		return err;
	}
	val = (unsigned short)mt9t_params->black_calib.low_target_thrld
	    & MT9T001_LOW_TARGET_THELD_MASK;
	val |= (mt9t_params->black_calib.high_target_thrld
		<< MT9T001_HIGH_TARGET_THELD_SHIFT)
	    & MT9T001_HIGH_TARGET_THELD_MASK;
	err = mt9t001_i2c_write_reg(&mt9t001_channel_info[ch_id].i2c_dev.
				    client, MT9T001_CAL_TARGET, val,
				    MT9T001_I2C_CONFIG);
	if (err < 0) {
		dev_err(mt9t001_i2c_dev[ch_id], "I2C write fails...\n");
		return err;
	}
	mt9t001_channel_info[ch_id].params = *mt9t_params;
	dev_dbg(mt9t001_i2c_dev[ch_id], "End of mt9t001 set params...\n");
	return err;
}

/*This function will get MT9T001 configuration values.*/
static int mt9t001_getparams(void *params, void *dec)
{
	int err = 0;
	int ch_id;
	struct mt9t001_params *mt9t_params = (struct mt9t001_params *) params;
	if (NULL == dec) {
		printk(KERN_ERR "NULL Pointer\n");
		return -EINVAL;
	}
	ch_id = ((struct decoder_device *)dec)->channel_id;
	dev_dbg(mt9t001_i2c_dev[ch_id], "Starting mt9t001_getparams.\n");

	/* check for null pointer */
	if (mt9t_params == NULL) {
		dev_err(mt9t001_i2c_dev[ch_id], "Null pointer.\n");
		return -EINVAL;
	}
	*mt9t_params = mt9t001_channel_info[ch_id].params;
	dev_dbg(mt9t001_i2c_dev[ch_id], "End of mt9t001 getparams...\n");
	return err;
}

/* This function is used to read value from register for i2c client */
static int mt9t001_i2c_read_reg(struct i2c_client *client,
				unsigned char reg, unsigned short *val,
				int configdev)
{
	int err = 0;
	struct i2c_msg msg[1];
	unsigned char data[2];

	if (!client->adapter)
		err = -ENODEV;
	else if (configdev == MT9T001_I2C_CONFIG) {
		msg->addr = client->addr;
		msg->flags = 0;
		msg->len = 1;
		msg->buf = data;
		data[0] = reg;
		err = i2c_transfer(client->adapter, msg, 1);
		if (err >= 0) {
			msg->flags = I2C_M_RD;
			msg->len = 2;	/* 2 byte read */
			err = i2c_transfer(client->adapter, msg, 1);
			if (err >= 0)
				*val = ((data[0] & 0x00FF) << 8)
				    | (data[1] & 0x00FF);
		}
	}
	return err;
}

/* This function is used to write value into register for i2c client */
static int mt9t001_i2c_write_reg(struct i2c_client *client,
				 unsigned char reg, unsigned short val,
				 int configdev)
{
	int trycnt = 0;
	unsigned short readval = 0;
	struct i2c_msg msg[1];
	unsigned char data[3];
	int err = -1;
	while ((err < 0) && (trycnt < 5)) {
		trycnt++;
		if (!client->adapter)
			err = -ENODEV;
		else if (configdev == MT9T001_I2C_CONFIG) {
			msg->addr = client->addr;
			msg->flags = 0;
			msg->len = 3;
			msg->buf = data;
			data[0] = reg;
			data[1] = (val & 0xFF00) >> 8;
			data[2] = (val & 0x00FF);
			err = i2c_transfer(client->adapter, msg, 1);
			if (err >= 0) {
				err =
				    mt9t001_i2c_read_reg(client, reg,
							 &readval,
							 MT9T001_I2C_CONFIG);
				if ((err >= 0) && (val != readval)) {
					printk
					    (KERN_WARNING
					     "\n WARNING: i2c readback"
						" failed, val = %d,"
						" readval = %d, reg = %d\n",
					     val, readval, reg);
				}
				readval = 0;
				/* readback error will not flag error */
				err = 0;
			}
		}
	}
	if (err < 0)
		printk(KERN_ERR "\n I2C write failed");
	return err;
}

/* This function is used to attach i2c client */
static int mt9t001_i2c_attach_client(struct i2c_client *client,
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

/* This function is used to detach i2c client */
static int mt9t001_i2c_detach_client(struct i2c_client *client)
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

/* This function is used to probe i2c adapter */
static int mt9t001_i2c_probe_adapter(struct i2c_adapter *adap)
{
	int err = 0;
	mt9t001_i2c_dev[0] = &(adap->dev);
	dev_dbg(mt9t001_i2c_dev[0], "Mt9t001 i2c probe adapter called...\n");
	err = mt9t001_i2c_attach_client(&mt9t001_channel_info[0].i2c_dev.
					client,
					&mt9t001_channel_info[0].i2c_dev.
					driver, adap,
					mt9t001_channel_info[0].i2c_dev.
					i2c_addr);
	dev_dbg(mt9t001_i2c_dev[0], "Mt9t001 i2c probe adapter ends...\n");
	return err;
}

/* This function is used initialize mt9t001 i2c client */
static int mt9t001_i2c_init(void)
{
	int err = 0;
	int i = 0;
	int j;

	/* Take instance of driver */
	struct i2c_driver *driver;
	static char strings[MT9T001_NUM_CHANNELS][80] =
	    { "MT9T001 channel0 Video Decoder I2C driver",
	};
	for (i = 0; i < MT9T001_NUM_CHANNELS; i++) {
		driver = &mt9t001_channel_info[i].i2c_dev.driver;
		driver->driver.name = strings[i];
		driver->id = I2C_DRIVERID_MISC;
		if (0 == i)
			driver->attach_adapter = mt9t001_i2c_probe_adapter;
		driver->detach_client = mt9t001_i2c_detach_client;
		err = vpif_register_decoder(&mt9t001_dev[i]);
		if (err < 0) {
			for (j = i - 1; j > 0; j--)
				vpif_unregister_decoder(&mt9t001_dev[j]);
			return err;
		}
	}
	return err;
}

/* This function is used detach mt9t001 i2c client */
static void mt9t001_i2c_cleanup(void)
{
	int i;
	for (i = 0; i < MT9T001_NUM_CHANNELS; i++) {
		if (mt9t001_channel_info[i].i2c_dev.i2c_registration & 0x01) {
			i2c_del_driver(&mt9t001_channel_info[i].i2c_dev.driver);
			mt9t001_channel_info[i].i2c_dev.client.adapter = NULL;
			mt9t001_channel_info[i].i2c_dev.i2c_registration = 0;
		}
		vpif_unregister_decoder(&mt9t001_dev[i]);
	}
}

module_init(mt9t001_i2c_init);
module_exit(mt9t001_i2c_cleanup);
MODULE_LICENSE("GPL");

/**************************************************************************/
/* End of file                                                        */
/**************************************************************************/
