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

static void td_fill(struct urb *urb, struct ed *ed, struct td *td,
		    u16 index, enum fhci_ta_type type, int toggle, u8 *data,
		    u32 len, u16 interval, u16 start_frame, bool ioc)
{
	td->urb = urb;
	td->ed = ed;
	td->type = type;
	td->toggle = toggle;
	td->data = data;
	td->len = len;
	td->iso_index = index;
	td->interval = interval;
	td->start_frame = start_frame;
	td->ioc = ioc;
	td->status = USB_TD_OK;
}

static void add_td_to_frame(struct fhci_time_frame *frame, struct td *td)
{
	list_add_tail(&td->frame_lh, &frame->tds_list);
}

static void add_tds_to_ed(struct ed *ed, struct td **td_list, int number)
{
	int i;

	for (i = 0; i < number; i++) {
		struct td *td = td_list[i];
		list_add_tail(&td->node, &ed->td_list);
	}
	if (ed->td_head == NULL)
		ed->td_head = td_list[0];
}

static struct td *peek_td_from_ed(struct ed *ed)
{
	struct td *td;

	if (!list_empty(&ed->td_list))
		td = list_entry(ed->td_list.next, struct td, node);
	else
		td = NULL;

	return td;
}

static struct td *remove_td_from_frame(struct fhci_time_frame *frame)
{
	struct td *td;

	if (!list_empty(&frame->tds_list)) {
		td = list_entry(frame->tds_list.next, struct td, frame_lh);
		list_del_init(frame->tds_list.next);
	} else
		td = NULL;

	return td;
}

static struct td *peek_td_from_frame(struct fhci_time_frame *frame)
{
	struct td *td;

	if (!list_empty(&frame->tds_list))
		td = list_entry(frame->tds_list.next, struct td, frame_lh);
	else
		td = NULL;

	return td;
}
static struct td *remove_td_from_ed(struct ed *ed)
{
	struct td *td;

	if (!list_empty(&ed->td_list)) {
		td = list_entry(ed->td_list.next, struct td, node);
		list_del_init(ed->td_list.next);

		/* if this TD was the ED's head, find next TD */
		if (!list_empty(&ed->td_list))
			ed->td_head = list_entry(ed->td_list.next, struct td,
						 node);
		else
			ed->td_head = NULL;
	} else
		td = NULL;

	return td;
}

static struct td *remove_td_from_done_list(struct fhci_controller_list *p_list)
{
	struct td *td;

	if (!list_empty(&p_list->done_list)) {
		td = list_entry(p_list->done_list.next, struct td, node);
		list_del_init(p_list->done_list.next);
	} else
		td = NULL;

	return td;
}

static void move_td_from_ed_to_done_list(struct fhci_usb *usb, struct ed *ed)
{
	struct td *td;

	td = ed->td_head;
	list_del_init(&td->node);

	/* If this TD was the ED's head,find next TD */
	if (!list_empty(&ed->td_list))
		ed->td_head = list_entry(ed->td_list.next, struct td, node);
	else {
		ed->td_head = NULL;
		ed->state = FHCI_ED_SKIP;
	}
	ed->toggle_carry = td->toggle;
	list_add_tail(&td->node, &usb->hc_list->done_list);
	if (td->ioc)
		usb->transfer_confirm(usb->fhci);
}

/* free done FHCI URB resource such as ED and TD*/
static void free_urb_priv(struct fhci_hcd *fhci, struct urb *urb)
{
	int i;
	struct urb_priv *urb_priv = urb->hcpriv;
	struct ed *ed = urb_priv->ed;

	for (i = 0; i < urb_priv->num_of_tds; i++) {
		list_del_init(&urb_priv->tds[i]->node);
		recycle_empty_td(fhci, urb_priv->tds[i]);
	}

	/* if this TD was the ED's head,find the next TD */
	if (!list_empty(&ed->td_list))
		ed->td_head = list_entry(ed->td_list.next, struct td, node);
	else
		ed->td_head = NULL;

	kfree(urb_priv->tds);
	kfree(urb_priv);
	urb->hcpriv = NULL;

	/* if this TD was the ED's head,find next TD */
	if (ed->td_head == NULL)
		list_del_init(&ed->node);
	fhci->active_urbs--;
}

