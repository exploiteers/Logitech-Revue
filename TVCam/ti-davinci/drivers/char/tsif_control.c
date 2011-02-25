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
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
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
#include <linux/clk.h>
#include <asm/arch/i2c-client.h>
#include <asm/arch/mux.h>
#include <asm/arch/tsif.h>
#include <asm/arch/clock.h>

#define DRIVER_NAME     "tsif_control"

struct tsif_control_dev {
	struct cdev c_dev;
	dev_t devno;
	unsigned int base;
	unsigned long tx_offset;
	unsigned long rx_offset;
	struct clk *tsif_clk;
};

static u8 tsif_tx_enable;	/* Transmit Enable Flag */
static u8 tsif_tx_ats_init;	/* tx_ats_init Flag */
static u8 tsif_rx_enable;	/* Receive Enable Flag */
static u8 tsif_if_parallel;	/* Is currently using the parallel interface */
static u32 tsif_int_status;	/* Interrupt Status */
static unsigned int tsif_control_major;
static DEFINE_SPINLOCK(tsif_control_dev_array_lock);
static unsigned int tsif_control_minor_start;
unsigned int tsif_control_minor_count = 3;
static struct tsif_control_dev *tsif_control_dev_array[TSIF_MAJORS];

static struct platform_device *tsif_control_device[2][3];

struct tsif_irq_data tsif_isr_data;

char *control_dev_name[] = { "rxcontrol",
	"txcontrol",
	"ats"
};

char *tsif_clk_name[] = { "TSIF0_CLK", "TSIF1_CLK" };

static unsigned char tsif_output_clk_freq[5][5] = {
	{8, 0x00, 0x40, 0x02, 0x08}, /* 13.5 MHz */
	{8, 0x9F, 0xB0, 0x02, 0x89}, /* 16.875 MHz */
	{4, 0x00, 0x40, 0x02, 0x08}, /* 27 MHz */
	{4, 0xFF, 0x80, 0x02, 0x07}, /* 54 MHz */
	{2, 0xBF, 0xA0, 0x03, 0x0A}, /* 81 MHz */
};

struct tsif_control_dev *tsif_control_dev_get_by_major(unsigned int index)
{
	struct tsif_control_dev *tsif_dev;

	spin_lock(&tsif_control_dev_array_lock);
	tsif_dev = tsif_control_dev_array[index];
	spin_unlock(&tsif_control_dev_array_lock);
	return tsif_dev;
}
EXPORT_SYMBOL(tsif_control_dev_get_by_major);

int tsif_stop_rx(struct inode *inode)
{
	unsigned int index, minor = iminor(inode);
	struct tsif_control_dev *tsif_dev;
	unsigned int ctrl0;

	index = minor / tsif_control_minor_count;
	tsif_dev = tsif_control_dev_get_by_major(index);
	ctrl0 = davinci_readl(tsif_dev->base + CTRL0);
	ctrl0 &= ~(TSIF_CTRL0_RCV_DMA_CTL | TSIF_CTRL0_RCV_CTL);
	davinci_writel(ctrl0, tsif_dev->base + CTRL0);
	tsif_rx_enable = 0;

	return 0;
}
EXPORT_SYMBOL(tsif_stop_rx);

int tsif_set_pat_config(struct inode *inode, unsigned long arg)
{
	char *pat_cfg_buff_ptr;
	u32 inten;
	struct tsif_control_dev *tsif_dev;
	unsigned int index, minor = iminor(inode);

	index = minor / tsif_control_minor_count;
	tsif_dev = tsif_control_dev_get_by_major(index);

	davinci_writel(TSIF_PAT_SENSE_CFG_RESETVAL,
		       tsif_dev->base + PAT_SENSE_CFG);

	inten = davinci_readl(tsif_dev->base + INTEN);
	inten &= ~TSIF_INTEN_PAT_DETECT_INTEN;
	davinci_writel(inten, tsif_dev->base + INTEN);

	pat_cfg_buff_ptr = (char *)virt_to_phys(kmalloc(192, GFP_KERNEL));

	if (pat_cfg_buff_ptr == NULL)
		return -1;

	davinci_writel((u32) pat_cfg_buff_ptr, tsif_dev->base + PAT_STORE_ADD);

	/* Clear PAT_detect */
	davinci_writel(TSIF_INTEN_CLR_PAT_DETECT_INTEN_CLR,
		       tsif_dev->base + INT_STATUS_CLR);

	davinci_writel(TSIF_PAT_SENSE_CFG_PAT_SENSE_EN,
		       tsif_dev->base + PAT_SENSE_CFG);

	/* PAT_detect_inten<-1 */
	inten = davinci_readl(tsif_dev->base + INTEN);
	inten |= TSIF_INTEN_SET_PAT_DETECT_INTEN_SET;
	davinci_writel(inten, tsif_dev->base + INTEN);

	/* PAT_detect_inten_set<-1 */
	inten = davinci_readl(tsif_dev->base + INTEN_SET);
	inten |= TSIF_INTEN_SET_PAT_DETECT_INTEN_SET;
	davinci_writel(inten, tsif_dev->base + INTEN_SET);

	init_completion(&tsif_isr_data.pat_complete);
	tsif_isr_data.pat_complete.done = 0;

	return 0;
}

int tsif_set_pmt_config(struct inode *inode, unsigned long arg)
{
	struct tsif_pmt_config pmt_cfg;
	char *pmt_cfg_buff_ptr;
	u32 inten;
	struct tsif_control_dev *tsif_dev;
	unsigned int index, minor = iminor(inode);

	index = minor / tsif_control_minor_count;
	tsif_dev = tsif_control_dev_get_by_major(index);

	/* copy the parameters to the configuration */
	if (copy_from_user(&pmt_cfg,
			   (struct tsif_pmt_config *)arg,
			   sizeof(struct tsif_pmt_config)))
		return -EFAULT;

	davinci_writel(TSIF_PMT_SENSE_CFG_RESETVAL,
		       tsif_dev->base + PMT_SENSE_CFG);
	inten = davinci_readl(tsif_dev->base + INTEN);
	inten &= ~TSIF_INTEN_PMT_DETECT_INTEN;
	davinci_writel(inten, tsif_dev->base + INTEN);

	pmt_cfg_buff_ptr = (char *)virt_to_phys(kmalloc(4096, GFP_KERNEL));

	if (pmt_cfg_buff_ptr == NULL)
		return -1;

	davinci_writel((u32) pmt_cfg_buff_ptr, tsif_dev->base + PMT_STORE_ADD);
	davinci_writel((TSIF_PMT_SENSE_CFG_PMT_SENSE_EN | (u32) pmt_cfg.
			pmt_pid), tsif_dev->base + PMT_SENSE_CFG);
	/* Clear PMT_detect */
	davinci_writel(TSIF_INTEN_CLR_PMT_DETECT_INTEN_CLR,
		       tsif_dev->base + INT_STATUS_CLR);

	/* PMT_detect_inten<-1 */
	inten = davinci_readl(tsif_dev->base + INTEN);
	inten |= TSIF_INTEN_SET_PMT_DETECT_INTEN_SET;
	davinci_writel(inten, tsif_dev->base + INTEN);

	/* PAT_detect_inten_set<-1 */
	inten = davinci_readl(tsif_dev->base + INTEN_SET);
	inten |= TSIF_INTEN_SET_PMT_DETECT_INTEN_SET;
	davinci_writel(inten, tsif_dev->base + INTEN_SET);

	init_completion(&tsif_isr_data.pmt_complete);
	tsif_isr_data.pmt_complete.done = 0;

	return 0;
}

