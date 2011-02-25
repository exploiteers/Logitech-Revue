/*
 * TI DaVinci EDMA Support
 *
 * Copyright (C) 2006 Texas Instruments.
 * Copyright (c) 2007-2008, MontaVista Software, Inc. <source@mvista.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/io.h>

#include <asm/arch/hardware.h>
#include <asm/arch/irqs.h>
#include <asm/arch/dma.h>

static unsigned int *edma_channels_arm;
static unsigned char *qdma_channels_arm;
static unsigned int *param_entry_arm;
static unsigned int *tcc_arm;
static unsigned int *param_entry_reserved;
static unsigned int davinci_qdma_ch_map;

static unsigned int cc_reg0_int;
static unsigned int cc_error_int;
static unsigned int *tc_error_int;

static spinlock_t dma_chan_lock;

/*
 * EDMA Driver Internal Data Structures
 */

/*
 * Array to maintain the Callback details registered against a particular TCC.
 * Used to call the callback functions linked to the particular channel.
 */
static struct davinci_dma_lch_intr {
	void (*callback)(int lch, u16 ch_status, void *data);
	void *data;
} intr_data[EDMA_MAX_TCC];

#define dma_handle_cb(lch, status) do { \
	if (intr_data[lch].callback) \
		intr_data[lch].callback(lch, status, intr_data[lch].data); \
} while (0)

/*
 * Resources bound to a Logical Channel (DMA/QDMA/LINK)
 *
 * When a request for a channel is made, the resources PaRAM Set and TCC
 * get bound to that channel. This information is needed internally by the
 * driver when a request is made to free the channel (Since it is the
 * responsibility of the driver to free up the channel-associated resources
 * from the Resource Manager layer).
 */
struct edma3_ch_bound_res {
	/* PaRAM Set number associated with the particular channel */
	unsigned int param_id;
	/* TCC associated with the particular channel */
	unsigned int tcc;
};

static struct edma3_ch_bound_res *dma_ch_bound_res;
static int edma_max_logical_ch;
static unsigned int davinci_edma_num_dmach;
static unsigned int davinci_edma_num_tcc;
static unsigned int davinci_edma_num_evtq;
static unsigned int davinci_edma_num_tc;
static unsigned int davinci_edma_num_param;
static unsigned int davinci_edma_chmap_exist;
static unsigned int *davinci_edmatc_base_addrs;
static unsigned int *edma2event_map;

/*
 * Each bit field of the elements below indicates whether a DMA Channel
 * is free or in use: 1 - free, 0 - in use.
 */
static unsigned int dma_ch_use_status[EDMA_NUM_DMA_CHAN_DWRDS] = {
	0xFFFFFFFFu,
	0xFFFFFFFFu
};

/*
 * Each bit field of the elements below indicates whether a interrupt
 * is free or in use: 1 - free, 0 - in use.
 */
static unsigned char qdma_ch_use_status[EDMA_NUM_QDMA_CHAN_BYTES] = {
	0xFFu
};

/*
 * Each bit field of the elements below indicates whether a PaRAM entry
 * is free or in use: 1 - free, 0 - in use.
 */
static unsigned int param_entry_use_status[EDMA_MAX_PARAM_SET / 32] = {
	0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu,
	0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu,
	0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu,
	0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu
};

/*
 * Each bit field of the elements below indicates whether a intrerrupt
 * is free or in use: 1 - free, 0 - in use.
 */
static unsigned long tcc_use_status[EDMA_NUM_DMA_CHAN_DWRDS] = {
	0xFFFFFFFFu,
	0xFFFFFFFFu
};

/*
 * Global Array to store the mapping between DMA channels and Interrupt
 * channels i.e. TCCs.
 * DMA channel X can use any TCC Y. Transfer completion interrupt will occur
 * on the TCC Y (IPR/IPRH Register, bit Y), but error interrupt will occur on
 * DMA channel X (EMR/EMRH register, bit X).
 * In that scenario, this DMA channel <-> TCC mapping will be used to point to
 * the correct callback function.
 */
static unsigned int edma_dma_ch_tcc_mapping[EDMA_MAX_DMACH];

/*
 * Global Array to store the mapping between QDMA channels and Interrupt
 * channels i.e. TCCs.
 * QDMA channel X can use any TCC Y. Transfer completion interrupt will occur
 * on the TCC Y (IPR/IPRH Register, bit Y), but error interrupt will occur on
 * QDMA channel X (QEMR register, bit X).
 * In that scenario, this QDMA channel <-> TCC mapping will be used to point to
 * the correct callback function.
 */
static unsigned int edma_qdma_ch_tcc_mapping[EDMA_MAX_QDMACH];

/*
 * The list of Interrupt Channels which get allocated while requesting the TCC.
 * It will be used while checking the IPR/IPRH bits in the RM ISR.
 */
static unsigned int allocated_tccs[2];

static char tc_error_int_name[EDMA_MAX_TC][20];

/*
 * EDMA Driver Internal Functions
 */

/* EDMA3 TC0 Error Interrupt Handler ISR Routine */

static irqreturn_t dma_tc0_err_handler(int irq, void *dev_id,
				       struct pt_regs *regs);
/* EDMA3 TC1 Error Interrupt Handler ISR Routine */
static irqreturn_t dma_tc1_err_handler(int irq, void *dev_id,
				       struct pt_regs *regs);
/* EDMA3 TC2 Error Interrupt Handler ISR Routine */
static irqreturn_t dma_tc2_err_handler(int irq, void *dev_id,
				       struct pt_regs *regs);
/* EDMA3 TC3 Error Interrupt Handler ISR Routine */
static irqreturn_t dma_tc3_err_handler(int irq, void *dev_id,
				       struct pt_regs *regs);
/* EDMA3 TC4 Error Interrupt Handler ISR Routine */
static irqreturn_t dma_tc4_err_handler(int irq, void *dev_id,
				       struct pt_regs *regs);
/* EDMA3 TC5 Error Interrupt Handler ISR Routine */
static irqreturn_t dma_tc5_err_handler(int irq, void *dev_id,
				       struct pt_regs *regs);
/* EDMA3 TC6 Error Interrupt Handler ISR Routine */
static irqreturn_t dma_tc6_err_handler(int irq, void *dev_id,
				       struct pt_regs *regs);
/*  EDMA3 TC7 Error Interrupt Handler ISR Routine */
static irqreturn_t dma_tc7_err_handler(int irq, void *dev_id,
				       struct pt_regs *regs);

/*
 * EDMA3 TC ISRs which need to be registered with the underlying OS by the user
 * (Not all TC error ISRs need to be registered -- only for the available
 * Transfer Controllers).
 */
irqreturn_t (*ptr_edmatc_isrs[EDMA_MAX_TC])(int irq, void *dev_id,
					    struct pt_regs *regs) = {
	&dma_tc0_err_handler,
	&dma_tc1_err_handler,
	&dma_tc2_err_handler,
	&dma_tc3_err_handler,
	&dma_tc4_err_handler,
	&dma_tc5_err_handler,
	&dma_tc6_err_handler,
	&dma_tc7_err_handler,
};

static void map_dma_ch_evt_queue(unsigned int dma_ch, unsigned int evt_queue)
{
	CLEAR_REG_VAL(DMAQNUM_CLR_MASK(dma_ch), EDMA_DMAQNUM(dma_ch >> 3));
	SET_REG_VAL(DMAQNUM_SET_MASK(dma_ch, evt_queue),
		    EDMA_DMAQNUM(dma_ch >> 3));
}

static void map_qdma_ch_evt_queue(unsigned int qdma_ch, unsigned int evt_queue)
{
	/* Map QDMA channel to event queue */
	CLEAR_REG_VAL(QDMAQNUM_CLR_MASK(qdma_ch), EDMA_QDMAQNUM);
	SET_REG_VAL(QDMAQNUM_SET_MASK(qdma_ch, evt_queue), EDMA_QDMAQNUM);
}

static void map_dma_ch_param_set(unsigned int lch, unsigned int param_set)
{
	if (davinci_edma_chmap_exist == 1) {
		/* Map PaRAM set number for specified lch */
		CLEAR_REG_VAL(DMACH_PARAM_CLR_MASK, EDMA_DCHMAP(lch));
		SET_REG_VAL(DMACH_PARAM_SET_MASK(param_set), EDMA_DCHMAP(lch));
	}
}

static void map_qdma_ch_param_set(unsigned int qdma_ch, unsigned int param_set)
{
	/* Map PaRAM Set Number for specified qdma_ch */
	CLEAR_REG_VAL(QDMACH_PARAM_CLR_MASK, EDMA_QCHMAP(qdma_ch));
	SET_REG_VAL(QDMACH_PARAM_SET_MASK(param_set), EDMA_QCHMAP(qdma_ch));

	/* Set CCNT as default Trigger Word */
	CLEAR_REG_VAL(QDMACH_TRWORD_CLR_MASK, EDMA_QCHMAP(qdma_ch));
	SET_REG_VAL(QDMACH_TRWORD_SET_MASK(param_set), EDMA_QCHMAP(qdma_ch));
}

static void register_callback(unsigned int tcc,
			      void (*callback)(int lch,
					       unsigned short ch_status,
					       void *data),
			      void *data)
{
	/* If callback function is not NULL */
	if (callback == NULL)
		return;

	if (tcc >= davinci_edma_num_tcc) {
		printk(KERN_WARNING "WARNING: DMA register callback failed - "
		       "invalid TCC %d\n", tcc);
		return;
	} else if (tcc < 32) {
		SET_REG_VAL(1 << tcc, EDMA_SH_IESR(EDMA_MASTER_SHADOW_REGION));

		pr_debug("IER = %x\n", EDMA_SH_IER(EDMA_MASTER_SHADOW_REGION));
	} else {
		SET_REG_VAL(1 << (tcc - 32),
			    EDMA_SH_IESRH(EDMA_MASTER_SHADOW_REGION));

		pr_debug("IERH = %x\n",
			 EDMA_SH_IERH(EDMA_MASTER_SHADOW_REGION));
	}

	/* Save the callback function also */
	intr_data[tcc].callback = callback;
	intr_data[tcc].data = data;
}

