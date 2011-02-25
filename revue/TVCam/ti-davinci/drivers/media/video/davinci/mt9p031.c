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
#include <linux/config.h>
#include <linux/init.h>
#include <linux/ctype.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/device.h>
#include <media/davinci/mt9p031.h>
#include <asm/arch/video_evm.h>

/*	Function prototype*/
static int mt9p031_initialize(void *dec, int flag);
static int mt9p031_deinitialize(void *dec);
static int mt9p031_setcontrol(struct v4l2_control *ctrl, void *dec);
static int mt9p031_getcontrol(struct v4l2_control *ctrl, void *dec);
static int mt9p031_querycontrol(struct v4l2_queryctrl *ctrl, void *dec);
static int mt9p031_setstd(v4l2_std_id *id, void *dec);
static int mt9p031_getstd(v4l2_std_id *id, void *dec);
static int mt9p031_querystd(v4l2_std_id *id, void *dec);
static int mt9p031_enumstd(struct v4l2_standard *std, void *dec);
static int mt9p031_setinput(int *index, void *dec);
static int mt9p031_getinput(int *index, void *dec);
static int mt9p031_enuminput(struct v4l2_input *input, void *dec);
static int mt9p031_set_format_params(struct mt9p031_format_params
				     *formats, void *dec);
static int mt9p031_setparams(void *params, void *dec);
static int mt9p031_getparams(void *params, void *dec);
static int mt9p031_i2c_read_reg(struct i2c_client *client,
				unsigned char reg, unsigned short *val);
static int mt9p031_i2c_write_reg(struct i2c_client *client,
				 unsigned char reg, unsigned short val);
static int mt9p031_i2c_attach_client(struct i2c_client *client,
				     struct i2c_driver *driver,
				     struct i2c_adapter *adap, int addr);
static int mt9p031_i2c_detach_client(struct i2c_client *client);
static int mt9p031_i2c_probe_adapter(struct i2c_adapter *adap);
static int mt9p031_i2c_init(void);
static void mt9p031_i2c_cleanup(void);

static struct v4l2_standard mt9p031_standards[MT9P031_MAX_NO_STANDARDS] = {
	{
		.index = 0,
		.id = V4L2_STD_MT9P031_VGA_30FPS,
		.name = "VGA-30",
		.frameperiod = {1, 30},
		.framelines = 480
	},
	{
		.index = 1,
		.id = V4L2_STD_MT9P031_SXGA_30FPS,
		.name = "SXGA-MP-30",
		.frameperiod = {1, 30},
		.framelines = 1024
	},
	{
		.index = 2,
		.id = V4L2_STD_MT9P031_UXGA_30FPS,
		.name = "UXGA-MP-30",
		.frameperiod = {1, 30},
		.framelines = 1200
	},
	{
		.index = 3,
		.id = V4L2_STD_MT9P031_QXGA_15FPS,
		.name = "QXGA-MP-15",
		.frameperiod = {1, 15},
		.framelines = 1536
	},
	{
		.index = 4,
		.id = V4L2_STD_MT9P031_480p_30FPS,
		.name = "480P-MT-30",
		.frameperiod = {1, 30},
		.framelines = 480
	},
	{
		.index = 5,
		.id = V4L2_STD_MT9P031_576p_25FPS,
		.name = "576P-MT-25",
		.frameperiod = {1, 25},
		.framelines = 576
	},
	{
		.index = 6,
		.id = V4L2_STD_MT9P031_720p_60FPS,
		.name = "720P-MP-60",
		.frameperiod = {1, 60},
		.framelines = 720
	},
	{
		.index = 7,
		.id = V4L2_STD_MT9P031_1080p_30FPS,
		.name = "1080P-MP-30",
		.frameperiod = {1, 30},
		.framelines = 1080
	},
	{
		.index = 8,
		.id = V4L2_STD_MT9P031_1080p_25FPS,
		.name = "1080P-MP-25",
		.frameperiod = {1, 25},
		.framelines = 1080
	},
	{
		.index = 9,
		.id = V4L2_STD_MT9P031_720p_30FPS,
		.name = "720P-MP-30",
		.frameperiod = {1, 30},
		.framelines = 720
	},
	{
		.index = 10,
		.id = V4L2_STD_MT9P031_960p_30FPS,
		.name = "960P-MP-30",
		.frameperiod = {1, 30},
		.framelines = 960
	}
};

/*
 * Parameters for  various format supported
 * Format  is
 * {
 * 	NUMBER OF PIXELS PER LINE, NUMBER OF LINES,
 *	HRIZONTAL BLANKING WIDTH, VERTICAL BLANKING WIDTH,
 *	SHUTTER WIDTH, ROW ADDRESS MODE, COL ADDRESS MODE,
 *	BLACK_LEVEL, PIXEL CLOCK CONTROL,
 *	ROW START, COL START,  READ MODE2 REG, OUTPUT CONFIG
 *	PLL M, PLL N, PLL P1
 * }
 */
static struct mt9p031_format_params
	mt9p031_format_parameters[MT9P031_MAX_NO_STANDARDS] = {
	/* V4L2_STD_MT9P031_VGA_30FPS */
	{2591, 1939, 840, 200, 478, 0x13, 0x13, 64, 0x8000, 0, 0, 0, 0x1F82,
		0x10, 4, 2 },
	/* V4L2_STD_MT9P031_SXGA_30FPS */
	{1296, 1035, 850, 30, 1022, 0x00, 0x00, 64, 0x8000, 0, 0, 0, 0x1F82,
		0x10, 2, 2 },
	/* V4L2_STD_MT9P031_UXGA_30FPS */
	{1615, 1209, 482, 26, 1022, 0x00, 0x00, 64, 0x8000, 0, 0, 0, 0x1F82,
		0x10, 2, 2 },
	/* V4L2_STD_MT9P031_QXGA_15FPS */
	{2063, 1545, 982, 40, 1022, 0x00, 0x00, 64, 0x8000, 0, 0, 0, 0x1F82,
		0x10, 2, 2 },
	/* V4L2_STD_MT9P031_480p_30FPS */
	{1499, 999, 800, 180, 478, 0x11, 0x11, 64, 0x8000, 0, 0, 0, 0x1F82,
		0x10, 2, 4 },
	/* V4L2_STD_MT9P031_576p_25FPS */
	{1500, 1180, 778, 240, 575, 0x11, 0x11, 64, 0x8000, 0, 0, 0, 0x1F82,
		0x10, 2, 4 },
	/* V4L2_STD_MT9P031_720p_60FPS */
	{2564, 1447, 460, 146, 718, 0x01, 0x01, 64, 0x8000, 0, 0, 0, 0x1F82,
		0x40, 2, 8 },
	/* V4L2_STD_MT9P031_1080p_30FPS */
	{1939, 1087, 488, 14, 1078, 0x00, 0x00, 64, 0x8000, 0, 0, 0, 0x1F82,
		0x40, 2, 8 },
	/* V4L2_STD_MT9P031_1080p_25FPS */
	{1939, 1087, 770, 14, 1078, 0x00, 0x00, 64, 0x8000, 0, 0, 0, 0x1F82,
		0x40, 2, 8 },
	/* V4L2_STD_MT9P031_720p_30FPS */
	{2583, 1491, 786, 0, 740, 0x11, 0x11, 64, 0x8000, 0, 0, 0, 0x1F8E,
		16, 3, 2 },
	/* V4L2_STD_MT9P031_960p_30FPS */
	{2575, 1951, 776, 10, 744, 0x11, 0x11, 64, 0x8000, 0, 0, 0, 0x1F8E,
		0x1C, 4, 2 }
};