int tsif_set_pcr_config(struct inode *inode, unsigned long arg)
{
	struct tsif_pcr_config pcr_cfg;
	struct tsif_control_dev *tsif_dev;
	unsigned int index, minor = iminor(inode);

	index = minor / tsif_control_minor_count;
	tsif_dev = tsif_control_dev_get_by_major(index);

	/* copy the parameters to the configuration */
	if (copy_from_user(&pcr_cfg,
			   (struct tsif_pcr_config *)arg,
			   sizeof(struct tsif_pcr_config)))
		return -EFAULT;

	davinci_writel(TSIF_PCR_SENSE_CFG_RESETVAL,
		       tsif_dev->base + PCR_SENSE_CFG);

	davinci_writel((TSIF_PCR_SENSE_CFG_PCR_SENSE_EN | (u32) pcr_cfg.
			pcr_pid), tsif_dev->base + PCR_SENSE_CFG);

	return 0;
}

int tsif_set_spec_pkt_config(struct inode *inode, unsigned long arg)
{
	struct tsif_spcpkt_config spcpkt_cfg;
	char *spcpkt_cfg_buff_ptr = NULL;
	u32 inten, tsif_ctrl1;
	struct tsif_control_dev *tsif_dev;
	unsigned int index, minor = iminor(inode);

	index = minor / tsif_control_minor_count;
	tsif_dev = tsif_control_dev_get_by_major(index);

	/* copy the parameters to the configuration */
	if (copy_from_user(&spcpkt_cfg,
			   (struct tsif_spcpkt_config *)arg,
			   sizeof(struct tsif_spcpkt_config)))
		return -EFAULT;

	inten = davinci_readl(tsif_dev->base + INTEN);
	inten &= ~TSIF_INTEN_SET_BOUNDARY_SPECIFIC_INTEN_SET;
	davinci_writel(inten, tsif_dev->base + INTEN);

	spcpkt_cfg_buff_ptr = (char *)virt_to_phys(kmalloc(4096, GFP_KERNEL));

	if (spcpkt_cfg_buff_ptr == NULL)
		return -1;

	if (&spcpkt_cfg != NULL) {
		if (spcpkt_cfg.pid < 0x2000) {
			davinci_writel((u32) spcpkt_cfg.pid,
				       tsif_dev->base + BSP_PID);
			tsif_ctrl1 = davinci_readl(tsif_dev->base + CTRL1);
			tsif_ctrl1 |= TSIF_CTRL1_STREAM_BNDRY_CTL;
			davinci_writel(tsif_ctrl1, tsif_dev->base + CTRL1);
		} else {
			tsif_ctrl1 = davinci_readl(tsif_dev->base + CTRL1);
			tsif_ctrl1 &= ~TSIF_CTRL1_STREAM_BNDRY_CTL;
		}
		davinci_writel((u32) spcpkt_cfg_buff_ptr,
			       tsif_dev->base + BSP_STORE_ADD);
	} else {
		tsif_ctrl1 = davinci_readl(tsif_dev->base + CTRL1);
		tsif_ctrl1 &= ~TSIF_CTRL1_STREAM_BNDRY_CTL;
		davinci_writel(tsif_ctrl1, tsif_dev->base + CTRL1);
		davinci_writel(TSIF_BSP_STORE_ADDR_RESETVAL,
			       tsif_dev->base + BSP_STORE_ADD);
	}

	/* Clear boundary_specific */
	davinci_writel(TSIF_INTEN_CLR_BOUNDARY_SPECIFIC_INTEN_CLR,
		       tsif_dev->base + INT_STATUS_CLR);
	/* boundary_specific_inten<-1 */
	inten = davinci_readl(tsif_dev->base + INTEN);
	inten |= TSIF_INTEN_SET_BOUNDARY_SPECIFIC_INTEN_SET;
	davinci_writel(inten, tsif_dev->base + INTEN);

	/* boundary_specific_inten_set<-1 */
	inten = davinci_readl(tsif_dev->base + INTEN_SET);
	inten |= TSIF_INTEN_SET_BOUNDARY_SPECIFIC_INTEN_SET;
	davinci_writel(inten, tsif_dev->base + INTEN_SET);

	init_completion(&tsif_isr_data.spcpkt_complete);
	tsif_isr_data.spcpkt_complete.done = 0;

	return 0;
}

int tsif_control_open(struct inode *inode, struct file *file)
{
	unsigned int index, minor = iminor(inode);
	struct tsif_control_dev *tsif_dev;

	index = minor / tsif_control_minor_count;
	tsif_dev = tsif_control_dev_get_by_major(index);

	tsif_dev->tsif_clk = clk_get(NULL, *(tsif_clk_name + index));
	if (IS_ERR(tsif_dev->tsif_clk)) {
		printk(KERN_ERR "Cannot get TSIF clock\n");
		return -1;
	}
	clk_enable(tsif_dev->tsif_clk);

	tsif_int_status = 0;

	davinci_writel(0x00000001, tsif_dev->base + EMULATION_CTRL);

	/* Enable the interrupt to arm */
	davinci_writel((davinci_readl(tsif_dev->base + CTRL1) | 0x8000),
		       tsif_dev->base + CTRL1);

	return 0;
}

int tsif_control_release(struct inode *inode, struct file *file)
{
	unsigned int index;
	struct tsif_control_dev *tsif_dev;

	index = iminor(inode) / tsif_control_minor_count;
	tsif_dev = tsif_control_dev_get_by_major(index);

	davinci_writel(0x0, tsif_dev->base + CTRL0);
	davinci_writel(0x0, tsif_dev->base + CTRL1);
	davinci_writel(0x0, tsif_dev->base + INTEN);
	davinci_writel(0xFFFFFFFF, tsif_dev->base + INTEN_CLR);
	davinci_writel(0xFFFFFFFF, tsif_dev->base + INT_STATUS_CLR);
	davinci_writel(0x00000001, tsif_dev->base + EMULATION_CTRL);

	clk_disable(tsif_dev->tsif_clk);

	return 0;
}

int tsif_control_mmap(struct file *filp, struct vm_area_struct *vma)
{
	if (remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff,
			    vma->vm_end - vma->vm_start, vma->vm_page_prot))
		return -EAGAIN;

	return 0;
}

