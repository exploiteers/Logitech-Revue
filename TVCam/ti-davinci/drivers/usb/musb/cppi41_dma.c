/*
 * Copyright (C) 2005-2006 by Texas Instruments
 * Copyright (c) 2008, MontaVista Software, Inc. <source@mvista.com>
 *
 * This file implements a DMA interface using TI's CPPI 4.1 DMA.
 *
 * This program is free software; you can distribute it and/or modify it
 * under the terms of the GNU General Public License (Version 2) as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston MA 02111-1307, USA.
 *
 */

#include <linux/errno.h>
#include <linux/dma-mapping.h>

#include <asm/arch/cppi41.h>

#include "musb_core.h"
#include "musb_dma.h"
#include "cppi41_dma.h"

/* Configuration */
#define USB_CPPI41_DESC_SIZE_SHIFT 6
#define USB_CPPI41_DESC_ALIGN	(1 << USB_CPPI41_DESC_SIZE_SHIFT)
#define USB_CPPI41_CH_NUM_PD	64
#define USB_CPPI41_MAX_BD	(USB_CPPI41_CH_NUM_PD * USB_CPPI41_NUM_CH * 2)

#define USB_DEFAULT_TX_MODE 	USB_GENERIC_RNDIS_MODE
#define USB_DEFAULT_RX_MODE 	USB_GENERIC_RNDIS_MODE

#undef	USB_CPPI41_DEBUG
#define USB_CPPI41_DEBUG_LEVEL	4

#undef DEBUG_CPPI_TD
#undef USBDRV_DEBUG

#ifdef USBDRV_DEBUG
#define dprintk(x,...) printk(x, ## __VA_ARGS__)
#else
#define dprintk(x,...)
#endif

/*
 * Data structure definitions
 */

/*
 * USB Packet Descriptor
 */
struct usb_pkt_desc;

struct usb_pkt_desc {
	/* Hardware descriptor fields from this point */
	struct cppi41_host_pkt_desc hw_desc;
	/* Protocol specific data */
	struct usb_pkt_desc *next_pd_ptr;
	u8 ch_num;
	u8 ep_num;
};

/**
 * struct usb_cppi_dma_ch - DMA Channel Control structure
 *
 * Used to process Tx/Rx packet descriptors.
 */
struct usb_cppi_dma_ch {
	void *pdMem;			/* Tx PD memory pointer */
	struct usb_pkt_desc *pdPoolHead; /* Free PD pool head */
	struct cppi41_dma_ch_obj dmaChObj; /* DMA channel object */
	struct cppi41_queue_obj queueObj; /* Tx queue object or Rx free */
					/* descriptor/buffer queue object */
};

/**
 * struct cppi41_channel - Channel Control Structure
 *
 * Using the same for Tx/Rx.
 */
struct cppi41_channel {
	struct dma_channel channel;

	struct usb_cppi_dma_ch *cppi_dma;
	volatile int teardown_pending;

	/* Which direction of which endpoint? */
	struct musb_hw_ep *end_pt;
	u8 transmit;
	u8 ch_num;			/* Channel number of Tx/Rx 0..3 */

	/* DMA mode: "transparent", RNDIS, CDC, or Generic RNDIS */
	u8 dma_mode;
	u8 autoreq;

	/* Book keeping for the current transfer request */
	dma_addr_t start_addr;
	u32 length;
	u32 curr_offset;
	u16 pkt_size;
	u8  transfer_mode;
	u8  zlp_queued;
};

struct pd_mem_chunk {
	void *base;
	u32 num_pds;
	int allocated;
};

/**
 * struct cppi41 - CPPI 4.1 DMA Controller Object
 *
 * Encapsulates all book keeping and data structures pertaining to
 * the CPPI 1.4 DMA controller.
 */
struct cppi41 {
	struct dma_controller controller;
	struct musb *musb;

	struct cppi41_channel txCppiCh[USB_CPPI41_NUM_CH];
	struct cppi41_channel rxCppiCh[USB_CPPI41_NUM_CH];

	struct pd_mem_chunk pdPool[USB_CPPI41_NUM_CH * 2];
	dma_addr_t pdMemPhys;		/* PD memory physical address */
	char *pdMem;			/* PD memory pointer */
	u8 pdMemRgn;			/* PD memory region number */
};

static void *alloc_pkt_desc(struct cppi41 *cppi)
{
	struct pd_mem_chunk *pd_pool = cppi->pdPool;
	int i;

	for (i = 0; i < USB_CPPI41_NUM_CH * 2; ++i)
		if (!pd_pool[i].allocated) {
			pd_pool[i].allocated = 1;
			return pd_pool[i].base;
		}

	return NULL;
}

static int free_pkt_desc(struct cppi41 *cppi, void *free_pd)
{
	struct pd_mem_chunk *pd_pool = cppi->pdPool;
	int i;

	for (i = 0; i < MUSB_C_NUM_EPS - 1; ++i)
		if (pd_pool[i].allocated &&
		    pd_pool[i].base == free_pd) {
			pd_pool[i].allocated = 0;
			return 0;
		}

	return 1;
}

#ifdef DEBUG_CPPI_TD
static void print_pd_list(struct usb_pkt_desc *pd_pool_head)
{
	struct usb_pkt_desc *curr_pd = pd_pool_head;
	int cnt = 0;

	while (curr_pd != NULL) {
		if (cnt % 8 == 0)
			dprintk("\n%02x ", cnt);
		cnt++;
		dprintk(" %p", curr_pd);
		curr_pd = (struct usb_pkt_desc *)curr_pd->next_pd_ptr;
	}
	dprintk("\n");
}
#endif

static struct usb_pkt_desc *usb_get_free_pd(struct usb_cppi_dma_ch *cppi_dma)
{
	struct usb_pkt_desc *curr_pd = cppi_dma->pdPoolHead;

	if (curr_pd != NULL) {
		cppi_dma->pdPoolHead = curr_pd->next_pd_ptr;
		curr_pd->next_pd_ptr = NULL;
	}
	return curr_pd;
}

static void usb_put_free_pd(struct usb_cppi_dma_ch *cppi_dma,
			    struct usb_pkt_desc *free_pd)
{
	free_pd->next_pd_ptr = cppi_dma->pdPoolHead;
	cppi_dma->pdPoolHead = free_pd;
}

/**
 * usb_init_cppi_ch - allocates memory for the DMA channel control structure
 *		      and packet descriptors
 */
