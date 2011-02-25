/*
 * ALSA PCM interface for the TI DAVINCI processor
 *
 * Author:      Vladimir Barinov, <vbarinov@ru.mvista.com>
 * Copyright:   (C) 2007 MontaVista Software, Inc., <source@mvista.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _DAVINCI_PCM_H
#define _DAVINCI_PCM_H

#include <asm/arch/edma.h>

struct davinci_pcm_dma_params {
	char *name;			/* stream identifier */
	int channel;			/* sync dma channel ID */
	dma_addr_t dma_addr;		/* device physical address for DMA */
	unsigned int data_type;		/* xfer data type */
	unsigned int num_data_stream;	/* number of serializers */
	enum dma_event_q eventq_no;	/* event queue number */
};

struct evm_snd_platform_data {
	char *clk_name;
	int tx_dma_ch;
	int rx_dma_ch;
	u32 tx_dma_offset;
	u32 rx_dma_offset;
	enum dma_event_q eventq_no;	/* event queue number */
	unsigned int codec_fmt;

	/* McASP specific fields */
	int tdm_slots;
	u8 op_mode;
	u8 num_serializer;
	u8 word_shift;
	u8 fixed_slot;
	u8 async_rxtx_clk;
	u8 *serial_dir;
	int sysclk_dir;
};

extern struct snd_soc_platform davinci_soc_platform;

#endif