int tsif_set_tx_config(struct inode *inode, unsigned long arg)
{
	u8 new_if;
	struct tsif_control_dev *tsif_dev;
	unsigned int index, minor = iminor(inode);
	u32 tx_ctrl, tsif_ctrl0, inten;
	struct tsif_tx_config tx_cfg;
	int pkt_size = 0;

	index = minor / tsif_control_minor_count;
	tsif_dev = tsif_control_dev_get_by_major(index);

	tsif_ctrl0 = davinci_readl(tsif_dev->base + CTRL0);

	/* copy the parameters to the configuration */
	if (copy_from_user(&tx_cfg,
			   (struct tsif_tx_config *)arg,
			   sizeof(struct tsif_tx_config)))
		return -EFAULT;

	davinci_writel(TSIF_RING_BUF_RD_CH_CTL_RESETVAL,
		       tsif_dev->base + RRB_CH_CTRL);
	inten = davinci_readl(tsif_dev->base + INTEN);
	inten &= ~TSIF_INTEN_RBR0_FULL_INTEN;
	davinci_writel(inten, tsif_dev->base + INTEN);
	if (&tx_cfg != NULL) {
		tx_ctrl = 0x00000000;
		switch (tx_cfg.if_mode) {
		case TSIF_IF_SERIAL_SYNC:
			if (tx_cfg.clk_speed == TSIF_13_5_MHZ_SERIAL_PARALLEL)
				set_tsif_clk(TSIF_13_5_MHZ_SERIAL_PARALLEL);
			else if (tx_cfg.clk_speed ==
				 TSIF_16_875_MHZ_SERIAL_PARALLEL)
				set_tsif_clk(TSIF_16_875_MHZ_SERIAL_PARALLEL);
			else if (tx_cfg.clk_speed == TSIF_27_0_MHZ_SERIAL)
				set_tsif_clk(TSIF_27_0_MHZ_SERIAL);
			else if (tx_cfg.clk_speed == TSIF_54_0_MHZ_SERIAL)
				set_tsif_clk(TSIF_54_0_MHZ_SERIAL);
			else if (tx_cfg.clk_speed == TSIF_81_0_MHZ_SERIAL)
				set_tsif_clk(TSIF_81_0_MHZ_SERIAL);
			else
				return -EINVAL;

			new_if = 0;
			tx_ctrl |= TSIF_CTRL0_TX_IF_MODE_SER_SYN;
			break;
		case TSIF_IF_SERIAL_ASYNC:
			if (tx_cfg.clk_speed == TSIF_13_5_MHZ_SERIAL_PARALLEL)
				set_tsif_clk(TSIF_13_5_MHZ_SERIAL_PARALLEL);
			else if (tx_cfg.clk_speed ==
				 TSIF_16_875_MHZ_SERIAL_PARALLEL)
				set_tsif_clk(TSIF_16_875_MHZ_SERIAL_PARALLEL);
			else if (tx_cfg.clk_speed == TSIF_27_0_MHZ_SERIAL)
				set_tsif_clk(TSIF_27_0_MHZ_SERIAL);
			else if (tx_cfg.clk_speed == TSIF_54_0_MHZ_SERIAL)
				set_tsif_clk(TSIF_54_0_MHZ_SERIAL);
			else if (tx_cfg.clk_speed == TSIF_81_0_MHZ_SERIAL)
				set_tsif_clk(TSIF_81_0_MHZ_SERIAL);
			else
				return -EINVAL;

			new_if = 0;
			tx_ctrl |= TSIF_CTRL0_TX_IF_MODE_SER_ASYN;
			break;
		case TSIF_IF_PARALLEL_SYNC:
			if (tx_cfg.clk_speed == TSIF_13_5_MHZ_SERIAL_PARALLEL)
				set_tsif_clk(TSIF_13_5_MHZ_SERIAL_PARALLEL);
			else if (tx_cfg.clk_speed ==
				 TSIF_16_875_MHZ_SERIAL_PARALLEL)
				set_tsif_clk(TSIF_16_875_MHZ_SERIAL_PARALLEL);
			else
				return -EINVAL;

			new_if = 1;
			tx_ctrl |= TSIF_CTRL0_TX_IF_MODE_PAR_SYN;
			break;
		case TSIF_IF_PARALLEL_ASYNC:
			if (tx_cfg.clk_speed == TSIF_13_5_MHZ_SERIAL_PARALLEL)
				set_tsif_clk(TSIF_13_5_MHZ_SERIAL_PARALLEL);
			else if (tx_cfg.clk_speed ==
				 TSIF_16_875_MHZ_SERIAL_PARALLEL)
				set_tsif_clk(TSIF_16_875_MHZ_SERIAL_PARALLEL);
			else
				return -EINVAL;

			new_if = 1;
			tx_ctrl |= TSIF_CTRL0_TX_IF_MODE_PAR_ASYN;
			break;
		case TSIF_IF_DMA:
		default:
			return -EINVAL;
		}
		switch (tx_cfg.stream_mode) {
		case TSIF_STREAM_TS:
			tx_ctrl |= TSIF_CTRL0_TX_STREAM_MODE_TS_ACTIVE;
			pkt_size = 192;
			break;
		case TSIF_STREAM_NON_TS:
			tx_ctrl |= TSIF_CTRL0_TX_STREAM_MODE_TS_INACTIVE;
			switch (tx_cfg.pkt_size) {
			case TSIF_200_BYTES_PER_PKT:
				tx_ctrl |= TSIF_CTRL0_TX_PKT_SIZE_BYTE_200;
				pkt_size = 200;
				break;
			case TSIF_208_BYTES_PER_PKT:
				tx_ctrl |= TSIF_CTRL0_TX_PKT_SIZE_BYTE_208;
				pkt_size = 208;
				break;
			case TSIF_216_BYTES_PER_PKT:
				tx_ctrl |= TSIF_CTRL0_TX_PKT_SIZE_BYTE_216;
				pkt_size = 216;
				break;
			case TSIF_224_BYTES_PER_PKT:
				tx_ctrl |= TSIF_CTRL0_TX_PKT_SIZE_BYTE_224;
				pkt_size = 224;
				break;
			case TSIF_232_BYTES_PER_PKT:
				tx_ctrl |= TSIF_CTRL0_TX_PKT_SIZE_BYTE_232;
				pkt_size = 232;
				break;
			case TSIF_240_BYTES_PER_PKT:
				tx_ctrl |= TSIF_CTRL0_TX_PKT_SIZE_BYTE_240;
				pkt_size = 240;
				break;
			case TSIF_248_BYTES_PER_PKT:
				tx_ctrl |= TSIF_CTRL0_TX_PKT_SIZE_BYTE_248;
				pkt_size = 248;
				break;
			case TSIF_256_BYTES_PER_PKT:
				tx_ctrl |= TSIF_CTRL0_TX_PKT_SIZE_BYTE_256;
				pkt_size = 256;
				break;
			default:
				return -EINVAL;
			}
			break;
		default:
			return -EINVAL;
		}
		switch (tx_cfg.ats_mode) {
		case TSIF_TX_ATS_REMOVE:
			tx_ctrl |= TSIF_CTRL0_TX_ATS_MODE_OUT_188;
			break;
		case TSIF_TX_ATS_THROUGH:
			tx_ctrl |= TSIF_CTRL0_TX_ATS_MODE_OUT_192;
			break;
		default:
			return -EINVAL;
		}
		if (tsif_if_parallel != new_if) {
			if (tsif_if_parallel) {
				davinci_cfg_reg(DM646X_PTSOMUX_PARALLEL,
						PINMUX_FREE);
				davinci_cfg_reg(DM646X_PTSIMUX_PARALLEL,
						PINMUX_FREE);
				davinci_cfg_reg(DM646X_PTSOMUX_SERIAL,
						PINMUX_RESV);
				davinci_cfg_reg(DM646X_PTSIMUX_SERIAL,
						PINMUX_RESV);
			} else {
				davinci_cfg_reg(DM646X_PTSOMUX_SERIAL,
						PINMUX_FREE);
				davinci_cfg_reg(DM646X_PTSIMUX_SERIAL,
						PINMUX_FREE);
				davinci_cfg_reg(DM646X_PTSOMUX_PARALLEL,
						PINMUX_RESV);
				davinci_cfg_reg(DM646X_PTSIMUX_PARALLEL,
						PINMUX_RESV);
			}
			tsif_if_parallel = new_if;
		}
		davinci_writel((u32) tx_cfg.interval_wait,
			       tsif_dev->base + ASYNC_TX_WAIT);

		tsif_ctrl0 = davinci_readl(tsif_dev->base + CTRL0);
		tsif_ctrl0 &= ~(TX_CONFIG);
		tsif_ctrl0 |= tx_ctrl;
		tsif_ctrl0 |= TSIF_CTRL0_TX_CLK_INV_MASK;
		davinci_writel(tsif_ctrl0, tsif_dev->base + CTRL0);
	} else {
		davinci_writel(TSIF_ASYNC_TX_WAIT_RESETVAL,
			       tsif_dev->base + ASYNC_TX_WAIT);
		tsif_ctrl0 = davinci_readl(tsif_dev->base + CTRL0);
		tsif_ctrl0 &= ~(TX_CONFIG) | 0x00000000;
		davinci_writel(tsif_ctrl0, tsif_dev->base + CTRL0);
	}

	/* Clear RBR0_full */
	davinci_writel(TSIF_INTEN_CLR_RBR0_FULL_INTEN_CLR,
		       tsif_dev->base + INT_STATUS_CLR);

	/* RBR0_full_inten<-1 */
	inten = davinci_readl(tsif_dev->base + INTEN);
	inten |= TSIF_INTEN_RBR0_FULL_INTEN;
	davinci_writel(inten, tsif_dev->base + INTEN);

	/* RBR0_full_inten_set<-1 */
	inten = davinci_readl(tsif_dev->base + INTEN_SET);
	inten |= TSIF_INTEN_SET_RBR0_FULL_INTEN_SET;
	davinci_writel(inten, tsif_dev->base + INTEN_SET);

	return 0;
}
EXPORT_SYMBOL(tsif_set_tx_config);

