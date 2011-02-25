/*
 * Driver for the MPC8272 on-chip peripheral USB device controller.
 * Specs and errata are available from <http://www.freescale.com>.
 *
 * This driver should work well with Serial "gadget" driver.
 *
 * Author: Gennadiy Kurtsman <source@mvista.com>
 *
 * Based on:
 * Montavista Pro 3.1 PQ2 USB-based tty device driver, by Vitaly Bordug
 *
 * 2005-2007 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 *
 */

#include <linux/module.h>
#include <linux/pci.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/smp_lock.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/moduleparam.h>
#include <linux/device.h>
#include <linux/usb_ch9.h>
#include <linux/usb_gadget.h>
#include <linux/fsl_devices.h>

#include <asm/byteorder.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/system.h>
#include <asm/unaligned.h>
#include <asm/immap_cpm2.h>
#include <asm/cpm2.h>
#include <asm/mpc8260.h>
#include <asm/of_platform.h>
#include "mpc8272_udc.h"

#define	DRIVER_DESC		"MPC8272_ADS USB Peripheral Controller"
#define	DRIVER_VERSION		"2007 July 10"

static struct _mpc8272_udc_dev* the_controller = NULL;

static const char driver_name [] = "mpc8272_udc";
static const char driver_desc [] = DRIVER_DESC;

#define EP0_NAME "ep0"
#define EP_IN_NAME "ep-in"
#define EP_OUT_NAME "ep-out"

static const char ep0name [] = EP0_NAME;
static const char *ep_name [] = {
	ep0name,
	EP_IN_NAME,
	EP_OUT_NAME,
};

static void mpc8272_usbc_kick_tx(struct _mpc8272_udc_dev *usbc_dev);
static int mpc8272_usbc_rx_ctrl(struct mpc8272_usbc_ep *ep, int pid,
				__u8 * data, __u16 len, int is_error);
static int mpc8272_usbc_rx_data_in(struct mpc8272_usbc_ep *ep, int pid,
				   __u8 * data, __u16 len, int is_error);
static int mpc8272_usbc_rx_data_out(struct mpc8272_usbc_ep *ep, int pid,
				    __u8 * data, __u16 len, int is_error);
static void mpc8272_usbc_txconf_ctrl(struct mpc8272_usbc_ep *ep, __u16 len,
				     int is_error);
static void mpc8272_usbc_txconf_data_in(struct mpc8272_usbc_ep *ep,
					__u16 len, int is_error);
static void mpc8272_usbc_txconf_data_out(struct mpc8272_usbc_ep *ep,
					 __u16 len, int is_error);

static mpc8272_usbc_rx_callback rx_cb[] = {
	mpc8272_usbc_rx_ctrl,
	mpc8272_usbc_rx_data_in,
	mpc8272_usbc_rx_data_out,
};

static mpc8272_usbc_txconf_callback txconf_cb[] = {
	mpc8272_usbc_txconf_ctrl,
	mpc8272_usbc_txconf_data_in,
	mpc8272_usbc_txconf_data_out,
};

/* when a driver is successfully registered, it will receive
 * control requests including set_configuration(), which enables
 * non-control requests.  then usb traffic follows until a
 * disconnect is reported.  then a host may connect again, or
 * the driver might get unbound.
 */
int usb_gadget_register_driver(struct usb_gadget_driver *driver)
{
	struct _mpc8272_udc_dev *usbc_dev = the_controller;
	int retval;
	if (!driver
			|| !driver->bind
			|| !driver->unbind
			|| !driver->disconnect
			|| !driver->setup)
		return -EINVAL;
	if (!usbc_dev)
		return -ENODEV;
	if (usbc_dev->driver)
		return -EBUSY;

	/* hook up the driver */
	driver->driver.bus = NULL;
	usbc_dev->driver = driver;
	usbc_dev->gadget.dev.driver = &driver->driver;
	retval = driver->bind(&usbc_dev->gadget);
	if (retval) {
		usbc_dev->driver = NULL;
		usbc_dev->gadget.dev.driver = NULL;
		return retval;
	}
	/* then enable host detection and ep0; and we're ready
	 * for set_configuration as well as eventual disconnect.
	 */

	return 0;
}
EXPORT_SYMBOL(usb_gadget_register_driver);

int usb_gadget_unregister_driver (struct usb_gadget_driver *driver)
{
	struct _mpc8272_udc_dev	*dev = the_controller;

	if (!dev)
		return -ENODEV;
	if (!driver || driver != dev->driver)
		return -EINVAL;

	driver->unbind (&dev->gadget);
	dev->gadget.dev.driver = NULL;
	dev->driver = NULL;

	return 0;
}
EXPORT_SYMBOL(usb_gadget_unregister_driver);

