/*
 * ALSA SoC I2S (McASP) Audio Layer for TI DAVINCI processor
 *
 * Author: Nirmal Pandey <n-pandey@ti.com>,
 *         Suresh Rajashekara <suresh.r@ti.com>
 *         Steve Chen, <schen@.mvista.com>
 *
 * Copyright:   (C) 2008 MontaVista Software, Inc., <source@mvista.com>
 * Copyright:   (C) 2007  Texas Instruments, India
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
#include "davinci-i2s-mcasp.h"

#include <linux/interrupt.h>
#include <asm/arch/irqs.h>

static inline void mcasp_set_bits(void __iomem *reg, u32 val)
{
	__raw_writel(__raw_readl(reg) | val, reg);
}

static inline void mcasp_clr_bits(void __iomem *reg, u32 val)
{
	__raw_writel((__raw_readl(reg) & ~(val)), reg);
}

static inline void mcasp_mod_bits(void __iomem *reg, u32 val, u32 mask)
{
	__raw_writel((__raw_readl(reg) & ~mask) | (val & mask), reg);
}

static inline void mcasp_set_reg(void __iomem *reg, u32 val)
{
	__raw_writel(val, reg);
}

static inline u32 mcasp_get_reg(void __iomem *reg)
{
	return __raw_readl(reg);
}

static inline void mcasp_set_ctl_reg(void __iomem *regs, u32 val)
{
	mcasp_set_bits(regs, val);
	while ((mcasp_get_reg(regs) & val) != val);
}

static irqreturn_t mcasp_irq_handler(int irq, void *dev_id,
				struct pt_regs *regs)
{
	struct platform_device *pdev = dev_id;
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_machine *machine = socdev->machine;
	struct snd_soc_cpu_dai *cpu_dai;
	struct davinci_audio_dev *dev;
	int link_cnt;
	void __iomem *base;

	for (link_cnt = 0; link_cnt < machine->num_links; link_cnt++) {
		cpu_dai = machine->dai_link[link_cnt].cpu_dai;
		dev = cpu_dai->private_data;
		base = dev->base;

		if (mcasp_get_reg(base + DAVINCI_MCASP_TXSTAT_REG) & 0x1) {
			mcasp_clr_bits(base + DAVINCI_MCASP_EVTCTLX_REG, 1);
			schedule_delayed_work(&dev->workq, HZ >> 4);
		}
	}

	return IRQ_HANDLED;
}

static void davinci_mcasp_workq_handler(void *arg)
{
	struct davinci_audio_dev *dev = arg;
	void __iomem *base = dev->base;

	mcasp_clr_bits(base + DAVINCI_MCASP_GBLCTL_REG, TXSERCLR | TXSMRST);

	mcasp_set_ctl_reg(base + DAVINCI_MCASP_GBLCTLX_REG, TXSERCLR);
	mcasp_set_reg(base + DAVINCI_MCASP_TXBUF_REG, 0);
	mcasp_set_reg(base + DAVINCI_MCASP_TXSTAT_REG, 1);
	mcasp_set_ctl_reg(base + DAVINCI_MCASP_GBLCTLX_REG, TXSMRST);

	mcasp_set_ctl_reg(base + DAVINCI_MCASP_EVTCTLX_REG, 1);

}
static int davinci_mcasp_i2s_probe(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_machine *machine = socdev->machine;
	struct snd_soc_cpu_dai *cpu_dai;
	struct davinci_audio_dev *dev;
	int ret;
	int link_cnt;

	ret = davinci_i2s_probe(pdev);

	if (ret < 0)
		return ret;

	for (link_cnt = 0; link_cnt < machine->num_links; link_cnt++) {
		cpu_dai = machine->dai_link[link_cnt].cpu_dai;
		dev = cpu_dai->private_data;

		mcasp_set_ctl_reg(dev->base + DAVINCI_MCASP_EVTCTLX_REG, 1);

		INIT_WORK(&dev->workq, davinci_mcasp_workq_handler, dev);
	}
	request_irq(IRQ_DA8XX_MCASPINT, mcasp_irq_handler, 0,
			  "MCASP status", pdev);
	return ret;
}

void davinci_mcasp_i2s_remove(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_machine *machine = socdev->machine;
	struct snd_soc_cpu_dai *cpu_dai;
	struct davinci_audio_dev *dev;
	int link_cnt;

	for (link_cnt = 0; link_cnt < machine->num_links; link_cnt++) {
		cpu_dai = machine->dai_link[link_cnt].cpu_dai;
		dev = cpu_dai->private_data;

		mcasp_clr_bits(dev->base + DAVINCI_MCASP_EVTCTLX_REG, 1);
		cancel_delayed_work(&dev->workq);
	}
	flush_scheduled_work();
	davinci_i2s_remove(pdev);
}

void mcasp_start_rx(struct davinci_audio_dev *dev)
{
	mcasp_set_ctl_reg(dev->base + DAVINCI_MCASP_GBLCTLR_REG, RXHCLKRST);
	mcasp_set_ctl_reg(dev->base + DAVINCI_MCASP_GBLCTLR_REG, RXCLKRST);
	mcasp_set_ctl_reg(dev->base + DAVINCI_MCASP_GBLCTLR_REG, RXSERCLR);
	mcasp_set_reg(dev->base + DAVINCI_MCASP_RXBUF_REG, 0);

	mcasp_set_ctl_reg(dev->base + DAVINCI_MCASP_GBLCTLR_REG, RXSMRST);
	mcasp_set_ctl_reg(dev->base + DAVINCI_MCASP_GBLCTLR_REG, RXFSRST);
	mcasp_set_reg(dev->base + DAVINCI_MCASP_RXBUF_REG, 0);

	mcasp_set_ctl_reg(dev->base + DAVINCI_MCASP_GBLCTLR_REG, RXSMRST);
	mcasp_set_ctl_reg(dev->base + DAVINCI_MCASP_GBLCTLR_REG, RXFSRST);
}

void mcasp_start_tx(struct davinci_audio_dev *dev)
{
	u8 offset = 0, i;
	u32 cnt;

	mcasp_set_ctl_reg(dev->base + DAVINCI_MCASP_GBLCTLX_REG, TXHCLKRST);
	mcasp_set_ctl_reg(dev->base + DAVINCI_MCASP_GBLCTLX_REG, TXCLKRST);
	mcasp_set_ctl_reg(dev->base + DAVINCI_MCASP_GBLCTLX_REG, TXSERCLR);
	mcasp_set_reg(dev->base + DAVINCI_MCASP_TXBUF_REG, 0);

	mcasp_set_ctl_reg(dev->base + DAVINCI_MCASP_GBLCTLX_REG, TXSMRST);
	mcasp_set_ctl_reg(dev->base + DAVINCI_MCASP_GBLCTLX_REG, TXFSRST);
	mcasp_set_reg(dev->base + DAVINCI_MCASP_TXBUF_REG, 0);

	for (i = 0; i < dev->num_serializer; i++) {
		if (dev->serial_dir[i] == TX_MODE) {
			offset = i;
			break;
		}
	}
	/* wait for TX ready */
	cnt = 0;
	while (!(mcasp_get_reg(dev->base + DAVINCI_MCASP_XRSRCTL_REG(offset)) &
		 TXSTATE) && (cnt < 100000))
		cnt++;

	mcasp_set_reg(dev->base + DAVINCI_MCASP_TXBUF_REG, 0);
}

