/*
 * AK4588 ADC/DAC SOC CODEC wrapper header file
 *
 * 2008 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#ifndef __SOUND_AK4XXX_SOC_ADDA_H
#define __SOUND_AK4XXX_SOC_ADDA_H

#define AK4XXX_CONTROL_1	0x0
#define AK4XXX_CONTROL_2	0x1
#define AK4XXX_LOUT_1		0x2
#define AK4XXX_ROUT_1		0x3
#define AK4XXX_LOUT_2		0x4
#define AK4XXX_ROUT_2		0x5
#define AK4XXX_LOUT_3		0x6
#define AK4XXX_ROUT_3		0x7

#define AK4XXX_DE_EMPH		0x8
#define AK4XXX_ATT_PWDC		0x9
#define AK4XXX_ZERO_DET		0xA

#define AK4XXX_LOUT_4		0xB
#define AK4XXX_ROUT_4		0xC

unsigned int ak4xxx_adda_read_cache(struct snd_soc_codec *codec,
				    unsigned int reg);
int ak4xxx_adda_write(struct snd_soc_codec *codec,
		      unsigned int reg, unsigned int value);
int ak4xxx_adda_hw_params(struct snd_pcm_substream *substream,
			  struct snd_pcm_hw_params *params);
int ak4xxx_adda_dapm_event(struct snd_soc_codec *codec, int event);
int ak4xxx_adda_init(struct snd_soc_device *socdev);

extern struct snd_soc_codec_dai ak4xxx_adda_dai;
extern struct snd_soc_codec_device soc_codec_ak4xxx_adda;

#endif		/*  */