static struct mt9p031_control_info
	mt9p031_control_information[MT9P031_MAX_NO_CONTROLS] = {
	{
	 .register_address = MT9P031_GLOBAL_GAIN,
	 .query_control = {
			   .id = V4L2_CID_GAIN,
			   .name = "GAIN",
			   .type = V4L2_CTRL_TYPE_INTEGER,
			   .minimum = 0,
			   .maximum = 128,
			   .step = 1,
			   .default_value = 8}
	},
	{
	 .register_address = MT9P031_SHUTTER_WIDTH,
	 .query_control = {
			   .id = V4L2_CID_EXPOSURE,
			   .name = "EXPOSURE",
			   .type = V4L2_CTRL_TYPE_INTEGER,
			   .minimum = 0,
			   .maximum = 7000,
			   .step = 1,
			   .default_value = 400}
	 },
};

static struct mt9p031_config mt9p031_configuration[MT9P031_NUM_CHANNELS] = {
	{
		.no_of_inputs = MT9P031_MAX_NO_INPUTS,
		.input[0] = {
		      .input_type = MT9P031_RAW_INPUT,
		      .input_info = {
				.index = 0,
				.name = "RAW-1",
				.type = V4L2_INPUT_TYPE_CAMERA,
				.std = V4L2_STD_MT9P031_STD_ALL
			},
			.no_of_standard = MT9P031_MAX_NO_STANDARDS,
			.standard = (struct v4l2_standard *)&mt9p031_standards,
			.format = (struct mt9p031_format_params *)
				&mt9p031_format_parameters,
			.no_of_controls = MT9P031_MAX_NO_CONTROLS,
			.controls = (struct mt9p031_control_info *)
				&mt9p031_control_information
		}
	}
};

static struct mt9p031_channel mt9p031_channel_info[MT9P031_NUM_CHANNELS] = {
	{
		.params.inputidx = 0,
		.params.std = V4L2_STD_MT9P031_VGA_30FPS,
		.i2c_dev = {
			.i2c_addr = (0xBA >> 1),
			.i2c_registration = 0
		},
		.dec_device = NULL
	}
};

static struct device *mt9p031_i2c_dev[MT9P031_NUM_CHANNELS];

static struct param_ops params_ops = {
	.setparams = mt9p031_setparams,
	.getparams = mt9p031_getparams
};

static struct control_ops controls_ops = {
	.count = MT9P031_MAX_NO_CONTROLS,
	.queryctrl = mt9p031_querycontrol,
	.setcontrol = mt9p031_setcontrol,
	.getcontrol = mt9p031_getcontrol
};

static struct input_ops inputs_ops = {
	.count = MT9P031_MAX_NO_INPUTS,
	.enuminput = mt9p031_enuminput,
	.setinput = mt9p031_setinput,
	.getinput = mt9p031_getinput
};

static struct standard_ops standards_ops = {
	.count = MT9P031_MAX_NO_STANDARDS,
	.setstd = mt9p031_setstd,
	.getstd = mt9p031_getstd,
	.enumstd = mt9p031_enumstd,
	.querystd = mt9p031_querystd,
};

static struct decoder_device mt9p031_dev[MT9P031_NUM_CHANNELS] = {
	{
		.name = "MT9P031",
		.if_type = INTERFACE_TYPE_RAW,
		.channel_id = 0,
		.capabilities = 0,
		.initialize = mt9p031_initialize,
		.std_ops = &standards_ops,
		.ctrl_ops = &controls_ops,
		.input_ops = &inputs_ops,
		.fmt_ops = NULL,
		.params_ops = &params_ops,
		.deinitialize = mt9p031_deinitialize
	}
};

/*
 * This function called by vpif driver to initialize the decoder with default
 * values
 */
