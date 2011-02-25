/*
 * linux/drivers/mmc/davinci.c
 *
 * TI DaVinci MMC controller file
 *
 * Copyright (C) 2006 Texas Instruments.
 *
 * ----------------------------------------------------------------------------
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * ----------------------------------------------------------------------------
 * Modifications:
 * ver. 1.0: Oct 2005, Purushotam Kumar   Initial version
 * ver 1.1:  Nov  2005, Purushotam Kumar  Solved bugs
 * ver 1.2:  Jan  2006, Purushotam Kumar   Added card remove insert support
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/mmc/host.h>
#include <linux/mmc/protocol.h>
#include <linux/mmc/card.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/scatterlist.h>

#include <asm/arch/mmc.h>
#include <asm/arch/edma.h>

#define	DAVINCI_MMC_REG_CTL	0x00
#define	DAVINCI_MMC_REG_CLK	0x04
#define	DAVINCI_MMC_REG_ST0	0x08
#define	DAVINCI_MMC_REG_ST1	0x0c
#define	DAVINCI_MMC_REG_IM	0x10
#define	DAVINCI_MMC_REG_TOR	0x14
#define	DAVINCI_MMC_REG_TOD	0x18
#define	DAVINCI_MMC_REG_BLEN	0x1c
#define	DAVINCI_MMC_REG_NBLK	0x20
#define	DAVINCI_MMC_REG_NBLC	0x24
#define	DAVINCI_MMC_REG_DRR	0x28
#define	DAVINCI_MMC_REG_DXR	0x2c
#define	DAVINCI_MMC_REG_CMD	0x30
#define	DAVINCI_MMC_REG_ARGHL	0x34
#define	DAVINCI_MMC_REG_RSP01	0x38
#define	DAVINCI_MMC_REG_RSP23	0x3c
#define	DAVINCI_MMC_REG_RSP45	0x40
#define	DAVINCI_MMC_REG_RSP67	0x44
#define	DAVINCI_MMC_REG_DRSP	0x48
#define	DAVINCI_MMC_REG_ETOK	0x4c
#define	DAVINCI_MMC_REG_CIDX	0x50
#define	DAVINCI_MMC_REG_CKC	0x54
#define	DAVINCI_MMC_REG_TORC	0x58
#define	DAVINCI_MMC_REG_TODC	0x5c
#define	DAVINCI_MMC_REG_BLNC	0x60
#define	DAVINCI_SDIO_REG_CTL	0x64
#define	DAVINCI_SDIO_REG_ST0	0x68
#define	DAVINCI_SDIO_REG_EN	0x6c
#define	DAVINCI_SDIO_REG_ST	0x70
#define	DAVINCI_MMC_REG_FIFO_CTL	0x74

#define DAVINCI_MMC_READW(host, reg) \
	__raw_readw((host)->virt_base + DAVINCI_MMC_REG_##reg)
#define DAVINCI_MMC_WRITEW(host, reg, val) \
	__raw_writew((val), (host)->virt_base + DAVINCI_MMC_REG_##reg)
#define DAVINCI_MMC_READL(host, reg) \
	__raw_readl((host)->virt_base + DAVINCI_MMC_REG_##reg)
#define DAVINCI_MMC_WRITEL(host, reg, val) \
	__raw_writel((val), (host)->virt_base + DAVINCI_MMC_REG_##reg)
/*
 * Command types
 */
#define DAVINCI_MMC_CMDTYPE_BC		0
#define DAVINCI_MMC_CMDTYPE_BCR		1
#define DAVINCI_MMC_CMDTYPE_AC		2
#define DAVINCI_MMC_CMDTYPE_ADTC	3
#define EDMA_MAX_LOGICAL_CHA_ALLOWED	7

#define DAVINCI_MMC_EVENT_BLOCK_XFERRED		(1 <<  0)
#define DAVINCI_MMC_EVENT_CARD_EXITBUSY		(1 <<  1)
#define	DAVINCI_MMC_EVENT_EOFCMD		(1 <<  2)
#define DAVINCI_MMC_EVENT_ERROR_DATATIMEOUT	(1 <<  3)
#define DAVINCI_MMC_EVENT_ERROR_CMDTIMEOUT	(1 <<  4)
#define DAVINCI_MMC_EVENT_ERROR_DATACRC		((1 << 6) | (1 << 5))
#define DAVINCI_MMC_EVENT_ERROR_CMDCRC		(1 <<  7)
#define DAVINCI_MMC_EVENT_WRITE			(1 <<  9)
#define DAVINCI_MMC_EVENT_READ			(1 << 10)

#define DAVINCI_MMC_EVENT_TIMEOUT_ERROR \
 (DAVINCI_MMC_EVENT_ERROR_DATATIMEOUT | DAVINCI_MMC_EVENT_ERROR_CMDTIMEOUT)
#define DAVINCI_MMC_EVENT_CRC_ERROR \
 (DAVINCI_MMC_EVENT_ERROR_DATACRC | DAVINCI_MMC_EVENT_ERROR_CMDCRC)
#define DAVINCI_MMC_EVENT_ERROR \
 (DAVINCI_MMC_EVENT_TIMEOUT_ERROR | DAVINCI_MMC_EVENT_CRC_ERROR)
#define DRIVER_NAME "davinci-mmc"
#define RSP_TYPE(x)	((x) & ~(MMC_RSP_BUSY|MMC_RSP_OPCODE))

struct edma_ch_mmcsd {
	unsigned char cnt_chanel;
	unsigned int chanel_num[EDMA_MAX_LOGICAL_CHA_ALLOWED];
};

struct mmc_davinci_host {
	struct resource 	*reg_res;
	spinlock_t		mmc_lock;
	int			is_card_busy;
	int			is_card_detect_progress;
	int			is_card_initialized;
	int			is_card_removed;
	int			is_init_progress;
	int			is_req_queued_up;
	struct mmc_host		*que_mmc_host;
	struct mmc_request	*que_mmc_request;
	struct mmc_command	*cmd;
	struct mmc_data		*data;
	struct mmc_host		*mmc;
	struct device		*dev;
	struct clk		*clk;
	struct resource		*mem_res;
	void __iomem		*virt_base;
	unsigned int		phys_base;
	unsigned int		rw_threshold;
	unsigned int		option_read;
	unsigned int		option_write;
	int			flag_sd_mmc;
	int			irq;
	unsigned char		bus_mode;

#define DAVINCI_MMC_DATADIR_NONE	0
#define DAVINCI_MMC_DATADIR_READ	1
#define DAVINCI_MMC_DATADIR_WRITE	2
	unsigned char		data_dir;
	u32			*buffer;
	u32			bytes_left;

	int			use_dma;
	int			do_dma;
	unsigned int		dma_rx_event;
	unsigned int		dma_tx_event;
	enum dma_event_q	queue_no;

	struct timer_list	timer;
	unsigned int		is_core_command;
	unsigned int		cmd_code;
	unsigned int		old_card_state;
	unsigned int		new_card_state;

	unsigned char		sd_support;

	struct edma_ch_mmcsd	edma_ch_details;

	unsigned int		sg_len;
	int			sg_idx;
	unsigned int		buffer_bytes_left;
	unsigned char		pio_set_dmatrig;
	int 			(*get_ro) (int);
};


void davinci_clean_channel(int ch_no);

/* MMCSD Init clock in Hz in opendain mode */
#define DAVINCI_MMC_INIT_CLOCK 		200000

/* This macro could not be defined to 0 (ZERO) or -ve value.
 * This value is multiplied to "HZ"
 * while requesting for timer interrupt every time for probing card.
 */
