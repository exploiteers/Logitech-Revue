/*
 * Synopsys DesignWare Cores (DWC) SATA host driver
 *
 * Author: Mark Miesfeld <mmiesfeld@amcc.com>
 *
 * Ported from 2.6.19.2 to 2.6.25/26 by Stefan Roese <sr@denx.de>
 * Copyright 2008 DENX Software Engineering
 *
 * Based on versions provided by AMCC and Synopsys which are:
 *          Copyright 2006 Applied Micro Circuits Corporation
 *          COPYRIGHT (C) 2005  SYNOPSYS, INC.  ALL RIGHTS RESERVED
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#ifdef CONFIG_SATA_DWC_DEBUG
#define DEBUG
#endif

#ifdef CONFIG_SATA_DWC_VDEBUG
#define VERBOSE_DEBUG
#define DEBUG_NCQ
#endif

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/libata.h>

#include <scsi/scsi_host.h>
#include <scsi/scsi_cmnd.h>

#define DRV_NAME        "sata-dwc"
#define DRV_VERSION     "1.0"

/* SATA DMA driver Globals */
#define DMA_NUM_CHANS			1
#define DMA_NUM_CHAN_REGS		8

/* SATA DMA Register definitions */
#define AHB_DMA_BRST_DFLT		16	/* 4 data items burst length */

struct dmareg {
	u32 low;		/* Low bits 0-31 */
	u32 high;		/* High bits 32-63 */
};

/* DMA Per Channel registers */

struct dma_chan_regs {
	struct dmareg sar;	/* Source Address */
	struct dmareg dar;	/* Destination address */
	struct dmareg llp;	/* Linked List Pointer */
	struct dmareg ctl;	/* Control */
	struct dmareg sstat;	/* Source Status not implemented in core */
	struct dmareg dstat;	/* Destination Status not implemented in core */
	struct dmareg sstatar;	/* Source Status Address not impl in core */
	struct dmareg dstatar;	/* Destination Status Address not implemented */
	struct dmareg cfg;	/* Config */
	struct dmareg sgr;	/* Source Gather */
	struct dmareg dsr;	/* Destination Scatter */
};

/* Generic Interrupt Registers */
struct dma_interrupt_regs {
	struct dmareg tfr;	/* Transfer Interrupt */
	struct dmareg block;	/* Block Interrupt */
	struct dmareg srctran;	/* Source Transfer Interrupt */
	struct dmareg dsttran;	/* Dest Transfer Interrupt */
	struct dmareg error;	/* Error */
};

struct ahb_dma_regs {
	struct dma_chan_regs	chan_regs[DMA_NUM_CHAN_REGS];	/* Channel Registers */
	struct dma_interrupt_regs interrupt_raw;	/* Raw Interrupt */
	struct dma_interrupt_regs interrupt_status;	/* Interrupt Status */
	struct dma_interrupt_regs interrupt_mask;	/* Interrupt Mask */
	struct dma_interrupt_regs interrupt_clear;	/* Interrupt Clear */
	struct dmareg		statusInt;		/* Interrupt combined */
	struct dmareg		rq_srcreg;		/* Src Trans Req */
	struct dmareg		rq_dstreg;		/* Dst Trans Req */
	struct dmareg		rq_sgl_srcreg;		/* Sngl Src Trans Req */
	struct dmareg		rq_sgl_dstreg;		/* Sngl Dst Trans Req */
	struct dmareg		rq_lst_srcreg;		/* Last Src Trans Req */
	struct dmareg		rq_lst_dstreg;		/* Last Dst Trans Req */
	struct dmareg		dma_cfg;		/* DMA Config */
	struct dmareg		dma_chan_en;		/* DMA Channel Enable */
	struct dmareg		dma_id;			/* DMA ID */
	struct dmareg		dma_test;		/* DMA Test */
	struct dmareg		res1;			/* reserved */
	struct dmareg		res2;			/* reserved */

	/* DMA Comp Params
	 * Param 6 = dma_param[0], Param 5 = dma_param[1],
	 * Param 4 = dma_param[2] ...
	 */
	struct dmareg		dma_params[6];
};

/* Data structure for linked list item */
struct lli {
	u32		sar;		/* Source Address */
	u32		dar;		/* Destination address */
	u32		llp;		/* Linked List Pointer */
	struct dmareg	ctl;		/* Control */
	struct dmareg	dstat;		/* Destination Status */
};

#define SATA_DWC_DMAC_LLI_SZ		(sizeof(struct lli))
#define SATA_DWC_DMAC_LLI_NUM		256
#define SATA_DWC_DMAC_LLI_TBL_SZ	(SATA_DWC_DMAC_LLI_SZ * SATA_DWC_DMAC_LLI_NUM)
#define SATA_DWC_DMAC_TWIDTH_BYTES	4
#define SATA_DWC_DMAC_CTRL_TSIZE_MAX	(0x00000800 * SATA_DWC_DMAC_TWIDTH_BYTES)

/* DMA Register Operation Bits */
#define DMA_EN			0x00000001		/* Enable AHB DMA */
#define DMA_CHANNEL(ch)		(0x00000001 << (ch))	/* Select channel */
#define DMA_ENABLE_CHAN(ch)	((0x00000001 << (ch)) |			\
				 ((0x000000001 << (ch)) << 8))	/* Enable channel */
#define DMA_DISABLE_CHAN(ch)	((0x000000001 << (ch)) << 8)	/* Disable channel */

/* Channel Control Register */
#define DMA_CTL_BLK_TS(size)	((size) & 0x000000FFF)	/* Blk Transfer size */
#define DMA_CTL_LLP_SRCEN	0x10000000		/* Blk chain enable Src */
#define DMA_CTL_LLP_DSTEN	0x08000000		/* Blk chain enable Dst */
/*
 * This define is used to set block chaining disabled in the control low
 * register.  It is already in little endian format so it can be &'d dirctly.
 * It is essentially: cpu_to_le32(~(DMA_CTL_LLP_SRCEN | DMA_CTL_LLP_DSTEN))
 */
#define DMA_CTL_LLP_DISABLE_LE32 0xffffffe7
#define DMA_CTL_SMS(num)	((num & 0x3) << 25)	/* Src Master Select */
#define DMA_CTL_DMS(num)	((num & 0x3) << 23)	/* Dst Master Select */
#define DMA_CTL_TTFC(type)	((type & 0x7) << 20)	/* Transfer Type & Flow Controller */
#define DMA_CTL_TTFC_P2M_DMAC	0x00000002		/* Per to mem, DMAC cntr */
#define DMA_CTL_TTFC_M2P_PER	0x00000003		/* Mem to per, peripheral cntr */
#define DMA_CTL_SRC_MSIZE(size)	((size & 0x7) << 14)	/* Src Burst Transaction Length */
#define DMA_CTL_DST_MSIZE(size)	((size & 0x7) << 11)	/* Dst Burst Transaction Length */
#define DMA_CTL_SINC_INC	0x00000000		/* Source Address Increment */
#define DMA_CTL_SINC_DEC	0x00000200
#define DMA_CTL_SINC_NOCHANGE	0x00000400
#define DMA_CTL_DINC_INC	0x00000000		/* Destination Address Increment */
#define DMA_CTL_DINC_DEC	0x00000080
#define DMA_CTL_DINC_NOCHANGE	0x00000100
#define DMA_CTL_SRC_TRWID(size)	((size & 0x7) << 4)	/* Source Transfer Width */
#define DMA_CTL_DST_TRWID(size)	((size & 0x7) << 1)	/* Destination Transfer Width */
#define DMA_CTL_INT_EN		0x00000001		/* Interrupt Enable */

/* Channel Configuration Register high bits */
#define DMA_CFG_FCMOD_REQ	0x00000001		/* Flow Control - request based */
#define DMA_CFG_PROTCTL		(0x00000003 << 2)	/* Protection Control */

/* Channel Configuration Register low bits */
#define DMA_CFG_RELD_DST	0x80000000		/* Reload Dest / Src Addr */
#define DMA_CFG_RELD_SRC	0x40000000
#define DMA_CFG_HS_SELSRC	0x00000800		/* Software handshake Src / Dest */
#define DMA_CFG_HS_SELDST	0x00000400
#define DMA_CFG_FIFOEMPTY       (0x00000001 << 9)	/* FIFO Empty bit */

/* Assign hardware handshaking interface (x) to destination / source peripheral */
#define DMA_CFG_HW_HS_DEST(int_num)	((int_num & 0xF) << 11)
#define DMA_CFG_HW_HS_SRC(int_num)	((int_num & 0xF) << 7)

/* Channel Linked List Pointer Register */
#define DMA_LLP_LMS(addr, master)	(((addr) & 0xfffffffc) | (master))
#define DMA_LLP_AHBMASTER1		0		/* List Master Select */
#define DMA_LLP_AHBMASTER2		1

#define SATA_DWC_MAX_PORTS	1

#define SATA_DWC_SCR_OFFSET	0x24
#define SATA_DWC_REG_OFFSET	0x64

/* DWC SATA Registers */
struct sata_dwc_regs {
	u32 fptagr;		/* 1st party DMA tag */
	u32 fpbor;		/* 1st party DMA buffer offset */
	u32 fptcr;		/* 1st party DMA Xfr count */
	u32 dmacr;		/* DMA Control */
	u32 dbtsr;		/* DMA Burst Transac size */
	u32 intpr;		/* Interrupt Pending */
	u32 intmr;		/* Interrupt Mask */
	u32 errmr;		/* Error Mask */
	u32 llcr;		/* Link Layer Control */
	u32 phycr;		/* PHY Control */
	u32 physr;		/* PHY Status */
	u32 rxbistpd;		/* Recvd BIST pattern def register */
	u32 rxbistpd1;		/* Recvd BIST data dword1 */
	u32 rxbistpd2;		/* Recvd BIST pattern data dword2 */
	u32 txbistpd;		/* Trans BIST pattern def register */
	u32 txbistpd1;		/* Trans BIST data dword1 */
	u32 txbistpd2;		/* Trans BIST data dword2 */
	u32 bistcr;		/* BIST Control Register */
	u32 bistfctr;		/* BIST FIS Count Register */
	u32 bistsr;		/* BIST Status Register */
	u32 bistdecr;		/* BIST Dword Error count register */
	u32 res[15];		/* Reserved locations */
	u32 testr;		/* Test Register */
	u32 versionr;		/* Version Register */
	u32 idr;		/* ID Register */
	u32 unimpl[192];	/* Unimplemented */
	u32 dmadr[256];	/* FIFO Locations in DMA Mode */
};

