/*
 * AK4588 ADC/DAC SOC CODEC wrapper
 *
 * Copyright (C) 2008 MontaVista Software, Inc. <source@mvista.com>
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <sound/driver.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <sound/ak4xxx-adda.h>
#include <sound/ak4588-soc.h>

#define AUDIO_NAME "AK4XXX ADDA"
#define AK4XXX_VERSION "0.1"

static const struct snd_akm4xxx_dac_channel ak4628_dac_info[] = {
	{
		.name = "PCM Playback Volume",
		.num_channels = 2,
	},
	{
		.name = "PCM Center Playback Volume",
		.num_channels = 2,
	},
	{
		.name = "PCM LFE Playback Volume",
		.num_channels = 2,
	},
	{
		.name = "PCM Rear Playback Volume",
		.num_channels = 2,
	},
};

static struct snd_akm4xxx ak4xxx_codec_data = {
		.type =	SND_AK4628,
		.num_adcs = 0,
		.num_dacs = 8,
		.dac_info = ak4628_dac_info,
};
/*
 * read ak4xxx register cache
 */
unsigned int ak4xxx_adda_read_cache(struct snd_soc_codec *codec,
				    unsigned int reg)
{
	struct snd_soc_ak4588_codec *chip = codec->private_data;
	struct snd_akm4xxx *ak = chip->addac;

	if ((reg == AK4588_SW_REG) && (chip->sw_reg_get))
		return chip->sw_reg_get(reg);
	else if (reg >= AK4XXX_IMAGE_SIZE)
		return -1;

	return snd_akm4xxx_get(ak, 0, reg);
}

/*
 * write to the ak4xxx register space
 */
int ak4xxx_adda_write(struct snd_soc_codec *codec,
		      unsigned int reg, unsigned int value)
{
	struct snd_soc_ak4588_codec *chip = codec->private_data;
	struct snd_akm4xxx *ak = chip->addac;

	if ((reg == AK4588_SW_REG) && (chip->sw_reg_set))
		chip->sw_reg_set(reg, value);
	else if (reg >= AK4XXX_IMAGE_SIZE)
		return -1;
	else
		snd_akm4xxx_write(ak, 0, reg, value);

	return 0;
}

int ak4xxx_adda_hw_params(struct snd_pcm_substream *substream,
			  struct snd_pcm_hw_params *params)
{
	return 0;
}

static int ak4xxx_adda_mute(struct snd_soc_codec_dai *dai, int mute)
{
	struct snd_soc_codec *codec = dai->codec;
	struct snd_soc_ak4588_codec *chip = codec->private_data;
	struct snd_akm4xxx *ak = chip->addac;

	snd_akm4xxx_mute(ak, mute);

	return 0;
}

static int ak4xxx_adda_set_dai_sysclk(struct snd_soc_codec_dai *codec_dai,
					 int clk_id, unsigned int freq, int dir)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct snd_soc_ak4588_codec *chip = codec->private_data;

	switch (freq) {
	case  8192000:
	case 11289600:
	case 12288000:
	case 16384000:
	case 16934400:
	case 18432000:
	case 22579200:
	case 24576000:
		chip->sysclk = freq;
		return 0;
	}

	return -EINVAL;
}

static int ak4xxx_adda_set_dai_fmt(struct snd_soc_codec_dai *codec_dai,
				   unsigned int fmt)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct snd_soc_ak4588_codec *chip = codec->private_data;
	u8 dif_val = 0;

	/* set master/slave audio interface */
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:
	case SND_SOC_DAIFMT_CBM_CFS:
		chip->master = 1;
		break;
	case SND_SOC_DAIFMT_CBS_CFS:
		chip->master = 0;
		break;
	default:
		return -EINVAL;
	}

	/* interface format */
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		dif_val = 0x3;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		dif_val = 0x1;
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		dif_val = 0x2;
		break;
	default:
		return -EINVAL;
	}

	/* set iface */
	dif_val = (ak4xxx_adda_read_cache(codec, 0) & 0xf3) | (dif_val << 2);
	ak4xxx_adda_write(codec, 0, dif_val);

	return 0;
}

static int ak4xxx_adda_hw_free(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_device *socdev = rtd->socdev;
	struct snd_soc_codec *codec = socdev->codec;
	struct snd_soc_ak4588_codec *chip = codec->private_data;
	struct snd_akm4xxx *ak = chip->addac;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		snd_akm4xxx_mute(ak, 1);

	return 0;
}

#define AK4XXX_RATES	(SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_48000 |\
			 SNDRV_PCM_RATE_64000 | SNDRV_PCM_RATE_96000)