static int mpc8272_disable (struct usb_ep *ep)
{
	return 0;
}

static int
mpc8272_enable (struct usb_ep *_ep, const struct usb_endpoint_descriptor *desc)
{
	struct _mpc8272_udc_dev	*dev;
	struct mpc8272_usbc_ep *ep;
	u16 max;
	unsigned long flags = 0;

	ep = container_of(_ep, struct mpc8272_usbc_ep, ep);
	if (!_ep || !desc || ep->desc
			|| desc->bDescriptorType != USB_DT_ENDPOINT)
		return -EINVAL;
	dev = ep->dev;
	if (ep == &dev->eps[0])
		return -EINVAL;
	if (!dev->driver)
		return -ESHUTDOWN;
	if (ep->ep_num != (desc->bEndpointAddress & 0x0f))
		return -EINVAL;

	switch (desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) {
	case USB_ENDPOINT_XFER_BULK:
	case USB_ENDPOINT_XFER_INT:
		break;
	default:
		return -EINVAL;
	}

	/* enabling the no-toggle interrupt mode would need an api hook */

	max = UDC_USBC_PACKET_SIZE;
	if (!max)
		return -EINVAL;

	if (!in_irq()) {
		spin_lock_irqsave(&ep->lock, flags);
	}

	ep->ep.maxpacket = max;
	ep->desc = (struct usb_endpoint_descriptor *)desc;
	if (!in_irq())	{
		spin_unlock_irqrestore(&ep->lock, flags);
	}
	return 0;
};

static int
mpc8272_queue(struct usb_ep *_ep, struct usb_request *_req, gfp_t gfp_flags)
{
	struct mpc8272_request *req;
	struct mpc8272_usbc_ep *ep;
	struct _mpc8272_udc_dev *usbc_dev;
	unsigned long flags = 0;
	int count;

	/* always require a cpu-view buffer so pio works */
	req = container_of(_req, struct mpc8272_request, req);
	if (unlikely(!_req || !_req->complete
		|| !_req->buf || !list_empty(&req->queue)))
			return -EINVAL;
	ep = (struct mpc8272_usbc_ep*)container_of(_ep, struct mpc8272_usbc_ep,
						   ep);
	if (unlikely(!_ep))
		return -EINVAL;
	usbc_dev = ep->dev;
	if (!strcmp(_ep->name, EP0_NAME)) {
		memcpy (usbc_dev->ctrl_data, _req->buf, _req->length);
		usbc_dev->ctrl_length = _req->length;
		_req->status = 0;
		_req->actual = _req->length;
		_req->complete(&ep->ep,_req);
	}
	if (!strcmp(_ep->name, EP_IN_NAME)) {
		if (!in_irq()) {
			spin_lock_irqsave(&ep->lock,flags);
		}
		count = __mpc8272_usbc_buf_room(&usbc_dev->write_buf);

		if ((count < _req->length) || !list_empty(&ep->queue)) {
			_req->status = -EINPROGRESS;
			_req->actual = 0;
			list_add_tail(&req->queue, &ep->queue);
			if (!in_irq()) {
				spin_unlock_irqrestore(&ep->lock,flags);
			}
		 } else {
			count = __mpc8272_usbc_buf_write(&usbc_dev->write_buf,
							 _req->buf,
							 _req->length);
			if ((count =
			    __mpc8272_usbc_buf_room(&usbc_dev->write_buf)) ==
			    (usbc_dev->write_buf.size - _req->length) - 1)
				mpc8272_usbc_kick_tx(usbc_dev);
			if (!in_irq()) {
				spin_unlock_irqrestore(&ep->lock,flags);
			}
			_req->status = 0;
			_req->actual = count;
			_req->complete(&ep->ep,_req);
		}
	}
	if (!strcmp(_ep->name, EP_OUT_NAME)) {
		if (!in_irq()) {
			spin_lock_irqsave(&ep->lock,flags);
		}
		count = __mpc8272_usbc_buf_len(&usbc_dev->read_buf);
		if (!count) {
			_req->status = -EINPROGRESS;
			_req->actual = 0;
			list_add_tail(&req->queue, &ep->queue);
			if (!in_irq()) {
				spin_unlock_irqrestore(&ep->lock,flags);
			}
		} else {
			count = __mpc8272_usbc_buf_read(&usbc_dev->read_buf,
							_req->buf,
							_req->length);
			if (!in_irq()) {
				spin_unlock_irqrestore(&ep->lock,flags);
			}
			_req->status = 0;
			_req->actual = count;
			_req->complete(&ep->ep,_req);
		}
	}

	return 0;
}

