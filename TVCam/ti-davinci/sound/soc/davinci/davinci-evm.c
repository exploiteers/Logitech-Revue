/*
 * ASoC driver for TI DAVINCI EVM platform
 *
 * Author:      Vladimir Barinov, <vbarinov@ru.mvista.com>
 * Copyright:   (C) 2007 MontaVista Software, Inc., <source@mvista.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <sound/driver.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>

#include <asm/mach-types.h>
#include <asm/dma.h>
#include <asm/arch/hardware.h>

#include "../codecs/tlv320aic3x.h"
#include "../codecs/codec_stubs.h"
#if defined(CONFIG_SND_DM365_INTERNAL_CODEC) || \
	defined(CONFIG_SND_DM365_INTERNAL_CODEC_MODULE)
#include "../codecs/cq93vc.h"
#include "davinci-i2s-vcif.h"
#endif
#include "davinci-pcm.h"
#include "davinci-i2s.h"
#include "davinci-i2s-mcbsp.h"
#include "davinci-i2s-mcasp.h"

#define DM644X_EVM_CODEC_CLOCK 22579200
#define DM355_EVM_CODEC_CLOCK 27000000
#define DM365_EVM_CODEC_CLOCK 27000000
#define DM646X_EVM_CODEC_CLOCK 27000000

static unsigned int evm_codec_clock;

static int evm_hw_params(struct snd_pcm_substream *substream,
			 struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_codec_dai *codec_dai = rtd->dai->codec_dai;
	struct snd_soc_cpu_dai *cpu_dai = rtd->dai->cpu_dai;
	struct davinci_audio_dev *dev = rtd->dai->cpu_dai->private_data;
	int ret = 0;

	/* set codec DAI configuration */
	if (codec_dai->dai_ops.set_fmt != NULL)
		ret = codec_dai->dai_ops.set_fmt(codec_dai, SND_SOC_DAIFMT_I2S |
						 SND_SOC_DAIFMT_CBM_CFM);
	if (ret < 0)
		return ret;

	/* set cpu DAI configuration */
	ret = cpu_dai->dai_ops.set_fmt(cpu_dai, dev->codec_fmt);
	if (ret < 0)
		return ret;

	/* set the codec system clock */
	if (codec_dai->dai_ops.set_sysclk != NULL)
		ret = codec_dai->dai_ops.set_sysclk(codec_dai, 0,
				evm_codec_clock, SND_SOC_CLOCK_OUT);

	if (ret < 0)
		return ret;

	return 0;
}

static struct snd_soc_ops evm_ops = {
	.hw_params = evm_hw_params,
};

/* davinci-evm machine dapm widgets */
static const struct snd_soc_dapm_widget aic3x_dapm_widgets[] = {
	SND_SOC_DAPM_HP("Headphone Jack", NULL),
	SND_SOC_DAPM_LINE("Line Out", NULL),
	SND_SOC_DAPM_MIC("Mic Jack", NULL),
	SND_SOC_DAPM_LINE("Line In", NULL),
};

/* davinci-evm machine audio_mapnections to the codec pins */
static const char *audio_map[][3] = {
	/* Headphone connected to HPLOUT, HPROUT */
	{"Headphone Jack", NULL, "HPLOUT"},
	{"Headphone Jack", NULL, "HPROUT"},

	/* Line Out connected to LLOUT, RLOUT */
	{"Line Out", NULL, "LLOUT"},
	{"Line Out", NULL, "RLOUT"},

	/* Mic connected to (MIC3L | MIC3R) */
	{"MIC3L", NULL, "Mic Bias 2V"},
	{"MIC3R", NULL, "Mic Bias 2V"},
	{"Mic Bias 2V", NULL, "Mic Jack"},

	/* Line In connected to (LINE1L | LINE2L), (LINE1R | LINE2R) */
	{"LINE1L", NULL, "Line In"},
	{"LINE2L", NULL, "Line In"},
	{"LINE1R", NULL, "Line In"},
	{"LINE2R", NULL, "Line In"},

	{NULL, NULL, NULL},
};

