/*
 * ALSA SoC I2S (McBSP) Audio Layer for TI DAVINCI processor
 *
 * Author:      Vladimir Barinov, <vbarinov@ru.mvista.com>
 * Copyright:   (C) 2007 MontaVista Software, Inc., <source@mvista.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _DAVINCI_I2S_H
#define _DAVINCI_I2S_H

#include <linux/workqueue.h>
#include <linux/io.h>
#include "davinci-pcm.h"

#define DAVINCI_I2S_RATES	SNDRV_PCM_RATE_8000_96000

enum {
	DAVINCI_AUDIO_WORD_8 = 0,
	DAVINCI_AUDIO_WORD_12,
	DAVINCI_AUDIO_WORD_16,
	DAVINCI_AUDIO_WORD_20,
	DAVINCI_AUDIO_WORD_24,
	DAVINCI_AUDIO_WORD_28,  /* This is only valid for McASP */
	DAVINCI_AUDIO_WORD_32,
};

struct davinci_audio_dev {
	void __iomem			*base;
	int				sample_rate;
	struct clk			*clk;
	struct davinci_pcm_dma_params	*dma_params[2];
	unsigned int			codec_fmt;

	/* McASP specific data */
	int				tdm_slots;
	u8				op_mode;
	u8				num_serializer;
	u8				*serial_dir;
	u8				word_shift;
	u8				fixed_slot;
	u8				async_rxtx_clk;
	unsigned int 			sysclk_freq;
	int				sysclk_dir;
	struct work_struct		workq;
};

int davinci_i2s_startup(struct snd_pcm_substream *substream);
int davinci_i2s_probe(struct platform_device *pdev);
void davinci_i2s_remove(struct platform_device *pdev);

#endif