static void unregister_callback(unsigned int lch, enum resource_type ch_type)
{
	unsigned int tcc;

	pr_debug("[%s]: start, lch = %d\n", __func__, lch);

	switch (ch_type) {
	case RES_DMA_CHANNEL:
		tcc = edma_dma_ch_tcc_mapping[lch];
		pr_debug("Mapped TCC %d for DMA channel\n", tcc);
		/* Reset */
		edma_dma_ch_tcc_mapping[lch] = EDMA_MAX_TCC;
		break;

	case RES_QDMA_CHANNEL:
		tcc = edma_qdma_ch_tcc_mapping[lch - EDMA_QDMA_CHANNEL_0];
		pr_debug("Mapped TCC %d for QDMA channel\n", tcc);
		/* Reset */
		edma_qdma_ch_tcc_mapping[lch - EDMA_QDMA_CHANNEL_0] =
			EDMA_MAX_TCC;
		break;

	default:
		return;
	}

	/* Remove the callback function and disable the interrupts */
	if (tcc >= davinci_edma_num_tcc) {
		printk(KERN_WARNING "WARNING: DMA unregister callback failed - "
		       "invalid tcc %d on lch %d\n", tcc, lch);
		return;
	} else if (tcc < 32)
		SET_REG_VAL(1 << tcc, EDMA_SH_IECR(EDMA_MASTER_SHADOW_REGION));
	else
		SET_REG_VAL(1 << (tcc - 32),
			    EDMA_SH_IECRH(EDMA_MASTER_SHADOW_REGION));

	intr_data[tcc].callback = 0;
	intr_data[tcc].data = 0;

	pr_debug("[%s]: end\n", __func__);
}

static int reserve_one_edma_channel(unsigned int res_id,
				    unsigned int res_id_set)
{
	int result = -1;
	u32 reg, idx = res_id / 32;

	spin_lock(&dma_chan_lock);
	if ((edma_channels_arm[idx] & res_id_set) != 0 &&
	    (dma_ch_use_status[idx] & res_id_set) != 0) {
		/* Mark it as non-available now */
		dma_ch_use_status[idx] &= ~res_id_set;
		if (res_id < 32) {
			/* Enable the DMA channel in the DRAE register */
			reg = EDMA_DRAE(EDMA_MASTER_SHADOW_REGION);
			SET_REG_VAL(res_id_set, reg);
			pr_debug("DRAE = %x\n", dma_read(reg));
			reg = EDMA_SH_EECR(EDMA_MASTER_SHADOW_REGION);
			SET_REG_VAL(res_id_set, reg);
		} else {
			reg = EDMA_DRAEH(EDMA_MASTER_SHADOW_REGION);
			SET_REG_VAL(res_id_set, reg);
			pr_debug("DRAEH = %x\n", dma_read(reg));
			reg = EDMA_SH_EECRH(EDMA_MASTER_SHADOW_REGION);
			SET_REG_VAL(res_id_set, reg);
		}
		result = res_id;
	}
	spin_unlock(&dma_chan_lock);
	return result;
}

static int reserve_any_edma_channel(void)
{
	int avl_id, result = -1;
	u32 idx, mask;

	for (avl_id = 0; avl_id < davinci_edma_num_dmach; ++avl_id) {
		idx = avl_id / 32;
		mask = 1 << (avl_id % 32);
		if ((~edma2event_map[idx] & mask) != 0) {
			result = reserve_one_edma_channel(avl_id, mask);
			if (result != -1)
				break;
		}
	}
	return result;
}

static int reserve_one_qdma_channel(unsigned int res_id,
				     unsigned int res_id_mask)
{
	int result = -1, idx = res_id / 32;
	u32 reg;

	if (res_id >= EDMA_NUM_QDMACH)
		return result;

	spin_lock(&dma_chan_lock);
	if ((qdma_channels_arm[idx]  & res_id_mask) != 0 &&
	    (qdma_ch_use_status[idx] & res_id_mask) != 0) {
		/* QDMA Channel Available, mark it as unavailable */
		qdma_ch_use_status[idx] &= ~res_id_mask;

		/* Enable the QDMA channel in the QRAE regs */
		reg = EDMA_QRAE(EDMA_MASTER_SHADOW_REGION);
		SET_REG_VAL(res_id_mask, reg);
		pr_debug("QDMA channel %u, QRAE = %x\n", res_id, dma_read(reg));

		result = res_id;
	}
	spin_unlock(&dma_chan_lock);
	return result;
}

static int reserve_any_qdma_channel(void)
{
	int avl_id, result = -1;
	u32 mask;

	for (avl_id = 0; avl_id < EDMA_NUM_QDMACH; ++avl_id) {
		mask = 1 << (avl_id % 32);
		result = reserve_one_qdma_channel(avl_id, mask);
		if (result != -1)
			break;
	}
	return result;
}

static int reserve_one_tcc(unsigned int res_id, unsigned int res_id_mask)
{
	int result = -1, idx = res_id / 32;
	u32 reg;

	spin_lock(&dma_chan_lock);
	if ((tcc_arm[idx] & res_id_mask) != 0 &&
	    (tcc_use_status[idx] & res_id_mask) != 0) {
		pr_debug("tcc = %x\n", res_id);

		/* Mark it as non-available now */
		tcc_use_status[idx] &= ~res_id_mask;

		/* Enable the TCC in the DRAE/DRAEH registers */
		if (res_id < 32) {
			reg = EDMA_DRAE(EDMA_MASTER_SHADOW_REGION);
			SET_REG_VAL(res_id_mask, reg);
			pr_debug("DRAE = %x\n", dma_read(reg));

			/* Add it to the Allocated TCCs list */
			allocated_tccs[0] |= res_id_mask;
		} else {
			reg = EDMA_DRAEH(EDMA_MASTER_SHADOW_REGION);
			SET_REG_VAL(res_id_mask, reg);
			pr_debug("DRAEH = %x\n", dma_read(reg));

			/* Add it to the Allocated TCCs list */
			allocated_tccs[1] |= res_id_mask;
		}
		result = res_id;
	}
	spin_unlock(&dma_chan_lock);
	return result;
}

static int reserve_any_tcc(void)
{
	int avl_id, result = -1;
	u32 mask;

	for (avl_id = 0; avl_id < davinci_edma_num_tcc; ++avl_id) {
		mask = 1 << (avl_id % 32);
		if ((~edma2event_map[avl_id / 32] & mask) != 0) {
			result = reserve_one_tcc(avl_id, mask);
			if (result != -1)
				break;
		}
	}
	return result;
}

static int reserve_one_edma_param(unsigned int res_id, unsigned int res_id_mask)
{
	int result = -1, idx = res_id / 32;
	u32 reg;

	spin_lock(&dma_chan_lock);
	if ((param_entry_arm[idx] & res_id_mask) != 0 &&
	    (param_entry_use_status[idx] & res_id_mask) != 0) {
		pr_debug("EDMA param = %x\n", res_id);
		/* Mark it as non-available now */
		param_entry_use_status[idx] &= ~res_id_mask;
		result = res_id;

		/* Also, make the actual PARAM Set NULL */
		reg = EDMA_PARAM_OPT(res_id);
		memset((void *)IO_ADDRESS(reg), 0x00, EDMA_PARAM_ENTRY_SIZE);
	}
	spin_unlock(&dma_chan_lock);
	return result;
}

static int reserve_any_edma_param(void)
{
	int avl_id, result = -1;
	u32 mask;

	for (avl_id = 0; avl_id < davinci_edma_num_param; ++avl_id) {
		mask = 1 << (avl_id % 32);
		if ((~param_entry_reserved[avl_id / 32] & mask) != 0) {
			result = reserve_one_edma_param(avl_id, mask);
			if (result != -1)
				break;
		}
	}
	return result;
}

static int reserve_contiguous_params(unsigned int res_id,
				     unsigned int num_params,
				     unsigned int start_param)
{
	u32 mask;
	int i;
	unsigned int available, j;
	unsigned int count = num_params;

	for (available = start_param; available < davinci_edma_num_param;
	     ++available) {
		mask = 1 << (available % 32);
		if ((~param_entry_reserved[available / 32] & mask) &&
		    (param_entry_arm[available / 32] & mask) &&
		    (param_entry_use_status[available / 32] & mask)) {
			count--;
			if (count == 0)
				break;
		} else if (res_id == EDMA_CONT_PARAMS_FIXED_EXACT)
			return -1;
		else
			count = num_params;
	}

	if (count)
		return -1;

	for (j = available - num_params + 1; j <= available; ++j) {
		mask = 1 << (j % 32);
		spin_lock(&dma_chan_lock);

		/* Mark the PARAM as non-available now */
		param_entry_use_status[j / 32] &= ~mask;

		/* Also, make the actual PARAM Set NULL */
		for (i = 0; i < 8; i++)
			dma_write(0x0, EDMA_PARAM(0x4 * i, j));

		spin_unlock(&dma_chan_lock);
	}
	return available - num_params + 1;
}

static int alloc_cont_resource(unsigned int res_id, unsigned int num_res,
			       unsigned int param)
{
	switch (res_id) {
	case EDMA_CONT_PARAMS_ANY:
		return reserve_contiguous_params(res_id, num_res,
						 davinci_edma_num_dmach);
	case EDMA_CONT_PARAMS_FIXED_EXACT:
	case EDMA_CONT_PARAMS_FIXED_NOT_EXACT:
		return reserve_contiguous_params(res_id, num_res, param);
	default:
		return -1;
	}
}

static int alloc_resource(unsigned int res_id, enum resource_type res_type)
{
	int result = -1;
	unsigned int res_id_set = 1 << (res_id % 32);

	switch (res_type) {
	case RES_DMA_CHANNEL:
		if (res_id == EDMA_DMA_CHANNEL_ANY)
			result = reserve_any_edma_channel();
		else if (res_id < davinci_edma_num_dmach)
			result = reserve_one_edma_channel(res_id, res_id_set);
		break;
	case RES_QDMA_CHANNEL:
		if (res_id == EDMA_QDMA_CHANNEL_ANY)
			result = reserve_any_qdma_channel();
		else if (res_id < EDMA_NUM_QDMACH)
			result = reserve_one_qdma_channel(res_id, res_id_set);
		break;
	case RES_TCC:
		if (res_id == EDMA_TCC_ANY)
			result = reserve_any_tcc();
		else if (res_id < davinci_edma_num_tcc)
			result = reserve_one_tcc(res_id, res_id_set);
		break;
	case RES_PARAM_SET:
		if (res_id == DAVINCI_EDMA_PARAM_ANY)
			result = reserve_any_edma_param();
		else if (res_id < davinci_edma_num_param)
			result = reserve_one_edma_param(res_id, res_id_set);
		break;
	}
	return result;
}