static int usb_init_cppi_ch(struct cppi41 *cppi, struct cppi41_channel *cppi_ch)
{
	struct usb_cppi_dma_ch *cppi_dma;
	int ret_code = -ENOMEM;

	DBG(USB_CPPI41_DEBUG_LEVEL, "ENTER: channel %d\n",
	    cppi_ch->ch_num);

	/* Allocate memory for the usb_cppi_dma_ch structure */
	cppi_dma = kzalloc(sizeof(struct usb_cppi_dma_ch), GFP_KERNEL);
	if (cppi_dma == NULL) {
		DBG(USB_CPPI41_DEBUG_LEVEL,
		    "ERROR: failed to allocate memory for USB CPPI "
		    "channel %d\n", cppi_ch->ch_num);
		goto exit;
	}
	cppi_ch->cppi_dma = cppi_dma;
	cppi_ch->channel.private_data = cppi;

	cppi_dma->pdMem = alloc_pkt_desc(cppi);
	if (cppi_dma->pdMem == NULL) {
		/* Descriptor region is not available */
		DBG(1, "ERROR: failed to allocate descriptors\n");

		/* Free Tx CPPI channel memory */
		kfree(cppi_dma);
		goto exit;
	}

	ret_code = 0;
 exit:
	DBG(USB_CPPI41_DEBUG_LEVEL, "EXIT: channel %d\n", cppi_ch->ch_num);

	return ret_code;
}

/**
 * usb_init_tx_ch
 *	- allocates memory for the DMA channel control structure and packet
 *	  descriptors;
 *	- initializes each descriptor and chain the Tx BD list ready to be
 *	  given to hardware.
 *
 * NOTE:
 *	1. This function assumes that the channel number passed is valid and
 *	   the ->cppi_dma pointer is NULL.
 *	2. This function will not do any error checking on these initialization
 *	   parameters to avoid duplicate error checks (done in caller function
 *	   or init parse functions).  Also, num_pds value and the alignment
 *	   requirement should be validated as per the specs during init time.
 */
static int usb_init_tx_ch(struct cppi41 *cppi, struct cppi41_channel *tx_ch)
{
	struct usb_pkt_desc *curr_pd;
	struct usb_cppi_dma_ch *cppi_dma;
	const struct cppi41_tx_ch *tx_info;
	u32 tag_info, pkt_info;
	int ret_code, i;

	DBG(USB_CPPI41_DEBUG_LEVEL, "ENTER: channel %d\n", tx_ch->ch_num);

	ret_code = usb_init_cppi_ch(cppi, tx_ch);
	if (ret_code)
		goto exit;

	tx_info = cppi41_dma_block[usb_cppi41_info.dma_block].txChInfo +
		  usb_cppi41_info.ep_dma_ch[tx_ch->ch_num];
	tag_info = (tx_info->portNum << CPPI41_SRC_TAG_PORT_NUM_SHIFT) |
		   (tx_info->chNum << CPPI41_SRC_TAG_CH_NUM_SHIFT) |
		   (tx_info->subChNum << CPPI41_SRC_TAG_SUB_CH_NUM_SHIFT);
	pkt_info = (CPPI41_PKT_TYPE_USB << CPPI41_PKT_TYPE_SHIFT) |
		   (CPPI41_RETURN_LINKED << CPPI41_RETURN_POLICY_SHIFT) |
		   (usb_cppi41_info.q_mgr << CPPI41_RETURN_QMGR_SHIFT) |
		   (usb_cppi41_info.tx_comp_q[0] << CPPI41_RETURN_QNUM_SHIFT);

	/*
	 * "Slice" PDs one-by-one from the chunk,
	 * allocate a buffer and token
	 */
	cppi_dma = tx_ch->cppi_dma;
	curr_pd = (struct usb_pkt_desc *)cppi_dma->pdMem;
	for (i = 0; i < USB_CPPI41_CH_NUM_PD; i++) {
		struct cppi41_host_pkt_desc *hw_desc = &curr_pd->hw_desc;

		hw_desc->desc_info = CPPI41_DESC_TYPE_HOST <<
				     CPPI41_DESC_TYPE_SHIFT;
		hw_desc->tag_info = tag_info;
		hw_desc->pkt_info = (CPPI41_PKT_TYPE_USB <<
				     CPPI41_PKT_TYPE_SHIFT) |
				    (CPPI41_RETURN_LINKED <<
				     CPPI41_RETURN_POLICY_SHIFT) |
				    (usb_cppi41_info.q_mgr <<
				     CPPI41_RETURN_QMGR_SHIFT) |
				    (usb_cppi41_info.tx_comp_q[0] <<
				     CPPI41_RETURN_QNUM_SHIFT);
		hw_desc->next_desc_ptr = 0;
		curr_pd->ch_num = tx_ch->ch_num;

		usb_put_free_pd(cppi_dma, curr_pd);
		curr_pd = (struct usb_pkt_desc *)((char *)curr_pd +
						  USB_CPPI41_DESC_ALIGN);
	}

 exit:
	DBG(USB_CPPI41_DEBUG_LEVEL, "EXIT: channel %d\n", tx_ch->ch_num);

	return ret_code;
}

/**
 * usb_init_rx_ch
 *	- allocates memory for the DMA channel control structure and packet
 *	  descriptors;
 *	- initializes each descriptor and chain the Rx BD list ready to be
 *	  given to hardware.
 *
 * NOTE:
 *	1. This function assumes that the channel number passed is valid and
 *	   the ->cppi_dma pointer is NULL.
 *	2. This function will not do any error checking on these initialization
 *	   parameters to avoid duplicate error checks (done in caller function
 *	   or init parse functions).  Also, num_pds value and the alignment
 *	   requirement should be validated as per the specs during init time.
 */
static int usb_init_rx_ch(struct cppi41 *cppi, struct cppi41_channel *rx_ch)
{
	struct usb_cppi_dma_ch *cppi_dma = NULL;
	struct usb_pkt_desc *curr_pd;
	int ret_code, i;

	DBG(USB_CPPI41_DEBUG_LEVEL, "ENTER: channel %d\n", rx_ch->ch_num);

	ret_code = usb_init_cppi_ch(cppi, rx_ch);
	if (ret_code)
		goto exit;
	/*
	 * "Slice" PDs one-by-one from the chunk,
	 * allocate a buffer and token.
	 */
	cppi_dma = rx_ch->cppi_dma;
	curr_pd = (struct usb_pkt_desc *)cppi_dma->pdMem;
	for (i = 0; i < USB_CPPI41_CH_NUM_PD; i++) {
#if 0
		struct cppi41_host_pkt_desc *hw_desc = &curr_pd->hw_desc;

		/* Update the hardware descriptor */
		hw_desc->pkt_info = (CPPI41_PKT_TYPE_USB <<
				     CPPI41_PKT_TYPE_SHIFT) |
				    (CPPI41_RETURN_LINKED <<
				     CPPI41_RETURN_POLICY_SHIFT) |
				    (usb_cppi41_info.q_mgr <<
				     CPPI41_RETURN_QMGR_SHIFT) |
				    (usb_cppi41_info.rx_comp_q[0] <<
				     CPPI41_RETURN_QNUM_SHIFT);
		hw_desc->next_desc_ptr = 0;
#endif

		curr_pd->ch_num = rx_ch->ch_num;

		usb_put_free_pd(cppi_dma, curr_pd);
		curr_pd = (struct usb_pkt_desc *)((char *)curr_pd +
						  USB_CPPI41_DESC_ALIGN);
	}

 exit:
	DBG(USB_CPPI41_DEBUG_LEVEL, "EXIT: channel %d\n", rx_ch->ch_num);

	return ret_code;
}