#define MULTIPLIER_TO_HZ 1

#define MMCST1_BUSY	(1 << 0)

static inline void wait_on_data(struct mmc_davinci_host *host)
{
	int cnt = 900000;
	u16 reg = DAVINCI_MMC_READW(host, ST1);

	while ((reg & MMCST1_BUSY) && cnt) {
		cnt--;
		udelay(1);
		reg = DAVINCI_MMC_READW(host, ST1);
	}
	if (!cnt)
		dev_warn(host->mmc->dev, "ERROR: TOUT waiting for BUSY\n");
}

/* PIO only */
static void mmc_davinci_sg_to_buf(struct mmc_davinci_host *host)
{
	struct scatterlist *sg;

	sg = host->data->sg + host->sg_idx;
	host->buffer_bytes_left = sg->length;
	host->buffer = page_address(sg->page) + sg->offset;
	if (host->buffer_bytes_left > host->bytes_left)
		host->buffer_bytes_left = host->bytes_left;
}

#define	DAVINCI_MMC_NO_RESOURCE		((resource_size_t)-1)

resource_size_t mmc_get_dma_resource(struct platform_device *dev,
				     unsigned long flag)
{
	struct resource *r;
	int i;

	for (i = 0; i < 10; i++) {
		/* Non-resource type flags will be AND'd off */
		r = platform_get_resource(dev, IORESOURCE_DMA, i);
		if (r == NULL)
			break;
		if ((r->flags & flag) == flag)
			return r->start;
	}

	return DAVINCI_MMC_NO_RESOURCE;
}

static void davinci_fifo_data_trans(struct mmc_davinci_host *host)
{
	int n, i;

	if (host->buffer_bytes_left == 0) {
		host->sg_idx++;
		BUG_ON(host->sg_idx == host->sg_len);
		mmc_davinci_sg_to_buf(host);
	}

	n = host->rw_threshold;
	if (n > host->buffer_bytes_left)
		n = host->buffer_bytes_left;

	host->buffer_bytes_left -= n;
	host->bytes_left -= n;

	if (host->data_dir == DAVINCI_MMC_DATADIR_WRITE)
		for (i = 0; i < (n / 4); i++) {
			DAVINCI_MMC_WRITEL(host, DXR, *host->buffer);
			host->buffer++;
		}
	else
		for (i = 0; i < (n / 4); i++) {
			*host->buffer = DAVINCI_MMC_READL(host, DRR);
			host->buffer++;
		}
}



static void mmc_davinci_start_command(struct mmc_davinci_host *host,
				      struct mmc_command *cmd)
{
	u32 cmdreg = 0;
	u32 resptype = 0;
	u32 cmdtype = 0;
	unsigned long flags;

	host->cmd = cmd;

	resptype = 0;
	cmdtype = 0;

	switch (RSP_TYPE(mmc_resp_type(cmd))) {
	case RSP_TYPE(MMC_RSP_R1):
		/* resp 1, resp 1b */
		resptype = 1;
		break;
	case RSP_TYPE(MMC_RSP_R2):
		resptype = 2;
		break;
	case RSP_TYPE(MMC_RSP_R3):
		resptype = 3;
		break;
	default:
		break;
	}

	/* Protocol layer does not provide command type, but our hardware
	 * needs it!
	 * any data transfer means adtc type (but that information is not
	 * in command structure, so we flagged it into host struct.)
	 * However, telling bc, bcr and ac apart based on response is
	 * not foolproof:
	 * CMD0  = bc  = resp0  CMD15 = ac  = resp0
	 * CMD2  = bcr = resp2  CMD10 = ac  = resp2
	 *
	 * Resolve to best guess with some exception testing:
	 * resp0 -> bc, except CMD15 = ac
	 * rest are ac, except if opendrain
	 */

	if (host->data_dir)
		cmdtype = DAVINCI_MMC_CMDTYPE_ADTC;
	else if (resptype == 0 && cmd->opcode != 15)
		cmdtype = DAVINCI_MMC_CMDTYPE_BC;
	else if (host->bus_mode == MMC_BUSMODE_OPENDRAIN)
		cmdtype = DAVINCI_MMC_CMDTYPE_BCR;
	else
		cmdtype = DAVINCI_MMC_CMDTYPE_AC;

	/*
	 * Set command Busy or not
	 * Linux core sending BUSY which is not defined for cmd 24
	 * as per mmc standard
	 */
	if (cmd->flags & MMC_RSP_BUSY)
		if (cmd->opcode != 24)
			cmdreg = cmdreg | (1 << 8);

	/* Set command index */
	cmdreg |= cmd->opcode;

	/* Setting initialize clock */
	if (cmd->opcode == 0)
		cmdreg = cmdreg | (1 << 14);

	/* Set for generating DMA Xfer event */
	if ((host->do_dma == 1) && (host->data != NULL)
	    && ((cmd->opcode == 18) || (cmd->opcode == 25)
		|| (cmd->opcode == 24) || (cmd->opcode == 17)))
		cmdreg = cmdreg | (1 << 16);

	if ((host->data != NULL) && (host->pio_set_dmatrig)
			&& (host->data_dir == DAVINCI_MMC_DATADIR_READ))
		cmdreg = cmdreg | (1 << 16);

	/* Setting whether command involves data transfer or not */
	if (cmdtype == DAVINCI_MMC_CMDTYPE_ADTC)
		cmdreg = cmdreg | (1 << 13);

	/* Setting whether stream or block transfer */
	if (cmd->flags & MMC_DATA_STREAM)
		cmdreg = cmdreg | (1 << 12);

	/* Setting whether data read or write */
	if (host->data_dir == DAVINCI_MMC_DATADIR_WRITE)
		cmdreg = cmdreg | (1 << 11);

	/* Setting response type */
	cmdreg = cmdreg | (resptype << 9);

	if (host->bus_mode == MMC_BUSMODE_PUSHPULL)
		cmdreg = cmdreg | (1 << 7);

	/* set Command timeout */
	DAVINCI_MMC_WRITEW(host, TOR, 0xFFFF);

	/* Enable interrupt */
	if (host->data_dir == DAVINCI_MMC_DATADIR_WRITE) {
		if (host->do_dma != 1)
			DAVINCI_MMC_WRITEW(host, IM, (DAVINCI_MMC_EVENT_EOFCMD |
				      DAVINCI_MMC_EVENT_WRITE |
				      DAVINCI_MMC_EVENT_ERROR_CMDCRC |
				      DAVINCI_MMC_EVENT_ERROR_DATACRC |
				      DAVINCI_MMC_EVENT_ERROR_CMDTIMEOUT |
				      DAVINCI_MMC_EVENT_ERROR_DATATIMEOUT |
				      DAVINCI_MMC_EVENT_BLOCK_XFERRED));
		else
			DAVINCI_MMC_WRITEW(host, IM, (DAVINCI_MMC_EVENT_EOFCMD |
				      DAVINCI_MMC_EVENT_ERROR_CMDCRC |
				      DAVINCI_MMC_EVENT_ERROR_DATACRC |
				      DAVINCI_MMC_EVENT_ERROR_CMDTIMEOUT |
				      DAVINCI_MMC_EVENT_ERROR_DATATIMEOUT |
				      DAVINCI_MMC_EVENT_BLOCK_XFERRED));
	} else if (host->data_dir == DAVINCI_MMC_DATADIR_READ) {
		if (host->do_dma != 1)
			DAVINCI_MMC_WRITEW(host, IM, (DAVINCI_MMC_EVENT_EOFCMD |
				      DAVINCI_MMC_EVENT_READ |
				      DAVINCI_MMC_EVENT_ERROR_CMDCRC |
				      DAVINCI_MMC_EVENT_ERROR_DATACRC |
				      DAVINCI_MMC_EVENT_ERROR_CMDTIMEOUT |
				      DAVINCI_MMC_EVENT_ERROR_DATATIMEOUT |
				      DAVINCI_MMC_EVENT_BLOCK_XFERRED));
		else
			DAVINCI_MMC_WRITEW(host, IM, (DAVINCI_MMC_EVENT_EOFCMD |
				      DAVINCI_MMC_EVENT_ERROR_CMDCRC |
				      DAVINCI_MMC_EVENT_ERROR_DATACRC |
				      DAVINCI_MMC_EVENT_ERROR_CMDTIMEOUT |
				      DAVINCI_MMC_EVENT_ERROR_DATATIMEOUT |
				      DAVINCI_MMC_EVENT_BLOCK_XFERRED));
	} else
		DAVINCI_MMC_WRITEW(host, IM, (DAVINCI_MMC_EVENT_EOFCMD |
				      DAVINCI_MMC_EVENT_ERROR_CMDCRC |
				      DAVINCI_MMC_EVENT_ERROR_DATACRC |
				      DAVINCI_MMC_EVENT_ERROR_CMDTIMEOUT |
				      DAVINCI_MMC_EVENT_ERROR_DATATIMEOUT));

