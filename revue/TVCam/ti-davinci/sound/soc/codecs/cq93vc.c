/*
 * ALSA SoC CQ0093 voice codec driver
 *
 * Copyright (C) 2008 Texas Instruments.
 *
 * Based on sound/soc/codecs/tlv320aic3x.c by Vladimir Barinov
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/io.h>
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

#include <asm/arch/hardware.h>
#include "cq93vc.h"

#define AUDIO_NAME "cq0093"
#define CQ93VC_VERSION "0.1"

int gSampleRate = 0;               // AYK - 1109
// this is the value calculated after reading 
//  DIV2 when 8KHz mono working properly, and calculating voicerate
//  using formula in the cq93vc_hw_params() fcn
//  DIV was 0x09, and I used sampleRate of 8000 in the formula
unsigned int voicerate = 20480000;     // SB - 030510

/* codec private data */
struct cq93vc_priv {
	void __iomem			*base;
	unsigned int sysclk;
};

static inline unsigned int cq93vc_read(struct snd_soc_codec *codec,
						unsigned int reg)
{
	struct cq93vc_priv *cq93vc = codec->private_data;

	return __raw_readb(cq93vc->base + reg);
}

/*
 * write to the cq0093 register space
 */
static inline int cq93vc_write(struct snd_soc_codec *codec, unsigned int reg,
		       u8 value)
{
	struct cq93vc_priv *cq93vc = codec->private_data;

	__raw_writeb(value, cq93vc->base + reg);

	return 0;
}

static int cq93vc_hw_params(struct snd_pcm_substream *substream,
			   struct snd_pcm_hw_params *params)
{       
        int sampleRate;
        unsigned int DIV2,val; 

        sampleRate = params_rate(params);
 
        //printk("CQ93VC: sampleRate = %d\n", sampleRate);
        if(sampleRate != gSampleRate)
        {        
            gSampleRate = sampleRate;


            // 256 x fs = voicerate/(DIV2 + 1)
            // i.e DIV2 = (voicerate/(256 x fs)) - 1;   

            //printk("CQ93VC: voicerate = %x\n", voicerate);
            DIV2 = (voicerate/(256 * sampleRate)) - 1;
        
            val = davinci_readl(DAVINCI_SYSTEM_MODULE_BASE + 0x48);
            //printk("CQ93VC: PERICLK.DIV2 = %x\n", val);

            // clear DIV2 field(bit 15 - 7)
            val &= 0xFFFF007F;   

            val |= DIV2 << 7;  
        
            davinci_writel(val, DAVINCI_SYSTEM_MODULE_BASE + 0x48); 
        }

	return 0;
}

/* Offset from VC_REG00 */
#define VC_REG05	0x14
#define VC_REG09	0x24
#define VC_REG12	0x30

#define POWER_ALL_ON	0xFD

/* Mute bits */
#define PGA_GAIN	0x2
#define DIG_ATTEN	0x3F
#define MUTE_ON		0x40

/* WFIFOCL bit */

#define VC_CTRL_REG_OFFSET 0x04

#define WFIFOCL		0x2000

static const struct snd_kcontrol_new cq93vc_snd_controls[] = {
	SOC_SINGLE("PGA Capture Volume", VC_REG05, 0, 0x3, 0),
	SOC_SINGLE("Mono DAC Playback Volume", VC_REG09, 0, 0x3f, 0),
};

static int cq93vc_mute(struct snd_soc_codec_dai *dai, int mute)
{
	struct snd_soc_codec *codec = dai->codec;
	u8 reg = cq93vc_read(codec, VC_REG09) & ~MUTE_ON;

	if (mute)
		cq93vc_write(codec, VC_REG09, reg | MUTE_ON);
	else
		cq93vc_write(codec, VC_REG09, reg);

	return 0;
}

