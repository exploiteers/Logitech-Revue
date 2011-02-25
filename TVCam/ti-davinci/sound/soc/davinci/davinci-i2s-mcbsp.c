/*
 * ALSA SoC I2S (McBSP) Audio Layer for TI DAVINCI processor
 *
 * Author:      Vladimir Barinov, <vbarinov@ru.mvista.com>
 * 		Steve Chen, <schen@mvista.com>
 * Copyright:   (C) 2008 MontaVista Software, Inc., <source@mvista.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/clk.h>

#include <sound/driver.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include <sound/soc.h>

#include "davinci-pcm.h"
#include "davinci-i2s.h"
#include "davinci-i2s-mcbsp.h"

#define MOD_REG_BIT(val, mask, set) do { \
	if (set) { \
		val |= mask; \
	} else { \
		val &= ~mask; \
	} \
} while (0)

static inline void davinci_mcbsp_write_reg(struct davinci_audio_dev *dev,
					   int reg, u32 val)
{
	__raw_writel(val, dev->base + reg);
}

static inline u32 davinci_mcbsp_read_reg(struct davinci_audio_dev *dev, int reg)
{
	return __raw_readl(dev->base + reg);
}

static void davinci_mcbsp_start(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct davinci_audio_dev *dev = rtd->dai->cpu_dai->private_data;
	u32 w;

	/* Start the sample generator and enable transmitter/receiver */
	w = davinci_mcbsp_read_reg(dev, DAVINCI_MCBSP_SPCR_REG);
	MOD_REG_BIT(w, DAVINCI_MCBSP_SPCR_GRST, 1);
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		MOD_REG_BIT(w, DAVINCI_MCBSP_SPCR_XRST, 1);
	else
		MOD_REG_BIT(w, DAVINCI_MCBSP_SPCR_RRST, 1);
	davinci_mcbsp_write_reg(dev, DAVINCI_MCBSP_SPCR_REG, w);

	/* Start frame sync */
	w = davinci_mcbsp_read_reg(dev, DAVINCI_MCBSP_SPCR_REG);
	MOD_REG_BIT(w, DAVINCI_MCBSP_SPCR_FRST, 1);
	davinci_mcbsp_write_reg(dev, DAVINCI_MCBSP_SPCR_REG, w);
}

static void davinci_mcbsp_stop(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct davinci_audio_dev *dev = rtd->dai->cpu_dai->private_data;
	u32 w;

	/* Start the sample generator and enable transmitter/receiver */
	w = davinci_mcbsp_read_reg(dev, DAVINCI_MCBSP_SPCR_REG);
	MOD_REG_BIT(w, DAVINCI_MCBSP_SPCR_GRST, 1);
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		MOD_REG_BIT(w, DAVINCI_MCBSP_SPCR_XRST, 1);
	else
		MOD_REG_BIT(w, DAVINCI_MCBSP_SPCR_RRST, 1);
	davinci_mcbsp_write_reg(dev, DAVINCI_MCBSP_SPCR_REG, w);

	/* Start frame sync */
	w = davinci_mcbsp_read_reg(dev, DAVINCI_MCBSP_SPCR_REG);
	MOD_REG_BIT(w, DAVINCI_MCBSP_SPCR_FRST, 1);
	davinci_mcbsp_write_reg(dev, DAVINCI_MCBSP_SPCR_REG, w);
}

