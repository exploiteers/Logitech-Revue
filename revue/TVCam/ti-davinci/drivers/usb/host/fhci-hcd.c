/*
 * Freescale QUICC Engine USB Host Controller Driver
 *
 * Copyright (c) Freescale Semicondutor, Inc. 2006.
 *               Shlomi Gridish <gridish@freescale.com>
 *               Jerry Huang <Chang-Ming.Huang@freescale.com>
 * Copyright (c) Logic Product Development, Inc. 2007
 *               Peter Barada <peterb@logicpd.com>
 * Copyright (c) MontaVista Software, Inc. 2008.
 *               Anton Vorontsov <avorontsov@ru.mvista.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#if defined(CONFIG_FHCI_DEBUG) && !defined(DEBUG)
#define DEBUG
#endif

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/usb.h>
#include <asm/of_platform.h>
#include <asm/qe.h>
#include <asm/gpio.h>
#include "../core/hcd.h"

#include "fhci.h"
#include "fhci-hub.c"
#include "fhci-q.c"
#include "fhci-dbg.c"
#include "fhci-mem.c"
#include "fhci-cq.c"
#if defined(CONFIG_FHCI_WITH_BDS)
#include "fhci-bds.c"
#elif defined(CONFIG_FHCI_WITH_TDS)
#include "fhci-tds.c"
#endif

static void recycle_frame(struct fhci_usb *usb, struct packet *pkt)
{
	pkt->data = NULL;
	pkt->len = 0;
	pkt->status = USB_TD_OK;
	pkt->info = 0;
	pkt->priv_data = NULL;

	cq_put(usb->ep0->empty_frame_Q, pkt);
}

static void *get_empty_frame(struct fhci_usb *usb)
{
	return cq_get(usb->ep0->empty_frame_Q);
}

/* confirm submitted packet */
static void transaction_confirm(struct fhci_usb *usb, struct packet *pkt)
{
	struct td *td;
	struct packet *td_pkt;
	struct ed *ed;
	u32 trans_len;
	bool td_done = false;

	td = remove_td_from_frame(usb->actual_frame);
	td_pkt = td->pkt;
	trans_len = pkt->len;
	td->status = pkt->status;
	if (td->type == FHCI_TA_IN && td_pkt->info & PKT_DUMMY_PACKET) {
		if (((u32) td->data + td->actual_len) && trans_len)
			memcpy(td->data + td->actual_len, pkt->data,
			       trans_len);
		cq_put(usb->ep0->dummy_packets_Q, pkt->data);
	}

	recycle_frame(usb, pkt);

	ed = td->ed;
	if (ed->mode == FHCI_TF_ISO) {
		if (ed->td_list.next->next != &ed->td_list) {
			struct td *td_next =
			    list_entry(ed->td_list.next->next, struct td,
				       node);

			td_next->start_frame = usb->actual_frame->frame_num;
		}
		td->actual_len = trans_len;
		td_done = true;
	} else if ((td->status & USB_TD_ERROR) &&
			!(td->status & USB_TD_TX_ER_NAK)) {
		/*
		 * There was an error on the transaction (but not NAK).
		 * If it is fatal error (data underrun, stall, bad pid or 3
		 * errors exceeded), mark this TD as done.
		 */
		if ((td->status & USB_TD_RX_DATA_UNDERUN) ||
				(td->status & USB_TD_TX_ER_STALL) ||
				(td->status & USB_TD_RX_ER_PID) ||
				(++td->error_cnt >= 3)) {
			ed->state = FHCI_ED_HALTED;
			td_done = true;

			if (td->status & USB_TD_RX_DATA_UNDERUN) {
				td->toggle = !td->toggle;
				td->actual_len += trans_len;
			}
		} else {
			/* it is not a fatal error -retry this transaction */
			td->nak_cnt = 0;
			td->error_cnt++;
			td->status = USB_TD_OK;
		}
	} else if (td->status & USB_TD_TX_ER_NAK) {
		/* there was a NAK response */
		td->nak_cnt++;
		td->error_cnt = 0;
		td->status = USB_TD_OK;
	} else {
		/* there was no error on transaction */
		td->error_cnt = 0;
		td->nak_cnt = 0;
		td->toggle = !td->toggle;
		td->actual_len += trans_len;

		if (td->len == td->actual_len)
			td_done = true;
	}

	if (td_done)
		move_td_from_ed_to_done_list(usb, ed);
}

static void qe_usb_stop_tx(u8 ep)
{
	qe_issue_cmd(QE_USB_STOP_TX, QE_CR_SUBBLOCK_USB, ep, 0);
}

static void qe_usb_restart_tx(u8 ep)
{
	qe_issue_cmd(QE_USB_RESTART_TX, QE_CR_SUBBLOCK_USB, ep, 0);
}

/* Cancel transmission on the USB endpoint*/
static void abort_transmission(struct fhci_usb *usb)
{
	/* issue stop Tx command */
	qe_usb_stop_tx(EP_ZERO);
	/* flush Tx FIFOs */
	out_8(&usb->fhci->regs->usb_comm, USB_CMD_FLUSH_FIFO | EP_ZERO);
	udelay(1000);
	/* reset Tx BDs */
	flush_bds(usb);
	/* issue restart Tx command */
	qe_usb_restart_tx(EP_ZERO);
}

/*
 * Flush all transmitted packets from BDs
 * This routine is called when disabling the USB port to flush all
 * transmissions that are allready scheduled in the BDs
 */
static void flush_all_transmissions(struct fhci_usb *usb)
{
	u8 mode;
	struct td *td;

	mode = in_8(&usb->fhci->regs->usb_mod);
	clrbits8(&usb->fhci->regs->usb_mod, USB_MODE_EN);

	flush_bds(usb);

	while ((td = peek_td_from_frame(usb->actual_frame)) != NULL) {
		struct packet *pkt = td->pkt;

		pkt->status = USB_TD_TX_ER_TIMEOUT;
		transaction_confirm(usb, pkt);
	}

	usb->actual_frame->frame_status = FRAME_END_TRANSMISSION;

	/* reset the event register */
	out_be16(&usb->fhci->regs->usb_event, 0xffff);
	/* enable the USB controller */
	out_8(&usb->fhci->regs->usb_mod, mode | USB_MODE_EN);
}

