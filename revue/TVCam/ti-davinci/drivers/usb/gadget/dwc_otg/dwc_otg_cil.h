/* ==========================================================================
 * Synopsys HS OTG Linux Software Driver and documentation (hereinafter,
 * "Software") is an Unsupported proprietary work of Synopsys, Inc. unless
 * otherwise expressly agreed to in writing between Synopsys and you.
 *
 * The Software IS NOT an item of Licensed Software or Licensed Product under
 * any End User Software License Agreement or Agreement for Licensed Product
 * with Synopsys or any supplement thereto. You are permitted to use and
 * redistribute this Software in source and binary forms, with or without
 * modification, provided that redistributions of source code must retain this
 * notice. You may not view, use, disclose, copy or distribute this file or
 * any information contained herein except pursuant to this license grant from
 * Synopsys. If you do not agree with this notice, including the disclaimer
 * below, then you are not authorized to use the Software.
 *
 * THIS SOFTWARE IS BEING DISTRIBUTED BY SYNOPSYS SOLELY ON AN "AS IS" BASIS
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE HEREBY DISCLAIMED. IN NO EVENT SHALL SYNOPSYS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * ========================================================================== */

#ifndef __DWC_CIL_H__
#define __DWC_CIL_H__

#include "dwc_otg_plat.h"

#ifdef DEBUG
#include "linux/timer.h"
#endif

/**
 * @file
 * This file contains the interface to the Core Interface Layer.
 */

/*
 * Added-sr: 2007-07-26
 *
 * Since the 405EZ (Ultra) only support 2047 bytes as
 * max transfer size, we have to split up bigger transfers
 * into multiple transfers of 1024 bytes sized messages.
 * I happens often, that transfers of 4096 bytes are
 * required (zero-gadget, file_storage-gadget).
 *
 * DWC_405EZ_MAX_XFER_LEN is set to 1024 right now, but could be 2047,
 * since the xfer-size field in the 405EZ USB device controller
 * implementation has 11 bits. Using 1024 seems to work for now.
 */
#define DWC_405EZ_MAX_XFER_LEN	1024

/**
 * The <code>dwc_ep</code> structure represents the state of a single
 * endpoint when acting in device mode. It contains the data items
 * needed for an endpoint to be activated and transfer packets.
 */
typedef struct dwc_ep {
	/** EP number used for register address lookup */
	uint8_t	 num;
	/** EP direction 0 = OUT */
	unsigned is_in : 1;
	/** EP active. */
	unsigned active : 1;

	/** Periodic Tx FIFO # for IN EPs For INTR EP set to 0
	 * to use non-periodic Tx FIFO If dedicated Tx FIFOs are enabled
	 * for all IN Eps - Tx FIFO # FOR IN EPs*/
	unsigned tx_fifo_num : 4;
	/** EP type: 0 - Control, 1 - ISOC,	 2 - BULK,	3 - INTR */
	unsigned type : 2;
#define DWC_OTG_EP_TYPE_CONTROL	   0
#define DWC_OTG_EP_TYPE_ISOC	   1
#define DWC_OTG_EP_TYPE_BULK	   2
#define DWC_OTG_EP_TYPE_INTR	   3

	/** DATA start PID for INTR and BULK EP */
	unsigned data_pid_start : 1;
	/** Frame (even/odd) for ISOC EP */
	unsigned even_odd_frame : 1;
	/** Max Packet bytes */
	unsigned maxpacket : 11;

	/** @name Transfer state */
	/** @{ */

	/**
	 * Pointer to the beginning of the transfer buffer -- do not modify
	 * during transfer.
	 */

	uint32_t dma_addr;

	uint8_t *start_xfer_buff;
	/** pointer to the transfer buffer */
	uint8_t *xfer_buff;
	/** Number of bytes to transfer */
	unsigned xfer_len : 19;
	/** Number of bytes transferred. */
	unsigned xfer_count : 19;
	/** Sent ZLP */
	unsigned sent_zlp : 1;
	/** Total len for control transfer */
	unsigned total_len : 19;

	/** stall clear flag */
	unsigned stall_clear_flag : 1;

	/*
	 * Added-sr: 2007-07-26
	 *
	 * Since the 405EZ (Ultra) only support 2047 bytes as
	 * max transfer size, we have to split up bigger transfers
	 * into multiple transfers of 1024 bytes sized messages.
	 * I happens often, that transfers of 4096 bytes are
	 * required (zero-gadget, file_storage-gadget).
	 *
	 * "bytes_pending" will hold the amount of bytes that are
	 * still pending to be send in further messages to complete
	 * the bigger transfer.
	 */
	u32 bytes_pending;

	/** @} */
} dwc_ep_t;