static void free_resource(unsigned int res_id,
			enum resource_type res_type)
{
	unsigned int res_id_set = 1 << (res_id % 32);

	spin_lock(&dma_chan_lock);
	switch (res_type) {
	case RES_DMA_CHANNEL:
		if (res_id >= davinci_edma_num_dmach)
			break;

		if ((edma_channels_arm[res_id / 32] & res_id_set) == 0)
			break;

		if ((~dma_ch_use_status[res_id / 32] & res_id_set) == 0)
			break;

		/* Make it as available */
		dma_ch_use_status[res_id / 32] |= res_id_set;

		/* Reset the DRAE/DRAEH bit also */
		if (res_id < 32)
			CLEAR_REG_VAL(res_id_set,
				      EDMA_DRAE(EDMA_MASTER_SHADOW_REGION));
		else
			CLEAR_REG_VAL(res_id_set,
				      EDMA_DRAEH(EDMA_MASTER_SHADOW_REGION));
		break;
	case RES_QDMA_CHANNEL:
		if (res_id >= EDMA_NUM_QDMACH)
			break;

		if ((qdma_channels_arm[0] & res_id_set) == 0)
			break;

		if ((~qdma_ch_use_status[0] & res_id_set) == 0)
			break;

		/* Make it as available */
		qdma_ch_use_status[0] |= res_id_set;

		/* Reset the DRAE/DRAEH bit also */
		CLEAR_REG_VAL(res_id_set, EDMA_QRAE(EDMA_MASTER_SHADOW_REGION));
		break;
	case RES_TCC:
		if (res_id >= davinci_edma_num_tcc)
			break;

		if ((tcc_arm[res_id / 32] & res_id_set) == 0)
			break;

		if ((~tcc_use_status[res_id / 32] & res_id_set) == 0)
			break;

		/* Make it as available */
		tcc_use_status[res_id / 32] |= res_id_set;

		/* Reset the DRAE/DRAEH bit also */
		if (res_id < 32) {
			CLEAR_REG_VAL(res_id_set,
				      EDMA_DRAE(EDMA_MASTER_SHADOW_REGION));

			/* Remove it from the Allocated TCCs list */
			allocated_tccs[0] &= ~res_id_set;
		} else {
			CLEAR_REG_VAL(res_id_set,
				      EDMA_DRAEH(EDMA_MASTER_SHADOW_REGION));

			/* Remove it from the Allocated TCCs list */
			allocated_tccs[1] &= ~res_id_set;
		}
		break;
	case RES_PARAM_SET:
		if (res_id >= davinci_edma_num_param)
			break;

		if ((param_entry_arm[res_id / 32] & res_id_set) == 0)
			break;

		if ((~param_entry_use_status[res_id / 32] & res_id_set) == 0)
			break;

		/* Make it as available */
		param_entry_use_status[res_id / 32] |= res_id_set;
		break;
	}
	spin_unlock(&dma_chan_lock);
}

static int get_symm_resource(void)
{
	int i;
	u32 idx, mask;

	for (i = 0; i < davinci_edma_num_dmach; i++) {
		idx = i / 32;
		mask = 1 << (i % 32);
		if (~edma2event_map[idx] & mask) {
			if (alloc_resource(i, RES_DMA_CHANNEL) == i) {
				if (alloc_resource(i, RES_PARAM_SET) == i) {
					if (alloc_resource(i, RES_TCC) == i)
						break;
					free_resource(i, RES_DMA_CHANNEL);
					free_resource(i, RES_PARAM_SET);
				} else
					free_resource(i, RES_DMA_CHANNEL);
			}
		}
	}
	return i;
}

/*
 * EDMA3 CC Transfer Completion Interrupt Handler
 */
static irqreturn_t dma_irq_handler(int irq, void *dev_id, struct pt_regs *regs)
{
	unsigned int cnt = 0;

	if (!(dma_read(EDMA_SH_IPR(0)) || dma_read(EDMA_SH_IPRH(0))))
		return IRQ_NONE;

	/* Loop while cnt < 10, breaks when no pending interrupt is found */
	while (cnt < 10) {
		u32 status_l = dma_read(EDMA_SH_IPR(0));
		u32 status_h = dma_read(EDMA_SH_IPRH(0));
		int lch, i;

		status_h &= allocated_tccs[1];
		if (!(status_l || status_h))
			break;

		lch = 0;
		while (status_l) {
			i = ffs(status_l);
			lch += i;

			/*
			 * If the user has not given any callback function
			 * while requesting the TCC, its TCC specific bit
			 * in the IPR register will NOT be cleared.
			 */
			if (intr_data[lch - 1].callback) {
				/* Clear the corresponding IPR bits */
				SET_REG_VAL(1 << (lch - 1), EDMA_SH_ICR(0));

				/* Call the callback function now */
				dma_handle_cb(lch - 1, DMA_COMPLETE);
			}
			status_l >>= i;
		}

		lch = 32;
		while (status_h) {
			i = ffs(status_h);
			lch += i;

			/*
			 * If the user has not given any callback function
			 * while requesting the TCC, its TCC specific bit
			 * in the IPRH register will NOT be cleared.
			 */
			if (intr_data[lch - 1].callback) {
				/* Clear the corresponding IPR bits */
				SET_REG_VAL(1 << (lch - 33), EDMA_SH_ICRH(0));

				/* Call the callback function now */
				dma_handle_cb(lch - 1, DMA_COMPLETE);
			}
			status_h >>= i;
		}

		cnt++;
	}

	dma_write(0x1, EDMA_SH_IEVAL(0));

	return IRQ_HANDLED;
}

/*
 * EDMA3 CC Error Interrupt Handler
 */
static irqreturn_t dma_ccerr_handler(int irq, void *dev_id,
				     struct pt_regs *regs)
{
	unsigned int mapped_tcc = 0;

	if (!(dma_read(EDMA_EMR) || dma_read(EDMA_EMRH) ||
	      dma_read(EDMA_QEMR) || dma_read(EDMA_CCERR)))
		return IRQ_NONE;

	while (1) {
		u32 status_emr = dma_read(EDMA_EMR);
		u32 status_emrh = dma_read(EDMA_EMRH);
		u32 status_qemr = dma_read(EDMA_QEMR);
		u32 status_ccerr = dma_read(EDMA_CCERR);
		int lch, i;

		if (!(status_emr || status_emrh || status_qemr || status_ccerr))
			break;

		lch = 0;
		while (status_emr) {
			i = ffs(status_emr);
			lch += i;
			/* Clear the corresponding EMR bits */
			SET_REG_VAL(1 << (lch - 1), EDMA_EMCR);
			/* Clear any SER */
			SET_REG_VAL(1 << (lch - 1), EDMA_SH_SECR(0));

			mapped_tcc = edma_dma_ch_tcc_mapping[lch - 1];
			dma_handle_cb(mapped_tcc, DMA_CC_ERROR);
			status_emr >>= i;
		}

		lch = 32;
		while (status_emrh) {
			i = ffs(status_emrh);
			lch += i;
			/* Clear the corresponding IPR bits */
			SET_REG_VAL(1 << (lch - 33), EDMA_EMCRH);
			/* Clear any SER */
			SET_REG_VAL(1 << (lch - 33), EDMA_SH_SECRH(0));

			mapped_tcc = edma_dma_ch_tcc_mapping[lch - 1];
			dma_handle_cb(mapped_tcc, DMA_CC_ERROR);
			status_emrh >>= i;
		}

		lch = 0;
		while (status_qemr) {
			i = ffs(status_qemr);
			lch += i;
			/* Clear the corresponding IPR bits */
			SET_REG_VAL(1 << (lch - 1), EDMA_QEMCR);
			SET_REG_VAL(1 << (lch - 1), EDMA_SH_QSECR(0));

			mapped_tcc = edma_qdma_ch_tcc_mapping[lch - 1];
			dma_handle_cb(mapped_tcc, QDMA_EVT_MISS_ERROR);
			status_qemr >>= i;
		}


		lch = 0;
		while (status_ccerr) {
			i = ffs(status_ccerr);
			lch += i;
			/* Clear the corresponding IPR bits */
			SET_REG_VAL(1 << (lch - 1), EDMA_CCERRCLR);
			status_ccerr >>= i;
		}
	}
	dma_write(0x1, EDMA_EEVAL);

	return IRQ_HANDLED;
}

/*
 * EDMA3 Transfer Controller Error Interrupt Handler
 */
static int dma_tc_err_handler(unsigned int tc_num)
{
	u32 tcregs, err_stat;

	if (tc_num >= davinci_edma_num_tc)
		return -EINVAL;

	tcregs = davinci_edmatc_base_addrs[tc_num];
	if (tcregs == 0)
		return 0;

	err_stat = dma_read(EDMATC_ERRSTAT(tcregs));
	if (err_stat) {
		if (err_stat & (1 << EDMA_TC_ERRSTAT_BUSERR_SHIFT))
			dma_write(1 << EDMA_TC_ERRSTAT_BUSERR_SHIFT,
				  EDMATC_ERRCLR(tcregs));

		if (err_stat & (1 << EDMA_TC_ERRSTAT_TRERR_SHIFT))
			dma_write(1 << EDMA_TC_ERRSTAT_TRERR_SHIFT,
				  EDMATC_ERRCLR(tcregs));

		if (err_stat & (1 << EDMA_TC_ERRSTAT_MMRAERR_SHIFT))
			dma_write(1 << EDMA_TC_ERRSTAT_MMRAERR_SHIFT,
				  EDMATC_ERRCLR(tcregs));
	}
	return 0;
}

/*
 * EDMA3 TC0 Error Interrupt Handler
 */
static irqreturn_t dma_tc0_err_handler(int irq, void *dev_id,
				       struct pt_regs *data)
{
	/* Invoke Error Handler ISR for TC0 */
	dma_tc_err_handler(0);

	return IRQ_HANDLED;
}

/*
 * EDMA3 TC1 Error Interrupt Handler
 */
static irqreturn_t dma_tc1_err_handler(int irq, void *dev_id,
				      struct pt_regs *data)
{
	/* Invoke Error Handler ISR for TC1*/
	dma_tc_err_handler(1);

	return IRQ_HANDLED;
}

/*
 * EDMA3 TC2 Error Interrupt Handler
 */