static int mt9p031_initialize(void *dec, int flag)
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
	ch_client = &mt9p031_channel_info[ch_id].i2c_dev.client;
	if (mt9p031_channel_info[ch_id].i2c_dev.i2c_registration & 0x01) {
		printk(KERN_ERR "MT9P031 driver is already initialized..\n");
		err = -EINVAL;
		return err;
	}

	/* Register MT9P031 I2C client */
	err |= i2c_add_driver(&mt9p031_channel_info[ch_id].i2c_dev.driver);
	if (err) {
		printk(KERN_ERR "Failed to register MT9P031 I2C client.\n");
		return -EINVAL;
	}

	mt9p031_channel_info[ch_id].i2c_dev.i2c_registration |= 0x1;
	mt9p031_channel_info[ch_id].dec_device = (struct decoder_device *)dec;
	dev_dbg(mt9p031_i2c_dev[ch_id], "Mt9p031 driver registered\n");
	if (DECODER_I2C_BIND_FLAG == flag) {
		/*
		 * check that decoder is set with default values once or not,
		 * if yes return, if not continue
		 */
		if (mt9p031_channel_info[ch_id].i2c_dev.i2c_registration & 0x02)
			return err;
	}
	dev_dbg(mt9p031_i2c_dev[ch_id], "Starting mt9p031_init....\n");

	if (mt9xxx_set_input_mux(0) < 0)
		dev_dbg(mt9p031_i2c_dev[ch_id], "Video Mux setting failed\n");

	/*
	 * Setup PLL to generate 96MHz master clock from ext clock
	 * assume ext clock is at 24MHz
	 */
	err |=
	    mt9p031_i2c_write_reg(ch_client, MT9P031_PLL_CTRL,
				  MT9P031_PLL_CTRL_POWER_UP);

	/* sleep for 10 msec */
	msleep(10);

	err |=
	    mt9p031_i2c_write_reg(ch_client, MT9P031_PLL_CFG1,
				  MT9P031_PLL_CFG1_DEFAULT);
	/* sleep for 10 msec */
	msleep(10);

	err |=
	    mt9p031_i2c_write_reg(ch_client, MT9P031_PLL_CFG2,
				  MT9P031_PLL_CFG2_DEFAULT);

	/* sleep for 10 msec */
	msleep(10);

	/* Use PLL clock */
	err |=
	    mt9p031_i2c_write_reg(ch_client, MT9P031_PLL_CTRL,
				  MT9P031_PLL_CTRL_USE_PLL_CLK);

	/* sleep for 10 msec */
	msleep(10);

	/* Configure the MT9P031 in normalpower up mode */
	err |=
	    mt9p031_i2c_write_reg(ch_client, MT9P031_ROW_START,
				  MT9P031_ROW_START_DEFAULT);
	err |=
	    mt9p031_i2c_write_reg(ch_client, MT9P031_COL_START,
				  MT9P031_COLUMN_START_DEFAULT);
	err |=
	    mt9p031_i2c_write_reg(ch_client, MT9P031_WIDTH,
				  MT9P031_COLUMN_SIZE_DEFAULT);
	err |=
	    mt9p031_i2c_write_reg(ch_client, MT9P031_HEIGHT,
				  MT9P031_ROW_SIZE_DEFAULT);
	err |=
	    mt9p031_i2c_write_reg(ch_client, MT9P031_HBLANK,
				  MT9P031_HOR_BLANK_DEFAULT);
	err |=
	    mt9p031_i2c_write_reg(ch_client, MT9P031_VBLANK,
				  MT9P031_VER_BLANK_DEFAULT);
	err |=
	    mt9p031_i2c_write_reg(ch_client, MT9P031_OUTPUT_CTRL,
				  MT9P031_OUTPUT_CTRL_DEFAULT);
	err |=
	    mt9p031_i2c_write_reg(ch_client, MT9P031_SHUTTER_WIDTH_UPPER,
				  MT9P031_SHUTTER_WIDTH_UPPER_DEFAULT);
	err |=
	    mt9p031_i2c_write_reg(ch_client, MT9P031_SHUTTER_WIDTH,
				  MT9P031_SHUTTER_WIDTH_DEFAULT);
	err |=
	    mt9p031_i2c_write_reg(ch_client, MT9P031_PIXEL_CLK_CTRL,
				  MT9P031_PIXEL_CLK_CTRL_DEFAULT);
	err |=
	    mt9p031_i2c_write_reg(ch_client, MT9P031_RESTART,
				  MT9P031_FRAME_RESTART_DEFAULT);
	err |=
	    mt9p031_i2c_write_reg(ch_client, MT9P031_SHUTTER_DELAY,
				  MT9P031_SHUTTER_DELAY_DEFAULT);
	err |=
	    mt9p031_i2c_write_reg(ch_client, MT9P031_READ_MODE1,
				  MT9P031_READ_MODE1_DEFAULT);
	err |=
	    mt9p031_i2c_write_reg(ch_client, MT9P031_READ_MODE2,
				  MT9P031_READ_MODE2_DEFAULT);
	err |=
	    mt9p031_i2c_write_reg(ch_client, MT9P031_ROW_ADDR_MODE,
				  MT9P031_ROW_ADDR_MODE_DEFAULT);
	err |=
	    mt9p031_i2c_write_reg(ch_client, MT9P031_COL_ADDR_MODE,
				  MT9P031_COL_ADDR_MODE_DEFAULT);
	err |=
	    mt9p031_i2c_write_reg(ch_client, MT9P031_GREEN1_GAIN,
				  MT9P031_GREEN1_GAIN_DEFAULT);
	err |=
	    mt9p031_i2c_write_reg(ch_client, MT9P031_BLUE_GAIN,
				  MT9P031_BLUE_GAIN_DEFAULT);
	err |=
	    mt9p031_i2c_write_reg(ch_client, MT9P031_RED_GAIN,
				  MT9P031_RED_GAIN_DEFAULT);
	err |=
	    mt9p031_i2c_write_reg(ch_client, MT9P031_GREEN2_GAIN,
				  MT9P031_GREEN2_GAIN_DEFAULT);
	err |=
	    mt9p031_i2c_write_reg(ch_client, MT9P031_GLOBAL_GAIN,
				  MT9P031_GLOBAL_GAIN_DEFAULT);
	err |=
	    mt9p031_i2c_write_reg(ch_client, MT9P031_ROW_BLK_TARGET,
				  MT9P031_ROW_BLK_TARGET_DEFAULT);
	err |=
	    mt9p031_i2c_write_reg(ch_client, MT9P031_BLC_DELTA_THRESHOLDS,
				  MT9P031_BLC_DELTA_THRESHOLDS_DEFAULT);
	err |=
	    mt9p031_i2c_write_reg(ch_client, MT9P031_BLC_TARGET_THRESHOLDS,
				  MT9P031_BLC_TARGET_THRESHOLDS_DEFAULT);
	err |=
	    mt9p031_i2c_write_reg(ch_client, MT9P031_GREEN1_OFFSET,
				  MT9P031_GREEN1_OFFSET_DEFAULT);
	err |=
	    mt9p031_i2c_write_reg(ch_client, MT9P031_GREEN2_OFFSET,
				  MT9P031_GREEN2_OFFSET_DEFAULT);
	err |=
	    mt9p031_i2c_write_reg(ch_client, MT9P031_RED_OFFSET,
				  MT9P031_RED_OFFSET_DEFAULT);
	err |=
	    mt9p031_i2c_write_reg(ch_client, MT9P031_BLUE_OFFSET,
				  MT9P031_BLUE_OFFSET_DEFAULT);
	err |=
	    mt9p031_i2c_write_reg(ch_client, MT9P031_BLK_LVL_CALIB,
				  MT9P031_BLK_LVL_CALIB_DEFAULT);
	err |=
	    mt9p031_i2c_write_reg(ch_client, MT9P031_TST_PAT_CTRL,
				  MT9P031_TST_PAT_CTRL_DEFAULT);

	err |=
	    mt9p031_i2c_write_reg(ch_client, MT9P031_TST_PAT_GR,
				  MT9P031_TST_PAT_GR_DEFAULT);
	err |=
	    mt9p031_i2c_write_reg(ch_client, MT9P031_TST_PAT_R,
				  MT9P031_TST_PAT_R_DEFAULT);
	err |=
	    mt9p031_i2c_write_reg(ch_client, MT9P031_TST_PAT_B,
				  MT9P031_TST_PAT_B_DEFAULT);
	err |=
	    mt9p031_i2c_write_reg(ch_client, MT9P031_TST_PAT_BAR_WIDTH,
				  MT9P031_TST_PAT_BAR_WIDTH_DEFAULT);

	/* sleep for 10 msec */
	msleep(10);
	/* Reset the sensor to be in default power up state */
	err |=
	    mt9p031_i2c_write_reg(ch_client, MT9P031_RESET,
				  MT9P031_RESET_ENABLE);
	/* sleep for 10 msec */
	msleep(10);
	/* Clear the bit to resume normal operation */
	err |=
	    mt9p031_i2c_write_reg(ch_client, MT9P031_RESET,
				  MT9P031_RESET_DISABLE);

	/*
	 * delay applying changes
	 * sleep for 10 msec
	 */
	msleep(10);
	err |=
	    mt9p031_i2c_write_reg(ch_client, MT9P031_OUTPUT_CTRL,
				  MT9P031_N0_CHANGE_MODE);
	/* sleep for 10 msec */
	msleep(10);
	err |=
	    mt9p031_i2c_write_reg(ch_client, MT9P031_PIXEL_CLK_CTRL,
				  MT9P031_PIX_CLK_RISING);
	/* sleep for 10 msec */
	msleep(10);

	/* Configure the MT9P031 in normalpower up mode */
	err |=
	    mt9p031_i2c_write_reg(ch_client, MT9P031_OUTPUT_CTRL,
				  MT9P031_NORMAL_OPERATION_MODE);
	/* sleep for 10 msec */
	msleep(10);
	/* call set standard */
	std = mt9p031_channel_info[ch_id].params.std;
	err |= mt9p031_setstd(&std, dec);

	/* call query standard */
	err |= mt9p031_querystd(&std, dec);

	/* call get input */
	err |= mt9p031_getinput(&index, dec);

	/* Configure for default video standard */
	if (err < 0) {
		err = -EINVAL;
		mt9p031_deinitialize(dec);
		return err;
	}
	mt9p031_channel_info[ch_id].i2c_dev.i2c_registration |= 0x2;
	dev_dbg(mt9p031_i2c_dev[ch_id], "End of mt9p031_init.\n");
	return err;
}