/*
 * This function forms the packet and transmit the packet. This function
 * will handle all endpoint type:ISO,interrupt,control and bulk
 */
static int add_packet(struct fhci_usb *usb, struct ed *ed,
			    struct td *td)
{
	u32 fw_transaction_time, len = 0;
	struct packet *pkt;
	u8 *data = NULL;

	/* calcalate data address,len and toggle and then add the transaction */
	if (td->toggle == USB_TD_TOGGLE_CARRY)
		td->toggle = ed->toggle_carry;

	switch (ed->mode) {
	case FHCI_TF_ISO:
		len = td->len;
		if (td->type != FHCI_TA_IN)
			data = td->data;
		break;
	case FHCI_TF_CTRL:
	case FHCI_TF_BULK:
		len = min(td->len - td->actual_len, ed->max_pkt_size);
		if (!((td->type == FHCI_TA_IN) &&
		      ((len + td->actual_len) == td->len)))
			data = td->data + td->actual_len;
		break;
	case FHCI_TF_INTR:
		len = min(td->len, ed->max_pkt_size);
		if (!((td->type == FHCI_TA_IN) &&
		      ((td->len + CRC_SIZE) >= ed->max_pkt_size)))
			data = td->data;
	default:
		break;
	}

	if (usb->port_status == FHCI_PORT_FULL)
		fw_transaction_time = (((len + PROTOCOL_OVERHEAD) * 11) >> 4);
	else
		fw_transaction_time = ((len + PROTOCOL_OVERHEAD) * 6);

	/* check if there's enough space in this frame to submit this TD */
	if (usb->actual_frame->total_bytes + len + PROTOCOL_OVERHEAD >=
			usb->max_bytes_per_frame)
		return -1;

	/* check if there's enough time in this frame to submit this TD */
	if (usb->actual_frame->frame_status != FRAME_IS_PREPARED &&
	    (usb->actual_frame->frame_status & FRAME_END_TRANSMISSION ||
	     (fw_transaction_time + usb->sw_transaction_time >=
	      1000 - get_sof_timer_count(usb))))
		return -1;

	/* update frame object fields before transmitting */
	pkt = get_empty_frame(usb);
	if (!pkt)
		return -1;
	td->pkt = pkt;

	pkt->info = 0;
	if (data == NULL) {
		data = cq_get(usb->ep0->dummy_packets_Q);
		BUG_ON(!data);
		pkt->info = PKT_DUMMY_PACKET;
	}
	pkt->data = data;
	pkt->len = len;
	pkt->status = USB_TD_OK;
	/* update TD status field before transmitting */
	td->status = USB_TD_INPROGRESS;
	/* update actual frame time object with the actual transmission */
	usb->actual_frame->total_bytes += (len + PROTOCOL_OVERHEAD);
	add_td_to_frame(usb->actual_frame, td);

	if (usb->port_status != FHCI_PORT_FULL &&
			usb->port_status != FHCI_PORT_LOW) {
		pkt->status = USB_TD_TX_ER_TIMEOUT;
		pkt->len = 0;
		transaction_confirm(usb, pkt);
	} else if (host_transaction(usb, pkt, td->type, ed->dev_addr,
			ed->ep_addr, ed->mode, ed->speed, td->toggle)) {
		/* remove TD from actual frame */
		list_del_init(&td->frame_lh);
		td->status = USB_TD_OK;
		if (pkt->info & PKT_DUMMY_PACKET)
			cq_put(usb->ep0->dummy_packets_Q, pkt->data);
		recycle_frame(usb, pkt);
		usb->actual_frame->total_bytes -= (len + PROTOCOL_OVERHEAD);
		return -1;
	}

	return len;
}

/*
 * This function goes through the endpoint list and schedules the
 * transactions within this list
 */
static int scan_ed_list(struct fhci_usb *usb,
			struct list_head *list, enum fhci_tf_mode list_type)
{
	static const int frame_part[4] = {
		[FHCI_TF_CTRL] = MAX_BYTES_PER_FRAME,
		[FHCI_TF_ISO] = (MAX_BYTES_PER_FRAME *
				 MAX_PERIODIC_FRAME_USAGE) / 100,
		[FHCI_TF_BULK] = MAX_BYTES_PER_FRAME,
		[FHCI_TF_INTR] = (MAX_BYTES_PER_FRAME *
				  MAX_PERIODIC_FRAME_USAGE) / 100
	};
	struct list_head *ed_lh = NULL;
	struct ed *ed;
	struct td *td;
	int ans = 1;
	u32 save_transaction_time = usb->sw_transaction_time;

	list_for_each(ed_lh, list) {
		ed = list_entry(ed_lh, struct ed, node);
		td = ed->td_head;

		if (td != NULL && td->status != USB_TD_INPROGRESS) {
			if (ed->state != FHCI_ED_OPER) {
				if (ed->state == FHCI_ED_URB_DEL) {
					td->status = USB_TD_OK;
					move_td_from_ed_to_done_list(usb, ed);
					ed->state = FHCI_ED_SKIP;
				}
			}
			/*
			 * if it isn't interrupt pipe or it is not iso pipe
			 * and the interval time passed
			 */
			else if (!(list_type == FHCI_TF_INTR ||
				   list_type == FHCI_TF_ISO) ||
				 (((usb->actual_frame->frame_num -
				    td->start_frame) & 0x7ff) >=
				  td->interval)) {
				if (add_packet(usb, ed, td) >= 0) {
					/* update time stamps in the TD */
					td->start_frame =
					    usb->actual_frame->frame_num;
					usb->sw_transaction_time +=
					    save_transaction_time;

					if (usb->actual_frame->total_bytes >=
					    usb->max_bytes_per_frame) {
						usb->actual_frame->
						    frame_status =
						    FRAME_DATA_END_TRANSMISSION;
						push_dummy_bd(usb->ep0);
						ans = 0;
						break;
					}
					if (usb->actual_frame->total_bytes >=
							frame_part[list_type])
						break;
				}
			}
		}
	}