/**
 * usb_deinit_dma_ch - frees memory previously allocated for the DMA channel
 *		       control structure and buffer descriptors
 *
 * NOTE: This function assumes that the channel number passed is valid and
 *	 this function will not do any error checking to avoid duplicate
 *	 error checks (done in caller function).
 */
static int usb_deinit_dma_ch(struct cppi41 *cppi,
			     struct cppi41_channel *cppi_ch)
{
	struct usb_cppi_dma_ch *cppi_dma;
	int ret_code;

	DBG(USB_CPPI41_DEBUG_LEVEL, "ENTER: channel %d\n", cppi_ch->ch_num);

	/* Check if channel structure is already de-allocated */
	cppi_dma = cppi_ch->cppi_dma;
	if (cppi_dma == NULL) {
		DBG(1, "ERROR: CPPI channel %d structure already freed\n",
		    cppi_ch->ch_num);
		return -EINVAL;
	}

	ret_code = free_pkt_desc(cppi, cppi_dma->pdMem);
	cppi_dma->pdMem = NULL;
	if (ret_code)
		DBG(USB_CPPI41_DEBUG_LEVEL,
		    "ERROR: failed to free the descriptor region for "
		    "channel %d\n", cppi_ch->ch_num);

	/* Free the Tx channel structure */
	kfree(cppi_dma);
	cppi_ch->cppi_dma = NULL;

	DBG(USB_CPPI41_DEBUG_LEVEL, "EXIT: channel %d\n", cppi_ch->ch_num);

	return 0;
}

/**
 * cppi41_controller_start - start DMA controller
 * @controller: the controller
 *
 * This function initializes the CPPI 4.1 Tx/Rx channels.
 */
static int __init cppi41_controller_start(struct dma_controller *controller)
{
	struct cppi41 *cppi;
	struct cppi41_channel *cppi_ch;
	void __iomem *reg_base;
	struct usb_pkt_desc *curr_pd;
	struct pd_mem_chunk *pd_pool;
	int i;

	cppi = container_of(controller, struct cppi41, controller);

	/*
	 * TODO: We may need to check num_pds here since CPPI 4.1 requires the
	 * descriptor count to be a multiple of 2 ^ 5 (i.e. 32). Similarly, the
	 * descriptor size should also be a multiple of 32.
	 */

	/*
	 * Allocate free packet descriptor pool for all Tx/Rx endpoints --
	 * dma_alloc_coherent()  will return a page aligned address, so our
	 * alignment requirement will be honored.
	 */
	cppi->pdMem = dma_alloc_coherent(cppi->musb->controller,
					 USB_CPPI41_MAX_BD *
					 USB_CPPI41_DESC_ALIGN,
					 &cppi->pdMemPhys,
					 GFP_KERNEL | GFP_DMA);
	if (cppi->pdMem == NULL) {
		DBG(USB_CPPI41_DEBUG_LEVEL, "ERROR: PD allocation failed\n");
		return 0;
	}
	if (cppi41_alloc_mem_rgn(usb_cppi41_info.q_mgr, cppi->pdMemPhys,
				 USB_CPPI41_DESC_SIZE_SHIFT,
				 get_count_order(USB_CPPI41_MAX_BD),
				 &cppi->pdMemRgn)) {
		DBG(USB_CPPI41_DEBUG_LEVEL, "ERROR: queue manager memory "
		    "region allocation failed\n");
		return 0;
	}

	/* Initialize the packet pool memory */
	pd_pool = cppi->pdPool;
	curr_pd = (struct usb_pkt_desc *)cppi->pdMem;
	for (i = 0; i < ARRAY_SIZE(cppi->pdPool); ++i) {
		pd_pool[i].base = curr_pd;
		pd_pool[i].num_pds = USB_CPPI41_CH_NUM_PD;
		pd_pool[i].allocated = 0;
		curr_pd += pd_pool[i].num_pds;
	}

	/* Configure the Tx channels */
	for (i = 0, cppi_ch = cppi->txCppiCh; i < ARRAY_SIZE(cppi->txCppiCh);
	     ++i, ++cppi_ch) {
		memzero(cppi_ch, sizeof(struct cppi41_channel));
		cppi_ch->transmit = 1;
		cppi_ch->ch_num = i;

		if (usb_init_tx_ch(cppi, cppi_ch))
			DBG(USB_CPPI41_DEBUG_LEVEL, "usb_init_tx_ch failed for "
			    "Tx channel %d\n", i);
	}

	/* Configure the Rx channels */
	for (i = 0, cppi_ch = cppi->rxCppiCh; i < ARRAY_SIZE(cppi->rxCppiCh);
	     ++i, ++cppi_ch) {
		memzero(cppi_ch, sizeof(struct cppi41_channel));
		cppi_ch->ch_num = i;

		if (usb_init_rx_ch(cppi, cppi_ch))
			DBG(USB_CPPI41_DEBUG_LEVEL, "usb_init_rx_ch failed for "
			    "Rx channel %d\n", i);
	}

	/* Do necessary configuartion in hardware to get started */
	reg_base = cppi->musb->ctrl_base;

	/* Disable auto request mode */
	musb_writel(reg_base, USB_AUTOREQ_REG, 0);

	/* Disable the CDC/RNDIS modes */
	musb_writel(reg_base, USB_MODE_REG, 0);

	return 1;
}

/**
 * cppi41_controller_stop - stop DMA controller
 * @controller: the controller
 *
 * De-initialize the DMA Controller as necessary.
 */