static void davinci_mcasp_start(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct davinci_audio_dev *dev = rtd->dai->cpu_dai->private_data;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		mcasp_start_tx(dev);
	else
		mcasp_start_rx(dev);
}

void mcasp_stop_rx(struct davinci_audio_dev *dev)
{
	mcasp_set_reg(dev->base + DAVINCI_MCASP_GBLCTLR_REG, 0);
	mcasp_set_reg(dev->base + DAVINCI_MCASP_RXSTAT_REG, 0xFFFFFFFF);
}

void mcasp_stop_tx(struct davinci_audio_dev *dev)
{
	mcasp_set_reg(dev->base + DAVINCI_MCASP_GBLCTLX_REG, 0);
	mcasp_set_reg(dev->base + DAVINCI_MCASP_TXSTAT_REG, 0xFFFFFFFF);
}

static void davinci_mcasp_stop(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct davinci_audio_dev *dev = rtd->dai->cpu_dai->private_data;

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		mcasp_stop_tx(dev);
	else
		mcasp_stop_rx(dev);
}

static int davinci_i2s_mcasp_set_dai_fmt(struct snd_soc_cpu_dai *cpu_dai,
					 unsigned int fmt)
{
	struct davinci_audio_dev *dev = cpu_dai->private_data;
	void __iomem *base = dev->base;

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBS_CFS:
		/* codec is clock and frame slave */
		mcasp_set_bits(base + DAVINCI_MCASP_ACLKXCTL_REG, ACLKXE);
		mcasp_set_bits(base + DAVINCI_MCASP_TXFMCTL_REG, AFSXE);

		mcasp_set_bits(base + DAVINCI_MCASP_ACLKRCTL_REG, ACLKRE);
		mcasp_set_bits(base + DAVINCI_MCASP_RXFMCTL_REG, AFSRE);

		mcasp_set_bits(base + DAVINCI_MCASP_PDIR_REG, (0x07 << 26));
		break;
	case SND_SOC_DAIFMT_CBM_CFS:
		/* codec is clock master and frame slave */
		mcasp_set_bits(base + DAVINCI_MCASP_ACLKXCTL_REG, ACLKXE);
		mcasp_set_bits(base + DAVINCI_MCASP_TXFMCTL_REG, AFSXE);

		mcasp_set_bits(base + DAVINCI_MCASP_ACLKRCTL_REG, ACLKRE);
		mcasp_set_bits(base + DAVINCI_MCASP_RXFMCTL_REG, AFSRE);

		mcasp_set_bits(base + DAVINCI_MCASP_PDIR_REG, (0x2d << 26));
		break;
	case SND_SOC_DAIFMT_CBM_CFM:
		/* codec is clock and frame master */
		mcasp_clr_bits(base + DAVINCI_MCASP_ACLKXCTL_REG, ACLKXE);
		mcasp_clr_bits(base + DAVINCI_MCASP_TXFMCTL_REG, AFSXE);

		mcasp_clr_bits(base + DAVINCI_MCASP_ACLKRCTL_REG, ACLKRE);
		mcasp_clr_bits(base + DAVINCI_MCASP_RXFMCTL_REG, AFSRE);

		mcasp_clr_bits(base + DAVINCI_MCASP_PDIR_REG, (0x3f << 26));
		break;
	default:
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_IB_NF:
		mcasp_clr_bits(base + DAVINCI_MCASP_ACLKXCTL_REG, ACLKXPOL);
		mcasp_clr_bits(base + DAVINCI_MCASP_TXFMCTL_REG, FSXPOL);

		mcasp_set_bits(base + DAVINCI_MCASP_ACLKRCTL_REG, ACLKRPOL);
		mcasp_clr_bits(base + DAVINCI_MCASP_RXFMCTL_REG, FSRPOL);
		break;
	case SND_SOC_DAIFMT_NB_IF:
		mcasp_set_bits(base + DAVINCI_MCASP_ACLKXCTL_REG, ACLKXPOL);
		mcasp_set_bits(base + DAVINCI_MCASP_TXFMCTL_REG, FSXPOL);

		mcasp_set_bits(base + DAVINCI_MCASP_ACLKRCTL_REG, ACLKRPOL);
		mcasp_set_bits(base + DAVINCI_MCASP_RXFMCTL_REG, FSRPOL);
		break;
	case SND_SOC_DAIFMT_IB_IF:
		mcasp_clr_bits(base + DAVINCI_MCASP_ACLKXCTL_REG, ACLKXPOL);
		mcasp_set_bits(base + DAVINCI_MCASP_TXFMCTL_REG, FSXPOL);

		mcasp_set_bits(base + DAVINCI_MCASP_ACLKRCTL_REG, ACLKRPOL);
		mcasp_set_bits(base + DAVINCI_MCASP_RXFMCTL_REG, FSRPOL);
		break;
	case SND_SOC_DAIFMT_NB_NF:
		mcasp_set_bits(base + DAVINCI_MCASP_ACLKXCTL_REG, ACLKXPOL);
		mcasp_clr_bits(base + DAVINCI_MCASP_TXFMCTL_REG, FSXPOL);

		mcasp_clr_bits(base + DAVINCI_MCASP_ACLKRCTL_REG, ACLKRPOL);
		mcasp_clr_bits(base + DAVINCI_MCASP_RXFMCTL_REG, FSRPOL);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}