	/*
	 * It is required by controoler b4 WRITE command that
	 * FIFO should be populated with 32 bytes
	 */
	if ((host->data_dir == DAVINCI_MMC_DATADIR_WRITE) &&
	    (cmdtype == DAVINCI_MMC_CMDTYPE_ADTC) && (host->do_dma != 1))
		davinci_fifo_data_trans(host);

	if (cmd->opcode == 7) {
		spin_lock_irqsave(&host->mmc_lock, flags);
		host->is_card_removed = 0;
		host->new_card_state = 1;
		host->is_card_initialized = 1;
		host->old_card_state = host->new_card_state;
		host->is_init_progress = 0;
		spin_unlock_irqrestore(&host->mmc_lock, flags);
	}
	if (cmd->opcode == 1 || cmd->opcode == 41) {
		spin_lock_irqsave(&host->mmc_lock, flags);
		host->is_card_initialized = 0;
		host->is_init_progress = 1;
		spin_unlock_irqrestore(&host->mmc_lock, flags);
	}

	host->is_core_command = 1;
	DAVINCI_MMC_WRITEL(host, ARGHL, cmd->arg);
	DAVINCI_MMC_WRITEL(host, CMD, cmdreg);
}

static void davinci_abort_dma(struct mmc_davinci_host *host)
{
	int sync_dev = 0;

	if (host->data_dir == DAVINCI_MMC_DATADIR_READ)
		sync_dev = host->dma_tx_event;
	else
		sync_dev = host->dma_rx_event;

	davinci_stop_dma(sync_dev);
	davinci_clean_channel(sync_dev);

}


static void mmc_davinci_dma_cb(int lch, u16 ch_status, void *data)
{
	if (DMA_COMPLETE != ch_status) {
		struct mmc_davinci_host *host = (struct mmc_davinci_host *)data;
		dev_warn(host->mmc->dev, "[DMA FAILED]");
		davinci_abort_dma(host);
	}
}

static void davinci_reinit_chan(struct mmc_davinci_host *host)
{
	davinci_stop_dma(host->dma_tx_event);
	davinci_clean_channel(host->dma_tx_event);

	davinci_stop_dma(host->dma_rx_event);
	davinci_clean_channel(host->dma_rx_event);
}

static int mmc_davinci_send_dma_request(struct mmc_davinci_host *host,
					struct mmc_request *req)
{
	int sync_dev;
	unsigned char i, j;
	unsigned short acnt, bcnt, ccnt;
	unsigned int src_port, dst_port, temp_ccnt;
	enum address_mode mode_src, mode_dst;
	enum fifo_width fifo_width_src, fifo_width_dst;
	unsigned short src_bidx, dst_bidx;
	unsigned short src_cidx, dst_cidx;
	unsigned short bcntrld;
	enum sync_dimension sync_mode;
	struct paramentry_descriptor temp;
	int edma_chan_num;
	struct mmc_data *data = host->data;
	struct scatterlist *sg = &data->sg[0];
	unsigned int count;
	int num_frames, frame;

#define MAX_C_CNT		64000

	frame = data->blksz;
	count = sg_dma_len(sg);

	if ((data->blocks == 1) && (count > data->blksz))
		count = frame;

	if (count % host->rw_threshold == 0) {
		acnt = 4;
		bcnt = host->rw_threshold / 4;
		num_frames = count / host->rw_threshold;
	} else {
		acnt = count;
		bcnt = 1;
		num_frames = 1;
	}

	if (num_frames > MAX_C_CNT) {
		temp_ccnt = MAX_C_CNT;
		ccnt = temp_ccnt;
	} else {
		ccnt = num_frames;
		temp_ccnt = ccnt;
	}

	if (host->data_dir == DAVINCI_MMC_DATADIR_WRITE) {
		/*AB Sync Transfer */
		sync_dev = host->dma_tx_event;

		src_port = (unsigned int)sg_dma_address(sg);
		mode_src = INCR;
		fifo_width_src = W8BIT;	/* It's not cared as modeDsr is INCR */
		src_bidx = acnt;
		src_cidx = acnt * bcnt;
		dst_port = host->phys_base + DAVINCI_MMC_REG_DXR;
		mode_dst = INCR;
		fifo_width_dst = W8BIT;	/* It's not cared as modeDsr is INCR */
		dst_bidx = 0;
		dst_cidx = 0;
		bcntrld = 8;
		sync_mode = ABSYNC;
	} else {
		sync_dev = host->dma_rx_event;

		src_port = host->phys_base + DAVINCI_MMC_REG_DRR;
		mode_src = INCR;
		fifo_width_src = W8BIT;
		src_bidx = 0;
		src_cidx = 0;
		dst_port = (unsigned int)sg_dma_address(sg);
		mode_dst = INCR;
		fifo_width_dst = W8BIT;	/* It's not cared as modeDsr is INCR */
		dst_bidx = acnt;
		dst_cidx = acnt * bcnt;
		bcntrld = 8;
		sync_mode = ABSYNC;
	}

	if (host->pio_set_dmatrig)
		davinci_dma_clear_event(sync_dev);
	davinci_set_dma_src_params(sync_dev, src_port, mode_src,
				   fifo_width_src);
	davinci_set_dma_dest_params(sync_dev, dst_port, mode_dst,
				    fifo_width_dst);
	davinci_set_dma_src_index(sync_dev, src_bidx, src_cidx);
	davinci_set_dma_dest_index(sync_dev, dst_bidx, dst_cidx);
	davinci_set_dma_transfer_params(sync_dev, acnt, bcnt, ccnt, bcntrld,
					sync_mode);

	davinci_get_dma_params(sync_dev, &temp);
	if (sync_dev == host->dma_tx_event) {
		if (host->option_write == 0) {
			host->option_write = temp.opt;
		} else {
			temp.opt = host->option_write;
			davinci_set_dma_params(sync_dev, &temp);
		}
	}
	if (sync_dev == host->dma_rx_event) {
		if (host->option_read == 0) {
			host->option_read = temp.opt;
		} else {
			temp.opt = host->option_read;
			davinci_set_dma_params(sync_dev, &temp);
		}
	}