/*
 * This function called by vpif driver to de-initialize the decoder
 */
static int mt9p031_deinitialize(void *dec)
{
	int ch_id;

	if (NULL == dec) {
		printk(KERN_ERR "NULL Pointer\n");
		return -EINVAL;
	}
	ch_id = ((struct decoder_device *)dec)->channel_id;
	if (mt9p031_channel_info[ch_id].i2c_dev.i2c_registration & 0x01) {
		i2c_del_driver(&mt9p031_channel_info[ch_id].i2c_dev.driver);
		mt9p031_channel_info[ch_id].i2c_dev.client.adapter = NULL;
		mt9p031_channel_info[ch_id].i2c_dev.i2c_registration &= ~(0x01);
		mt9p031_channel_info[ch_id].dec_device = NULL;
	}
	return 0;
}

/* Function to set the control parameters */
static int mt9p031_setcontrol(struct v4l2_control *ctrl, void *dec)
{
	int i = 0;
	int err = 0;
	int ch_id;
	int value;
	int input_idx;
	struct mt9p031_control_info *control = NULL;

	if (NULL == dec) {
		printk(KERN_ERR "NULL Pointer\n");
		return -EINVAL;
	}
	ch_id = ((struct decoder_device *)dec)->channel_id;
	dev_dbg(mt9p031_i2c_dev[ch_id], "Mt9p031 set control started.\n");
	input_idx = mt9p031_channel_info[ch_id].params.inputidx;

	/* check for null pointer */
	if (ctrl == NULL) {
		dev_err(mt9p031_i2c_dev[ch_id], "NULL pointer.\n");
		return -EINVAL;
	}
	value = (unsigned short)ctrl->value;
	for (i = 0; i < mt9p031_configuration[ch_id].input[input_idx].
	     no_of_controls; i++) {
		control = &mt9p031_configuration[ch_id].input[input_idx].
		    controls[i];
		if ((control->query_control).id == ctrl->id)
			break;
	}
	if (i == mt9p031_configuration[ch_id].input[input_idx].no_of_controls)
		return -EINVAL;
	if (((control->query_control).minimum > value)
	    || ((control->query_control).maximum < value))
		return -EINVAL;
	err = mt9p031_i2c_write_reg(&mt9p031_channel_info[ch_id].i2c_dev.
				   client, control->register_address, value);
	if (err < 0) {
		dev_err(mt9p031_i2c_dev[ch_id],
			"MT9P031 set control fails...\n");
		return err;
	}
	dev_dbg(mt9p031_i2c_dev[ch_id], "Mt9p031 set control done\n");
	return err;
}

/* Function to get the control parameter */
static int mt9p031_getcontrol(struct v4l2_control *ctrl, void *dec)
{
	int input_idx;
	struct mt9p031_control_info *control = NULL;
	int ch_id;
	int i = 0;
	int err = 0;

	if (NULL == dec) {
		printk(KERN_ERR "NULL Pointer\n");
		return -EINVAL;
	}
	ch_id = ((struct decoder_device *)dec)->channel_id;
	dev_dbg(mt9p031_i2c_dev[ch_id], "Mt9p031 get control started.\n");
	input_idx = mt9p031_channel_info[ch_id].params.inputidx;

	/* check for null pointer */
	if (ctrl == NULL) {
		dev_err(mt9p031_i2c_dev[ch_id], "NULL pointer\n");
		return -EINVAL;
	}
	for (i = 0; i < mt9p031_configuration[ch_id].input[input_idx].
		 no_of_controls; i++) {
		control = &mt9p031_configuration[ch_id].input[input_idx].
			  controls[i];
		if ((control->query_control).id == ctrl->id)
			break;
	}
	if (i == mt9p031_configuration[ch_id].input[input_idx].no_of_controls) {
		dev_err(mt9p031_i2c_dev[ch_id],
			"mt9p031_getcontrol Invalid id...\n");
		return -EINVAL;
	}
	err = mt9p031_i2c_read_reg(&mt9p031_channel_info[ch_id].i2c_dev.
				   client, control->register_address,
				   (unsigned short *)&(ctrl->value));
	if (err < 0) {
		dev_err(mt9p031_i2c_dev[ch_id],
		       "MT9P031 get control fails...\n");
		return err;
	}
	dev_dbg(mt9p031_i2c_dev[ch_id], "Mt9p031 get control done.\n");
	return err;
}