static int davinci_i2s_mcasp_set_dai_sysclk(struct snd_soc_cpu_dai *cpu_dai,
					    int clk_id, unsigned int freq,
					    int dir)
{
	struct davinci_audio_dev *dev = cpu_dai->private_data;
	void __iomem *base = dev->base;

	if (freq != 24576000)
		return -1;

	dev->sysclk_freq = freq;
	dev->sysclk_dir = dir;

	if (dir == SND_SOC_CLOCK_IN) {
		/* AHCLK is the clock input from codec */
		mcasp_clr_bits(base + DAVINCI_MCASP_AHCLKXCTL_REG, AHCLKXE);
		mcasp_clr_bits(base + DAVINCI_MCASP_PDIR_REG, (0x1 << 27));

		mcasp_clr_bits(base + DAVINCI_MCASP_AHCLKRCTL_REG, AHCLKRE);
		mcasp_clr_bits(base + DAVINCI_MCASP_PDIR_REG, (0x1 << 30));

	} else {
		/* AHCLK is the clock output to codec */
		mcasp_set_bits(base + DAVINCI_MCASP_AHCLKXCTL_REG, AHCLKXE);
		mcasp_set_bits(base + DAVINCI_MCASP_PDIR_REG, (0x1 << 27));

		mcasp_set_bits(base + DAVINCI_MCASP_AHCLKRCTL_REG, AHCLKRE);
		mcasp_set_bits(base + DAVINCI_MCASP_PDIR_REG, (0x1 << 30));

	}
	return 0;
}