	if (host->sg_len > 1) {
		davinci_get_dma_params(sync_dev, &temp);
		temp.opt &= ~TCINTEN;
		davinci_set_dma_params(sync_dev, &temp);

		for (i = 0; i < host->sg_len - 1; i++) {

			sg = &data->sg[i + 1];

			if (i != 0) {
				j = i - 1;
				davinci_get_dma_params(host->edma_ch_details.
						       chanel_num[j], &temp);
				temp.opt &= ~TCINTEN;
				davinci_set_dma_params(host->edma_ch_details.
						       chanel_num[j], &temp);
			}

			edma_chan_num = host->edma_ch_details.chanel_num[i];

			frame = data->blksz;
			count = sg_dma_len(sg);

			if ((data->blocks == 1) && (count > data->blksz))
				count = frame;

			ccnt = count / host->rw_threshold;

			if (sync_dev == host->dma_tx_event)
				temp.src = (unsigned int)sg_dma_address(sg);
			else
				temp.dst = (unsigned int)sg_dma_address(sg);

			temp.opt |= TCINTEN;

			temp.ccnt = (temp.ccnt & 0xFFFF0000) | (ccnt);

			davinci_set_dma_params(edma_chan_num, &temp);
			if (i != 0) {
				j = i - 1;
				davinci_dma_link_lch(host->edma_ch_details.
						     chanel_num[j],
						     edma_chan_num);
			}
		}
		davinci_dma_link_lch(sync_dev,
				     host->edma_ch_details.chanel_num[0]);
	}

	davinci_start_dma(sync_dev);
	return 0;
}

static int mmc_davinci_start_dma_transfer(struct mmc_davinci_host *host,
					  struct mmc_request *req)
{
	int use_dma = 1, i;
	struct mmc_data *data = host->data;
	int block_size = (1 << data->blksz_bits);

	host->sg_len = dma_map_sg(host->mmc->dev, data->sg, host->sg_len,
				  ((data->
				    flags & MMC_DATA_WRITE) ? DMA_TO_DEVICE :
				   DMA_FROM_DEVICE));

	/* Decide if we can use DMA */
	for (i = 0; i < host->sg_len; i++) {
		if ((data->sg[i].length % block_size) != 0) {
			use_dma = 0;
			break;
		}
	}

	if (!use_dma) {
		dma_unmap_sg(host->mmc->dev, data->sg, host->sg_len,
			     (data->
			      flags & MMC_DATA_WRITE) ? DMA_TO_DEVICE :
			     DMA_FROM_DEVICE);
		return -1;
	}

	host->do_dma = 1;

	mmc_davinci_send_dma_request(host, req);

	return 0;

}

static int davinci_release_dma_channels(struct mmc_davinci_host *host)
{
	int i;
	davinci_free_dma(host->dma_tx_event);
	davinci_free_dma(host->dma_rx_event);

	if (host->edma_ch_details.cnt_chanel) {
		for(i = 0;i < host->edma_ch_details.cnt_chanel;i++)
			davinci_free_dma(host->edma_ch_details.chanel_num[i]);
		host->edma_ch_details.cnt_chanel = 0;
	}

	return 0;
}

static int davinci_acquire_dma_channels(struct mmc_davinci_host *host)
{
	int edma_chan_num, tcc = 0, r, sync_dev, i;

	/* Acquire master DMA write channel */
	r = davinci_request_dma(host->dma_tx_event, "MMC_WRITE",
				     mmc_davinci_dma_cb, host,
				     &edma_chan_num, &tcc, host->queue_no);
	if (r) {
		dev_warn(host->mmc->dev,
			 "MMC: davinci_request_dma() failed with %d\n", r);
		return r;
	}

	/* Acquire master DMA read channel */
	r = davinci_request_dma(host->dma_rx_event, "MMC_READ",
				     mmc_davinci_dma_cb, host,
				     &edma_chan_num, &tcc, host->queue_no);
	if (r) {
		dev_warn(host->mmc->dev,
			 "MMC: davinci_request_dma() failed with %d\n", r);
		goto free_master_write;
	}

	host->edma_ch_details.cnt_chanel = 0;

	/* currently data Writes are done using single block mode,
	 * so no DMA slave write channel is required for now */

	/* Create a DMA slave read channel */
	sync_dev = host->dma_rx_event;
	for(i = 0;i < EDMA_MAX_LOGICAL_CHA_ALLOWED;i++){
		r = davinci_request_dma(DAVINCI_EDMA_PARAM_ANY, "LINK",
						 NULL, NULL, &edma_chan_num,
						 &sync_dev, host->queue_no);
		if (r) {
			dev_warn(host->mmc->dev,
				 "MMC: davinci_request_dma() failed with %d\n", r);
			for(i = 0;i < host->edma_ch_details.cnt_chanel;i++)
				davinci_free_dma(host->edma_ch_details.chanel_num[i]);
			goto free_master_read;
		}

		host->edma_ch_details.cnt_chanel++;
		host->edma_ch_details.chanel_num[i] = edma_chan_num;
	}

	return 0;

free_master_read:
	davinci_free_dma(host->dma_rx_event);
free_master_write:
	davinci_free_dma(host->dma_tx_event);

	return r;
}

static void mmc_davinci_prepare_data(struct mmc_davinci_host *host,
				     struct mmc_request *req)
{
	int timeout, sg_len;
	u16 reg;
	host->data = req->data;
	if (req->data == NULL) {
		host->data_dir = DAVINCI_MMC_DATADIR_NONE;
		DAVINCI_MMC_WRITEW(host, BLEN, 0);
		DAVINCI_MMC_WRITEW(host, NBLK, 0);
		return;
	}
	/* Init idx */
	host->sg_idx = 0;

	dev_dbg(host->mmc->dev,
		"MMCSD : Data xfer (%s %s), "
		"DTO %d cycles + %d ns, %d blocks of %d bytes\n",
		(req->data->flags & MMC_DATA_STREAM) ? "stream" : "block",
		(req->data->flags & MMC_DATA_WRITE) ? "write" : "read",
		req->data->timeout_clks, req->data->timeout_ns,
		req->data->blocks, 1 << req->data->blksz_bits);

	/* Convert ns to clock cycles by assuming 20MHz frequency
	 * 1 cycle at 20MHz = 500 ns
	 */
	timeout = req->data->timeout_clks + req->data->timeout_ns / 500;
	if (timeout > 0xffff)
		timeout = 0xffff;

	DAVINCI_MMC_WRITEW(host, TOD, timeout);
	DAVINCI_MMC_WRITEW(host, NBLK, req->data->blocks);
	DAVINCI_MMC_WRITEW(host, BLEN, (1 << req->data->blksz_bits));
	host->data_dir = (req->data->flags & MMC_DATA_WRITE) ?
	    DAVINCI_MMC_DATADIR_WRITE : DAVINCI_MMC_DATADIR_READ;

	/* Configure the FIFO */
	switch (host->data_dir) {
	case DAVINCI_MMC_DATADIR_WRITE:
		reg = DAVINCI_MMC_READW(host, FIFO_CTL);
		reg |= 0x1;
		DAVINCI_MMC_WRITEW(host, FIFO_CTL, reg);
		DAVINCI_MMC_WRITEW(host, FIFO_CTL, 0x0);
		reg = DAVINCI_MMC_READW(host, FIFO_CTL);
		reg |= (1 << 1);
		DAVINCI_MMC_WRITEW(host, FIFO_CTL, reg);
		reg = DAVINCI_MMC_READW(host, FIFO_CTL);
		reg |= (1 << 2);
		DAVINCI_MMC_WRITEW(host, FIFO_CTL, reg);
		break;
	case DAVINCI_MMC_DATADIR_READ:
		reg = DAVINCI_MMC_READW(host, FIFO_CTL);
		reg |= 0x1;
		DAVINCI_MMC_WRITEW(host, FIFO_CTL, reg);
		DAVINCI_MMC_WRITEW(host, FIFO_CTL, 0x0);
		reg = DAVINCI_MMC_READW(host, FIFO_CTL);
		reg |= (1 << 2);
		DAVINCI_MMC_WRITEW(host, FIFO_CTL, reg);
		break;
	default:
		break;
	}