static struct usb_request *
mpc8272_alloc_request(struct usb_ep *_ep, gfp_t gfp_flags)
{
	struct mpc8272_request     *req;

	if (!_ep)
		return NULL;
	req = kmalloc(sizeof *req, gfp_flags);
	if (!req)
		return NULL;

	memset(req, 0, sizeof *req);
	INIT_LIST_HEAD(&req->queue);
	return &req->req;
}

static void
mpc8262_free_request(struct usb_ep *_ep, struct usb_request *_req)
{
	struct mpc8272_request     *req;

	if (!_ep || !_req)
		return;

	req = container_of(_req, struct mpc8272_request, req);
	WARN_ON(!list_empty(&req->queue));
	kfree(req);
}

static struct usb_ep_ops mpc8272_ep_ops = {
	.enable = mpc8272_enable,
	.disable = mpc8272_disable,
	.alloc_request = mpc8272_alloc_request,
	.free_request = mpc8262_free_request,
	.queue = mpc8272_queue,
}

MODULE_DESCRIPTION (DRIVER_DESC);
MODULE_AUTHOR ("Gennadiy Kurtsman");
MODULE_LICENSE ("GPL");

static int  mpc8272_udc_get_frame (struct usb_gadget *_gadget)
{
	return 0;
}

static const struct usb_gadget_ops mpc8272_udc_ops = {
	.get_frame	= mpc8272_udc_get_frame,
};

/**********************************************************************************
 * Callbacks and helpers called from ISR
 *********************************************************************************/

/* Parse standard setup request */
static inline int
mpc8272_usbc_setup(struct mpc8272_usbc_ep *ep, __u8 * data, int len)
{
	struct _mpc8272_udc_dev *usbc_dev = ep->dev;
	struct usb_ctrlrequest *cmd = (struct usb_ctrlrequest *) data;
	__u16 typeReq, wValue, wIndex, wLength;
	int ret = -EINVAL;
	int handshake;

	if (len != 8)
		return ret;

	typeReq = (cmd->bRequestType << 8) | cmd->bRequest;
	wValue = le16_to_cpu(cmd->wValue);
	wIndex = le16_to_cpu(cmd->wIndex);
	wLength = le16_to_cpu(cmd->wLength);
	handshake = 1;

	data = usbc_dev->ctrl_data;

	switch (typeReq) {
	case DeviceRequest | USB_REQ_GET_STATUS:
	case EndpointRequest | USB_REQ_GET_STATUS:
	case InterfaceRequest | USB_REQ_GET_STATUS:
		data[0] = 0;
		data[1] = 0;
		len = 2;
		ret = 0;
		break;

	case DeviceOutRequest | USB_REQ_SET_ADDRESS:
		/* Assign address */
		usbc_dev->address = (__u8) wValue;
		len = 0;
		ret = 0;
		handshake = 0;
		break;

	default:
		spin_unlock (&ep->lock);
		if (cmd->bRequest == USB_REQ_SET_CONFIGURATION) {
			/* according to USB Spec */
			usbc_dev->eps[1].pid = UDC_BD_PID1;
			usbc_dev->eps[2].pid = UDC_BD_RXPID_0;
		}
		ret = usbc_dev->driver->setup (&usbc_dev->gadget, cmd);
		if (ret >= 0)
			len=usbc_dev->ctrl_length;
		spin_lock (&ep->lock);
	}
	if (ret < 0)
		return ret;
	else {
		if (len > wLength)
			len = wLength;
		/* this for setup packet must start from DATA1 */
		ep->pid = UDC_BD_PID1;
		if ((cmd->bRequest == USB_REQ_GET_DESCRIPTOR) &&
		    ((wValue >> 8) == USB_DT_DEVICE)) {
			((struct usb_device_descriptor*)data)->bMaxPacketSize0 = UDC_USBC_CTL_PACKET_SIZE;
		}
		if ((cmd->bRequest == USB_REQ_GET_DESCRIPTOR) &&
			((wValue >> 8) == USB_DT_CONFIG) &&
			(len == sizeof(struct usb_config_descriptor) +
				sizeof(struct usb_interface_descriptor) +
				(sizeof(struct usb_endpoint_descriptor) - 2) *
				(UDC_USBC_MAX_EPS - 1))) {
			struct usb_endpoint_descriptor* ed =
				(struct usb_endpoint_descriptor*)(data +
				sizeof(struct usb_config_descriptor) +
				sizeof(struct usb_interface_descriptor));
			int i;

			for (i=0; i<(UDC_USBC_MAX_EPS - 1); i++) {
				ed->wMaxPacketSize =
					cpu_to_le16(UDC_USBC_PACKET_SIZE);
				ed = (struct usb_endpoint_descriptor*)((u8*)ed +
				     sizeof(struct usb_endpoint_descriptor) -2);
			}
		}
		mpc8272_usbc_tx_data(ep, usbc_dev->ctrl_data, len);
	}
	return ret;
}