#define SCR_SCONTROL_DET_ENABLE		0x00000001
#define SCR_SSTATUS_DET_PRESENT		0x00000001
#define SCR_SERROR_DIAG_X		0x04000000

/* DWC SATA Register Operations */
#define	SATA_DWC_TXFIFO_DEPTH		0x01FF
#define	SATA_DWC_RXFIFO_DEPTH		0x01FF

#define SATA_DWC_DMACR_TMOD_TXCHEN	0x00000004
#define	SATA_DWC_DMACR_TXCHEN		(0x00000001 | SATA_DWC_DMACR_TMOD_TXCHEN)
#define	SATA_DWC_DMACR_RXCHEN		(0x00000002 | SATA_DWC_DMACR_TMOD_TXCHEN)
#define SATA_DWC_DMACR_TX_CLEAR(v)	(((v) & ~SATA_DWC_DMACR_TXCHEN) | SATA_DWC_DMACR_TMOD_TXCHEN)
#define SATA_DWC_DMACR_RX_CLEAR(v)	(((v) & ~SATA_DWC_DMACR_RXCHEN) | SATA_DWC_DMACR_TMOD_TXCHEN)
#define SATA_DWC_DMACR_TXRXCH_CLEAR	SATA_DWC_DMACR_TMOD_TXCHEN

#define SATA_DWC_DBTSR_MWR(size)	((size/4) & SATA_DWC_TXFIFO_DEPTH)
#define SATA_DWC_DBTSR_MRD(size)	(((size/4) & SATA_DWC_RXFIFO_DEPTH) << 16)
#define	SATA_DWC_INTPR_DMAT		0x00000001
#define SATA_DWC_INTPR_NEWFP		0x00000002
#define SATA_DWC_INTPR_PMABRT		0x00000004
#define SATA_DWC_INTPR_ERR		0x00000008
#define SATA_DWC_INTPR_NEWBIST		0x00000010
#define SATA_DWC_INTPR_IPF		0x10000000
#define	SATA_DWC_INTMR_DMATM		0x00000001
#define SATA_DWC_INTMR_NEWFPM		0x00000002
#define SATA_DWC_INTMR_PMABRTM		0x00000004
#define SATA_DWC_INTMR_ERRM		0x00000008
#define SATA_DWC_INTMR_NEWBISTM		0x00000010
#define SATA_DWC_LLCR_SCRAMEN		0x00000001
#define SATA_DWC_LLCR_DESCRAMEN		0x00000002
#define SATA_DWC_LLCR_RPDEN		0x00000004

/* This is all error bits, zero's are reserved fields. */
#define SATA_DWC_SERROR_ERR_BITS	0x0FFF0F03

#define SATA_DWC_SCR0_SPD_GET(v)	((v >> 4) & 0x0000000F)

struct sata_dwc_device {
	struct device			*dev;		/* generic device struct */
	struct ata_host_set		*host;
	void __iomem			*reg_base;
	struct sata_dwc_regs __iomem	*sata_dwc_regs;	/* DW Synopsys SATA specific */
	int				irq_dma;
};

#define SATA_DWC_QCMD_MAX	32

struct sata_dwc_device_port {
	struct sata_dwc_device	*hsdev;
	int			cmd_issued[SATA_DWC_QCMD_MAX];
	struct lli		*llit[SATA_DWC_QCMD_MAX];	/* Our DMA LLI table */
	dma_addr_t		llit_dma[SATA_DWC_QCMD_MAX];
	u32			dma_chan[SATA_DWC_QCMD_MAX];
	int			dma_pending[SATA_DWC_QCMD_MAX];
};

/*
 * Commonly used DWC SATA driver Macros
 */
#define HSDEV_FROM_HOST(host)	(struct sata_dwc_device *)(host)->private_data
#define HSDEV_FROM_AP(ap)	(struct sata_dwc_device *)(ap)->host_set->private_data
#define HSDEVP_FROM_AP(ap)	(struct sata_dwc_device_port *)(ap)->private_data
#define HSDEV_FROM_QC(qc)	(struct sata_dwc_device *)(qc)->ap->host_set->private_data
#define HSDEV_FROM_HSDEVP(p)	(struct sata_dwc_device *)(hsdevp)->hsdev

enum {
	SATA_DWC_CMD_ISSUED_NOT		= 0,
	SATA_DWC_CMD_ISSUED_PEND	= 1,
	SATA_DWC_CMD_ISSUED_EXEC	= 2,
	SATA_DWC_CMD_ISSUED_NODATA	= 3,

	SATA_DWC_DMA_PENDING_NONE	= 0,
	SATA_DWC_DMA_PENDING_TX		= 1,
	SATA_DWC_DMA_PENDING_RX		= 2,
};

/*
 * Globals
 */
static u32 sata_dwc_sactive_issued;	/* issued queued ops */
static u32 sata_dwc_sactive_queued;	/* queued ops */
static struct ahb_dma_regs *sata_dma_regs;
static u32 dma_interrupt_count;
static void *scr_addr_sstatus;
static struct device *dwc_dev;

/*
 * Prototypes
 */
static void sata_dwc_bmdma_start_by_tag(struct ata_queued_cmd *qc, u8 tag);
static int sata_dwc_qc_complete(struct ata_port *ap, struct ata_queued_cmd *qc,
				u32 check_status);
static void sata_dwc_dma_xfer_complete(struct ata_port *ap, u32 check_status);
static void sata_dwc_port_stop(struct ata_port *ap);
static void sata_dwc_clear_dmacr(struct sata_dwc_device_port *hsdevp, u8 tag);

static int dma_dwc_init(struct sata_dwc_device *hsdev, struct resource *res,
			int irq);
static void dma_dwc_exit(struct sata_dwc_device *hsdev);
static int dma_dwc_xfer_setup(struct ata_queued_cmd *qc,
			      struct lli *lli, dma_addr_t dma_lli,
			      void __iomem *addr, int dir);
static void dma_dwc_xfer_start(int dma_ch);
static void dma_dwc_terminate_dma(int dma_ch);

static inline const char *dir_2_txt(enum dma_data_direction dir)
{
	switch (dir) {
	case DMA_BIDIRECTIONAL:
		return "bi";
	case DMA_FROM_DEVICE:
		return "from";
	case DMA_TO_DEVICE:
		return "to";
	case DMA_NONE:
		return "none";
	default:
		return "err";
	}
}

static inline const char *prot_2_txt(enum ata_tf_protocols protocol)
{
	switch (protocol) {
	case ATA_PROT_UNKNOWN:
		return "unknown";
	case ATA_PROT_NODATA:
		return "nodata";
	case ATA_PROT_PIO:
		return "pio";
	case ATA_PROT_DMA:
		return "dma";
	case ATA_PROT_NCQ:
		return "ncq";
	default:
		return "err";
	}
}

static inline const char *ata_cmd_2_txt(const struct ata_taskfile *tf)
{
	switch (tf->command) {
	case ATA_CMD_CHK_POWER :
		return "ATA_CMD_CHK_POWER";
	case ATA_CMD_EDD :
		return "ATA_CMD_EDD";
	case ATA_CMD_FLUSH :
		return "ATA_CMD_FLUSH";
	case ATA_CMD_FLUSH_EXT :
		return "ATA_CMD_FLUSH_EXT";
	case ATA_CMD_ID_ATA :
		return "ATA_CMD_ID_ATA";
	case ATA_CMD_ID_ATAPI :
		return "ATA_CMD_ID_ATAPI";
	case ATA_CMD_FPDMA_READ :
		return "ATA_CMD_FPDMA_READ";
	case ATA_CMD_FPDMA_WRITE :
		return "ATA_CMD_FPDMA_WRITE";
	case ATA_CMD_READ :
		return "ATA_CMD_READ";
	case ATA_CMD_READ_EXT :
		return "ATA_CMD_READ_EXT";
	case ATA_CMD_WRITE :
		return "ATA_CMD_WRITE";
	case ATA_CMD_WRITE_EXT :
		return "ATA_CMD_WRITE_EXT";
	case ATA_CMD_PIO_READ :
		return "ATA_CMD_PIO_READ";
	case ATA_CMD_PIO_READ_EXT :
		return "ATA_CMD_PIO_READ_EXT";
	case ATA_CMD_PIO_WRITE :
		return "ATA_CMD_PIO_WRITE";
	case ATA_CMD_PIO_WRITE_EXT :
		return "ATA_CMD_PIO_WRITE_EXT";
	case ATA_CMD_SET_FEATURES :
		return "ATA_CMD_SET_FEATURES";
	case ATA_CMD_PACKET :
		return "ATA_CMD_PACKET";
	default :
		return "ATA_CMD_???";
	}
}

static inline int sata_dwc_ata_is_dma(u8 prot)
{
	return ((prot == ATA_PROT_DMA) || (prot == ATA_PROT_ATAPI_DMA) ||
		(prot == ATA_PROT_NCQ));
}

static inline int sata_dwc_ata_is_ncq(u8 prot)
{
	return (prot == ATA_PROT_NCQ);
}

static void sata_dwc_tf_dump(struct ata_taskfile *tf)
{
	dev_vdbg(dwc_dev, "taskfile cmd: 0x%02x protocol: %s"
			  " flags: 0x%lx device: %x\n",
		 tf->command, prot_2_txt(tf->protocol),
		 tf->flags, tf->device);
	dev_vdbg(dwc_dev, "feature: 0x%02x nsect: 0x%x lbal: 0x%x"
			  " lbam: 0x%x lbah: 0x%x\n",
		 tf->feature, tf->nsect, tf->lbal,
		 tf->lbam, tf->lbah);
	dev_vdbg(dwc_dev, "hob_feature: 0x%02x hob_nsect: 0x%x hob_lbal: 0x%x"
			  " hob_lbam: 0x%x hob_lbah: 0x%x\n",
		 tf->hob_feature, tf->hob_nsect,
		 tf->hob_lbal, tf->hob_lbam, tf->hob_lbah);
}