	sg_len = (req->data->blocks == 1) ? 1 : req->data->sg_len;
	host->sg_len = sg_len;

	host->bytes_left = req->data->blocks * (1 << req->data->blksz_bits);

	if ((host->use_dma == 1) && (host->bytes_left % host->rw_threshold == 0)
	    && (mmc_davinci_start_dma_transfer(host, req) == 0)) {
		host->buffer = NULL;
		host->bytes_left = 0;
	} else {
		/* Revert to CPU Copy */

		host->do_dma = 0;
		mmc_davinci_sg_to_buf(host);
	}
}

static void mmc_davinci_request(struct mmc_host *mmc, struct mmc_request *req)
{
	struct mmc_davinci_host *host = mmc_priv(mmc);
	unsigned long flags;

	if (host->is_card_removed) {
		if (req->cmd) {
			req->cmd->error |= MMC_ERR_TIMEOUT;
			mmc_request_done(mmc, req);
		}
		dev_dbg(host->mmc->dev,
			"From code segment excuted when card removed\n");
		return;
	}

	wait_on_data(host);

	if (!host->is_card_detect_progress) {
		spin_lock_irqsave(&host->mmc_lock, flags);
		host->is_card_busy = 1;
		spin_unlock_irqrestore(&host->mmc_lock, flags);
		host->do_dma = 0;
		mmc_davinci_prepare_data(host, req);
		mmc_davinci_start_command(host, req->cmd);
	} else {
		/* Queue up the request as card dectection is being excuted */
		host->que_mmc_host = mmc;
		host->que_mmc_request = req;
		spin_lock_irqsave(&host->mmc_lock, flags);
		host->is_req_queued_up = 1;
		spin_unlock_irqrestore(&host->mmc_lock, flags);
	}
}

static unsigned int calculate_freq_for_card(struct mmc_davinci_host *host,
			unsigned int mmc_req_freq)
{
	unsigned int mmc_freq, cpu_arm_clk, mmc_push_pull;

	cpu_arm_clk = clk_get_rate(host->clk);

	if (cpu_arm_clk > (2 * mmc_req_freq)) {
		mmc_push_pull =
		    ((unsigned int)cpu_arm_clk / (2 * mmc_req_freq)) - 1;
	} else
		mmc_push_pull = 0;

	mmc_freq = (unsigned int)cpu_arm_clk / (2 * (mmc_push_pull + 1));

	if (mmc_freq > mmc_req_freq)
		mmc_push_pull = mmc_push_pull + 1;

	return mmc_push_pull;
}

static void mmc_davinci_set_ios(struct mmc_host *mmc, struct mmc_ios *ios)
{
	unsigned short status;
	unsigned int open_drain_freq, cpu_arm_clk;
	unsigned int mmc_push_pull_freq;
	u16 reg;
	struct mmc_davinci_host *host = mmc_priv(mmc);

	cpu_arm_clk = clk_get_rate(host->clk);
	dev_dbg(host->mmc->dev, "clock %dHz busmode %d powermode %d \
			Vdd %d.%02d\n",
		ios->clock, ios->bus_mode, ios->power_mode,
		ios->vdd / 100, ios->vdd % 100);

	reg = DAVINCI_MMC_READW(host, CTL);
	switch (ios->bus_width) {
	case MMC_BUS_WIDTH_8:
		dev_dbg(host->mmc->dev, "\nEnabling 8 bit mode\n");
		reg |=	(1 << 8);
		reg &= ~(1 << 2);
		break;
	case MMC_BUS_WIDTH_4:
		dev_dbg(host->mmc->dev, "\nEnabling 4 bit mode\n");
		reg &= ~(1 << 8);
		reg |=	(1 << 2);
		break;
	default:
		dev_dbg(host->mmc->dev, "\nEnabling 1 bit mode\n");
		reg &= ~((1 << 8) | (1 << 2));
	}
	DAVINCI_MMC_WRITEW(host, CTL, reg);

	if (ios->bus_mode == MMC_BUSMODE_OPENDRAIN) {
		open_drain_freq =
		    ((unsigned int)(cpu_arm_clk + 2*DAVINCI_MMC_INIT_CLOCK - 1)
			/ (2 * DAVINCI_MMC_INIT_CLOCK)) - 1;
		if (open_drain_freq > 0xff)
			open_drain_freq = 0xff;
		DAVINCI_MMC_WRITEW(host, CLK, open_drain_freq | 0x100);

	} else {
		mmc_push_pull_freq = calculate_freq_for_card(host, ios->clock);
		reg = DAVINCI_MMC_READW(host, CLK);
		reg &= ~(0x100);
		DAVINCI_MMC_WRITEW(host, CLK, reg);
		udelay(10);
		DAVINCI_MMC_WRITEW(host, CLK, mmc_push_pull_freq | 0x100);
		udelay(10);
	}
	host->bus_mode = ios->bus_mode;
	if (ios->power_mode == MMC_POWER_UP) {
		/* Send clock cycles, poll completion */
		reg = DAVINCI_MMC_READW(host, IM);
		DAVINCI_MMC_WRITEW(host, IM, 0);
		DAVINCI_MMC_WRITEL(host, ARGHL, 0x0);
		DAVINCI_MMC_WRITEL(host, CMD, 0x4000);
		status = 0;
		while (!(status & (DAVINCI_MMC_EVENT_EOFCMD)))
			status = DAVINCI_MMC_READW(host, ST0);

		DAVINCI_MMC_WRITEW(host, IM, reg);
	}
}

static void mmc_davinci_xfer_done(struct mmc_davinci_host *host,
				  struct mmc_data *data)
{
	unsigned long flags;
	host->data = NULL;
	host->data_dir = DAVINCI_MMC_DATADIR_NONE;
	if (data->error == MMC_ERR_NONE)
		data->bytes_xfered += data->blocks * (1 << data->blksz_bits);

	if (host->do_dma) {
		davinci_abort_dma(host);

		dma_unmap_sg(host->mmc->dev, data->sg, host->sg_len,
			     (data->
			      flags & MMC_DATA_WRITE) ? DMA_TO_DEVICE :
			     DMA_FROM_DEVICE);
	}

	if (data->error == MMC_ERR_TIMEOUT) {
		spin_lock_irqsave(&host->mmc_lock, flags);
		host->is_card_busy = 0;
		spin_unlock_irqrestore(&host->mmc_lock, flags);
		mmc_request_done(host->mmc, data->mrq);
		return;
	}

	if (!data->stop) {
		spin_lock_irqsave(&host->mmc_lock, flags);
		host->is_card_busy = 0;
		spin_unlock_irqrestore(&host->mmc_lock, flags);
		mmc_request_done(host->mmc, data->mrq);
		return;
	}
	mmc_davinci_start_command(host, data->stop);
}

static void mmc_davinci_cmd_done(struct mmc_davinci_host *host,
				 struct mmc_command *cmd)
{
	unsigned long flags;
	host->cmd = NULL;