/*
 * Reasons for halting a host channel.
 */
typedef enum dwc_otg_halt_status {
	DWC_OTG_HC_XFER_NO_HALT_STATUS,
	DWC_OTG_HC_XFER_COMPLETE,
	DWC_OTG_HC_XFER_URB_COMPLETE,
	DWC_OTG_HC_XFER_ACK,
	DWC_OTG_HC_XFER_NAK,
	DWC_OTG_HC_XFER_NYET,
	DWC_OTG_HC_XFER_STALL,
	DWC_OTG_HC_XFER_XACT_ERR,
	DWC_OTG_HC_XFER_FRAME_OVERRUN,
	DWC_OTG_HC_XFER_BABBLE_ERR,
	DWC_OTG_HC_XFER_DATA_TOGGLE_ERR,
	DWC_OTG_HC_XFER_AHB_ERR,
	DWC_OTG_HC_XFER_PERIODIC_INCOMPLETE,
	DWC_OTG_HC_XFER_URB_DEQUEUE
} dwc_otg_halt_status_e;

/**
 * Host channel descriptor. This structure represents the state of a single
 * host channel when acting in host mode. It contains the data items needed to
 * transfer packets to an endpoint via a host channel.
 */
typedef struct dwc_hc {
	/** Host channel number used for register address lookup */
	uint8_t	 hc_num;

	/** Device to access */
	unsigned dev_addr : 7;

	/** EP to access */
	unsigned ep_num : 4;

	/** EP direction. 0: OUT, 1: IN */
	unsigned ep_is_in : 1;

	/**
	 * EP speed.
	 * One of the following values:
	 *	- DWC_OTG_EP_SPEED_LOW
	 *	- DWC_OTG_EP_SPEED_FULL
	 *	- DWC_OTG_EP_SPEED_HIGH
	 */
	unsigned speed : 2;
#define DWC_OTG_EP_SPEED_LOW	0
#define DWC_OTG_EP_SPEED_FULL	1
#define DWC_OTG_EP_SPEED_HIGH	2

	/**
	 * Endpoint type.
	 * One of the following values:
	 *	- DWC_OTG_EP_TYPE_CONTROL: 0
	 *	- DWC_OTG_EP_TYPE_ISOC: 1
	 *	- DWC_OTG_EP_TYPE_BULK: 2
	 *	- DWC_OTG_EP_TYPE_INTR: 3
	 */
	unsigned ep_type : 2;

	/** Max packet size in bytes */
	unsigned max_packet : 11;

	/**
	 * PID for initial transaction.
	 * 0: DATA0,<br>
	 * 1: DATA2,<br>
	 * 2: DATA1,<br>
	 * 3: MDATA (non-Control EP),
	 *	  SETUP (Control EP)
	 */
	unsigned data_pid_start : 2;
#define DWC_OTG_HC_PID_DATA0 0
#define DWC_OTG_HC_PID_DATA2 1
#define DWC_OTG_HC_PID_DATA1 2
#define DWC_OTG_HC_PID_MDATA 3
#define DWC_OTG_HC_PID_SETUP 3

	/** Number of periodic transactions per (micro)frame */
	unsigned multi_count: 2;

	/** @name Transfer State */
	/** @{ */

	/** Pointer to the current transfer buffer position. */
	uint8_t *xfer_buff;
	/** Total number of bytes to transfer. */
	uint32_t xfer_len;
	/** Number of bytes transferred so far. */
	uint32_t xfer_count;
	/** Packet count at start of transfer.*/
	uint16_t start_pkt_count;

	/**
	 * Flag to indicate whether the transfer has been started. Set to 1 if
	 * it has been started, 0 otherwise.
	 */
	uint8_t xfer_started;

	/**
	 * Set to 1 to indicate that a PING request should be issued on this
	 * channel. If 0, process normally.
	 */
	uint8_t do_ping;

	/**
	 * Set to 1 to indicate that the error count for this transaction is
	 * non-zero. Set to 0 if the error count is 0.
	 */
	uint8_t error_state;

	/**
	 * Set to 1 to indicate that this channel should be halted the next
	 * time a request is queued for the channel. This is necessary in
	 * slave mode if no request queue space is available when an attempt
	 * is made to halt the channel.
	 */
	uint8_t halt_on_queue;

	/**
	 * Set to 1 if the host channel has been halted, but the core is not
	 * finished flushing queued requests. Otherwise 0.
	 */
	uint8_t halt_pending;

	/**
	 * Reason for halting the host channel.
	 */
	dwc_otg_halt_status_e	halt_status;

	/*
	 * Split settings for the host channel
	 */
	uint8_t do_split;	   /**< Enable split for the channel */
	uint8_t complete_split;	   /**< Enable complete split */
	uint8_t hub_addr;	   /**< Address of high speed hub */

	uint8_t port_addr;	   /**< Port of the low/full speed device */
	/** Split transaction position
	 * One of the following values:
	 *	  - DWC_HCSPLIT_XACTPOS_MID
	 *	  - DWC_HCSPLIT_XACTPOS_BEGIN
	 *	  - DWC_HCSPLIT_XACTPOS_END
	 *	  - DWC_HCSPLIT_XACTPOS_ALL */
	uint8_t xact_pos;

	/** Set when the host channel does a short read. */
	uint8_t short_read;

	/**
	 * Number of requests issued for this channel since it was assigned to
	 * the current transfer (not counting PINGs).
	 */
	uint8_t requests;

	/**
	 * Queue Head for the transfer being processed by this channel.
	 */
	struct dwc_otg_qh *qh;

	/** @} */

	/** Entry in list of host channels. */
	struct list_head	hc_list_entry;
} dwc_hc_t;