static int cq93vc_add_controls(struct snd_soc_codec *codec)
{
	int err, i;

	for (i = 0; i < ARRAY_SIZE(cq93vc_snd_controls); i++) {
		err = snd_ctl_add(codec->card,
				  snd_soc_cnew(&cq93vc_snd_controls[i],
					       codec, NULL));
		if (err < 0)
			return err;
	}

	return 0;
}

static int cq93vc_set_dai_sysclk(struct snd_soc_codec_dai *codec_dai,
				int clk_id, unsigned int freq, int dir)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct cq93vc_priv *cq93vc = codec->private_data;

	switch (freq) {
	case 22579200:
	case 27000000:
	case 33868800:
		cq93vc->sysclk = freq;
		return 0;
	}

	return -EINVAL;
}

static int cq93vc_set_dai_fmt(struct snd_soc_codec_dai *codec_dai,
			     unsigned int fmt)
{
	return 0;
}

static int cq93vc_dapm_event(struct snd_soc_codec *codec, int event)
{
	switch (event) {
	case SNDRV_CTL_POWER_D0:
		/* all power is driven by DAPM system */
		cq93vc_write(codec, VC_REG12, POWER_ALL_ON);
		break;
	case SNDRV_CTL_POWER_D1:
	case SNDRV_CTL_POWER_D2:
		break;
	case SNDRV_CTL_POWER_D3hot:
		/*
		 * all power is driven by DAPM system,
		 * so output power is safe if bypass was set
		 */
		cq93vc_write(codec, VC_REG12, 0x0);
		break;
	case SNDRV_CTL_POWER_D3cold:
		/* force all power off */
		cq93vc_write(codec, VC_REG12, 0x0);
		break;
	}
	codec->dapm_state = event;

	return 0;
}

#define CQ93VC_RATES	(SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000)
#define CQ93VC_FORMATS	(SNDRV_PCM_FMTBIT_U8 | SNDRV_PCM_FMTBIT_S16_LE)

struct snd_soc_codec_dai cq93vc_dai = {
	.name = "cq93vc",
	.playback = {
		.stream_name = "Playback",
		.channels_min = 1,
		.channels_max = 2,
		.rates = CQ93VC_RATES,
		.formats = CQ93VC_FORMATS,},
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = 2,
		.rates = CQ93VC_RATES,
		.formats = CQ93VC_FORMATS,},
	.ops = {
		.hw_params = cq93vc_hw_params,
	},
	.dai_ops = {
		.digital_mute = cq93vc_mute,
		.set_sysclk = cq93vc_set_dai_sysclk,
	}
};
EXPORT_SYMBOL_GPL(cq93vc_dai);

struct snd_soc_codec_dai aic3x_dai;
EXPORT_SYMBOL_GPL(aic3x_dai);


static int cq93vc_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->codec;

	cq93vc_dapm_event(codec, SNDRV_CTL_POWER_D3cold);

	return 0;
}

static int cq93vc_resume(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->codec;

	cq93vc_dapm_event(codec, codec->suspend_dapm_state);

	return 0;
}

/*
 * initialise the CQ93VC driver
 * register the mixer and dsp interfaces with the kernel
 */