static int davinci_config_channel_size(struct snd_pcm_substream *substream,
				       int channel_size, int slot_ext)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct davinci_audio_dev *dev = rtd->dai->cpu_dai->private_data;
	void __iomem *base = dev->base;
	u32 fmt = 0;
	u32 mask;
	int slot_size = 0;

	switch (channel_size) {
	case DAVINCI_AUDIO_WORD_8:
		fmt = 0x03;
		mask = 0x000000FF;
		slot_size = 8;
		break;

	case DAVINCI_AUDIO_WORD_12:
		fmt = 0x05;
		mask = 0x00000FFF;
		slot_size = 12;
		break;

	case DAVINCI_AUDIO_WORD_16:
		fmt = 0x07;
		mask = 0x0000FFFF;
		slot_size = 16;
		break;

	case DAVINCI_AUDIO_WORD_20:
		fmt = 0x09;
		mask = 0x000FFFFF;
		slot_size = 20;
		break;

	case DAVINCI_AUDIO_WORD_24:
		fmt = 0x0B;
		mask = 0x00FFFFFF;
		slot_size = 24;
		break;

	case DAVINCI_AUDIO_WORD_28:
		fmt = 0x0D;
		mask = 0x0FFFFFFF;
		slot_size = 28;
		break;

	case DAVINCI_AUDIO_WORD_32:
		fmt = 0x0F;
		mask = 0xFFFFFFFF;
		slot_size = 32;
		break;

	default:
		/* should never get here since the value already checked */
		return -1;
	}

	if (dev->word_shift) {
		mcasp_set_bits(base + DAVINCI_MCASP_TXFMT_REG, TXROT(4));
		mcasp_set_bits(base + DAVINCI_MCASP_RXFMT_REG, RXROT(4));
		mcasp_set_reg(base + DAVINCI_MCASP_TXMASK_REG, mask);
		mcasp_set_reg(base + DAVINCI_MCASP_RXMASK_REG, mask);
		mcasp_set_bits(base + DAVINCI_MCASP_TXFMT_REG, FSXDLY(1));
	} else {
		mask = 0xFFFFFFFF;
		if (channel_size > DAVINCI_AUDIO_WORD_16) {
			mcasp_set_bits(base + DAVINCI_MCASP_TXFMT_REG,
				       TXROT(4));
			mcasp_set_bits(base + DAVINCI_MCASP_RXFMT_REG,
				       RXROT(4));
		}
		mcasp_set_reg(base + DAVINCI_MCASP_TXMASK_REG, mask);
		mcasp_set_reg(base + DAVINCI_MCASP_RXMASK_REG, mask);
	}

	/*
	 * Overwrite if slot size if codec only supports specific slot sizes
	 * Also, check for the need to extend the slot size.  This is to
	 * handle SNDRV_PCM_FORMAT_S24_LE which needs 32 bit slot whereas
	 * SNDRV_PCM_FORMAT_S24_3LE fits into 24 bit slot
	 */

	if (dev->fixed_slot || slot_ext) {
		slot_size = 32;
		fmt = 0x0F;
	}

	mcasp_mod_bits(base + DAVINCI_MCASP_RXFMT_REG, RXSSZ(fmt), RXSSZ(0xF));
	mcasp_mod_bits(base + DAVINCI_MCASP_TXFMT_REG, TXSSZ(fmt), TXSSZ(0xF));

	return slot_size;
}