static int cppi41_controller_stop(struct dma_controller *controller)
{
	struct cppi41 *cppi;
	void __iomem *reg_base;
	int i;

	cppi = container_of(controller, struct cppi41, controller);

	DBG(USB_CPPI41_DEBUG_LEVEL, "De-initializing Rx and Tx channels\n");

	for (i = 0; i < ARRAY_SIZE(cppi->txCppiCh); i++)
		usb_deinit_dma_ch(cppi, &cppi->txCppiCh[i]);

	for (i = 0; i < ARRAY_SIZE(cppi->rxCppiCh); i++)
		usb_deinit_dma_ch(cppi, &cppi->rxCppiCh[i]);

	/* Free the host descriptor allocated for all Tx/Rx channels */
	if (cppi41_free_mem_rgn(usb_cppi41_info.q_mgr, cppi->pdMemRgn))
		DBG(USB_CPPI41_DEBUG_LEVEL, "ERROR: failed to free CPPI "
		    "packet descriptors\n");

	dma_free_coherent(cppi->musb->controller,
			  USB_CPPI41_MAX_BD * USB_CPPI41_DESC_ALIGN,
			  cppi->pdMem, cppi->pdMemPhys);

	reg_base = cppi->musb->ctrl_base;

	/* Disable auto request mode */
	musb_writel(reg_base, USB_AUTOREQ_REG, 0);

	/* Disable the CDC/RNDIS modes */
	musb_writel(reg_base, USB_MODE_REG, 0);

	return 1;
}

/**
 * cppi41_channel_alloc - allocate a CPPI channel for DMA.
 * @controller: the controller
 * @ep: 	the endpoint
 * @is_tx:	1 for Tx channel, 0 for Rx channel
 *
 * With CPPI, channels are bound to each transfer direction of a non-control
 * endpoint, so allocating (and deallocating) is mostly a way to notice bad
 * housekeeping on the software side.  We assume the IRQs are always active.
 */
static struct dma_channel *cppi41_channel_alloc(struct dma_controller
						*controller,
						struct musb_hw_ep *ep, u8 is_tx)
{
	struct cppi41 *cppi;
	struct cppi41_channel  *cppi_ch;
	struct usb_cppi_dma_ch *cppi_dma;
	u32 ch_num, ep_num = ep->epnum;

	cppi = container_of(controller, struct cppi41, controller);

	/* Remember, ep_num: 1 .. Max_EP, and CPPI ch_num: 0 .. Max_EP - 1 */
	ch_num = ep_num - 1;

	if (ep_num > USB_CPPI41_NUM_CH) {
		DBG(1, "No %cx DMA channel for EP%d\n",
		    is_tx ? 'T' : 'R', ep_num);
		return NULL;
	}

	cppi_ch = (is_tx ? cppi->txCppiCh : cppi->rxCppiCh) + ch_num;
	cppi_dma = cppi_ch->cppi_dma;
	if (cppi_dma == NULL) {
		DBG(1, "CPPI %cx channel %d was not created\n",
		    is_tx ? 'T' : 'R', ch_num);
		return NULL;
	}

	/* As of now, just return the corresponding CPPI 4.1 channel handle */
	if (is_tx) {
		const struct cppi41_tx_ch *tx_info;

		tx_info = cppi41_dma_block[usb_cppi41_info.dma_block].txChInfo +
			  usb_cppi41_info.ep_dma_ch[ch_num];
		if (cppi41_queue_init(&cppi_dma->queueObj,
				      tx_info->txQueue->qMgr,
				      tx_info->txQueue->qNum)) {
			DBG(1, "ERROR: cppi41_queue_init failed for "
			    "Tx queue\n");
			return NULL;
		}

		/* Initialize the CPPI 4.1 DMA channel */
		if (cppi41_tx_ch_init(&cppi_dma->dmaChObj,
				      usb_cppi41_info.dma_block,
				      usb_cppi41_info.ep_dma_ch[ch_num])) {
			DBG(1, "ERROR: cppi41_tx_ch_init failed for "
			    "channel %d\n", ch_num);
			return NULL;
		}
		/*
		 * Teardown descriptor will be pushed to the same Tx completion
		 * queue as the other Tx descriptors.
		 */
		cppi41_tx_ch_configure(&cppi_dma->dmaChObj,
				       usb_cppi41_info.q_mgr,
				       usb_cppi41_info.tx_comp_q[0]);
	} else {
		struct cppi41_rx_ch_cfg rx_cfg;
		int i;

		if (cppi41_queue_init(&cppi_dma->queueObj,
				      usb_cppi41_info.q_mgr,
				      usb_cppi41_info.rx_fdb_q[0])) {
			DBG(1, "ERROR: cppi41_queue_init failed for "
			    "FDB queue\n");
			return NULL;
		}
		/* Initialize the CPPI 4.1 DMA channel */
		if (cppi41_rx_ch_init(&cppi_dma->dmaChObj,
				      usb_cppi41_info.dma_block,
				      usb_cppi41_info.ep_dma_ch[ch_num])) {
			DBG(1, "ERROR: cppi41_rx_ch_init failed\n");
			return NULL;
		}

		rx_cfg.defaultDescType = cppi41_rx_host_desc;
		rx_cfg.sopOffset = 0;
		rx_cfg.retryOnStarvation = 1;
		rx_cfg.rxQueue.qMgr = usb_cppi41_info.q_mgr;
		rx_cfg.rxQueue.qNum = usb_cppi41_info.rx_comp_q[0];
		for (i = 0; i < 4; i++) {
			rx_cfg.cfg.hostPkt.fdbQueue[i].qMgr =
				usb_cppi41_info.q_mgr;
			rx_cfg.cfg.hostPkt.fdbQueue[i].qNum =
				usb_cppi41_info.rx_fdb_q[0];
		}
		cppi41_rx_ch_configure(&cppi_dma->dmaChObj, &rx_cfg);
	}

	/* Enable the DMA channel */
	cppi41_dma_ch_enable(&cppi_dma->dmaChObj);

	if (cppi_ch->end_pt)
		DBG(1, "Re-allocating DMA %cx channel %d (%p)\n",
		    is_tx ? 'T' : 'R', ch_num, cppi_ch);

	cppi_ch->end_pt = ep;
	cppi_ch->ch_num = ch_num;
	cppi_ch->channel.status = MUSB_DMA_STATUS_FREE;

	DBG(USB_CPPI41_DEBUG_LEVEL, "Allocated DMA %cx channel %d for EP%d\n",
	    is_tx ? 'T' : 'R', ch_num, ep_num);

	return &cppi_ch->channel;
}

/**
 * cppi41_channel_release - release a CPPI DMA channel
 * @channel: the channel
 */
static void cppi41_channel_release(struct dma_channel *channel)
{
	struct cppi41_channel *cppi_ch;

	/* REVISIT: for paranoia, check state and abort if needed... */
	cppi_ch = container_of(channel, struct cppi41_channel, channel);
	if (cppi_ch->end_pt == NULL)
		DBG(1, "Releasing idle DMA channel %p\n", cppi_ch);

	/* But for now, not its IRQ */
	cppi_ch->end_pt = NULL;
	channel->status = MUSB_DMA_STATUS_UNKNOWN;

	cppi41_dma_ch_disable(&cppi_ch->cppi_dma->dmaChObj);
}