#define AK4XXX_FORMATS	(SNDRV_PCM_FMTBIT_S24_LE |  SNDRV_PCM_FMTBIT_S32_LE |\
			 SNDRV_PCM_FMTBIT_S16_LE)

struct snd_soc_codec_dai ak4xxx_adda_dai = {
	.name = "ak4xxx",
	.playback = {
		.stream_name = "AK4xxx Playback",
		.channels_min = 2,
		.channels_max = 2,
		.rates = AK4XXX_RATES,
		.formats = AK4XXX_FORMATS,
	},
	.capture = {
		.stream_name = "AK4xxx Capture",
		.channels_min = 2,
		.channels_max = 2,
		.rates = AK4XXX_RATES,
		.formats = AK4XXX_FORMATS,
	},
	.ops = {
		.hw_params = ak4xxx_adda_hw_params,
		.hw_free = ak4xxx_adda_hw_free,
	},
	.dai_ops = {
		.digital_mute = ak4xxx_adda_mute,
		.set_sysclk = ak4xxx_adda_set_dai_sysclk,
		.set_fmt = ak4xxx_adda_set_dai_fmt,
	}
};
EXPORT_SYMBOL_GPL(ak4xxx_adda_dai);

static void ak4xxx_adda_lock(struct snd_akm4xxx *ak, int chip)
{
	struct snd_soc_ak4588_codec *priv_data = ak->private_data[0];
	struct snd_soc_codec *codec = priv_data->codec;

	mutex_lock(&codec->mutex);
}

static void ak4xxx_adda_unlock(struct snd_akm4xxx *ak, int chip)
{
	struct snd_soc_ak4588_codec *priv_data = ak->private_data[0];
	struct snd_soc_codec *codec = priv_data->codec;

	mutex_unlock(&codec->mutex);
}

int ak4xxx_adda_dapm_event(struct snd_soc_codec *codec, int event)
{
	switch (event) {
	case SNDRV_CTL_POWER_D0:
	case SNDRV_CTL_POWER_D1:
	case SNDRV_CTL_POWER_D2:
	case SNDRV_CTL_POWER_D3:
	default:
		break;
	}
	codec->dapm_state = event;
	return 0;
}

static const struct snd_soc_dapm_widget ak4628_dapm_widgets[] = {
	SND_SOC_DAPM_DAC("ak4628 DAC GLB", "AK4xxx Playback", 0x0A, 0, 0),
	SND_SOC_DAPM_DAC("ak4628 DAC_0", "AK4xxx Playback", 0x09, 1, 0),
	SND_SOC_DAPM_DAC("ak4628 DAC_1", "AK4xxx Playback", 0x09, 2, 0),
	SND_SOC_DAPM_DAC("ak4628 DAC_2", "AK4xxx Playback", 0x09, 3, 0),
	SND_SOC_DAPM_DAC("ak4628 DAC_3", "AK4xxx Playback", 0x09, 6, 0),
	SND_SOC_DAPM_ADC("ak4628 ADC_GLB", "AK4xxx Capture", 0x0A, 1, 0),

	SND_SOC_DAPM_OUTPUT("LF-RF_OUT"),
	SND_SOC_DAPM_OUTPUT("LS-RS_OUT"),
	SND_SOC_DAPM_OUTPUT("C-SW_OUT"),
	SND_SOC_DAPM_OUTPUT("LB-RB_OUT"),

	SND_SOC_DAPM_INPUT("LF-RF_IN"),
	SND_SOC_DAPM_INPUT("LS-RS_IN"),
	SND_SOC_DAPM_INPUT("C-SW_IN"),
	SND_SOC_DAPM_INPUT("LB-RB_IN"),
};

static int ak4xxx_adda_add_widgets(struct snd_soc_codec *codec)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(ak4628_dapm_widgets); i++)
		snd_soc_dapm_new_control(codec, &ak4628_dapm_widgets[i]);

	return 0;
}

#if defined(CONFIG_SND_SOC_AK4588_I2C)
/*
 * callback from snd_akm4xxx_write that actually writes into hardware
 */
static void ak4xxx_adda_hw_write(struct snd_akm4xxx *ak, int chip,
				 unsigned char reg, unsigned char val)