static int davinci_config_clock(struct snd_pcm_substream *substream,
				int rate, int slot_size)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct davinci_audio_dev *dev = rtd->dai->cpu_dai->private_data;
	void __iomem *base = dev->base;
	u32 bclk, div;

	/* Update clock only if the timer is set to internal */
	if ((mcasp_get_reg(base + DAVINCI_MCASP_ACLKXCTL_REG) & ACLKXE) == 0)
		return 0;

	/*
	 * bclk = rate * #tdm_slots * (slot_size)
	 * div = (sysclk / bclk) - 1
	 *
	 * for example
	 *    rate = 48000  - rate sometime known as fs or sample rate
	 *    tdm_slot = 2  - standard i2s format
	 *    slot_size = 32 - standard i2s format for bits per slot
	 *    sysclk = 24576000  - from hardware spec.
	 *
	 *    bclk =  48000 * 2 * 32 = 3072000
	 *    div = (24576000 / 3072000) - 1 = 7
	 */
	bclk = rate * dev->tdm_slots * slot_size;
	div = (dev->sysclk_freq / bclk) - 1;

	mcasp_mod_bits(base + DAVINCI_MCASP_ACLKXCTL_REG, ACLKXDIV(div), 0x1F);
	mcasp_mod_bits(base + DAVINCI_MCASP_ACLKRCTL_REG, ACLKRDIV(div), 0x1F);
	return 0;
}

