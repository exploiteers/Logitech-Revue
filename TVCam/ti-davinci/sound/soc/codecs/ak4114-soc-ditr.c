/*
 * AK4114 DIT/DIR wrapper
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
#include <sound/ak4114.h>
#include <sound/ak4588-soc.h>

#define AUDIO_NAME "AK4114 DIT/R"
#define AK4114_DITR_VERSION "0.1"

#define STUB_RATES	SNDRV_PCM_RATE_48000
#define STUB_FORMATS	(SNDRV_PCM_FMTBIT_S16_LE |  \
			 SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE)

struct snd_soc_codec_dai ak4114_dir_dai = {
	.name = "DIR",
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = 384,
		.rates = STUB_RATES,
		.formats = STUB_FORMATS,
	},
};
EXPORT_SYMBOL_GPL(ak4114_dir_dai);

/*
 * Read AK4114 register cache
 */
unsigned int ak4114_ditr_read(struct snd_soc_codec *codec, unsigned int reg)
{
	struct snd_soc_ak4588_codec *chip = codec->private_data;
	struct ak4114 *ak = chip->spdif;

	return snd_ak4114_reg_read(ak, reg);
}

/*
 * Write to the AK4114 register space
 */
int ak4114_ditr_write(struct snd_soc_codec *codec,
			     unsigned int reg, unsigned int value)
{
	struct snd_soc_ak4588_codec *chip = codec->private_data;
	struct ak4114 *ak = chip->spdif;

	snd_ak4114_reg_write(ak, reg, 0, value);
	return 0;
}