{
	struct snd_soc_ak4588_codec *priv_data = ak->private_data[0];
	struct snd_soc_codec *codec = priv_data->codec;
	u8 data[3];

	/* data format are
	 *   Byte 1 (slave address) - 0 0 1 0 0 CAD1 CAD0 R/W
	 *   Byte 2 (Reg address)   - * * * A4 A3 A2 A1 A0
	 *   Byte 3 (Data)          - D7 D6 D5 D4 D3 D2 D1 D0
	 */

	/*
	 * 0x20 - Fixed pattern.
	 * 0x02 - CAD1 = 0, CAD0 = 1, When both CAD1/0 = 0, the message is
	 *	  for DIT/R part, all other patterns are for the ADC/DAC part.
	 * 0x01 - R/W = 1 is write mode
	 */
	data[0] = 0x21 | 0x02;
	data[1] = reg & 0x1f;
	data[2] = val & 0xff;

	codec->hw_write(codec->control_data, data, 3);
}

#elif defined(CONFIG_SND_SOC_AK4588_SPI)
static void ak4xxx_adda_hw_write(struct snd_akm4xxx *ak, int chip,
				 unsigned char reg, unsigned char val)
{
	struct snd_soc_ak4588_codec *priv_data = ak->private_data[0];
	struct snd_soc_codec *codec = priv_data->codec;
	u8 buf[2];

	/* data format are
	 *   Byte 0 (Control + Reg address)  - CAD1 CAD0 R/W A4 A3 A2 A1 A0
	 *   Byte 1 (Data)                   - D7 D6 D5 D4 D3 D2 D1 D0
	 */

	/*
	 * 0x01 - CAD1 = 0, CAD0 = 1, When both CAD1/0 = 0, the message is
	 *	for DIT/R part, all other patterns are for the ADC/DAC part.
	 * 0x01 - R/W = 1 is write mode
	 */
	buf[0] = 0x60 | (reg & 0x1f);
	buf[1] = val & 0xff;

	codec->hw_write(codec->control_data, buf, 2);
}

#else
	/* Add other interfaces here */
#endif

/*
 * initialise the AK4XXX driver
 */
int ak4xxx_adda_init(struct snd_soc_device *socdev)
{
	struct snd_soc_codec *codec = socdev->codec;
	struct snd_soc_ak4588_codec *chip = codec->private_data;
	struct snd_akm4xxx *ak = chip->addac;

	ak->card = codec->card;

	/* off, with power on */
	ak4xxx_adda_dapm_event(codec, SNDRV_CTL_POWER_D3hot);

	snd_akm4xxx_build_controls(ak);
	ak4xxx_adda_add_widgets(codec);
	snd_akm4xxx_init(ak);

	return 0;
}



static int ak4xxx_adda_probe(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->codec;
	struct snd_soc_ak4588_codec *ak4588_data = codec->private_data;
	struct snd_akm4xxx *ak;
	int ret = 0;

	printk(KERN_INFO "AK4xxx ADC/DAC %s\n", AK4XXX_VERSION);

	ak = &ak4xxx_codec_data;
	ak->private_data[0] = ak4588_data;
	ak4588_data->addac = ak;

	if (ak->ops.lock == NULL)
		ak->ops.lock = ak4xxx_adda_lock;
	if (ak->ops.unlock == NULL)
		ak->ops.unlock = ak4xxx_adda_unlock;
	if (ak->ops.write == NULL)
		ak->ops.write = ak4xxx_adda_hw_write;

	return ret;
}
static int ak4xxx_adda_remove(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->codec;

	/* power down chip */
	if (codec->control_data)
		ak4xxx_adda_dapm_event(codec, SNDRV_CTL_POWER_D3);

	return 0;
}

static int ak4xxx_adda_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->codec;

	ak4xxx_adda_dapm_event(codec, SNDRV_CTL_POWER_D3cold);

	return 0;
}

static int ak4xxx_adda_resume(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->codec;
	struct snd_soc_ak4588_codec *ak4588_data = codec->private_data;
	struct snd_akm4xxx *ak = ak4588_data->addac;


	/* copy over register cache into hardware */
	snd_akm4xxx_reset(ak, 0);

	ak4xxx_adda_dapm_event(codec, codec->suspend_dapm_state);

	return 0;
}

struct snd_soc_codec_device soc_codec_ak4xxx_adda = {
	.probe = ak4xxx_adda_probe,
	.remove = ak4xxx_adda_remove,
	.suspend = ak4xxx_adda_suspend,
	.resume = ak4xxx_adda_resume,
};
EXPORT_SYMBOL_GPL(soc_codec_ak4xxx_adda);

MODULE_DESCRIPTION("ASoC AK4XXX codec driver");
MODULE_AUTHOR("Steve Chen");
MODULE_LICENSE("GPL");
