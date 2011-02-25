/*
 * AK4114 DIT/DIR wrapper header file
 *
 * 2008 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#ifndef AK4114_SOC_DITR_H
#define AK4114_SOC_DITR_H

unsigned int ak4114_ditr_read(struct snd_soc_codec *codec, unsigned int reg);
int ak4114_ditr_write(struct snd_soc_codec *codec,
		      unsigned int reg, unsigned int value);
int ak4114_ditr_dapm_event(struct snd_soc_codec *codec, int event);
int ak4114_ditr_init(struct snd_soc_device *socdev);

extern struct snd_soc_codec_dai ak4114_dir_dai;
extern struct snd_soc_codec_device soc_codec_ak4114_ditr;

#endif	/* AK4114_SOC_DITR_H */