/* Function to query control parameter */
static int mt9p031_querycontrol(struct v4l2_queryctrl *ctrl, void *dec)
{
	int err = 0;
	int id;
	int ch_id;
	int i = 0;
	int input_idx;
	struct mt9p031_control_info *control = NULL;

	if (NULL == dec) {
		printk(KERN_ERR "NULL Pointer\n");
		return -EINVAL;
	}

	ch_id = ((struct decoder_device *)dec)->channel_id;
	dev_dbg(mt9p031_i2c_dev[ch_id], "Starting mt9p031_queryctrl...\n");
	input_idx = mt9p031_channel_info[ch_id].params.inputidx;
	if (NULL == ctrl) {
		dev_err(mt9p031_i2c_dev[ch_id], "NULL Pointer\n");
		return -EINVAL;
	}

	id = ctrl->id;
	memset(ctrl, 0, sizeof(struct v4l2_queryctrl));
	ctrl->id = id;
	for (i = 0; i < mt9p031_configuration[ch_id].input[input_idx].
		no_of_controls; i++) {
		control = &mt9p031_configuration[ch_id].input[input_idx].
			  controls[i];
		if ((control->query_control).id == ctrl->id)
			break;
	}

	if (i == mt9p031_configuration[ch_id].input[input_idx].no_of_controls) {
		dev_err(mt9p031_i2c_dev[ch_id], "Invalid id...\n");
		return -EINVAL;
	}
	memcpy(ctrl, &control->query_control, sizeof(struct v4l2_queryctrl));
	dev_dbg(mt9p031_i2c_dev[ch_id], "End of mt9p031_queryctrl...\n");
	return err;
}

/* Function to set the video standard */
static int mt9p031_setstd(v4l2_std_id *id, void *dec)
{
	int err = 0, ch_id, i = 0;
	int input_idx;
	struct v4l2_standard *standard;
	struct mt9p031_format_params *formats;

	if (NULL == dec) {
		printk(KERN_ERR "NULL Pointer\n");
		return -EINVAL;
	}

	ch_id = ((struct decoder_device *)dec)->channel_id;
	dev_dbg(mt9p031_i2c_dev[ch_id], "Mt9p031 set standard started...\n");
	input_idx = mt9p031_channel_info[ch_id].params.inputidx;
	if (id == NULL) {
		dev_err(mt9p031_i2c_dev[ch_id], "NULL Pointer\n");
		return -EINVAL;
	}

	for (i = 0; i < mt9p031_configuration[ch_id].input[input_idx].
		no_of_standard; i++) {
		standard = &mt9p031_configuration[ch_id].input[input_idx].
			   standard[i];
		if (standard->id == *id)
			break;
	}

	if (i == mt9p031_configuration[ch_id].input[input_idx].no_of_standard) {
		dev_err(mt9p031_i2c_dev[ch_id],
			"mt9p031_setstd:Invalid id...\n");
		return -EINVAL;
	}

	formats = &mt9p031_configuration[ch_id].input[input_idx].format[i];
	err = mt9p031_set_format_params(formats, dec);
	if (err < 0) {
		dev_err(mt9p031_i2c_dev[ch_id], "Set standard failed.\n");
		return err;
	}

	mt9p031_channel_info[ch_id].params.std = *id;
	dev_dbg(mt9p031_i2c_dev[ch_id], "End of mt9p031 set standard...\n");
	return err;
}

/* Function to get the video standard */
static int mt9p031_getstd(v4l2_std_id *id, void *dec)
{
	int err = 0;
	int ch_id;

	if (NULL == dec) {
		printk(KERN_ERR "NULL Pointer\n");
		return -EINVAL;
	}
	ch_id = ((struct decoder_device *)dec)->channel_id;
	dev_dbg(mt9p031_i2c_dev[ch_id], "Mt9p031 get standard started...\n");

	if (id == NULL) {
		dev_err(mt9p031_i2c_dev[ch_id], "NULL Pointer\n");
		return -EINVAL;
	}

	*id = mt9p031_channel_info[ch_id].params.std;
	dev_dbg(mt9p031_i2c_dev[ch_id], "End of getstd function.\n");
	return err;
}

/* Function querystd not supported by mt9p031 */
static int mt9p031_querystd(v4l2_std_id *id, void *dec)
{
	int err = 0;
	int ch_id;

	if (NULL == dec) {
		printk(KERN_ERR "NULL Pointer\n");
		return -EINVAL;
	}
	ch_id = ((struct decoder_device *)dec)->channel_id;
	dev_dbg(mt9p031_i2c_dev[ch_id],
		"MT9P031 does not support this ioctl...\n");

	if (NULL == id) {
		dev_err(mt9p031_i2c_dev[ch_id], "NULL Pointer\n");
		return -EINVAL;
	}

	*id = V4L2_STD_MT9P031_STD_ALL;
	return err;
}

/* Function to enumerate standards supported*/
static int mt9p031_enumstd(struct v4l2_standard *std, void *dec)
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
		input_idx < mt9p031_configuration[ch_id].no_of_inputs;
		input_idx++) {
		sumstd += mt9p031_configuration[ch_id].input[input_idx]
			  .no_of_standard;
		if (index < sumstd) {
			sumstd -= mt9p031_configuration[ch_id]
				  .input[input_idx].no_of_standard;
			break;
		}
	}

	if (input_idx == mt9p031_configuration[ch_id].no_of_inputs)
		return -EINVAL;

	index -= sumstd;
	memset(std, 0, sizeof(*std));
	memcpy(std, &mt9p031_configuration[ch_id].input[input_idx].
		standard[index], sizeof(struct v4l2_standard));
	std->index = index1;
	return err;
}

/* Function to set the input */
static int mt9p031_setinput(int *index, void *dec)
{
	int err = 0;
	int ch_id;

	if (NULL == dec) {
		printk(KERN_ERR "NULL Pointer\n");
		return -EINVAL;
	}

	ch_id = ((struct decoder_device *)dec)->channel_id;
	dev_dbg(mt9p031_i2c_dev[ch_id], "Start of set input function.\n");

	/* check for null pointer */
	if (index == NULL) {
		dev_err(mt9p031_i2c_dev[ch_id], "NULL Pointer.\n");
		return -EINVAL;
	}

	if ((*index >= mt9p031_configuration[ch_id].no_of_inputs)
		|| (*index < 0))
		return -EINVAL;

	mt9p031_channel_info[ch_id].params.inputidx = *index;
	dev_dbg(mt9p031_i2c_dev[ch_id], "End of set input function.\n");
	return err;
}

