/*
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
#include <linux/kernel.h>
#include <linux/config.h>
#include <linux/init.h>
#include <linux/ctype.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/device.h>
#include <asm/arch/video_evm.h>
#include <asm/arch/i2c-client.h>
#include <asm/arch/hardware.h>
#include <asm/arch/mux.h>
#include <asm/arch/gpio.h>
#include <asm/arch/cpu.h>
#include <asm/arch/cpld.h>
#include <linux/io.h>

#define MSP430_I2C_ADDR		(0x25)
#define PCA9543A_I2C_ADDR	(0x73)

static struct tvp514x_evm_config dmxxx_tvp514x_evm_chan_config = {
	.num_channels = 1,
	.channels[0] = {
		.i2c_addr = (0xBA >> 1),
		.num_inputs = 2,
		.input[0] = {
			.name = "COMPOSITE",
			.input_sel = 0x05,
			.lock_mask = 0x0E,
		},
		.input[1] = {
			.name = "SVIDEO",
			.input_sel = 0x46,
			.lock_mask = 0x06,
		},
	},
};

static struct tvp514x_evm_config dm365_tvp514x_evm_chan_config = {
	.num_channels = 1,
	.channels[0] = {
		.i2c_addr = 0x5D,
		.num_inputs = 2,
		.input[0] = {
			.name = "COMPOSITE",
			.input_sel = 0x05,
			.lock_mask = 0x0E,
		},
		.input[1] = {
			.name = "SVIDEO",
			.input_sel = 0x46,
			.lock_mask = 0x06,
		},
	},
};

struct tvp514x_evm_chan *tvp514x_get_evm_chan_config(unsigned char channel)
{
	struct tvp514x_evm_chan *chan_cfg = NULL;
	if (channel == 0) {
		if (cpu_is_davinci_dm365())
			chan_cfg =
			  &dm365_tvp514x_evm_chan_config.channels[channel];
		else
			chan_cfg =
			  &dmxxx_tvp514x_evm_chan_config.channels[channel];
	}
	return chan_cfg;
}
EXPORT_SYMBOL(tvp514x_get_evm_chan_config);


/* For DM365, we need set tvp5146 reset line low for
 * working with sensor. This API is used for this
 * purpose state is 0 for reset low and 1 for high
 */
void tvp514x_hw_reset(int state)
{
	u32 val1;
	if (cpu_is_davinci_dm365()) {
		val1 = cpld_read(DM365_CPLD_REGISTER19);
		if (state)
			val1 &= ~DM365_TVP5146_RST_MASK;
		else
			val1 |= 0x01010101;
		cpld_write(val1, DM365_CPLD_REGISTER19);
	}
}

/* Om DM365, Imager and TVP5146 I2C address collide. So we need to
 * keep imager on reset when working with tvp5146 and vice-versa
 * This function use GIO40 to act as reset to imager
 */
void image_sensor_hw_reset(int state)
{
	u32 val1;
	if (cpu_is_davinci_dm365()) {
		val1 = cpld_read(DM365_CPLD_REGISTER3);
		val1 |= DM365_IMAGER_RST_MASK;
		cpld_write(val1, DM365_CPLD_REGISTER3);
		/* CPLD to Route GPIO to Imager Reset line */
		val1 = cpld_read(DM365_CPLD_REGISTER16);
		val1 &= ~0x40404040;
		cpld_write(val1, DM365_CPLD_REGISTER16);
		val1 = cpld_read(DM365_CPLD_REGISTER18);
		val1 |= 0x20202020;
		cpld_write(val1, DM365_CPLD_REGISTER18);
		val1 = cpld_read(DM365_CPLD_REGISTER18);
		val1 &= ~0x01010101;
		cpld_write(val1, DM365_CPLD_REGISTER18);
                
#ifndef CONFIG_MTD_CFI /* When NOR flash is use and at 16-bit access mode. GPIO 40 is used as address bus */ 
		/* Pin mux to use GPIO40 */
		davinci_cfg_reg(DM365_GPIO40, PINMUX_RESV);

		/* Configure GPIO40 to be output and hight */
		if (state)
			gpio_direction_output(40, 1);
		else
			gpio_direction_output(40, 0);
#endif
	}
}

/* type = 0 for TVP5146 and 1 for Image sensor */
int tvp514x_set_input_mux(unsigned char channel)
{
	int err = 0;
	u8 val[2];
	u32 val1;
	val[0] = 8;
	val[1] = 0x0;

	if (cpu_is_davinci_dm644x())
		return err;

/* Logi	if (cpu_is_davinci_dm355())
		err = davinci_i2c_write(2, val, MSP430_I2C_ADDR);
	else */if (cpu_is_davinci_dm365()) {
		/* drive the sensor reset line low */
		image_sensor_hw_reset(0);
		/* set tvp5146 reset line high */
		tvp514x_hw_reset(1);
		/* Set Mux for TVP5146 */
		val1 = cpld_read(DM365_CPLD_REGISTER3);
		val1 &= ~DM365_VIDEO_MUX_MASK;
		val1 |= DM365_TVP5146_SEL;
		cpld_write(val1, DM365_CPLD_REGISTER3);
	}
	return err;
}
EXPORT_SYMBOL(tvp514x_set_input_mux);

int mt9xxx_set_input_mux(unsigned char channel)
{
	int err = 0;
	u8 val[2];
	u32 val1;
	val[0] = 8;

	if (cpu_is_davinci_dm644x())
		return err;

	val[1] = 0x80;

	if (cpu_is_davinci_dm355()) {
		//Logi err = davinci_i2c_write(2, val, MSP430_I2C_ADDR);
		if (err)
			return err;
	} else if (cpu_is_davinci_dm365()) {
		/* Drive TVP5146 Reset to low */
		tvp514x_hw_reset(0);
		/* drive the sensor reset line high */
		image_sensor_hw_reset(1);
		/* Set Mux for image sensor */
		val1 = cpld_read(DM365_CPLD_REGISTER3);
		val1 &= ~DM365_VIDEO_MUX_MASK;
		val1 |= DM365_SENSOR_SEL;
		cpld_write(val1, DM365_CPLD_REGISTER3);
	}
	/* For image sensor we need to program I2C switch
	 * PCA9543A
	 */
	//Logi val[0] = 0x01;
	//Logi davinci_i2c_write(1, val, PCA9543A_I2C_ADDR);
	return 0;
}
EXPORT_SYMBOL(mt9xxx_set_input_mux);

int tvp7002_set_input_mux(unsigned char channel)
{
	u32 val;
	val = cpld_read(DM365_CPLD_REGISTER3);
	val &= ~DM365_VIDEO_MUX_MASK;
	val |= DM365_TVP7002_SEL;
	cpld_write(val, DM365_CPLD_REGISTER3);
	return 0;
}
EXPORT_SYMBOL(tvp7002_set_input_mux);

int video_davinci_evm_init(void)
{
	return 0;
}

void video_davinci_evm_cleanup(void)
{
}

MODULE_LICENSE("GPL");
/* Function for module initialization and cleanup */
module_init(video_davinci_evm_init);
module_exit(video_davinci_evm_cleanup);