/*******************************************************************************
 * Function: get_burst_length_encode
 * arguments: datalength: length in bytes of data
 * returns value to be programmed in register corrresponding to data length
 * This value is effectively the log(base 2) of the length
 ******************************************************************************/
static inline int get_burst_length_encode(int datalength)
{
	int items = datalength >> 2;	/* div by 4 to get lword count */

	if (items >= 64)
		return 5;

	if (items >= 32)
		return 4;

	if (items >= 16)
		return 3;

	if (items >= 8)
		return 2;

	if (items >= 4)
		return 1;

	return 0;
}

static inline void clear_chan_interrupts(int c)
{
	out_le32(&(sata_dma_regs->interrupt_clear.tfr.low), DMA_CHANNEL(c));
	out_le32(&(sata_dma_regs->interrupt_clear.block.low), DMA_CHANNEL(c));
	out_le32(&(sata_dma_regs->interrupt_clear.srctran.low), DMA_CHANNEL(c));
	out_le32(&(sata_dma_regs->interrupt_clear.dsttran.low), DMA_CHANNEL(c));
	out_le32(&(sata_dma_regs->interrupt_clear.error.low), DMA_CHANNEL(c));
}

/*******************************************************************************
 * Function: dma_request_channel
 * arguments: None
 * returns channel number if available else -1
 * This function assigns the next available DMA channel from the list to the
 * requester
 ******************************************************************************/
static int dma_request_channel(void)
{
	int i;

	for (i = 0; i < DMA_NUM_CHANS; i++) {
		if (!(in_le32(&(sata_dma_regs->dma_chan_en.low)) &
		    DMA_CHANNEL(i)))
			return i;
	}

	/* TODO this should maybe change to warn of info */
	dev_err(dwc_dev, "%s NO channel chan_en: 0x%08x\n", __func__,
		in_le32(&(sata_dma_regs->dma_chan_en.low)));

	return -1;
}

/*******************************************************************************
 * Function: dma_dwc_interrupt
 * arguments: irq, dev_id, pt_regs
 * returns channel number if available else -1
 * Interrupt Handler for DW AHB SATA DMA
 ******************************************************************************/
static irqreturn_t dma_dwc_interrupt(int irq, void *hsdev_instance,
					struct pt_regs *regs)
{
	int chan;
	u32 tfr_reg, err_reg;
	unsigned long flags;
	struct sata_dwc_device *hsdev =
		(struct sata_dwc_device *)hsdev_instance;
	struct ata_host_set *host = (struct ata_host_set *)hsdev->host;
	struct ata_port *ap;
	struct sata_dwc_device_port *hsdevp;
	u8 tag = 0;
	unsigned int port = 0;

	spin_lock_irqsave(&host->lock, flags);

	ap = host->ports[port];
	hsdevp = HSDEVP_FROM_AP(ap);
	tag = ap->active_tag;

	tfr_reg = in_le32(&(sata_dma_regs->interrupt_status.tfr.low));
	err_reg = in_le32(&(sata_dma_regs->interrupt_status.error.low));

	dev_dbg(ap->dev, "eot=0x%08x err=0x%08x pending=%d active port=%d\n",
		tfr_reg, err_reg, hsdevp->dma_pending[tag], port);

	for (chan = 0; chan < DMA_NUM_CHANS; chan++) {
		/* Check for end-of-transfer interrupt. */
		if (tfr_reg & DMA_CHANNEL(chan)) {
			/* Each DMA command produces 2 interrupts.  Only
			 * complete the command after both interrupts have been
			 * seen. (See sata_dwc_isr())
			 */
			dma_interrupt_count++;

			sata_dwc_clear_dmacr(hsdevp, tag);

			if (hsdevp->dma_pending[tag] == 0) {
				dev_err(ap->dev, "DMA not pending eot=0x%08x "
					"err=0x%08x tag=0x%02x pending=%d\n",
					tfr_reg, err_reg, tag,
					hsdevp->dma_pending[tag]);
			}

			if ((dma_interrupt_count % 2) == 0)
				sata_dwc_dma_xfer_complete(ap, 1);

			/* Clear the interrupt */
			out_le32(&(sata_dma_regs->interrupt_clear.tfr.low),
				 DMA_CHANNEL(chan));
		}

		/* Check for error interrupt. */
		if (err_reg & DMA_CHANNEL(chan)) {
			/* TODO Need error handler ! */
			dev_err(ap->dev, "error interrupt err_reg=0x%08x\n",
				err_reg);

			/* Clear the interrupt. */
			out_le32(&(sata_dma_regs->interrupt_clear.error.low),
				 DMA_CHANNEL(chan));
		}
	}

	spin_unlock_irqrestore(&host->lock, flags);
	return IRQ_HANDLED;
}

/*******************************************************************************
 * Function: dma_request_interrupts
 * arguments: hsdev
 * returns status
 * This function registers ISR for a particular DMA channel interrupt
 ******************************************************************************/
static int dma_request_interrupts(struct sata_dwc_device *hsdev, int irq)
{
	int retval = 0;
	int chan;

	for (chan = 0; chan < DMA_NUM_CHANS; chan++) {
		/* Unmask error interrupt */
		out_le32(&sata_dma_regs->interrupt_mask.error.low,
			 DMA_ENABLE_CHAN(chan));

		/* Unmask end-of-transfer interrupt */
		out_le32(&sata_dma_regs->interrupt_mask.tfr.low,
			 DMA_ENABLE_CHAN(chan));
	}

	retval = request_irq(irq, dma_dwc_interrupt, 0, "SATA DMA", hsdev);
	if (retval) {
		dev_err(dwc_dev, "%s: could not get IRQ %d\n", __func__, irq);
		return -ENODEV;
	}

	/* Mark this interrupt as requested */
	hsdev->irq_dma = irq;

	return 0;
}

/*******************************************************************************
 * Function: map_sg_to_lli
 * arguments: sg: scatter/gather list(sg)
 *	      num_elems: no of elements in sg list
 *	      dma_lli: LLI table
 *	      dest: destination address
 *	      read: whether the transfer is read or write
 * returns array of AHB DMA Linked List Items
 * This function creates a list of LLIs for DMA Xfr and returns the number
 * of elements in the DMA linked list.
 *
 * Note that the Synopsis driver has a comment proposing that better performance
 * is possible by only enabling interrupts on the last item in the linked list.
 * However, it seems that could be a problem if an error happened on one of the
 * first items.  The transfer would halt, but no error interrupt would occur.
 *
 * Currently this function sets interrupts enabled for each linked list item:
 * DMA_CTL_INT_EN.
 ******************************************************************************/
static int map_sg_to_lli(struct ata_queued_cmd *qc, struct lli *lli,
			 dma_addr_t dma_lli, void __iomem *dmadr_addr, int dir)
{
	int idx = 0;
	int fis_len = 0;
	dma_addr_t next_llp;
	int bl;
	struct scatterlist *sg;

	dev_dbg(dwc_dev, "%s: qc=%p lli=%p dma_lli=0x%08x dmadr=0x%08x\n",
		__func__, qc, lli, dma_lli, (u32)dmadr_addr);

	bl = get_burst_length_encode(AHB_DMA_BRST_DFLT);

	ata_for_each_sg(sg, qc) {
		u32 addr, offset;
		u32 sg_len, len;

		addr = (u32) sg_dma_address(sg);
		sg_len = sg_dma_len(sg);

		dev_dbg(dwc_dev, "%s: sg=%p sg_addr=0x%x sg_len=%d\n",
			__func__, sg, addr, sg_len);

		while (sg_len) {

			if (idx >= SATA_DWC_DMAC_LLI_NUM) {
				/* The LLI table is not large enough. */
				dev_err(dwc_dev, "LLI table overrun (idx=%d)\n",
					idx);
				break;
			}
			len = (sg_len > SATA_DWC_DMAC_CTRL_TSIZE_MAX) ?
				SATA_DWC_DMAC_CTRL_TSIZE_MAX : sg_len;

			offset = addr & 0xffff;
			if ((offset + sg_len) > 0x10000)
				len = 0x10000 - offset;

			/*
			 * Make sure a LLI block is not created that will span a
			 * 8K max FIS boundary.  If the block spans such a FIS
			 * boundary, there is a chance that a DMA burst will
			 * cross that boundary -- this results in an error in
			 * the host controller.
			 */
			if (fis_len + len > 8192) {
				dev_dbg(dwc_dev, "SPLITTING: fis_len=%d(0x%x) "
						 "len=%d(0x%x)\n",
					fis_len, fis_len, len, len);
				len = 8192 - fis_len;
				fis_len = 0;
			} else {
				fis_len += len;
			}
			if (fis_len == 8192)
				fis_len = 0;

			/*
			 * Set DMA addresses and lower half of control register
			 * based on direction.
			 */
			if (dir == DMA_FROM_DEVICE) {
				lli[idx].dar = cpu_to_le32(addr);
				lli[idx].sar = cpu_to_le32((u32)dmadr_addr);

				lli[idx].ctl.low = cpu_to_le32(
					DMA_CTL_TTFC(DMA_CTL_TTFC_P2M_DMAC) |
					DMA_CTL_SMS(0) |
					DMA_CTL_DMS(1) |
					DMA_CTL_SRC_MSIZE(bl) |
					DMA_CTL_DST_MSIZE(bl) |
					DMA_CTL_SINC_NOCHANGE |
					DMA_CTL_SRC_TRWID(2) |
					DMA_CTL_DST_TRWID(2) |
					DMA_CTL_INT_EN |
					DMA_CTL_LLP_SRCEN |
					DMA_CTL_LLP_DSTEN);
			} else {	/* DMA_TO_DEVICE */
				lli[idx].sar = cpu_to_le32(addr);
				lli[idx].dar = cpu_to_le32((u32)dmadr_addr);

				lli[idx].ctl.low = cpu_to_le32(
					DMA_CTL_TTFC(DMA_CTL_TTFC_M2P_PER) |
					DMA_CTL_SMS(1) |
					DMA_CTL_DMS(0) |
					DMA_CTL_SRC_MSIZE(bl) |
					DMA_CTL_DST_MSIZE(bl) |
					DMA_CTL_DINC_NOCHANGE |
					DMA_CTL_SRC_TRWID(2) |
					DMA_CTL_DST_TRWID(2) |
					DMA_CTL_INT_EN |
					DMA_CTL_LLP_SRCEN |
					DMA_CTL_LLP_DSTEN);
			}

			dev_dbg(dwc_dev, "%s setting ctl.high len: 0x%08x"
					 " val: 0x%08x\n",
				__func__, len, DMA_CTL_BLK_TS(len / 4));

			/* Program the LLI CTL high register */
			lli[idx].ctl.high = cpu_to_le32(DMA_CTL_BLK_TS(len / 4));

			/* Program the next pointer.  The next pointer must be
			 * the physical address, not the virtual address.
			 */
			next_llp = (dma_lli + ((idx + 1) * sizeof(struct lli)));

			/* The last 2 bits encode the list master select. */
			next_llp = DMA_LLP_LMS(next_llp, DMA_LLP_AHBMASTER2);

			lli[idx].llp = cpu_to_le32(next_llp);

			idx++;
			sg_len -= len;
			addr += len;
		}
	}

	/*
	 * The last next ptr has to be zero and the last control low register
	 * has to have LLP_SRC_EN and LLP_DST_EN (linked list pointer source
	 * and destination enable) set back to 0 (disabled.)  This is what tells
	 * the core that this is the last item in the linked list.
	 */
	if (idx) {
		lli[idx-1].llp = 0x00000000;
		lli[idx-1].ctl.low &= DMA_CTL_LLP_DISABLE_LE32;

		/* Flush cache to memory */
		dma_cache_sync(lli, (sizeof(struct lli) * idx),
			       DMA_BIDIRECTIONAL);
	}

	return idx;
}