	if (!cmd) {
		dev_warn(host->mmc->dev, "%s(): No cmd ptr\n", __func__);
		return;
	}

	if (cmd->flags & MMC_RSP_PRESENT) {
		if (cmd->flags & MMC_RSP_136) {
			/* response type 2 */
			cmd->resp[3] = DAVINCI_MMC_READL(host, RSP01);
			cmd->resp[2] = DAVINCI_MMC_READL(host, RSP23);
			cmd->resp[1] = DAVINCI_MMC_READL(host, RSP45);
			cmd->resp[0] = DAVINCI_MMC_READL(host, RSP67);
		} else {
		/* response types 1, 1b, 3, 4, 5, 6 */
		cmd->resp[0] = DAVINCI_MMC_READL(host, RSP67);
		}
	}

	if (host->data == NULL || cmd->error != MMC_ERR_NONE) {
		if (cmd->error == MMC_ERR_TIMEOUT)
			cmd->mrq->cmd->retries = 0;
		spin_lock_irqsave(&host->mmc_lock, flags);
		host->is_card_busy = 0;
		spin_unlock_irqrestore(&host->mmc_lock, flags);
		mmc_request_done(host->mmc, cmd->mrq);
	}

}

static irqreturn_t mmc_davinci_irq(int irq, void *dev_id, struct pt_regs *regs)
{
	struct mmc_davinci_host *host = (struct mmc_davinci_host *)dev_id;
	u16 status;
	int end_command;
	int end_transfer;
	unsigned long flags;
	u16 reg;

	if (host->is_core_command) {
		if (host->cmd == NULL && host->data == NULL) {
			status = DAVINCI_MMC_READW(host, ST0);
			dev_dbg(host->mmc->dev, "Spurious interrupt 0x%04x\n",
				status);
			/* Disable the interrupt from mmcsd */
			DAVINCI_MMC_WRITEW(host, IM, 0);
			return IRQ_HANDLED;
		}
	}
	end_command = 0;
	end_transfer = 0;

	status = DAVINCI_MMC_READW(host, ST0);
	if (status == 0)
		return IRQ_HANDLED;

	if (host->is_core_command) {
		if (host->is_card_initialized) {
			if (host->new_card_state == 0) {
				if (host->cmd) {
					host->cmd->error |= MMC_ERR_TIMEOUT;
					mmc_davinci_cmd_done(host, host->cmd);
				}
				dev_dbg(host->mmc->dev,
					"From code segment excuted when card \
					removed\n");
				return IRQ_HANDLED;
			}
		}

		while (status != 0) {
			if (host->data_dir == DAVINCI_MMC_DATADIR_WRITE) {
				if (status & DAVINCI_MMC_EVENT_WRITE) {
					/* Buffer almost empty */
					if (host->bytes_left > 0)
						davinci_fifo_data_trans(host);
				}
			}

			if (host->data_dir == DAVINCI_MMC_DATADIR_READ)
				if (status & DAVINCI_MMC_EVENT_READ)
					/* Buffer almost empty */
					if (host->bytes_left > 0)
						davinci_fifo_data_trans(host);

			if (status & DAVINCI_MMC_EVENT_BLOCK_XFERRED) {
				/* Block sent/received */
				if (host->data != NULL) {
					if (host->do_dma == 1)
						end_transfer = 1;
					else {
						/* if datasize <
						 * host_rw_threshold no RX ints
						 * are generated */
						if (host->bytes_left > 0)
							davinci_fifo_data_trans
							    (host);
						end_transfer = 1;
					}
				} else
					dev_warn(host->mmc->dev,
						 "TC:host->data is NULL\n");
			}

			if (status & DAVINCI_MMC_EVENT_ERROR_DATATIMEOUT) {
				/* Data timeout */
				if ((host->data) &&
						(host->new_card_state != 0)) {
					host->data->error |= MMC_ERR_TIMEOUT;
					spin_lock_irqsave(&host->mmc_lock,
							flags);
					host->is_card_removed = 1;
					host->new_card_state = 0;
					host->is_card_initialized = 0;
					spin_unlock_irqrestore(&host->mmc_lock,
							       flags);
					dev_dbg(host->mmc->dev,
						"MMCSD: Data timeout, CMD%d \
						and status is %x\n",
						host->cmd->opcode, status);

					if (host->cmd)
						host->cmd->error |=
						    MMC_ERR_TIMEOUT;
					end_transfer = 1;
				}
			}

			if (status & DAVINCI_MMC_EVENT_ERROR_DATACRC) {
				/* DAT line portion is diabled and in reset
				 * state */
				reg = DAVINCI_MMC_READW(host, CTL);
				reg |= (1 << 1);
				DAVINCI_MMC_WRITEW(host, CTL, reg);
				udelay(10);
				reg = DAVINCI_MMC_READW(host, CTL);
				reg &= ~(1 << 1);
				DAVINCI_MMC_WRITEW(host, CTL, reg);

				/* Data CRC error */
				if (host->data) {
					host->data->error |= MMC_ERR_BADCRC;
					dev_dbg(host->mmc->dev,
						"MMCSD: Data CRC error, bytes \
						left %d\n",
						host->bytes_left);
					end_transfer = 1;
				} else
					dev_dbg(host->mmc->dev,
						"MMCSD: Data CRC error\n");
			}

			if (status & DAVINCI_MMC_EVENT_ERROR_CMDTIMEOUT) {
				if (host->do_dma)
					davinci_abort_dma(host);

				/* Command timeout */
				if (host->cmd) {
					/* Timeouts are normal in case of
					 * MMC_SEND_STATUS
					 */
					if (host->cmd->opcode !=
					    MMC_ALL_SEND_CID) {
						dev_dbg(host->mmc->dev,
							"MMCSD: Command \
							timeout, CMD%d and \
							status is %x\n",
							host->cmd->opcode,
							status);
						spin_lock_irqsave(
								&host->mmc_lock,
								  flags);
						host->new_card_state = 0;
						host->is_card_initialized = 0;
						spin_unlock_irqrestore
						    (&host->mmc_lock, flags);
					}
					host->cmd->error |= MMC_ERR_TIMEOUT;
					end_command = 1;

				}
			}

			if (status & DAVINCI_MMC_EVENT_ERROR_CMDCRC) {
				/* Command CRC error */
				dev_dbg(host->mmc->dev, "Command CRC error\n");
				if (host->cmd) {
					/* Ignore CMD CRC errors during high
					 * speed operation */
					if (host->mmc->ios.clock <= 25000000) {
						host->cmd->error |=
						    MMC_ERR_BADCRC;
					}
					end_command = 1;
				}
			}

			if (status & DAVINCI_MMC_EVENT_EOFCMD)
				end_command = 1;

			if (host->data == NULL) {
				status = DAVINCI_MMC_READW(host, ST0);
				if (status != 0) {
					dev_dbg(host->mmc->dev,
						"Status is %x at end of ISR \
						when host->data is NULL",
						status);
					status = 0;

				}
			} else
				status = DAVINCI_MMC_READW(host, ST0);
		}

		if (end_command)
			mmc_davinci_cmd_done(host, host->cmd);

		if (end_transfer)
			mmc_davinci_xfer_done(host, host->data);
	} else {
		if (host->cmd_code == 13) {
			if (status & DAVINCI_MMC_EVENT_EOFCMD) {
				spin_lock_irqsave(&host->mmc_lock, flags);
				host->new_card_state = 1;
				spin_unlock_irqrestore(&host->mmc_lock, flags);

			} else {
				spin_lock_irqsave(&host->mmc_lock, flags);
				host->is_card_removed = 1;
				host->new_card_state = 0;
				host->is_card_initialized = 0;
				spin_unlock_irqrestore(&host->mmc_lock, flags);
			}

			spin_lock_irqsave(&host->mmc_lock, flags);
			host->is_card_detect_progress = 0;
			spin_unlock_irqrestore(&host->mmc_lock, flags);

			if (host->is_req_queued_up) {
				mmc_davinci_request(host->que_mmc_host,
						    host->que_mmc_request);
				spin_lock_irqsave(&host->mmc_lock, flags);
				host->is_req_queued_up = 0;
				spin_unlock_irqrestore(&host->mmc_lock, flags);
			}

		}

		if (host->cmd_code == 1 || host->cmd_code == 55) {
			if (status & DAVINCI_MMC_EVENT_EOFCMD) {
				spin_lock_irqsave(&host->mmc_lock, flags);
				host->is_card_removed = 0;
				host->new_card_state = 1;
				host->is_card_initialized = 0;
				spin_unlock_irqrestore(&host->mmc_lock, flags);
			} else {

				spin_lock_irqsave(&host->mmc_lock, flags);
				host->is_card_removed = 1;
				host->new_card_state = 0;
				host->is_card_initialized = 0;
				spin_unlock_irqrestore(&host->mmc_lock, flags);
			}

			spin_lock_irqsave(&host->mmc_lock, flags);
			host->is_card_detect_progress = 0;
			spin_unlock_irqrestore(&host->mmc_lock, flags);

			if (host->is_req_queued_up) {
				mmc_davinci_request(host->que_mmc_host,
						    host->que_mmc_request);
				spin_lock_irqsave(&host->mmc_lock, flags);
				host->is_req_queued_up = 0;
				spin_unlock_irqrestore(&host->mmc_lock, flags);
			}
		}

		if (host->cmd_code == 0) {
			if (status & DAVINCI_MMC_EVENT_EOFCMD) {
				host->is_core_command = 0;

				if (host->flag_sd_mmc) {
					host->flag_sd_mmc = 0;
					host->cmd_code = 1;
					/* Issue cmd1 */
					DAVINCI_MMC_WRITEL(host, ARGHL,
							0x80300000);
					DAVINCI_MMC_WRITEL(host, CMD,
							0x00000601);
				} else {
					host->flag_sd_mmc = 1;
					host->cmd_code = 55;
					/* Issue cmd55 */
					DAVINCI_MMC_WRITEL(host, ARGHL, 0x0);
					DAVINCI_MMC_WRITEL(host, CMD,
					    ((0x0 | (1 << 9) | 55)));
				}

				dev_dbg(host->mmc->dev,
					"MMC-Probing mmc with cmd%d\n",
					host->cmd_code);
			} else {
				spin_lock_irqsave(&host->mmc_lock, flags);
				host->new_card_state = 0;
				host->is_card_initialized = 0;
				host->is_card_detect_progress = 0;
				spin_unlock_irqrestore(&host->mmc_lock, flags);
			}
		}

	}
	return IRQ_HANDLED;
}