	/* be fair to each ED(move list head around) */
	move_head_to_tail(list);
	usb->sw_transaction_time = save_transaction_time;

	return ans;
}

static u32 rotate_frames(struct fhci_usb *usb)
{
	struct fhci_hcd *fhci = usb->fhci;

	if (!list_empty(&usb->actual_frame->tds_list)) {
		if ((((in_be16(&fhci->pram->frame_num) & 0x07ff) -
		      usb->actual_frame->frame_num) & 0x7ff) > 5)
			flush_actual_frame(usb);
		else
			return -EINVAL;
	}

	usb->actual_frame->frame_status = FRAME_IS_PREPARED;
	usb->actual_frame->frame_num = in_be16(&fhci->pram->frame_num) & 0x7ff;
	usb->actual_frame->total_bytes = 0;

	return 0;
}

/*
 * This function schedule the USB transaction and will process the
 * endpoint in the following order: iso, interrupt, control and bulk.
 */
static void schedule_transactions(struct fhci_usb *usb)
{
	int left = 1;

	if (usb->actual_frame->frame_status & FRAME_END_TRANSMISSION)
		if (rotate_frames(usb) != 0)
			return;

	if (usb->actual_frame->frame_status & FRAME_END_TRANSMISSION)
		return;

	if (usb->actual_frame->total_bytes == 0) {
		/* schedule the next available ISO transfer
		 *or next stage of the ISO transfer*/
		scan_ed_list(usb, &usb->hc_list->iso_list, FHCI_TF_ISO);

		/*
		 * schedule the next available interrupt transfer or
		 * the next stage of the interrupt transfer
		 */
		scan_ed_list(usb, &usb->hc_list->intr_list, FHCI_TF_INTR);

		/*
		 * schedule the next available control transfer
		 * or the next stage of the control transfer
		 */
		left =
		    scan_ed_list(usb, &usb->hc_list->ctrl_list, FHCI_TF_CTRL);
	}

	/*
	 * schedule the next available bulk transfer or the next stage of the
	 * bulk transfer
	 */
	if (left > 0)
		scan_ed_list(usb, &usb->hc_list->bulk_list, FHCI_TF_BULK);
}

/* initialize the endpoint zero */
static u32 endpoint_zero_init(struct fhci_usb *usb,
			      enum fhci_mem_alloc data_mem,
			      u32 ring_len)
{
	u32 rc;

	rc = create_endpoint(usb, data_mem, ring_len);
	if (rc)
		return rc;

	/* inilialize endpoint registers */
	init_endpoint_registers(usb, usb->ep0, data_mem);

	return 0;
}

/* enable the USB interrupts */
static void fhci_usb_enable_interrupt(struct fhci_usb *usb)
{
	struct fhci_hcd *fhci = usb->fhci;

	if (usb->intr_nesting_cnt == 1) {
		/* initialize the USB interrupt */
		enable_irq(fhci_to_hcd(fhci)->irq);

		/* initialize the event register and mask register */
		out_be16(&usb->fhci->regs->usb_event, 0xffff);
		out_be16(&usb->fhci->regs->usb_mask, usb->saved_msk);

		/* enable the timer interrupts */
		enable_irq(fhci->timer_irq);
	} else if (usb->intr_nesting_cnt > 1)
		fhci_info(fhci, "unbalanced USB interrupts nesting\n");
	usb->intr_nesting_cnt--;
}

/* diable the usb interrupt */
static void fhci_usb_disable_interrupt(struct fhci_usb *usb)
{
	struct fhci_hcd *fhci = usb->fhci;

	if (usb->intr_nesting_cnt == 0) {
		/* diable the timer interrupt */
		disable_irq(fhci->timer_irq);

		/* disable the usb interrupt */
		disable_irq(fhci_to_hcd(fhci)->irq);
		out_be16(&usb->fhci->regs->usb_mask, 0);
	}
	usb->intr_nesting_cnt++;
}

/* enable the USB controller */
static u32 fhci_usb_enable(struct fhci_hcd *fhci)
{
	struct fhci_usb *usb = fhci->usb_lld;

	out_be16(&usb->fhci->regs->usb_event, 0xffff);
	out_be16(&usb->fhci->regs->usb_mask, usb->saved_msk);
	setbits8(&usb->fhci->regs->usb_mod, USB_MODE_EN);

	mdelay(100);

	return 0;
}

/* disable the USB controller */
static u32 fhci_usb_disable(struct fhci_hcd *fhci)
{
	struct fhci_usb *usb = fhci->usb_lld;

	fhci_usb_disable_interrupt(usb);
	usb_port_disable(fhci);

	/* disable the usb controller */
	if (usb->port_status == FHCI_PORT_FULL ||
			usb->port_status == FHCI_PORT_LOW)
		device_disconnected_interrupt(fhci);

	clrbits8(&usb->fhci->regs->usb_mod, USB_MODE_EN);

	return 0;
}

/* check the bus state by polling the QE bit on the IO ports */
static int fhci_ioports_check_bus_state(struct fhci_hcd *fhci)
{
	u8 bits = 0;

	/* check USBOE,if transmitting,exit */
	if (!gpio_get_value(fhci->gpios[GPIO_USBOE]))
		return -1;

	/* check USBRP */
	if (gpio_get_value(fhci->gpios[GPIO_USBRP]))
		bits |= 0x2;

	/* check USBRN */
	if (gpio_get_value(fhci->gpios[GPIO_USBRN]))
		bits |= 0x1;

	return bits;
}