/*******************************************************************************
 * Function: dma_dwc_xfer_start
 * arguments: Channel number
 * Return : None
 * Enables the DMA channel
 ******************************************************************************/
static void dma_dwc_xfer_start(int dma_ch)
{
	/* Enable the DMA channel */
	out_le32(&(sata_dma_regs->dma_chan_en.low),
		 in_le32(&(sata_dma_regs->dma_chan_en.low)) |
		 DMA_ENABLE_CHAN(dma_ch));
}

static int dma_dwc_channel_enabled(int ch)
{
	if (in_le32(&(sata_dma_regs->dma_chan_en.low)) & DMA_CHANNEL(ch))
		return 1;

	return 0;
}

static void dma_dwc_terminate_dma(int dma_ch)
{
	int enabled = dma_dwc_channel_enabled(dma_ch);

	dev_info(dwc_dev, "%s terminate DMA on channel=%d enabled=%d\n",
		 __func__, dma_ch, enabled);

	if (enabled)  {
		out_le32(&(sata_dma_regs->dma_chan_en.low),
			 DMA_DISABLE_CHAN(dma_ch));

		do {
			enabled = dma_dwc_channel_enabled(dma_ch);
		} while (enabled);
	}
}

static int dma_dwc_xfer_setup(struct ata_queued_cmd *qc,
			      struct lli *lli, dma_addr_t dma_lli,
			      void __iomem *addr, int dir)
{
	int dma_ch;
	int num_lli;

	/* Acquire DMA channel */
	dma_ch = dma_request_channel();
	if (dma_ch == -1) {
		dev_err(dwc_dev, "%s: dma channel unavailable\n", __func__);
		return -EAGAIN;
	}

	/* Convert SG list to linked list of items (LLIs) for AHB DMA */
	num_lli = map_sg_to_lli(qc, lli, dma_lli, addr, dir);

	dev_dbg(dwc_dev, "%s qc: 0x%p, count: %d lli: %p dma_lli: 0x%0xlx"
			 " addr: %p lli count: %d\n",
		__func__, qc, qc->n_elem, lli, dma_lli, addr, num_lli);

	/* Clear channel interrupts */
	clear_chan_interrupts(dma_ch);

	/* Program the CFG register. */
	out_le32(&(sata_dma_regs->chan_regs[dma_ch].cfg.high),
		 DMA_CFG_PROTCTL | DMA_CFG_FCMOD_REQ);
	out_le32(&(sata_dma_regs->chan_regs[dma_ch].cfg.low), 0);

	/* Program the address of the linked list */
	out_le32(&(sata_dma_regs->chan_regs[dma_ch].llp.low),
		 DMA_LLP_LMS(dma_lli, DMA_LLP_AHBMASTER2));

	/* Program the CTL register with src enable / dst enable */
	out_le32(&(sata_dma_regs->chan_regs[dma_ch].ctl.low),
		 DMA_CTL_LLP_SRCEN | DMA_CTL_LLP_DSTEN);

	return 0;
}

/*******************************************************************************
 * Function: dma_dwc_exit
 * arguments: None
 * returns status
 * This function exits the SATA DMA driver
 ******************************************************************************/
static void dma_dwc_exit(struct sata_dwc_device *hsdev)
{
	dev_dbg(dwc_dev, "%s:\n", __func__);
	if (sata_dma_regs)
		iounmap(sata_dma_regs);

	if (hsdev->irq_dma)
		free_irq(hsdev->irq_dma, hsdev);
}

/*******************************************************************************
 * Function: dma_dwc_init
 * arguments: hsdev
 * returns status
 * This function initializes the SATA DMA driver
 ******************************************************************************/
static int dma_dwc_init(struct sata_dwc_device *hsdev, struct resource *res,
			int irq)
{
	int err;

	sata_dma_regs = ioremap(res->start, res->end - res->start + 1);
	if (!sata_dma_regs) {
		dev_err(dwc_dev, "%s: ioremap failed\n", __func__);
		err = -ENODEV;
		goto error_out;
	}

	err = dma_request_interrupts(hsdev, irq);
	if (err) {
		dev_err(dwc_dev, "%s: dma_request_interrupts returns %d\n",
			__func__, err);
		goto error_out;
	}

	/* Enabe DMA */
	out_le32(&(sata_dma_regs->dma_cfg.low), DMA_EN);

	dev_notice(dwc_dev, "DMA initialized\n");
	dev_dbg(dwc_dev, "SATA DMA registers=0x%p\n", sata_dma_regs);

	return 0;

error_out:
	dma_dwc_exit(hsdev);

	return err;
}

static u32 sata_dwc_scr_read(struct ata_port *ap, unsigned int scr)
{
	u32 val;

	if (scr > SCR_NOTIFICATION) {
		dev_err(ap->dev, "%s: Incorrect SCR offset 0x%02x\n",
			__func__, scr);
		return 0;
	}

	val = in_le32((void *)ap->ioaddr.scr_addr + (scr * 4));
	dev_dbg(ap->dev, "%s: id=%d reg=%d val=val=0x%08x\n",
		__func__, ap->id, scr, val);

	return val;
}

static void sata_dwc_scr_write(struct ata_port *ap, unsigned int scr, u32 val)
{
	dev_dbg(ap->dev, "%s: id=%d reg=%d val=val=0x%08x\n",
		__func__, ap->id, scr, val);
	if (scr > SCR_NOTIFICATION) {
		dev_err(ap->dev, "%s: Incorrect SCR offset 0x%02x\n",
			__func__, scr);
		return ;
	}
	out_le32((void *)ap->ioaddr.scr_addr + (scr * 4), val);
}

static inline u32 core_scr_read(unsigned int scr)
{
	return in_le32((void __iomem *)scr_addr_sstatus + (scr * 4));
}

static inline void core_scr_write(unsigned int scr, u32 val)
{
	out_le32((void __iomem *)scr_addr_sstatus + (scr * 4), val);
}

static inline void clear_serror(void)
{
	out_le32((void __iomem *)scr_addr_sstatus + 4,
		 in_le32((void __iomem *)scr_addr_sstatus + 4));
}

static inline void clear_intpr(struct sata_dwc_device *hsdev)
{
	out_le32(&hsdev->sata_dwc_regs->intpr,
		 in_le32(&hsdev->sata_dwc_regs->intpr));
}

static inline void clear_interrupt_bit(struct sata_dwc_device *hsdev, u32 bit)
{
	out_le32(&hsdev->sata_dwc_regs->intpr,
		 in_le32(&hsdev->sata_dwc_regs->intpr));
}

static inline void disable_err_irq(struct sata_dwc_device *hsdev)
{
	out_le32(&hsdev->sata_dwc_regs->intmr,
		 in_le32(&hsdev->sata_dwc_regs->intmr) & ~SATA_DWC_INTMR_ERRM);
	out_le32(&hsdev->sata_dwc_regs->errmr, ~SATA_DWC_SERROR_ERR_BITS);
}

static inline void enable_err_irq(struct sata_dwc_device *hsdev)
{
	out_le32(&hsdev->sata_dwc_regs->intmr,
		 in_le32(&hsdev->sata_dwc_regs->intmr) | SATA_DWC_INTMR_ERRM);
	out_le32(&hsdev->sata_dwc_regs->errmr, SATA_DWC_SERROR_ERR_BITS);
}

static inline u32 qcmd_tag_to_mask(u8 tag)
{
	return (0x00000001 << (tag & 0x1f));
}

/* See ahci.c */
static void sata_dwc_error_intr(struct ata_port *ap,
				struct sata_dwc_device *hsdev, uint intpr)
{
	struct sata_dwc_device_port *hsdevp = HSDEVP_FROM_AP(ap);
	struct ata_eh_info *ehi = &ap->eh_info;
	unsigned int err_mask = 0, action = 0;
	struct ata_queued_cmd *qc;
	u32 serror;
	u8 status, tag;
	u32 err_reg;

	ata_ehi_clear_desc(ehi);

	serror = core_scr_read(SCR_ERROR);
	status = ata_chk_status(ap);

	err_reg = in_le32(&(sata_dma_regs->interrupt_status.error.low));
	tag = ap->active_tag;

	dev_err(ap->dev, "%s SCR_ERROR=0x%08x intpr=0x%08x status=0x%08x "
		"dma_intp=%d pending=%d issued=%d dma_err_status=0x%08x\n",
		__func__, serror, intpr, status, dma_interrupt_count,
		hsdevp->dma_pending[tag], hsdevp->cmd_issued[tag], err_reg);

	/* Clear error register and interrupt bit */
	clear_serror();
	clear_interrupt_bit(hsdev, SATA_DWC_INTPR_ERR);

	/* This is the only error happening now.  TODO check for exact error */
	err_mask |= AC_ERR_HOST_BUS;
	action |= ATA_EH_SOFTRESET;

	/* Pass this on to EH */
	ehi->serror |= serror;
	ehi->action |= action;