/* Logic for a aic3x as connected on a davinci-evm */
static int evm_aic3x_init(struct snd_soc_codec *codec)
{
	int i;

	/* Add davinci-evm specific widgets */
	for (i = 0; i < ARRAY_SIZE(aic3x_dapm_widgets); i++)
		snd_soc_dapm_new_control(codec, &aic3x_dapm_widgets[i]);

	/* Set up davinci-evm specific audio path audio_map */
	for (i = 0; audio_map[i][0] != NULL; i++)
		snd_soc_dapm_connect_input(codec, audio_map[i][0],
					   audio_map[i][1], audio_map[i][2]);

	/* not connected */
	snd_soc_dapm_set_endpoint(codec, "MONO_LOUT", 0);
	snd_soc_dapm_set_endpoint(codec, "HPLCOM", 0);
	snd_soc_dapm_set_endpoint(codec, "HPRCOM", 0);

	/* always connected */
	snd_soc_dapm_set_endpoint(codec, "Headphone Jack", 1);
	snd_soc_dapm_set_endpoint(codec, "Line Out", 1);
	snd_soc_dapm_set_endpoint(codec, "Mic Jack", 1);
	snd_soc_dapm_set_endpoint(codec, "Line In", 1);

	snd_soc_dapm_sync_endpoints(codec);

	return 0;
}

/* davinci-evm digital audio interface glue - connects codec <--> CPU */
static struct snd_soc_dai_link davinci_evm_dai = {
	.name = "TLV320AIC3X",
	.stream_name = "AIC3X",
	.cpu_dai = davinci_i2s_dai,
	.codec_dai = &aic3x_dai,
	.init = evm_aic3x_init,
	.ops = &evm_ops,
};

#if defined(CONFIG_SND_DM365_INTERNAL_CODEC) || \
	defined(CONFIG_SND_DM365_INTERNAL_CODEC_MODULE)
/* dm365-evm on-chip voice codec glue - connects codec <--> CPU */
static struct snd_soc_dai_link cq93_evm_dai = {
	.name = "cq0093vc",
	.stream_name = "cq93",
	.cpu_dai = vcif_i2s_dai,
	.codec_dai = &cq93vc_dai,
};
#endif

static struct snd_soc_dai_link dm646x_evm_dai[] = {
	{
		.name = "TLV320AIC3X",
		.stream_name = "AIC3X",
		.cpu_dai = davinci_iis_mcasp_dai,
		.codec_dai = &aic3x_dai,
		.init = evm_aic3x_init,
		.ops = &evm_ops,
	},
	{
		.name = "MCASP SPDIF",
		.stream_name = "spdif",
		.cpu_dai = davinci_dit_mcasp_dai,
		.codec_dai = dit_stub_dai,
		.ops = &evm_ops,
	},
};

/* davinci dm644x evm audio machine driver */
static struct snd_soc_machine dm644x_snd_soc_machine_evm = {
	.name = "DaVinci EVM",
	.dai_link = &davinci_evm_dai,
	.num_links = 1,
};

/* evm audio private data */
static struct aic3x_setup_data dm644x_evm_aic3x_setup = {
	.i2c_address = 0x1b,
};

/* evm audio subsystem */
static struct snd_soc_device dm644x_evm_snd_devdata = {
	.machine = &dm644x_snd_soc_machine_evm,
	.platform = &davinci_soc_platform,
	.codec_dev = &soc_codec_dev_aic3x,
	.codec_data = &dm644x_evm_aic3x_setup,
};

static struct resource dm644x_evm_snd_resources[] = {
	{
		.start = DAVINCI_MCBSP_BASE,
		.end = DAVINCI_MCBSP_BASE + SZ_8K - 1,
		.flags = IORESOURCE_MEM,
	},
};

static struct evm_snd_platform_data dm644x_evm_snd_data = {
	.clk_name	= "McBSPCLK",
	.tx_dma_ch	= DM644X_DMACH_MCBSP_TX,
	.rx_dma_ch	= DM644X_DMACH_MCBSP_RX,
	.tx_dma_offset	= DAVINCI_MCBSP_DXR_REG,
	.rx_dma_offset	= DAVINCI_MCBSP_DRR_REG,
	.codec_fmt	= SND_SOC_DAIFMT_CBM_CFM | SND_SOC_DAIFMT_IB_NF,
	.eventq_no	= EVENTQ_3,
};

