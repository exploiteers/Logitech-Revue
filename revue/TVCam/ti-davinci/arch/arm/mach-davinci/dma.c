/*
 * Copyright (C) 2006 Texas Instruments.
 * Copyright (c) 2007-2008, MontaVista Software, Inc. <source@mvista.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>

#include <linux/io.h>

#include <asm/arch/hardware.h>
#include <asm/arch/memory.h>
#include <asm/arch/irqs.h>
#include <asm/arch/dma.h>
#include <asm/arch/cpu.h>

/* SoC specific EDMA3 hardware information, should be provided for a new SoC */

/* DaVinci DM644x specific EDMA3 information */

/*
 * Each bit field of the elements below indicate the corresponding DMA channel
 * availability on EDMA_MASTER_SHADOW_REGION side events
 */
static unsigned int dm644x_edma_channels_arm[EDMA_NUM_DMA_CHAN_DWRDS] = {
	0xFFFFFFFF, 0xFFFFFFFF
};

/*
 * Each bit field of the elements below indicate the corresponding QDMA channel
 * availability on EDMA_MASTER_SHADOW_REGION side events
 */
static unsigned char dm644x_qdma_channels_arm[EDMA_NUM_QDMA_CHAN_BYTES] = {
	0x10
};

/*
 *  Each bit field of the elements below indicate corresponding PaRAM entry
 *  availability on EDMA_MASTER_SHADOW_REGION side events
 */
static unsigned int dm644x_param_entry_arm[] = {
	0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF
};

/*
 *  Each bit field of the elements below indicate corresponding TCC
 *  availability on EDMA_MASTER_SHADOW_REGION side events
 */
static unsigned int dm644x_tcc_arm[EDMA_NUM_DMA_CHAN_DWRDS] = {
	0xFFFFFFFF, 0xFFFFFFFF
};

/*
 *  Each bit field of the elements below indicate whether the corresponding
 *  PaRAM entry is available for ANY DMA channel or not.
 *   1- reserved, 0 - not
 *   (First 64 PaRAM Sets are reserved for 64 DMA Channels)
 */
static unsigned int dm644x_param_entry_reserved[] = {
	0xFFFFFFFF, 0xFFFFFFFF, 0, 0
};

static struct edma_map dm644x_queue_priority_mapping[EDMA_DM644X_NUM_EVQUE] = {
	/* {Event Queue No, Priority} */
	{0, 0},
	{1, 1}
};

static struct edma_map dm644x_queue_watermark_level[EDMA_DM644X_NUM_EVQUE] = {
	/* {Event Queue No, Watermark Level} */
	{0, 16},
	{1, 16}
};

static struct edma_map dm644x_queue_tc_mapping[EDMA_DM644X_NUM_EVQUE] = {
	/* {Event Queue No, TC no} */
	{0, 0},
	{1, 1}
};

/* DaVinci DM646x specific EDMA3 information */

/*
 * Each bit field of the elements below indicate the corresponding DMA channel
 * availability on EDMA_MASTER_SHADOW_REGION side events
 */
static unsigned int dm646x_edma_channels_arm[EDMA_NUM_DMA_CHAN_DWRDS] = {
	0x30FF1FF0, 0x00C007FF
};

/*
 * Each bit field of the elements below indicate the corresponding QDMA channel
 * availability on EDMA_MASTER_SHADOW_REGION side events
 */
static unsigned char dm646x_qdma_channels_arm[EDMA_NUM_QDMA_CHAN_BYTES] = {
	0x80
};

/*
 *  Each bit field of the elements below indicate corresponding PaRAM entry
 *  availability on EDMA_MASTER_SHADOW_REGION side events
 */
