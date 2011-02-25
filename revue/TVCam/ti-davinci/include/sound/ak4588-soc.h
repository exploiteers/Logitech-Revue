/*
 * AK4588 SOC CODEC wrapper header file
 *
 * 2008 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#ifndef AK4XXX_SOC_H
#define AK4XXX_SOC_H

#include <sound/ak4xxx-adda.h>
#include <sound/ak4114.h>

#define AK4588_SW_REG	0xFF

struct snd_soc_ak4588_codec {
	struct snd_akm4xxx *addac;
	struct ak4114 *spdif;
	unsigned int sysclk;
	int master;
	struct snd_soc_codec *codec;
	int (*sw_reg_set)(unsigned int reg, unsigned int value);
	unsigned int (*sw_reg_get)(unsigned int reg);
};

#define AK4588_SPDIF_MASK	0x100

extern struct snd_soc_codec_device soc_codec_dev_ak4588;

#endif	/* AK4XXX_SOC_H */
