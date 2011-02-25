/*
 * Copyright (C) 2006 Texas Instruments Inc
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option)any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/semaphore.h>
#include <asm/arch/clock.h>
#include <asm/arch/i2c-client.h>
#include <asm/arch/mux.h>
#include <asm/arch/tsif.h>

#define DRIVER_NAME     "tsif_data"

struct tsif_data_dev {
	struct cdev c_dev;
	dev_t devno;
	unsigned int base;
	struct completion rx_complete[9];
	struct completion tx_complete;
	unsigned long tx_offset[9];
	unsigned long rx_offset[9];
	int in_ring_w_count[9];
	u32 pstart[9];
	unsigned long buf_size[9];
	unsigned int one_ring_size;
	unsigned int pkt_size;
};

int dma_stall = 1;
static u32 tsif_int_status;	/* Interrupt Status */
static unsigned int tsif_data_major;
static unsigned int tsif_data_minor_start;
unsigned int tsif_data_minor_count = 9;
unsigned int tsif_device_count = 2;

static DEFINE_SPINLOCK(tsif_data_dev_array_lock);
static struct platform_device *tsif_data_device[2][9];
static struct tsif_data_dev *tsif_data_dev_array[TSIF_MAJORS];
static struct fasync_struct *tsif_async_queue;

char *device_name[] = { "rxfilter0",
	"rxfilter1",
	"rxfilter2",
	"rxfilter3",
	"rxfilter4",
	"rxfilter5",
	"rxfilter6",
	"rxfilter7",
	"txout"
};

char *tsif_interrupt_name[] = { "tsif0_isr",
	"tsif1_isr"
};

struct tsif_data_dev *tsif_data_dev_get_by_major(unsigned int index)
{
	struct tsif_data_dev *tsif_dev;

	spin_lock(&tsif_data_dev_array_lock);
	tsif_dev = tsif_data_dev_array[index];
	spin_unlock(&tsif_data_dev_array_lock);
	return tsif_dev;
}
EXPORT_SYMBOL(tsif_data_dev_get_by_major);

int tsif_data_open(struct inode *inode, struct file *file)
{
	unsigned int index;
	struct tsif_data_dev *tsif_dev;

	index = iminor(inode) / tsif_data_minor_count;
	tsif_dev = tsif_data_dev_get_by_major(index);

	tsif_int_status = 0;

	/* Enable the interrupt to arm */
	davinci_writel((davinci_readl(tsif_dev->base + CTRL1) | 0x8000),
		       tsif_dev->base + CTRL1);

	return 0;
}

int tsif_data_release(struct inode *inode, struct file *file)
{
	unsigned int index;
	struct tsif_data_dev *tsif_dev;

	index = iminor(inode) / tsif_data_minor_count;
	tsif_dev = tsif_data_dev_get_by_major(index);

	davinci_writel(0x0, tsif_dev->base + WRB_CH_CTRL);
	davinci_writel(0x0, tsif_dev->base + RRB_CH_CTRL);

	return 0;
}

void tsif_request_tx_buffer(struct inode *inode, unsigned long arg)
{
	unsigned long adr;
	u32 size;
	unsigned int index, ch;
	struct tsif_data_dev *tsif_dev;

	ch = iminor(inode) % tsif_data_minor_count;
	index = iminor(inode) / tsif_data_minor_count;
	tsif_dev = tsif_data_dev_get_by_major(index);

	get_user(size, (u32 *) arg);
	tsif_dev->tx_offset[ch] =
	    __get_free_pages(GFP_KERNEL | GFP_DMA, get_order(size));

	adr = (unsigned long)tsif_dev->tx_offset[ch];
	size = PAGE_SIZE << (get_order(size));
	while (size > 0) {
		/* make sure the frame buffers
		   are never swapped out of memory */
		SetPageReserved(virt_to_page(adr));
		adr += PAGE_SIZE;
		size -= PAGE_SIZE;
	}
	/* convert virtual address to physical */
	tsif_dev->tx_offset[ch] =
	    (unsigned long)virt_to_phys((void *)(tsif_dev->tx_offset[ch]));

	return;
}

void tsif_query_tx_buffer(struct inode *inode, unsigned long arg)
{
	unsigned int index, ch;
	struct tsif_data_dev *tsif_dev;

	ch = iminor(inode) % tsif_data_minor_count;
	index = iminor(inode) / tsif_data_minor_count;
	tsif_dev = tsif_data_dev_get_by_major(index);

	put_user(tsif_dev->tx_offset[ch], (unsigned long *)arg);

	return;
}

void tsif_request_rx_buffer(struct inode *inode, unsigned long arg)
{
	unsigned long adr;
	u32 size;
	unsigned int index, ch;
	struct tsif_data_dev *tsif_dev;

	ch = iminor(inode) % tsif_data_minor_count;
	index = iminor(inode) / tsif_data_minor_count;
	tsif_dev = tsif_data_dev_get_by_major(index);

	get_user(size, (u32 *) arg);
	tsif_dev->rx_offset[ch] =
	    __get_free_pages(GFP_KERNEL | GFP_DMA, get_order(size));

	adr = (unsigned long)tsif_dev->rx_offset[ch];
	size = PAGE_SIZE << (get_order(size));
	while (size > 0) {
		/* make sure the frame buffers
		   are never swapped out of memory */
		SetPageReserved(virt_to_page(adr));
		adr += PAGE_SIZE;
		size -= PAGE_SIZE;
	}
	/* convert virtual address to physical */
	tsif_dev->rx_offset[ch] =
	    (unsigned long)virt_to_phys((void *)(tsif_dev->rx_offset[ch]));

	return;
}