#ifdef DEBUG
struct dwc_otg_core_if;
typedef struct hc_xfer_info {
	struct dwc_otg_core_if	*core_if;
	dwc_hc_t		*hc;
} hc_xfer_info_t;
#endif

/**
 * The <code>dwc_otg_core_if</code> structure contains information needed to manage
 * the DWC_otg controller acting in either host or device mode. It
 * represents the programming view of the controller as a whole.
 */
typedef struct dwc_otg_core_if {
	/** Parameters that define how the core should be configured.*/
	dwc_otg_core_params_t	   *core_params;

	/** Core Global registers starting at offset 000h. */
	dwc_otg_core_global_regs_t *core_global_regs;

	/** Device-specific information */
	dwc_otg_dev_if_t		   *dev_if;
	/** Host-specific information */
	dwc_otg_host_if_t		   *host_if;

	/*
	 * Set to 1 if the core PHY interface bits in USBCFG have been
	 * initialized.
	 */
	uint8_t phy_init_done;

	/*
	 * SRP Success flag, set by srp success interrupt in FS I2C mode
	 */
	uint8_t srp_success;
	uint8_t srp_timer_started;

	/* Common configuration information */
	/** Power and Clock Gating Control Register */

	/*__iomem*/
	uint32_t __iomem *pcgcctl;
#define DWC_OTG_PCGCCTL_OFFSET 0xE00

	/** Push/pop addresses for endpoints or host channels.*/
	uint32_t *data_fifo[MAX_EPS_CHANNELS];
#define DWC_OTG_DATA_FIFO_OFFSET 0x1000
#define DWC_OTG_DATA_FIFO_SIZE 0x1000

	/** Total RAM for FIFOs (Bytes) */
	uint16_t total_fifo_size;
	/** Size of Rx FIFO (Bytes) */
	uint16_t rx_fifo_size;
	/** Size of Non-periodic Tx FIFO (Bytes) */
	uint16_t nperio_tx_fifo_size;


	/** 1 if DMA is enabled, 0 otherwise. */
	uint8_t dma_enable;

	/** 1 if dedicated Tx FIFOs are enabled, 0 otherwise. */
	uint8_t en_multiple_tx_fifo;

	/** Set to 1 if multiple packets of a high-bandwidth transfer is in
	 * process of being queued */
	uint8_t queuing_high_bandwidth;

	/** Hardware Configuration -- stored here for convenience.*/
	hwcfg1_data_t hwcfg1;
	hwcfg2_data_t hwcfg2;
	hwcfg3_data_t hwcfg3;
	hwcfg4_data_t hwcfg4;

	/** The operational State, during transations
	 * (a_host>>a_peripherial and b_device=>b_host) this may not
	 * match the core but allows the software to determine
	 * transitions.
	 */
	uint8_t op_state;

	/**
	 * Set to 1 if the HCD needs to be restarted on a session request
	 * interrupt. This is required if no connector ID status change has
	 * occurred since the HCD was last disconnected.
	 */
	uint8_t restart_hcd_on_session_req;

	/** HCD callbacks */
	/** A-Device is a_host */
#define A_HOST		(1)
	/** A-Device is a_suspend */
#define A_SUSPEND	(2)
	/** A-Device is a_peripherial */
#define A_PERIPHERAL	(3)
	/** B-Device is operating as a Peripheral. */
#define B_PERIPHERAL	(4)
	/** B-Device is operating as a Host. */
#define B_HOST		(5)

	/** HCD callbacks */
	struct dwc_otg_cil_callbacks *hcd_cb;
	/** PCD callbacks */
	struct dwc_otg_cil_callbacks *pcd_cb;

	/** Device mode Periodic Tx FIFO Mask */
	uint32_t p_tx_msk;
	/** Device mode Periodic Tx FIFO Mask */
	uint32_t tx_msk;
	void	*priv;

#ifdef DEBUG
	uint32_t		start_hcchar_val[MAX_EPS_CHANNELS];

	hc_xfer_info_t		hc_xfer_info[MAX_EPS_CHANNELS];
	struct timer_list	hc_xfer_timer[MAX_EPS_CHANNELS];

	uint32_t		hfnum_7_samples;
	uint64_t		hfnum_7_frrem_accum;
	uint32_t		hfnum_0_samples;
	uint64_t		hfnum_0_frrem_accum;
	uint32_t		hfnum_other_samples;
	uint64_t		hfnum_other_frrem_accum;
#endif

} dwc_otg_core_if_t;