static int davinci_i2s_set_dai_fmt(struct snd_soc_cpu_dai *cpu_dai,
				   unsigned int fmt)
{
	struct davinci_audio_dev *dev = cpu_dai->private_data;
	u32 w;

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBS_CFS:
		davinci_mcbsp_write_reg(dev, DAVINCI_MCBSP_PCR_REG,
					DAVINCI_MCBSP_PCR_FSXM |
					DAVINCI_MCBSP_PCR_FSRM |
					DAVINCI_MCBSP_PCR_CLKXM |
					DAVINCI_MCBSP_PCR_CLKRM);
		davinci_mcbsp_write_reg(dev, DAVINCI_MCBSP_SRGR_REG,
					DAVINCI_MCBSP_SRGR_FSGM);
		break;
	case SND_SOC_DAIFMT_CBM_CFM:
		davinci_mcbsp_write_reg(dev, DAVINCI_MCBSP_PCR_REG, 0);
		break;
	default:
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_IB_NF:
		w = davinci_mcbsp_read_reg(dev, DAVINCI_MCBSP_PCR_REG);
		MOD_REG_BIT(w, DAVINCI_MCBSP_PCR_CLKXP |
			       DAVINCI_MCBSP_PCR_CLKRP, 1);
		davinci_mcbsp_write_reg(dev, DAVINCI_MCBSP_PCR_REG, w);
		break;
	case SND_SOC_DAIFMT_NB_IF:
		w = davinci_mcbsp_read_reg(dev, DAVINCI_MCBSP_PCR_REG);
		MOD_REG_BIT(w, DAVINCI_MCBSP_PCR_FSXP |
			       DAVINCI_MCBSP_PCR_FSRP, 1);
		davinci_mcbsp_write_reg(dev, DAVINCI_MCBSP_PCR_REG, w);
		break;
	case SND_SOC_DAIFMT_IB_IF:
		w = davinci_mcbsp_read_reg(dev, DAVINCI_MCBSP_PCR_REG);
		MOD_REG_BIT(w, DAVINCI_MCBSP_PCR_CLKXP |
			       DAVINCI_MCBSP_PCR_CLKRP |
			       DAVINCI_MCBSP_PCR_FSXP |
			       DAVINCI_MCBSP_PCR_FSRP, 1);
		davinci_mcbsp_write_reg(dev, DAVINCI_MCBSP_PCR_REG, w);
		break;
	case SND_SOC_DAIFMT_NB_NF:
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int davinci_i2s_hw_params(struct snd_pcm_substream *substream,
				 struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct davinci_audio_dev *dev = rtd->dai->cpu_dai->private_data;
	struct davinci_pcm_dma_params *dma_params =
					  dev->dma_params[substream->stream];
	struct snd_interval *i = NULL;
	int mcbsp_word_length;
	u32 w;

	/* general line settings */
	w = davinci_mcbsp_read_reg(dev, DAVINCI_MCBSP_SPCR_REG);
	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		w &= ~(DAVINCI_MCBSP_SPCR_RRST);
		w |= DAVINCI_MCBSP_SPCR_RINTM(3) | DAVINCI_MCBSP_SPCR_FREE;
		davinci_mcbsp_write_reg(dev, DAVINCI_MCBSP_SPCR_REG, w);
		davinci_mcbsp_write_reg(dev, DAVINCI_MCBSP_RCR_REG,
					DAVINCI_MCBSP_RCR_RFRLEN1(1) |
					DAVINCI_MCBSP_RCR_RDATDLY(1));
	} else {
		w &= ~(DAVINCI_MCBSP_SPCR_XRST);
		w |= DAVINCI_MCBSP_SPCR_XINTM(3) | DAVINCI_MCBSP_SPCR_FREE;
		davinci_mcbsp_write_reg(dev, DAVINCI_MCBSP_SPCR_REG, w);
		davinci_mcbsp_write_reg(dev, DAVINCI_MCBSP_XCR_REG,
					DAVINCI_MCBSP_XCR_XFRLEN1(1) |
					DAVINCI_MCBSP_XCR_XDATDLY(1) |
					DAVINCI_MCBSP_XCR_XFIG);
	}

	i = hw_param_interval(params, SNDRV_PCM_HW_PARAM_SAMPLE_BITS);
	w = davinci_mcbsp_read_reg(dev, DAVINCI_MCBSP_SRGR_REG);
	MOD_REG_BIT(w, DAVINCI_MCBSP_SRGR_FWID(snd_interval_value(i) - 1), 1);
	davinci_mcbsp_write_reg(dev, DAVINCI_MCBSP_SRGR_REG, w);

	i = hw_param_interval(params, SNDRV_PCM_HW_PARAM_FRAME_BITS);
	w = davinci_mcbsp_read_reg(dev, DAVINCI_MCBSP_SRGR_REG);
	MOD_REG_BIT(w, DAVINCI_MCBSP_SRGR_FPER(snd_interval_value(i) - 1), 1);
	davinci_mcbsp_write_reg(dev, DAVINCI_MCBSP_SRGR_REG, w);

	/* Determine xfer data type */
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S8:
		dma_params->data_type = 1;
		mcbsp_word_length = DAVINCI_AUDIO_WORD_8;
		break;
	case SNDRV_PCM_FORMAT_S16_LE:
		dma_params->data_type = 2;
		mcbsp_word_length = DAVINCI_AUDIO_WORD_16;
		break;
	case SNDRV_PCM_FORMAT_S32_LE:
		dma_params->data_type = 4;
		mcbsp_word_length = DAVINCI_AUDIO_WORD_32;
		break;
	default:
		printk(KERN_WARNING "davinci-i2s: unsupported PCM format");
		return -EINVAL;
	}

	if (substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		w = davinci_mcbsp_read_reg(dev, DAVINCI_MCBSP_RCR_REG);
		MOD_REG_BIT(w, DAVINCI_MCBSP_RCR_RWDLEN1(mcbsp_word_length) |
		       DAVINCI_MCBSP_RCR_RWDLEN2(mcbsp_word_length), 1);
		davinci_mcbsp_write_reg(dev, DAVINCI_MCBSP_RCR_REG, w);
	} else {

		w = davinci_mcbsp_read_reg(dev, DAVINCI_MCBSP_XCR_REG);
		MOD_REG_BIT(w, DAVINCI_MCBSP_XCR_XWDLEN1(mcbsp_word_length) |
		       DAVINCI_MCBSP_XCR_XWDLEN2(mcbsp_word_length), 1);
		davinci_mcbsp_write_reg(dev, DAVINCI_MCBSP_XCR_REG, w);
	}
	return 0;
}

static int davinci_i2s_trigger(struct snd_pcm_substream *substream, int cmd)
{
	int ret = 0;

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		davinci_mcbsp_start(substream);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		davinci_mcbsp_stop(substream);
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

struct snd_soc_cpu_dai davinci_i2s_dai[] = {
	{
		.name = "davinci-i2s",
		.id = 0,
		.type = SND_SOC_DAI_I2S,
		.probe = davinci_i2s_probe,
		.remove = davinci_i2s_remove,
		.playback = {
			.channels_min = 1,
			.channels_max = 2,
			.rates = DAVINCI_I2S_RATES,
			.formats = SNDRV_PCM_FMTBIT_S16_LE,},
		.capture = {
			.channels_min = 1,
			.channels_max = 2,
			.rates = DAVINCI_I2S_RATES,
			.formats = SNDRV_PCM_FMTBIT_S16_LE,},
		.ops = {
			.startup = davinci_i2s_startup,
			.trigger = davinci_i2s_trigger,
			.hw_params = davinci_i2s_hw_params,},
		.dai_ops = {
			.set_fmt = davinci_i2s_set_dai_fmt,
		},
	},
};
EXPORT_SYMBOL_GPL(davinci_i2s_dai);

MODULE_AUTHOR("Vladimir Barinov");
MODULE_DESCRIPTION("TI DAVINCI I2S (McBSP) SoC Interface");
MODULE_LICENSE("GPL");