	qc = ata_qc_from_tag(ap, tag);
	if (qc)
		qc->err_mask |= err_mask;
	else
		ehi->err_mask |= err_mask;

	ata_port_abort(ap);

	/*
	  if (irq_stat & PORT_IRQ_FREEZE)
	  ata_port_freeze(ap);
	  else
	  ata_port_abort(ap);
	*/
}

/*******************************************************************************
 * Function : sata_dwc_isr
 * arguments : irq, void *dev_instance, struct pt_regs *regs
 * Return value : irqreturn_t - status of IRQ
 * This Interrupt handler called via port ops registered function.
 * .irq_handler = sata_dwc_isr
 ******************************************************************************/
static irqreturn_t sata_dwc_isr(int irq, void *dev_instance,
				struct pt_regs *regs)
{
	struct ata_host_set *host = (struct ata_host_set *)dev_instance;
	struct sata_dwc_device *hsdev = HSDEV_FROM_HOST(host);
	struct ata_port *ap;
	struct ata_queued_cmd *qc;
	unsigned long flags;
	u8 status, tag;
	int handled, num_processed, port = 0;
	uint intpr, sactive, sactive2, tag_mask;
	struct sata_dwc_device_port *hsdevp;

	spin_lock_irqsave(&host->lock, flags);

	/* Read the interrupt register */
	intpr = in_le32(&hsdev->sata_dwc_regs->intpr);

	ap = host->ports[port];
	hsdevp = HSDEVP_FROM_AP(ap);

	dev_dbg(ap->dev, "%s intpr=0x%08x active_tag=%d\n", __func__, intpr,
		ap->active_tag);

	/* Check for error interrupt */
	if (intpr & SATA_DWC_INTPR_ERR) {
		sata_dwc_error_intr(ap, hsdev, intpr);
		handled = 1;
		goto DONE;
	}

	/* Check for DMA SETUP FIS (FP DMA) interrupt */
	if (intpr & SATA_DWC_INTPR_NEWFP) {
		clear_interrupt_bit(hsdev, SATA_DWC_INTPR_NEWFP);

		tag = (u8)(in_le32(&hsdev->sata_dwc_regs->fptagr));
		dev_dbg(ap->dev, "%s: NEWFP tag=%d\n", __func__, tag);
		if (hsdevp->cmd_issued[tag] != SATA_DWC_CMD_ISSUED_PEND)
			dev_warn(ap->dev, "CMD tag=%d not pending?\n", tag);

		sata_dwc_sactive_issued |= qcmd_tag_to_mask(tag);

		qc = ata_qc_from_tag(ap, tag);
		/* Start FP DMA for NCQ command.  At this point the tag is the
		 * active tag.  It is the tag that matches the command about to
		 * be completed.
		 */
		qc->ap->active_tag = tag;
		sata_dwc_bmdma_start_by_tag(qc, tag);

		handled = 1;
		goto DONE;
	}

	sactive = core_scr_read(SCR_ACTIVE);
	tag_mask = (sata_dwc_sactive_issued | sactive) ^ sactive;

	/* If no sactive issued and tag_mask is zero then this is not NCQ */
	if (sata_dwc_sactive_issued == 0 && tag_mask == 0) {
		tag = 0;
		qc = ata_qc_from_tag(ap, tag);

		/* DEV interrupt w/ no active qc? */
		if (unlikely(!qc || (qc->tf.flags & ATA_TFLAG_POLLING))) {
			dev_err(ap->dev, "%s interrupt with no active qc=%p\n",
				__func__, qc);
			ata_check_status(ap);
			handled = 1;
			goto DONE;
		}

		status = ata_chk_status(ap);

		qc->ap->active_tag = tag;
		hsdevp->cmd_issued[tag] = SATA_DWC_CMD_ISSUED_NOT;

		if (status & ATA_ERR) {
			dev_dbg(ap->dev, "interrupt ATA_ERR (0x%x)\n", status);
			sata_dwc_qc_complete(ap, qc, 1);
			handled = 1;
			goto DONE;
		}

		dev_dbg(ap->dev, "%s non-NCQ cmd interrupt, protocol: %s\n",
			__func__, prot_2_txt(qc->tf.protocol));
DRVSTILLBUSY:
		if (sata_dwc_ata_is_dma(qc->tf.protocol)) {
			/* Each DMA transaction produces 2 interrupts.  The DMAC
			 * transfer complete interrupt and the SATA controller
			 * operation done interrupt. The command should be
			 * completed only after both interrupts are seen.
			 */
			dma_interrupt_count++;
			if (hsdevp->dma_pending[tag] == SATA_DWC_DMA_PENDING_NONE) {
				dev_err(ap->dev, "%s: DMA not pending"
						 " intpr=0x%08x status=0x%08x"
						 " pending=%d\n",
					__func__, intpr, status,
					hsdevp->dma_pending[tag]);
			}

			if ((dma_interrupt_count % 2) == 0)
				sata_dwc_dma_xfer_complete(ap, 1);
		} else {
			if (unlikely(sata_dwc_qc_complete(ap, qc, 1)))
				goto DRVSTILLBUSY;
		}

		handled = 1;
		goto DONE;
	}

	/*
	 * This is a NCQ command.  At this point we need to figure out for which
	 * tags we have gotten a completion interrupt.  One interrupt may serve
	 * as completion for more than one operation when commands are queued
	 * (NCQ).  We need to process each completed command.
	 */

PROCESS:  /* process completed commands */
	sactive = core_scr_read(SCR_ACTIVE);
	tag_mask = (sata_dwc_sactive_issued | sactive) ^ sactive;

	if (sactive != 0 || sata_dwc_sactive_issued > 1 || tag_mask > 1) {
		dev_dbg(ap->dev, "%s NCQ: sactive=0x%08x sactive_issued=0x%08x"
				 " tag_mask=0x%08x\n",
			__func__, sactive, sata_dwc_sactive_issued, tag_mask);
	}

	if ((tag_mask | sata_dwc_sactive_issued) != sata_dwc_sactive_issued) {
		dev_warn(ap->dev, "Bad tag mask?  sactive=0x%08x "
			 "sata_dwc_sactive_issued=0x%08x  tag_mask=0x%08x\n",
			 sactive, sata_dwc_sactive_issued, tag_mask);
	}

	/* read just to clear ... not bad if currently still busy */
	status = ata_chk_status(ap);
	dev_dbg(ap->dev, "%s ATA status register=0x%x\n", __func__, status);

	tag = 0;
	num_processed = 0;
	while (tag_mask) {
		num_processed++;
		while (!(tag_mask & 0x00000001)) {
			tag++;
			tag_mask <<= 1;
		}
		tag_mask &= (~0x00000001);
		qc = ata_qc_from_tag(ap, tag);

		/* To be picked up by completion functions */
		qc->ap->active_tag = tag;
		hsdevp->cmd_issued[tag] = SATA_DWC_CMD_ISSUED_NOT;

		/* Let libata/scsi layers handle error */
		if (status & ATA_ERR) {
			dev_dbg(ap->dev, "%s ATA_ERR (0x%x)\n",
				__func__, status);
			sata_dwc_qc_complete(ap, qc, 1);
			handled = 1;
			goto DONE;
		}

		/* Process completed command */
		dev_dbg(ap->dev, "%s NCQ command, protocol: %s\n", __func__,
			prot_2_txt(qc->tf.protocol));
		if (sata_dwc_ata_is_dma(qc->tf.protocol)) {
			dma_interrupt_count++;
			if (hsdevp->dma_pending[tag] == SATA_DWC_DMA_PENDING_NONE)
				dev_warn(ap->dev, "%s: DMA not pending?\n",
					 __func__);
			if ((dma_interrupt_count % 2) == 0)
				sata_dwc_dma_xfer_complete(ap, 1);
		} else {
			if (unlikely(sata_dwc_qc_complete(ap, qc, 1)))
				goto STILLBUSY;
		}
		continue;

STILLBUSY:
		ap->stats.idle_irq++;
		dev_warn(ap->dev, "STILL BUSY IRQ ata%d: irq trap\n", ap->id);
	} /* while tag_mask */

	/*
	 * Check to see if any commands completed while we were processing
	 * our initial set of completed commands (reading of status clears
	 * interrupts, so we might miss a completed command interrupt if
	 * one came in whil we were processing -- we read status as part of
	 * processing a completed command).
	 */
	sactive2 = core_scr_read(SCR_ACTIVE);
	if (sactive2 != sactive) {
		dev_dbg(ap->dev, "More completed - sactive=0x%x"
				 " sactive2=0x%x\n",
			sactive, sactive2);
		goto PROCESS;
	}
	handled = 1;

DONE:
	spin_unlock_irqrestore(&host->lock, flags);
	return IRQ_RETVAL(handled);
}

static void sata_dwc_irq_clear(struct ata_port *ap)
{
	/* unused */
}

static void sata_dwc_clear_dmacr(struct sata_dwc_device_port *hsdevp, u8 tag)
{
	struct sata_dwc_device *hsdev = HSDEV_FROM_HSDEVP(hsdevp);

	if (hsdevp->dma_pending[tag] == SATA_DWC_DMA_PENDING_RX) {
		out_le32(&(hsdev->sata_dwc_regs->dmacr),
			 SATA_DWC_DMACR_RX_CLEAR(
				 in_le32(&(hsdev->sata_dwc_regs->dmacr))));
	} else if (hsdevp->dma_pending[tag] == SATA_DWC_DMA_PENDING_TX) {
		out_le32(&(hsdev->sata_dwc_regs->dmacr),
			 SATA_DWC_DMACR_TX_CLEAR(
				 in_le32(&(hsdev->sata_dwc_regs->dmacr))));
	} else {
		/* This should not happen, it indicates the driver is out of
		 * sync.  If it does happen, clear dmacr anyway.
		 */
		dev_err(dwc_dev, "%s DMA protocol RX and TX DMA not pending"
				 " tag=0x%02x pending=%d dmacr: 0x%08x\n",
			__func__, tag, hsdevp->dma_pending[tag],
			in_le32(&(hsdev->sata_dwc_regs->dmacr)));
		out_le32(&(hsdev->sata_dwc_regs->dmacr),
				SATA_DWC_DMACR_TXRXCH_CLEAR);
	}
}

