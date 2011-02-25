/*
 * ALSA SoC I2S (McBSP) Audio Layer for TI DAVINCI processor
 *
 * Author:      Vladimir Barinov, <vbarinov@ru.mvista.com>
 * Copyright:   (C) 2007 MontaVista Software, Inc., <source@mvista.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/clk.h>

#include <sound/driver.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/initval.h>
#include <sound/soc.h>

#include "davinci-pcm.h"
#include "davinci-i2s.h"
#include "davinci-i2s-mcbsp.h"
#include "davinci-i2s-mcasp.h"

int davinci_i2s_startup(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_cpu_dai *cpu_dai = rtd->dai->cpu_dai;
	struct davinci_audio_dev *dev = rtd->dai->cpu_dai->private_data;

	cpu_dai->dma_data = dev->dma_params[substream->stream];

	return 0;
}
EXPORT_SYMBOL(davinci_i2s_startup);

int davinci_i2s_probe(struct platform_device *pdev)
{
	int tmp, link_cnt;
	int count = 0;
	int backup_count = 0;
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_machine *machine = socdev->machine;
	struct snd_soc_cpu_dai *cpu_dai;
	struct davinci_audio_dev *dev;
	struct resource *mem, *ioarea;
	struct evm_snd_platform_data *parray = pdev->dev.platform_data;
	struct evm_snd_platform_data *pdata;
	struct davinci_pcm_dma_params *dma_data;
	int ret = 0;

	dev = kzalloc(sizeof(struct davinci_audio_dev) * machine->num_links,
		      GFP_KERNEL);
	if (!dev)
		return  -ENOMEM;

	dma_data = kzalloc(sizeof(struct davinci_pcm_dma_params) *
			(machine->num_links << 1), GFP_KERNEL);
	if (!dma_data)
		goto err_release_dev;

	for (link_cnt = 0; link_cnt < machine->num_links; link_cnt++) {
		mem = platform_get_resource(pdev, IORESOURCE_MEM, link_cnt);
		if (!mem) {
			dev_err(&pdev->dev, "no mem resource?\n");
			ret = -ENODEV;
			backup_count = 0;
			goto err_release_data;
		}

		ioarea = request_mem_region(mem->start,
				(mem->end - mem->start) + 1, pdev->name);
		if (!ioarea) {
			dev_err(&pdev->dev, "Audio region already claimed\n");
			ret = -EBUSY;
			backup_count = 1;
			goto err_release_data;
		}


		cpu_dai = machine->dai_link[link_cnt].cpu_dai;
		cpu_dai->private_data = &dev[link_cnt];

		pdata = &parray[link_cnt];
		dev[link_cnt].clk = clk_get(&pdev->dev, pdata->clk_name);

		if (IS_ERR(dev[link_cnt].clk)) {
			ret = -ENODEV;
			backup_count = 2;
			goto err_release_data;
		}
		clk_enable(dev[link_cnt].clk);

		dev[link_cnt].base = (void __iomem *)IO_ADDRESS(mem->start);
		dev[link_cnt].op_mode = pdata->op_mode;
		dev[link_cnt].tdm_slots = pdata->tdm_slots;
		dev[link_cnt].num_serializer = pdata->num_serializer;
		dev[link_cnt].serial_dir = pdata->serial_dir;
		dev[link_cnt].codec_fmt = pdata->codec_fmt;
		dev[link_cnt].sysclk_dir = pdata->sysclk_dir;
		dev[link_cnt].word_shift = pdata->word_shift;
		dev[link_cnt].fixed_slot = pdata->fixed_slot;
		dev[link_cnt].async_rxtx_clk = pdata->async_rxtx_clk;

		dma_data[count].name = "I2S PCM Stereo out";
		dma_data[count].channel = pdata->tx_dma_ch;
		dma_data[count].eventq_no = pdata->eventq_no;
		dma_data[count].dma_addr = (dma_addr_t) (pdata->tx_dma_offset +
					   io_v2p(dev[link_cnt].base));
		dev[link_cnt].dma_params[SNDRV_PCM_STREAM_PLAYBACK] =
				&dma_data[count];


		count++;
		dma_data[count].name = "I2S PCM Stereo in";
		dma_data[count].channel = pdata->rx_dma_ch;
		dma_data[count].eventq_no = pdata->eventq_no;
		dma_data[count].dma_addr = (dma_addr_t)(pdata->rx_dma_offset +
					    io_v2p(dev[link_cnt].base));
		dev[link_cnt].dma_params[SNDRV_PCM_STREAM_CAPTURE] =
				&dma_data[count];
		count++;

	}
	return 0;

err_release_data:
	for (tmp = link_cnt; tmp >= 0; tmp--) {
		if (backup_count > 2)
			clk_disable(dev[tmp].clk);

		if (backup_count > 1) {
			mem = platform_get_resource(pdev, IORESOURCE_MEM, tmp);
			release_mem_region(mem->start,
					   (mem->end - mem->start) + 1);
		}
		backup_count = 3;
	}
	kfree(dma_data);

err_release_dev:
	kfree(dev);

	return ret;
}
EXPORT_SYMBOL(davinci_i2s_probe);

void davinci_i2s_remove(struct platform_device *pdev)
{
	int i;
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_machine *machine = socdev->machine;
	struct snd_soc_cpu_dai *cpu_dai = machine->dai_link->cpu_dai;
	struct davinci_audio_dev *dev_list = cpu_dai->private_data;
	struct davinci_audio_dev *dev;
	struct resource *mem;
	struct davinci_pcm_dma_params *ptr;

	for (i = 0; i < machine->num_links; i++) {
		dev = &dev_list[i];
		clk_disable(dev->clk);
		clk_put(dev->clk);
		dev->clk = NULL;

		mem = platform_get_resource(pdev, IORESOURCE_MEM, i);
		release_mem_region(mem->start, (mem->end - mem->start) + 1);
	}
	ptr = dev_list->dma_params[SNDRV_PCM_STREAM_PLAYBACK];
	kfree(ptr);
	kfree(dev_list);
}
EXPORT_SYMBOL(davinci_i2s_remove);

MODULE_AUTHOR("Vladimir Barinov");
MODULE_DESCRIPTION("TI DAVINCI I2S (McBSP) SoC Interface");
MODULE_LICENSE("GPL");