/*
 * The following functions support initialization of the CIL driver component
 * and the DWC_otg controller.
 */
extern dwc_otg_core_if_t *dwc_otg_cil_init(const uint32_t *_reg_base_addr,
				dwc_otg_core_params_t *_core_params);
extern void dwc_otg_cil_remove(dwc_otg_core_if_t *_core_if);
extern void dwc_otg_core_init(dwc_otg_core_if_t *_core_if);
extern void dwc_otg_core_host_init(dwc_otg_core_if_t *_core_if);
extern void dwc_otg_core_dev_init(dwc_otg_core_if_t *_core_if);
extern void dwc_otg_enable_global_interrupts(dwc_otg_core_if_t *_core_if);
extern void dwc_otg_disable_global_interrupts(dwc_otg_core_if_t *_core_if);
extern void dwc_otg_disable_all_interrupts(dwc_otg_core_if_t *_core_if);

/** @name Device CIL Functions
 * The following functions support managing the DWC_otg controller in device
 * mode.
 */
/**@{*/
extern void dwc_otg_wakeup(dwc_otg_core_if_t *_core_if);
extern void dwc_otg_read_setup_packet(dwc_otg_core_if_t *_core_if,
					uint32_t *_dest);
extern uint32_t dwc_otg_get_frame_number(dwc_otg_core_if_t *_core_if);
extern void dwc_otg_ep0_activate(dwc_otg_core_if_t *_core_if, dwc_ep_t *_ep);
extern void dwc_otg_ep_activate(dwc_otg_core_if_t *_core_if, dwc_ep_t *_ep);
extern void dwc_otg_ep_deactivate(dwc_otg_core_if_t *_core_if, dwc_ep_t *_ep);
extern void dwc_otg_ep_start_transfer(dwc_otg_core_if_t *_core_if,
					dwc_ep_t *_ep);
extern void dwc_otg_ep0_start_transfer(dwc_otg_core_if_t *_core_if,
					dwc_ep_t *_ep);
extern void dwc_otg_ep0_continue_transfer(dwc_otg_core_if_t *_core_if,
					dwc_ep_t *_ep);
extern void dwc_otg_ep_write_packet(dwc_otg_core_if_t *_core_if,
					dwc_ep_t *_ep, int _dma);
extern void dwc_otg_ep_set_stall(dwc_otg_core_if_t *_core_if, dwc_ep_t *_ep);
extern void dwc_otg_ep_clear_stall(dwc_otg_core_if_t *_core_if, dwc_ep_t *_ep);
extern void dwc_otg_enable_device_interrupts(dwc_otg_core_if_t *_core_if);
extern void dwc_otg_dump_dev_registers(dwc_otg_core_if_t *_core_if);
/**@}*/

/** @name Host CIL Functions
 * The following functions support managing the DWC_otg controller in host
 * mode.
 */