static void sata_dwc_dma_xfer_complete(struct ata_port *ap, u32 check_status)
{
	struct ata_queued_cmd *qc;
	struct sata_dwc_device_port *hsdevp = HSDEVP_FROM_AP(ap);
	struct sata_dwc_device *hsdev = HSDEV_FROM_AP(ap);
	u8 tag = 0;

	tag = ap->active_tag;
	qc = ata_qc_from_tag(ap, tag);

#ifdef DEBUG_NCQ
	if (tag > 0) {
		dev_info(ap->dev, "%s tag=%u cmd=0x%02x dma dir=%s proto=%s "
			 "dmacr=0x%08x\n", __func__, qc->tag, qc->tf.command,
			 dir_2_txt(qc->dma_dir), prot_2_txt(qc->tf.protocol),
			 in_le32(&(hsdev->sata_dwc_regs->dmacr)));
	}
#endif

	if (sata_dwc_ata_is_dma(qc->tf.protocol)) {
		if (hsdevp->dma_pending[tag] == SATA_DWC_DMA_PENDING_NONE) {
			dev_err(ap->dev, "%s DMA protocol RX and TX DMA not "
				"pending dmacr: 0x%08x\n", __func__,
				in_le32(&(hsdev->sata_dwc_regs->dmacr)));
		}

		hsdevp->dma_pending[tag] = SATA_DWC_DMA_PENDING_NONE;
		sata_dwc_qc_complete(ap, qc, check_status);
		ap->active_tag = ATA_TAG_POISON;
	} else {
		sata_dwc_qc_complete(ap, qc, check_status);
	}
}

static int sata_dwc_qc_complete(struct ata_port *ap, struct ata_queued_cmd *qc,
				u32 check_status)
{
	u8 status = 0;
	int i = 0;
	u32 mask = 0x0;
	u8 tag = qc->tag;
	struct sata_dwc_device_port *hsdevp = HSDEVP_FROM_AP(ap);
	u32 serror;

	dev_dbg(ap->dev, "%s checkstatus? %x\n", __func__, check_status);

	if (hsdevp->dma_pending[tag] == SATA_DWC_DMA_PENDING_TX)
		dev_err(ap->dev, "TX DMA PENDING\n");
	else if (hsdevp->dma_pending[tag] == SATA_DWC_DMA_PENDING_RX)
		dev_err(ap->dev, "RX DMA PENDING\n");

	if (check_status) {
		i = 0;
		do {
			/* check main status, clearing INTRQ */
			status = ata_chk_status(ap);
			if (status & ATA_BUSY) {
				dev_dbg(ap->dev, "STATUS BUSY (0x%02x) [%d]\n",
						status, i);
			}
			if (++i > 10)
				break;
		} while (status & ATA_BUSY);

		status = ata_chk_status(ap);
		if (unlikely(status & ATA_BUSY))
			dev_err(ap->dev, "QC complete cmd=0x%02x"
					 " STATUS BUSY (0x%02x) [%d]\n",
				qc->tf.command, status, i);
		serror = sata_dwc_scr_read(ap, SCR_ERROR);
		if (serror & SATA_DWC_SERROR_ERR_BITS)
			dev_err(ap->dev, "****** SERROR=0x%08x ******\n",
				serror);
	}
	dev_dbg(ap->dev, "QC complete cmd=0x%02x status=0x%02x"
			 " ata%u: protocol=%d\n",
		qc->tf.command, status, ap->id, qc->tf.protocol);

	/* clear active bit */
	mask = (~(qcmd_tag_to_mask(tag)));
	sata_dwc_sactive_queued = sata_dwc_sactive_queued & mask;
	sata_dwc_sactive_issued = sata_dwc_sactive_issued & mask;

	/* Complete taskfile transaction (does not read SCR registers) */
	ata_qc_complete(qc);

	return 0;
}

static void sata_dwc_enable_interrupts(struct sata_dwc_device *hsdev)
{
	/* Enable selective interrupts by setting the interrupt mask register */
	out_le32(&hsdev->sata_dwc_regs->intmr,
		 SATA_DWC_INTMR_ERRM |
		 SATA_DWC_INTMR_NEWFPM |
		 SATA_DWC_INTMR_PMABRTM |
		 SATA_DWC_INTMR_DMATM);

	/* Unmask the error bits that should trigger an error interrupt by
	 * setting the error mask register.
	 */
	out_le32(&hsdev->sata_dwc_regs->errmr, SATA_DWC_SERROR_ERR_BITS);

	dev_dbg(dwc_dev, "%s: INTMR = 0x%08x, ERRMR = 0x%08x\n", __func__,
		in_le32(&hsdev->sata_dwc_regs->intmr),
		in_le32(&hsdev->sata_dwc_regs->errmr));
}

static void sata_dwc_setup_port(struct ata_ioports *port, unsigned long base)
{
	port->cmd_addr = base + 0x00;
	port->data_addr = base + 0x00;

	port->error_addr = base + 0x04;
	port->feature_addr = base + 0x04;

	port->nsect_addr = base + 0x08;

	port->lbal_addr = base + 0x0c;
	port->lbam_addr = base + 0x10;
	port->lbah_addr = base + 0x14;

	port->device_addr = base + 0x18;
	port->command_addr = base + 0x1c;
	port->status_addr = base + 0x1c;

	port->altstatus_addr = base + 0x20;
	port->ctl_addr = base + 0x20;

	port->scr_addr = base + SATA_DWC_SCR_OFFSET;
}

/*******************************************************************************
 * Function : sata_dwc_port_start
 * arguments : struct ata_ioports *port
 * Return value : returns 0 if success, error code otherwise
 * This function allocates the scatter gather LLI table for AHB DMA
 ******************************************************************************/
static int sata_dwc_port_start(struct ata_port *ap)
{
	int err = 0;
	struct sata_dwc_device *hsdev;
	struct sata_dwc_device_port *hsdevp = NULL;
	struct device *pdev;
	u32 sstatus;
	int i;

	hsdev = HSDEV_FROM_AP(ap);

	dev_dbg(ap->dev, "%s: port_no=%d\n", __func__, ap->port_no);

	hsdev->host = ap->host_set;
	pdev = ap->host_set->dev;
	if (!pdev) {
		dev_err(ap->dev, "%s: no ap->host->dev\n", __func__);
		err = -ENODEV;
		goto CLEANUP;
	}

	/* Allocate Port Struct */
	hsdevp = kzalloc(sizeof(*hsdevp), GFP_KERNEL);
	if (!hsdevp) {
		dev_err(ap->dev, "%s: kmalloc failed for hsdevp\n", __func__);
		err = -ENOMEM;
		goto CLEANUP;
	}

	hsdevp->hsdev = hsdev;

	for (i = 0; i < SATA_DWC_QCMD_MAX; i++)
		hsdevp->cmd_issued[i] = SATA_DWC_CMD_ISSUED_NOT;

	ap->prd = 0;	/* set these so libata doesn't use them */
	ap->prd_dma = 0;

	/*
	 * DMA - Assign scatter gather LLI table. We can't use the libata
	 * version since it's PRD is IDE PCI specific.
	 */
	for (i = 0; i < SATA_DWC_QCMD_MAX; i++) {
		hsdevp->llit[i] = dma_alloc_coherent(pdev,
						     SATA_DWC_DMAC_LLI_TBL_SZ,
						     &(hsdevp->llit_dma[i]),
						     GFP_ATOMIC);
		if (!hsdevp->llit[i]) {
			dev_err(ap->dev, "%s: dma_alloc_coherent failed\n",
				__func__);
			err = -ENOMEM;
			goto CLEANUP;
		}
	}

	if (ap->port_no == 0)  {
		dev_dbg(ap->dev, "%s: clearing TXCHEN, RXCHEN in DMAC\n",
			__func__);
		out_le32(&hsdev->sata_dwc_regs->dmacr,
			 SATA_DWC_DMACR_TXRXCH_CLEAR);

		dev_dbg(ap->dev, "%s: setting burst size in DBTSR\n",
			__func__);
		out_le32(&hsdev->sata_dwc_regs->dbtsr,
			 (SATA_DWC_DBTSR_MWR(AHB_DMA_BRST_DFLT) |
			  SATA_DWC_DBTSR_MRD(AHB_DMA_BRST_DFLT)));
	}

	/* Clear any error bits before libata starts issuing commands */
	clear_serror();

	ap->private_data = hsdevp;

	/* Are we in Gen I or II */
	sstatus = sata_dwc_scr_read(ap, SCR_STATUS);
	switch (SATA_DWC_SCR0_SPD_GET(sstatus)) {
	case 0x0:
		dev_info(ap->dev,
			 "**** No negotiated speed (nothing attached?) ****\n");
		break;
	case 0x1:
		dev_info(ap->dev, "**** GEN I speed rate negotiated ****\n");
		break;
	case 0x2:
		dev_info(ap->dev, "**** GEN II speed rate negotiated ****\n");
		break;
	}

CLEANUP:
	if (err) {
		if (hsdevp)
			kfree(hsdevp);
		sata_dwc_port_stop(ap);
		dev_dbg(ap->dev, "%s: fail\n", __func__);
	} else {
		dev_dbg(ap->dev, "%s: done\n", __func__);
	}

	return err;
}

static void sata_dwc_port_stop(struct ata_port *ap)
{
	int i;
	struct sata_dwc_device *hsdev = HSDEV_FROM_AP(ap);
	struct sata_dwc_device_port *hsdevp = HSDEVP_FROM_AP(ap);

	dev_dbg(ap->dev, "%s: ap->id = %d\n", __func__, ap->id);

	if (hsdevp && hsdev) {
		/* deallocate LLI table */
		for (i = 0; i < SATA_DWC_QCMD_MAX; i++) {
			dma_free_coherent(ap->host_set->dev,
					  SATA_DWC_DMAC_LLI_TBL_SZ,
					  hsdevp->llit[i], hsdevp->llit_dma[i]);
		}

		kfree(hsdevp);
	}
	ap->private_data = NULL;
}

/*******************************************************************************
 * Function : sata_dwc_exec_command_by_tag
 * arguments : ata_port *ap, ata_taskfile *tf, u8 tag, u32 cmd_issued
 * Return value : None
 * This function keeps track of individual command tag ids and calls
 * ata_exec_command in libata
 ******************************************************************************/