int tsif_set_rx_config(struct inode *inode, unsigned long arg)
{
	struct tsif_control_dev *tsif_dev;
	unsigned int index, minor = iminor(inode);
	u32 tsif_ctrl0, tsif_ctrl1, inten;
	u32 rx_ctrl0, rx_ctrl1;
	struct tsif_rx_config rx_cfg;
	int pkt_size = 0;

	index = minor / tsif_control_minor_count;
	tsif_dev = tsif_control_dev_get_by_major(index);

	tsif_ctrl0 = davinci_readl(tsif_dev->base + CTRL0);
	/* copy the parameters to the configuration */
	if (copy_from_user(&rx_cfg,
			   (struct tsif_rx_config *)arg,
			   sizeof(struct tsif_rx_config)))
		return -EFAULT;

	if (tsif_stop_rx(inode) != 0)
		return -EPERM;

	davinci_writel(TSIF_RING_BUFFER_WRITE_CHANNEL_CONTROL_RESETVAL,
		       tsif_dev->base + WRB_CH_CTRL);

	/* Disbale all receiver relarted interrupts */
	inten = davinci_readl(tsif_dev->base + INTEN);
	inten &= ~(TSIF_INTEN_RBW7_FULL_INTEN |
		   TSIF_INTEN_RBW6_FULL_INTEN |
		   TSIF_INTEN_RBW5_FULL_INTEN |
		   TSIF_INTEN_RBW4_FULL_INTEN |
		   TSIF_INTEN_RBW3_FULL_INTEN |
		   TSIF_INTEN_RBW2_FULL_INTEN |
		   TSIF_INTEN_RBW1_FULL_INTEN |
		   TSIF_INTEN_RBW0_FULL_INTEN | TSIF_INTEN_GOP_START_INTEN);
	davinci_writel(inten, tsif_dev->base + INTEN);

	if (&rx_cfg != NULL) {
		rx_ctrl0 = rx_ctrl1 = 0x00000000;
		switch (rx_cfg.if_mode) {
		case TSIF_IF_SERIAL_SYNC:
			rx_ctrl0 |= TSIF_CTRL0_RCV_IF_MODE_SER_SYN;
			break;
		case TSIF_IF_SERIAL_ASYNC:
			rx_ctrl0 |= TSIF_CTRL0_RCV_IF_MODE_SER_ASYN;
			break;
		case TSIF_IF_PARALLEL_SYNC:
			rx_ctrl0 |= TSIF_CTRL0_RCV_IF_MODE_PAR_SYN;
			break;
		case TSIF_IF_PARALLEL_ASYNC:
			rx_ctrl0 |= TSIF_CTRL0_RCV_IF_MODE_PAR_ASYN;
			break;
		case TSIF_IF_DMA:
			rx_ctrl0 |= TSIF_CTRL0_RCV_IF_MODE_DMA;
			break;
		default:
			return -EINVAL;
		}
		switch (rx_cfg.filter_mode) {
		case TSIF_PID_FILTER_BYPASS:{
				switch (rx_cfg.stream_mode) {
				case TSIF_STREAM_TS:
					rx_ctrl0 |=
					   TSIF_CTRL0_RCV_STREAM_MODE_TS_ACTIVE;
					pkt_size = 192;
					break;
				case TSIF_STREAM_NON_TS:
					rx_ctrl0 |=
					 TSIF_CTRL0_RCV_STREAM_MODE_TS_INACTIVE;
					switch (rx_cfg.pkt_size) {
					case TSIF_200_BYTES_PER_PKT:
						rx_ctrl0 |=
					 TSIF_CTRL0_RCV_PKT_SIZE_BYTE_200;
						pkt_size = 200;
						break;
					case TSIF_208_BYTES_PER_PKT:
						rx_ctrl0 |=
					TSIF_CTRL0_RCV_PKT_SIZE_BYTE_208;
						pkt_size = 208;
						break;
					case TSIF_216_BYTES_PER_PKT:
						rx_ctrl0 |=
					    TSIF_CTRL0_RCV_PKT_SIZE_BYTE_216;
						pkt_size = 216;
						break;
					case TSIF_224_BYTES_PER_PKT:
						rx_ctrl0 |=
					    TSIF_CTRL0_RCV_PKT_SIZE_BYTE_224;
						pkt_size = 224;
						break;
					case TSIF_232_BYTES_PER_PKT:
						rx_ctrl0 |=
					    TSIF_CTRL0_RCV_PKT_SIZE_BYTE_232;
						pkt_size = 232;
						break;
					case TSIF_240_BYTES_PER_PKT:
						rx_ctrl0 |=
					    TSIF_CTRL0_RCV_PKT_SIZE_BYTE_240;
						pkt_size = 240;
						break;
					case TSIF_248_BYTES_PER_PKT:
						rx_ctrl0 |=
					    TSIF_CTRL0_RCV_PKT_SIZE_BYTE_248;
						pkt_size = 248;
						break;
					case TSIF_256_BYTES_PER_PKT:
						rx_ctrl0 |=
					    TSIF_CTRL0_RCV_PKT_SIZE_BYTE_256;
						pkt_size = 256;
						break;
					default:
						return -EINVAL;
					}
					break;
				default:
					return -EINVAL;
				}
				break;
			}
		case TSIF_PID_FILTER_FULL_MANUAL:
			rx_ctrl1 |=
			    TSIF_CTRL1_STREAM_BNDRY_CTL |
			    TSIF_CTRL1_PID_FILTER_EN_ACTIVATE |
			    TSIF_CTRL1_PID_FILTER_CTL_FULL_MAN;
			rx_ctrl0 |= TSIF_CTRL0_RCV_STREAM_MODE_TS_ACTIVE;
			pkt_size = 192;
			break;
		case TSIF_PID_FILTER_SEMI_AUTO_A:
			rx_ctrl1 |=
			    TSIF_CTRL1_STREAM_BNDRY_CTL |
			    TSIF_CTRL1_PID_FILTER_EN_ACTIVATE |
			    TSIF_CTRL1_PID_FILTER_CTL_SEMI_A;
			rx_ctrl0 |= TSIF_CTRL0_RCV_STREAM_MODE_TS_ACTIVE;
			pkt_size = 192;
			break;
		case TSIF_PID_FILTER_SEMI_AUTO_B:
			rx_ctrl1 |=
			    TSIF_CTRL1_STREAM_BNDRY_CTL |
			    TSIF_CTRL1_PID_FILTER_EN_ACTIVATE |
			    TSIF_CTRL1_PID_FILTER_CTL_SEMI_B;
			rx_ctrl0 |= TSIF_CTRL0_RCV_STREAM_MODE_TS_ACTIVE;
			pkt_size = 192;
			break;
		case TSIF_PID_FILTER_FULL_AUTO:
			rx_ctrl1 |=
			    TSIF_CTRL1_STREAM_BNDRY_CTL |
			    TSIF_CTRL1_PID_FILTER_EN_ACTIVATE |
			    TSIF_CTRL1_PID_FILTER_CTL_FULL_AUT;
			rx_ctrl0 |= TSIF_CTRL0_RCV_STREAM_MODE_TS_ACTIVE;
			pkt_size = 192;
			break;
		default:
			return -EINVAL;
		}
		switch (rx_cfg.ats_mode) {
		case TSIF_RX_ATS_THROUGH:
			rx_ctrl0 |= TSIF_CTRL0_RCV_ATS_MODE_IN_192;
			break;
		case TSIF_RX_ATS_NOADD:
			rx_ctrl0 |= TSIF_CTRL0_RCV_ATS_MODE_IN_188;
			break;
		case TSIF_RX_ATS_CHANGE:
			rx_ctrl0 |= TSIF_CTRL0_RCV_ATS_MODE_CHANGE_192;
			break;
		case TSIF_RX_ATS_ADD:
			rx_ctrl0 |= TSIF_CTRL0_RCV_ATS_MODE_ADD_188;
			break;
		default:
			return -EINVAL;
		}
		tsif_ctrl0 = davinci_readl(tsif_dev->base + CTRL0);
		tsif_ctrl0 &= ~(RX_CONFIG);
		tsif_ctrl0 |= rx_ctrl0;
		davinci_writel(tsif_ctrl0, tsif_dev->base + CTRL0);

		tsif_ctrl1 = davinci_readl(tsif_dev->base + CTRL1);
		tsif_ctrl1 |= rx_ctrl1;
		davinci_writel(tsif_ctrl1, tsif_dev->base + CTRL1);
	} else {

		tsif_ctrl0 = davinci_readl(tsif_dev->base + CTRL0);
		tsif_ctrl0 &= ~(RX_CONFIG) | 0x00000000;
		davinci_writel(tsif_ctrl0, tsif_dev->base + CTRL0);
		tsif_ctrl1 = davinci_readl(tsif_dev->base + CTRL1);
		tsif_ctrl1 &= ~(TSIF_CTRL1_PID_FILTER_CTL) | 0x00000000;
		davinci_writel(tsif_ctrl1, tsif_dev->base + CTRL1);
	}

	return 0;
}
EXPORT_SYMBOL(tsif_set_rx_config);