/* Handles SOF interrupt */
static void sof_interrupt(struct fhci_hcd *fhci)
{
	struct fhci_usb *usb = fhci->usb_lld;

	if ((usb->port_status == FHCI_PORT_DISABLED) &&
	    (usb->vroot_hub->port.wPortStatus & USB_PORT_STAT_CONNECTION) &&
	    !(usb->vroot_hub->port.wPortChange & USB_PORT_STAT_C_CONNECTION)) {
		if (usb->vroot_hub->port.wPortStatus & USB_PORT_STAT_LOW_SPEED)
			usb->port_status = FHCI_PORT_LOW;
		else
			usb->port_status = FHCI_PORT_FULL;
		/* Disable IDLE */
		usb->saved_msk &= ~USB_E_IDLE_MASK;
		out_be16(&usb->fhci->regs->usb_mask, usb->saved_msk);
	}

	qe_reset_ref_timer_16(fhci->timer, 2000000, usb->max_frame_usage);

	host_transmit_actual_frame(usb);
	usb->actual_frame->frame_status = FRAME_IS_TRANSMITTED;

	schedule_transactions(usb);
}

/* Handles device disconnected interrupt on port */
static void device_disconnected_interrupt(struct fhci_hcd *fhci)
{
	struct fhci_usb *usb = fhci->usb_lld;

	fhci_dbg(fhci, "-> %s\n", __func__);

	fhci_usb_disable_interrupt(usb);
	clrbits8(&usb->fhci->regs->usb_mod, USB_MODE_LSS);
	usb->port_status = FHCI_PORT_DISABLED;

	/* Enable IDLE since we want to know if something comes along */
	usb->saved_msk |= USB_E_IDLE_MASK;
	out_be16(&usb->fhci->regs->usb_mask, usb->saved_msk);

	usb->vroot_hub->port.wPortStatus &= ~USB_PORT_STAT_CONNECTION;
	usb->vroot_hub->port.wPortChange |= USB_PORT_STAT_C_CONNECTION;
	usb->max_bytes_per_frame = 0;
	fhci_usb_enable_interrupt(usb);

	fhci_dbg(fhci, "<- %s\n", __func__);
}

/* detect a new device connected on the USB port*/
static void device_connected_interrupt(struct fhci_hcd *fhci)
{

	struct fhci_usb *usb = fhci->usb_lld;
	int state;
	int ret;

	fhci_dbg(fhci, "-> %s\n", __func__);

	fhci_usb_disable_interrupt(usb);
	state = fhci_ioports_check_bus_state(fhci);

	/* low-speed device was connected to the USB port */
	if (state == 1) {
		ret = qe_usb_clock_set(fhci->lowspeed_clk, USB_CLOCK >> 3);
		if (ret) {
			fhci_info(fhci, "Low-Speed device is not supported, "
				  "try use BRGx\n");
			goto out;
		}

		usb->port_status = FHCI_PORT_LOW;
		setbits8(&usb->fhci->regs->usb_mod, USB_MODE_LSS);
		usb->vroot_hub->port.wPortStatus |=
		    (USB_PORT_STAT_LOW_SPEED |
		     USB_PORT_STAT_CONNECTION);
		usb->vroot_hub->port.wPortChange |=
		    USB_PORT_STAT_C_CONNECTION;
		usb->max_bytes_per_frame =
		    (MAX_BYTES_PER_FRAME >> 3) - 7;
		usb_port_enable(usb);
	} else if (state == 2) {
		ret = qe_usb_clock_set(fhci->fullspeed_clk, USB_CLOCK);
		if (ret) {
			fhci_info(fhci, "Full-Speed device is not supported, "
				  "try use CLKx\n");
			goto out;
		}

		usb->port_status = FHCI_PORT_FULL;
		clrbits8(&usb->fhci->regs->usb_mod, USB_MODE_LSS);
		usb->vroot_hub->port.wPortStatus &=
		    ~USB_PORT_STAT_LOW_SPEED;
		usb->vroot_hub->port.wPortStatus |=
		    USB_PORT_STAT_CONNECTION;
		usb->vroot_hub->port.wPortChange |=
		    USB_PORT_STAT_C_CONNECTION;
		usb->max_bytes_per_frame = (MAX_BYTES_PER_FRAME - 15);
		usb_port_enable(usb);
	}
out:
	fhci_usb_enable_interrupt(usb);
	fhci_dbg(fhci, "<- %s\n", __func__);
}

static irqreturn_t fhci_frame_limit_timer_irq(int irq, void *_hcd,
					      struct pt_regs *regs)
{
	struct usb_hcd *hcd = _hcd;
	struct fhci_hcd *fhci = hcd_to_fhci(hcd);
	struct fhci_usb *usb = fhci->usb_lld;
	
	_raw_spin_lock(&fhci->lock);

	qe_reset_ref_timer_16(fhci->timer, 2000000, 1000);

	if (usb->actual_frame->frame_status == FRAME_IS_TRANSMITTED) {
		usb->actual_frame->frame_status = FRAME_TIMER_END_TRANSMISSION;
		push_dummy_bd(usb->ep0);
	}

	schedule_transactions(usb);

	_raw_spin_unlock(&fhci->lock);

	return IRQ_HANDLED;
}