void tsif_query_rx_buffer(struct inode *inode, unsigned long arg)
{
	unsigned int index, ch;
	struct tsif_data_dev *tsif_dev;

	ch = iminor(inode) % tsif_data_minor_count;
	index = iminor(inode) / tsif_data_minor_count;
	tsif_dev = tsif_data_dev_get_by_major(index);

	put_user(tsif_dev->rx_offset[ch], (unsigned long *)arg);

	return;
}

int tsif_data_mmap(struct file *filp, struct vm_area_struct *vma)
{
	if (remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff,
			    vma->vm_end - vma->vm_start, vma->vm_page_prot))
		return -EAGAIN;

	return 0;
}

int tsif_get_pkt_size(struct inode *inode)
{
	unsigned int index;
	int pkt_size = 0;
	unsigned int ctrl0;
	struct tsif_data_dev *tsif_dev;

	index = iminor(inode) / tsif_data_minor_count;
	tsif_dev = tsif_data_dev_get_by_major(index);

	ctrl0 = davinci_readl(tsif_dev->base + CTRL0);

	if ((ctrl0 & TSIF_CTRL0_RCV_STREAM_MODE_TS_ACTIVE) ||
	    (ctrl0 & TSIF_CTRL0_TX_STREAM_MODE_TS_ACTIVE))
		pkt_size = 192;
	else {
		switch (ctrl0 & TSIF_CTRL0_RCV_PKT_SIZE_MASK) {
		case TSIF_CTRL0_RCV_PKT_SIZE_BYTE_200:
			pkt_size = 200;
			break;
		case TSIF_CTRL0_RCV_PKT_SIZE_BYTE_208:
			pkt_size = 208;
			break;
		case TSIF_CTRL0_RCV_PKT_SIZE_BYTE_216:
			pkt_size = 216;
			break;
		case TSIF_CTRL0_RCV_PKT_SIZE_BYTE_224:
			pkt_size = 224;
			break;
		case TSIF_CTRL0_RCV_PKT_SIZE_BYTE_232:
			pkt_size = 232;
			break;
		case TSIF_CTRL0_RCV_PKT_SIZE_BYTE_240:
			pkt_size = 240;
			break;
		case TSIF_CTRL0_RCV_PKT_SIZE_BYTE_248:
			pkt_size = 248;
			break;
		case TSIF_CTRL0_RCV_PKT_SIZE_BYTE_256:
			pkt_size = 256;
			break;
		default:
			return -EINVAL;
		}
	}
	return pkt_size;
}
EXPORT_SYMBOL(tsif_get_pkt_size);

int tsif_set_pid_filter_config(struct inode *inode, unsigned long arg)
{
	struct tsif_data_dev *tsif_dev;
	struct tsif_pid_filter_config pid_filter_cfg;
	u16 ch = iminor(inode) % tsif_data_minor_count;
	u32 wrb_ch_ctrl, inten, index;

	index = iminor(inode) / tsif_data_minor_count;
	tsif_dev = tsif_data_dev_get_by_major(index);

	/* copy the parameters to the configuration */
	if (copy_from_user(&pid_filter_cfg,
			   (struct tsif_pid_filter_config *)arg,
			   sizeof(struct tsif_pid_filter_config)))
		return -EFAULT;

	outl(0x00000000, tsif_pid_flt_cfg[index][ch]);
	wrb_ch_ctrl = davinci_readl(tsif_dev->base + WRB_CH_CTRL);
	wrb_ch_ctrl &= ~(TSIF_RING_BUF_WR_CH_CTL_EN_ACTIVATE << (ch * 4));
	davinci_writel(wrb_ch_ctrl, tsif_dev->base + WRB_CH_CTRL);
	inten = davinci_readl(tsif_dev->base + INTEN);
	inten &= ~(TSIF_INTEN_RBW0_FULL_INTEN << ch);
	davinci_writel(inten, tsif_dev->base + INTEN);

	outl((TSIF_PID_FILTER_ENABLE_MASK |
	      ((u32) pid_filter_cfg.stream_type << 16) |
	      ((u32) pid_filter_cfg.pid)),  tsif_pid_flt_cfg[index][ch]);
	return 0;
}

/*
 * This function is used to convert user space virtual address
 * to physical address.
 */