int tsif_control_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
		       unsigned long arg)
{
	int ret = 0;
	unsigned int index;
	struct tsif_control_dev *tsif_dev;

	index = iminor(inode) / tsif_control_minor_count;
	tsif_dev = tsif_control_dev_get_by_major(index);

	switch (cmd) {
	case TSIF_SET_RX_CONFIG:{
			tsif_set_rx_config(inode, arg);
			break;
		}

	case TSIF_GET_RX_CONFIG:{
			u32 tsif_ctrl0, tsif_ctrl1;
			struct tsif_rx_config rx_cfg;

			tsif_ctrl0 = davinci_readl(tsif_dev->base + CTRL0);
			tsif_ctrl1 = davinci_readl(tsif_dev->base + CTRL1);
			if ((tsif_ctrl0 & TSIF_CTRL0_RCV_IF_MODE_DMA) == 0) {
				switch (tsif_ctrl0 &
					TSIF_CTRL0_RCV_IF_MODE_MASK) {
				case TSIF_CTRL0_RCV_IF_MODE_SER_SYN:
					rx_cfg.if_mode = TSIF_IF_SERIAL_SYNC;
					break;
				case TSIF_CTRL0_RCV_IF_MODE_SER_ASYN:
					rx_cfg.if_mode = TSIF_IF_SERIAL_ASYNC;
					break;
				case TSIF_CTRL0_RCV_IF_MODE_PAR_SYN:
					rx_cfg.if_mode = TSIF_IF_PARALLEL_SYNC;
					break;
				case TSIF_CTRL0_RCV_IF_MODE_PAR_ASYN:
					rx_cfg.if_mode = TSIF_IF_PARALLEL_ASYNC;
					break;
				}
			} else
				rx_cfg.if_mode = TSIF_IF_DMA;

			if ((tsif_ctrl0 & TSIF_CTRL0_RCV_STREAM_MODE_TS_ACTIVE)
			    == 0)
				rx_cfg.stream_mode = TSIF_STREAM_NON_TS;
			else
				rx_cfg.stream_mode = TSIF_STREAM_TS;
			switch (tsif_ctrl0 & TSIF_CTRL0_RCV_ATS_MODE_MASK) {
			case TSIF_CTRL0_RCV_ATS_MODE_IN_192:
				rx_cfg.ats_mode = TSIF_RX_ATS_THROUGH;
				break;
			case TSIF_CTRL0_RCV_ATS_MODE_IN_188:
				rx_cfg.ats_mode = TSIF_RX_ATS_NOADD;
				break;
			case TSIF_CTRL0_RCV_ATS_MODE_CHANGE_192:
				rx_cfg.ats_mode = TSIF_RX_ATS_CHANGE;
				break;
			case TSIF_CTRL0_RCV_ATS_MODE_ADD_188:
				rx_cfg.ats_mode = TSIF_RX_ATS_ADD;
				break;
			}

			if ((tsif_ctrl1 & TSIF_CTRL1_PID_FILTER_EN_ACTIVATE) ==
			    0)
				rx_cfg.filter_mode = TSIF_PID_FILTER_BYPASS;
			else {
				switch (tsif_ctrl1 &
					TSIF_CTRL1_PID_FILTER_CTL_MASK) {
				case TSIF_CTRL1_PID_FILTER_CTL_FULL_MAN:
					rx_cfg.filter_mode =
					    TSIF_PID_FILTER_FULL_MANUAL;
					break;
				case TSIF_CTRL1_PID_FILTER_CTL_SEMI_A:
					rx_cfg.filter_mode =
					    TSIF_PID_FILTER_SEMI_AUTO_A;
					break;
				case TSIF_CTRL1_PID_FILTER_CTL_SEMI_B:
					rx_cfg.filter_mode =
					    TSIF_PID_FILTER_SEMI_AUTO_B;
					break;
				case TSIF_CTRL1_PID_FILTER_CTL_FULL_AUT:
					rx_cfg.filter_mode =
					    TSIF_PID_FILTER_FULL_AUTO;
					break;
				default:
					return -EINVAL;
				}
			}

			if ((tsif_ctrl0 & TSIF_CTRL0_RCV_STREAM_MODE_TS_ACTIVE)
			    == 0) {
				switch (tsif_ctrl0 &
					TSIF_CTRL0_RCV_PKT_SIZE_MASK) {
				case TSIF_CTRL0_RCV_PKT_SIZE_BYTE_200:
					rx_cfg.pkt_size =
					    TSIF_200_BYTES_PER_PKT;
					break;
				case TSIF_CTRL0_RCV_PKT_SIZE_BYTE_208:
					rx_cfg.pkt_size =
					    TSIF_208_BYTES_PER_PKT;
					break;
				case TSIF_CTRL0_RCV_PKT_SIZE_BYTE_216:
					rx_cfg.pkt_size =
					    TSIF_216_BYTES_PER_PKT;
					break;
				case TSIF_CTRL0_RCV_PKT_SIZE_BYTE_224:
					rx_cfg.pkt_size =
					    TSIF_224_BYTES_PER_PKT;
					break;
				case TSIF_CTRL0_RCV_PKT_SIZE_BYTE_232:
					rx_cfg.pkt_size =
					    TSIF_232_BYTES_PER_PKT;
					break;
				case TSIF_CTRL0_RCV_PKT_SIZE_BYTE_240:
					rx_cfg.pkt_size =
					    TSIF_240_BYTES_PER_PKT;
					break;
				case TSIF_CTRL0_RCV_PKT_SIZE_BYTE_248:
					rx_cfg.pkt_size =
					    TSIF_248_BYTES_PER_PKT;
					break;
				case TSIF_CTRL0_RCV_PKT_SIZE_BYTE_256:
					rx_cfg.pkt_size =
					    TSIF_256_BYTES_PER_PKT;
					break;
				default:
					return -EINVAL;
				}
			} else
				rx_cfg.pkt_size = 192;

			/* copy the configuration to the user space */
			if (copy_to_user((struct tsif_rx_config *)arg,
					 &rx_cfg,
					 sizeof(struct tsif_rx_config)))
				return -EFAULT;
			break;
		}

	case TSIF_SET_TX_CONFIG:{
			tsif_set_tx_config(inode, arg);
			break;
		}

	case TSIF_GET_TX_CONFIG:{
			u32 tsif_ctrl;
			struct tsif_tx_config tx_cfg;

			tsif_ctrl = davinci_readl(tsif_dev->base + CTRL0);
			switch (tsif_ctrl & TSIF_CTRL0_TX_IF_MODE) {
			case TSIF_CTRL0_TX_IF_MODE_SER_SYN:
				tx_cfg.if_mode = TSIF_IF_SERIAL_SYNC;
				break;
			case TSIF_CTRL0_TX_IF_MODE_SER_ASYN:
				tx_cfg.if_mode = TSIF_IF_SERIAL_ASYNC;
				break;
			case TSIF_CTRL0_TX_IF_MODE_PAR_SYN:
				tx_cfg.if_mode = TSIF_IF_PARALLEL_SYNC;
				break;
			case TSIF_CTRL0_TX_IF_MODE_PAR_ASYN:
				tx_cfg.if_mode = TSIF_IF_PARALLEL_ASYNC;
				break;
			}
			if ((tsif_ctrl & TSIF_CTRL0_TX_STREAM_MODE) == 0)
				tx_cfg.stream_mode = TSIF_STREAM_NON_TS;
			else
				tx_cfg.stream_mode = TSIF_STREAM_TS;
			switch (tsif_ctrl & TSIF_CTRL0_TX_ATS_MODE) {
			case TSIF_CTRL0_TX_ATS_MODE_OUT_188:
				tx_cfg.ats_mode = TSIF_TX_ATS_REMOVE;
				break;
			case TSIF_CTRL0_TX_ATS_MODE_OUT_192:
				tx_cfg.ats_mode = TSIF_TX_ATS_THROUGH;
				break;
			}
			tx_cfg.interval_wait =
			    davinci_readb(tsif_dev->base + ASYNC_TX_WAIT);
			if ((tsif_ctrl & TSIF_CTRL0_TX_STREAM_MODE) == 0) {
				switch (tsif_ctrl & TSIF_CTRL0_TX_PKT_SIZE) {
				case TSIF_CTRL0_TX_PKT_SIZE_BYTE_200:
					tx_cfg.pkt_size =
					    TSIF_200_BYTES_PER_PKT;
					break;
				case TSIF_CTRL0_TX_PKT_SIZE_BYTE_208:
					tx_cfg.pkt_size =
					    TSIF_208_BYTES_PER_PKT;
					break;
				case TSIF_CTRL0_TX_PKT_SIZE_BYTE_216:
					tx_cfg.pkt_size =
					    TSIF_216_BYTES_PER_PKT;
					break;
				case TSIF_CTRL0_TX_PKT_SIZE_BYTE_224:
					tx_cfg.pkt_size =
					    TSIF_224_BYTES_PER_PKT;
					break;
				case TSIF_CTRL0_TX_PKT_SIZE_BYTE_232:
					tx_cfg.pkt_size =
					    TSIF_232_BYTES_PER_PKT;
					break;
				case TSIF_CTRL0_TX_PKT_SIZE_BYTE_240:
					tx_cfg.pkt_size =
					    TSIF_240_BYTES_PER_PKT;
					break;
				case TSIF_CTRL0_TX_PKT_SIZE_BYTE_248:
					tx_cfg.pkt_size =
					    TSIF_248_BYTES_PER_PKT;
					break;
				case TSIF_CTRL0_TX_PKT_SIZE_BYTE_256:
					tx_cfg.pkt_size =
					    TSIF_256_BYTES_PER_PKT;
					break;
				default:
					return -EINVAL;
				}
			} else
				tx_cfg.pkt_size = 192;

			/* copy the configuration to the user space */
			if (copy_to_user((struct tsif_tx_config *)arg,
					 &tx_cfg,
					 sizeof(struct tsif_tx_config)))
				return -EFAULT;
			break;
		}

	case TSIF_START_RX:{
			u32 tsif_ctrl0;

			tsif_ctrl0 = davinci_readl(tsif_dev->base + CTRL0);
			tsif_ctrl0 |=
			    (TSIF_CTRL0_RCV_DMA_CTL | TSIF_CTRL0_RCV_CTL);
			davinci_writel(tsif_ctrl0, tsif_dev->base + CTRL0);
			tsif_rx_enable = 1;
			break;
		}

	case TSIF_STOP_RX:{
			u32 tsif_ctrl0;

			tsif_ctrl0 = davinci_readl(tsif_dev->base + CTRL0);
			tsif_ctrl0 &=
			    ~(TSIF_CTRL0_RCV_DMA_CTL | TSIF_CTRL0_RCV_CTL);
			davinci_writel(tsif_ctrl0, tsif_dev->base + CTRL0);
			tsif_rx_enable = 0;
			break;
		}

	case TSIF_START_TX:{
			u32 tsif_ctrl0;
			void *addr;

			addr =
			    (void *)davinci_readl(tsif_dev->base +
						  RRB0_STRT_ADD);

			if (davinci_readl(tsif_dev->base + RRB0_STRT_ADD) ==
			    0x00000000)
				return -EPERM;

			tsif_ctrl0 = davinci_readl(tsif_dev->base + CTRL0);
			tsif_ctrl0 |=
			    (TSIF_CTRL0_TX_DMA_CTL | TSIF_CTRL0_TX_CTL);
			davinci_writel(tsif_ctrl0, tsif_dev->base + CTRL0);
			tsif_tx_enable = 1;
			break;
		}

	case TSIF_STOP_TX:{
			u32 tsif_ctrl0;

			tsif_ctrl0 = davinci_readl(tsif_dev->base + CTRL0);
			tsif_ctrl0 &=
			    ~(TSIF_CTRL0_TX_DMA_CTL | TSIF_CTRL0_TX_CTL);
			davinci_writel(tsif_ctrl0, tsif_dev->base + CTRL0);
			tsif_tx_enable = 0;
			tsif_tx_ats_init = 0;
			break;
		}

	case TSIF_CONFIG_PAT:{
			unsigned char flag;
			unsigned int pat_store_add;

			get_user(flag, (unsigned char *)arg);

			if (flag)
				tsif_set_pat_config(inode, arg);
			else {
				davinci_writel(0x0, tsif_dev->base +
					       PAT_SENSE_CFG);

				pat_store_add = (unsigned int)
				    phys_to_virt(davinci_readl
						 (tsif_dev->base +
						  PAT_STORE_ADD));
				kfree((void *)pat_store_add);
			}
			break;
		}

	case TSIF_GET_PAT_PKT:{
			unsigned int pat_store_add;

			wait_for_completion_interruptible(&tsif_isr_data.
							  pat_complete);

			pat_store_add =
			    davinci_readl(tsif_dev->base + PAT_STORE_ADD);
			pat_store_add =
			    (unsigned int)phys_to_virt(pat_store_add);

			/* copy to user the PAT packet */
			if (copy_to_user
			    ((void *)arg, (void *)pat_store_add, 192))
				return -EFAULT;
			else
				return 0;
			break;
		}		/* case */

	case TSIF_GET_PAT_CONFIG:{
			struct tsif_pat_config pat_cfg;

			pat_cfg.flag = davinci_readl(tsif_dev->base +
						     PAT_SENSE_CFG) >> 16;

			/* copy the configuration to the user space */
			if (copy_to_user((struct tsif_pat_config *)arg,
					  &pat_cfg,
					 sizeof(struct tsif_pat_config)))
				return -EFAULT;
			break;
		}

	case TSIF_GET_PMT_CONFIG:{
			struct tsif_pmt_config pmt_cfg;

			pmt_cfg.pmt_pid = davinci_readl(tsif_dev->base +
							PMT_SENSE_CFG) & 0x1fff;
			pmt_cfg.flag = davinci_readl(tsif_dev->base +
						     PMT_SENSE_CFG) >> 16;

			/* copy the configuration to the user space */
			if (copy_to_user((struct tsif_pmt_config *)arg,
					 &pmt_cfg,
					 sizeof(struct tsif_pmt_config)))
				return -EFAULT;
			break;
		}

	case TSIF_CONFIG_PMT:{
			struct tsif_pmt_config pmt_cfg;
			unsigned int pmt_store_add;

			if (copy_from_user(&pmt_cfg,
					   (struct tsif_pmt_config *)arg,
					   sizeof(struct tsif_pmt_config)))
				return -EFAULT;

			if (pmt_cfg.flag)
				tsif_set_pmt_config(inode, arg);
			else {
				davinci_writel(0x0,
					       tsif_dev->base + PMT_SENSE_CFG);

				pmt_store_add = (unsigned int)
				    phys_to_virt(davinci_readl
						 (tsif_dev->base +
						  PMT_STORE_ADD));
				kfree((void *)pmt_store_add);
			}
			break;
		}

	case TSIF_GET_PMT_PKT:{
			unsigned int pmt_store_add;

			wait_for_completion_interruptible(&tsif_isr_data.
							  pmt_complete);

			pmt_store_add =
			    davinci_readl(tsif_dev->base + PMT_STORE_ADD);
			pmt_store_add =
			    (unsigned int)phys_to_virt(pmt_store_add);

			/* copy to user the PMT packet */
			if (copy_to_user
			    ((void *)arg, (void *)pmt_store_add, 4096))
				return -EFAULT;
			else
				return 0;
			break;
		}

	case TSIF_ENABLE_PCR:{
			tsif_set_pcr_config(inode, arg);
			break;
		}

	case TSIF_DISABLE_PCR:{
			davinci_writel(0x0, tsif_dev->base + PCR_SENSE_CFG);
			break;
		}

	case TSIF_SET_SPCPKT_CONFIG:{
			tsif_set_spec_pkt_config(inode, arg);
			break;
		}

	case TSIF_GET_SPCPKT_CONFIG:{
			struct tsif_spcpkt_config spcpkt_cfg;
			char *spcpkt_cfg_buff_ptr;

			if ((davinci_readl(tsif_dev->base + CTRL1) &
			     TSIF_CTRL1_STREAM_BNDRY_CTL) == 0)
				spcpkt_cfg.pid = 0x2000; /* set illegal PID */
			else
				spcpkt_cfg.pid =
				    (u16) (davinci_readl
					   (tsif_dev->base + BSP_PID) & 0x1fff);

			spcpkt_cfg_buff_ptr =
			    (u8 *) davinci_readl(tsif_dev->base +
						 BSP_STORE_ADD);
			/* copy the configuration to the user space */
			if (copy_to_user((struct tsif_spcpkt_config *)arg,
					 &spcpkt_cfg,
					 sizeof(struct tsif_spcpkt_config)))
				return -EFAULT;
			break;
		}

	case TSIF_SET_ATS:{
			unsigned long ats_val;

			get_user(ats_val, (int *)arg);
			ats_val |= (1 << 30);
			davinci_writel(ats_val, tsif_dev->base + TX_ATS_INIT);
			break;
		}

	case TSIF_GET_ATS:{
			unsigned int ats_val;

			ats_val = davinci_readl(tsif_dev->base +
						TX_ATS_MONITOR);
			put_user(ats_val, (int *)arg);
			break;
		}

	case TSIF_BYPASS_ENABLE:{
			davinci_writel(0x01000000,
				       tsif_dev->base + BP_MODE_CFG);
			break;
		}

	case TSIF_SET_CONSEQUENTIAL_MODE:{
			unsigned int ctrl0;

			ctrl0 = davinci_readl(tsif_dev->base + CTRL0);
			ctrl0 |= TSIF_CTRL0_RCV_IF_MODE_DMA;
			davinci_writel(ctrl0, tsif_dev->base + CTRL0);
			break;
		}

	case TSIF_ENABLE_GOP_DETECT:{
			unsigned char gop_flag;

			get_user(gop_flag, (int *)arg);
			if (gop_flag) {
				davinci_writel(
					       (davinci_readl(
						tsif_dev->base + INTEN) |
						TSIF_INTEN_GOP_START_INTEN),
						tsif_dev->base + INTEN);

				davinci_writel(
				       (davinci_readl(
					tsif_dev->base + INTEN_SET) |
					TSIF_INTEN_SET_GOP_START_INTEN_SET),
					tsif_dev->base + INTEN_SET);

				davinci_writel(
					       (davinci_readl(
						tsif_dev->base + CTRL1) | 0x20),
						tsif_dev->base + CTRL1);
			} else {
				davinci_writel(
				       (davinci_readl(
					tsif_dev->base + CTRL1) & 0xFFFFFFDF),
					tsif_dev->base + CTRL1);
			}
			break;
		}

	case TSIF_GET_SPCPKT:{
			unsigned int bsp_store_add;

			wait_for_completion_interruptible(&tsif_isr_data.
							  spcpkt_complete);

			bsp_store_add =
				davinci_readl(tsif_dev->base + BSP_STORE_ADD);
			bsp_store_add =
				(unsigned int)phys_to_virt(bsp_store_add);

			/* copy to user the specific packet */
			if (copy_to_user
			    ((void *)arg, (void *)bsp_store_add, 4096))
				return -EFAULT;
			else
				return 0;
			break;
		}
	case TSIF_SET_ENDIAN_CTL:{
			unsigned long endian_ctl;
			unsigned long cur_reg_val;

			get_user(endian_ctl, (int *)arg);
			cur_reg_val = davinci_readl(tsif_dev->base + CTRL1);

			if (endian_ctl)
				cur_reg_val |= 1 << 14;
			else
				cur_reg_val &= ~(1 << 14);

			davinci_writel(cur_reg_val, tsif_dev->base + CTRL1);
		break;
		}

	}

	return ret;
}