/*
 * Prepares for transmit on BULK-IN EP.
 * Called from TX conf ISR and from char/write target
 */
static void
mpc8272_usbc_kick_tx(struct _mpc8272_udc_dev *usbc_dev)
{
	struct mpc8272_usbc_ep *ep = &usbc_dev->eps[1];	/* bulk-in */
	int len = __mpc8272_usbc_buf_len_flat(&ep->dev->write_buf);

	if (!len)
		return;
	if (len > UDC_USBC_PACKET_SIZE)
		len = UDC_USBC_PACKET_SIZE;
	mpc8272_usbc_tx_data(ep, __mpc8272_usbc_buf_data(&ep->dev->write_buf),
			     len);
}

/* EP0 0 control receive callback */
static int
mpc8272_usbc_rx_ctrl(struct mpc8272_usbc_ep *ep, int pid, __u8 * data,
		     __u16 len, int is_error)
{
	int ret;

	if (is_error)
		return 0;

	if (!len)
		return 0;	/* It was a setup status. No action required */

	/*
	 * Current packet is rejected, new one will be initiated in
	 * mpc8272_usbc_setup()
	 */
	if (ep->data)
		mpc8272_usbc_tx_recover(ep);

	if (pid != UDC_BD_RXPID_SETUP)
		ret = -EINVAL;
	else
		ret = mpc8272_usbc_setup(ep, data, len);

	if (ret)
		mpc8272_usbc_tx_handshake(ep->dev, 0, USB_PID_STALL);

	return 0;
}

/* EP0 1 - bulk in receive callback */
static int
mpc8272_usbc_rx_data_in(struct mpc8272_usbc_ep *ep, int pid, __u8 * data,
			__u16 len, int is_error)
{
	return 0;
}

/* EP0 2 - bulk out receive callback */
static int
mpc8272_usbc_rx_data_out(struct mpc8272_usbc_ep *ep, int pid, __u8 * data,
			 __u16 len, int is_error)
{
	int room_len;
	struct mpc8272_request *req;
	int count;
	struct _mpc8272_udc_dev *usbc_dev = (struct _mpc8272_udc_dev *)ep->dev;

	mpc8272_usbc_tx_handshake(ep->dev, 2, USB_PID_NAK);
	if (is_error) {
		goto mpc8272_handshake_exit;
	}

	/* If pid is not what expected - do nothing */
	if (pid != ep->pid) {
		goto mpc8272_handshake_exit;
	}

	ep->pid = (pid == UDC_BD_RXPID_0) ? UDC_BD_RXPID_1 : UDC_BD_RXPID_0;

	room_len = __mpc8272_usbc_buf_room(&ep->dev->read_buf);
	if (room_len < len) {
		return 1;	/* no room */
	}
	__mpc8272_usbc_buf_write(&ep->dev->read_buf, data, len);
	count = __mpc8272_usbc_buf_len(&usbc_dev->read_buf);
	if (!count)
		goto mpc8272_handshake_exit;
	if (list_empty(&ep->queue))
		goto mpc8272_handshake_exit;
	req = list_entry(ep->queue.next, struct mpc8272_request, queue);
	list_del_init(&req->queue);
	count = __mpc8272_usbc_buf_read(&usbc_dev->read_buf, req->req.buf,
					req->req.length);
	req->req.status = 0;
	req->req.actual = count;
	req->req.complete (&ep->ep, &req->req);

      mpc8272_handshake_exit:
	mpc8272_usbc_tx_handshake(ep->dev, 2, USB_PID_ACK);

	return 0;
}