static int mmc_davinci_get_ro(struct mmc_host *mmc)
{
	struct mmc_davinci_host *host = mmc_priv(mmc);

	return host->get_ro ? host->get_ro(mmc->index) : 0;
}

static struct mmc_host_ops mmc_davinci_ops = {
	.request = mmc_davinci_request,
	.set_ios = mmc_davinci_set_ios,
	.get_ro = mmc_davinci_get_ro
};

void mmc_check_card(unsigned long data)
{
	struct mmc_davinci_host *host = (struct mmc_davinci_host *)data;
	unsigned long flags;
	struct mmc_card *card = NULL;

	if (host->mmc && host->mmc->card_selected)
		card = host->mmc->card_selected;

	if ((!host->is_card_detect_progress) || (!host->is_init_progress)) {
		if (host->is_card_initialized) {
			host->is_core_command = 0;
			host->cmd_code = 13;
			spin_lock_irqsave(&host->mmc_lock, flags);
			host->is_card_detect_progress = 1;
			spin_unlock_irqrestore(&host->mmc_lock, flags);
			/* Issue cmd13 */
			DAVINCI_MMC_WRITEL(host, ARGHL, (card && (card->state
							 & MMC_STATE_SDCARD))
					? (card->rca << 16) : 0x10000);
			DAVINCI_MMC_WRITEL(host, CMD, 0x0000028D);
		} else {
			host->is_core_command = 0;
			host->cmd_code = 0;
			spin_lock_irqsave(&host->mmc_lock, flags);
			host->is_card_detect_progress = 1;
			spin_unlock_irqrestore(&host->mmc_lock, flags);
			/* Issue cmd0 */
			DAVINCI_MMC_WRITEL(host, ARGHL, 0);
			DAVINCI_MMC_WRITEL(host, CMD, 0x4000);
		}
		DAVINCI_MMC_WRITEW(host, IM, (DAVINCI_MMC_EVENT_EOFCMD |
				      DAVINCI_MMC_EVENT_ERROR_CMDCRC |
				      DAVINCI_MMC_EVENT_ERROR_DATACRC |
				      DAVINCI_MMC_EVENT_ERROR_CMDTIMEOUT |
				      DAVINCI_MMC_EVENT_ERROR_DATATIMEOUT));

	}
}

static void init_mmcsd_host(struct mmc_davinci_host *host)
{
	u16 reg;
	/* CMD line portion is disabled and in reset state */
	reg = DAVINCI_MMC_READW(host, CTL);
	reg |= 0x1;
	DAVINCI_MMC_WRITEW(host, CTL, reg);
	/* DAT line portion is disabled and in reset state */
	reg = DAVINCI_MMC_READW(host, CTL);
	reg |= (1 << 1);
	DAVINCI_MMC_WRITEW(host, CTL, reg);
	udelay(10);

	DAVINCI_MMC_WRITEW(host, CLK, 0x0);
	reg = DAVINCI_MMC_READW(host, CLK);
	reg |= (1 << 8);
	DAVINCI_MMC_WRITEW(host, CLK, reg);

	DAVINCI_MMC_WRITEW(host, TOR, 0xFFFF);
	DAVINCI_MMC_WRITEW(host, TOD, 0xFFFF);

	reg = DAVINCI_MMC_READW(host, CTL);
	reg &= ~(0x1);
	DAVINCI_MMC_WRITEW(host, CTL, reg);
	reg = DAVINCI_MMC_READW(host, CTL);
	reg &= ~(1 << 1);
	DAVINCI_MMC_WRITEW(host, CTL, reg);
	udelay(10);
}

static void davinci_mmc_check_status(unsigned long data)
{
	unsigned long flags;
	struct mmc_davinci_host *host = (struct mmc_davinci_host *)data;
	if (!host->is_card_busy) {
		if (host->old_card_state ^ host->new_card_state) {
			davinci_reinit_chan(host);
			init_mmcsd_host(host);
			mmc_detect_change(host->mmc, 0);
			spin_lock_irqsave(&host->mmc_lock, flags);
			host->old_card_state = host->new_card_state;
			spin_unlock_irqrestore(&host->mmc_lock, flags);
		} else {
			mmc_check_card(data);
		}

	}
	mod_timer(&host->timer, jiffies + MULTIPLIER_TO_HZ * HZ);
}

