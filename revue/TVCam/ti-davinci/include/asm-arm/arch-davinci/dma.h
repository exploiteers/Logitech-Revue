/*
 * DaVinci DMA definitions
 *
 * Author: Kevin Hilman, MontaVista Software, Inc. <source@mvista.com>
 *
 * 2007 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */
#ifndef __ASM_ARCH_DMA_H
#define __ASM_ARCH_DMA_H

#define MAX_DMA_ADDRESS			0xffffffff

#include "edma.h"

struct edma_map {
	int param1;
	int param2;
};

struct dma_init_info {
	unsigned int	edma_num_dmach;
	unsigned int	edma_num_tcc;
	unsigned int	edma_num_evtq;
	unsigned int	edma_num_tc;
	unsigned int	edma_num_param;

	unsigned int	edma_chmap_exist;

	unsigned int	*edmatc_base_addrs;

	unsigned int	*edma2event_map;

	unsigned int	*edma_channels_arm;
	unsigned char	*qdma_channels_arm;
	unsigned int	*param_entry_arm;
	unsigned int	*tcc_arm;
	unsigned int	*param_entry_reserved;

	struct edma_map	*q_pri;
	struct edma_map	*q_tc;
	struct edma_map	*q_wm;

	unsigned int	cc_reg0_int;
	unsigned int	cc_error_int;
	unsigned int	*tc_error_int;
};

extern int davinci_dma_init(struct dma_init_info *diip);

#endif /* __ASM_ARCH_DMA_H */