/**@{*/
extern void dwc_otg_hc_init(dwc_otg_core_if_t *_core_if, dwc_hc_t *_hc);
extern void dwc_otg_hc_halt(dwc_otg_core_if_t *_core_if,
				dwc_hc_t *_hc,
				dwc_otg_halt_status_e _halt_status);
extern void dwc_otg_hc_cleanup(dwc_otg_core_if_t *_core_if, dwc_hc_t *_hc);
extern void dwc_otg_hc_start_transfer(dwc_otg_core_if_t *_core_if,
					dwc_hc_t *_hc);
extern int dwc_otg_hc_continue_transfer(dwc_otg_core_if_t *_core_if,
					dwc_hc_t *_hc);
extern void dwc_otg_hc_do_ping(dwc_otg_core_if_t *_core_if, dwc_hc_t *_hc);
extern void dwc_otg_hc_write_packet(dwc_otg_core_if_t *_core_if,
					dwc_hc_t *_hc);
extern void dwc_otg_enable_host_interrupts(dwc_otg_core_if_t *_core_if);
extern void dwc_otg_disable_host_interrupts(dwc_otg_core_if_t *_core_if);

/**
 * This function Reads HPRT0 in preparation to modify.	It keeps the
 * WC bits 0 so that if they are read as 1, they won't clear when you
 * write it back
 */
static inline uint32_t dwc_otg_read_hprt0(dwc_otg_core_if_t *_core_if)
{
	hprt0_data_t hprt0;
	hprt0.d32 = dwc_read_reg32(_core_if->host_if->hprt0);
	hprt0.b.prtena = 0;
	hprt0.b.prtconndet = 0;
	hprt0.b.prtenchng = 0;
	hprt0.b.prtovrcurrchng = 0;
	return hprt0.d32;
}

extern void dwc_otg_dump_host_registers(dwc_otg_core_if_t *_core_if);
/**@}*/

/** @name Common CIL Functions
 * The following functions support managing the DWC_otg controller in either
 * device or host mode.
 */
/**@{*/

extern void dwc_otg_read_packet(dwc_otg_core_if_t *core_if,
				uint8_t *dest,
				uint16_t bytes);

extern void dwc_otg_dump_global_registers(dwc_otg_core_if_t *_core_if);

extern void dwc_otg_flush_tx_fifo(dwc_otg_core_if_t *_core_if,
				const int _num);
extern void dwc_otg_flush_rx_fifo(dwc_otg_core_if_t *_core_if);
extern void dwc_otg_core_reset(dwc_otg_core_if_t *_core_if);

#define NP_TXFIFO_EMPTY -1
#define MAX_NP_TXREQUEST_Q_SLOTS 8
/**
 * This function returns the endpoint number of the request at
 * the top of non-periodic TX FIFO, or -1 if the request FIFO is
 * empty.
 */
static inline int dwc_otg_top_nptxfifo_epnum(dwc_otg_core_if_t *_core_if)
{
	gnptxsts_data_t txstatus = {.d32 = 0};

	txstatus.d32 = dwc_read_reg32(&_core_if->core_global_regs->gnptxsts);
	return txstatus.b.nptxqspcavail == MAX_NP_TXREQUEST_Q_SLOTS ?
		-1 : txstatus.b.nptxqtop_chnep;
}

/**
 * This function returns the Core Interrupt register.
 */
static inline uint32_t dwc_otg_read_core_intr(dwc_otg_core_if_t *_core_if)
{
	return dwc_read_reg32(&_core_if->core_global_regs->gintsts) &
		dwc_read_reg32(&_core_if->core_global_regs->gintmsk);
}

/**
 * This function returns the OTG Interrupt register.
 */
static inline uint32_t dwc_otg_read_otg_intr(dwc_otg_core_if_t *_core_if)
{
	return dwc_read_reg32(&_core_if->core_global_regs->gotgint);
}

/**
 * This function reads the Device All Endpoints Interrupt register and
 * returns the IN endpoint interrupt bits.
 */
static inline uint32_t dwc_otg_read_dev_all_in_ep_intr(dwc_otg_core_if_t
								*_core_if)
{
	uint32_t v;

	v = dwc_read_reg32(&_core_if->dev_if->dev_global_regs->daint) &
		dwc_read_reg32(&_core_if->dev_if->dev_global_regs->daintmsk);
	v &= 0xffff;
	return v;

}

/**
 * This function reads the Device All Endpoints Interrupt register and
 * returns the OUT endpoint interrupt bits.
 */