/* davinci dm355 evm audio machine driver */
static struct snd_soc_machine dm355_snd_soc_machine_evm = {
	.name = "DaVinci DM355 EVM",
	.dai_link = &davinci_evm_dai,
	.num_links = 1,
};

/* evm audio private data */
static struct aic3x_setup_data dm355_evm_aic3x_setup = {
	.i2c_address = 0x1b,
};

/* evm audio subsystem */
static struct snd_soc_device dm355_evm_snd_devdata = {
	.machine = &dm355_snd_soc_machine_evm,
	.platform = &davinci_soc_platform,
	.codec_dev = &soc_codec_dev_aic3x,
	.codec_data = &dm355_evm_aic3x_setup,
};

static struct resource dm355_evm_snd_resources[] = {
	{
		.start = DM355_MCBSP1_BASE,
		.end = DM355_MCBSP1_BASE + SZ_8K - 1,
		.flags = IORESOURCE_MEM,
	},
};

static struct evm_snd_platform_data dm355_evm_snd_data = {
	.clk_name	= "McBSPCLK1",
	.tx_dma_ch	= DM355_DMA_MCBSP1_TX,
	.rx_dma_ch	= DM355_DMA_MCBSP1_RX,
	.tx_dma_offset	= DAVINCI_MCBSP_DXR_REG,
	.rx_dma_offset	= DAVINCI_MCBSP_DRR_REG,
	.codec_fmt	= SND_SOC_DAIFMT_CBM_CFM | SND_SOC_DAIFMT_IB_NF,
	.eventq_no	= EVENTQ_3,
};

#if defined(CONFIG_SND_DM365_EXTERNAL_CODEC) || \
	defined(CONFIG_SND_DM365_EXTERNAL_CODEC_MODULE)
/* davinci dm365 evm audio machine driver */
static struct snd_soc_machine dm365_snd_soc_machine_evm = {
	.name = "DaVinci DM365 EVM",
	.dai_link = &davinci_evm_dai,
	.num_links = 1,
};

/* evm audio private data */
static struct aic3x_setup_data dm365_evm_aic3x_setup = {
	.i2c_address = 0x18,
};

/* evm audio subsystem */
static struct snd_soc_device dm365_evm_snd_devdata = {
	.machine = &dm365_snd_soc_machine_evm,
	.platform = &davinci_soc_platform,
	.codec_dev = &soc_codec_dev_aic3x,
	.codec_data = &dm365_evm_aic3x_setup,
};

static struct resource dm365_evm_snd_resources[] = {
	{
		.start = DM365_MCBSP_BASE,
		.end = DM365_MCBSP_BASE + SZ_8K - 1,
		.flags = IORESOURCE_MEM,
	},
};

static struct evm_snd_platform_data dm365_evm_snd_data = {
	.clk_name	= "McBSPCLK",
	.tx_dma_ch	= DM365_DMA_MCBSP_TX,
	.rx_dma_ch	= DM365_DMA_MCBSP_RX,
	.tx_dma_offset	= DAVINCI_MCBSP_DXR_REG,
	.rx_dma_offset	= DAVINCI_MCBSP_DRR_REG,
	.codec_fmt	= SND_SOC_DAIFMT_CBM_CFM | SND_SOC_DAIFMT_IB_NF,
	.eventq_no	= EVENTQ_3,
};
#endif

#if defined(CONFIG_SND_DM365_INTERNAL_CODEC) || \
	defined(CONFIG_SND_DM365_INTERNAL_CODEC_MODULE)
/* davinci dm365 evm audio machine driver */
static struct snd_soc_machine cq93_snd_soc_machine_evm = {
	.name = "On-chip voice codec",
	.dai_link = &cq93_evm_dai,
	.num_links = 1,
};