/* EP0 0 control transmit confirmation callback */
static void
mpc8272_usbc_txconf_ctrl(struct mpc8272_usbc_ep *ep, __u16 len, int is_error)
{
	if (is_error) {
		return;
	}
	if (ep->dev->address) {	/* Set address pending */
		ep->dev->usb_rgs->usb_usadr = ep->dev->address;
		ep->dev->address = 0;
	}
	ep->ack_len += len;
	if (ep->ack_len < ep->data_len) {
		ep->pid = (ep->pid == UDC_BD_PID1) ? UDC_BD_PID0 : UDC_BD_PID1;
		mpc8272_usbc_tx(ep, &ep->data[ep->ack_len],
			    ep->data_len - ep->ack_len,
			    UDC_BD_TC | UDC_BD_CNF | BD_SC_INTRPT);
	} else if ((ep->ack_len == ep->data_len)
		   && !(ep->data_len % ep->max_len)) {
		ep->pid = (ep->pid == UDC_BD_PID1) ? UDC_BD_PID0 : UDC_BD_PID1;
		mpc8272_usbc_tx(ep, ep->data, 0,
			    UDC_BD_TC | UDC_BD_CNF | BD_SC_INTRPT);
	} else {
		/* All done */
		ep->data = NULL;
	}
	return;
}

/* EP0 1 - bulk in transmit confirmation callback */
static void
mpc8272_usbc_txconf_data_in(struct mpc8272_usbc_ep *ep, __u16 len, int is_error)
{
	struct mpc8272_request *req = NULL;
	int count;
	struct _mpc8272_udc_dev *usbc_dev = (struct _mpc8272_udc_dev *)ep->dev;

	if (!is_error) {
		__mpc8272_usbc_buf_inc_read(&ep->dev->write_buf, len);
		ep->pid = (ep->pid == UDC_BD_PID0) ? UDC_BD_PID1 : UDC_BD_PID0;
		while (!(list_empty(&ep->queue))) {

			req = list_entry(ep->queue.next, struct mpc8272_request,
					 queue);
			count = __mpc8272_usbc_buf_room(&usbc_dev->write_buf);
			if (count >= req->req.length) {
				list_del_init(&req->queue);
				count = __mpc8272_usbc_buf_write(
					&usbc_dev->write_buf, req->req.buf,
					req->req.length);
				req->req.status = 0;
				req->req.actual = count;
				req->req.complete(&ep->ep,&req->req);
			} else
			break;
		}
	}
	mpc8272_usbc_kick_tx(ep->dev);
}

/* EP0 2 - bulk out transmit confirmation callback */
static void
mpc8272_usbc_txconf_data_out(struct mpc8272_usbc_ep *ep, __u16 len,
			     int is_error)
{
	return;
}

/* Reset Interrupt handler */
static void
mpc8272_usbc_reset_isr(struct _mpc8272_udc_dev *usbc_dev)
{
	int i;

	for (i = 0; i < UDC_USBC_MAX_EPS; i++) {
		spin_lock(&usbc_dev->eps[i].lock);
		mpc8272_usbc_tx_recover(&usbc_dev->eps[i]);
		spin_unlock(&usbc_dev->eps[i].lock);
	}
	usbc_dev->usb_rgs->usb_usadr = 0;
}

static irqreturn_t mpc8272_usbc_isr (int irq, void *_dev, struct pt_regs *regs)
{
	struct _mpc8272_udc_dev* usbc_dev = _dev;
	__u16 usb_usber = usbc_dev->usb_rgs->usb_usber;
	int i;

	/* Confirm */
	usbc_dev->usb_rgs->usb_usber = usb_usber;

	if ((usb_usber & UDC_USBE_RXB)) {
		/* Handle receive */
		for (i = 0; i < UDC_USBC_MAX_EPS; i++)
			mpc8272_usbc_rx_isr(usbc_dev, i);
	}
	if ((usb_usber & UDC_USBE_TXB)) {
		/* Handle transmit confirmation */
		for (i = 0; i < UDC_USBC_MAX_EPS; i++)
			mpc8272_usbc_tx_isr(usbc_dev, i);
	}
	if ((usb_usber & UDC_USBE_RESET)) {
		/* Reset. Clear address and kill all transactions in progress */
		mpc8272_usbc_reset_isr(usbc_dev);
	}
	return IRQ_HANDLED;
}