static irqreturn_t dma_tc2_err_handler(int irq, void *dev_id,
				      struct pt_regs *data)
{
	/* Invoke Error Handler ISR for TC2*/
	dma_tc_err_handler(2);

	return IRQ_HANDLED;
}

/*
 * EDMA3 TC3 Error Interrupt Handler
 */
static irqreturn_t dma_tc3_err_handler(int irq, void *dev_id,
				       struct pt_regs *data)
{
	/* Invoke Error Handler ISR for TC3*/
	dma_tc_err_handler(3);

	return IRQ_HANDLED;
}

/*
 * EDMA3 TC4 Error Interrupt Handler
 */
static irqreturn_t dma_tc4_err_handler(int irq, void *dev_id,
				       struct pt_regs *data)
{
	/* Invoke Error Handler ISR for TC4*/
	dma_tc_err_handler(4);

	return IRQ_HANDLED;
}

/*
 * EDMA3 TC5 Error Interrupt Handler
 */
static irqreturn_t dma_tc5_err_handler(int irq, void *dev_id,
				       struct pt_regs *data)
{
	/* Invoke Error Handler ISR for TC5*/
	dma_tc_err_handler(5);

	return IRQ_HANDLED;
}

/*
 * EDMA3 TC6 Error Interrupt Handler
 */
static irqreturn_t dma_tc6_err_handler(int irq, void *dev_id,
				       struct pt_regs *data)
{
	/* Invoke Error Handler ISR for TC6*/
	dma_tc_err_handler(6);

	return IRQ_HANDLED;
}

/*
 * EDMA3 TC7 Error Interrupt Handler
 */
static irqreturn_t dma_tc7_err_handler(int irq, void *dev_id,
				       struct pt_regs *data)
{
	/* Invoke Error Handler ISR for TC7*/
	dma_tc_err_handler(7);

	return IRQ_HANDLED;
}

/*
 * davinci_get_qdma_channel - convert QDMA channel to logical channel
 * Arguments:
 *	ch	- input QDMA channel.
 *
 * Return: logical channel associated with QDMA channel or logical channel
 *	   associated with QDMA channel 0 for out of range channel input.
 */
int davinci_get_qdma_channel(int ch)
{
	if (ch >= 0 || ch <= EDMA_MAX_CHANNEL)
		return davinci_qdma_ch_map + ch;
	else    /* return channel 0 for out of range values */
		return davinci_qdma_ch_map;
}
EXPORT_SYMBOL(davinci_get_qdma_channel);

/*
 * davinci_request_dma - requests for the DMA device passed if it is free
 *
 * Arguments:
 *	dev_id	   - request for the PaRAM entry device ID
 *	dev_name   - device name
 *	callback   - pointer to the channel callback.
 *	Arguments:
 *	    lch   - channel number which is the IPR bit position,
 *		    indicating from which channel the interrupt arised.
 *	    data  - channel private data, which is received as one of the
 *		    arguments in davinci_request_dma.
 *	data	  - private data for the channel to be requested which is used
 *		    to pass as a parameter in the callback function in IRQ
 *		    handler.
 *	lch	  - contains the device id allocated
 *	tcc	  - Transfer Completion Code, used to set the IPR register bit
 *		    after transfer completion on that channel.
 *	eventq_no - Event Queue no to which the channel will be associated with
 *		    (valid only if you are requesting for a DMA MasterChannel).
 *		    Values : 0 to 7
 *
 * Return: zero on success, or corresponding error number on failure
 */
int davinci_request_dma(int dev_id, const char *dev_name,
			void (*callback)(int lch, u16 ch_status, void *data),
			void *data, int *lch, int *tcc,
			enum dma_event_q eventq_no)
{
	int ret_code = 0, param_id, tcc_val, symm;
	u32 reg;

	pr_debug("[%s]: start, dev_id = %d, dev_name = %s\n",
		 __func__, dev_id, dev_name != NULL ? dev_name : "");

	/* Validating the arguments passed first */
	if (lch == NULL || tcc == NULL || eventq_no >= davinci_edma_num_evtq) {
		ret_code = -EINVAL;
		goto request_dma_exit;
	}

	if (dev_id >= 0 && dev_id < davinci_edma_num_dmach) {
		if (alloc_resource(dev_id, RES_DMA_CHANNEL) != dev_id) {
			/* DMA channel allocation failed */
			pr_debug("DMA channel allocation failed\n");
			ret_code = -EINVAL;
			goto request_dma_exit;
		}
		*lch = dev_id;
		pr_debug("DMA channel %d allocated\n", *lch);

		/*
		 * Allocate PaRAM Set.
		 * 64 DMA Channels are mapped to the first 64 PaRAM entries.
		 */
		if (alloc_resource(dev_id, RES_PARAM_SET) != dev_id) {
			/* PaRAM Set allocation failed */
			/* free previously allocated resources */
			free_resource(dev_id, RES_DMA_CHANNEL);

			pr_debug("PaRAM Set allocation failed\n");
			ret_code = -EINVAL;
			goto request_dma_exit;
		}

		/* Allocate TCC (1-to-1 mapped with the DMA channel) */
		if (alloc_resource(dev_id, RES_TCC) != dev_id) {
			/* TCC allocation failed */
			/* free previously allocated resources */
			free_resource(dev_id, RES_PARAM_SET);
			free_resource(dev_id, RES_DMA_CHANNEL);

			pr_debug("TCC allocation failed\n");
			ret_code = -EINVAL;
			goto request_dma_exit;
		}

		param_id = dev_id;
		spin_lock(&dma_chan_lock);
		dma_ch_bound_res[dev_id].param_id = param_id;
		spin_unlock(&dma_chan_lock);
		pr_debug("PaRAM Set %d allocated\n", param_id);

		*tcc = dev_id;
		pr_debug("TCC %d allocated\n", *tcc);

		spin_lock(&dma_chan_lock);
		dma_ch_bound_res[dev_id].tcc = *tcc;
		spin_unlock(&dma_chan_lock);

		/*
		 * All resources allocated.
		 * Store the mapping b/w DMA channel and TCC first.
		 */
		edma_dma_ch_tcc_mapping[*lch] = *tcc;

		/* Register callback function */
		register_callback(*tcc, callback, data);

		/* Map DMA channel to event queue */
		map_dma_ch_evt_queue(*lch, eventq_no);

		/* Map DMA channel to PaRAM Set */
		map_dma_ch_param_set(*lch, param_id);

	} else if (dma_is_qdmach(dev_id)) {
		/*
		 * Allocate QDMA channel first.
		 * Modify the *lch to point it to the correct QDMA
		 * channel and then check whether the same channel
		 * has been allocated or not.
		 */
		*lch = dev_id - EDMA_QDMA_CHANNEL_0;
		if (alloc_resource(*lch, RES_QDMA_CHANNEL) != *lch) {
			/* QDMA Channel allocation failed */
			pr_debug("QDMA channel allocation failed\n");
			ret_code = -EINVAL;
			goto request_dma_exit;
		}

		/* Requested Channel allocated successfully */
		*lch = dev_id;
		pr_debug("QDMA channel %d allocated\n", *lch);

		/* Allocate param set */
		param_id = alloc_resource(DAVINCI_EDMA_PARAM_ANY,
					  RES_PARAM_SET);

		if (param_id == -1) {
			/*
			 * PaRAM Set allocation failed --
			 * Free previously allocated resources.
			 */
			free_resource(dev_id - EDMA_QDMA_CHANNEL_0,
				      RES_QDMA_CHANNEL);

			pr_debug("PaRAM channel allocation failed\n");
			ret_code = -EINVAL;
			goto request_dma_exit;
		}
		pr_debug("PaRAM Set %d allocated\n", param_id);

		/* Allocate TCC */
		tcc_val = alloc_resource(*tcc, RES_TCC);
		if (tcc_val == -1) {
			/*
			 * TCC allocation failed --
			 * Free previously allocated resources.
			 */
			free_resource(param_id, RES_PARAM_SET);

			free_resource(dev_id - EDMA_QDMA_CHANNEL_0,
				      RES_QDMA_CHANNEL);

			pr_debug("TCC allocation failed\n");
			ret_code = -EINVAL;
			goto request_dma_exit;
		}

		pr_debug("TCC %d allocated\n", tcc_val);
		*tcc = tcc_val;

		spin_lock(&dma_chan_lock);
		dma_ch_bound_res[*lch].param_id = param_id;
		dma_ch_bound_res[*lch].tcc = *tcc;
		spin_unlock(&dma_chan_lock);

		/*
		 * All resources allocated.
		 * Store the mapping b/w QDMA channel and TCC first.
		 */
		edma_qdma_ch_tcc_mapping[*lch - EDMA_QDMA_CHANNEL_0] = *tcc;

		/* Register callback function */
		register_callback(*tcc, callback, data);

		/* Map QDMA channel to event queue */
		map_qdma_ch_evt_queue(*lch - EDMA_QDMA_CHANNEL_0, eventq_no);

		/* Map QDMA channel to PaRAM Set */
		map_qdma_ch_param_set(*lch - EDMA_QDMA_CHANNEL_0, param_id);

	} else if (dev_id == EDMA_DMA_CHANNEL_ANY) {
		if (*tcc == EDMA_TCC_SYMM) {
			symm = get_symm_resource();

			if (symm >= 0 && symm < davinci_edma_num_dmach) {
				*lch = symm;
				pr_debug("EDMA_DMA_CHANNEL_ANY:: "
					 "channel %d allocated\n", (*lch));

				param_id = symm;
				pr_debug("EDMA_DMA_CHANNEL_ANY:: "
					 "param %d allocated\n", param_id);

				*tcc = symm;
				pr_debug("EDMA_DMA_CHANNEL_ANY:: "
					 "tcc %d allocated\n", (*tcc));
			} else {
				/*
				 * Failed to get an EDMA Channel with
				 * a symmetric PARAM and TCC
				 */
				pr_debug("Channel allocation with a symmetric"
					 "PARAM and TCC failed \r\n");
				ret_code = -EINVAL;
				goto request_dma_exit;
			}
		} else {
			*lch = alloc_resource(EDMA_DMA_CHANNEL_ANY,
					      RES_DMA_CHANNEL);
			if (*lch == -1) {
				pr_debug("EINVAL\n");
				ret_code = -EINVAL;
				goto request_dma_exit;
			}
			pr_debug("EDMA_DMA_CHANNEL_ANY:: "
				 "channel %d allocated\n", *lch);

			/*
			 * Allocate PARAM set tied to the
			 * DMA channel (1-to-1 mapping)
			 */
			param_id = alloc_resource(*lch, RES_PARAM_SET);
			if (param_id == -1) {
				/*
				 * PaRAM Set allocation failed, free previously
				 * allocated resources.
				 */
				pr_debug("PaRAM Set allocation failed\n");
				free_resource(*lch, RES_DMA_CHANNEL);
				ret_code = -EINVAL;
				goto request_dma_exit;
			}
			pr_debug("EDMA_DMA_CHANNEL_ANY:: "
				 "PaRAM Set %d allocated\n", param_id);

			/* Allocate TCC */
			*tcc = alloc_resource(*tcc, RES_TCC);

			if (*tcc == -1) {
				/* free previously allocated resources */
				free_resource(param_id, RES_PARAM_SET);
				free_resource(*lch, RES_DMA_CHANNEL);

				pr_debug("free resource\n");
				ret_code = -EINVAL;
				goto request_dma_exit;
			}
			pr_debug("EDMA_DMA_CHANNEL_ANY:: "
				 "TCC %d allocated\n", *tcc);
		}

		spin_lock(&dma_chan_lock);
		dma_ch_bound_res[*lch].param_id = param_id;
		dma_ch_bound_res[*lch].tcc = *tcc;
		spin_unlock(&dma_chan_lock);

		/*
		 * All resources allocated.
		 * Store the mapping b/w DMA channel and TCC first.
		 */
		edma_dma_ch_tcc_mapping[*lch] = *tcc;

		/* Register callback function */
		register_callback(*tcc, callback, data);

		/* Map DMA channel to event queue */
		map_dma_ch_evt_queue(*lch, eventq_no);

		/* Map DMA channel to PaRAM Set */
		map_dma_ch_param_set(*lch, param_id);

	} else if (dev_id == EDMA_QDMA_CHANNEL_ANY) {
		*lch = alloc_resource(dev_id, RES_QDMA_CHANNEL);

		if (*lch == -1) {
			/* QDMA Channel allocation failed */
			ret_code = -EINVAL;
			goto request_dma_exit;
		}
		/* Channel allocated successfully */
		*lch += EDMA_QDMA_CHANNEL_0;

		pr_debug("EDMA_QDMA_CHANNEL_ANY: channel %d allocated\n", *lch);

		/* Allocate param set */
		param_id = alloc_resource(DAVINCI_EDMA_PARAM_ANY,
					  RES_PARAM_SET);
		if (param_id == -1) {
			/*
			 * PaRAM Set allocation failed, free previously
			 * allocated resources.
			 */
			free_resource(dev_id - EDMA_QDMA_CHANNEL_0,
				      RES_QDMA_CHANNEL);
			ret_code = -EINVAL;
			goto request_dma_exit;
		}
		pr_debug("EDMA_QDMA_CHANNEL_ANY: PaRAM Set %d allocated\n",
			 param_id);

		/* Allocate TCC */
		tcc_val = alloc_resource(*tcc, RES_TCC);
		if (tcc_val == -1) {
			/* free previously allocated resources */
			free_resource(param_id, RES_PARAM_SET);
			free_resource(dev_id - EDMA_QDMA_CHANNEL_0,
				      RES_QDMA_CHANNEL);
			ret_code = -EINVAL;
			goto request_dma_exit;
		}
		pr_debug("EDMA_QDMA_CHANNEL_ANY: TCC %d allocated\n", tcc_val);
		*tcc = tcc_val;

		spin_lock(&dma_chan_lock);
		dma_ch_bound_res[*lch].param_id = param_id;
		dma_ch_bound_res[*lch].tcc = *tcc;
		spin_unlock(&dma_chan_lock);

		/*
		 * All resources allocated.
		 * Store the mapping b/w QDMA channel and TCC first.
		 */
		edma_qdma_ch_tcc_mapping[*lch - EDMA_QDMA_CHANNEL_0] = *tcc;

		/* Register callback function */
		register_callback(*tcc, callback, data);

		/* Map QDMA channel to event queue */
		map_qdma_ch_evt_queue(*lch - EDMA_QDMA_CHANNEL_0, eventq_no);

		/* Map QDMA channel to PaRAM Set */
		map_qdma_ch_param_set(*lch - EDMA_QDMA_CHANNEL_0, param_id);

	} else if (dev_id == DAVINCI_EDMA_PARAM_ANY) {
		/* Check for the valid TCC */
		if (*tcc >= davinci_edma_num_tcc) {
			/* Invalid TCC passed. */
			ret_code = -EINVAL;
			goto request_dma_exit;
		}

		/* Allocate a PaRAM Set */
		*lch = alloc_resource(dev_id, RES_PARAM_SET);
		if (*lch == -1) {
			ret_code = -EINVAL;
			goto request_dma_exit;
		}
		pr_debug("DAVINCI_EDMA_PARAM_ANY: link channel %d allocated\n",
			 *lch);

		/* link channel allocated */
		spin_lock(&dma_chan_lock);
		dma_ch_bound_res[*lch].param_id = *lch;
		spin_unlock(&dma_chan_lock);

		/* Assign the link field to NO link. i.e 0xFFFF */
		SET_REG_VAL(0xFFFFu, EDMA_PARAM_LINK_BCNTRLD(*lch));

		/*
		 * Check whether user has passed a NULL TCC or not.
		 * If it is not NULL, use that value to set the OPT.TCC field
		 * of the link channel and enable the interrupts also.
		 * Otherwise, disable the interrupts.
		 */
		reg = EDMA_PARAM_OPT(*lch);
		if (*tcc >= 0) {
			/* Set the OPT.TCC field */
			CLEAR_REG_VAL(TCC, reg);
			SET_REG_VAL((0x3F & *tcc) << 12, reg);

			/* Set TCINTEN bit in PaRAM entry */
			SET_REG_VAL(TCINTEN, reg);

			/* Store the TCC also */
			spin_lock(&dma_chan_lock);
			dma_ch_bound_res[*lch].tcc = *tcc;
			spin_unlock(&dma_chan_lock);
		} else
			CLEAR_REG_VAL(TCINTEN, reg);
		goto request_dma_exit;
	} else {
		ret_code = -EINVAL;
		goto request_dma_exit;
	}