static irqreturn_t fhci_irq(struct usb_hcd *hcd, struct pt_regs *regs)
{
	struct fhci_hcd *fhci = hcd_to_fhci(hcd);
	struct fhci_usb *usb;
	u16 usb_er = 0;

	_raw_spin_lock(&fhci->lock);

	usb = fhci->usb_lld;

	usb->intr_counter++;
	do {
		usb_er |= in_be16(&usb->fhci->regs->usb_event) &
			  in_be16(&usb->fhci->regs->usb_mask);

		/* clear event bits for next time */
		out_be16(&usb->fhci->regs->usb_event, usb_er);

		fhci_dbg_isr(fhci, usb_er);

		if (usb_er & USB_E_RESET_MASK) {
			if ((usb->port_status == FHCI_PORT_FULL) ||
			    (usb->port_status == FHCI_PORT_LOW)) {
				device_disconnected_interrupt(fhci);
				usb_er &= ~USB_E_IDLE_MASK;
			} else if (usb->port_status ==
				   FHCI_PORT_WAITING) {
				usb->port_status = FHCI_PORT_DISCONNECTING;

				/* Turn on IDLE since we want to disconnect */
				usb->saved_msk |= USB_E_IDLE_MASK;
				out_be16(&usb->fhci->regs->usb_event,
					 usb->saved_msk);
			} else if (usb->port_status == FHCI_PORT_DISABLED) {
				if (fhci_ioports_check_bus_state(fhci) == 1 &&
						usb->port_status != FHCI_PORT_LOW &&
						usb->port_status != FHCI_PORT_FULL)
					device_connected_interrupt(fhci);
			}
			usb_er &= ~USB_E_RESET_MASK;
		}

		if (usb_er & USB_E_MSF_MASK) {
			abort_transmission(fhci->usb_lld);
			usb_er &= ~USB_E_MSF_MASK;
		}

		if (usb_er & (USB_E_SOF_MASK | USB_E_SFT_MASK)) {
			sof_interrupt(fhci);
			usb_er &= ~(USB_E_SOF_MASK | USB_E_SFT_MASK);
		}
#ifdef CONFIG_FHCI_WITH_BDS
		if (usb_er & USB_E_RXB_MASK) {
			if (receive_packet_interrupt(fhci) == 0)
				usb_er &= ~USB_E_RXB_MASK;
		}
#endif /* CONFIG_FHCI_WITH_BDS */

		if (usb_er & USB_E_TXB_MASK) {
			tx_conf_interrupt(fhci->usb_lld);
			usb_er &= ~USB_E_TXB_MASK;
#ifdef CONFIG_FHCI_WITH_BDS
			continue;
#endif /* CONFIG_FHCI_WITH_BDS */
		}

		if (usb_er & USB_E_TXE1_MASK) {
			tx_conf_interrupt(fhci->usb_lld);
			usb_er &= ~USB_E_TXE1_MASK;
#ifdef CONFIG_FHCI_WITH_BDS
			continue;
#endif /* CONFIG_FHCI_WITH_BDS */
		}

		if (usb_er & USB_E_IDLE_MASK) {
			if (usb->port_status == FHCI_PORT_DISABLED &&
					usb->port_status != FHCI_PORT_LOW &&
					usb->port_status != FHCI_PORT_FULL) {
				usb_er &= ~USB_E_RESET_MASK;
				device_connected_interrupt(fhci);
			} else if (usb->port_status ==
					FHCI_PORT_DISCONNECTING) {
				//usb->port_status = FHCI_PORT_WAITING;

				/* Disable IDLE */
				usb->saved_msk &= ~USB_E_IDLE_MASK;
				out_be16(&usb->fhci->regs->usb_mask,
					 usb->saved_msk);
			} else
				fhci_dbg_isr(fhci, -1);

			usb_er &= ~USB_E_IDLE_MASK;
		}
	} while (usb_er);

	_raw_spin_unlock(&fhci->lock);

	return IRQ_HANDLED;
}

static void fhci_mem_free(struct fhci_hcd *fhci)
{
	struct td *td;
	struct ed *ed;

	while (!list_empty(&fhci->empty_eds)) {
		ed = list_entry(fhci->empty_eds.next, struct ed, node);
		list_del(fhci->empty_eds.next);
	}

	while (!list_empty(&fhci->empty_tds)) {
		td = list_entry(fhci->empty_tds.next, struct td, node);
		list_del(fhci->empty_tds.next);
	}

	kfree(fhci->vroot_hub);
	kfree(fhci->hc_list);
}

static DECLARE_TASKLET(fhci_tasklet, process_done_list, 0);

static int fhci_mem_init(struct fhci_hcd *fhci)
{
	int i, error = 0;

	fhci->hc_list = kzalloc(sizeof(*fhci->hc_list), GFP_KERNEL);
	if (!fhci->hc_list)
		return -ENOMEM;

	INIT_LIST_HEAD(&fhci->hc_list->ctrl_list);
	INIT_LIST_HEAD(&fhci->hc_list->bulk_list);
	INIT_LIST_HEAD(&fhci->hc_list->iso_list);
	INIT_LIST_HEAD(&fhci->hc_list->intr_list);
	INIT_LIST_HEAD(&fhci->hc_list->done_list);

	fhci->vroot_hub = kzalloc(sizeof(*fhci->vroot_hub), GFP_KERNEL);
	if (!fhci->vroot_hub)
		return -ENOMEM;


	INIT_LIST_HEAD(&fhci->empty_eds);
	INIT_LIST_HEAD(&fhci->empty_tds);

	/* initialize work queue to handle done list */
	fhci_tasklet.data = (unsigned long)fhci;
	fhci->process_done_task = &fhci_tasklet;

	for (i = 0; i < MAX_TDS; i++) {
		struct td *td = kmalloc(sizeof(*td), GFP_KERNEL);

		if (!td) {
			error = 1;
			break;
		}
		recycle_empty_td(fhci, td);
	}
	for (i = 0; i < MAX_EDS; i++) {
		struct ed *ed = kmalloc(sizeof(*ed), GFP_KERNEL);

		if (!ed) {
			error = 1;
			break;
		}
		recycle_empty_ed(fhci, ed);
	}

	if (error) {
		fhci_mem_free(fhci);
		return -ENOMEM;
	}

	fhci->active_urbs = 0;

	return error;
}

/* transfer complted callback*/
static u32 transfer_confirm_callback(struct fhci_hcd *fhci)
{
	if (!fhci->process_done_task->state)
		tasklet_schedule(fhci->process_done_task);
	return 0;
}

/* destroy the fhci_usb structure */
static void fhci_usb_free(void *lld)
{
	struct fhci_usb *usb = lld;
	struct fhci_hcd *fhci = usb->fhci;

	if (usb) {
		config_transceiver(fhci, FHCI_OP_POWER_OFF);
		config_transceiver(fhci, FHCI_OP_DISCONNECT);

		endpoint_zero_free(usb);
#if defined(CONFIG_FHCI_HAS_NO_RT_SOF)
		kfree(usb->sof_pkts);
#elif defined(CONFIG_FHCI_HAS_UC_RT_SOF)
		u8 *tmp_crc5_pkts = phys_to_virt(usb->pram->sof_tbl);
		kfree(tmp_crc5_pkts);
#endif
		kfree(usb->actual_frame);
		kfree(usb);
	}
}

