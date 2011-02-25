/*
 * ASoC driver for TI DA8xx EVM platform
 *
 * Author: Steve Chen <schen@mvista.com>
 * Copyright (C) 2008 MontaVista Software, Inc. <source@mvista.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <sound/driver.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/ak4xxx-adda.h>
#include <sound/ak4114-soc-ditr.h>
#include <sound/ak4588-soc.h>
#include <asm/arch/hardware.h>

#include "../codecs/tlv320aic3x.h"
#include "../codecs/codec_stubs.h"
#include "../codecs/ak4xxx-soc-adda.h"
#include "davinci-pcm.h"
#include "davinci-i2s.h"
#include "davinci-i2s-mcasp.h"

#define DA8XX_EVM_CODEC_CLOCK 24576000

static int evm_hw_params(struct snd_pcm_substream *substream,
			 struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_codec_dai *codec_dai = rtd->dai->codec_dai;
	struct snd_soc_cpu_dai *cpu_dai = rtd->dai->cpu_dai;
	struct davinci_audio_dev *dev = rtd->dai->cpu_dai->private_data;
	int ret = 0;

	/* set codec DAI configuration */
	if (codec_dai->dai_ops.set_fmt != NULL)
		ret = codec_dai->dai_ops.set_fmt(codec_dai, SND_SOC_DAIFMT_I2S |
						 SND_SOC_DAIFMT_CBM_CFM);
	if (ret < 0)
		return ret;

	/* set cpu DAI configuration */
	ret = cpu_dai->dai_ops.set_fmt(cpu_dai, dev->codec_fmt);
	if (ret < 0)
		return ret;

	/* Set CPU DAI system clock */
	if (cpu_dai->dai_ops.set_sysclk != NULL)
		ret = cpu_dai->dai_ops.set_sysclk(cpu_dai, 0,
						  DA8XX_EVM_CODEC_CLOCK,
						  dev->sysclk_dir);
	if (ret < 0)
		return ret;

	/* Set the codec system clock */
	if (codec_dai->dai_ops.set_sysclk != NULL)
		ret = codec_dai->dai_ops.set_sysclk(codec_dai, 0,
						    DA8XX_EVM_CODEC_CLOCK,
						    SND_SOC_CLOCK_OUT);

	return ret < 0 ? ret : 0;
}

static struct snd_soc_ops evm_ops = {
	.hw_params	= evm_hw_params,
};

#if defined(CONFIG_SND_DA8XX_AIC3106_CODEC)

/* DA8xx EVM machine dapm widgets */
static const struct snd_soc_dapm_widget aic3x_dapm_widgets[] = {
	SND_SOC_DAPM_HP("Headphone Jack", NULL),
	SND_SOC_DAPM_LINE("Line Out", NULL),
	SND_SOC_DAPM_MIC("Mic Jack", NULL),
	SND_SOC_DAPM_LINE("Line In", NULL),
};

/* DA8xx EVM machine audio_mapnections to the codec pins */
static const char *audio_map[][3] = {
	/* Headphone connected to HPLOUT, HPROUT */
	{ "Headphone Jack", NULL, "HPLOUT" },
	{ "Headphone Jack", NULL, "HPROUT" },

	/* Line Out connected to LLOUT, RLOUT */
	{ "Line Out", NULL, "LLOUT"},
	{ "Line Out", NULL, "RLOUT"},

	/* Mic connected to (MIC3L | MIC3R) */
	{ "MIC3L", NULL, "Mic Bias 2V" },
	{ "MIC3R", NULL, "Mic Bias 2V" },
	{ "Mic Bias 2V", NULL, "Mic Jack" },

	/* Line In connected to (LINE1L | LINE2L), (LINE1R | LINE2R) */
	{ "LINE1L", NULL, "Line In" },
	{ "LINE2L", NULL, "Line In" },
	{ "LINE1R", NULL, "Line In" },
	{ "LINE2R", NULL, "Line In" },

	{NULL, NULL, NULL},
};