unsigned int tsif_user_virt_to_phys(unsigned int virt)
{
	pgd_t *pgd;
	pmd_t *pmd;
	pte_t *pte;

	struct mm_struct *mm = current->mm;
	pgd = pgd_offset(mm, virt);
	if (!(pgd_none(*pgd) || pgd_bad(*pgd))) {
		pmd = pmd_offset(pgd, virt);

		if (!(pmd_none(*pmd) || pmd_bad(*pmd))) {
			pte = pte_offset_kernel(pmd, virt);

			if (pte_present(*pte)) {
				return __pa(page_address(pte_page(*pte))
						+ (virt & ~PAGE_MASK));
			}
		}
	}

	return 0;
}
EXPORT_SYMBOL(tsif_user_virt_to_phys);

int tsif_data_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
		    unsigned long arg)
{
	int pkt_size = 0, ret = 0;
	unsigned int index, minor = iminor(inode) % tsif_data_minor_count;
	struct tsif_data_dev *tsif_dev;

	index = iminor(inode) / tsif_data_minor_count;
	tsif_dev = tsif_data_dev_get_by_major(index);

	switch (cmd) {
	case TSIF_REQ_TX_BUF:{
			tsif_request_tx_buffer(inode, arg);
			break;
		}

	case TSIF_QUERY_TX_BUF:{
			tsif_query_tx_buffer(inode, arg);
			break;
		}

	case TSIF_REQ_RX_BUF:{
			tsif_request_rx_buffer(inode, arg);
			break;
		}

	case TSIF_QUERY_RX_BUF:{
			tsif_query_rx_buffer(inode, arg);
			break;
		}

	case TSIF_GET_READ_AVAIL:{
			u16 ch = iminor(inode) % tsif_data_minor_count;
			unsigned int index;
			struct tsif_read_avail read_avail;
			u32 size, wrb_rp_add;
			u32 start_addr, end_addr;

			index = iminor(inode) / tsif_data_minor_count;
			tsif_dev = tsif_data_dev_get_by_major(index);

			/* copy the parameters  */
			if (copy_from_user(&read_avail,
					   (struct tsif_read_avail *)arg,
					   sizeof(struct tsif_read_avail)))
				return -EFAULT;

			start_addr = inl(tsif_wrb_start_addr[index][ch]);
			end_addr = inl(tsif_wrb_end_addr[index][ch]);
			/*
			 * This condition will nver be met, if the user does
			 * not piggy back the acknowledgment and sets directly
			 * moves the read pointer
			 */
			if (read_avail.bytes_last_read) {
				wrb_rp_add = inl(tsif_wrb_read_addr[index][ch]);
				wrb_rp_add += read_avail.bytes_last_read;
				/* Need to add a logic when the bytes read is
				   more than the last pointer */
				/* if the data read needs a loopback of the read
				   pointer */
				/*if (wrb_rp_add >= end_addr) */
				if (wrb_rp_add > end_addr)
					wrb_rp_add = start_addr;
				outl(wrb_rp_add, tsif_wrb_read_addr[index][ch]);
			}

			size = end_addr - start_addr;
			wrb_rp_add = inl(tsif_wrb_read_addr[index][ch]);

			if (wrb_rp_add >
			    inl(tsif_wrb_present_addr[index][ch])) {
				read_avail.len =
				    size - wrb_rp_add +
				    inl(tsif_wrb_present_addr[index][ch]);
				dma_stall = 0;
			} else if (wrb_rp_add <
				   inl(tsif_wrb_present_addr[index][ch])) {
				read_avail.len =
				    inl(tsif_wrb_present_addr[index][ch]) -
				    wrb_rp_add;
				dma_stall = 0;
			} else {
				/* Both are equal. This might be caused if
				   there is no new data or if the queue is full.
				   Currently not handled. Add the logic once
				   previous things start working */
				if (dma_stall)
					read_avail.len = 0;
				else
					read_avail.len = size;
			}

			pkt_size = tsif_get_pkt_size(inode);
			read_avail.len = (read_avail.len / pkt_size) * pkt_size;

			/* copy the configuration to the user space */
			if (copy_to_user((struct tsif_read_avail *)arg,
					 &read_avail,
					 sizeof(struct tsif_read_avail)))
				return -EFAULT;

			break;
		}

	case TSIF_GET_WRITE_AVAIL:{
			struct tsif_write_avail write_avail;
			u32 size, rrb0_wp_add;
			u32 start_addr, end_addr;
			int pkt_size;

			/* copy the parameters  */
			if (copy_from_user(&write_avail,
					   (struct tsif_write_avail *)arg,
					   sizeof(struct tsif_write_avail)))
				return -EFAULT;

			start_addr =
			    davinci_readl(tsif_dev->base + RRB0_STRT_ADD);
			end_addr = davinci_readl(tsif_dev->base + RRB0_END_ADD);
			pkt_size = tsif_get_pkt_size(inode);

			if (write_avail.bytes_last_wrote) {
				rrb0_wp_add =
				    davinci_readl(tsif_dev->base + RRB0_WP_ADD);
				rrb0_wp_add += write_avail.bytes_last_wrote;
				if (rrb0_wp_add > end_addr)
					rrb0_wp_add = start_addr;
				davinci_writel(rrb0_wp_add,
					       tsif_dev->base + RRB0_WP_ADD);
			}

			size = end_addr - start_addr;

			rrb0_wp_add =
			    davinci_readl(tsif_dev->base + RRB0_WP_ADD);

			if (davinci_readl(tsif_dev->base + RRB0_RP_ADD) >
			    davinci_readl(tsif_dev->base + RRB0_WP_ADD))
				write_avail.len =
				    davinci_readl(tsif_dev->base +
						  RRB0_RP_ADD) -
				    davinci_readl(tsif_dev->base + RRB0_WP_ADD);
			else if (rrb0_wp_add ==
				 davinci_readl(tsif_dev->base + RRB0_RP_ADD)) {
				write_avail.len = size - pkt_size;
			} else
				write_avail.len =
				    size - rrb0_wp_add +
				    davinci_readl(tsif_dev->base + RRB0_RP_ADD);

			write_avail.len =
			    (write_avail.len / pkt_size) * pkt_size;

			/* copy the configuration to the user space */
			if (copy_to_user((struct tsif_write_avail *)arg,
					 &write_avail,
					 sizeof(struct tsif_write_avail)))
				return -EFAULT;
			break;
		}

	case TSIF_SET_READ_AVAIL:{
			struct tsif_read_avail read_avail;
			u32 wrb_rp_add;
			u32 start_addr, end_addr;

			/* copy the parameters  */
			if (copy_from_user(&read_avail,
					   (struct tsif_read_avail *)arg,
					   sizeof(struct tsif_read_avail)))
				return -EFAULT;

			start_addr = inl(tsif_wrb_start_addr[index][minor]);
			end_addr = inl(tsif_wrb_end_addr[index][minor]);

			wrb_rp_add = inl(tsif_wrb_read_addr[index][minor]);
			wrb_rp_add += read_avail.bytes_last_read;

			if (wrb_rp_add == end_addr)
				wrb_rp_add = start_addr;

			/* This is a split condition. */
			if (wrb_rp_add > end_addr) {
				wrb_rp_add -= end_addr;
				wrb_rp_add += start_addr;
			}

			outl(wrb_rp_add, tsif_wrb_read_addr[index][minor]);
			break;

		}		/* case TSIF_SET_READ_AVAIL */

	case TSIF_SET_WRITE_AVAIL:{
			struct tsif_write_avail write_avail;
			u32 rrb0_wp_add;
			u32 start_addr, end_addr;

			/* copy the parameters  */
			if (copy_from_user(&write_avail,
					   (struct tsif_write_avail *)arg,
					   sizeof(struct tsif_write_avail)))
				return -EFAULT;

			start_addr =
			    davinci_readl(tsif_dev->base + RRB0_STRT_ADD);
			end_addr = davinci_readl(tsif_dev->base + RRB0_END_ADD);

			rrb0_wp_add =
			    davinci_readl(tsif_dev->base + RRB0_WP_ADD);
			/* Initialize the write pointer for the first time when
			   the data is moved */
			rrb0_wp_add += write_avail.bytes_last_wrote;

			if (rrb0_wp_add == end_addr) {
				rrb0_wp_add = start_addr;
			} else if (rrb0_wp_add > end_addr) {
				/* This is a split case */
				rrb0_wp_add -= end_addr;
				rrb0_wp_add += start_addr;
			}

			davinci_writel(rrb0_wp_add,
				       tsif_dev->base + RRB0_WP_ADD);
			break;

		}		/* case TSIF_SET_WRITE_AVAIL */

	case TSIF_RX_RING_BUF_CONFIG:{
			struct tsif_rx_ring_buf_config rx_ring_buf_cfg;
			u32 inten = 0, wrb_ch_ctrl = 0;
			int pkt_size = tsif_get_pkt_size(inode);

			/* copy the parameters to the configuration */
			if (copy_from_user(&rx_ring_buf_cfg,
					   (struct tsif_rx_ring_buf_config *)
					   arg,
					   sizeof(struct
						  tsif_rx_ring_buf_config)))
				return -EFAULT;

			if (rx_ring_buf_cfg.pstart != NULL) {
				rx_ring_buf_cfg.pread =
				    rx_ring_buf_cfg.pstart +
				    (pkt_size * rx_ring_buf_cfg.buf_size / 3) *
				    2;
				rx_ring_buf_cfg.pread =
				    (char *)tsif_user_virt_to_phys(
				    (unsigned int)rx_ring_buf_cfg.pread);

				rx_ring_buf_cfg.pstart =
				    (char *)tsif_user_virt_to_phys(
				    (unsigned int)rx_ring_buf_cfg.pstart);

				outl((u32) rx_ring_buf_cfg.pstart,
				     tsif_wrb_start_addr[index][minor]);
				outl((u32) (rx_ring_buf_cfg.pstart +
				     (pkt_size * rx_ring_buf_cfg.buf_size)),
				     tsif_wrb_end_addr[index][minor]);
				outl((u32) rx_ring_buf_cfg.pread,
				     tsif_wrb_read_addr[index][minor]);

				tsif_dev->pstart[minor] =
				    inl(tsif_wrb_start_addr[index][minor]);
				tsif_dev->buf_size[minor] =
				    pkt_size * rx_ring_buf_cfg.buf_size;
				tsif_dev->one_ring_size =
				    (rx_ring_buf_cfg.buf_size / 3);
				tsif_dev->pkt_size = pkt_size;
				tsif_dev->in_ring_w_count[minor] = 2;

				outl((pkt_size * tsif_dev->one_ring_size - 8),
				     (tsif_wrb_subtraction[index][minor]));

				/* Clear RBW_full */
				davinci_writel(
				(TSIF_INTEN_CLR_RBW0_FULL_INTEN_CLR << minor),
				tsif_dev->base + INT_STATUS_CLR);
				/* RBW_full_inten<-1 */
				inten = davinci_readl(tsif_dev->base + INTEN);
				inten |= (TSIF_INTEN_RBW0_FULL_INTEN << minor);
				davinci_writel(inten, tsif_dev->base + INTEN);

				/* RBW_full_inten_set<-1 */
				inten =
				    davinci_readl(tsif_dev->base + INTEN_SET);
				inten |=
				    (TSIF_INTEN_SET_RBW0_FULL_INTEN_SET <<
				     minor);
				davinci_writel(inten,
					       tsif_dev->base + INTEN_SET);

				/* Enable ring buffer write channel 7 */
				wrb_ch_ctrl =
				    davinci_readl(tsif_dev->base + WRB_CH_CTRL);
				wrb_ch_ctrl |= (1 << (minor * 4));
				davinci_writel(wrb_ch_ctrl,
					       tsif_dev->base + WRB_CH_CTRL);
			}
			break;
		}		/* case TSIF_RX_RING_BUF_CONFIG */

	case TSIF_TX_RING_BUF_CONFIG:{
			struct tsif_tx_ring_buf_config tx_ring_buf_cfg;
			int pkt_size = tsif_get_pkt_size(inode);

			/* copy the parameters to the configuration */
			if (copy_from_user(&tx_ring_buf_cfg,
					   (struct tsif_tx_ring_buf_config *)
					   arg,
					   sizeof(struct
						  tsif_tx_ring_buf_config)))
				return -EFAULT;

			if (tx_ring_buf_cfg.pstart != NULL) {
				tx_ring_buf_cfg.pstart =
				    (char *)tsif_user_virt_to_phys(
				    (unsigned int)tx_ring_buf_cfg.pstart);

				davinci_writel((u32) tx_ring_buf_cfg.pstart,
					       tsif_dev->base + RRB0_STRT_ADD);
				davinci_writel((u32) tx_ring_buf_cfg.pstart +
					       (pkt_size *
						tx_ring_buf_cfg.buf_size),
					       tsif_dev->base + RRB0_END_ADD);
				davinci_writel((u32) tx_ring_buf_cfg.pstart,
					       tsif_dev->base + RRB0_WP_ADD);
				davinci_writel((u32) tx_ring_buf_cfg.psub,
					       tsif_dev->base + RRB0_SUB_ADD);

				/* Enable ring buffer read channel 0 */
				davinci_writel(0x00000001,
					       tsif_dev->base + RRB_CH_CTRL);
			}
			break;
		}		/* case TSIF_TX_RING_BUF_CONFIG */

	case TSIF_GET_RX_BUF_STATUS:{
			struct tsif_buf_status rx_buf_status;
			u16 ch = iminor(inode) % tsif_data_minor_count;
			unsigned int index;

			index = iminor(inode) / tsif_data_minor_count;

			rx_buf_status.pstart =
				(u8 *) inl(tsif_wrb_start_addr[index][ch]);
			rx_buf_status.pend =
				(u8 *) inl(tsif_wrb_end_addr[index][ch]);
			rx_buf_status.pread_write =
				(u8 *) inl(tsif_wrb_read_addr[index][ch]);
			rx_buf_status.pcurrent =
				(u8 *) inl(tsif_wrb_present_addr[index][ch]);
			rx_buf_status.psub =
				(u32) inl(tsif_wrb_subtraction[index][ch]);
			/* copy the configuration to the user space */
			if (copy_to_user((struct tsif_buf_status *)arg,
					 &rx_buf_status,
					 sizeof(struct tsif_buf_status)))
				return -EFAULT;
			break;
		}

	case TSIF_GET_TX_BUF_STATUS:{
			struct tsif_buf_status tx_buf_status;

			tx_buf_status.pstart =
			   (u8 *) davinci_readl(tsif_dev->base + RRB0_STRT_ADD);
			tx_buf_status.pend =
			    (u8 *) davinci_readl(tsif_dev->base + RRB0_END_ADD);
			tx_buf_status.pread_write =
			     (u8 *) davinci_readl(tsif_dev->base + RRB0_WP_ADD);
			tx_buf_status.pcurrent =
			     (u8 *) davinci_readl(tsif_dev->base + RRB0_RP_ADD);
			tx_buf_status.psub =
			     (u32) davinci_readl(tsif_dev->base + RRB0_SUB_ADD);
			/* copy the configuration to the user space */
			if (copy_to_user((struct tsif_buf_status *)arg,
					 &tx_buf_status,
					 sizeof(struct tsif_buf_status)))
				return -EFAULT;
			break;
		}

	case TSIF_WAIT_FOR_RX_COMPLETE:{
			wait_for_completion_interruptible(&tsif_dev->
							  rx_complete
							  [minor]);
			return 0;
			break;
		}		/* case TSIF_WAIT_FOR_RX_COMPLETE */

	case TSIF_WAIT_FOR_TX_COMPLETE:{
			wait_for_completion_interruptible(&tsif_dev->
							tx_complete);
			return 0;
			break;
		}		/* case TSIF_WAIT_FOR_TX_COMPLETE */

	case TSIF_SET_PID_FILTER_CONFIG:{
			tsif_set_pid_filter_config(inode, arg);
			break;
		}

	case TSIF_GET_PID_FILTER_CONFIG:{
			u16 ch = iminor(inode) % tsif_data_minor_count;
			u32 flt_cfg, index;
			struct tsif_pid_filter_config pid_filter_cfg;

			index = iminor(inode) / tsif_data_minor_count;
			flt_cfg = inl(tsif_pid_flt_cfg[index][ch]);
			if ((flt_cfg & TSIF_PID_FILTER_ENABLE_MASK) != 0) {
				pid_filter_cfg.pid = (u16) (flt_cfg & 0x1fff);
				pid_filter_cfg.stream_type =
					(u8) (flt_cfg >> 16);
			} else {
				pid_filter_cfg.pid = 0x2000;
				pid_filter_cfg.stream_type = 0x00;
			}
			/* copy the configuration to the user space */
			if (copy_to_user((struct tsif_pid_filter_config *)arg,
					 &pid_filter_cfg,
					 sizeof(struct tsif_pid_filter_config)))
				return -EFAULT;
			break;
		}
	}			/* switch */

	return ret;
}