static struct file_operations tsif_control_fops = {
	.owner = THIS_MODULE,
	.open = tsif_control_open,
	.release = tsif_control_release,
	.ioctl = tsif_control_ioctl,
	.mmap = tsif_control_mmap,
};

void tsif_pat_complete_cmd(void)
{
	complete(&tsif_isr_data.pat_complete);
}
EXPORT_SYMBOL(tsif_pat_complete_cmd);

void tsif_pmt_complete_cmd(void)
{
	complete(&tsif_isr_data.pmt_complete);
}
EXPORT_SYMBOL(tsif_pmt_complete_cmd);

void tsif_spcpkt_complete_cmd(void)
{
	complete(&tsif_isr_data.spcpkt_complete);
}
EXPORT_SYMBOL(tsif_spcpkt_complete_cmd);

static int tsif_remove(struct device *device)
{
	return 0;
}

static void tsif_platform_release(struct device *device)
{
	/* This function does nothing */
}

static struct device_driver tsif_driver = {
	.name = "tsif_control",
	.bus = &platform_bus_type,
	.remove = tsif_remove,
};

static struct class *tsif_control_class;

#define CDCE949                 (0x6C)

int set_tsif_clk(enum tsif_clk_speed clk_speed)
{
	unsigned int value;
	unsigned int sys_vddpwdn = (unsigned int)IO_ADDRESS(0x01C40048);
	unsigned int sys_tsif_ctl = (unsigned int)IO_ADDRESS(0x01C40050);
	unsigned int sys_vsclkdis = (unsigned int)IO_ADDRESS(0x01C4006C);

	davinci_cfg_reg(DM646X_CRGMUX, PINMUX_RESV);
	davinci_cfg_reg(DM646X_STSOMUX, PINMUX_RESV);
	davinci_cfg_reg(DM646X_STSIMUX, PINMUX_RESV);

	{
		int err = 0;
		char val2[2];

		val2[0] = 0x27 | 0x80;
		val2[1] = tsif_output_clk_freq[clk_speed][0];
		err = davinci_i2c_write(2, val2, CDCE949);

		val2[0] = 0x28 | 0x80;
		val2[1] = tsif_output_clk_freq[clk_speed][1];
		err = davinci_i2c_write(2, val2, CDCE949);

		val2[0] = 0x29 | 0x80;
		val2[1] = tsif_output_clk_freq[clk_speed][2];
		err = davinci_i2c_write(2, val2, CDCE949);

		val2[0] = 0x2a | 0x80;
		val2[1] = tsif_output_clk_freq[clk_speed][3];
		err = davinci_i2c_write(2, val2, CDCE949);

		val2[0] = 0x2b | 0x80;
		val2[1] = tsif_output_clk_freq[clk_speed][4];
		err = davinci_i2c_write(2, val2, CDCE949);

		val2[0] = 0x24 | 0x80;
		val2[1] = 0x6f;
		err = davinci_i2c_write(2, val2, CDCE949);
	}

	mdelay(200);

	value = 2 << 12 |	/* Auxclk */
		8 << 8 |	/* CRG0_VCXI */
		2 << 4 |	/* Auxclk */
		0;		/* CRG0_VCXI */
	outl(value, sys_tsif_ctl);

	/* Enable all the TSIF related clocks */
	value = inl(sys_vsclkdis);
	value &= ~(unsigned int)(0x000000FC);
	outl(value, sys_vsclkdis);

	/* Powerup UART1 Flow control and Data */
	value = inl(sys_vddpwdn);
	value &= ~(unsigned int)(0x000000C0);
	outl(value, sys_vddpwdn);

	return 0;
}
EXPORT_SYMBOL(set_tsif_clk);