/* this routine called to complete and free done URB */
static void urb_complete_free(struct fhci_hcd *fhci, struct urb *urb)
{
	free_urb_priv(fhci, urb);

	if (urb->status == -EINPROGRESS) {
		if (urb->actual_length != urb->transfer_buffer_length &&
				urb->transfer_flags & URB_SHORT_NOT_OK)
			urb->status = -EREMOTEIO;
		else
			urb->status = 0;
	}
	_raw_spin_unlock(&fhci->lock);

	usb_hcd_giveback_urb(fhci_to_hcd(fhci), urb, NULL);

	_raw_spin_lock(&fhci->lock);
}

/*
 * caculate transfer length/stats and update the urb
 * Precondition: irqsafe(only for urb-?status locking)
 */
static void done_td(struct urb *urb, struct td *td)
{
	struct ed *ed = td->ed;
	u32 cc = td->status;

	/* ISO...drivers see per-TD length/status */
	if (ed->mode == FHCI_TF_ISO) {
		u32 len;
		if (!(urb->transfer_flags & URB_SHORT_NOT_OK &&
				cc == USB_TD_RX_DATA_UNDERUN))
			cc = USB_TD_OK;

		if (usb_pipeout(urb->pipe))
			len = urb->iso_frame_desc[td->iso_index].length;
		else
			len = td->actual_len;

		urb->actual_length += len;
		urb->iso_frame_desc[td->iso_index].actual_length = len;
		urb->iso_frame_desc[td->iso_index].status =
			status_to_error(cc);
	}

	/* BULK,INT,CONTROL... drivers see aggregate length/status,
	 * except that "setup" bytes aren't counted and "short" transfers
	 * might not be reported as errors.
	 */
	else {
		if (td->error_cnt >= 3)
			urb->error_count = 3;

		/* control endpoint only have soft stalls */

		/* update packet status if needed(short may be ok) */
		if (!(urb->transfer_flags & URB_SHORT_NOT_OK) &&
				cc == USB_TD_RX_DATA_UNDERUN) {
			ed->state = FHCI_ED_OPER;
			cc = USB_TD_OK;
		}
		if (cc != USB_TD_OK) {
			if (urb->status == -EINPROGRESS)
				urb->status = status_to_error(cc);
		}

		/* count all non-empty packets except control SETUP packet */
		if (td->type != FHCI_TA_SETUP || td->iso_index != 0)
			urb->actual_length += td->actual_len;
	}
}

/* there are some pedning request to unlink */
static void del_ed_list(struct fhci_hcd *fhci, struct ed *ed)
{
	struct td *td = peek_td_from_ed(ed);
	struct urb *urb = td->urb;
	struct urb_priv *urb_priv = urb->hcpriv;

	if (urb_priv->state == URB_DEL) {
		td = remove_td_from_ed(ed);
		/* HC may have partly processed this TD */
		if (td->status != USB_TD_INPROGRESS)
			done_td(urb, td);

		/* URB is done;clean up */
		if (++(urb_priv->tds_cnt) == urb_priv->num_of_tds)
			urb_complete_free(fhci, urb);
	}
}

/*
 * Process normal completions(error or sucess) and clean the schedule.
 *
 * This is the main path for handing urbs back to drivers. The only other patth
 * is process_del_list(),which unlinks URBs by scanning EDs,instead of scanning
 * the (re-reversed) done list as this does.
 */
static void process_done_list(unsigned long data)
{
	struct urb *urb;
	struct ed *ed;
	struct td *td;
	struct urb_priv *urb_priv;
	struct fhci_hcd *fhci = (struct fhci_hcd *)data;

	disable_irq(fhci->timer_irq);
	disable_irq(fhci_to_hcd(fhci)->irq);
	_raw_spin_lock(&fhci->lock);

	td = remove_td_from_done_list(fhci->hc_list);
	while (td != NULL) {
		urb = td->urb;
		urb_priv = urb->hcpriv;
		ed = td->ed;

		/* update URB's length and status from TD */
		done_td(urb, td);
		urb_priv->tds_cnt++;

		/*
		 * if all this urb's TDs are done, call complete()
		 * Interrupt transfers are the onley special case:
		 * they are reissued,until "deleted" by usb_unlink_urb
		 * (real work done in a SOF intr, by process_del_list)
		 */
		if (urb_priv->tds_cnt == urb_priv->num_of_tds) {
			urb_complete_free(fhci, urb);
		} else if (urb_priv->state == URB_DEL &&
				ed->state == FHCI_ED_SKIP) {
			del_ed_list(fhci, ed);
			ed->state = FHCI_ED_OPER;
		} else if (ed->state == FHCI_ED_HALTED) {
			urb_priv->state = URB_DEL;
			ed->state = FHCI_ED_URB_DEL;
			del_ed_list(fhci, ed);
			ed->state = FHCI_ED_OPER;
		}

		td = remove_td_from_done_list(fhci->hc_list);
	}

	_raw_spin_unlock(&fhci->lock);
	enable_irq(fhci->timer_irq);
	enable_irq(fhci_to_hcd(fhci)->irq);
}