#if USB_DEFAULT_TX_MODE != USB_TRANSPARENT_MODE || \
    USB_DEFAULT_RX_MODE != USB_TRANSPARENT_MODE
static void cppi41_mode_update(struct cppi41_channel *cppi_ch, u8 mode)
{
	if (mode != cppi_ch->dma_mode) {
		struct cppi41 *cppi = cppi_ch->channel.private_data;
		void *__iomem reg_base = cppi->musb->ctrl_base;
		u32 reg_val = musb_readl(reg_base, USB_MODE_REG);
		u8 ep_num = cppi_ch->ch_num + 1;

		if (cppi_ch->transmit) {
			reg_val &= ~USB_TX_MODE_MASK(ep_num);
			reg_val |= mode << USB_TX_MODE_SHIFT(ep_num);
		} else {
			reg_val &= ~USB_RX_MODE_MASK(ep_num);
			reg_val |= mode << USB_RX_MODE_SHIFT(ep_num);
		}
		musb_writel(reg_base, USB_MODE_REG, reg_val);
		cppi_ch->dma_mode = mode;
	}
}
#else
static inline void cppi41_mode_update(struct cppi41_channel *, u8) {}
#endif

/*
 * CPPI 4.1 Tx:
 * ============
 * Tx is a lot more reasonable than Rx: RNDIS mode seems to behave well except
 * how it handles the exactly-N-packets case. It appears that there's a hiccup
 * in that case (maybe the DMA completes before a ZLP gets written?) boiling
 * down to not being able to rely on the XFER DMA writing any terminating zero
 * length packet before the next transfer is started...
 *
 * We alsways send terminating ZLPs *explictly* using DMA instead of doing it
 * by PIO after an IRQ.
 *
 */

/**
 * cppi41_next_tx_segment - DMA write for the next chunk of a buffer
 * @tx_ch:	Tx channel
 *
 * Context: controller IRQ-locked
 */
static unsigned cppi41_next_tx_segment(struct cppi41_channel *tx_ch)
{
	struct usb_pkt_desc *curr_pd;
	struct cppi41_host_pkt_desc *hw_desc;
	struct usb_cppi_dma_ch *tx_cppi = tx_ch->cppi_dma;
	u32 length = tx_ch->length - tx_ch->curr_offset;
	u32 pkt_size = tx_ch->pkt_size;
	unsigned num_pds, n;

	/*
	 * Tx can use the accelerated modes where we can probably fit this
	 * transfer in one PD and one IRQ.  The only time we would NOT want
	 * to use it is when the hardware constraints prevent it, or if we'd
	 * trigger the "send a ZLP?" confusion.
	 */
	if (USB_DEFAULT_TX_MODE != USB_TRANSPARENT_MODE &&
	    (pkt_size & 0x3f) == 0 &&
	    (USB_DEFAULT_TX_MODE == USB_GENERIC_RNDIS_MODE ||
	     length % pkt_size != 0)) {
		num_pds  = 1;
		pkt_size = length;
		cppi41_mode_update(tx_ch, USB_DEFAULT_TX_MODE);
	} else {
		num_pds  = (length + pkt_size - 1) / pkt_size;
		cppi41_mode_update(tx_ch, USB_TRANSPARENT_MODE);
	}

	/*
	 * If length of transmit buffer is a multiple of USB endpoint size
	 * then send the zero length packet.
	 */
	if (tx_ch->transfer_mode && !tx_ch->zlp_queued &&
	    length % pkt_size == 0)
		num_pds++;

	for (n = 0; n < num_pds; n++) {
		/* Get the free Tx host packet descriptors from Tx PD pool */
		curr_pd = usb_get_free_pd(tx_cppi);
		if (curr_pd == NULL) {
			DBG(1, "No Tx PDs\n");
			break;
		}

		if (length < pkt_size)
			pkt_size = length;

		hw_desc = &curr_pd->hw_desc;
		hw_desc->desc_info = (CPPI41_DESC_TYPE_HOST <<
				      CPPI41_DESC_TYPE_SHIFT) | pkt_size;
		hw_desc->buf_ptr = tx_ch->start_addr + tx_ch->curr_offset;
		hw_desc->buf_len = pkt_size;

		curr_pd->ep_num = tx_ch->end_pt->epnum;

		tx_ch->curr_offset += pkt_size;
		length -= pkt_size;

		if (pkt_size == 0)
			tx_ch->zlp_queued = 1;

		cppi41_queue_push(&tx_cppi->queueObj, virt_to_phys(curr_pd),
				  USB_CPPI41_DESC_ALIGN, pkt_size);
	}

	return n;
}

#if USB_DEFAULT_RX_MODE == USB_GENERIC_RNDIS_MODE
static void cppi41_autoreq_update(struct cppi41_channel *rx_ch, u8 autoreq)
{
	struct cppi41 *cppi = rx_ch->channel.private_data;

	if (is_host_active(cppi->musb) &&
	    autoreq != rx_ch->autoreq) {
		void *__iomem reg_base = cppi->musb->ctrl_base;
		u32 reg_val = musb_readl(reg_base, USB_AUTOREQ_REG);
		u8 ep_num = rx_ch->ch_num + 1;

		reg_val &= ~USB_RX_AUTOREQ_MASK(ep_num);
		reg_val |= autoreq << USB_RX_AUTOREQ_SHIFT(ep_num);

		musb_writel(reg_base, USB_AUTOREQ_REG, reg_val);
		rx_ch->autoreq = autoreq;
	}
}

static void cppi41_set_ep_size(struct cppi41_channel *rx_ch, u32 pkt_size)
{
	struct cppi41 *cppi = rx_ch->channel.private_data;
	void *__iomem reg_base = cppi->musb->ctrl_base;
	u8 ep_num = rx_ch->ch_num + 1;

	musb_writel(reg_base, USB_GENERIC_RNDIS_EP_SIZE_REG(ep_num), pkt_size);
}

#else
static inline void cppi41_autoreq_update(struct cppi41_channel *, u8) {}
static inline void cppi41_set_ep_size(struct cppi41_channel *, u32) {}
#endif