static int __init tsif_control_init(void)
{
	struct platform_device *pldev;
	int result, ret = 0;
	dev_t devno;
	unsigned int size = 0, mem_size, i, j;

	set_tsif_clk(TSIF_16_875_MHZ_SERIAL_PARALLEL);
	davinci_cfg_reg(DM646X_PTSOMUX_PARALLEL, PINMUX_RESV);
	davinci_cfg_reg(DM646X_PTSIMUX_PARALLEL, PINMUX_RESV);
	tsif_if_parallel = 1;

	size = tsif_device_count * tsif_control_minor_count;
	/* Register the driver in the kernel */
	result = alloc_chrdev_region(&devno, 0, size, DRIVER_NAME);
	if (result < 0) {
		printk(KERN_ERR "TSIF: Module intialization failed.\
			could not register character device\n");
		result = -ENODEV;
		goto init_fail;
	}
	tsif_control_major = MAJOR(devno);

	mem_size = sizeof(struct tsif_control_dev);

	for (i = 0; i < tsif_device_count; i++) {
		tsif_control_dev_array[i] = kmalloc(mem_size, GFP_KERNEL);
		/* Initialize of character device */
		cdev_init(&tsif_control_dev_array[i]->c_dev,
			  &tsif_control_fops);
		tsif_control_dev_array[i]->c_dev.owner = THIS_MODULE;
		tsif_control_dev_array[i]->c_dev.ops = &tsif_control_fops;

		devno =
		    MKDEV(tsif_control_major,
			  i * tsif_control_minor_count +
			  tsif_control_minor_start);

		/* addding character device */
		result =
		    cdev_add(&tsif_control_dev_array[i]->c_dev, devno,
			     tsif_control_minor_count);
		if (result) {
			printk("TSIF:Error adding TSIF\n");
			unregister_chrdev_region(devno, size);
			goto init_fail;
		}
		tsif_control_dev_array[i]->devno = i;
	}

	tsif_control_class = class_create(THIS_MODULE, "tsif_control");
	if (!tsif_control_class) {
		for (i = 0; i < tsif_device_count; i++)
			cdev_del(&tsif_control_dev_array[i]->c_dev);
		result = -EIO;
		goto init_fail;
	}

	/* register driver as a platform driver */
	if (driver_register(&tsif_driver) != 0) {
		for (i = 0; i < tsif_device_count; i++) {
			unregister_chrdev_region(devno, size);
			cdev_del(&tsif_control_dev_array[i]->c_dev);
			kfree(tsif_control_dev_array[i]);
		}
		result = -EINVAL;
		goto init_fail;
	}

	for (i = 0; i < tsif_device_count; i++) {
		for (j = 0; j < tsif_control_minor_count; j++) {
			char name[50];
			/* Register the drive as a platform device */
			tsif_control_device[i][j] = NULL;

			pldev = kmalloc(sizeof(*pldev), GFP_KERNEL);
			if (!pldev)
				continue;

			memset(pldev, 0, sizeof(*pldev));
			pldev->name = *(control_dev_name + j);
			pldev->id = i;
			pldev->dev.release = tsif_platform_release;
			tsif_control_device[i][j] = pldev;

			if (platform_device_register(pldev) != 0) {
				driver_unregister(&tsif_driver);
				unregister_chrdev_region(devno, size);
				cdev_del(&tsif_control_dev_array[i]->c_dev);
				kfree(pldev);
				kfree(tsif_control_dev_array[i]);
				result = -EINVAL;
				goto init_fail;
			}

			devno =
			    MKDEV(tsif_control_major,
				  tsif_control_minor_start +
				  i * tsif_control_minor_count + j);
			ret = sprintf(name, "hd_tsif%d_", i);
			sprintf(&name[ret], *(control_dev_name + j));
			class_device_create(tsif_control_class, NULL, devno,
						NULL, name);
		}
		tsif_control_dev_array[i]->base =
		    (unsigned int)(TSIF_BASE + i * 0x400);
	}

	return 0;

init_fail:
	davinci_cfg_reg(DM646X_CRGMUX, PINMUX_FREE);
	davinci_cfg_reg(DM646X_STSOMUX, PINMUX_FREE);
	davinci_cfg_reg(DM646X_STSIMUX, PINMUX_FREE);
	davinci_cfg_reg(DM646X_PTSOMUX_PARALLEL, PINMUX_FREE);
	davinci_cfg_reg(DM646X_PTSIMUX_PARALLEL, PINMUX_FREE);
	return result;

}