static int
mpc8272_usbc_hw_ep_init(struct _mpc8272_udc_dev *usbc_dev, int ep_idx,
		    int ep_num, __u16 ep_type, int max_len)
{
	struct mpc8272_usbc_ep *ep;
	usb_ep_parms *ep_parms;
	__u16 ep_parms_offset;
	volatile __u16 *p_usep_reg;
	__u16 usep_reg;

	if (ep_idx >= UDC_USBC_MAX_EPS)
		return -EINVAL;

	ep = &(usbc_dev->eps[ep_idx]);
	ep->ep.name = ep_name [ep_idx];
	if (ep_idx)
		list_add_tail (&usbc_dev->eps[ep_idx].ep.ep_list,
			       &usbc_dev->gadget.ep_list);
	ep->ep.ops = &mpc8272_ep_ops;

	ep->ep_idx = ep_idx;
	ep->ep_num = ep_num;
	ep->dev = usbc_dev;
	ep->max_len = max_len;
	p_usep_reg = &usbc_dev->usb_rgs->usb_usep1 + ep_idx;
	INIT_LIST_HEAD(&ep->queue);

	/* Allocate memory for buffer descriptors */
	ep->tx_base_offset = cpm_dpalloc(UDC_USBC_TX_BDS *
					 sizeof(mpc8272_cpm_bd),
					 sizeof (mpc8272_cpm_bd));
	ep->tx_base = (mpc8272_cpm_bd*)(cpm_dpram_addr(ep->tx_base_offset));
	if (!(ep->tx_base)) {
		cpm_dpfree(ep->tx_base_offset);
		ep->tx_base_offset = 0;
		BUG_ON(ep->tx_base == NULL);
		return -ENOMEM;
	}
	ep->rx_base_offset = cpm_dpalloc(UDC_USBC_RX_BDS *
					 sizeof(mpc8272_cpm_bd),
					 sizeof (mpc8272_cpm_bd));
	ep->rx_base = (mpc8272_cpm_bd*)(cpm_dpram_addr(ep->rx_base_offset));
	if (!(ep->rx_base)) {
		cpm_dpfree(ep->tx_base_offset);
		ep->tx_base_offset = 0;
		ep->tx_base = NULL;
		cpm_dpfree(ep->rx_base_offset);
		ep->rx_base_offset = 0;
		BUG_ON(ep->rx_base == NULL);
		return -ENOMEM;
	}
	ep->tx_bd = ep->conf_bd = ep->tx_base;
	ep->rx_bd = ep->rx_base;
	ep_parms_offset = cpm_dpalloc(sizeof (usb_ep_parms),
			sizeof (usb_ep_parms));
	ep_parms = (usb_ep_parms*) cpm_dpram_addr(ep_parms_offset);
	if (ep_parms == NULL) {
		cpm_dpfree(ep->tx_base_offset);
		ep->tx_base_offset = 0;
		ep->tx_base = NULL;
		cpm_dpfree(ep->rx_base_offset);
		ep->rx_base_offset = 0;
		ep->rx_base = NULL;
		cpm_dpfree(ep_parms_offset);
		BUG_ON(ep->ep_parms == NULL);
		return -ENOMEM;
	}
	ep->ep_parms = ep_parms;
	ep->ep_parms_offset = ep_parms_offset;
	memset((void *) ep_parms, 0, sizeof (usb_ep_parms));

	*(__u16 *)((unsigned long) &usbc_dev->usb_prms->ep0_ptr +
		ep_idx * sizeof(__u16)) = ep->ep_parms_offset;
	ep_parms->rbase = ep_parms->rbptr = ep->rx_base_offset;
	ep_parms->tbase = ep_parms->tbptr = ep->tx_base_offset;

	/*
	 * for control and bulk pipe, rx buffer is 8 bytes data and 2 bytes
	 * CRC
	 */
	ep_parms->mrblr = UDC_USBC_PACKET_SIZE + 2;
	ep_parms->tfcr = ep_parms->rfcr = CPMFCR_GBL | UDC_USB_FCR_BO_BE;

	/* Init Tx BD ring */
	{
		mpc8272_cpm_bd *_bd = ep->tx_base;
		memset(ep->tx_base, 0,
		       UDC_USBC_TX_BDS * sizeof (mpc8272_cpm_bd));
		UDC_BD_STATUS_SET(_bd + UDC_USBC_TX_BDS - 1, BD_SC_WRAP);
	}
	/* Allocate Rx buffer pool and init Rx BD ring */
	ep->rx_pool =
	    kmalloc(UDC_USBC_RX_BDS * (UDC_USBC_PACKET_SIZE + 4), GFP_KERNEL);
	if (!ep->rx_pool) {
		return -ENOMEM;
	}
	/* Init Rx BD ring */
	{
		mpc8272_cpm_bd *_bd = ep->rx_base;
		__u8 *_buf = ep->rx_pool;
		int _i;
		for (_i = 0; _i < UDC_USBC_RX_BDS; _i++) {
			UDC_BD_DATA_SET(_bd, _buf);
			UDC_BD_LENGTH_SET(_bd, 0);
			UDC_BD_STATUS_SET(_bd, BD_SC_INTRPT | BD_SC_EMPTY);
			_bd++;
			_buf += UDC_USBC_PACKET_SIZE + 4;
		}
		UDC_BD_STATUS_SET((--_bd),
				 BD_SC_INTRPT | BD_SC_WRAP | BD_SC_EMPTY);
	}

	ep->rx_cb = rx_cb[ep_idx];
	ep->txconf_cb = txconf_cb[ep_idx];

	ep->ep_type = ep_type;
	usep_reg = (ep_num << UDC_USEP_NUM_SHIFT) | ep_type;
	/* IT: */
	usep_reg |= UDC_USEP_RTE;

	*p_usep_reg = usep_reg;

	mpc8272_usbc_cmd(ep, UDC_CP_CMD_USB_RESTART_TX);

	return 0;
}