static void davinci_hw_common_param(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct davinci_audio_dev *dev = rtd->dai->cpu_dai->private_data;
	struct davinci_pcm_dma_params *dma_params =
					dev->dma_params[substream->stream];
	void __iomem *base = dev->base;
	int i;
	int num_data_stream = 0;

	/* Default configuration */
	mcasp_set_bits(base + DAVINCI_MCASP_PWREMUMGT_REG, SOFT);

	/* All PINS as McASP */
	mcasp_set_reg(base + DAVINCI_MCASP_PFUNC_REG, 0x00000000);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		mcasp_set_reg(base + DAVINCI_MCASP_TXSTAT_REG, 0xFFFFFFFF);
		mcasp_clr_bits(base + DAVINCI_MCASP_XEVTCTL_REG, TXDATADMADIS);
	} else {
		mcasp_set_reg(base + DAVINCI_MCASP_RXSTAT_REG, 0xFFFFFFFF);
		mcasp_clr_bits(base + DAVINCI_MCASP_REVTCTL_REG, RXDATADMADIS);
	}

	for (i = 0; i < dev->num_serializer; i++) {
		mcasp_mod_bits(base + DAVINCI_MCASP_XRSRCTL_REG(i),
			       dev->serial_dir[i], 0x3);
		if (dev->serial_dir[i] == TX_MODE) {
			mcasp_set_bits(base + DAVINCI_MCASP_PDIR_REG, AXR(i));
			if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
				num_data_stream++;
		} else {
			mcasp_clr_bits(base + DAVINCI_MCASP_PDIR_REG, AXR(i));
			if ((dev->serial_dir[i] == RX_MODE) &&
			   (substream->stream == SNDRV_PCM_STREAM_CAPTURE))
				num_data_stream++;
		}
	}
	dma_params->num_data_stream = num_data_stream;
}
static void davinci_hw_iis_param(struct snd_pcm_substream *substream)
{
	int i, active_slots;
	u32 mask = 0;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct davinci_audio_dev *dev = rtd->dai->cpu_dai->private_data;
	void __iomem *base = dev->base;

	active_slots = (dev->tdm_slots > 31) ? 32 : dev->tdm_slots;
	for (i = 0; i < active_slots; i++)
		mask |= (1 << i);

	if (dev->async_rxtx_clk)
		mcasp_set_bits(base + DAVINCI_MCASP_ACLKXCTL_REG, TX_ASYNC);
	else
		mcasp_clr_bits(base + DAVINCI_MCASP_ACLKXCTL_REG, TX_ASYNC);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		/* bit stream is MSB first */
		mcasp_set_reg(base + DAVINCI_MCASP_TXTDM_REG, mask);
		mcasp_set_bits(base + DAVINCI_MCASP_TXFMT_REG, TXORD);

		if ((dev->tdm_slots >= 2) || (dev->tdm_slots <= 32))
			mcasp_mod_bits(base + DAVINCI_MCASP_TXFMCTL_REG,
				       FSXMOD(dev->tdm_slots), FSXMOD(0x1FF));
		else
			printk(KERN_ERR "playback tdm slot %d not supported\n",
				dev->tdm_slots);

		/* set pulse width to word if i2s mode */
		if (dev->tdm_slots == 2)
			mcasp_set_bits(base + DAVINCI_MCASP_TXFMCTL_REG,
					FSXDUR);
		else
			mcasp_clr_bits(base + DAVINCI_MCASP_TXFMCTL_REG,
					FSXDUR);

	} else {
		/* bit stream is MSB first with 1 bit delay */
		mcasp_set_bits(base + DAVINCI_MCASP_RXFMT_REG,
				FSRDLY(1) | RXORD);
		mcasp_set_reg(base + DAVINCI_MCASP_RXTDM_REG, mask);

		if ((dev->tdm_slots >= 2) || (dev->tdm_slots <= 32))
			mcasp_mod_bits(base + DAVINCI_MCASP_RXFMCTL_REG,
				       FSRMOD(dev->tdm_slots), FSRMOD(0x1FF));
		else
			printk(KERN_ERR "capture tdm slot %d not supported\n",
				dev->tdm_slots);

		/* set pulse width to word if i2s mode */
		if (dev->tdm_slots == 2)
			mcasp_set_bits(base + DAVINCI_MCASP_RXFMCTL_REG,
					FSRDUR);
		else
			mcasp_clr_bits(base + DAVINCI_MCASP_RXFMCTL_REG,
					FSRDUR);
	}
}