static unsigned int dm646x_param_entry_arm[] = {
	0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/*
 *  Each bit field of the elements below indicate corresponding TCC
 *  availability on EDMA_MASTER_SHADOW_REGION side events
 */
static unsigned int dm646x_tcc_arm[EDMA_NUM_DMA_CHAN_DWRDS] = {
	0x30FF1FF0, 0x00C007FF
};

/*
 *  Each bit field of the elements below indicate whether the corresponding
 *  PaRAM entry is available for ANY DMA channel or not.
 *   1- reserved, 0 - not
 *   (First 64 PaRAM Sets are reserved for 64 DMA Channels)
 */
static unsigned int dm646x_param_entry_reserved[] = {
	0xFFFFFFFF, 0xFFFFFFFF, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static struct edma_map dm646x_queue_priority_mapping[EDMA_DM646X_NUM_EVQUE] = {
	/* {Event Queue No, Priority} */
	{0, 0},
	{1, 1},
	{2, 2},
	{3, 3}
};

static struct edma_map dm646x_queue_watermark_level[EDMA_DM646X_NUM_EVQUE] = {
	/* {Event Queue No, Watermark Level} */
	{0, 16},
	{1, 16},
	{2, 16},
	{3, 16}
};

static struct edma_map dm646x_queue_tc_mapping[EDMA_DM646X_NUM_EVQUE] = {
	/* {Event Queue No, TC no} */
	{0, 0},
	{1, 1},
	{2, 2},
	{3, 3},
};

/* DaVinci DM355 specific EDMA3 information */

/*
 * Each bit field of the elements below indicate the corresponding DMA channel
 * availability on EDMA_MASTER_SHADOW_REGION side events
 */
static unsigned int dm355_edma_channels_arm[] = {
	0xFFFFFFFF, 0xFFFFFFFF
};

/*
 * Each bit field of the elements below indicate the corresponding QDMA channel
 * availability on EDMA_MASTER_SHADOW_REGION side events
 */
static unsigned char dm355_qdma_channels_arm[EDMA_NUM_QDMA_CHAN_BYTES] = {
	0xFF
};

/*
 *  Each bit field of the elements below indicate corresponding PaRAM entry
 *  availability on EDMA_MASTER_SHADOW_REGION side events
 */
static unsigned int dm355_param_entry_arm[] = {
	0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF
};

/*
 *  Each bit field of the elements below indicate corresponding TCC
 *  availability on EDMA_MASTER_SHADOW_REGION side events
 */
static unsigned int dm355_tcc_arm[EDMA_NUM_DMA_CHAN_DWRDS] = {
	0xFFFFFFFF, 0xFFFFFFFF
};

/*
 *  Each bit field of the elements below indicate whether the corresponding
 *  PaRAM entry is available for ANY DMA channel or not.
 *   1- reserved, 0 - not
 *   (First 64 PaRAM Sets are reserved for 64 DMA Channels)
 */
static unsigned int dm355_param_entry_reserved[] = {
	0xFFFFFFFF, 0xFFFFFFFF, 0, 0
};

static struct edma_map dm355_queue_priority_mapping[] = {
	/* {Event Queue No, Priority} */
	{0, 0},
	{1, 7}
};

static struct edma_map dm355_queue_watermark_level[] = {
	/* {Event Queue No, Watermark Level} */
	{0, 16},
	{1, 16}
};

static struct edma_map dm355_queue_tc_mapping[] = {
	/* {Event Queue No, TC no} */
	{0, 0},
	{1, 1}
};

/* DaVinci DM365 specific EDMA3 information */

/*
 * Each bit field of the elements below indicate the corresponding DMA channel
 * availability on EDMA_MASTER_SHADOW_REGION side events
 */
static unsigned int dm365_edma_channels_arm[EDMA_NUM_DMA_CHAN_DWRDS] = {
	0xFFFFFFFFu, 0xFFFFFFFFu
};

/*
 * Each bit field of the elements below indicate the corresponding QDMA channel
 * availability on EDMA_MASTER_SHADOW_REGION side events
 */
static unsigned char dm365_qdma_channels_arm[EDMA_NUM_QDMA_CHAN_BYTES] = {
	0x000000FFu
};

/*
 *  Each bit field of the elements below indicate corresponding PARAM entry
 *  availability on EDMA_MASTER_SHADOW_REGION side events
 */
static unsigned int dm365_param_entry_arm[] = {
	0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu,
	0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu
};

/*
 *  Each bit field of the elements below indicate corresponding TCC
 *  availability on EDMA_MASTER_SHADOW_REGION side events
 */
static unsigned int dm365_tcc_arm[EDMA_NUM_DMA_CHAN_DWRDS] = {
	0xFFFFFFFFu, 0xFFFFFFFFu
};

/*
 *  Each bit field of the elements below indicate whether the corresponding
 *   PARAM entry is available for ANY DMA channel or not.
 *   1- reserved, 0 - not
 *   (First 64 PaRAM Sets are reserved for 64 DMA Channels)
 */
static unsigned int dm365_param_entry_reserved[] = {
	0xFFFFFFFFu, 0xFFFFFFFFu, 0x0u, 0x0u,
	0x0u, 0x0u, 0x0u, 0x0u
};

static struct edma_map dm365_queue_priority_mapping[EDMA_DM365_NUM_EVQUE] = {
	/* {Event Queue No, Priority} */
	{0, 7},
	{1, 7},
	{2, 7},
	{3, 0}
};

static struct edma_map dm365_queue_watermark_level[EDMA_DM365_NUM_EVQUE] = {
	/* {Event Queue No, Watermark Level} */
	{0, 16},
	{1, 16},
	{2, 16},
	{3, 16}
};

static struct edma_map dm365_queue_tc_mapping[EDMA_DM365_NUM_EVQUE] = {
	/* {Event Queue No, TC no} */
	{0, 0},
	{1, 1},
	{2, 2},
	{3, 3},
};

/*
 * Mapping of DMA channels to Hardware Events from
 * various peripherals, which use EDMA for data transfer.
 * All channels need not be mapped, some can be free also.
 */
static unsigned int dm644x_dma_ch_hw_event_map[EDMA_NUM_DMA_CHAN_DWRDS] = {
	DM644X_DMACH2EVENT_MAP0,
	DM644X_DMACH2EVENT_MAP1
};

static unsigned int dm355_dma_ch_hw_event_map[EDMA_NUM_DMA_CHAN_DWRDS] = {
	DM355_DMACH2EVENT_MAP0,
	DM355_DMACH2EVENT_MAP1
};

static unsigned int dm646x_dma_ch_hw_event_map[EDMA_NUM_DMA_CHAN_DWRDS] = {
	DM646X_DMACH2EVENT_MAP0,
	DM646X_DMACH2EVENT_MAP1
};

static unsigned int dm365_dma_ch_hw_event_map[EDMA_NUM_DMA_CHAN_DWRDS] = {
	DM365_DMACH2EVENT_MAP0,
	DM365_DMACH2EVENT_MAP1
};

/* Array containing physical addresses of all the TCs present */
u32 dm644x_edmatc_base_addrs[EDMA_MAX_TC] = {
	(u32)DAVINCI_DMA_3PTC0_BASE,
	(u32)DAVINCI_DMA_3PTC1_BASE,
};
u32 dm646x_edmatc_base_addrs[EDMA_MAX_TC] = {
	(u32)DAVINCI_DMA_3PTC0_BASE,
	(u32)DAVINCI_DMA_3PTC1_BASE,
	(u32)DAVINCI_DM646X_DMA_3PTC2_BASE,
	(u32)DAVINCI_DM646X_DMA_3PTC3_BASE,
};
u32 dm355_edmatc_base_addrs[EDMA_MAX_TC] = {
	(u32)DAVINCI_DMA_3PTC0_BASE,
	(u32)DAVINCI_DMA_3PTC1_BASE,
};
u32 dm365_edmatc_base_addrs[EDMA_MAX_TC] = {
	(u32) DAVINCI_DMA_3PTC0_BASE,
	(u32) DAVINCI_DMA_3PTC1_BASE,
	(u32) DM365_DMA_3PTC2_BASE,
	(u32) DM365_DMA_3PTC3_BASE
};

/*
 * Variable which will be used internally for referring transfer controllers'
 * error interrupts.
 */
unsigned int dm644x_tc_error_int[EDMA_MAX_TC] = {
	IRQ_TCERRINT0, IRQ_TCERRINT,
	0, 0, 0, 0, 0, 0,
};
unsigned int dm646x_tc_error_int[EDMA_MAX_TC] = {
	IRQ_TCERRINT0, IRQ_TCERRINT,
	IRQ_DM646X_TCERRINT2, IRQ_DM646X_TCERRINT3,
	0, 0, 0, 0,
};
unsigned int dm355_tc_error_int[EDMA_MAX_TC] = {
	IRQ_TCERRINT0, IRQ_TCERRINT,
	0, 0, 0, 0, 0, 0,
};
unsigned int dm365_tc_error_int[EDMA_MAX_TC] = {
	IRQ_TCERRINT0, IRQ_TCERRINT,
	IRQ_DM365_TCERRINT2, IRQ_DM365_TCERRINT3
};

int __init dma_init(void)
{
	struct dma_init_info info;

	info.edma_num_dmach = EDMA_DAVINCI_NUM_DMACH;
	info.edma_num_tcc = EDMA_DAVINCI_NUM_TCC;

	info.cc_reg0_int  = IRQ_CCINT0;
	info.cc_error_int = IRQ_CCERRINT;

	if (cpu_is_davinci_dm6467()) {
		info.edma_num_evtq = EDMA_DM646X_NUM_EVQUE;
		info.edma_num_tc = EDMA_DM646X_NUM_TC;
		info.edma_num_param = EDMA_DM646X_NUM_PARAMENTRY;

		info.edma_chmap_exist = EDMA_DM646X_CHMAP_EXIST;

		info.edmatc_base_addrs = dm646x_edmatc_base_addrs;

		info.edma2event_map = dm646x_dma_ch_hw_event_map;

		info.edma_channels_arm = dm646x_edma_channels_arm;
		info.qdma_channels_arm = dm646x_qdma_channels_arm;
		info.param_entry_arm = dm646x_param_entry_arm;
		info.tcc_arm = dm646x_tcc_arm;
		info.param_entry_reserved = dm646x_param_entry_reserved;

		info.q_pri = dm646x_queue_priority_mapping;
		info.q_tc  = dm646x_queue_tc_mapping;
		info.q_wm  = dm646x_queue_watermark_level;

		info.tc_error_int = dm646x_tc_error_int;
	} else if (cpu_is_davinci_dm355()) {
		info.edma_num_evtq = EDMA_DM355_NUM_EVQUE;
		info.edma_num_tc = EDMA_DM355_NUM_TC;
		info.edma_num_param = EDMA_DM355_NUM_PARAMENTRY;

		info.edma_chmap_exist = EDMA_DM355_CHMAP_EXIST;

		info.edmatc_base_addrs = dm355_edmatc_base_addrs;

		info.edma2event_map = dm355_dma_ch_hw_event_map;

		info.edma_channels_arm = dm355_edma_channels_arm;
		info.qdma_channels_arm = dm355_qdma_channels_arm;
		info.param_entry_arm = dm355_param_entry_arm;
		info.tcc_arm = dm355_tcc_arm;
		info.param_entry_reserved = dm355_param_entry_reserved;

		info.q_pri = dm355_queue_priority_mapping;
		info.q_tc  = dm355_queue_tc_mapping;
		info.q_wm  = dm355_queue_watermark_level;

		info.tc_error_int = dm355_tc_error_int;
	} else if (cpu_is_davinci_dm365()) {
		info.edma_num_evtq = EDMA_DM365_NUM_EVQUE;
		info.edma_num_tc = EDMA_DM365_NUM_TC;
		info.edma_num_param = EDMA_DM365_NUM_PARAMENTRY;

		info.edma_chmap_exist = EDMA_DM365_CHMAP_EXIST;

		info.edmatc_base_addrs = dm365_edmatc_base_addrs;

		info.edma2event_map = dm365_dma_ch_hw_event_map;

		info.edma_channels_arm = dm365_edma_channels_arm;
		info.qdma_channels_arm = dm365_qdma_channels_arm;
		info.param_entry_arm = dm365_param_entry_arm;
		info.tcc_arm = dm365_tcc_arm;
		info.param_entry_reserved = dm365_param_entry_reserved;

		info.q_pri = dm365_queue_priority_mapping;
		info.q_tc  = dm365_queue_tc_mapping;
		info.q_wm  = dm365_queue_watermark_level;

		info.tc_error_int = dm365_tc_error_int;
	} else { /* Must be dm6446 or dm357 */
		if (!cpu_is_davinci_dm644x() && !cpu_is_davinci_dm357())
			BUG();

		info.edma_num_evtq = EDMA_DM644X_NUM_EVQUE;
		info.edma_num_tc = EDMA_DM644X_NUM_TC;
		info.edma_num_param = EDMA_DM644X_NUM_PARAMENTRY;

		info.edma_chmap_exist = EDMA_DM644X_CHMAP_EXIST;

		info.edmatc_base_addrs = dm644x_edmatc_base_addrs;

		info.edma2event_map = dm644x_dma_ch_hw_event_map;

		info.edma_channels_arm = dm644x_edma_channels_arm;
		info.qdma_channels_arm = dm644x_qdma_channels_arm;
		info.param_entry_arm = dm644x_param_entry_arm;
		info.tcc_arm = dm644x_tcc_arm;
		info.param_entry_reserved = dm644x_param_entry_reserved;

		info.q_pri = dm644x_queue_priority_mapping;
		info.q_tc  = dm644x_queue_tc_mapping;
		info.q_wm  = dm644x_queue_watermark_level;

		info.tc_error_int = dm644x_tc_error_int;
	}

	return davinci_dma_init(&info);
}
arch_initcall(dma_init);