static inline uint32_t dwc_otg_read_dev_all_out_ep_intr(dwc_otg_core_if_t
								*_core_if)
{
	uint32_t v;
	v = dwc_read_reg32(&_core_if->dev_if->dev_global_regs->daint) &
		dwc_read_reg32(&_core_if->dev_if->dev_global_regs->daintmsk);
	v = (v & 0xffff0000) >> 16;
	return v;
}

/**
 * This function returns the Device IN EP Interrupt register
 */
static inline uint32_t dwc_otg_read_dev_in_ep_intr(dwc_otg_core_if_t *_core_if,
							dwc_ep_t *_ep)
{
	dwc_otg_dev_if_t *dev_if = _core_if->dev_if;
	uint32_t v, msk, emp;
	msk = dwc_read_reg32(&dev_if->dev_global_regs->diepmsk);
	emp = dwc_read_reg32(&dev_if->dev_global_regs->dtknqr4_fifoemptymsk);
	msk |= ((emp >> _ep->num) & 0x1) << 7;
	v = dwc_read_reg32(&dev_if->in_ep_regs[_ep->num]->diepint) & msk;

#if 0
	dwc_otg_dev_if_t *dev_if = _core_if->dev_if;
	uint32_t v;
	v = dwc_read_reg32(&dev_if->in_ep_regs[_ep->num]->diepint) &
		dwc_read_reg32(&dev_if->dev_global_regs->diepmsk);
#endif
	return v;
}
/**
 * This function returns the Device OUT EP Interrupt register
 */
static inline uint32_t dwc_otg_read_dev_out_ep_intr(dwc_otg_core_if_t *_core_if,
							dwc_ep_t *_ep)
{
	dwc_otg_dev_if_t *dev_if = _core_if->dev_if;
	uint32_t v;
	v = dwc_read_reg32(&dev_if->out_ep_regs[_ep->num]->doepint) &
			dwc_read_reg32(&dev_if->dev_global_regs->doepmsk);
	return v;
}

/**
 * This function returns the Host All Channel Interrupt register
 */
static inline uint32_t dwc_otg_read_host_all_channels_intr(dwc_otg_core_if_t
								*_core_if)
{
	return dwc_read_reg32(&_core_if->host_if->host_global_regs->haint);
}

static inline uint32_t dwc_otg_read_host_channel_intr(dwc_otg_core_if_t
							*_core_if,
							dwc_hc_t *_hc)
{
	return dwc_read_reg32(&_core_if->host_if->hc_regs[_hc->hc_num]->hcint);
}


/**
 * This function returns the mode of the operation, host or device.
 *
 * @return 0 - Device Mode, 1 - Host Mode
 */
static inline uint32_t dwc_otg_mode(dwc_otg_core_if_t *_core_if)
{
	return dwc_read_reg32(&_core_if->core_global_regs->gintsts) & 0x1;
}

static inline uint8_t dwc_otg_is_device_mode(dwc_otg_core_if_t *_core_if)
{
	return dwc_otg_mode(_core_if) != DWC_HOST_MODE;
}
static inline uint8_t dwc_otg_is_host_mode(dwc_otg_core_if_t *_core_if)
{
	return dwc_otg_mode(_core_if) == DWC_HOST_MODE;
}

extern int32_t dwc_otg_handle_common_intr(dwc_otg_core_if_t *_core_if);


/**
 * DWC_otg CIL callback structure.	This structure allows the HCD and
 * PCD to register functions used for starting and stopping the PCD
 * and HCD for role change on for a DRD.
 */
typedef struct dwc_otg_cil_callbacks {
	/** Start function for role change */
	int (*start)(void *_p);
	/** Stop Function for role change */
	int (*stop)(void *_p);
	/** Disconnect Function for role change */
	int (*disconnect)(void *_p);
	/** Resume/Remote wakeup Function */
	int (*resume_wakeup)(void *_p);
	/** Suspend function */
	int (*suspend)(void *_p);
	/** Session Start (SRP) */
	int (*session_start)(void *_p);
	/** Pointer passed to start() and stop() */
	void *p;
} dwc_otg_cil_callbacks_t;

extern void dwc_otg_cil_register_pcd_callbacks(dwc_otg_core_if_t *_core_if,
						dwc_otg_cil_callbacks_t *_cb,
						void *_p);
extern void dwc_otg_cil_register_hcd_callbacks(dwc_otg_core_if_t *_core_if,
						dwc_otg_cil_callbacks_t *_cb,
						void *_p);

#endif /* __DWC_CIL_H__ */