/* Function to get the input*/
static int mt9p031_getinput(int *index, void *dec)
{
	int err = 0;
	int ch_id;

	if (NULL == dec) {
		printk(KERN_ERR "NULL Pointer.\n");
		return -EINVAL;
	}
	ch_id = ((struct decoder_device *)dec)->channel_id;
	dev_dbg(mt9p031_i2c_dev[ch_id], "Start of get input function.\n");

	/* check for null pointer */
	if (index == NULL) {
		dev_err(mt9p031_i2c_dev[ch_id], "NULL Pointer.\n");
		return -EINVAL;
	}
	*index = mt9p031_channel_info[ch_id].params.inputidx;

	/* Call setinput function to set the input */
	err = mt9p031_setinput(index, dec);
	dev_dbg(mt9p031_i2c_dev[ch_id], "End of get input function.\n");
	return err;
}

/* Function to enumerate the input */
static int mt9p031_enuminput(struct v4l2_input *input, void *dec)
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
		printk(KERN_ERR "MT9P031: NULL Pointer.\n");
		return -EINVAL;
	}

	/* Only one input is available */
	if (input->index >= mt9p031_configuration[ch_id].no_of_inputs)
		return -EINVAL;

	index = input->index;
	memset(input, 0, sizeof(*input));
	input->index = index;
	memcpy(input, &mt9p031_configuration[ch_id].input[index].input_info,
			sizeof(struct v4l2_input));
	return err;
}

/* Function to set the format parameters*/
static int mt9p031_set_format_params(struct mt9p031_format_params
				     *formats, void *dec)
{
	int err = 0;
	int ch_id;
	unsigned short val;
	struct i2c_client *ch_client;
	unsigned short mt9p031_output_ctrl;

	if (NULL == dec) {
		printk(KERN_ERR "NULL Pointer\n");
		return -EINVAL;
	}

	ch_id = ((struct decoder_device *)dec)->channel_id;
	ch_client = &mt9p031_channel_info[ch_id].i2c_dev.client;
	dev_dbg(mt9p031_i2c_dev[ch_id], "Mt9p031 set format params started.\n");
	if (formats == NULL) {
		dev_err(mt9p031_i2c_dev[ch_id], "NULL Pointer.\n");
		return -EINVAL;
	}

	/*
	 * Write the height width and blanking information required
	 * for particular format
	 */
	err = mt9p031_i2c_write_reg(ch_client, MT9P031_HEIGHT,
				    formats->row_size);

	if (err < 0) {
		dev_err(mt9p031_i2c_dev[ch_id], "I2C write fails...height.\n");
		return err;
	}
	err = mt9p031_i2c_write_reg(ch_client, MT9P031_WIDTH,
				    formats->col_size);
	if (err < 0) {
		dev_err(mt9p031_i2c_dev[ch_id], "I2C write fails...width\n");
		return err;
	}

	/* Configure for default video standard */
	err = mt9p031_i2c_write_reg(ch_client, MT9P031_HBLANK,
				    formats->h_blank);
	if (err < 0) {
		dev_err(mt9p031_i2c_dev[ch_id], "I2C write fails...hblk\n");
		return err;
	}
	err = mt9p031_i2c_write_reg(ch_client, MT9P031_VBLANK,
				    formats->v_blank);
	if (err < 0) {
		dev_err(mt9p031_i2c_dev[ch_id], "I2C write fails...vblk\n");
		return err;
	}
	err = mt9p031_i2c_write_reg(ch_client, MT9P031_SHUTTER_WIDTH,
				    (unsigned short)(formats->
				     shutter_width &
				     MT9P031_SHUTTER_WIDTH_LOWER_MASK));
	if (err < 0) {
		dev_err(mt9p031_i2c_dev[ch_id],
			"I2C write fails...shutterwidth.\n");
		return err;
	}
	err = mt9p031_i2c_write_reg(ch_client, MT9P031_SHUTTER_WIDTH_UPPER,
				    (unsigned short)(formats->
				     shutter_width >>
				     MT9P031_SHUTTER_WIDTH_UPPER_SHIFT));
	if (err < 0) {
		dev_err(mt9p031_i2c_dev[ch_id], "I2C write fails...upper.\n");
		return err;
	}
	err = mt9p031_i2c_write_reg(ch_client, MT9P031_ROW_ADDR_MODE,
				    formats->row_addr_mode);
	if (err < 0) {
		dev_err(mt9p031_i2c_dev[ch_id],
			"I2C write fails...addrmoderow.\n");
		return err;
	}
	err = mt9p031_i2c_write_reg(ch_client, MT9P031_COL_ADDR_MODE,
				    formats->col_addr_mode);
	if (err < 0) {
		dev_err(mt9p031_i2c_dev[ch_id],
			"I2C write fails...addrmodecol.\n");
		return err;
	}
	err = mt9p031_i2c_write_reg(ch_client, MT9P031_ROW_BLK_DEF_OFFSET,
				    formats->black_level);
	if (err < 0) {
		dev_err(mt9p031_i2c_dev[ch_id],
			"I2C write fails...black_level.\n");
		return err;
	}
	err = mt9p031_i2c_write_reg(ch_client, MT9P031_ROW_START,
				    formats->row_start);
	if (err < 0) {
		dev_err(mt9p031_i2c_dev[ch_id],
			"I2C write fails...rowstart.\n");
		return err;
	}
	err = mt9p031_i2c_write_reg(ch_client, MT9P031_COL_START,
				    formats->col_start);
	if (err < 0) {
		dev_err(mt9p031_i2c_dev[ch_id], "I2C write fails...colstart\n");
		return err;
	}
	err = mt9p031_i2c_write_reg(ch_client, MT9P031_PIXEL_CLK_CTRL,
				    formats->pixel_clk_control);
	if (err < 0) {
		dev_err(mt9p031_i2c_dev[ch_id], "I2C write fails...clkctrl\n");
		return err;
	}

	err = mt9p031_i2c_write_reg(ch_client, MT9P031_PLL_CTRL,
			    MT9P031_PLL_USE_EXTERNAL);

	if (err < 0) {
		printk(KERN_ERR "\n I2C write fails...pll external");
		return err;
	}
	/* sleep for 10 msec */
	msleep(10);

	val = (formats->pll_m << MT9P031_PLL_M_SHIFT) |
		((formats->pll_n - 1) & MT9P031_PLL_N_MASK);
	err = mt9p031_i2c_write_reg(ch_client, MT9P031_PLL_CFG1,
			    val);

	if (err < 0) {
		printk(KERN_ERR "\n I2C write fails...pll change");
		return err;
	}
	/* sleep for 10 msec */
	msleep(10);

	err = mt9p031_i2c_write_reg(ch_client, MT9P031_PLL_CFG2,
			    (formats->pll_p1 - 1) & MT9P031_PLL_P1_MASK);

	if (err < 0) {
		printk(KERN_ERR "\n I2C write fails...pll change");
		return err;
	}
	/* sleep for 10 msec */
	msleep(10);

	err = mt9p031_i2c_write_reg(ch_client, MT9P031_PLL_CTRL,
			    MT9P031_PLL_ENABLE);

	if (err < 0) {
		printk(KERN_ERR "\n I2C write fails...pll enable");
		return err;
	}
	/* sleep for 10 msec */
	msleep(10);

	/* applying changes  */
	mt9p031_output_ctrl = (formats->output_config) &
				MT9P031_OUTPUT_CTRL_MASK;
	err = mt9p031_i2c_write_reg(ch_client, MT9P031_OUTPUT_CTRL,
				    mt9p031_output_ctrl);
	if (err < 0) {
		dev_err(mt9p031_i2c_dev[ch_id], "I2C write fails.outputctrl\n");
		return err;
	}

	dev_dbg(mt9p031_i2c_dev[ch_id], "End of mt9p031 set format params.\n");
	return err;
}