static void
mpc8272_udc_hw_deinit(struct _mpc8272_udc_dev *udc_dev,
		       struct of_device *ofdev)
{
	int i;

	udc_dev->usb_rgs->usb_usmod = 0;
	free_irq(udc_dev->irq, udc_dev);
	for (i=0; i<ARRAY_SIZE(udc_dev->eps); i++) {
		if (udc_dev->eps[i].tx_base_offset) {
			cpm_dpfree(udc_dev->eps[i].tx_base_offset);
			udc_dev->eps[i].tx_base_offset = 0;
			iounmap(udc_dev->eps[i].tx_base);
		}
		udc_dev->eps[i].tx_base = NULL;
		if (udc_dev->eps[i].rx_base_offset) {
			cpm_dpfree(udc_dev->eps[i].rx_base_offset);
			udc_dev->eps[i].rx_base_offset = 0;
			iounmap(udc_dev->eps[i].rx_base);
		}
		udc_dev->eps[i].rx_base = NULL;
		if (udc_dev->eps[i].ep_parms_offset) {
			cpm_dpfree(udc_dev->eps[i].ep_parms_offset);
			udc_dev->eps[i].ep_parms_offset = 0;
			iounmap(udc_dev->eps[i].ep_parms);
		}
		udc_dev->eps[i].ep_parms = NULL;
		if (udc_dev->eps[i].rx_pool)
			kfree(udc_dev->eps[i].rx_pool);
		udc_dev->eps[i].rx_pool = NULL;
	}
}

static int
mpc8272_udc_hw_init(struct _mpc8272_udc_dev *udc_dev,
		     struct of_device *ofdev)
{
	cpm2_map_t *intram;
	struct usb_params *usb_prms;
	struct usb_ctlr *usb_rgs;
	struct resource r;
	int ret;

	/* Get IMMR addr, USB params */
	if (!(intram = udc_dev->intram = ioremap(CPM_MAP_ADDR,
						  sizeof(cpm2_map_t))))
		return -ENOMEM;

	memset(&r, 0, sizeof(r));
	ret = of_address_to_resource(ofdev->node, 0, &r);
	if (ret)
		goto unmap_intram;
	if (!(usb_rgs = udc_dev->usb_rgs = ioremap(r.start,
						   r.end - r.start + 1))) {
		ret = -ENOMEM;
		goto unmap_intram;
	}

	memset(&r, 0, sizeof(r));
	ret = of_address_to_resource(ofdev->node, 1, &r);
	if (ret)
		goto unmap_rgs;
	if (!(usb_prms = udc_dev->usb_prms = ioremap(r.start,
						     r.end - r.start + 1))) {
		ret = -ENOMEM;
		goto unmap_rgs;
	}

	/* Initialize USB parameters RAM and registers */
	memset((void *)usb_prms, 0, sizeof(struct usb_params));

	/* Initialize endpoints */
	INIT_LIST_HEAD (&udc_dev->gadget.ep_list);
	udc_dev->gadget.ep0 = &udc_dev->eps[0].ep;
	if ((ret = mpc8272_usbc_hw_ep_init(udc_dev, 0, 0, UDC_USEP_TM_CTRL,
				 	   UDC_USBC_CTL_PACKET_SIZE)))
		goto unmap_prms;
	if ((ret = mpc8272_usbc_hw_ep_init(udc_dev, 1, 1, UDC_USEP_TM_BULK,
					   UDC_USBC_PACKET_SIZE)))
		goto unmap_prms;
	if ((ret = mpc8272_usbc_hw_ep_init(udc_dev, 2, 2, UDC_USEP_TM_BULK,
					   UDC_USBC_PACKET_SIZE)))
		goto unmap_prms;