/* Logic for a aic3x as connected on a davinci-evm */
static int evm_aic3x_init(struct snd_soc_codec *codec)
{
	int i;

	/* Add davinci-evm specific widgets */
	for (i = 0; i < ARRAY_SIZE(aic3x_dapm_widgets); i++)
		snd_soc_dapm_new_control(codec, &aic3x_dapm_widgets[i]);

	/* Set up davinci-evm specific audio path audio_map */
	for (i = 0; audio_map[i][0] != NULL; i++)
		snd_soc_dapm_connect_input(codec, audio_map[i][0],
					   audio_map[i][1], audio_map[i][2]);

	/* not connected */
	snd_soc_dapm_set_endpoint(codec, "MONO_LOUT", 0);
	snd_soc_dapm_set_endpoint(codec, "HPLCOM", 0);
	snd_soc_dapm_set_endpoint(codec, "HPRCOM", 0);

	/* always connected */
	snd_soc_dapm_set_endpoint(codec, "Headphone Jack", 1);
	snd_soc_dapm_set_endpoint(codec, "Line Out", 1);
	snd_soc_dapm_set_endpoint(codec, "Mic Jack", 1);
	snd_soc_dapm_set_endpoint(codec, "Line In", 1);

	snd_soc_dapm_sync_endpoints(codec);

	return 0;
}

#elif defined(CONFIG_SND_DA8XX_AK4588_CODEC)

static u8 *serializer_direction;
static u8 tx_serializer[] = { 5, 6, 7, 8 };
static u8 rx_serializer[] = { 0, 1, 2, 10 };
static u16 sw_reg_value = 0x801;

static int mcasp_serializer_control_set(unsigned int reg, unsigned int value)
{
	int i;
	u16 mask;

	sw_reg_value = value;
	mask = 0x1;
	for (i = 0; i < ARRAY_SIZE(tx_serializer); i++) {
		if (sw_reg_value & mask)
			serializer_direction[tx_serializer[i]] = TX_MODE;
		else
			serializer_direction[tx_serializer[i]] = INACTIVE_MODE;

		mask <<= 1;
	}

	mask = 0x100;
	for (i = 0; i < ARRAY_SIZE(rx_serializer); i++) {
		if (sw_reg_value & mask)
			serializer_direction[rx_serializer[i]] = RX_MODE;
		else
			serializer_direction[rx_serializer[i]] = INACTIVE_MODE;

		mask <<= 1;
	}
	return 0;
}

static unsigned int mcasp_serializer_control_get(unsigned int reg)
{
	return sw_reg_value;
}

/* DA8xx EVM machine dapm widgets */
static const struct snd_kcontrol_new ak4588_serializer_controls[] = {
	SOC_SINGLE("LB-RB Playback Switch", AK4588_SW_REG, 0, 1, 0),
	SOC_SINGLE("C-SW Playback Switch",  AK4588_SW_REG, 1, 1, 0),
	SOC_SINGLE("LS-RS Playback Switch", AK4588_SW_REG, 2, 1, 0),
	SOC_SINGLE("LF-RF Playback Switch", AK4588_SW_REG, 3, 1, 0),
	SOC_SINGLE("LF-RF Capture Switch",  AK4588_SW_REG, 8, 1, 0),
	SOC_SINGLE("LS-RS Capture Switch",  AK4588_SW_REG, 9, 1, 0),
	SOC_SINGLE("C-SW Capture Switch",   AK4588_SW_REG, 10, 1, 0),
	SOC_SINGLE("LB-RB Capture Switch",  AK4588_SW_REG, 11, 1, 0),
};

static int evm_ak4588_control_init(struct snd_soc_codec *codec)
{
	struct snd_soc_ak4588_codec *ak4588 = codec->private_data;
	int i, err;

	ak4588->sw_reg_get = mcasp_serializer_control_get;
	ak4588->sw_reg_set = mcasp_serializer_control_set;

	for (i = 0; i < ARRAY_SIZE(ak4588_serializer_controls); i++) {
		err = snd_ctl_add(codec->card,
				  snd_soc_cnew(&ak4588_serializer_controls[i],
					       codec, NULL));
		if (err < 0)
			return err;
	}
	return 0;
}
#endif