/*
 * CPPI 4.1 Rx:
 * ============
 * Consider a 1KB bulk Rx buffer in two scenarios: (a) it's fed two 300 byte
 * packets back-to-back, and (b) it's fed two 512 byte packets back-to-back.
 * (Full speed transfers have similar scenarios.)
 *
 * The correct behavior for Linux is that (a) fills the buffer with 300 bytes,
 * and the next packet goes into a buffer that's queued later; while (b) fills
 * the buffer with 1024 bytes.  How to do that with accelerated DMA modes?
 *
 * Rx queues in RNDIS mode (one single BD) handle (a) correctly but (b) loses
 * BADLY because nothing (!) happens when that second packet fills the buffer,
 * much less when a third one arrives -- which makes it not a "true" RNDIS mode.
 * In the RNDIS protocol short-packet termination is optional, and it's fine if
 * the peripherals (not hosts!) pad the messages out to end of buffer. Standard
 * PCI host controller DMA descriptors implement that mode by default... which
 * is no accident.
 *
 * Generic RNDIS mode is the only way to reliably make both cases work.  This
 * mode is identical to the "normal" RNDIS mode except for the case where the
 * last packet of the segment matches the max USB packet size -- in this case,
 * the packet will be closed when a value (0x10000 max) in the Generic RNDIS
 * EP Size register is reached.  This mode will work for the network drivers
 * (CDC/RNDIS) as well as for the mass storage drivers where there is no short
 * packet.
 *
 * BUT we can only use non-transparent modes when USB packet size is a multiple
 * of 64 bytes. Let's see what happens in this case...
 *
 * Rx queues (2 BDs with 512 bytes each) have converse problems to RNDIS mode:
 * (b) is handled right but (a) loses badly.  DMA doesn't stop after receiving
 * a short packet and processes both of those PDs; so both packets are loaded
 * into the buffer (with 212 byte gap between them), and the next buffer queued
 * will NOT get its 300 bytes of data.  Even in the case when there should be
 * no short packets (URB_SHORT_NOT_OK is set), queueing several packets in the
 * host mode doesn't win us anything since we have to manually "prod" the Rx
 * process after each packet is received by setting ReqPkt bit in endpoint's
 * RXCSR; in the peripheral mode without short packets, queueing could be used
 * BUT we'll have to *teardown* the channel if a short packet still arrives in
 * the peripheral mode, and to "collect" the left-over packet descriptors from
 * the free descriptor/buffer queue in both cases...
 *
 * One BD at a time is the only way to make make both cases work reliably, with
 * software handling both cases correctly, at the significant penalty of needing
 * an IRQ per packet.  (The lack of I/O overlap can be slightly ameliorated by
 * enabling double buffering.)
 *
 * There seems to be no way to identify for sure the cases where the CDC mode
 * is appropriate...
 *
 */

/**
 * cppi41_next_rx_segment - DMA read for the next chunk of a buffer
 * @rx_ch:	Rx channel
 *
 * Context: controller IRQ-locked (?)
 *
 * NOTE: In the transparent mode, we have to queue one packet at a time since:
 *	 - we must avoid starting reception of another packet after receiving
 *	   a short packet;
 *	 - in host mode we have to set ReqPkt bit in the endpoint's RXCSR after
 *	   receiving each packet but the last one... ugly!
 */
static unsigned cppi41_next_rx_segment(struct cppi41_channel *rx_ch)
{
	struct cppi41 *cppi = rx_ch->channel.private_data;
	struct usb_cppi_dma_ch *rx_cppi = rx_ch->cppi_dma;
	struct usb_pkt_desc *curr_pd;
	struct cppi41_host_pkt_desc *hw_desc;
	u32 length = rx_ch->length - rx_ch->curr_offset;
	u32 pkt_size = rx_ch->pkt_size;

	if (!length)
		return 0;

	/*
	 * Rx can use the generic RNDIS mode where we can probably fit this
	 * transfer in one PD and one IRQ (or two with a short packet).
	 */
	if (USB_DEFAULT_RX_MODE == USB_GENERIC_RNDIS_MODE &&
	    (pkt_size & 0x3f) == 0 && length >= 2 * pkt_size) {
		cppi41_mode_update(rx_ch, USB_DEFAULT_RX_MODE);
		cppi41_autoreq_update(rx_ch, USB_AUTOREQ_ALL_BUT_EOP);

		if (likely(length < 0x10000))
			pkt_size = length - length % pkt_size;
		else
			pkt_size = 0x10000;
		cppi41_set_ep_size(rx_ch, pkt_size);
	} else {
		cppi41_mode_update(rx_ch, USB_TRANSPARENT_MODE);
		cppi41_autoreq_update(rx_ch, USB_NO_AUTOREQ);
	}

	/* Get the free Rx packet descriptor from free Rx PD pool */
	curr_pd = usb_get_free_pd(rx_cppi);
	if (curr_pd == NULL) {
		/* Shouldn't ever happen! */
		DBG(1, "No Rx PDs\n");
		return 0;
	}

	/*
	 * HCD arranged ReqPkt for the first packet.
	 * We arrange it for all but the last one.
	 */
	if (is_host_active(cppi->musb) && rx_ch->channel.actual_len) {
		void __iomem *epio = rx_ch->end_pt->regs;
		u16 csr = musb_readw(epio, MUSB_RXCSR);

		csr |= MUSB_RXCSR_H_REQPKT | MUSB_RXCSR_H_WZC_BITS;
		musb_writew(epio, MUSB_RXCSR, csr);
	}

	if (length < pkt_size)
		pkt_size = length;

	hw_desc = &curr_pd->hw_desc;
	hw_desc->orig_buf_ptr = rx_ch->start_addr + rx_ch->curr_offset;
	hw_desc->orig_buf_len = pkt_size;

	curr_pd->ep_num = rx_ch->end_pt->epnum;

	rx_ch->curr_offset += pkt_size;

	/*
	 * Push the free Rx packet descriptor
	 * to the free descriptor/buffer queue.
	 */
	cppi41_queue_push(&rx_cppi->queueObj, virt_to_phys(curr_pd),
			  USB_CPPI41_DESC_ALIGN, 0);

	return 1;
}

/**
 * cppi41_channel_program - program channel for data transfer
 * @channel:	the channel
 * @maxpacket:	max packet size
 * @mode:	for Rx, 1 unless the USB protocol driver promised to treat
 *		all short reads as errors and kick in high level fault recovery;
 *		for Tx, 0 unless the protocol driver _requires_ short-packet
 *		termination mode
 * @dma_addr:	DMA address of buffer
 * @length:	length of buffer
 *
 * Context: controller IRQ-locked
 */