	udc_dev->eps[1].pid = UDC_BD_PID0;
	udc_dev->eps[2].pid = UDC_BD_RXPID_0;
	usb_rgs->usb_usber = 0xffff;
	usb_rgs->usb_usbmr = UDC_USBE_TXE0 | UDC_USBE_BSY | UDC_USBE_TXB
			| UDC_USBE_RXB | UDC_USBE_RESET;

	udc_dev->irq = irq_of_parse_and_map(ofdev->node, 0);
	if (udc_dev->irq == NO_IRQ) {
		ret = -EINVAL;
		goto unmap_prms;
	}
	if ((ret = request_irq(udc_dev->irq, mpc8272_usbc_isr, SA_RESTART,
			       "mpc8272_udc", udc_dev)))
		goto unmap_prms;

	udc_dev->usb_rgs->usb_usmod = UDC_USMOD_ENABLE;

	return 0;

unmap_prms:
	iounmap(usb_prms);
unmap_rgs:
	iounmap(usb_rgs);
unmap_intram:
	iounmap(intram);

	return ret;
}

static int __devinit mpc8272_udc_probe(struct of_device *ofdev,
				       const struct of_device_id *match)
{
	int ret = 0;
	u32 dma;

	if (the_controller) {
		WARN_ON(the_controller);
		return -EBUSY;
	}
	the_controller = dma_alloc_coherent(NULL,
					    sizeof(struct _mpc8272_udc_dev),
					    &dma, GFP_KERNEL);
	if (!the_controller) {
		WARN_ON(!the_controller);
		return -ENOMEM;
	}
	memset(the_controller, 0, sizeof (struct _mpc8272_udc_dev));
	the_controller->gadget.ops = &mpc8272_udc_ops;
	spin_lock_init(&the_controller->eps[0].lock);
	spin_lock_init(&the_controller->eps[1].lock);
	spin_lock_init(&the_controller->eps[2].lock);
	strcpy (the_controller->gadget.dev.bus_id, "gadget");
	the_controller->gadget.name = driver_name;
	__mpc8272_usbc_buf_init(&the_controller->read_buf,
				MPC8272_USBC_BUF_SIZE);
	__mpc8272_usbc_buf_init(&the_controller->write_buf,
				MPC8272_USBC_BUF_SIZE);
	ret = mpc8272_udc_hw_init(the_controller, ofdev);
	if (ret)
		goto err;
	ret = device_register(&the_controller->gadget.dev);
	if (ret)
		goto err;
	printk(KERN_INFO "MPC8272 USB peripheral device\n");
	return ret;

err:
	mpc8272_udc_hw_deinit(the_controller, ofdev);
	dma_free_coherent(NULL, sizeof(struct _mpc8272_udc_dev), the_controller,
			  virt_to_phys(the_controller));
	the_controller = NULL;
	printk(KERN_ERR "Unable to initialize USB peripheral device\n");
	return ret;
}

static int mpc8272_udc_remove(struct of_device *ofdev)
{
	mpc8272_udc_hw_deinit(the_controller, ofdev);
	iounmap(the_controller->intram);
	iounmap(the_controller->usb_rgs);
	iounmap(the_controller->usb_prms);
	dev_set_drvdata(&ofdev->dev, NULL);
	dma_free_coherent(NULL, sizeof(struct _mpc8272_udc_dev), the_controller,
			  virt_to_phys(the_controller));
	the_controller = NULL;
	return 0;
}

static struct of_device_id mpc8272_udc_match[] = {
	{
		.compatible = "fsl,mpc8272-usb",
	},
	{},
};

static struct of_platform_driver udc_driver = {
	.name = (char *)driver_name,
	.match_table = mpc8272_udc_match,
	.probe = mpc8272_udc_probe,
	.remove  = mpc8272_udc_remove,
	/* FIXME power management support */
	/* .suspend = ... disable UDC */
	/* .resume = ... re-enable UDC */
};

static int __init udc_init (void)
{
	printk(KERN_INFO "%s (%s)\n", driver_desc, DRIVER_VERSION);
	return of_register_platform_driver(&udc_driver);
}
module_init(udc_init);

static void __exit udc_exit(void)
{
	of_unregister_platform_driver(&udc_driver);
}
module_exit(udc_exit);
