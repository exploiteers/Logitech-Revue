/*
 * ALSA SoC I2S Audio Layer for TI DAVINCI processor
 *
 * Copyright (C) 2008 Texas Instruments.
 *
 * Based on sound/soc/davinci/davinci-i2s-mcbsp.c by Vladimir Barinov
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
#include "davinci-i2s-vcif.h"

#define MOD_REG_BIT(val, mask, set) do { \
	if (set) { \
		val |= mask; \
	} else { \
		val &= ~mask; \
	} \
} while (0)

static inline void davinci_vcif_write_reg(struct davinci_audio_dev *dev,
					   int reg, u32 val)
{
	__raw_writel(val, dev->base + reg);
}

static inline u32 davinci_vcif_read_reg(struct davinci_audio_dev *dev, int reg)
{
	return __raw_readl(dev->base + reg);
}

static void davinci_vcif_start(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct davinci_audio_dev *dev = rtd->dai->cpu_dai->private_data;
	u32 w;

	/* Start the sample generator and enable transmitter/receiver */
	w = davinci_vcif_read_reg(dev, DAVINCI_VCIF_CTRL_REG);
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		MOD_REG_BIT(w, DAVINCI_VCIF_CTRL_RSTDAC, 1);
	else
		MOD_REG_BIT(w, DAVINCI_VCIF_CTRL_RSTADC, 1);

	davinci_vcif_write_reg(dev, DAVINCI_VCIF_CTRL_REG, w);
}

static void davinci_vcif_stop(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct davinci_audio_dev *dev = rtd->dai->cpu_dai->private_data;
	u32 w;

	/* Reset transmitter/receiver and sample rate/frame sync generators */
	w = davinci_vcif_read_reg(dev, DAVINCI_VCIF_CTRL_REG);
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		MOD_REG_BIT(w, DAVINCI_VCIF_CTRL_RSTDAC, 0);
	else
		MOD_REG_BIT(w, DAVINCI_VCIF_CTRL_RSTADC, 0);

	davinci_vcif_write_reg(dev, DAVINCI_VCIF_CTRL_REG, w);
}

static int davinci_vcif_hw_params(struct snd_pcm_substream *substream,
				 struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct davinci_audio_dev *dev = rtd->dai->cpu_dai->private_data;
	struct davinci_pcm_dma_params *dma_params =
				dev->dma_params[substream->stream];
	u32 w;

	/* general line settings */
	davinci_vcif_write_reg(dev,
			DAVINCI_VCIF_CTRL_REG, DAVINCI_VCIF_CTRL_MASK);

	davinci_vcif_write_reg(dev,
			DAVINCI_VCIF_INTCLR_REG, DAVINCI_VCIF_INT_MASK);

	davinci_vcif_write_reg(dev,
			DAVINCI_VCIF_INTEN_REG, DAVINCI_VCIF_INT_MASK);

	w = davinci_vcif_read_reg(dev, DAVINCI_VCIF_CTRL_REG);
	/* Determine xfer data type */
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_U8:
		dma_params->data_type = 0;

		MOD_REG_BIT(w, DAVINCI_VCIF_CTRL_RD_BITS_8 |
					DAVINCI_VCIF_CTRL_RD_UNSIGNED |
					DAVINCI_VCIF_CTRL_WD_BITS_8 |
					DAVINCI_VCIF_CTRL_WD_UNSIGNED, 1);
		break;
	case SNDRV_PCM_FORMAT_S8:
		dma_params->data_type = 1;

		MOD_REG_BIT(w, DAVINCI_VCIF_CTRL_RD_BITS_8 |
					DAVINCI_VCIF_CTRL_WD_BITS_8, 1);

		MOD_REG_BIT(w, DAVINCI_VCIF_CTRL_RD_UNSIGNED |
					DAVINCI_VCIF_CTRL_WD_UNSIGNED, 0);
		break;
	case SNDRV_PCM_FORMAT_S16_LE:
		dma_params->data_type = 2;

		MOD_REG_BIT(w, DAVINCI_VCIF_CTRL_RD_BITS_8 |
					DAVINCI_VCIF_CTRL_RD_UNSIGNED |
					DAVINCI_VCIF_CTRL_WD_BITS_8 |
					DAVINCI_VCIF_CTRL_WD_UNSIGNED, 0);
		break;
	default:
		printk(KERN_WARNING "davinci-vcif: unsupported PCM format");
		return -EINVAL;
	}

	davinci_vcif_write_reg(dev, DAVINCI_VCIF_CTRL_REG, w);

	return 0;
}

static int davinci_vcif_trigger(struct snd_pcm_substream *substream, int cmd)
{
	int ret = 0;

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		davinci_vcif_start(substream);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		davinci_vcif_stop(substream);
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

#define DAVINCI_VCIF_RATES	(SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000)

struct snd_soc_cpu_dai vcif_i2s_dai[] = {
	{
		.name = "davinci-vcif",
		.id = 0,
		.type = SND_SOC_DAI_I2S,
		.probe = davinci_i2s_probe,
		.remove = davinci_i2s_remove,
		.playback = {
			.channels_min = 1,
			.channels_max = 2,
			.rates = DAVINCI_VCIF_RATES,
			.formats = SNDRV_PCM_FMTBIT_S16_LE,
		},
		.capture = {
			.channels_min = 1,
			.channels_max = 2,
			.rates = DAVINCI_VCIF_RATES,
			.formats = SNDRV_PCM_FMTBIT_S16_LE,
		},
		.ops = {
			.startup = davinci_i2s_startup,
			.trigger = davinci_vcif_trigger,
			.hw_params = davinci_vcif_hw_params,
		},
	},
};
EXPORT_SYMBOL_GPL(vcif_i2s_dai);

MODULE_AUTHOR("Hui Geng");
MODULE_DESCRIPTION("TI DAVINCI VCIF SoC Interface");
MODULE_LICENSE("GPL");
