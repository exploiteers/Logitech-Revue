/*
 * AK4588 SOC CODEC wrapper
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
#include <linux/spi/spi.h>
#include <linux/spi/davinci_spi_master.h>
#include <sound/driver.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>
#include <sound/ak4xxx-adda.h>
#include <sound/ak4588-soc.h>
#include <sound/ak4114-soc-ditr.h>
#include "ak4xxx-soc-adda.h"

#define AUDIO_NAME "AK4588"
#define AK4588_VERSION "0.1"

static struct snd_soc_device *ak4588_socdev;

/*
 * Read AK4588 register cache
 */
static unsigned int ak4588_wrapper_read(struct snd_soc_codec *codec,
					unsigned int reg)
{
	if (reg & AK4588_SPDIF_MASK)
		return ak4114_ditr_read(codec, reg & ~AK4588_SPDIF_MASK);
	else
		return ak4xxx_adda_read_cache(codec, reg & ~AK4588_SPDIF_MASK);
}

/*
 * Write to the AK4588 register space
 */
static int ak4588_wrapper_write(struct snd_soc_codec *codec,
				unsigned int reg, unsigned int value)
{
	if (reg & AK4588_SPDIF_MASK)
		return ak4114_ditr_write(codec, reg & ~AK4588_SPDIF_MASK,
					 value);
	else
		return ak4xxx_adda_write(codec, reg & ~AK4588_SPDIF_MASK,
					 value);
}

struct snd_soc_codec_dai ak4588_wrapper_dai[2];
EXPORT_SYMBOL_GPL(ak4588_wrapper_dai);

static int ak4588_wrapper_dapm_event(struct snd_soc_codec *codec, int event)
{
	ak4xxx_adda_dapm_event(codec, event);
	ak4114_ditr_dapm_event(codec, event);

	return 0;
}

/*
 * Initialise the AK4588 driver
 */
static int ak4588_wrapper_init(struct snd_soc_device *socdev)
{
	struct snd_soc_codec *codec = socdev->codec;
	int ret = 0;

	codec->name = "ak4xxx";
	codec->owner = THIS_MODULE;
	codec->read = ak4588_wrapper_read;
	codec->write = ak4588_wrapper_write;
	codec->dapm_event = ak4588_wrapper_dapm_event;
	ak4588_wrapper_dai[0] = ak4xxx_adda_dai,
	ak4588_wrapper_dai[1] = ak4114_dir_dai,
	codec->dai = ak4588_wrapper_dai;
	codec->num_dai = 2;

	/* Register PCMs */
	ret = snd_soc_new_pcms(socdev, SNDRV_DEFAULT_IDX1, SNDRV_DEFAULT_STR1);
	if (ret < 0) {
		printk(KERN_ERR "ak4588: failed to create PCMs\n");
		goto pcm_err;
	}

	ak4xxx_adda_init(socdev);
	ak4114_ditr_init(socdev);

	ret = snd_soc_register_card(socdev);
	if (ret < 0) {
		printk(KERN_ERR "ak4588: failed to register card\n");
		goto card_err;
	}
	return ret;

card_err:
	snd_soc_free_pcms(socdev);
	snd_soc_dapm_free(socdev);
pcm_err:
	return ret;
}

#if defined(CONFIG_SND_SOC_AK4588_I2C)

/*
 * AK4588 2-wire address can be up to 4 devices with device addresses:
 * 0x20/1 - for DIR/DIT block
 * 0x22/3
 * 0x24/5 - for ADC/DAC block
 * 0x26/7
 */
static unsigned short normal_i2c[] = { 0, I2C_CLIENT_END };

/* Magic definition of all other variables and things */
I2C_CLIENT_INSMOD;

static struct i2c_driver ak4588_i2c_driver;
static struct i2c_client client_template;
/*
 * If the I2C layer weren't so broken, we could pass this kind of data around.
 */

static int ak4588_i2c_codec_probe(struct i2c_adapter *adap, int addr, int kind)
{
	struct snd_soc_device *socdev = ak4588_socdev;
	struct ak4xxx_setup_data *setup = socdev->codec_data;
	struct snd_soc_codec *codec = socdev->codec;
	struct i2c_client *i2c;
	int ret;

	if (addr != setup->i2c_address)
		return -ENODEV;

	client_template.adapter = adap;
	client_template.addr = addr;

	i2c = kmemdup(&client_template, sizeof(client_template), GFP_KERNEL);
	if (i2c == NULL) {
		kfree(codec);
		return -ENOMEM;
	}
	i2c_set_clientdata(i2c, codec);
	codec->control_data = i2c;

	ret = i2c_attach_client(i2c);
	if (ret < 0) {
		printk(KERN_ERR "ak4588: failed to attach codec at addr %x\n",
		       addr);
		goto err;
	}

	ret = ak4588_wrapper_init(socdev);
	if (ret < 0) {
		printk(KERN_ERR "ak4588: failed to initialise AK4xxx\n");
		goto err;
	}
	return ret;

err:
	kfree(codec);
	kfree(i2c);
	return ret;
}

static int ak4588_i2c_detach(struct i2c_client *client)
{
	i2c_detach_client(client);
	kfree(client);

	return 0;
}