/*
 * adds urb to the endpoint descriptor list
 * arguments:
 * fhci		data structure for the Low level host controller
 * ep		USB Host endpoint data structure
 * urb		USB request block data structure
 */
static void queue_urb(struct fhci_hcd *fhci, struct urb *urb,
		      struct usb_host_endpoint *ep)
{
	struct ed *ed = ep->hcpriv;
	struct urb_priv *urb_priv = urb->hcpriv;
	u32 data_len = urb->transfer_buffer_length;
	int urb_state = 0;
	int toggle = 0;
	struct td *td;
	u8 *data;
	u16 cnt = 0;

	if (ed == NULL) {
		ed = get_empty_ed(fhci);
		ed->dev_addr = (u8) usb_pipedevice(urb->pipe);
		ed->ep_addr = (u8) usb_pipeendpoint(urb->pipe);
		switch (usb_pipetype(urb->pipe)) {
		case PIPE_CONTROL:
			ed->mode = FHCI_TF_CTRL;
			break;
		case PIPE_BULK:
			ed->mode = FHCI_TF_BULK;
			break;
		case PIPE_INTERRUPT:
			ed->mode = FHCI_TF_INTR;
			break;
		case PIPE_ISOCHRONOUS:
			ed->mode = FHCI_TF_ISO;
			break;
		default:
			break;
		}
		ed->speed = (urb->dev->speed == USB_SPEED_LOW) ?
			FHCI_LOW_SPEED : FHCI_FULL_SPEED;
		ed->max_pkt_size = usb_maxpacket(urb->dev,
			urb->pipe, usb_pipeout(urb->pipe));
		ep->hcpriv = ed;
	}

	/* for ISO transfer calculate start frame index */
	if (ed->mode == FHCI_TF_ISO && urb->transfer_flags & URB_ISO_ASAP)
		urb->start_frame = ed->td_head ? ed->last_iso + 1 :
						 get_frame_num(fhci);

	/*
	 * OHCI handles the DATA toggle itself,we just use the USB
	 * toggle bits
	 */
	if (usb_gettoggle(urb->dev, usb_pipeendpoint(urb->pipe),
			  usb_pipeout(urb->pipe)))
		toggle = USB_TD_TOGGLE_CARRY;
	else {
		toggle = USB_TD_TOGGLE_DATA0;
		usb_settoggle(urb->dev, usb_pipeendpoint(urb->pipe),
			      usb_pipeout(urb->pipe), 1);
	}

	urb_priv->tds_cnt = 0;
	urb_priv->ed = ed;
	if (data_len > 0)
		data = urb->transfer_buffer;
	else
		data = NULL;


	/*
	 * FIXME: we seem to not need this on mainline kernels, probably this
	 *        is usb core bug somewhere.
	 */
	if (!data && urb->transfer_dma) {
		/*
		 * We back-map the DMA buffer and use it instead of a null
		 * transfer_buffer.
		 */
		data = bus_to_virt(urb->transfer_dma);
	}