static void __exit tsif_control_exit(void)
{
	dev_t devno;
	unsigned int size, i,  j;

	/* Release major/minor numbers */
	if (tsif_control_major != 0) {
		devno = MKDEV(tsif_control_major, tsif_control_minor_start);
		size = tsif_device_count * tsif_control_minor_count;
		unregister_chrdev_region(devno, size);
	}

	for (i = 0; i < tsif_device_count; i++) {
		for (j = 0; j < tsif_control_minor_count; j++) {
			devno = MKDEV(tsif_control_major,
				      tsif_control_minor_start +
				      i * tsif_control_minor_count + j);
			class_device_destroy(tsif_control_class, devno);
			platform_device_unregister(tsif_control_device[i][j]);
			kfree(tsif_control_device[i][j]);
		}
	}

	if (tsif_control_class != NULL) {
		size = tsif_device_count * tsif_control_minor_count;
		for (i = 0; i < tsif_device_count; i++) {
			cdev_del(&tsif_control_dev_array[i]->c_dev);
			kfree(tsif_control_dev_array[i]);
		}
		class_destroy(tsif_control_class);
		driver_unregister(&tsif_driver);
	}
	davinci_cfg_reg(DM646X_CRGMUX, PINMUX_FREE);
	davinci_cfg_reg(DM646X_STSOMUX, PINMUX_FREE);
	davinci_cfg_reg(DM646X_STSIMUX, PINMUX_FREE);
	if(tsif_if_parallel) {
		davinci_cfg_reg(DM646X_PTSOMUX_PARALLEL, PINMUX_FREE);
		davinci_cfg_reg(DM646X_PTSIMUX_PARALLEL, PINMUX_FREE);
	} else {
		davinci_cfg_reg(DM646X_PTSOMUX_SERIAL, PINMUX_FREE);
		davinci_cfg_reg(DM646X_PTSIMUX_SERIAL, PINMUX_FREE);
	}
}

module_init(tsif_control_init);
module_exit(tsif_control_exit);

MODULE_DESCRIPTION("TSIF Driver");
MODULE_AUTHOR("Texas Instruments");
MODULE_LICENSE("GPL");