/* S/PDIF */
static void davinci_hw_dit_param(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct davinci_audio_dev *dev = rtd->dai->cpu_dai->private_data;

	/* Set the PDIR for Serialiser as output */
	mcasp_set_bits(dev->base + DAVINCI_MCASP_PDIR_REG, AFSX);

	/* TXMASK for 24 bits */
	mcasp_set_reg(dev->base + DAVINCI_MCASP_TXMASK_REG, 0x00FFFFFF);

	/* Set the TX format : 24 bit right rotation, 32 bit slot, Pad 0
	   and LSB first */
	mcasp_set_bits(dev->base + DAVINCI_MCASP_TXFMT_REG,
		       TXROT(6) | TXSSZ(15));

	/* Set TX frame synch : DIT Mode, 1 bit width, internal, rising edge */
	mcasp_set_reg(dev->base + DAVINCI_MCASP_TXFMCTL_REG,
		      AFSXE | FSXMOD(0x180));

	/* Set the TX tdm : for all the slots */
	mcasp_set_reg(dev->base + DAVINCI_MCASP_TXTDM_REG, 0xFFFFFFFF);

	/* Set the TX clock controls : div = 1 and internal */
	mcasp_set_bits(dev->base + DAVINCI_MCASP_ACLKXCTL_REG,
		       ACLKXE | TX_ASYNC);
	mcasp_mod_bits(dev->base + DAVINCI_MCASP_ACLKXCTL_REG,
		       ACLKXDIV(0), 0x1F);

	mcasp_clr_bits(dev->base + DAVINCI_MCASP_XEVTCTL_REG, TXDATADMADIS);

	/* Only 44100 and 48000 are valid, both have the same setting */
	mcasp_set_bits(dev->base + DAVINCI_MCASP_AHCLKXCTL_REG,
		       AHCLKXDIV(3) | AHCLKXE);

	/* Enable the DIT */
	mcasp_set_bits(dev->base + DAVINCI_MCASP_TXDITCTL_REG, DITEN);
}

static int davinci_i2s_mcasp_hw_params(struct snd_pcm_substream *substream,
					struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct davinci_audio_dev *dev = rtd->dai->cpu_dai->private_data;
	struct davinci_pcm_dma_params *dma_params =
					dev->dma_params[substream->stream];
	int word_length;
	int slot_size;
	int slot_ext = 0;

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S8:
		dma_params->data_type = 1;
		word_length = DAVINCI_AUDIO_WORD_8;
		break;
	case SNDRV_PCM_FORMAT_S16_LE:
		dma_params->data_type = 4;
		word_length = DAVINCI_AUDIO_WORD_16;
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		dma_params->data_type = 4;
		word_length = DAVINCI_AUDIO_WORD_24;
		slot_ext = 1;
		break;
	case SNDRV_PCM_FORMAT_S32_LE:
		dma_params->data_type = 4;
		word_length = DAVINCI_AUDIO_WORD_32;
		break;
	default:
		printk(KERN_WARNING "davinci-i2s: unsupported PCM format");
		return -EINVAL;
	}
	slot_size = davinci_config_channel_size(substream, word_length,
						slot_ext);
	davinci_config_clock(substream, params_rate(params), slot_size);
	davinci_hw_common_param(substream);

	if (dev->op_mode == DAVINCI_MCASP_DIT_MODE)
		davinci_hw_dit_param(substream);
	else
		davinci_hw_iis_param(substream);

	return 0;
}

static int davinci_i2s_mcasp_trigger(struct snd_pcm_substream *substream,
				     int cmd)
{
	int ret = 0;

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		davinci_mcasp_start(substream);
		break;
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		davinci_mcasp_stop(substream);
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

#define MCASP_IIS_FORMATS	(SNDRV_PCM_FMTBIT_S16_LE | \
				 SNDRV_PCM_FMTBIT_S24_LE | \
				 SNDRV_PCM_FMTBIT_S32_LE)

struct snd_soc_cpu_dai davinci_iis_mcasp_dai[] = {
	{
		.name = "davinci-i2s",
		.id = 0,
		.type = SND_SOC_DAI_I2S,
		.probe = davinci_mcasp_i2s_probe,
		.remove = davinci_mcasp_i2s_remove,
		.playback = {
			.channels_min = 1,
			.channels_max = 384,
			.rates = DAVINCI_I2S_RATES,
			.formats = MCASP_IIS_FORMATS,
		},
		.capture = {
			.channels_min = 1,
			.channels_max = 384,
			.rates = DAVINCI_I2S_RATES,
			.formats = MCASP_IIS_FORMATS,
		},
		.ops = {
			.startup = davinci_i2s_startup,
			.trigger = davinci_i2s_mcasp_trigger,
			.hw_params = davinci_i2s_mcasp_hw_params,
		},
		.dai_ops = {
			.set_fmt = davinci_i2s_mcasp_set_dai_fmt,
			.set_sysclk = davinci_i2s_mcasp_set_dai_sysclk,
		},
	},
};
EXPORT_SYMBOL_GPL(davinci_iis_mcasp_dai);