/* This function will configure MT9P031 for bayer pattern capture */
static int mt9p031_setparams(void *params, void *dec)
{
	int err = 0;
	unsigned short val;
	int ch_id;
	struct mt9p031_params *mt9t_params = (struct mt9p031_params *)params;

	if (NULL == dec) {
		printk(KERN_ERR "NULL Pointer\n");
		return -EINVAL;
	}
	ch_id = ((struct decoder_device *)dec)->channel_id;
	dev_dbg(mt9p031_i2c_dev[ch_id],
		"Start of mt9p031 set params function.\n");

	/* check for null pointer */
	if (mt9t_params == NULL) {
		dev_err(mt9p031_i2c_dev[ch_id], "Null pointer.\n");
		return -EINVAL;
	}
	err |= mt9p031_setinput(&(mt9t_params->inputidx), dec);
	if (err < 0) {
		dev_dbg(mt9p031_i2c_dev[ch_id],
			"Set format parameters failed.\n");
		return err;
	}
	err |= mt9p031_setstd(&(mt9t_params->std), dec);
	if (err < 0) {
		dev_dbg(mt9p031_i2c_dev[ch_id],
			"Set format parameters failed.\n");
		return err;
	}

	/* set video format related parameters */
	err = mt9p031_set_format_params(&mt9t_params->format, dec);
	if (err < 0) {
		dev_err(mt9p031_i2c_dev[ch_id],
			"Set format parameters failed.\n");
		return err;
	}

	/* Write the gain information */
	val = (unsigned short)(mt9t_params->rgb_gain.
			       gr1_analog_gain & MT9P031_ANALOG_GAIN_MASK);
	val |= ((mt9t_params->rgb_gain.gr1_digital_gain &
		 MT9P031_DIGITAL_GAIN_MASK) << MT9P031_DIGITAL_GAIN_SHIFT);
	val |= ((mt9t_params->rgb_gain.gr1_analog_mult &
		 MT9P031_ANALOG_MULT_MASK) << MT9P031_ANALOG_MULT_SHIFT);

	err = mt9p031_i2c_write_reg(&mt9p031_channel_info[ch_id].i2c_dev.
				    client, MT9P031_GREEN1_GAIN, val);
	if (err < 0) {
		dev_err(mt9p031_i2c_dev[ch_id], "I2C write fails....\n");
		return err;
	}

	/* Write the gain information */
	val = (unsigned short)(mt9t_params->rgb_gain.
			       b_analog_gain & MT9P031_ANALOG_GAIN_MASK);
	val |= ((mt9t_params->rgb_gain.b_digital_gain &
		 MT9P031_DIGITAL_GAIN_MASK) << MT9P031_DIGITAL_GAIN_SHIFT);
	val |= ((mt9t_params->rgb_gain.b_analog_mult &
		 MT9P031_ANALOG_MULT_MASK) << MT9P031_ANALOG_MULT_SHIFT);

	err = mt9p031_i2c_write_reg(&mt9p031_channel_info[ch_id].i2c_dev.
				    client, MT9P031_BLUE_GAIN, val);
	if (err < 0) {
		dev_err(mt9p031_i2c_dev[ch_id], "I2C write fails....\n");
		return err;
	}

	/* Write the gain information */
	val = (unsigned short)(mt9t_params->rgb_gain.
			       r_analog_gain & MT9P031_ANALOG_GAIN_MASK);
	val |= ((mt9t_params->rgb_gain.r_digital_gain &
		 MT9P031_DIGITAL_GAIN_MASK) << MT9P031_DIGITAL_GAIN_SHIFT);
	val |= ((mt9t_params->rgb_gain.r_analog_mult &
		 MT9P031_ANALOG_MULT_MASK) << MT9P031_ANALOG_MULT_SHIFT);

	err = mt9p031_i2c_write_reg(&mt9p031_channel_info[ch_id].i2c_dev.
				    client, MT9P031_RED_GAIN, val);
	if (err < 0) {
		dev_err(mt9p031_i2c_dev[ch_id], "I2C write fails....\n");
		return err;
	}

	/* Write the gain information */
	val = (unsigned short)(mt9t_params->rgb_gain.
			       gr2_analog_gain & MT9P031_ANALOG_GAIN_MASK);
	val |= ((mt9t_params->rgb_gain.gr2_digital_gain &
		 MT9P031_DIGITAL_GAIN_MASK) << MT9P031_DIGITAL_GAIN_SHIFT);
	val |= ((mt9t_params->rgb_gain.gr2_analog_mult &
		 MT9P031_ANALOG_MULT_MASK) << MT9P031_ANALOG_MULT_SHIFT);

	err = mt9p031_i2c_write_reg(&mt9p031_channel_info[ch_id].i2c_dev.
				    client, MT9P031_GREEN2_GAIN, val);
	if (err < 0) {
		dev_err(mt9p031_i2c_dev[ch_id], "I2C write fails....\n");
		return err;
	}

	val = 0;
	err = mt9p031_i2c_write_reg(&mt9p031_channel_info[ch_id].i2c_dev.
				    client, MT9P031_TST_PAT_CTRL, val);
	if (mt9t_params->test_pat.en) {
		/* enable pattern control */
		val = 1;
		/* set mode */
		val |= mt9t_params->test_pat.mode;
		err |=
		    mt9p031_i2c_write_reg(&mt9p031_channel_info[ch_id].i2c_dev.
					  client, MT9P031_TST_PAT_CTRL, val);
		val = mt9t_params->test_pat.val_gr & MT9P031_TST_PAT_VAL_MASK;
		err |=
		    mt9p031_i2c_write_reg(&mt9p031_channel_info[ch_id].i2c_dev.
					  client, MT9P031_TST_PAT_GR, val);
		val = mt9t_params->test_pat.val_r & MT9P031_TST_PAT_VAL_MASK;
		err |=
		    mt9p031_i2c_write_reg(&mt9p031_channel_info[ch_id].i2c_dev.
					  client, MT9P031_TST_PAT_R, val);
		val = mt9t_params->test_pat.val_b & MT9P031_TST_PAT_VAL_MASK;
		err |=
		    mt9p031_i2c_write_reg(&mt9p031_channel_info[ch_id].i2c_dev.
					  client, MT9P031_TST_PAT_B, val);
		val =
		    mt9t_params->test_pat.
		    bar_width & MT9P031_TST_PAT_BAR_WIDTH_MASK;
		err |=
		    mt9p031_i2c_write_reg(&mt9p031_channel_info[ch_id].i2c_dev.
					  client, MT9P031_TST_PAT_BAR_WIDTH,
					  val);
	}
	if (err < 0) {
		dev_err(mt9p031_i2c_dev[ch_id], "I2C write fails....\n");
		return err;
	}

	err = mt9p031_i2c_read_reg(&mt9p031_channel_info[ch_id].i2c_dev.
				   client, MT9P031_READ_MODE2, &val);
	if (err < 0) {
		dev_err(mt9p031_i2c_dev[ch_id], "READ_MODE2 read failed....\n");
		return err;
	}
	val |= ((mt9t_params->misc_cfg.mirror_row & MT9P031_ONE_BIT_MASK)
		<< MT9P031_MIRROR_ROW_SHIFT);
	val |= ((mt9t_params->misc_cfg.mirror_row & MT9P031_ONE_BIT_MASK)
		<< MT9P031_MIRROR_COL_SHIFT);
	val |= ((mt9t_params->misc_cfg.show_dk_rows & MT9P031_ONE_BIT_MASK)
		<< MT9P031_SHOW_DK_ROWS_SHIFT);
	val |= ((mt9t_params->misc_cfg.show_dk_cols & MT9P031_ONE_BIT_MASK)
		<< MT9P031_SHOW_DK_COLS_SHIFT);
	err |= mt9p031_i2c_write_reg(&mt9p031_channel_info[ch_id].i2c_dev.
				     client, MT9P031_READ_MODE2, val);

	mt9p031_channel_info[ch_id].params = *mt9t_params;
	dev_dbg(mt9p031_i2c_dev[ch_id], "End of mt9p031 set params...\n");
	return err;
}