static int davinci_mmc_probe(struct platform_device *pdev)
{
	struct davinci_mmc_platform_data *minfo = pdev->dev.platform_data;
	struct mmc_host *mmc;
	struct mmc_davinci_host *host = NULL;
	struct resource *res;
	resource_size_t dma_rx_chan, dma_tx_chan, dma_eventq;
	int ret = 0;
	int irq;

	if (minfo == NULL) {
		dev_err(&pdev->dev, "platform data missing\n");
		return -ENODEV;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	irq = platform_get_irq(pdev, 0);
	if (res == NULL || irq < 0)
		return -ENODEV;

	res = request_mem_region(res->start, res->end - res->start + 1,
				 pdev->name);
	if (res == NULL)
		return -EBUSY;

	mmc = mmc_alloc_host(sizeof(struct mmc_davinci_host), &pdev->dev);
	if (mmc == NULL) {
		ret = -ENOMEM;
		goto err_free_mem_region;
	}

	host = mmc_priv(mmc);
	host->mmc = mmc;

	spin_lock_init(&host->mmc_lock);

	host->mem_res = res;
	host->irq = irq;

	host->phys_base = host->mem_res->start;
	host->virt_base = (void __iomem *) IO_ADDRESS(host->phys_base);

	host->use_dma = 0;

#ifdef CONFIG_MMC_DAVINCI_DMA
	dma_rx_chan = mmc_get_dma_resource(pdev, IORESOURCE_DMA_RX_CHAN);
	if (dma_rx_chan >= 0) {
		host->dma_rx_event = dma_rx_chan;
		dma_tx_chan = mmc_get_dma_resource(pdev,
						   IORESOURCE_DMA_TX_CHAN);
		if (dma_tx_chan >= 0) {
			host->dma_tx_event = dma_tx_chan;
			dma_eventq = mmc_get_dma_resource(pdev,
						IORESOURCE_DMA_EVENT_Q);
			if (dma_eventq >= 0) {
				host->queue_no = dma_eventq;
				host->use_dma = 1;
			} else {
				host->dma_tx_event = 0;
				host->dma_rx_event = 0;
			}
		} else
			host->dma_rx_event = 0;
	}
#endif

	host->clk = clk_get(&pdev->dev, minfo->mmc_clk);
	if (IS_ERR(host->clk)) {
		ret = -ENODEV;
		goto err_free_mmc_host;
	}

	ret = clk_enable(host->clk);
	if (ret)
		goto err_put_clk;

	init_mmcsd_host(host);

	if (minfo->use_8bit_mode) {
		dev_info(mmc->dev, "Supporting 8-bit mode\n");
		mmc->caps |= MMC_CAP_8_BIT_DATA;
	}

	if (minfo->use_4bit_mode) {
		dev_info(mmc->dev, "Supporting 4-bit mode\n");
		mmc->caps |= MMC_CAP_4_BIT_DATA;
	}

	if (!minfo->use_8bit_mode && !minfo->use_4bit_mode)
		dev_info(mmc->dev, "Supporting 1-bit mode\n");

	host->get_ro = minfo->get_ro;

	host->pio_set_dmatrig = minfo->pio_set_dmatrig;

	host->rw_threshold = minfo->rw_threshold;

	mmc->ops = &mmc_davinci_ops;
	mmc->f_min = 312500;
	if (minfo->max_frq)
		mmc->f_max = minfo->max_frq;
	else
		mmc->f_max = 25000000;
	mmc->ocr_avail = MMC_VDD_32_33;

	mmc->max_phys_segs = EDMA_MAX_LOGICAL_CHA_ALLOWED + 1;
	mmc->max_hw_segs = EDMA_MAX_LOGICAL_CHA_ALLOWED + 1;
	mmc->max_sectors = 256;

	/* Restrict the max size of seg we can handle */
	mmc->max_seg_size = mmc->max_sectors * 512;

	dev_dbg(mmc->dev, "max_phys_segs=%d\n", mmc->max_phys_segs);
	dev_dbg(mmc->dev, "max_hw_segs=%d\n", mmc->max_hw_segs);
	dev_dbg(mmc->dev, "max_sect=%d\n", mmc->max_sectors);
	dev_dbg(mmc->dev, "max_seg_size=%d\n", mmc->max_seg_size);

	if (host->use_dma) {
		dev_info(mmc->dev, "Using DMA mode\n");
		ret = davinci_acquire_dma_channels(host);
		if (ret){
			printk("Acquire dma channels fail!!!!!!!!!!\n");
			goto err_release_clk;
		}
	} else {
		dev_info(mmc->dev, "Not Using DMA mode\n");
	}

	host->sd_support = 1;
	ret = request_irq(host->irq, mmc_davinci_irq, 0, DRIVER_NAME, host);
	if (ret){
		printk("IRQ register fail!!!!!!!!!!\n");
		goto err_release_dma;
	}	

	host->dev = &pdev->dev;
	platform_set_drvdata(pdev, host);
	mmc_add_host(mmc);

	init_timer(&host->timer);
	host->timer.data = (unsigned long)host;
	host->timer.function = davinci_mmc_check_status;
	host->timer.expires = jiffies + MULTIPLIER_TO_HZ * HZ;
	add_timer(&host->timer);

	return 0;

err_release_dma:
	davinci_release_dma_channels(host);
err_release_clk:
	clk_disable(host->clk);
err_put_clk:
	clk_put(host->clk);
err_free_mmc_host:
	mmc_free_host(mmc);
err_free_mem_region:
	release_mem_region(res->start, res->end - res->start + 1);

	return ret;
}

static int davinci_mmcsd_remove(struct platform_device *dev)
{
	struct mmc_davinci_host *host = platform_get_drvdata(dev);
	unsigned long flags;

	if (host) {
		spin_lock_irqsave(&host->mmc_lock, flags);
		del_timer(&host->timer);
		spin_unlock_irqrestore(&host->mmc_lock, flags);

		mmc_remove_host(host->mmc);
		platform_set_drvdata(dev, NULL);
		free_irq(host->irq, host);
		davinci_release_dma_channels(host);
		clk_disable(host->clk);
		clk_put(host->clk);
		release_resource(host->mem_res);
		kfree(host->mem_res);
		mmc_free_host(host->mmc);
	}

	return 0;
}

#ifdef CONFIG_PM
static int davinci_mmcsd_suspend(struct platform_device *pdev,
		pm_message_t state)
{
	struct mmc_davinci_host *host = platform_get_drvdata(pdev);
	int ret = 0;

	if (host && host->mmc)
		ret = mmc_suspend_host(host->mmc, state);

	return ret;
}

static int davinci_mmcsd_resume(struct platform_device *pdev)
{
	struct mmc_davinci_host *host = platform_get_drvdata(pdev);
	int ret = 0;

	if (host && host->mmc)
		ret = mmc_resume_host(host->mmc);

	return ret;
}
#else
#define davinci_mmcsd_suspend	NULL
#define davinci_mmcsd_resume	NULL
#endif

static struct platform_driver davinci_mmcsd_driver = {
	.probe 		= davinci_mmc_probe,
	.remove		= davinci_mmcsd_remove,
	.suspend	= davinci_mmcsd_suspend,
	.resume		= davinci_mmcsd_resume,
	.driver		= {
		.name	= DRIVER_NAME,
	},
};


static int davinci_mmcsd_init(void)
{
	return platform_driver_register(&davinci_mmcsd_driver);
}

static void __exit davinci_mmcsd_exit(void)
{
	platform_driver_unregister(&davinci_mmcsd_driver);
}

module_init(davinci_mmcsd_init);
module_exit(davinci_mmcsd_exit);

MODULE_DESCRIPTION("DAVINCI Multimedia Card driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS(DRIVER_NAME);
MODULE_AUTHOR("Texas Instruments");