/* evm audio subsystem */
static struct snd_soc_device cq93_vc_snd_devdata = {
	.machine = &cq93_snd_soc_machine_evm,
	.platform = &davinci_soc_platform,
	.codec_dev = &soc_codec_dev_cq93vc,
};

static struct resource cq93_vc_snd_resources[] = {
	{
		.start = DM365_VOICE_CODEC_BASE,
		.end = DM365_VOICE_CODEC_BASE + (SZ_1K >> 4) - 1,
		.flags = IORESOURCE_MEM,
	},
};

struct resource voice_codec_resources[] = {
	{
		.start = DM365_VOICE_CODEC_BASE + 0x80,
		.end = DM365_VOICE_CODEC_BASE + 0x80 + (SZ_1K >> 3) - 1,
		.flags = IORESOURCE_MEM,
	},
};
EXPORT_SYMBOL_GPL(voice_codec_resources);

static struct evm_snd_platform_data cq93_vc_snd_data = {
	.clk_name	= "VOICECODEC_CLK",
	.tx_dma_ch	= DM365_DMA_VCIF_TX,
	.rx_dma_ch	= DM365_DMA_VCIF_RX,
	.tx_dma_offset	= DAVINCI_VCIF_WFIFO_REG,
	.rx_dma_offset	= DAVINCI_VCIF_RFIFO_REG,
	.codec_fmt	= SND_SOC_DAIFMT_CBM_CFM | SND_SOC_DAIFMT_IB_NF,
	.eventq_no	= EVENTQ_3,
};
#endif
/* davinci dm646x evm audio machine driver */
static struct snd_soc_machine dm646x_snd_soc_machine_evm = {
	.name = "DaVinci DM6467 EVM",
	.dai_link = dm646x_evm_dai,
	.num_links = 2,
};

/* evm audio private data */
static struct aic3x_setup_data dm646x_evm_aic3x_setup = {
	.i2c_address = 0x18,
};

/* evm audio subsystem */
static struct snd_soc_device dm646x_evm_snd_devdata[] = {
	{
		.machine = &dm646x_snd_soc_machine_evm,
		.platform = &davinci_soc_platform,
		.codec_dev = &soc_codec_dev_aic3x,
		.codec_data = &dm646x_evm_aic3x_setup,
	},
};

static struct resource dm646x_evm_snd_resources[] = {
	{
		.start = DAVINCI_DM646X_MCASP0_REG_BASE,
		.end = DAVINCI_DM646X_MCASP0_REG_BASE + (SZ_1K << 1) - 1,
		.flags = IORESOURCE_MEM,
	},
	{
		.start = DAVINCI_DM646X_MCASP1_REG_BASE,
		.end = DAVINCI_DM646X_MCASP1_REG_BASE + (SZ_1K << 1) - 1,
		.flags = IORESOURCE_MEM,
	},
};

static u8 dm646x_iis_serializer_direction[] = {
	TX_MODE, RX_MODE, INACTIVE_MODE, INACTIVE_MODE,
};

static u8 dm646x_dit_serializer_direction[] = {
	TX_MODE, INACTIVE_MODE, INACTIVE_MODE, INACTIVE_MODE,
};

static struct evm_snd_platform_data dm646x_evm_snd_data[] = {
	{
		.clk_name	= "McASPCLK0",
		.tx_dma_ch	= DAVINCI_DM646X_DMA_MCASP0_AXEVT0,
		.rx_dma_ch	= DAVINCI_DM646X_DMA_MCASP0_AREVT0,
		.tx_dma_offset	= 0x400,
		.rx_dma_offset	= 0x400,
		.op_mode	= DAVINCI_MCASP_IIS_MODE,
		.num_serializer	= 4,
		.tdm_slots	= 2,
		.serial_dir	= dm646x_iis_serializer_direction,
		.eventq_no	= EVENTQ_3,
		.codec_fmt	= SND_SOC_DAIFMT_CBM_CFM | SND_SOC_DAIFMT_IB_NF,
	},
	{
		.clk_name	= "McASPCLK1",
		.tx_dma_ch	= DAVINCI_DM646X_DMA_MCASP1_AXEVT1,
		.rx_dma_ch	= -1,
		.tx_dma_offset	= 0x400,
		.rx_dma_offset	= 0,
		.op_mode	= DAVINCI_MCASP_DIT_MODE,
		.num_serializer	= 4,
		.tdm_slots	= 32,
		.serial_dir	= dm646x_dit_serializer_direction,
		.eventq_no	= EVENTQ_3,
		.codec_fmt	= SND_SOC_DAIFMT_CBS_CFS | SND_SOC_DAIFMT_IB_NF,
	},
};