int tsif_fasync(int fd, struct file *filp, int mode)
{
	/* Hook up the our file descriptor to the signal */
	return fasync_helper(fd, filp, mode, &tsif_async_queue);
}

static struct file_operations tsif_data_fops = {
	.owner = THIS_MODULE,
	.open = tsif_data_open,
	.release = tsif_data_release,
	.ioctl = tsif_data_ioctl,
	.mmap = tsif_data_mmap,
	.fasync = tsif_fasync,
};

static int tsif_remove(struct device *device)
{
	return 0;
}

static void tsif_platform_release(struct device *device)
{
	/* This function does nothing */
}

static struct device_driver tsif_driver = {
	.name = "tsif_data",
	.bus = &platform_bus_type,
	.remove = tsif_remove,
};

irqreturn_t tsif_isr(int irq, void *dev_id, struct pt_regs *regs)
{
	struct tsif_data_dev *tsif_dev = dev_id;
	u32 int_status = 0;
	u32 rcv_status = 0;
	unsigned long flags;

	local_irq_save(flags);
	int_status = davinci_readl(tsif_dev->base + INT_STATUS);
	rcv_status = davinci_readl(tsif_dev->base + RCV_PKT_STAT);
	davinci_writel(int_status, tsif_dev->base + INT_STATUS_CLR);
	davinci_writel(rcv_status, tsif_dev->base + INT_STATUS_CLR);

	if (int_status & 0x8000) {
		/* printk ("## PID7 Interrupt\n"); */
		tsif_dev->in_ring_w_count[7]++;
		if (tsif_dev->in_ring_w_count[7] == 3)
			tsif_dev->in_ring_w_count[7] = 0;
		davinci_writel(tsif_dev->pstart[7] +
			       (tsif_dev->one_ring_size *
				tsif_dev->pkt_size *
				tsif_dev->in_ring_w_count[7]),
			       tsif_dev->base + WRB7_RP_ADD);
		complete(&tsif_dev->rx_complete[7]);
	}

	if (int_status & 0x4000) {
		/* printk ("## PID6 Interrupt\n"); */
		tsif_dev->in_ring_w_count[6]++;
		if (tsif_dev->in_ring_w_count[6] == 3)
			tsif_dev->in_ring_w_count[6] = 0;
		davinci_writel(tsif_dev->pstart[6] +
			       (tsif_dev->one_ring_size *
				tsif_dev->pkt_size *
				tsif_dev->in_ring_w_count[6]),
			       tsif_dev->base + WRB6_RP_ADD);
		complete(&tsif_dev->rx_complete[6]);
	}

	if (int_status & 0x2000) {
		/* printk ("## PID5 Interrupt\n"); */
		tsif_dev->in_ring_w_count[5]++;
		if (tsif_dev->in_ring_w_count[5] == 3)
			tsif_dev->in_ring_w_count[5] = 0;
		davinci_writel(tsif_dev->pstart[5] +
			       (tsif_dev->one_ring_size *
				tsif_dev->pkt_size *
				tsif_dev->in_ring_w_count[5]),
			       tsif_dev->base + WRB5_RP_ADD);
		complete(&tsif_dev->rx_complete[5]);
	}

	if (int_status & 0x1000) {
		/* printk ("## PID4 Interrupt\n"); */
		tsif_dev->in_ring_w_count[4]++;
		if (tsif_dev->in_ring_w_count[4] == 3)
			tsif_dev->in_ring_w_count[4] = 0;
		davinci_writel(tsif_dev->pstart[4] +
			       (tsif_dev->one_ring_size *
				tsif_dev->pkt_size *
				tsif_dev->in_ring_w_count[4]),
			       tsif_dev->base + WRB4_RP_ADD);
		complete(&tsif_dev->rx_complete[4]);
	}

	if (int_status & 0x800) {
		/* printk ("## PID3 Interrupt\n"); */
		tsif_dev->in_ring_w_count[3]++;
		if (tsif_dev->in_ring_w_count[3] == 3)
			tsif_dev->in_ring_w_count[3] = 0;
		davinci_writel(tsif_dev->pstart[3] +
			       (tsif_dev->one_ring_size *
				tsif_dev->pkt_size *
				tsif_dev->in_ring_w_count[3]),
			       tsif_dev->base + WRB3_RP_ADD);
		complete(&tsif_dev->rx_complete[3]);
	}

	if (int_status & 0x400) {
		/* printk ("## PID2 Interrupt\n"); */
		tsif_dev->in_ring_w_count[2]++;
		if (tsif_dev->in_ring_w_count[2] == 3)
			tsif_dev->in_ring_w_count[2] = 0;
		davinci_writel(tsif_dev->pstart[2] +
			       (tsif_dev->one_ring_size *
				tsif_dev->pkt_size *
				tsif_dev->in_ring_w_count[2]),
			       tsif_dev->base + WRB2_RP_ADD);
		complete(&tsif_dev->rx_complete[2]);
	}

	if (int_status & 0x200) {
		/* printk ("## PID1 Interrupt\n"); */
		tsif_dev->in_ring_w_count[1]++;
		if (tsif_dev->in_ring_w_count[1] == 3)
			tsif_dev->in_ring_w_count[1] = 0;
		davinci_writel(tsif_dev->pstart[1] +
			       (tsif_dev->one_ring_size *
				tsif_dev->pkt_size *
				tsif_dev->in_ring_w_count[1]),
			       tsif_dev->base + WRB1_RP_ADD);
		complete(&tsif_dev->rx_complete[1]);
	}

	if (int_status & 0x100) {
		/* printk ("## PID0 Interrupt\n"); */
		tsif_dev->in_ring_w_count[0]++;
		if (tsif_dev->in_ring_w_count[0] == 3)
			tsif_dev->in_ring_w_count[0] = 0;
		davinci_writel(tsif_dev->pstart[0] +
			       (tsif_dev->one_ring_size *
				tsif_dev->pkt_size *
				tsif_dev->in_ring_w_count[0]),
			       tsif_dev->base + WRB0_RP_ADD);
		complete(&tsif_dev->rx_complete[0]);
	}

	if (int_status & 0x40) {
		/* Notify all the waiting applications using SIGIO */
		kill_fasync(&tsif_async_queue, SIGIO, POLL_IN);
	}

	if (int_status & 0x20) {
		/* Notify all the waiting applications using SIGIO */
		kill_fasync(&tsif_async_queue, SIGIO, POLL_IN);
	}

	if (int_status & 0x10) {
		/*printk ("### RX Buf Full\n");*/
		complete(&tsif_dev->tx_complete);
	}

	if (int_status & 0x00000004) {
		/* printk ("### PAT Interrupt Received\n"); */
		tsif_pat_complete_cmd();
	}

	if (int_status & 0x00000008) {
		/* printk ("### PMT Interrupt Received\n"); */
		tsif_pmt_complete_cmd();
	}
	if (int_status & 0x00000002) {
		/* printk ("### GOP Interrupt Received\n"); */
		kill_fasync(&tsif_async_queue, SIGIO, POLL_IN);
	}

	if (int_status & 0x00000001) {
		/* printk ("### BSP Interrupt Received\n"); */
		tsif_spcpkt_complete_cmd();
	}
	local_irq_restore(flags);
	return IRQ_HANDLED;
}