static int cq93vc_init(struct snd_soc_device *socdev)
{
	struct snd_soc_codec *codec = socdev->codec;
	int ret = 0;
	unsigned int val;

	codec->name = "cq93vc";
	codec->owner = THIS_MODULE;
	codec->read = cq93vc_read;
	codec->write = cq93vc_write;
	codec->dapm_event = cq93vc_dapm_event;
	codec->dai = &cq93vc_dai;
	codec->num_dai = 1;

	/* register pcms */
	ret = snd_soc_new_pcms(socdev, SNDRV_DEFAULT_IDX1, SNDRV_DEFAULT_STR1);
	if (ret < 0) {
		printk(KERN_ERR "cq93vc: failed to create pcms\n");
		goto pcm_err;
	}
	/* Set the PGA Gain to 12 dB */
	cq93vc_write(codec, VC_REG05, PGA_GAIN);
	/* Set the DAC digital attenuation to 0 dB */
	cq93vc_write(codec, VC_REG09, DIG_ATTEN);

#if 0
	val = davinci_readl(DM365_VOICE_CODEC_BASE + VC_CTRL_REG_OFFSET);
	val |= WFIFOCL;  // clear voice codec HW fifo 
	davinci_writel(val, DM365_VOICE_CODEC_BASE + VC_CTRL_REG_OFFSET); 
	/* delay 1 us */
	udelay(1);
	val = davinci_readl(DM365_VOICE_CODEC_BASE + VC_CTRL_REG_OFFSET);
	val &= ~WFIFOCL;  // drop the clear bit 
	davinci_writel(val, DM365_VOICE_CODEC_BASE + VC_CTRL_REG_OFFSET); 

	printk(KERN_INFO "  cq93vc_init: cleared voice codec HW Fifo\n");
#endif

	/* off, with power on */
	cq93vc_dapm_event(codec, SNDRV_CTL_POWER_D3hot);

	cq93vc_add_controls(codec);

	ret = snd_soc_register_card(socdev);
	if (ret < 0) {
		printk(KERN_ERR "cq93vc: failed to register card\n");
		goto card_err;
	}

	return ret;

card_err:
	snd_soc_free_pcms(socdev);
	snd_soc_dapm_free(socdev);
pcm_err:
	return ret;
}

static struct snd_soc_device *cq93vc_socdev;

static int cq93vc_probe(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct cq93vc_setup_data *setup;
	struct snd_soc_codec *codec;
	struct cq93vc_priv *cq93vc;
	struct resource *mem, *ioarea;
	int ret = 0;

	printk(KERN_INFO "CQ0093 Voice Codec %s\n", CQ93VC_VERSION);

	setup = socdev->codec_data;
	codec = kzalloc(sizeof(struct snd_soc_codec), GFP_KERNEL);
	if (codec == NULL)
		return -ENOMEM;

	cq93vc = kzalloc(sizeof(struct cq93vc_priv), GFP_KERNEL);
	if (cq93vc == NULL) {
		kfree(codec);
		return -ENOMEM;
	}

	mem = voice_codec_resources;

	ioarea = request_mem_region(mem->start,
			(mem->end - mem->start) + 1, "voice codec");

	if (!ioarea)
		printk(KERN_INFO "Voice codec region already claimed\n");

	cq93vc->base = (void __iomem *)IO_ADDRESS(mem->start);

	codec->private_data = cq93vc;
	socdev->codec = codec;
	mutex_init(&codec->mutex);
	INIT_LIST_HEAD(&codec->dapm_widgets);
	INIT_LIST_HEAD(&codec->dapm_paths);

	cq93vc_socdev = socdev;

	ret = cq93vc_init(socdev);

	return ret;
}

static int cq93vc_remove(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->codec;
	struct resource *mem;

	/* power down chip */
	if (codec->control_data)
		cq93vc_dapm_event(codec, SNDRV_CTL_POWER_D3);

	mem = voice_codec_resources;
	release_mem_region(mem->start, (mem->end - mem->start) + 1);

	snd_soc_free_pcms(socdev);
	snd_soc_dapm_free(socdev);
	kfree(codec->private_data);
	kfree(codec);

	return 0;
}

struct snd_soc_codec_device soc_codec_dev_cq93vc = {
	.probe = cq93vc_probe,
	.remove = cq93vc_remove,
	.suspend = cq93vc_suspend,
	.resume = cq93vc_resume,
};
EXPORT_SYMBOL_GPL(soc_codec_dev_cq93vc);

struct snd_soc_codec_device soc_codec_dev_aic3x;
EXPORT_SYMBOL_GPL(soc_codec_dev_aic3x);

MODULE_DESCRIPTION("ASoC CQ0093 voice codec driver");
MODULE_AUTHOR("Hui Geng");
MODULE_LICENSE("GPL");