static struct platform_device *evm_snd_device;

static int __init evm_init(void)
{
	int ret = 0;
	int res_size;
	struct snd_soc_device *evm_snd_devdata;
	struct resource *evm_snd_resources;
	struct evm_snd_platform_data *evm_snd_data;

	if (cpu_is_davinci_dm355()) {
		evm_snd_devdata = &dm355_evm_snd_devdata;
		evm_snd_resources = dm355_evm_snd_resources;
		res_size = ARRAY_SIZE(dm355_evm_snd_resources);
		evm_snd_data = &dm355_evm_snd_data;
		evm_codec_clock = DM355_EVM_CODEC_CLOCK;
	} else if (cpu_is_davinci_dm365()) {
#if defined(CONFIG_SND_DM365_EXTERNAL_CODEC) || \
	defined(CONFIG_SND_DM365_EXTERNAL_CODEC_MODULE)
		evm_snd_devdata = &dm365_evm_snd_devdata;
		evm_snd_resources = dm365_evm_snd_resources;
		res_size = ARRAY_SIZE(dm365_evm_snd_resources);
		evm_snd_data = &dm365_evm_snd_data;
		evm_codec_clock = DM365_EVM_CODEC_CLOCK;
#elif defined(CONFIG_SND_DM365_INTERNAL_CODEC) || \
	defined(CONFIG_SND_DM365_INTERNAL_CODEC_MODULE)
		evm_snd_devdata = &cq93_vc_snd_devdata;
		evm_snd_resources = cq93_vc_snd_resources;
		res_size = ARRAY_SIZE(cq93_vc_snd_resources);
		evm_snd_data = &cq93_vc_snd_data;
#endif
	} else if (cpu_is_davinci_dm6467()) {
		evm_snd_devdata = dm646x_evm_snd_devdata;
		evm_snd_resources = dm646x_evm_snd_resources;
		res_size = ARRAY_SIZE(dm646x_evm_snd_resources);
		evm_snd_data = dm646x_evm_snd_data;
		evm_codec_clock = DM646X_EVM_CODEC_CLOCK;
	} else {
		if (!cpu_is_davinci_dm644x() && !cpu_is_davinci_dm357())
			BUG();
		evm_snd_devdata = &dm644x_evm_snd_devdata;
		evm_snd_resources = dm644x_evm_snd_resources;
		res_size = ARRAY_SIZE(dm644x_evm_snd_resources);
		evm_snd_data = &dm644x_evm_snd_data;
		evm_codec_clock = DM644X_EVM_CODEC_CLOCK;
	}

	evm_snd_device = platform_device_alloc("soc-audio", -1);
	if (!evm_snd_device)
		return -ENOMEM;

	platform_set_drvdata(evm_snd_device, evm_snd_devdata);
	evm_snd_devdata->dev = &evm_snd_device->dev;
	evm_snd_device->dev.platform_data = evm_snd_data;

	ret = platform_device_add_resources(evm_snd_device,
					    evm_snd_resources, res_size);

	if (ret) {
		platform_device_put(evm_snd_device);
		return ret;
	}

	ret = platform_device_add(evm_snd_device);
	if (ret)
		platform_device_put(evm_snd_device);

	return ret;
}

static void __exit evm_exit(void)
{
	platform_device_unregister(evm_snd_device);
}

module_init(evm_init);
module_exit(evm_exit);

MODULE_AUTHOR("Vladimir Barinov");
MODULE_DESCRIPTION("TI DAVINCI EVM ASoC driver");
MODULE_LICENSE("GPL");