static struct class *tsif_data_class;

static int __init tsif_data_init(void)
{
	struct platform_device *pldev;
	int result, ret = 0;
	dev_t devno;
	unsigned int size = 0, mem_size, i, j;

	set_tsif_clk(TSIF_16_875_MHZ_SERIAL_PARALLEL);
	davinci_cfg_reg(DM646X_PTSOMUX_PARALLEL, PINMUX_RESV);
	davinci_cfg_reg(DM646X_PTSIMUX_PARALLEL, PINMUX_RESV);

	size = tsif_device_count * tsif_data_minor_count;
	/* Register the driver in the kernel */
	result = alloc_chrdev_region(&devno, 0, size, DRIVER_NAME);
	if (result < 0) {
		printk(KERN_ERR "TSIF: Module intialization failed.\
			could not register character device\n");
		result = -ENODEV;
		goto init_fail;
	}
	tsif_data_major = MAJOR(devno);

	mem_size = sizeof(struct tsif_data_dev);

	for (i = 0; i < tsif_device_count; i++) {
		tsif_data_dev_array[i] = kmalloc(mem_size, GFP_KERNEL);
		/* Initialize of character device */
		cdev_init(&tsif_data_dev_array[i]->c_dev, &tsif_data_fops);
		tsif_data_dev_array[i]->c_dev.owner = THIS_MODULE;
		tsif_data_dev_array[i]->c_dev.ops = &tsif_data_fops;

		devno =
		    MKDEV(tsif_data_major,
			  i * tsif_data_minor_count + tsif_data_minor_start);

		/* addding character device */
		result =
		    cdev_add(&tsif_data_dev_array[i]->c_dev, devno,
			     tsif_data_minor_count);
		if (result) {
			printk("TSIF:Error adding TSIF\n");
			unregister_chrdev_region(devno, size);
			goto init_fail;
		}
		tsif_data_dev_array[i]->devno = i;
	}

	tsif_data_class = class_create(THIS_MODULE, "tsif_data");
	if (!tsif_data_class) {
		for (i = 0; i < tsif_device_count; i++)
			cdev_del(&tsif_data_dev_array[i]->c_dev);
		result = -EIO;
		goto init_fail;
	}

	/* register driver as a platform driver */
	if (driver_register(&tsif_driver) != 0) {
		for (i = 0; i < tsif_device_count; i++) {
			unregister_chrdev_region(devno, size);
			cdev_del(&tsif_data_dev_array[i]->c_dev);
			kfree(tsif_data_dev_array[i]);
		}
		result = -EINVAL;
		goto init_fail;
	}

	for (i = 0; i < tsif_device_count; i++) {
		for (j = 0; j < tsif_data_minor_count; j++) {
			char name[50];
			/* Register the drive as a platform device */
			tsif_data_device[i][j] = NULL;

			pldev = kmalloc(sizeof(*pldev), GFP_KERNEL);
			if (!pldev)
				continue;

			memset(pldev, 0, sizeof(*pldev));
			pldev->name = *(device_name + j);
			pldev->id = i;
			pldev->dev.release = tsif_platform_release;
			tsif_data_device[i][j] = pldev;

			if (platform_device_register(pldev) != 0) {
				driver_unregister(&tsif_driver);
				unregister_chrdev_region(devno, size);
				cdev_del(&tsif_data_dev_array[i]->c_dev);
				kfree(pldev);
				kfree(tsif_data_dev_array[i]);
				result = -EINVAL;
				goto init_fail;
			}

			devno =
			    MKDEV(tsif_data_major,
				  tsif_data_minor_start + i *
				  tsif_data_minor_count + j);
			ret = sprintf(name, "hd_tsif%d_", i);
			sprintf(&name[ret], *(device_name + j));
			class_device_create(tsif_data_class, NULL, devno,
						NULL, name);
			init_completion(&tsif_data_dev_array[i]->
					rx_complete[j]);
			tsif_data_dev_array[i]->rx_complete[j].done = 0;
		}
		tsif_data_dev_array[i]->base =
		    (unsigned int)(TSIF_BASE + i * 0x400);
		request_irq(IRQ_DM646X_TSIFINT0 + i, tsif_isr, SA_INTERRUPT,
			    tsif_interrupt_name[i], tsif_data_dev_array[i]);
		init_completion(&tsif_data_dev_array[i]->tx_complete);
		tsif_data_dev_array[i]->tx_complete.done = 0;
	}

	return 0;
init_fail:
	davinci_cfg_reg(DM646X_PTSOMUX_PARALLEL, PINMUX_FREE);
	davinci_cfg_reg(DM646X_PTSIMUX_PARALLEL, PINMUX_FREE);

	return result;
}