	reg = EDMA_PARAM_OPT(param_id);
	if (callback) {
		CLEAR_REG_VAL(TCC, reg);
		SET_REG_VAL((0x3F & *tcc) << 12, reg);

		/* Set TCINTEN bit in PaRAM entry */
		SET_REG_VAL(TCINTEN, reg);
	} else
		CLEAR_REG_VAL(TCINTEN, reg);

	/* Assign the link field to NO link. i.e 0xFFFF */
	SET_REG_VAL(0xFFFFu, EDMA_PARAM_LINK_BCNTRLD(param_id));

request_dma_exit:
	pr_debug("[%s]: end\n", __func__);
	return ret_code;
}
EXPORT_SYMBOL(davinci_request_dma);

/*
 * davinci_free_dma - free DMA channel
 * Arguments:
 *	dev_id	- request for the PaRAM entry device ID
 *
 * Return: zero on success, or corresponding error no on failure
 */
int davinci_free_dma(int lch)
{
	int param_id, tcc, ret_code = 0;

	pr_debug("[%s]: start, lch = %d\n", __func__, lch);

	if (lch >= 0 && lch < davinci_edma_num_dmach) {
		/* Disable any ongoing transfer first */
		davinci_stop_dma(lch);

		/* Un-register the callback function */
		unregister_callback(lch, RES_DMA_CHANNEL);

		/* Remove DMA channel to PaRAM Set mapping */
		if (davinci_edma_chmap_exist == 1)
			CLEAR_REG_VAL(DMACH_PARAM_CLR_MASK, EDMA_DCHMAP(lch));

		param_id = dma_ch_bound_res[lch].param_id;
		tcc = dma_ch_bound_res[lch].tcc;

		pr_debug("Free PaRAM Set %d\n", param_id);
		free_resource(param_id, RES_PARAM_SET);
		spin_lock(&dma_chan_lock);
		dma_ch_bound_res[lch].param_id = 0;
		spin_unlock(&dma_chan_lock);

		pr_debug("Free TCC %d\n", tcc);
		free_resource(tcc, RES_TCC);
		spin_lock(&dma_chan_lock);
		dma_ch_bound_res[lch].tcc = 0;
		spin_unlock(&dma_chan_lock);

		pr_debug("Free DMA channel %d\n", lch);
		free_resource(lch, RES_DMA_CHANNEL);
	} else  if (lch >= davinci_edma_num_dmach &&
		    lch <  davinci_edma_num_param) {
		param_id = dma_ch_bound_res[lch].param_id;

		pr_debug("Free LINK channel %d\n", param_id);
		free_resource(param_id, RES_PARAM_SET);
		spin_lock(&dma_chan_lock);
		dma_ch_bound_res[lch].param_id = 0;
		spin_unlock(&dma_chan_lock);
	} else if (dma_is_qdmach(lch)) {
		/* Disable any ongoing transfer first */
		davinci_stop_dma(lch);

		/* Un-register the callback function */
		unregister_callback(lch, RES_QDMA_CHANNEL);

		/* Remove QDMA channel to PaRAM Set mapping */
		CLEAR_REG_VAL(QDMACH_PARAM_CLR_MASK,
			      EDMA_QCHMAP(lch - EDMA_QDMA_CHANNEL_0));
		/* Reset trigger word */
		CLEAR_REG_VAL(QDMACH_TRWORD_CLR_MASK,
			      EDMA_QCHMAP(lch - EDMA_QDMA_CHANNEL_0));

		param_id = dma_ch_bound_res[lch].param_id;
		tcc = dma_ch_bound_res[lch].tcc;

		pr_debug("Free ParamSet %d\n", param_id);
		free_resource(param_id, RES_PARAM_SET);
		spin_lock(&dma_chan_lock);
		dma_ch_bound_res[lch].param_id = 0;
		spin_unlock(&dma_chan_lock);

		pr_debug("Free TCC %d\n", tcc);
		free_resource(tcc, RES_TCC);
		spin_lock(&dma_chan_lock);
		dma_ch_bound_res[lch].tcc = 0;
		spin_unlock(&dma_chan_lock);

		pr_debug("Free QDMA channel %d\n", lch);
		free_resource(lch - EDMA_QDMA_CHANNEL_0, RES_QDMA_CHANNEL);
	} else
		ret_code = -1;

	pr_debug("[%s]: end\n", __func__);
	return ret_code;
}
EXPORT_SYMBOL(davinci_free_dma);