#if defined(CONFIG_SND_DA8XX_AIC3106_CODEC)

static struct snd_soc_dai_link da8xx_evm_dai[] = {
	{
		.name		= "TLV320AIC3X",
		.stream_name	= "AIC3X",
#ifdef CONFIG_SND_DAVINCI_SOC_MCASP1_ARM_CNTL
		.cpu_dai	= davinci_iis_mcasp_dai,
#else
		.cpu_dai	= davinci_iis_mcasp_dsp_dai,
#endif
		.codec_dai	= &aic3x_dai,
		.init		= evm_aic3x_init,
		.ops		= &evm_ops,
	},
};

#elif defined(CONFIG_SND_DA8XX_AK4588_CODEC)

static struct snd_soc_dai_link da8xx_evm_avr_dai[] = {
	{
		.name		= "AK4588 SPDIF RX",
		.stream_name	= "DIR",
#ifdef CONFIG_SND_DAVINCI_SOC_MCASP0_ARM_CNTL
		.cpu_dai	= davinci_dir_mcasp_dai,
#else
		.cpu_dai	= davinci_iis_mcasp_dsp_dai,
#endif
		.codec_dai	= &ak4114_dir_dai,
		.ops		= &evm_ops,
	},
	{
		.name		= "AK4588 ADDA",
		.stream_name	= "AK4xxx",
#ifdef CONFIG_SND_DAVINCI_SOC_MCASP1_ARM_CNTL
		.cpu_dai	= davinci_iis_mcasp_dai,
#else
		.cpu_dai	= davinci_iis_mcasp_dsp_dai,
#endif
		.codec_dai	= &ak4xxx_adda_dai,
		.init		= evm_ak4588_control_init,
		.ops		= &evm_ops,
	},
	{
		.name		= "AK4588 SPDIF TX",
		.stream_name	= "DIT",
#ifdef CONFIG_SND_DAVINCI_SOC_MCASP2_ARM_CNTL
		.cpu_dai	= davinci_dit_mcasp_dai,
#else
		.cpu_dai	= davinci_iis_mcasp_dsp_dai,
#endif
		.codec_dai	= dit_stub_dai,
		.ops		= &evm_ops,
	},
};
#endif

/* DA8xx EVM audio machine driver */

#if defined(CONFIG_SND_DA8XX_AIC3106_CODEC)

static u8 da8xx_iis_serializer_direction[] = {
	RX_MODE,       INACTIVE_MODE, INACTIVE_MODE, INACTIVE_MODE,
	INACTIVE_MODE, TX_MODE,       INACTIVE_MODE, INACTIVE_MODE,
	INACTIVE_MODE, INACTIVE_MODE, INACTIVE_MODE, INACTIVE_MODE,
};

static struct snd_soc_machine da8xx_snd_soc_machine = {
	.name		= "DA8XX EVM",
	.dai_link	= da8xx_evm_dai,
	.num_links	= 1,
};

static struct aic3x_setup_data da8xx_evm_aic3x_setup = {
	.i2c_address	= 0x18,
	.variant	= AIC3106_CODEC
};

/* evm audio subsystem */
static struct snd_soc_device da8xx_evm_snd_devdata[] = {
	{
		.machine	= &da8xx_snd_soc_machine,
		.platform	= &davinci_soc_platform,
		.codec_dev	= &soc_codec_dev_aic3x,
		.codec_data	= &da8xx_evm_aic3x_setup,
	},
};

static struct resource da8xx_evm_aic_snd_resources[] = {
	{
		.start	= DA8XX_MCASP1_CNTRL_BASE,
		.end	= DA8XX_MCASP1_CNTRL_BASE + (SZ_1K * 2) - 1,
		.flags	= IORESOURCE_MEM
	},
};