/* initialize the USB*/
static u32 fhci_usb_init(struct fhci_hcd *fhci)
{
	struct fhci_usb *usb = fhci->usb_lld;

	memset_io(usb->fhci->pram, 0, FHCI_PRAM_SIZE);

	usb->port_status = FHCI_PORT_DISABLED;
	usb->max_frame_usage = FRAME_TIME_USAGE;
#ifdef CONFIG_FHCI_WITH_BDS
	/* the predefined time limitation are for the driver with
	 * transcation level interface, so we have to take about 2 times
	 */
	usb->sw_transaction_time = SW_FIX_TIME_BETWEEN_TRANSACTION * 2;
#else
	usb->sw_transaction_time = SW_FIX_TIME_BETWEEN_TRANSACTION;
#endif

	usb->actual_frame = kzalloc(sizeof(*usb->actual_frame), GFP_KERNEL);
	if (!usb->actual_frame) {
		fhci_usb_free(usb);
		return -ENOMEM;
	}

	INIT_LIST_HEAD(&usb->actual_frame->tds_list);

	/* initializing registers on chip, clear frame number */
	out_be16(&fhci->pram->frame_num, 0);

	/* clear rx state */
	out_be32(&fhci->pram->rx_state, 0);

	/* set mask register */
	usb->saved_msk = (USB_E_TXB_MASK |
			  USB_E_TXE1_MASK |
			  USB_E_IDLE_MASK |
			  USB_E_RESET_MASK | USB_E_SFT_MASK | USB_E_MSF_MASK);

#ifdef CONFIG_FHCI_WITH_BDS
	usb->saved_msk |= USB_E_RXB_MASK;
#endif

	out_8(&usb->fhci->regs->usb_mod, USB_MODE_HOST | USB_MODE_EN);

	/* clearing the mask register */
	out_be16(&usb->fhci->regs->usb_mask, 0);

	/* initialing the event register */
	out_be16(&usb->fhci->regs->usb_event, 0xffff);

	if (endpoint_zero_init(usb, DEFAULT_DATA_MEM, DEFAULT_RING_LEN) != 0) {
		fhci_usb_free(usb);
		return -EINVAL;
	}

	return 0;
}

/* initialize the fhci_usb struct and the corresponding data staruct*/
static struct fhci_usb *fhci_create_lld(struct fhci_hcd *fhci)
{
	struct fhci_usb *usb;

	/* allocate memory for SCC data structure */
	usb = kzalloc(sizeof(*usb), GFP_KERNEL);
	if (!usb) {
		fhci_err(fhci, "no memory for SCC data struct\n");
		return NULL;
	}

	usb->fhci = fhci;
	usb->hc_list = fhci->hc_list;
	usb->vroot_hub = fhci->vroot_hub;

	usb->transfer_confirm = transfer_confirm_callback;

	return usb;
}

static int fhci_start(struct usb_hcd *hcd)
{
	int ret;
	struct fhci_hcd *fhci = hcd_to_fhci(hcd);

	ret = fhci_mem_init(fhci);

	fhci->usb_lld = fhci_create_lld(fhci);
	if (!fhci->usb_lld) {
		fhci_err(fhci, "low level driver config failed\n");
		fhci_mem_free(fhci);
		return -ENODEV;
	}
	if (fhci_usb_init(fhci)) {
		fhci_err(fhci, "low level driver initialize failed\n");
		fhci_mem_free(fhci);
		return -ENODEV;
	}
	_raw_spin_lock_init(&fhci->lock);

	/* connect the virtual root hub */
	fhci->vroot_hub->dev_num = 1;	/* this field may be needed to fix */
	fhci->vroot_hub->hub.wHubStatus = 0;
	fhci->vroot_hub->hub.wHubChange = 0;
	fhci->vroot_hub->port.wPortStatus = 0;
	fhci->vroot_hub->port.wPortChange = 0;

	hcd->state = HC_STATE_RUNNING;

	/*
	 * From here on, khubd concurrently accesses the root
	 * hub; drivers will be talking to enumerated devices.
	 * (On restart paths, khubd already knows about the root
	 * hub and could find work as soon as we wrote FLAG_CF.)
	 *
	 * Before this point the HC was idle/ready.  After, khubd
	 * and device drivers may start it running.
	 */
	fhci_usb_enable(fhci);

	return 0;
}

static void fhci_stop(struct usb_hcd *hcd)
{
	struct fhci_hcd *fhci = hcd_to_fhci(hcd);

	fhci_usb_disable_interrupt(fhci->usb_lld);
	fhci_usb_disable(fhci);

	fhci_usb_free(fhci->usb_lld);
	fhci->usb_lld = NULL;
	fhci_mem_free(fhci);
}

static int fhci_urb_enqueue(struct usb_hcd *hcd, struct usb_host_endpoint *ep,
			    struct urb *urb, gfp_t mem_flags)
{
	struct fhci_hcd *fhci = hcd_to_fhci(hcd);
	u32 pipe = urb->pipe;
	int i, size = 0;
	struct urb_priv *urb_priv;
	unsigned long flags;

	switch (usb_pipetype(pipe)) {
	case PIPE_CONTROL:
		/* 1 td fro setup,1 for ack */
		size = 2;
	case PIPE_BULK:
		/* one td for every 4096 bytes(can be upto 8k) */
		size += urb->transfer_buffer_length / 4096;
		/* ...add for any remaining bytes... */
		if ((urb->transfer_buffer_length % 4096) != 0)
			size++;
		/* ..and maybe a zero length packet to wrap it up */
		if (size == 0)
			size++;
		else if ((urb->transfer_flags & URB_ZERO_PACKET) != 0
			 && (urb->transfer_buffer_length
			     % usb_maxpacket(urb->dev, pipe,
					     usb_pipeout(pipe))) != 0)
			size++;
		break;
	case PIPE_ISOCHRONOUS:
		size = urb->number_of_packets;
		if (size <= 0)
			return -EINVAL;
		for (i = 0; i < urb->number_of_packets; i++) {
			urb->iso_frame_desc[i].actual_length = 0;
			urb->iso_frame_desc[i].status = (u32) (-EXDEV);
		}
		break;
	case PIPE_INTERRUPT:
		size = 1;
	}