int ak4114_ditr_dapm_event(struct snd_soc_codec *codec, int event)
{
	/*
	 * The entire device can be powered down.
	 * Not sure if that is what we want to do here...
	 */
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

#if defined(CONFIG_SND_SOC_AK4588_I2C)

/*
 * Callbacks from ak4114 driver's reg_read/write that access the hardware
 */
static unsigned char ak4114_ditr_hw_read(void *private_data, unsigned char addr)
{
	/*
	 * Since DA8xx EVM audio card does not use I2C,
	 * this will be left blank for now...
	 */
	return 0;
}

static void ak4114_ditr_hw_write(void *private_data, unsigned char addr,
				 unsigned char data)

{
	struct snd_soc_codec *codec = private_data;
	u8 buf[3];

	/*
	 * Data format is:
	 * byte 1 (slave address) - 0  0  1  0  0  CAD1 CAD0 R/W
	 * byte 2 (reg address)   - *  *  *  A4 A3  A2	 A1  A0
	 * byte 3 (data)	  - D7 D6 D5 D4 D3  D2	 D1  D0
	 */

	/*
	 * 0x20 - Fixed pattern.
	 * 0x02 - CAD1 = 0, CAD0 = 1, When both CAD1/0 = 0, the message is
	 *	  for DIT/R part, all other patterns are for the ADC/DAC part.
	 * 0x01 - R/W = 1 is write mode
	 */
	buf[0] = 0x21;
	buf[1] = addr & 0x1f;
	buf[2] = data;

	codec->hw_write(codec->control_data, buf, 3);
}

#elif defined(CONFIG_SND_SOC_AK4588_SPI)

/*
 * Callbacks from ak4114 driver's reg_read/write that access the hardware
 */
static unsigned char ak4114_ditr_hw_read(void *private_data, unsigned char addr)
{
	struct snd_soc_codec *codec = private_data;
	u8 buf[2];

	/*
	 * Data format is:
	 * byte 0 (control + reg address) - CAD1 CAD0 R/W A4 A3 A2 A1 A0
	 * byte 1 (data)		  -  D7   D6  D5  D4 D3 D2 D1 D0
	 */

	/*
	 * 0x00 - CAD1 = 0, CAD0 = 0; when both CAD1/0 = 0, the message is
	 *	  for DIT/R part, all other patterns are for the ADC/DAC part.
	 * 0x00 - R/W = 0 is read mode
	 */
	buf[0] = addr & 0x1f;
	buf[1] = 0;

	codec->hw_read(codec->control_data, buf, 2);

	return buf[1];
}

static void ak4114_ditr_hw_write(void *private_data, unsigned char addr,
					unsigned char data)
{
	struct snd_soc_codec *codec = private_data;
	u8 buf[2];

	/*
	 * Data format is:
	 * byte 0 (control + reg address) - CAD1 CAD0 R/W A4 A3 A2 A1 A0
	 * byte 1 (data)		  -  D7   D6  D5  D4 D3 D2 D1 D0
	 */

	/*
	 * 0x00 - CAD1 = 0, CAD0 = 0; when both CAD1/0 = 0, the message is
	 *	  for DIT/R part, all other patterns are for the ADC/DAC part.
	 * 0x20 - R/W = 1 is write mode
	 */
	buf[0] = 0x20 | (addr & 0x1f);
	buf[1] = data;

	codec->hw_write(codec->control_data, buf, 2);
}

#else
	/* Add other methods to access the registers here */
#endif

/*
 * Initialise the AK4114 driver
 */
int ak4114_ditr_init(struct snd_soc_device *socdev)
{
	int ret = 0;
	struct snd_soc_codec *codec = socdev->codec;
	struct snd_pcm *pcm;
	struct snd_pcm_substream *pss = NULL, *css = NULL;
	struct list_head *list;
	struct snd_device *dev;
	struct snd_pcm_str *pcm_str;
	struct snd_soc_ak4588_codec *ak4588_data = codec->private_data;
	static unsigned char ak4114_init_vals[] = {
		AK4114_RST | AK4114_PWN | AK4114_OCKS1 | AK4114_CM1,
		AK4114_DEM0 | AK4114_DIF0 | AK4114_DIF2,
		AK4114_TX1E | AK4114_TX0E,
		AK4114_EFH0 | AK4114_DIT,
		AK4117_MQI | AK4117_MAT | AK4117_MCI | AK4117_MDTS |
			AK4117_MPE | AK4117_MAN,
		AK4114_QINT | AK4114_CINT | AK4114_UNLCK | AK4114_PEM |
			AK4114_PAR,
		0
	};
	static unsigned char ak4114_init_txcsb[] = {
		0x0, 0x00, 0x0, 0x00, 0x00
	};

	ret = snd_ak4114_create(codec->card,
				ak4114_ditr_hw_read, ak4114_ditr_hw_write,
				ak4114_init_vals, ak4114_init_txcsb,
				codec, &ak4588_data->spdif);

	if (ret == 0)
		ak4588_data->spdif->using_soc_layer = 1;

	/* off, with power on */
	ak4114_ditr_dapm_event(codec, SNDRV_CTL_POWER_D3hot);

	list_for_each(list, &codec->card->devices) {
		dev = snd_device(list);
		pcm = dev->device_data;
		pcm_str = pcm->streams;
		if (strstr(pcm->id, "DIT"))
			pss = pcm_str[SNDRV_PCM_STREAM_PLAYBACK].substream;

		if (strstr(pcm->id, "DIR"))
			css = pcm_str[SNDRV_PCM_STREAM_CAPTURE].substream;
	}

	if (pss != NULL && css != NULL)
		snd_ak4114_build(ak4588_data->spdif, pss, css);
	else
		printk(KERN_INFO "Play or Capture substream is NULL (%p, %p)\n",
		       pss, css);

	/*
	 * Note that the device can to be partially powered down.
	 * Therefore, no DAPM widgets to initialize...
	 */

	return ret;
}


static int ak4114_ditr_probe(struct platform_device *pdev)
{
	printk(KERN_INFO "AK4114 DIT/DIR %s\n", AK4114_DITR_VERSION);

	return 0;
}

static int ak4114_ditr_remove(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->codec;

	/* power down chip */
	if (codec->control_data)
		ak4114_ditr_dapm_event(codec, SNDRV_CTL_POWER_D3);

	return 0;
}

static int ak4114_ditr_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->codec;

	ak4114_ditr_dapm_event(codec, SNDRV_CTL_POWER_D3cold);

	return 0;
}

static int ak4114_ditr_resume(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->codec;
	struct snd_soc_ak4588_codec *ak4588_data = codec->private_data;

	/* copy over register cache into hardware */
	snd_ak4114_reinit(ak4588_data->spdif);

	ak4114_ditr_dapm_event(codec, codec->suspend_dapm_state);

	return 0;
}

struct snd_soc_codec_device soc_codec_ak4114_ditr = {
	.probe = ak4114_ditr_probe,
	.remove = ak4114_ditr_remove,
	.suspend = ak4114_ditr_suspend,
	.resume = ak4114_ditr_resume,
};
EXPORT_SYMBOL_GPL(soc_codec_ak4114_ditr);

MODULE_DESCRIPTION("ASoC AK4XXX DIT/R wrapper");
MODULE_AUTHOR("Steve Chen");
MODULE_LICENSE("GPL");