static struct evm_snd_platform_data da8xx_evm_snd_data[] = {
	{
		.clk_name	= "McASPCLK1",
		.tx_dma_ch	= DA8XX_DMACH_MCASP1_TX,
		.rx_dma_ch	= DA8XX_DMACH_MCASP1_RX,
		.tx_dma_offset	= 0x2000,
		.rx_dma_offset	= 0x2000,
		.op_mode	= DAVINCI_MCASP_IIS_MODE,
		.num_serializer	= 12,
		.tdm_slots	= 2,
		.serial_dir	= da8xx_iis_serializer_direction,
		.eventq_no	= EVENTQ_1,
		.codec_fmt	= SND_SOC_DAIFMT_CBM_CFM | SND_SOC_DAIFMT_IB_NF,
		.sysclk_dir	= SND_SOC_CLOCK_OUT,
	},
};

#elif defined(CONFIG_SND_DA8XX_AK4588_CODEC)

static u8 da8xx_iis_serializer_direction[] = {
	INACTIVE_MODE, INACTIVE_MODE, INACTIVE_MODE, INACTIVE_MODE,
	INACTIVE_MODE, TX_MODE,       INACTIVE_MODE, INACTIVE_MODE,
	INACTIVE_MODE, INACTIVE_MODE, RX_MODE,       INACTIVE_MODE,
};

static struct snd_soc_machine da8xx_snd_soc_avr_machine = {
	.name = "DA8xx EVM",
	.dai_link = da8xx_evm_avr_dai,
	.num_links = 3,
};

static struct snd_soc_device da8xx_evm_avr_snd_devdata[] = {
	{
		.machine	= &da8xx_snd_soc_avr_machine,
		.platform	= &davinci_soc_platform,
		.codec_dev	= &soc_codec_dev_ak4588,
	},
};

static struct resource da8xx_evm_snd_resources[] = {
	{
		.start	= DA8XX_MCASP0_CNTRL_BASE,
		.end	= DA8XX_MCASP0_CNTRL_BASE + (SZ_1K * 2) - 1,
		.flags	= IORESOURCE_MEM
	},
	{
		.start	= DA8XX_MCASP1_CNTRL_BASE,
		.end	= DA8XX_MCASP1_CNTRL_BASE + (SZ_1K * 2) - 1,
		.flags	= IORESOURCE_MEM
	},
	{
		.start	= DA8XX_MCASP2_CNTRL_BASE,
		.end	= DA8XX_MCASP2_CNTRL_BASE + (SZ_1K * 2) - 1,
		.flags	= IORESOURCE_MEM
	},
};

static u8 da8xx_dir_serializer_direction[] = {
	INACTIVE_MODE, INACTIVE_MODE, INACTIVE_MODE, INACTIVE_MODE,
	INACTIVE_MODE, INACTIVE_MODE, INACTIVE_MODE, INACTIVE_MODE,
	INACTIVE_MODE, RX_MODE,       INACTIVE_MODE, INACTIVE_MODE,
	INACTIVE_MODE, INACTIVE_MODE, INACTIVE_MODE, INACTIVE_MODE,
};

static u8 da8xx_dit_serializer_direction[] = {
	TX_MODE, INACTIVE_MODE, INACTIVE_MODE, INACTIVE_MODE,
};