/*
 * The API will return the starting point of a set of
 * contiguous PARAM's that have been requested
 * Return value on failure will be -1
 * Arguments:
 * 	res_id - can only be EDMA_CONT_PARAMS_ANY or
 *		 EDMA_CONT_PARAMS_FIXED_EXACT or
 *		 EDMA_CONT_PARAMS_FIXED_NOT_EXACT
 * 	num    - number of contiguous PARAM's
 *	param  - the start value of PARAM that should be passed if res_id
 *		 is EDMA_CONT_PARAMS_FIXED_EXACT or
 *		 EDMA_CONT_PARAMS_FIXED_NOT_EXACT
 */
int davinci_request_params(unsigned int res_id, unsigned int num,
			   unsigned int param)
{
	int result = -1, i;

	pr_debug("[%s]: start\n", __func__);

	if (!num) {
		pr_debug("Number of PARAM sets requested cannot be 0\n");
		return -EINVAL;
	}

	result = alloc_cont_resource(res_id, num, param);
	if (result != -1) {
		spin_lock(&dma_chan_lock);

		for (i = result; i < result + num; ++i)
			dma_ch_bound_res[i].param_id = i;

		spin_unlock(&dma_chan_lock);
	}

	pr_debug("[%s]: end\n", __func__);

	return result;
}
EXPORT_SYMBOL(davinci_request_params);

/*
 * The API will free a set of contiguous PARAM sets
 * Arguments:
 * start - start location of PARAM sets
 * num   - num of contiguous PARAM sets
 */
void davinci_free_params(unsigned int start, unsigned int num)
{
	int i;
	u32 mask;

	pr_debug("[%s]: start\n", __func__);

	spin_lock(&dma_chan_lock);

	for (i = start; i < start + num; ++i) {
		mask = 1 << (i % 32);
		param_entry_use_status[i / 32] |= mask;
	}

	spin_unlock(&dma_chan_lock);
	pr_debug("[%s]: end\n", __func__);
}
EXPORT_SYMBOL(davinci_free_params);

/*
 * The API will return the PARAM associated with a logical channel
 * Return value on success will be the value of the PARAM
 * Arguments:
 *     lch - logical channel number
 */
int davinci_get_param(int lch)
{
	return dma_ch_bound_res[lch].param_id;
}
EXPORT_SYMBOL(davinci_get_param);

/*
 * The API will return the TCC associated with a logical channel
 * Return value on success will be the value of the TCC
 * Arguments:
 *     lch - logical channel number
 */
int davinci_get_tcc(int lch)
{
	return dma_ch_bound_res[lch].tcc;
}
EXPORT_SYMBOL(davinci_get_tcc);

/*
 * DMA source parameters setup
 * Arguments:
 *	lch	 - logical channel number
 *	src_port - source port address
 *	mode	 - indicates wether addressing mode is FIFO
 */
int davinci_set_dma_src_params(int lch, u32 src_port,
			       enum address_mode mode, enum fifo_width width)
{
	int ret_code, param_id;
	u32 reg;

	pr_debug("[%s]: start\n", __func__);

	if (!(lch >= 0 && lch < edma_max_logical_ch)) {
		ret_code = -1;
		goto exit;
	}

	/* Address in FIFO mode not 32 bytes aligned */
	if (mode && (src_port & 0x1Fu) != 0) {
		ret_code = -1;
		goto exit;
	}

	param_id = dma_ch_bound_res[lch].param_id;

	/* Set the source port address in source register of PaRAM structure */
	dma_write(src_port, EDMA_PARAM_SRC(param_id));

	/* Set the FIFO addressing mode */
	if (mode) {
		reg = EDMA_PARAM_OPT(param_id);
		/* reset SAM and FWID */
		CLEAR_REG_VAL(SAM | EDMA_FWID, reg);
		/* set SAM and program FWID */
		SET_REG_VAL(SAM | ((width & 0x7) << 8), reg);
	}
	ret_code = 0;
exit:
	pr_debug("[%s]: end\n", __func__);
	return ret_code;
}
EXPORT_SYMBOL(davinci_set_dma_src_params);

/*
 * DMA destination parameters setup
 * Arguments:
 *	lch	  - logical channel number or PaRAM device
 *	dest_port - destination port address
 *	mode	  - indicates wether addressing mode is FIFO
 */

int davinci_set_dma_dest_params(int lch, u32 dest_port,
				enum address_mode mode, enum fifo_width width)
{
	int ret_code, param_id;
	u32 reg;

	pr_debug("[%s]: start\n", __func__);

	if (!(lch >= 0 && lch < edma_max_logical_ch)) {
		ret_code = -1;
		goto exit;
	}

	if (mode && (dest_port & 0x1F) != 0) {
		/* Address in FIFO mode not 32-byte aligned */
		ret_code = -1;
		goto exit;
	}

	param_id = dma_ch_bound_res[lch].param_id;

	/* Set the dest port address in dest register of PaRAM structure */
	dma_write(dest_port, EDMA_PARAM_DST(param_id));

	/* Set the FIFO addressing mode */
	if (mode) {
		reg = EDMA_PARAM_OPT(param_id);
		/* reset DAM and FWID */
		CLEAR_REG_VAL((DAM | EDMA_FWID), reg);
		/* set DAM and program FWID */
		SET_REG_VAL(DAM | ((width & 0x7) << 8), reg);
	}
	ret_code = 0;
exit:
	pr_debug("[%s]: end\n", __func__);
	return ret_code;
}
EXPORT_SYMBOL(davinci_set_dma_dest_params);

/*
 * DMA source index setup
 * Arguments:
 *	lch	 - logical channel number or param device
 *	src_bidx - source B-register index
 *	src_cidx - source C-register index
 */

int davinci_set_dma_src_index(int lch, u16 src_bidx, u16 src_cidx)
{
	int ret_code, param_id;
	u32 reg;

	pr_debug("[%s]: start\n", __func__);

	if (!(lch >= 0 && lch < edma_max_logical_ch)) {
		ret_code = -1;
		goto exit;
	}

	param_id = dma_ch_bound_res[lch].param_id;

	reg = EDMA_PARAM_SRC_DST_BIDX(param_id);
	CLEAR_REG_VAL(0xffff, reg);
	SET_REG_VAL(src_bidx, reg);

	reg = EDMA_PARAM_SRC_DST_CIDX(param_id);
	CLEAR_REG_VAL(0xffff, reg);
	SET_REG_VAL(src_cidx, reg);
	ret_code = 0;
exit:
	pr_debug("[%s]: end\n", __func__);
	return ret_code;
}
EXPORT_SYMBOL(davinci_set_dma_src_index);

/*
 * DMA destination index setup
 * Arguments:
 *	lch	  - logical channel number or param device
 *	dest_bidx - destination B-register index
 *	dest_cidx - destination C-register index
 */

int davinci_set_dma_dest_index(int lch, u16 dest_bidx, u16 dest_cidx)
{
	int ret_code, param_id;
	u32 reg;

	pr_debug("[%s]: start\n", __func__);
	if (!(lch >= 0 && lch < edma_max_logical_ch)) {
		ret_code = -1;
		goto exit;
	}

	param_id = dma_ch_bound_res[lch].param_id;

	reg = EDMA_PARAM_SRC_DST_BIDX(param_id);
	CLEAR_REG_VAL(0xffff0000, reg);
	SET_REG_VAL(dest_bidx << 16, reg);

	reg = EDMA_PARAM_SRC_DST_CIDX(param_id);
	CLEAR_REG_VAL(0xffff0000, reg);
	SET_REG_VAL(dest_cidx << 16, reg);
	ret_code = 0;
exit:
	pr_debug("[%s]: end\n", __func__);
	return ret_code;
}
EXPORT_SYMBOL(davinci_set_dma_dest_index);

/*
 * DMA transfer parameters setup
 * ARGUMENTS:
 *	lch	- channel or param device for configuration of aCount, bCount,
 *		  and cCount registers
 *	acnt	- acnt register value to be configured
 *	bcnt	- bcnt register value to be configured
 *	ccnt	- ccnt register value to be configured
 */
int davinci_set_dma_transfer_params(int lch, u16 acnt, u16 bcnt, u16 ccnt,
				    u16 bcntrld, enum sync_dimension sync_mode)
{
	int param_id, ret_code = 0;
	u32 reg;

	pr_debug("[%s]: start\n", __func__);

	if (lch >= 0 && lch < edma_max_logical_ch) {

		param_id = dma_ch_bound_res[lch].param_id;

		reg = EDMA_PARAM_LINK_BCNTRLD(param_id);
		CLEAR_REG_VAL(0xffff0000, reg);
		SET_REG_VAL(((u32)bcntrld & 0xffff) << 16, reg);

		reg = EDMA_PARAM_OPT(param_id);
		if (sync_mode == ASYNC)
			CLEAR_REG_VAL(SYNCDIM, reg);
		else
			SET_REG_VAL(SYNCDIM, reg);

		/* Set the acount, bcount, ccount registers */
		dma_write((((u32)bcnt & 0xffff) << 16) | acnt,
			  EDMA_PARAM_A_B_CNT(param_id));
		dma_write(ccnt, EDMA_PARAM_CCNT(param_id));
	} else
		ret_code = -1;

	pr_debug("[%s]: end\n", __func__);
	return ret_code;
}
EXPORT_SYMBOL(davinci_set_dma_transfer_params);

/*
 * davinci_set_dma_params -
 * ARGUMENTS:
 *	lch - logical channel number
 */
int davinci_set_dma_params(int lch, struct paramentry_descriptor *d)
{
	int param_id, ret_code = 0;

	pr_debug("[%s]: start\n", __func__);

	if (d != NULL && lch >= 0 && lch < edma_max_logical_ch) {
		param_id = dma_ch_bound_res[lch].param_id;

		dma_write(d->opt, EDMA_PARAM_OPT(param_id));
		dma_write(d->src, EDMA_PARAM_SRC(param_id));
		dma_write(d->a_b_cnt, EDMA_PARAM_A_B_CNT(param_id));
		dma_write(d->dst, EDMA_PARAM_DST(param_id));
		dma_write(d->src_dst_bidx, EDMA_PARAM_SRC_DST_BIDX(param_id));
		dma_write(d->link_bcntrld, EDMA_PARAM_LINK_BCNTRLD(param_id));
		dma_write(d->src_dst_cidx, EDMA_PARAM_SRC_DST_CIDX(param_id));
		dma_write(d->ccnt, EDMA_PARAM_CCNT(param_id));
	} else
		ret_code = -1;

	pr_debug("[%s]: end\n", __func__);
	return ret_code;
}
EXPORT_SYMBOL(davinci_set_dma_params);

