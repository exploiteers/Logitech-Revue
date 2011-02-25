/*
 * ALSA SoC DIT/DIR driver header
 *
 * Author:      Steve Chen,  <schen@mvista.com>
 * Copyright:   (C) 2008 MontaVista Software, Inc., <source@mvista.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef CODEC_STUBS_H
#define CODEC_STUBS_H

#ifdef CONFIG_SND_SOC_CODEC_STUBS
extern struct snd_soc_codec_dai dit_stub_dai[];
extern struct snd_soc_codec_dai dir_stub_dai[];
#else
#define dit_stub_dai	NULL
#define dir_stub_dai	NULL
#endif

#endif /* CODEC_STUBS_H */