struct snd_soc_cpu_dai davinci_dit_mcasp_dai[] = {
	{
		.name = "davinci-dit",
		.id = 1,
		.type = SND_SOC_DAI_I2S,
		.playback = {
			.channels_min = 1,
			.channels_max = 384,
			.rates = DAVINCI_I2S_RATES,
			.formats = MCASP_IIS_FORMATS,
		},
		.ops = {
			.startup = davinci_i2s_startup,
			.trigger = davinci_i2s_mcasp_trigger,
			.hw_params = davinci_i2s_mcasp_hw_params,
		},
		.dai_ops = {
			.set_fmt = davinci_i2s_mcasp_set_dai_fmt,
		},
	},
};
EXPORT_SYMBOL_GPL(davinci_dit_mcasp_dai);

struct snd_soc_cpu_dai davinci_dir_mcasp_dai[] = {
	{
		.name = "davinci-dir",
		.id = 2,
		.type = SND_SOC_DAI_I2S,
		.capture = {
			.channels_min = 1,
			.channels_max = 384,
			.rates = DAVINCI_I2S_RATES,
			.formats = MCASP_IIS_FORMATS,
		},
		.ops = {
			.startup = davinci_i2s_startup,
			.trigger = davinci_i2s_mcasp_trigger,
			.hw_params = davinci_i2s_mcasp_hw_params,
		},
		.dai_ops = {
			.set_fmt = davinci_i2s_mcasp_set_dai_fmt,
			.set_sysclk = davinci_i2s_mcasp_set_dai_sysclk,
		},
	},
};
EXPORT_SYMBOL_GPL(davinci_dir_mcasp_dai);

/*
 * Stub functions when the DSP is configured to control the McASP
 */
static int davinci_i2s_mcasp_dsp_hw_params(struct snd_pcm_substream *substream,
					   struct snd_pcm_hw_params *params)
{
	return 0;
}
static int davinci_i2s_mcasp_dsp_trigger(struct snd_pcm_substream *substream,
					 int cmd)
{
	return 0;
}
static int davinci_i2s_mcasp_dsp_set_dai_fmt(struct snd_soc_cpu_dai *cpu_dai,
					     unsigned int fmt)
{
	return 0;
}

struct snd_soc_cpu_dai davinci_iis_mcasp_dsp_dai[] = {
	{
		.name = "davinci-i2s-dsp",
		.id = 0,
		.type = SND_SOC_DAI_I2S,
		.playback = {
			.channels_min = 1,
			.channels_max = 2,
			.rates = DAVINCI_I2S_RATES,
			.formats = SNDRV_PCM_FMTBIT_S16_LE,
		},
		.capture = {
			.channels_min = 1,
			.channels_max = 2,
			.rates = DAVINCI_I2S_RATES,
			.formats = SNDRV_PCM_FMTBIT_S16_LE,
		},
		.ops = {
			.startup = davinci_i2s_startup,
			.trigger = davinci_i2s_mcasp_dsp_trigger,
			.hw_params = davinci_i2s_mcasp_dsp_hw_params,
		},
		.dai_ops = {
			.set_fmt = davinci_i2s_mcasp_dsp_set_dai_fmt,
		},
	},
};
EXPORT_SYMBOL_GPL(davinci_iis_mcasp_dsp_dai);

MODULE_AUTHOR("Steve Chen");
MODULE_DESCRIPTION("TI DAVINCI I2S (McASP) SoC Interface");
MODULE_LICENSE("GPL");