static void sata_dwc_exec_command_by_tag(struct ata_port *ap,
					 struct ata_taskfile *tf,
					 u8 tag, u32 cmd_issued)
{
	struct sata_dwc_device_port *hsdevp = HSDEVP_FROM_AP(ap);

	dev_dbg(ap->dev, "%s cmd(0x%02x): %s tag=%d\n", __func__, tf->command,
		ata_cmd_2_txt(tf), tag);

	hsdevp->cmd_issued[tag] = cmd_issued;

	/* Clear SError before executing a new command.
	 *
	 * TODO if we read a PM's registers now, we will throw away the task
	 * file values loaded into the shadow registers for this command.
	 *
	 * sata_dwc_scr_write and read can not be used here. Clearing the PM
	 * managed SError register for the disk needs to be done before the
	 * task file is loaded.
	 */
	clear_serror();
	ata_exec_command(ap, tf);
}

static void sata_dwc_bmdma_setup_by_tag(struct ata_queued_cmd *qc, u8 tag)
{
	sata_dwc_exec_command_by_tag(qc->ap, &qc->tf, tag,
				     SATA_DWC_CMD_ISSUED_PEND);
}

static void sata_dwc_bmdma_setup(struct ata_queued_cmd *qc)
{
	u8 tag = qc->tag;

	if (sata_dwc_ata_is_ncq(qc->tf.protocol)) {
		dev_dbg(qc->ap->dev, "%s: ap->sactive=0x%08x tag=%d\n",
			__func__, qc->ap->sactive, tag);
	} else {
		tag = 0;
	}

	sata_dwc_bmdma_setup_by_tag(qc, tag);
}

static void sata_dwc_bmdma_start_by_tag(struct ata_queued_cmd *qc, u8 tag)
{
	int start_dma;
	u32 reg, dma_chan;
	struct sata_dwc_device *hsdev = HSDEV_FROM_QC(qc);
	struct ata_port *ap = qc->ap;
	struct sata_dwc_device_port *hsdevp = HSDEVP_FROM_AP(ap);
	int dir = qc->dma_dir;
	dma_chan = hsdevp->dma_chan[tag];

	/* Used for ata_bmdma_start(qc) -- we are not BMDMA compatible */

	if (hsdevp->cmd_issued[tag] != SATA_DWC_CMD_ISSUED_NOT) {
		start_dma = 1;
		if (dir == DMA_TO_DEVICE)
			hsdevp->dma_pending[tag] = SATA_DWC_DMA_PENDING_TX;
		else
			hsdevp->dma_pending[tag] = SATA_DWC_DMA_PENDING_RX;
	} else {
		dev_err(ap->dev, "%s: Command not pending cmd_issued=%d"
				 " (tag=%d) - DMA NOT started\n",
			__func__, hsdevp->cmd_issued[tag], tag);
		start_dma = 0;
	}

	dev_dbg(ap->dev, "%s qc=%p tag: %x cmd: 0x%02x"
			 " dma_dir: %s start_dma? %x\n",
		__func__, qc, tag, qc->tf.command,
		dir_2_txt(qc->dma_dir), start_dma);
	sata_dwc_tf_dump(&(qc->tf));

	if (start_dma) {
		reg = sata_dwc_scr_read(ap, SCR_ERROR);
		if (reg & SATA_DWC_SERROR_ERR_BITS) {
			dev_err(ap->dev, "%s: ****** SError=0x%08x ******\n",
				__func__, reg);
		}

		if (dir == DMA_TO_DEVICE)
			out_le32(&hsdev->sata_dwc_regs->dmacr,
					SATA_DWC_DMACR_TXCHEN);
		else
			out_le32(&hsdev->sata_dwc_regs->dmacr,
					SATA_DWC_DMACR_RXCHEN);

		/* Enable AHB DMA transfer on the specified channel */
		dma_dwc_xfer_start(dma_chan);
	}
}

static void sata_dwc_bmdma_start(struct ata_queued_cmd *qc)
{
	u8 tag = qc->tag;

	if (sata_dwc_ata_is_ncq(qc->tf.protocol)) {
		dev_dbg(qc->ap->dev, "%s: ap->sactive=0x%08x tag=%d\n",
			__func__, qc->ap->sactive, tag);
	} else {
		tag = 0;
	}

	dev_dbg(qc->ap->dev, "%s\n", __func__);
	sata_dwc_bmdma_start_by_tag(qc, tag);
}

/*******************************************************************************
 * Function : sata_dwc_qc_prep_by_tag
 * arguments : ata_queued_cmd *qc, u8 tag
 * Return value : None
 * qc_prep for a particular queued command based on tag
 ******************************************************************************/
static void sata_dwc_qc_prep_by_tag(struct ata_queued_cmd *qc, u8 tag)
{
	struct ata_port *ap = qc->ap;
	u32 dma_chan;
	struct sata_dwc_device *hsdev = HSDEV_FROM_AP(ap);
	struct sata_dwc_device_port *hsdevp = HSDEVP_FROM_AP(ap);
	int dir;
	int err;

	dir = qc->dma_dir;

	if ((dir == DMA_NONE) || (qc->tf.protocol == ATA_PROT_PIO))
		return;

	dev_dbg(ap->dev, "%s: port=%d dma dir=%s n_elem=%d\n",
		__func__, ap->port_no, dir_2_txt(dir), qc->n_elem);

	dma_chan = dma_dwc_xfer_setup(qc, hsdevp->llit[tag],
				      hsdevp->llit_dma[tag],
				      &hsdev->sata_dwc_regs->dmadr,
				      dir);
	if (dma_chan < 0) {
		dev_err(ap->dev, "%s: dma_dwc_xfer_setup returns err %d\n",
			__func__, err);
		return;
	}

	hsdevp->dma_chan[tag] = dma_chan;
}

static unsigned int sata_dwc_qc_issue(struct ata_queued_cmd *qc)
{
	struct ata_port *ap = qc->ap;

#ifdef DEBUG_NCQ
	if (qc->tag > 0 || ap->sactive > 1) {
		dev_info(ap->dev, "%s ap id=%d cmd(0x%02x)=%s qc tag=%d prot=%s"
				  " ap active_tag=0x%08x ap sactive=0x%08x\n",
			 __func__, ap->id, qc->tf.command,
			 ata_cmd_2_txt(&qc->tf), qc->tag,
			 prot_2_txt(qc->tf.protocol),
			 ap->active_tag, ap->sactive);
	}
#endif

	if (qc->tf.protocol == ATA_PROT_NCQ) {
		ap->ops->tf_load(ap, &qc->tf);
		sata_dwc_exec_command_by_tag(ap, &qc->tf, qc->tag,
					     SATA_DWC_CMD_ISSUED_PEND);
	} else {
		ata_qc_issue_prot(qc);
	}

	return 0;
}

/*******************************************************************************
 * Function : sata_dwc_eng_timeout
 * arguments : ata_port *ap
 * Return value : None
 * error handler for DMA time out
 * ata_eng_timeout(ap) -- this does bmdma stuff which can not be done by this
 * driver. SEE ALSO ata_qc_timeout(ap)
 ******************************************************************************/
static void sata_dwc_eng_timeout(struct ata_port *ap)
{
	struct ata_queued_cmd *qc;
	struct sata_dwc_device *hsdev = HSDEV_FROM_AP(ap);
	struct sata_dwc_device_port *hsdevp = HSDEVP_FROM_AP(ap);
	u8  tag;
	uint mask = 0x0;
	unsigned long flags;
	u32 serror, intpr, dma_ch;

	tag = ap->active_tag;
	dma_ch = hsdevp->dma_chan[tag];
	qc = ata_qc_from_tag(ap, tag);

	dev_err(ap->dev, "%s: id=%d active_tag=%d qc=%p dma_chan=%d\n",
		__func__, ap->id, tag, qc, dma_ch);

	if (!qc) {
		dev_err(ap->dev, "%s: timeout without queued command\n",
			__func__);
		return;
	}

	intpr = in_le32(&hsdev->sata_dwc_regs->intpr);
	serror = core_scr_read(SCR_ERROR);

	dev_err(ap->dev, "intpr=0x%08x serror=0x%08x\n", intpr, serror);

	/* If there are no error bits set, can we just pass this on to eh? */
	if (!(serror & SATA_DWC_SERROR_ERR_BITS) &&
	    !(intpr & SATA_DWC_INTPR_ERR)) {

		spin_lock_irqsave(&ap->host_set->lock, flags);
		if (dma_dwc_channel_enabled(dma_ch))
			dma_dwc_terminate_dma(dma_ch);

		hsdevp->dma_pending[tag] = SATA_DWC_DMA_PENDING_NONE;

		/* clear active bit */
		mask = (~(qcmd_tag_to_mask(tag)));
		sata_dwc_sactive_queued = sata_dwc_sactive_queued & mask;
		sata_dwc_sactive_issued = sata_dwc_sactive_issued & mask;

		spin_unlock_irqrestore(&ap->host_set->lock, flags);
	} else {
		/* This is wrong, what really needs to be done is a reset. */

		spin_lock_irqsave(ap->lock, flags);

		if (sata_dwc_ata_is_dma(qc->tf.protocol)) {
			/* disable DMAC */
			dma_dwc_terminate_dma(dma_ch);
		}

		spin_unlock_irqrestore(ap->lock, flags);
	}

	WARN_ON(!(qc->flags & ATA_QCFLAG_ACTIVE));
	if (qc->flags & ATA_QCFLAG_ACTIVE) {
		qc->err_mask |= AC_ERR_TIMEOUT;
		/*
		 * test-only: The original code (AMCC: 2.6.19) called
		 * ata_eng_timeout(ap) here. This function is not available
		 * anymore. So what to do now?
		 */
	}
}

/*******************************************************************************
 * Function : sata_dwc_qc_prep
 * arguments : ata_queued_cmd *qc
 * Return value : None
 * qc_prep for a particular queued command
 ******************************************************************************/
