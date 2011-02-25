/*
 * ALSA SoC DaVinci DIT/DIR driver
 *
 *  TI DaVinci audio controller can operate in DIT/DIR (SPDI/F) where
 *  no codec is needed.  This file provides stub codec that can be used
 *  in these configurations.
 *
 * Author:      Steve Chen,  <schen@mvista.com>
 * Copyright:   (C) 2008 MontaVista Software, Inc., <source@mvista.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <sound/soc.h>
#include <sound/pcm.h>

#define STUB_RATES	SNDRV_PCM_RATE_48000
#define STUB_FORMATS	(SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE | \
			 SNDRV_PCM_FMTBIT_S32_LE)

struct snd_soc_codec_dai dit_stub_dai[] = {
	{
		.name = "DIT",
		.playback = {
			.stream_name = "Playback",
			.channels_min = 1,
			.channels_max = 384,
			.rates = STUB_RATES,
			.formats = STUB_FORMATS,
		},
	}
};

struct snd_soc_codec_dai dir_stub_dai[] = {
	{
		.name = "DIR",
		.capture = {
			.stream_name = "Capture",
			.channels_min = 1,
			.channels_max = 384,
			.rates = STUB_RATES,
			.formats = STUB_FORMATS,
		},
	}
};