static int cppi41_channel_program(struct dma_channel *channel,	u16 maxpacket,
				  u8 mode, dma_addr_t dma_addr, u32 length)
{
	struct cppi41_channel *cppi_ch;
	unsigned queued;

	cppi_ch = container_of(channel, struct cppi41_channel, channel);

	switch (channel->status) {
	case MUSB_DMA_STATUS_BUS_ABORT:
	case MUSB_DMA_STATUS_CORE_ABORT:
		/* Fault IRQ handler should have handled cleanup */
		WARN("%cx DMA%d not cleaned up after abort!\n",
		     cppi_ch->transmit ? 'T' : 'R', cppi_ch->ch_num);
		break;
	case MUSB_DMA_STATUS_BUSY:
		WARN("Program active channel? %cx DMA%d\n",
		     cppi_ch->transmit ? 'T' : 'R', cppi_ch->ch_num);
		break;
	case MUSB_DMA_STATUS_UNKNOWN:
		DBG(1, "%cx DMA%d not allocated!\n",
		    cppi_ch->transmit ? 'T' : 'R', cppi_ch->ch_num);
		/* FALLTHROUGH */
	case MUSB_DMA_STATUS_FREE:
		break;
	}

	channel->status = MUSB_DMA_STATUS_BUSY;

	/* Set the transfer parameters, then queue up the first segment */
	cppi_ch->start_addr = dma_addr;
	cppi_ch->curr_offset = 0;
	cppi_ch->pkt_size = maxpacket;
	cppi_ch->length = length;
	cppi_ch->transfer_mode = mode;
	cppi_ch->zlp_queued = 0;

	/* Tx or Rx channel? */
	if (cppi_ch->transmit)
		queued = cppi41_next_tx_segment(cppi_ch);
	else
		queued = cppi41_next_rx_segment(cppi_ch);

	return (queued > 0);
}

static void usb_cppi_ch_teardown(struct cppi41_channel *cppi_ch)
{
	struct usb_cppi_dma_ch *cppi_dma = cppi_ch->cppi_dma;
	unsigned long bd_addr;

#ifdef DEBUG_CPPI_TD
	printk("Before teardown...\n");
	print_pd_list(cppi_dma->pdPoolHead);
#endif

	/* Initiate teardown for a CPPI DMA channel */
	cppi_ch->teardown_pending = 1;
	cppi41_dma_ch_teardown(&cppi_dma->dmaChObj);

	while (cppi_ch->teardown_pending) {}

#ifdef DEBUG_CPPI_TD
	printk("After teardown\n");
	print_pd_list(cppi_dma->pdPoolHead);
#endif

	/*
	 * There might be packets on the Tx queue that were not sent --
	 * they need to be recycled properly.
	 */
	while ((bd_addr = cppi41_queue_pop(&cppi_dma->queueObj)) != 0) {
		struct usb_pkt_desc *curr_pd = phys_to_virt(bd_addr);

		dprintk("PD (%p) popped from queue\n", curr_pd);

		/*
		 * Return Tx PDs to the software list --
		 * this is protected by critical section.
		 */
		usb_put_free_pd(cppi_dma, curr_pd);
	}

}

/*
 * cppi41_channel_abort
 *
 * Context: controller IRQ-locked, endpoint selected.
 */
static int cppi41_channel_abort(struct dma_channel *channel)
{
	struct cppi41 *cppi;
	struct cppi41_channel *cppi_ch;
	struct musb  *musb;
	void __iomem *reg_base, *epio;
	u32 csr, td_reg;
	u8 ch_num, ep_num;

	cppi_ch = container_of(channel, struct cppi41_channel, channel);
	ch_num = cppi_ch->ch_num;

	switch (channel->status) {
	case MUSB_DMA_STATUS_BUS_ABORT:
	case MUSB_DMA_STATUS_CORE_ABORT:
		/* From Rx or Tx fault IRQ handler */
	case MUSB_DMA_STATUS_BUSY:
		/* The hardware needs shutting down... */
		dprintk("%s: DMA busy, status = %x\n",
			__func__, channel->status);
		break;
	case MUSB_DMA_STATUS_UNKNOWN:
		DBG(8, "%cx DMA%d not allocated\n",
		    cppi_ch->transmit ? 'T' : 'R', ch_num);
		/* FALLTHROUGH */
	case MUSB_DMA_STATUS_FREE:
		break;
	}

	cppi = cppi_ch->channel.private_data;
	musb = cppi->musb;
	reg_base = musb->ctrl_base;
	epio = cppi_ch->end_pt->regs;
	ep_num = ch_num + 1;

	if (cppi_ch->transmit) {
		dprintk("Tx channel teardown, cppi_ch = %p\n", cppi_ch);

		/* Tear down the Tx channel */
		usb_cppi_ch_teardown(cppi_ch);

		/* Issue CPPI FIFO teardown for Tx channel */
		td_reg = musb_readl(reg_base, USB_TEARDOWN_REG);
		td_reg |= USB_TX_TDOWN_MASK(ep_num);
		musb_writel(reg_base, USB_TEARDOWN_REG, td_reg);

		csr  = musb_readw(epio, MUSB_TXCSR);
		csr |= MUSB_TXCSR_FLUSHFIFO | MUSB_TXCSR_H_WZC_BITS;
		musb_writew(epio, MUSB_TXCSR, csr);
	} else { /* Rx */
		dprintk("Rx channel teardown, cppi_ch = %p\n", cppi_ch);

		/* Flush the FIFO of endpoint */
		csr  = musb_readw(epio, MUSB_RXCSR);
		csr |= MUSB_RXCSR_FLUSHFIFO | MUSB_RXCSR_H_WZC_BITS;
		musb_writew(epio, MUSB_RXCSR, csr);

		/* Issue CPPI FIFO teardown for Rx channel... */
		td_reg = musb_readl(reg_base, USB_TEARDOWN_REG);
		td_reg |= USB_RX_TDOWN_MASK(ep_num);
		musb_writel(reg_base, USB_TEARDOWN_REG, td_reg);

		/* Tear down the DMA channel */
		usb_cppi_ch_teardown(cppi_ch);

		/*
		 * NOTE: docs don't guarantee any of this works...  we expect
		 * that if the USB core stops telling the CPPI core to pull
		 * more data from it, then it'll be safe to flush current Rx
		 * DMA state iff any pending FIFO transfer is done.
		 */

		/* For host, ensure ReqPkt is never set again */
		cppi41_autoreq_update(cppi_ch, USB_NO_AUTOREQ);

		csr = musb_readw(epio, MUSB_RXCSR);

		/* For host, clear (just) ReqPkt at end of current packet(s) */
		if (is_host_active(cppi->musb))
			csr &= ~MUSB_RXCSR_H_REQPKT;
		csr |= MUSB_RXCSR_H_WZC_BITS;

		/* Clear DMA enable */
		csr &= ~MUSB_RXCSR_DMAENAB;
		musb_writew(epio, MUSB_RXCSR, csr);

		/* Flush the FIFO of endpoint once again */
		csr  = musb_readw(epio, MUSB_RXCSR);
		csr |= MUSB_RXCSR_FLUSHFIFO | MUSB_RXCSR_H_WZC_BITS;
		musb_writew(epio, MUSB_RXCSR, csr);

		udelay(50);
	}

	/* Re-enable the DMA channel */
	cppi41_dma_ch_enable(&cppi_ch->cppi_dma->dmaChObj);

	channel->status = MUSB_DMA_STATUS_FREE;

	return 0;
}