static void __exit tsif_data_exit(void)
{
	dev_t devno;
	unsigned int size, i,  j;

	/* Release major/minor numbers */
	if (tsif_data_major != 0) {
		devno = MKDEV(tsif_data_major, tsif_data_minor_start);
		size = tsif_device_count * tsif_data_minor_count;
		unregister_chrdev_region(devno, size);
	}

	for (i = 0; i < tsif_device_count; i++) {
		for (j = 0; j < tsif_data_minor_count; j++) {
			devno =
			    MKDEV(tsif_data_major,
				  tsif_data_minor_start + i *
				  tsif_data_minor_count + j);
			class_device_destroy(tsif_data_class, devno);
			platform_device_unregister(tsif_data_device[i][j]);
			kfree(tsif_data_device[i][j]);
		}
	}

	if (tsif_data_class != NULL) {
		size = tsif_device_count * tsif_data_minor_count;
		for (i = 0; i < tsif_device_count; i++) {
			free_irq(IRQ_DM646X_TSIFINT0 + i,
				 tsif_data_dev_array[i]);
			cdev_del(&tsif_data_dev_array[i]->c_dev);
			kfree(tsif_data_dev_array[i]);
		}
		class_destroy(tsif_data_class);
		driver_unregister(&tsif_driver);
	}
	davinci_cfg_reg(DM646X_PTSOMUX_PARALLEL, PINMUX_FREE);
	davinci_cfg_reg(DM646X_PTSIMUX_PARALLEL, PINMUX_FREE);
}

module_init(tsif_data_init);
module_exit(tsif_data_exit);

MODULE_DESCRIPTION("TSIF Driver");
MODULE_AUTHOR("Texas Instruments");
MODULE_LICENSE("GPL");