/*
 * davinci_get_dma_params -
 * ARGUMENTS:
 *	lch - logical channel number
 */
int davinci_get_dma_params(int lch, struct paramentry_descriptor *d)
{
	int ret_code, param_id;

	pr_debug("[%s]: start\n", __func__);

	if (d == NULL || lch >= edma_max_logical_ch) {
		ret_code = -1;
		goto exit;
	}

	param_id = dma_ch_bound_res[lch].param_id;

	d->opt = dma_read(EDMA_PARAM_OPT(param_id));
	d->src = dma_read(EDMA_PARAM_SRC(param_id));
	d->a_b_cnt = dma_read(EDMA_PARAM_A_B_CNT(param_id));
	d->dst = dma_read(EDMA_PARAM_DST(param_id));
	d->src_dst_bidx = dma_read(EDMA_PARAM_SRC_DST_BIDX(param_id));
	d->link_bcntrld = dma_read(EDMA_PARAM_LINK_BCNTRLD(param_id));
	d->src_dst_cidx = dma_read(EDMA_PARAM_SRC_DST_CIDX(param_id));
	d->ccnt = dma_read(EDMA_PARAM_CCNT(param_id));
	ret_code = 0;
exit:
	pr_debug("[%s]: end\n", __func__);
	return ret_code;
}
EXPORT_SYMBOL(davinci_get_dma_params);

/*
 * davinci_start_dma - starts the DMA on the channel passed
 * Arguments:
 *	lch - logical channel number
 */
int davinci_start_dma(int lch)
{
	int ret_code = 0, mask;

	pr_debug("[%s]: start\n", __func__);

	if (lch >= 0 && lch < davinci_edma_num_dmach) {
		/* DMA Channel */
		if (edmach_has_event(lch)) {

			pr_debug("ER = %x\n", dma_read(EDMA_SH_ER(0)));

			if (lch < 32) {
				mask = 1 << lch;
				/* Clear any pedning error */
				dma_write(mask, EDMA_EMCR);
				/* Clear any SER */
				dma_write(mask, EDMA_SH_SECR(0));
				dma_write(mask, EDMA_SH_EESR(0));
				dma_write(mask, EDMA_SH_ECR(0));
			} else {
				mask = 1 << (lch - 32);
				/* Clear any pedning error */
				dma_write(mask, EDMA_EMCRH);
				/* Clear any SER */
				dma_write(mask, EDMA_SH_SECRH(0));
				dma_write(mask, EDMA_SH_EESRH(0));
				dma_write(mask, EDMA_SH_ECRH(0));
			}
			pr_debug("EER = %x\n", dma_read(EDMA_SH_EER(0)));
		} else {
			pr_debug("ESR = %x\n", dma_read(EDMA_SH_ESR(0)));

			if (lch < 32)
				dma_write(1 << lch, EDMA_SH_ESR(0));
			else
				dma_write(1 << (lch - 32), EDMA_SH_ESRH(0));
		}
	} else if (dma_is_qdmach(lch))
		/* QDMA Channel */
		dma_write(1 << (lch - EDMA_QDMA_CHANNEL_0), EDMA_SH_QEESR(0));
	else
		ret_code = EINVAL;

	pr_debug("[%s]: end\n", __func__);
	return ret_code;
}
EXPORT_SYMBOL(davinci_start_dma);

/*
 * davinci_stop_dma - stops the DMA on the channel passed
 * Arguments:
 *	lch - logical channel number
 */
int davinci_stop_dma(int lch)
{
	u32 reg, mask;
	int ret_code = 0;

	pr_debug("[%s]: start\n", __func__);

	if (lch >= 0 && lch < davinci_edma_num_dmach) {
		/* DMA Channel */
		if (lch < 32) {
			mask = 1 << lch;
			if (edmach_has_event(lch)) {
				reg = EDMA_SH_EECR(0);
				dma_write(mask, reg);
				CLEAR_EVENT(mask, EDMA_SH_ER(0),
					    EDMA_SH_ECR(0));
			}
			CLEAR_EVENT(mask, EDMA_SH_SER(0), EDMA_SH_SECR(0));
			CLEAR_EVENT(mask, EDMA_EMR, EDMA_EMCR);
		} else {
			mask = 1 << (lch - 32);
			if (edmach_has_event(lch)) {
				reg = EDMA_SH_EECRH(0);
				dma_write(mask, reg);
				CLEAR_EVENT(mask, EDMA_SH_ERH(0),
					    EDMA_SH_ECRH(0));
			}
			CLEAR_EVENT(mask, EDMA_SH_SERH(0), EDMA_SH_SECRH(0));
			CLEAR_EVENT(mask, EDMA_EMRH, EDMA_EMCRH);
		}
		pr_debug("EER = %x\n", dma_read(EDMA_SH_EER(0)));
	} else if (dma_is_qdmach(lch)) {
		/* QDMA Channel */
		dma_write(1 << (lch - EDMA_QDMA_CHANNEL_0), EDMA_QEECR);
		pr_debug("QER = %x\n", dma_read(EDMA_QER));
		pr_debug("QEER = %x\n", dma_read(EDMA_QEER));
	} else
		ret_code = -1;

	pr_debug("[%s]: end\n", __func__);
	return ret_code;
}
EXPORT_SYMBOL(davinci_stop_dma);

/*
 * davinci_dma_link_lch - link two logical channels passed through by linking
 *			  the link field of head to the param pointed by the
 * 			  lch_queue.
 * Arguments:
 *	lch_head  - logical channel number in which the link field is linked
 *		    to the PaRAM pointed to by lch_queue
 *	lch_queue - logical channel number or the PaRAM entry number which is
 *		    to be linked to the lch_head
 */
int davinci_dma_link_lch(int lch_head, int lch_queue)
{
	u16 link;
	u32 reg;
	int ret_code = 0;

	pr_debug("[%s]: start\n", __func__);

	if ((lch_head  >= 0 && lch_head  < edma_max_logical_ch) ||
	    (lch_queue >= 0 && lch_queue < edma_max_logical_ch)) {
		unsigned int param1_id = dma_ch_bound_res[lch_head].param_id;
		unsigned int param2_id = dma_ch_bound_res[lch_queue].param_id;

		/* program LINK */
		link = (u16)IO_ADDRESS(EDMA_PARAM_OPT(param2_id));

		reg = EDMA_PARAM_LINK_BCNTRLD(param1_id);
		CLEAR_REG_VAL(0xffff, reg);
		SET_REG_VAL(link, reg);
	} else
		ret_code = -1;

	pr_debug("[%s]: end\n", __func__);
	return ret_code;
}
EXPORT_SYMBOL(davinci_dma_link_lch);

/*
 * davinci_dma_unlink_lch - unlink the two logical channels passed through by
 *			    setting the link field of head to 0xffff.
 * Arguments:
 *	lch_head  - logical channel number from which the link field is
 *		    to be removed
 *	lch_queue - logical channel number or the PaRAM entry number,
 *	             which is to be unlinked from lch_head
 */
int davinci_dma_unlink_lch(int lch_head, int lch_queue)
{
	u32 reg;
	unsigned int param_id = 0;
	int ret_code = 0;

	pr_debug("[%s]: start\n", __func__);

	if ((lch_head  >= 0 && lch_head  < edma_max_logical_ch) ||
	    (lch_queue >= 0 && lch_queue < edma_max_logical_ch)) {
		param_id = dma_ch_bound_res[lch_head].param_id;
		reg = EDMA_PARAM_LINK_BCNTRLD(param_id);
		SET_REG_VAL(0xffff, reg);
	} else
		ret_code = -1;

	pr_debug("[%s]: end\n", __func__);
	return ret_code;
}
EXPORT_SYMBOL(davinci_dma_unlink_lch);

/*
 * davinci_dma_chain_lch - chains two logical channels passed through
 * Arguments:
 * lch_head  - logical channel number which will trigger the chained channel
 *	       'lch_queue'
 * lch_queue - logical channel number which will be triggered by 'lch_head'
 */
int davinci_dma_chain_lch(int lch_head, int lch_queue)
{
	int ret_code = 0;

	pr_debug("[%s]: start\n", __func__);

	if ((lch_head  >= 0 && lch_head  < edma_max_logical_ch) ||
	    (lch_queue >= 0 && lch_queue < edma_max_logical_ch)) {
		unsigned int param_id = dma_ch_bound_res[lch_head].param_id;

		/* set TCCHEN */
		SET_REG_VAL(TCCHEN, EDMA_PARAM_OPT(param_id));

		/* Program TCC */
		CLEAR_REG_VAL(TCC, EDMA_PARAM_OPT(param_id));
		SET_REG_VAL((lch_queue & 0x3f) << 12, EDMA_PARAM_OPT(param_id));
	} else
		ret_code = -1;

	pr_debug("[%s]: end\n", __func__);
	return ret_code;
}
EXPORT_SYMBOL(davinci_dma_chain_lch);

/*
 * davinci_dma_unchain_lch - unchain the logical channels passed through
 * Arguments:
 *	lch_head  - logical channel number from which the link field is to be
 *		    removed
 *	lch_queue - logical channel number or the PaRAM entry number which is
 *		    to be unlinked from 'lch_head'
 */
int davinci_dma_unchain_lch(int lch_head, int lch_queue)
{
	int ret_code = 0;

	pr_debug("[%s]: start\n", __func__);

	if ((lch_head  >= 0 && lch_head  < edma_max_logical_ch) ||
	    (lch_queue >= 0 && lch_queue < edma_max_logical_ch)) {
		unsigned int param_id = dma_ch_bound_res[lch_head].param_id;

		/* reset TCCHEN */
		SET_REG_VAL(~TCCHEN, EDMA_PARAM_OPT(param_id));
		/* reset ITCCHEN */
		SET_REG_VAL(~ITCCHEN, EDMA_PARAM_OPT(param_id));
	} else
		ret_code = -1;

	pr_debug("[%s]: end\n", __func__);
	return ret_code;
}
EXPORT_SYMBOL(davinci_dma_unchain_lch);