/**
 * dma_controller_create - instantiate an object representing DMA controller.
 */
struct dma_controller * __init dma_controller_create(struct musb  *musb,
						     void __iomem *mregs)
{
	struct cppi41 *cppi;

	cppi = kzalloc(sizeof *cppi, GFP_KERNEL);
	if (!cppi)
		return NULL;

	/* Initialize the CPPI 4.1 DMA controller structure */
	cppi->musb  = musb;
	cppi->controller.start = cppi41_controller_start;
	cppi->controller.stop  = cppi41_controller_stop;
	cppi->controller.channel_alloc = cppi41_channel_alloc;
	cppi->controller.channel_release = cppi41_channel_release;
	cppi->controller.channel_program = cppi41_channel_program;
	cppi->controller.channel_abort = cppi41_channel_abort;

	return &cppi->controller;
}

/**
 * dma_controller_destroy - destroy a previously instantiated DMA controller
 * @controller: the controller
 */
void dma_controller_destroy(struct dma_controller *controller)
{
	struct cppi41 *cppi;

	cppi = container_of(controller, struct cppi41, controller);

	/* Free the CPPI object */
	kfree(cppi);
}

static void usb_process_tx_queue(struct cppi41 *cppi, unsigned index)
{
	struct cppi41_queue_obj tx_queue_obj;
	unsigned long bd_addr;

	if (cppi41_queue_init(&tx_queue_obj, usb_cppi41_info.q_mgr,
			      usb_cppi41_info.tx_comp_q[index])) {
		DBG(1, "ERROR: cppi41_queue_init failed for "
		    "Tx completion queue");
		return;
	}

	while ((bd_addr = cppi41_queue_pop(&tx_queue_obj))) {
		struct usb_pkt_desc  *curr_pd = phys_to_virt(bd_addr);
		struct cppi41_channel *tx_ch;
		u8 ch_num, ep_num;
		u32 length;

		if (unlikely(cppi41_get_teardown_info(curr_pd, NULL,
						      &ch_num, NULL) == 0)) {
			u8 *ptr;

			dprintk("Teardown descriptor (%p) popped from "
				"Tx completion queue\n", curr_pd);
			ptr = memchr(usb_cppi41_info.ep_dma_ch, ch_num,
				     USB_CPPI41_NUM_CH);
			if (ptr == NULL) {
				DBG(1, "ERROR: unknown DMA channel in Tx "
				    "teardown descriptor\n");
				continue;
			}

			cppi->txCppiCh[*ptr].teardown_pending = 0;
			continue;
		}

		/* Extract the data from received packet descriptor */
		ch_num = curr_pd->ch_num;
		ep_num = curr_pd->ep_num;
		length = curr_pd->hw_desc.buf_len;

		tx_ch = &cppi->txCppiCh[ch_num];
		tx_ch->channel.actual_len += length;

		/*
		 * Return Tx PD to the software list --
		 * this is protected by critical section
		 */
		usb_put_free_pd(tx_ch->cppi_dma, curr_pd);

		if (unlikely(tx_ch->channel.actual_len >= tx_ch->length &&
			     !(tx_ch->transfer_mode &&
			       length == tx_ch->pkt_size))) {
			tx_ch->channel.status = MUSB_DMA_STATUS_FREE;

			/* Tx completion routine callback */
			musb_dma_completion(cppi->musb, ep_num, 1);
		} else
			cppi41_next_tx_segment(tx_ch);
	}
}

static void usb_process_rx_queue(struct cppi41 *cppi, unsigned index)
{
	struct cppi41_queue_obj rx_queue_obj;
	unsigned long bd_addr;

	if (cppi41_queue_init(&rx_queue_obj, usb_cppi41_info.q_mgr,
			      usb_cppi41_info.rx_comp_q[index])) {
		DBG(1, "ERROR: cppi41_queue_init failed for Rx queue\n");
		return;
	}

	while ((bd_addr = cppi41_queue_pop(&rx_queue_obj))) {
		struct usb_pkt_desc  *curr_pd = phys_to_virt(bd_addr);
		struct cppi41_channel *rx_ch;
		u8 ch_num, ep_num;
		u32 length;

		if (!cppi41_get_teardown_info(curr_pd, NULL, &ch_num, NULL)) {
			u8 *ptr;

			dprintk("Teardown descriptor (%p) popped from "
				"Rx completion queue\n", curr_pd);
			ptr = memchr(usb_cppi41_info.ep_dma_ch, ch_num,
				     USB_CPPI41_NUM_CH);
			if (ptr == NULL) {
				DBG(1, "ERROR: unknown DMA channel in Rx "
				    "teardown descriptor\n");
				continue;
			}

			cppi->rxCppiCh[*ptr].teardown_pending = 0;
			continue;
		}

		/* Extract the data from received packet descriptor */
		ch_num = curr_pd->ch_num;
		ep_num = curr_pd->ep_num;
		length = curr_pd->hw_desc.buf_len;

		rx_ch = &cppi->rxCppiCh[ch_num];
		rx_ch->channel.actual_len += length;

		/*
		 * Return Rx PD to the software list --
		 * this is protected by critical section
		 */
		usb_put_free_pd(rx_ch->cppi_dma, curr_pd);

		if (unlikely(rx_ch->channel.actual_len >= rx_ch->length ||
			     length < curr_pd->hw_desc.orig_buf_len)) {
			rx_ch->channel.status = MUSB_DMA_STATUS_FREE;

			/* Rx completion routine callback */
			musb_dma_completion(cppi->musb, ep_num, 0);
		} else
			cppi41_next_rx_segment(rx_ch);
	}
}

/*
 * cppi41_completion - handle interrupts from the Tx/Rx completion queues
 *
 * NOTE: since we have to manually prod the Rx process in the transparent mode,
 *	 we certainly want to handle the Rx queues first.
 */
void cppi41_completion(struct musb *musb, u32 rx, u32 tx)
{
	struct cppi41 *cppi;
	unsigned index;

	cppi = container_of(musb->dma_controller, struct cppi41, controller);

	/* Process packet descriptors from the Rx queues */
	for (index = 0; rx != 0; rx >>= 1, index++)
		if (rx & 1)
			usb_process_rx_queue(cppi, index);

	/* Process packet descriptors from the Tx completion queues */
	for (index = 0; tx != 0; tx >>= 1, index++)
		if (tx & 1)
			usb_process_tx_queue(cppi, index);
}