static struct evm_snd_platform_data da8xx_evm_avr_snd_data[] = {
	{			/* DIR */
		.clk_name	= "McASPCLK0",
		.tx_dma_ch	= -1,
		.rx_dma_ch	= DA8XX_DMACH_MCASP0_RX,
		.tx_dma_offset	= 0,
		.rx_dma_offset	= 0x2000,
		.op_mode	= DAVINCI_MCASP_IIS_MODE,
		.num_serializer	= 16,
		.tdm_slots	= 2,
		.serial_dir	= da8xx_dir_serializer_direction,
		.eventq_no	= EVENTQ_1,
		.codec_fmt	= SND_SOC_DAIFMT_CBM_CFM | SND_SOC_DAIFMT_IB_IF,
		.word_shift	= 1,
		.fixed_slot	= 1,
		.async_rxtx_clk	= 1,
	},
	{			/* Analog I/O */
		.clk_name	= "McASPCLK1",
		.tx_dma_ch	= DA8XX_DMACH_MCASP1_TX,
		.rx_dma_ch	= DA8XX_DMACH_MCASP1_RX,
		.tx_dma_offset	= 0x2000,
		.rx_dma_offset	= 0x2000,
		.op_mode	= DAVINCI_MCASP_IIS_MODE,
		.num_serializer	= 12,
		.tdm_slots	= 2,
		.serial_dir	= da8xx_iis_serializer_direction,
		.eventq_no	= EVENTQ_1,
		.codec_fmt	= SND_SOC_DAIFMT_CBM_CFS |
				  SND_SOC_DAIFMT_NB_IF |
				  SND_SOC_DAIFMT_MSB,
		.sysclk_dir	= SND_SOC_CLOCK_IN,
		.word_shift	= 1,
		.fixed_slot	= 1,
		.async_rxtx_clk	= 1,
	},
	{			/* DIT */
		.clk_name	= "McASPCLK2",
		.tx_dma_ch	= DA8XX_DMACH_MCASP2_TX,
		.rx_dma_ch	= -1,
		.tx_dma_offset	= 0x2000,
		.rx_dma_offset	= 0,
		.op_mode	= DAVINCI_MCASP_DIT_MODE,
		.num_serializer	= 4,
		.tdm_slots	= 32,
		.serial_dir	= da8xx_dit_serializer_direction,
		.eventq_no	= EVENTQ_1,
		.codec_fmt	= SND_SOC_DAIFMT_CBS_CFS | SND_SOC_DAIFMT_IB_NF,
		.sysclk_dir	= SND_SOC_CLOCK_OUT,
	},
};
#endif

static struct platform_device *evm_snd_device;

static int __init da8xx_evm_init(void)
{
	struct snd_soc_device *evm_snd_devdata;
	struct resource *evm_snd_resources;
	struct evm_snd_platform_data *evm_snd_data;
	int res_size, ret = 0;

#if defined(CONFIG_SND_DA8XX_AK4588_CODEC)
	serializer_direction = da8xx_iis_serializer_direction;
	evm_snd_devdata = da8xx_evm_avr_snd_devdata;
	evm_snd_resources = da8xx_evm_snd_resources;
	res_size = ARRAY_SIZE(da8xx_evm_snd_resources);
	evm_snd_data = da8xx_evm_avr_snd_data;
#elif defined(CONFIG_SND_DA8XX_AIC3106_CODEC)
	evm_snd_devdata = da8xx_evm_snd_devdata;
	evm_snd_resources = da8xx_evm_aic_snd_resources;
	res_size = ARRAY_SIZE(da8xx_evm_aic_snd_resources);
	evm_snd_data = da8xx_evm_snd_data;
#endif

	evm_snd_device = platform_device_alloc("soc-audio", -1);
	if (evm_snd_device == NULL)
		return -ENOMEM;

	platform_set_drvdata(evm_snd_device, evm_snd_devdata);
	evm_snd_devdata->dev = &evm_snd_device->dev;
	evm_snd_device->dev.platform_data = evm_snd_data;

	ret = platform_device_add_resources(evm_snd_device,
					    evm_snd_resources, res_size);
	if (ret)
		goto dev_put;

	ret = platform_device_add(evm_snd_device);
	if (!ret)
		return 0;

dev_put:
	platform_device_put(evm_snd_device);
	return ret;
}

static void __exit da8xx_evm_exit(void)
{
	platform_device_unregister(evm_snd_device);
}

module_init(da8xx_evm_init);
module_exit(da8xx_evm_exit);

MODULE_AUTHOR("Steve Chen <schen@mvista.com>");
MODULE_DESCRIPTION("TI DA8xx EVM ASoC driver");
MODULE_LICENSE("GPL");