	switch (ed->mode) {
	case FHCI_TF_BULK:
		if (urb->transfer_flags & URB_ZERO_PACKET &&
				urb->transfer_buffer_length > 0 &&
				((urb->transfer_buffer_length %
				usb_maxpacket(urb->dev, urb->pipe,
				usb_pipeout(urb->pipe))) == 0))
			urb_state = US_BULK0;
		while (data_len > 4096) {
			td = td_alloc_fill(fhci, urb, urb_priv, ed, cnt,
				usb_pipeout(urb->pipe) ? FHCI_TA_OUT :
							 FHCI_TA_IN,
				cnt ? USB_TD_TOGGLE_CARRY :
				      toggle,
				data, 4096, 0, 0, true);
			data += 4096;
			data_len -= 4096;
			cnt++;
		}

		td = td_alloc_fill(fhci, urb, urb_priv, ed, cnt,
			usb_pipeout(urb->pipe) ? FHCI_TA_OUT : FHCI_TA_IN,
			cnt ? USB_TD_TOGGLE_CARRY : toggle,
			data, data_len, 0, 0, true);
		cnt++;

		if (urb->transfer_flags & URB_ZERO_PACKET &&
				cnt < urb_priv->num_of_tds) {
			td = td_alloc_fill(fhci, urb, urb_priv, ed, cnt,
				usb_pipeout(urb->pipe) ? FHCI_TA_OUT :
							 FHCI_TA_IN,
				USB_TD_TOGGLE_CARRY, NULL, 0, 0, 0, true);
			cnt++;
		}
		break;
	case FHCI_TF_INTR:
		urb->start_frame = get_frame_num(fhci) + 1;
		td = td_alloc_fill(fhci, urb, urb_priv, ed, cnt++,
			usb_pipeout(urb->pipe) ? FHCI_TA_OUT : FHCI_TA_IN,
			USB_TD_TOGGLE_DATA0, data, data_len,
			urb->interval, urb->start_frame, true);
		break;
	case FHCI_TF_CTRL:
		ed->dev_addr = usb_pipedevice(urb->pipe);
		ed->max_pkt_size = usb_maxpacket(urb->dev, urb->pipe,
			usb_pipeout(urb->pipe));
		td = td_alloc_fill(fhci, urb, urb_priv, ed, cnt++,
			FHCI_TA_SETUP, USB_TD_TOGGLE_DATA0, urb->setup_packet,
			8, 0, 0, true);

		if (data_len > 0) {
			td = td_alloc_fill(fhci, urb, urb_priv, ed, cnt++,
				usb_pipeout(urb->pipe) ? FHCI_TA_OUT :
							 FHCI_TA_IN,
				USB_TD_TOGGLE_DATA1, data, data_len, 0, 0,
				true);
		}
		td = td_alloc_fill(fhci, urb, urb_priv, ed, cnt++,
			usb_pipeout(urb->pipe) ? FHCI_TA_IN : FHCI_TA_OUT,
			USB_TD_TOGGLE_DATA1, data, 0, 0, 0, true);
		urb_state = US_CTRL_SETUP;
		break;
	case FHCI_TF_ISO:
		for (cnt = 0; cnt < urb->number_of_packets; cnt++) {
			u16 frame = (u16) urb->start_frame;

			/*
			 * FIXME scheduling should handle frame counter
			 * roll-around ... exotic case (and OHCI has
			 * a 2^16 iso range, vs other HCs max of 2^10)
			 */
			frame += cnt * urb->interval;
			frame &= 0x07ff;
			td = td_alloc_fill(fhci, urb, urb_priv, ed, cnt,
				usb_pipeout(urb->pipe) ? FHCI_TA_OUT :
							 FHCI_TA_IN,
				USB_TD_TOGGLE_DATA0,
				data + urb->iso_frame_desc[cnt].offset,
				urb->iso_frame_desc[cnt].length,
				urb->interval, frame, true);
		}
	default:
		break;
	}

	/*
	 * set the state of URB
	 * control pipe:3 states -- setup,data,status
	 * interrupt and bulk pipe:1 state -- data
	 */
	urb->pipe &= ~0x1f;
	urb->pipe |= urb_state & 0x1f;

	urb_priv->state = URB_INPROGRESS;

	if (!ed->td_head) {
		ed->state = FHCI_ED_OPER;
		switch (ed->mode) {
		case FHCI_TF_CTRL:
			list_add(&ed->node, &fhci->hc_list->ctrl_list);
			break;
		case FHCI_TF_BULK:
			list_add(&ed->node, &fhci->hc_list->bulk_list);
			break;
		case FHCI_TF_INTR:
			list_add(&ed->node, &fhci->hc_list->intr_list);
			break;
		case FHCI_TF_ISO:
			list_add(&ed->node, &fhci->hc_list->iso_list);
		default:
			break;
		}
	}

	add_tds_to_ed(ed, urb_priv->tds, urb_priv->num_of_tds);
	fhci->active_urbs++;
}