static int ak4588_i2c_attach(struct i2c_adapter *adap)
{
	return i2c_probe(adap, &addr_data, ak4588_i2c_codec_probe);
}
/* machine i2c codec control layer */
static struct i2c_driver ak4588_i2c_driver = {
	.driver = {
		.name	= "AK4588 I2C Codec",
		.owner	= THIS_MODULE,
	},
	.id		= I2C_DRIVERID_I2CDEV,
	.attach_adapter = ak4588_i2c_attach,
	.detach_client	= ak4588_i2c_detach,
};

static struct i2c_client client_template = {
	.name	= "AK4588",
	.driver = &ak4588_i2c_driver,
};

#elif defined(CONFIG_SND_SOC_AK4588_SPI)

static int ak4588_wrapper_hw_write(void *spi, const char *buf, int len)
{
	spi_write(spi, buf, len);

	return buf[1];
}

static int ak4588_wrapper_hw_read(void *spi, char *buf, int len)
{
	spi_write_then_read(spi, &buf[0], 1, &buf[1], 1);

	return buf[1];
}

static int __devinit ak4588_spi_codec_probe(struct spi_device *spi)
{
	int ret;
	struct snd_soc_device *socdev = ak4588_socdev;
	struct snd_soc_codec *codec = socdev->codec;

	codec->control_data = spi;
	codec->hw_write = ak4588_wrapper_hw_write;
	codec->hw_read = ak4588_wrapper_hw_read;

	ret = ak4588_wrapper_init(socdev);
	if (ret < 0) {
		printk(KERN_ERR "ak4588: failed to initialise AK4xxx\n");
		goto err;
	}
	return ret;

err:
	kfree(codec);
	return -1;
}

static struct spi_driver spi_codec_driver = {
	.driver = {
		   .name = "AK4588 audio codec",
		   .bus = &spi_bus_type,
		   .owner = THIS_MODULE,
		   },
	.probe = ak4588_spi_codec_probe,
};
#endif

static struct snd_soc_ak4588_codec ak4588_codec_data;

static int ak4588_probe(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec;
	int ret = 0;
#ifdef CONFIG_SND_SOC_AK4588_I2C
	struct ak4xxx_setup_data *setup = socdev->codec_data;
#endif

	printk(KERN_INFO "AK4588 Audio Codec %s\n", AK4588_VERSION);

	codec = kzalloc(sizeof(struct snd_soc_codec), GFP_KERNEL);
	if (codec == NULL)
		return -ENOMEM;

	ak4588_codec_data.codec = codec;
	codec->private_data = &ak4588_codec_data;
	socdev->codec = codec;

	mutex_init(&codec->mutex);
	INIT_LIST_HEAD(&codec->dapm_widgets);
	INIT_LIST_HEAD(&codec->dapm_paths);

	ak4588_socdev = socdev;

	soc_codec_ak4xxx_adda.probe(pdev);
	soc_codec_ak4114_ditr.probe(pdev);

#if defined(CONFIG_SND_SOC_AK4588_I2C)
	if (setup->i2c_address) {
		normal_i2c[0] = setup->i2c_address;
		codec->hw_write = (hw_write_t) i2c_master_send;
		ret = i2c_add_driver(&ak4588_i2c_driver);
		if (ret != 0)
			printk(KERN_ERR "can't add i2c driver");
	}
#elif defined(CONFIG_SND_SOC_AK4588_SPI)
	ret = spi_register_driver(&spi_codec_driver);

	if (ak4588_wrapper_read(codec, AK4588_SPDIF_MASK) == 0xff)
		ret = -1;

#else
	/* Add other interfaces here */
#endif
	return ret;
}

static int ak4588_remove(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->codec;

	/* power down chip */
	soc_codec_ak4xxx_adda.remove(pdev);
	soc_codec_ak4114_ditr.remove(pdev);

	snd_soc_free_pcms(socdev);
	snd_soc_dapm_free(socdev);

#if defined(CONFIG_SND_SOC_AK4588_I2C)
	i2c_del_driver(&ak4588_i2c_driver);
#elif defined(CONFIG_SND_SOC_AK4588_SPI)
	spi_unregister_driver(&spi_codec_driver);
#endif
	kfree(codec);

	return 0;
}

static int ak4588_suspend(struct platform_device *pdev, pm_message_t state)
{
	soc_codec_ak4xxx_adda.suspend(pdev, state);
	soc_codec_ak4114_ditr.suspend(pdev, state);
	return 0;
}

static int ak4588_resume(struct platform_device *pdev)
{
	soc_codec_ak4xxx_adda.resume(pdev);
	soc_codec_ak4114_ditr.resume(pdev);
	return 0;
}

struct snd_soc_codec_device soc_codec_dev_ak4588 = {
	.probe = ak4588_probe,
	.remove = ak4588_remove,
	.suspend = ak4588_suspend,
	.resume = ak4588_resume,
};
EXPORT_SYMBOL_GPL(soc_codec_dev_ak4588);

MODULE_DESCRIPTION("ASoC AK4XXX codec driver");
MODULE_AUTHOR("Steve Chen");
MODULE_LICENSE("GPL");