static void sata_dwc_qc_prep(struct ata_queued_cmd *qc)
{
	u32 sactive;
	u8 tag = qc->tag;

#ifdef DEBUG_NCQ
	if (qc->tag > 0) {
		dev_info(qc->ap->dev, "%s: qc->tag=%d ap->active_tag=0x%08x\n",
			 __func__, tag, qc->ap->active_tag);
	}
#endif

	if (qc->tf.protocol == ATA_PROT_NCQ) {
		sactive = core_scr_read(SCR_ACTIVE);
		sactive |= (0x00000001 << tag);
		core_scr_write(SCR_ACTIVE, sactive);
		dev_dbg(qc->ap->dev, "%s: tag=%d ap->sactive = 0x%08x"
				     " sactive=0x%08x\n",
			__func__, tag, qc->ap->sactive, sactive);
	} else {
		tag = 0;
	}

	sata_dwc_qc_prep_by_tag(qc, tag);
}

/*
 * test-only: Needed when no drive is connected upon driver startup.
 * Otherwise, by using the default prereset routine, the driver crashes
 * upon loading.
 * Not sure if this could be handled differently.
 */
static int sata_dwc_prereset(struct ata_port *ap)
{
	struct ata_eh_context *ehc = &ap->eh_context;

	/* if we're about to do hardreset, nothing more to do */
	if (ehc->i.action & ATA_EH_HARDRESET)
		return 0;

	if (ata_port_online(ap))
		if (ata_busy_sleep(ap, ATA_TMOUT_BOOT_QUICK, ATA_TMOUT_BOOT)) {
			ata_port_printk(ap, KERN_WARNING,
					"device busy after hardreset\n");
			return -EIO;
		} else
			return 0;
	else
		return -ENODEV;
}

static void sata_dwc_error_handler(struct ata_port *ap)
{
	/* perform recovery */
	ata_do_eh(ap, sata_dwc_prereset, ata_std_softreset, sata_std_hardreset,
		  ata_std_postreset);
}

/*
 * scsi mid-layer and libata interface structures
 */
static struct scsi_host_template sata_dwc_sht = {
	.module 		= THIS_MODULE,
	.name			= DRV_NAME,
	.ioctl			= ata_scsi_ioctl,
	.queuecommand		= ata_scsi_queuecmd,
	/*
	 * test-only: Currently this driver doesn't handle NCQ
	 * correctly. We enable NCQ but set the queue depth to a
	 * max of 1. This will get fixed in in a future release.
	 */
	.can_queue		= ATA_DEF_QUEUE,	/* ATA_MAX_QUEUE */
	.this_id		= ATA_SHT_THIS_ID,
	.sg_tablesize		= LIBATA_MAX_PRD,
	.max_sectors		= ATA_MAX_SECTORS,
	.cmd_per_lun		= ATA_SHT_CMD_PER_LUN,
	.emulated		= ATA_SHT_EMULATED,
	.use_clustering		= ATA_SHT_USE_CLUSTERING,
	.proc_name		= DRV_NAME,
	.dma_boundary		= ATA_DMA_BOUNDARY,
	.slave_configure	= ata_scsi_slave_config,
	.slave_destroy		= ata_scsi_slave_destroy,
	.bios_param		= ata_std_bios_param,
};

static const struct ata_port_operations sata_dwc_ops = {
	.port_disable		= ata_port_disable,
	.check_status		= ata_check_status,
	.dev_select		= ata_std_dev_select,

	.error_handler		= sata_dwc_error_handler,

	.tf_read		= ata_tf_read,
	.tf_load		= ata_tf_load,

	.exec_command		= ata_exec_command,
	.data_xfer		= ata_mmio_data_xfer,

	.qc_prep		= sata_dwc_qc_prep,
	.qc_issue		= sata_dwc_qc_issue,
	.eng_timeout            = sata_dwc_eng_timeout,

	.bmdma_setup		= sata_dwc_bmdma_setup,
	.bmdma_start		= sata_dwc_bmdma_start,
	.bmdma_stop		= ata_bmdma_stop,
	.bmdma_status		= ata_bmdma_status,

	.irq_handler		= sata_dwc_isr,
	.irq_clear		= sata_dwc_irq_clear,

	.scr_read		= sata_dwc_scr_read,
	.scr_write		= sata_dwc_scr_write,

	.port_start		= sata_dwc_port_start,
	.port_stop		= sata_dwc_port_stop,
};

static const struct ata_port_info sata_dwc_port_info[] = {
	{
		.host_flags	= ATA_FLAG_SATA | ATA_FLAG_NO_LEGACY |
		ATA_FLAG_MMIO | ATA_FLAG_PIO_POLLING |
		ATA_FLAG_SRST | ATA_FLAG_NCQ | ATA_FLAG_SETXFER_POLLING,
		.pio_mask	= 0x1f,	/* pio 0-4 */
		.mwdma_mask	= 0x07,
		.udma_mask	= 0x7f,
		.port_ops	= &sata_dwc_ops,
	},
};


static int sata_dwc_probe(struct platform_device *pdev)
{
	struct sata_dwc_device *hsdev = NULL;
	struct ata_probe_ent *probe_ent = NULL;
	u32 idr, versionr;
	char *ver = (char *)&versionr;
	void __iomem *base = NULL;
	int err = 0;
	struct resource *res;
	int irq;

	/* Allocate DWC SATA device */
	hsdev = kzalloc(sizeof(*hsdev), GFP_KERNEL);
	if (hsdev == NULL) {
		dev_err(&pdev->dev, "kmalloc failed for hsdev\n");
		err = -ENOMEM;
		goto error_out;
	}

	/* Allocate and fill host */
	probe_ent = kzalloc(sizeof(*probe_ent), GFP_KERNEL);
	if (probe_ent == NULL) {
		dev_err(&pdev->dev, "kmalloc failed for probe_ent\n");
		err = -ENOMEM;
		goto error_out;
	}

	/* Get physical SATA register base address */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "no reg addr\n");
		err = -ENODEV;
		goto error_out;
	}

	/* Ioremap SATA registers */
	base = ioremap(res->start, res->end - res->start + 1);
	if (!base) {
		dev_err(&pdev->dev, "ioremap failed for 0x%llx\n", res->start);
		err = -ENODEV;
		goto error_out;
	}
	hsdev->reg_base = base;
	dev_dbg(&pdev->dev, "ioremap done (0x%llx -> 0x%08x)\n",
		res->start, (u32)base);

	/* Synopsys DWC SATA specific Registers */
	hsdev->sata_dwc_regs = (void *__iomem)(base + SATA_DWC_REG_OFFSET);

	probe_ent->dev = &pdev->dev;
	INIT_LIST_HEAD(&probe_ent->node);

	/* Setup port */
	scr_addr_sstatus = base + SATA_DWC_SCR_OFFSET;
	sata_dwc_setup_port(&probe_ent->port[0], (unsigned long)base);

	/* Read the ID and Version Registers */
	idr = in_le32(&hsdev->sata_dwc_regs->idr);
	versionr = in_le32(&hsdev->sata_dwc_regs->versionr);
	dev_notice(&pdev->dev, "id %d, controller version %c.%c%c\n",
		   idr, ver[0], ver[1], ver[2]);

	/* Get SATA DMA interrupt number */
	res = platform_get_resource(pdev, IORESOURCE_IRQ, 1);
	if (!res) {
		dev_err(&pdev->dev, "no SATA DMA irq\n");
		err = -ENODEV;
		goto error_out;
	}
	irq = res->start;

	/* Get physical SATA DMA register base address */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	if (!res) {
		dev_err(&pdev->dev, "no reg addr\n");
		err = -ENODEV;
		goto error_out;
	}

	/* Save dev for later use in dev_xxx() routines */
	dwc_dev = &pdev->dev;

	/* Initialize AHB DMAC */
	dma_dwc_init(hsdev, res, irq);

	/* Enable SATA Interrupts */
	sata_dwc_enable_interrupts(hsdev);

	/* Get SATA interrupt number */
	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res) {
		dev_err(&pdev->dev, "no SATA irq\n");
		err = -ENODEV;
		goto error_out;
	}
	irq = res->start;

	/* initialize probe_ent */
	probe_ent->sht		= &sata_dwc_sht;
	probe_ent->host_flags	= sata_dwc_port_info[0].host_flags;
	probe_ent->pio_mask	= sata_dwc_port_info[0].pio_mask;
	probe_ent->udma_mask	= sata_dwc_port_info[0].udma_mask;
	probe_ent->port_ops	= sata_dwc_port_info[0].port_ops;

	probe_ent->irq = irq;
	probe_ent->irq_flags = 0;
	probe_ent->mmio_base = base;
	probe_ent->private_data = hsdev;
	probe_ent->n_ports = 1;

	/*
	 * Now, register with libATA core, this will also initiate the
	 * device discovery process, invoking our port_start() handler &
	 * error_handler() to execute a dummy Softreset EH session
	 */
	err = ata_device_add(probe_ent);
	if (!err) {
		dev_err(&pdev->dev, "failed to add device\n");
		err = -ENODEV;
		goto error_out;
	}

	kfree(probe_ent);

	/* Everything is fine */
	return 0;

error_out:
	/* Free SATA DMA resources */
	dma_dwc_exit(hsdev);

	if (base)
		iounmap(base);

	if (probe_ent)
		kfree(probe_ent);

	if (hsdev)
		kfree(hsdev);

	return err;
}


static int sata_dwc_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct ata_host_set *host = dev_get_drvdata(dev);
	struct ata_host_set *host2 = host->next;
	struct sata_dwc_device *hsdev = host->private_data;

	ata_host_set_remove(host);
	if (host2)
		ata_host_set_remove(host2);

	dev_set_drvdata(dev, NULL);

	/* Free SATA DMA resources */
	dma_dwc_exit(hsdev);

	iounmap(hsdev->reg_base);
	kfree(hsdev);

	dev_dbg(&pdev->dev, "done\n");

	return 0;
}

static struct platform_driver sata_dwc_driver = {
	.probe = sata_dwc_probe,
	.remove = sata_dwc_remove,
	.driver = {
		.name = DRV_NAME,
		.owner = THIS_MODULE,
	},
};

static int __init sata_dwc_init(void)
{
	int err;

	err = platform_driver_register(&sata_dwc_driver);

	return err;
}

static void __exit sata_dwc_exit(void)
{
	platform_driver_unregister(&sata_dwc_driver);
}

module_init(sata_dwc_init);
module_exit(sata_dwc_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mark Miesfeld <mmiesfeld@amcc.com>");
MODULE_DESCRIPTION("DesignWare Cores SATA controller low lever driver");
MODULE_VERSION(DRV_VERSION);