/*
 * davinci_clean_channel - clean PaRAM entry and bring back EDMA to initial
 *			   state if media has been removed before EDMA has
 *			   finished.  It is useful for removable media.
 * Arguments:
 *	lch - logical channel number
 */
void davinci_clean_channel(int lch)
{
	u32 mask, value = 0, count;

	pr_debug("[%s]: start\n", __func__);

	if (lch < 0 || lch >= davinci_edma_num_dmach)
		return;
	if (lch < 32) {
		pr_debug("EMR = %x\n", dma_read(EDMA_EMR));
		mask = 1 << lch;
		dma_write(mask, EDMA_SH_ECR(0));
		/* Clear the corresponding EMR bits */
		dma_write(mask, EDMA_EMCR);
		/* Clear any SER */
		dma_write(mask, EDMA_SH_SECR(0));
		/* Clear any EER */
		dma_write(mask, EDMA_SH_EECR(0));

	} else {
		pr_debug("EMRH = %x\n", dma_read(EDMA_EMRH));
		mask = 1 << (lch - 32);
		dma_write(mask, EDMA_SH_ECRH(0));
		/* Clear the corresponding EMRH bits */
		dma_write(mask, EDMA_EMCRH);
		/* Clear any SER */
		dma_write(mask, EDMA_SH_SECRH(0));
		/* Clear any EERH */
		dma_write(mask, EDMA_SH_EECRH(0));
	}

	for (count = 0; count < davinci_edma_num_evtq; count++)
		value |= 1 << count;

	dma_write((1 << 16) | value, EDMA_CCERRCLR);

	pr_debug("[%s]: end\n", __func__);
}
EXPORT_SYMBOL(davinci_clean_channel);

/*
 * davinci_dma_getposition - returns the current transfer points for the DMA
 *			     source and destination
 * Arguments:
 *	lch - logical channel number
 *	src - source port position
 *	dst - destination port position
 */
void davinci_dma_getposition(int lch, dma_addr_t *src, dma_addr_t *dst)
{
	struct paramentry_descriptor temp;

	davinci_get_dma_params(lch, &temp);
	if (src != NULL)
		*src = temp.src;
	if (dst != NULL)
		*dst = temp.dst;
}
EXPORT_SYMBOL(davinci_dma_getposition);

/*
 * davinci_dma_clear_event - clear an outstanding event on the DMA channel
 * Arguments:
 *     lch - logical channel number
 */
void davinci_dma_clear_event(int lch)
{
	if (lch < 0 || lch >= davinci_edma_num_dmach)
		return;
	if (lch < 32)
		dma_write(1 << lch, EDMA_ECR);
	else
		dma_write(1 << (lch - 32), EDMA_ECRH);
}
EXPORT_SYMBOL(davinci_dma_clear_event);

/*
 * Register different ISRs with the underlying OS.
 */
static int register_dma_interrupts(void)
{
	int result, i;

	result = request_irq(cc_reg0_int, dma_irq_handler, 0,
			     "EDMA Completion", NULL);
	if (result < 0) {
		pr_debug("request_irq failed for dma_irq_handler, error = %d\n",
			 result);
		return result;
	}

	result = request_irq(cc_error_int, dma_ccerr_handler, 0,
			     "EDMA CC Error", NULL);
	if (result < 0) {
		pr_debug("request_irq failed for dma_ccerr_handler, "
			 "error = %d\n", result);
		return result;
	}

	for (i = 0; i < davinci_edma_num_tc; i++) {
		snprintf(tc_error_int_name[i], 19, "EDMA TC%d Error", i);
		result = request_irq(tc_error_int[i], ptr_edmatc_isrs[i], 0,
				     tc_error_int_name[i], NULL);
		if (result < 0) {
			pr_debug("request_irq failed for dma_tc%d_err_handler, "
				 "error = %d\n", i, result);
			return result;
		}
	}
	return result;
}

/*
 * EDMA3 Initialisation on DaVinci
 */
int __init davinci_dma_init(struct dma_init_info *info)
{
#ifndef CONFIG_SKIP_EDMA3_REGS_INIT
	struct edma_map *q_pri	= info->q_pri;
	struct edma_map *q_wm	= info->q_wm;
	struct edma_map *q_tc	= info->q_tc;
	unsigned int i;
#endif

	davinci_edma_num_dmach = info->edma_num_dmach;
	davinci_edma_num_tcc = info->edma_num_tcc;
	davinci_edma_num_evtq = info->edma_num_evtq;
	davinci_edma_num_tc = info->edma_num_tc;
	davinci_edmatc_base_addrs = info->edmatc_base_addrs;
	davinci_qdma_ch_map = davinci_edma_num_param = info->edma_num_param;
	edma_max_logical_ch = davinci_qdma_ch_map + EDMA_NUM_QDMACH;

	davinci_edma_chmap_exist = info->edma_chmap_exist;

	edma2event_map = info->edma2event_map;

	edma_channels_arm = info->edma_channels_arm;
	qdma_channels_arm = info->qdma_channels_arm;
	param_entry_arm = info->param_entry_arm;
	tcc_arm = info->tcc_arm;
	param_entry_reserved = info->param_entry_reserved;

	cc_reg0_int  = info->cc_reg0_int;
	cc_error_int = info->cc_error_int;
	tc_error_int = info->tc_error_int;

	dma_ch_bound_res = kmalloc(sizeof(struct edma3_ch_bound_res) *
				   edma_max_logical_ch, GFP_KERNEL);

	/* Reset the DCHMAP registers if they exist */
	if (davinci_edma_chmap_exist == 1)
		memset((void *)IO_ADDRESS(EDMA_DCHMAP(0)), 0, EDMA_DCHMAP_SIZE);

	/* Reset book-keeping info */
	memset(dma_ch_bound_res, 0, (sizeof(struct edma3_ch_bound_res) *
				     edma_max_logical_ch));
	memset(intr_data, 0, sizeof(intr_data));
	memset(edma_dma_ch_tcc_mapping,  0, sizeof(edma_dma_ch_tcc_mapping));
	memset(edma_qdma_ch_tcc_mapping, 0, sizeof(edma_qdma_ch_tcc_mapping));

#ifndef CONFIG_SKIP_EDMA3_REGS_INIT
	memset((void *)IO_ADDRESS(EDMA_PARAM_OPT(0)), 0, EDMA_PARAM_SIZE);

	/* Clear error registers */
	dma_write(~0, EDMA_EMCR);
	dma_write(~0, EDMA_EMCRH);
	dma_write(~0, EDMA_QEMCR);
	dma_write(~0, EDMA_CCERRCLR);

	for (i = 0; i < davinci_edma_num_evtq; i++) {
		u32 mask;

#if EDMA_EVENT_QUEUE_TC_MAPPING == 1
		/* Event queue to TC mapping, if it exists */
		mask = QUETCMAP_CLR_MASK(q_tc[i].param1);
		CLEAR_REG_VAL(mask, EDMA_QUETCMAP);
		mask = QUETCMAP_SET_MASK(q_tc[i].param1, q_tc[i].param2);
		SET_REG_VAL(mask, EDMA_QUETCMAP);
#endif

		/* Event queue priority */
		mask = QUEPRI_CLR_MASK(q_pri[i].param1);
		CLEAR_REG_VAL(mask, EDMA_QUEPRI);
		mask = QUEPRI_SET_MASK(q_pri[i].param1, q_pri[i].param2);
		SET_REG_VAL(mask, EDMA_QUEPRI);

		/* Event queue watermark level */
		mask = QUEWMTHR_CLR_MASK(q_wm[i].param1);
		CLEAR_REG_VAL(mask, EDMA_QWMTHRA);
		mask = QUEWMTHR_SET_MASK(q_wm[i].param1, q_wm[i].param2);
		SET_REG_VAL(mask, EDMA_QWMTHRA);
	}
#endif	/* CONFIG_SKIP_EDMA3_REGS_INIT */

	/* Clear region specific shadow registers */
	dma_write(edma_channels_arm[0] | tcc_arm[0],
		  EDMA_SH_ECR(EDMA_MASTER_SHADOW_REGION));
	dma_write(edma_channels_arm[1] | tcc_arm[1],
		  EDMA_SH_ECRH(EDMA_MASTER_SHADOW_REGION));
	dma_write(edma_channels_arm[0] | tcc_arm[0],
		  EDMA_SH_EECR(EDMA_MASTER_SHADOW_REGION));
	dma_write(edma_channels_arm[1] | tcc_arm[1],
		  EDMA_SH_EECRH(EDMA_MASTER_SHADOW_REGION));
	dma_write(edma_channels_arm[0] | tcc_arm[0],
		  EDMA_SH_SECR(EDMA_MASTER_SHADOW_REGION));
	dma_write(edma_channels_arm[1] | tcc_arm[1],
		  EDMA_SH_SECRH(EDMA_MASTER_SHADOW_REGION));
	dma_write(edma_channels_arm[0] | tcc_arm[0],
		  EDMA_SH_IECR(EDMA_MASTER_SHADOW_REGION));
	dma_write(edma_channels_arm[1] | tcc_arm[1],
		  EDMA_SH_IECRH(EDMA_MASTER_SHADOW_REGION));
	dma_write(edma_channels_arm[0] | tcc_arm[0],
		  EDMA_SH_ICR(EDMA_MASTER_SHADOW_REGION));
	dma_write(edma_channels_arm[1] | tcc_arm[1],
		  EDMA_SH_ICRH(EDMA_MASTER_SHADOW_REGION));
	dma_write(qdma_channels_arm[0],
		  EDMA_SH_QEECR(EDMA_MASTER_SHADOW_REGION));
	dma_write(qdma_channels_arm[0],
		  EDMA_SH_QSECR(EDMA_MASTER_SHADOW_REGION));

	/* Reset region access enable registers for the master shadow region */
	dma_write(0, EDMA_DRAE(EDMA_MASTER_SHADOW_REGION));
	dma_write(0, EDMA_DRAEH(EDMA_MASTER_SHADOW_REGION));
	dma_write(0, EDMA_QRAE(EDMA_MASTER_SHADOW_REGION));

	if (register_dma_interrupts())
		return -EINVAL;

	spin_lock_init(&dma_chan_lock);

	return 0;
}

MODULE_AUTHOR("Texas Instruments");
MODULE_LICENSE("GPL");
