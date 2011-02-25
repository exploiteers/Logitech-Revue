/*
 * TI DA8xx EDMA config file
 *
 * Copyright (C) 2008 MontaVista Software, Inc. <source@mvista.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/init.h>

#include <asm/arch/hardware.h>
#include <asm/arch/irqs.h>
#include <asm/arch/dma.h>
#include <asm/arch/cpu.h>

/*
 * DA8xx specific EDMA3 information
 */

/*
 * Each bit field of the elements below indicate the corresponding DMA channel
 * availability on EDMA_MASTER_SHADOW_REGION side events
 */
static unsigned int da8xx_edma_channels_arm[EDMA_NUM_DMA_CHAN_DWRDS] = {
	DA8XX_EDMA_ARM_OWN, 0
};

/*
 * Each bit field of the elements below indicate the corresponding QDMA channel
 * availability on EDMA_MASTER_SHADOW_REGION side events
 */
static unsigned char da8xx_qdma_channels_arm[EDMA_NUM_QDMA_CHAN_BYTES] = {
	0x000000F0u
};

/*
 * Each bit field of the elements below indicate corresponding PaRAM entry
 * availability on EDMA_MASTER_SHADOW_REGION side events
 */
static unsigned int da8xx_param_entry_arm[] = {
	DA8XX_EDMA_ARM_OWN, 0xFF000000u, 0xFFFFFFFFu, 0xFFFFFFFFu
};

/*
 * Each bit field of the elements below indicate corresponding TCC
 * availability on EDMA_MASTER_SHADOW_REGION side events
 */
static unsigned int da8xx_tcc_arm[EDMA_NUM_DMA_CHAN_DWRDS] = {
	DA8XX_EDMA_ARM_OWN, 0
};

/*
 * Each bit field of the elements below indicate whether the corresponding
 * PaRAM entry is available for ANY DMA channel or not: 1 - reserved, 0 - not.
 * (First 32 PaRAM sets are reserved for 32 DMA channels.)
 */
static unsigned int da8xx_param_entry_reserved[] = {
	0xFFFFFFFFu, 0, 0, 0
};

static struct edma_map da8xx_queue_priority_mapping[EDMA_DA8XX_NUM_EVQUE] = {
	/* {Event Queue No, Priority} */
	{0, 0},
	{1, 7}
};

static struct edma_map da8xx_queue_watermark_level[EDMA_DA8XX_NUM_EVQUE] = {
	/* {Event Queue No, Watermark Level} */
	{0, 16},
	{1, 16}
};

static struct edma_map da8xx_queue_tc_mapping[EDMA_DA8XX_NUM_EVQUE] = {
	/* {Event Queue No, TC no} */
	{0, 0},
	{1, 1}
};

/*
 * Mapping of DMA channels to Hardware Events from
 * various peripherals, which use EDMA for data transfer.
 * All channels need not be mapped, some can be free also.
 */
static unsigned int da8xx_dma_ch_hw_event_map[EDMA_NUM_DMA_CHAN_DWRDS] = {
	DA8XX_DMACH2EVENT_MAP0,
	DA8XX_DMACH2EVENT_MAP1
};

/* Array containing physical addresses of all the TCs present */
static u32 da8xx_edmatc_base_addrs[EDMA_MAX_TC] = {
	DA8XX_TPTC0_BASE,
	DA8XX_TPTC1_BASE
};

/*
 * Variable which will be used internally for referring transfer controllers'
 * error interrupts.
 */
static unsigned int da8xx_tc_error_int[EDMA_MAX_TC] = {
	IRQ_DA8XX_TCERRINT0, IRQ_DA8XX_TCERRINT1
};

static struct dma_init_info dma_info __initdata = {
	.edma_num_dmach 	= EDMA_DA8XX_NUM_DMACH,
	.edma_num_tcc		= EDMA_DA8XX_NUM_TCC,
	.edma_num_evtq		= EDMA_DA8XX_NUM_EVQUE,
	.edma_num_tc		= EDMA_DA8XX_NUM_TC,
	.edma_num_param 	= EDMA_DA8XX_NUM_PARAMENTRY,

	.edma_chmap_exist	= EDMA_DA8XX_CHMAP_EXIST,

	.edmatc_base_addrs	= da8xx_edmatc_base_addrs,

	.edma2event_map		= da8xx_dma_ch_hw_event_map,

	.edma_channels_arm	= da8xx_edma_channels_arm,
	.qdma_channels_arm	= da8xx_qdma_channels_arm,
	.param_entry_arm	= da8xx_param_entry_arm,
	.tcc_arm		= da8xx_tcc_arm,
	.param_entry_reserved	= da8xx_param_entry_reserved,

	.q_pri			= da8xx_queue_priority_mapping,
	.q_tc			= da8xx_queue_tc_mapping,
	.q_wm			= da8xx_queue_watermark_level,

	.cc_reg0_int		= IRQ_DA8XX_CCINT0,
	.cc_error_int		= IRQ_DA8XX_CCERRINT,
	.tc_error_int		= da8xx_tc_error_int,
};

static int __init dma_init(void)
{
	return davinci_dma_init(&dma_info);
}
arch_initcall(dma_init);