	/* allocate the private part of the URB */
	urb_priv = kzalloc(sizeof(*urb_priv), mem_flags);
	if (!urb_priv)
		return -ENOMEM;

	/* allocate the private part of the URB */
	urb_priv->tds = kzalloc(size * sizeof(struct td), mem_flags);
	if (!urb_priv->tds) {
		kfree(urb_priv);
		return -ENOMEM;
	}

	local_irq_save(flags);
	_raw_spin_lock(&fhci->lock);
	/* fill the private part of the URB */
	urb_priv->num_of_tds = size;

	urb->status = -EINPROGRESS;
	urb->actual_length = 0;
	urb->error_count = 0;
	urb->hcpriv = urb_priv;

	queue_urb(fhci, urb, ep);

	_raw_spin_unlock(&fhci->lock);
	local_irq_restore(flags);
	return 0;
}

/* dequeue FHCI URB */
static int fhci_urb_dequeue(struct usb_hcd *hcd, struct urb *urb)
{
	struct fhci_hcd *fhci = hcd_to_fhci(hcd);
	struct fhci_usb *usb = fhci->usb_lld;
	unsigned long flags;

	if (!urb || !urb->dev || !urb->dev->bus)
		goto out;

	local_irq_save(flags);
	_raw_spin_lock(&fhci->lock);

	if (usb->port_status != FHCI_PORT_DISABLED) {
		struct urb_priv *urb_priv;

		/*
		 * flag the urb's data for deletion in some upcoming
		 * SF interrupt's delete list processing
		 */
		urb_priv = urb->hcpriv;

		if (!urb_priv || (urb_priv->state == URB_DEL))
			goto out2;

		urb_priv->state = URB_DEL;

		/* already pending? */
		urb_priv->ed->state = FHCI_ED_URB_DEL;
	} else
		urb_complete_free(fhci, urb);

out2:
	_raw_spin_unlock(&fhci->lock);
	local_irq_restore(flags);
out:
	return 0;
}

static void fhci_endpoint_disable(struct usb_hcd *hcd,
				  struct usb_host_endpoint *ep)
{
	struct fhci_hcd *fhci;
	struct ed *ed;
	unsigned long flags;

	fhci = hcd_to_fhci(hcd);
	local_irq_save(flags);
	_raw_spin_lock(&fhci->lock);
	ed = ep->hcpriv;
	if (ed) {
		while (ed->td_head != NULL) {
			struct td *td = remove_td_from_ed(ed);
			urb_complete_free(fhci, td->urb);
		}
		recycle_empty_ed(fhci, ed);
		ep->hcpriv = NULL;
	}
	_raw_spin_unlock(&fhci->lock);
	local_irq_restore(flags);
}

static int fhci_get_frame_number(struct usb_hcd *hcd)
{
	struct fhci_hcd *fhci = hcd_to_fhci(hcd);

	return get_frame_num(fhci);
}

static const struct hc_driver fhci_driver = {
	.description = "fsl,usb-fhci",
	.product_desc = "FHCI HOST Controller",
	.hcd_priv_size = sizeof(struct fhci_hcd),

	/* generic hardware linkage */
	.irq = fhci_irq,
	.flags = HCD_USB11 | HCD_MEMORY,

	/* basic lifecycle operation */
	.start = fhci_start,
	.stop = fhci_stop,

	/* managing i/o requests and associated device resources */
	.urb_enqueue = fhci_urb_enqueue,
	.urb_dequeue = fhci_urb_dequeue,
	.endpoint_disable = fhci_endpoint_disable,

	/* scheduling support */
	.get_frame_number = fhci_get_frame_number,

	/* root hub support */
	.hub_status_data = fhci_hub_status_data,
	.hub_control = fhci_hub_control,
};

struct fhci_probe_info {
	struct resource regs;
	unsigned long pram_addr;
	struct resource usb_irq;
	int gpios[NUM_GPIOS];
	enum qe_clock fullspeed_clk;
	enum qe_clock lowspeed_clk;
	unsigned int power_budget;
};

static int __devinit fhci_probe(struct device *dev, struct fhci_probe_info *pi)
{
	unsigned long ret;
	int i;
	struct usb_hcd *hcd = NULL;
	struct fhci_hcd *fhci;

	if (usb_disabled())
		return -ENODEV;

	hcd = usb_create_hcd(&fhci_driver, dev, dev->bus_id);
	if (!hcd) {
		dev_dbg(dev, "could not create hcd\n");
		return -ENOMEM;
	}

	dev_set_drvdata(dev, hcd);
	fhci = hcd_to_fhci(hcd);

	hcd->self.controller = dev;
	hcd->power_budget = pi->power_budget;
	hcd->regs = ioremap(pi->regs.start, pi->regs.end - pi->regs.start + 1);
	fhci->regs = hcd->regs;
	memcpy(fhci->gpios, pi->gpios, sizeof(fhci->gpios));

	ret = qe_muram_alloc_fixed(pi->pram_addr, FHCI_PRAM_SIZE);
	if (IS_ERR_VALUE(ret) || ret != pi->pram_addr) {
		dev_err(dev, "failed to allocate usb pram\n");
		goto err_pram_alloc;
	}
	fhci->pram = qe_muram_addr(pi->pram_addr);

	for (i = 0; i < NUM_GPIOS; i++) {
		int gpio = fhci->gpios[i];

		if (gpio < 0) {
			if (gpio < GPIO_SPEED) {
				dev_err(dev, "incorrect GPIO%d: %d\n",
					i, gpio);
				goto err_gpios;
			} else {
				dev_info(dev, "assuming board doesn't have "
					"%s gpio\n", gpio == GPIO_SPEED ?
					"speed" : "suspn");
			}
		}

		ret = gpio_request(gpio, dev->bus_id);
		if (ret) {
			dev_err(dev, "failed to request gpio %d", i);
			goto err_gpios;
		}
	}

	ret = fhci->timer = qe_get_timer(16, &fhci->timer_irq);
	if (ret < 0) {
		dev_err(dev, "failed to request qe timer: %li", ret);
		goto err_get_timer;
	}

	fhci->fullspeed_clk = pi->fullspeed_clk;
	fhci->lowspeed_clk = pi->lowspeed_clk;

	ret = request_irq(fhci->timer_irq, fhci_frame_limit_timer_irq,
			  IRQF_DISABLED | IRQF_TIMER, "qe timer (usb)", hcd);
	if (ret) {
		dev_err(dev, "failed to request timer irq");
		goto err_timer_irq;
	}

	dev_info(dev, "at 0x%p,irq %d\n", hcd->regs, pi->usb_irq.start);

	/* start with low-speed, if possible */
	if (fhci->lowspeed_clk != QE_CLK_NONE)
		qe_usb_clock_set(fhci->lowspeed_clk, USB_CLOCK >> 3);
	else
		qe_usb_clock_set(fhci->fullspeed_clk, USB_CLOCK);

	config_transceiver(fhci, FHCI_OP_HOST);

	ret = usb_add_hcd(hcd, pi->usb_irq.start, IRQF_DISABLED |
						  IRQF_NODELAY);
	if (ret < 0)
		goto err_add_hcd;

	fhci_dfs_create(fhci);

	return 0;

err_add_hcd:
	free_irq(fhci->timer_irq, hcd);
err_timer_irq:
	qe_put_timer(fhci->timer);
err_get_timer:
err_gpios:
	while (--i >= 0) {
		if (fhci->gpios[i] >= 0)
			gpio_free(fhci->gpios[i]);
	}
err_pram_alloc:
	usb_put_hcd(hcd);
	return ret;
}