/* This function will get MT9P031 configuration values.*/
static int mt9p031_getparams(void *params, void *dec)
{
	int err = 0;
	int ch_id;
	struct mt9p031_params *mt9t_params = (struct mt9p031_params *)params;

	if (NULL == dec) {
		printk(KERN_ERR "NULL Pointer\n");
		return -EINVAL;
	}
	ch_id = ((struct decoder_device *)dec)->channel_id;
	dev_dbg(mt9p031_i2c_dev[ch_id], "Starting mt9p031_getparams.\n");

	/* check for null pointer */
	if (mt9t_params == NULL) {
		dev_err(mt9p031_i2c_dev[ch_id], "Null pointer.\n");
		return -EINVAL;
	}
	*mt9t_params = mt9p031_channel_info[ch_id].params;
	dev_dbg(mt9p031_i2c_dev[ch_id], "End of mt9p031 getparams...\n");
	return err;
}

/* This function is used to read value from register for i2c client */
static int mt9p031_i2c_read_reg(struct i2c_client *client,
				unsigned char reg, unsigned short *val)
{
	int err = 0;
	struct i2c_msg msg[1];
	unsigned char data[2];

	if (!client->adapter)
		err = -ENODEV;
	else {
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
			if (err >= 0) {
				*val = ((data[0] & 0x00FF) << 8)
				    | (data[1] & 0x00FF);
			}
		}
	}
	return err;
}

/* This function is used to write value into register for i2c client */
static int mt9p031_i2c_write_reg(struct i2c_client *client,
				 unsigned char reg, unsigned short val)
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
		else {
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
				    mt9p031_i2c_read_reg(client, reg, &readval);
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
	return err;
}

/* This function is used to attach i2c client */
static int mt9p031_i2c_attach_client(struct i2c_client *client,
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
static int mt9p031_i2c_detach_client(struct i2c_client *client)
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
static int mt9p031_i2c_probe_adapter(struct i2c_adapter *adap)
{
	int err = 0;
	mt9p031_i2c_dev[0] = &(adap->dev);
	dev_dbg(mt9p031_i2c_dev[0], "Mt9t031 i2c probe adapter called...\n");
	err = mt9p031_i2c_attach_client(&mt9p031_channel_info[0].i2c_dev.
					client,
					&mt9p031_channel_info[0].i2c_dev.
					driver, adap,
					mt9p031_channel_info[0].i2c_dev.
					i2c_addr);
	dev_dbg(mt9p031_i2c_dev[0], "Mt9p031 i2c probe adapter ends...\n");
	return err;
}

/* This function is used initialize mt9p031 i2c client */
static int mt9p031_i2c_init(void)
{
	int err = 0;
	int i = 0;
	int j;
	struct i2c_driver *driver;
	static char strings[MT9P031_NUM_CHANNELS][80] =
	    { "MT9P031 channel0 Video Decoder I2C driver",
	};

	for (i = 0; i < MT9P031_NUM_CHANNELS; i++) {
		driver = &mt9p031_channel_info[i].i2c_dev.driver;
		driver->driver.name = strings[i];
		driver->id = I2C_DRIVERID_MISC;
		if (0 == i)
			driver->attach_adapter = mt9p031_i2c_probe_adapter;
		driver->detach_client = mt9p031_i2c_detach_client;
		err = vpif_register_decoder(&mt9p031_dev[i]);
		if (err < 0) {
			for (j = i - 1; j > 0; j--)
				vpif_unregister_decoder(&mt9p031_dev[j]);
			return err;
		}
	}
	return err;
}

/* This function is used detach mt9p031 i2c client */
static void mt9p031_i2c_cleanup(void)
{
	int i;

	for (i = 0; i < MT9P031_NUM_CHANNELS; i++) {
		if (mt9p031_channel_info[i].i2c_dev.i2c_registration & 0x01) {
			i2c_del_driver(&mt9p031_channel_info[i].i2c_dev.driver);
			mt9p031_channel_info[i].i2c_dev.client.adapter = NULL;
			mt9p031_channel_info[i].i2c_dev.i2c_registration = 0;
		}
		vpif_unregister_decoder(&mt9p031_dev[i]);
	}
}

module_init(mt9p031_i2c_init);
module_exit(mt9p031_i2c_cleanup);
MODULE_LICENSE("GPL");

/**************************************************************************/
/* End of file                                                        */
/**************************************************************************/