static int __devexit fhci_remove(struct device *dev)
{
	struct usb_hcd *hcd = dev_get_drvdata(dev);
	struct fhci_hcd *fhci = hcd_to_fhci(hcd);

	fhci_dfs_destroy(fhci);
	usb_remove_hcd(hcd);
	free_irq(fhci->timer_irq, hcd);
	qe_put_timer(fhci->timer);
	qe_muram_free(qe_muram_offset(fhci->pram));
	usb_put_hcd(hcd);

	return 0;
}

static int __devinit of_fhci_probe(struct of_device *ofdev,
				   const struct of_device_id *ofid)
{
	int ret;
	struct device *dev = &ofdev->dev;
	struct fhci_probe_info pi = {
		.lowspeed_clk = QE_CLK_NONE,
		.fullspeed_clk = QE_CLK_NONE,
	};
	int size;
	const unsigned long *pram_addr;
	const char *clk;
	const u32 *power_budget;
	struct device_node *np;
	int i;

	np = of_find_compatible_node(NULL, NULL, "fsl,qe-muram-usb-pram");
	if (!np) {
		dev_err(dev, "can't find usb-pram node\n");
		return -ENOENT;
	}

	pram_addr = of_get_property(np, "reg", &size);
	if (!pram_addr || size < sizeof(*pram_addr)) {
		dev_err(dev, "can't get pram offset\n");
		of_node_put(np);
		return -EINVAL;;
	}
	pi.pram_addr = *pram_addr;
	of_node_put(np);

	ret = of_address_to_resource(ofdev->node, 0, &pi.regs);
	if (ret) {
		dev_err(dev, "can't get regs\n");
		return -EINVAL;
	}

	clk = of_get_property(ofdev->node, "fullspeed-clock", NULL);
	if (clk) {
		pi.fullspeed_clk = qe_clock_source(clk);
		if (pi.fullspeed_clk == QE_CLK_DUMMY) {
			dev_err(dev, "wrong fullspeed-clock\n");
			return -EINVAL;
		}
	}

	clk = of_get_property(ofdev->node, "lowspeed-clock", NULL);
	if (clk) {
		pi.lowspeed_clk = qe_clock_source(clk);
		if (pi.lowspeed_clk == QE_CLK_DUMMY) {
			dev_err(dev, "wrong lowspeed-clock\n");
			return -EINVAL;
		}
	}

	if (pi.fullspeed_clk == QE_CLK_NONE &&
			pi.lowspeed_clk == QE_CLK_NONE) {
		dev_err(dev, "no clocks specified\n");
		return -EINVAL;
	}

	power_budget = of_get_property(ofdev->node, "hub-power-budget", &size);
	if (power_budget && size == sizeof(*power_budget))
		pi.power_budget = *power_budget;

	ret = of_irq_to_resource(ofdev->node, 0, &pi.usb_irq);
	if (ret == NO_IRQ) {
		dev_err(dev, "can't get usb irq\n");
		return ret;
	}

	/* gpios error and sanity checks are in the fhci_probe() */
	for (i = 0; i < NUM_GPIOS; i++)
		pi.gpios[i] = of_get_gpio(ofdev->node, i);

	return fhci_probe(dev, &pi);
}

static int __devexit of_fhci_remove(struct of_device *ofdev)
{
	return fhci_remove(&ofdev->dev);
}

static struct of_device_id of_fhci_match[] = {
	{ .compatible = "fsl,usb-fhci", },
	{},
};
MODULE_DEVICE_TABLE(of, of_fhci_match);

static struct of_platform_driver of_fhci_driver = {
	.name		= "fsl,usb-fhci",
	.match_table	= of_fhci_match,
	.probe		= of_fhci_probe,
	.remove		= __devexit_p(of_fhci_remove),
};

static int __init fhci_module_init(void)
{
	return of_register_platform_driver(&of_fhci_driver);
}
module_init(fhci_module_init);

static void __exit fhci_module_exit(void)
{
	of_unregister_platform_driver(&of_fhci_driver);
}
module_exit(fhci_module_exit);

MODULE_DESCRIPTION("USB Freescale Host Controller Interface Driver");
MODULE_AUTHOR("Shlomi Gridish <gridish@freescale.com>, "
	      "Jerry Huang <Chang-Ming.Huang@freescale.com>");
MODULE_LICENSE("GPL");
