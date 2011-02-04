/*
 * Intel CE4100 NAND controller driver
 *
 * Copyright (c) 2010 Google, Inc
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/jiffies.h>
#include <linux/pci.h>
#include <linux/wait.h>

#include <asm/io.h>

#define DRV_NAME        	"intel_ce_nand"
#define DRV_VERSION     	"1.2"
#define DRV_DESC        	"Intel CE4100 NAND controller driver"

MODULE_DESCRIPTION(DRV_DESC);
MODULE_VERSION(DRV_VERSION);
MODULE_AUTHOR("Eugene Surovegin <es@google.com>");
MODULE_LICENSE("GPL");

#define DBG_LEVEL 		0

#if DBG_LEVEL > 0
#  define DBG(f,x...)		printk(KERN_ERR DRV_NAME f, ##x)
#else
#  define DBG(f,x...)		((void)0)
#endif
#if DBG_LEVEL > 1
#  define DBG2(f,x...) 		DBG(f, ##x)
#else
#  define DBG2(f,x...) 		((void)0)
#endif
#if DBG_LEVEL > 2
#  define DBG3(f,x...) 		DBG(f, ##x)
#else
#  define DBG3(f,x...) 		((void)0)
#endif

#if defined(CONFIG_MTD_NAND_INTEL_CE_CEFDK)
   /* Basic sanity check */
#  if CONFIG_MTD_NAND_INTEL_CE_CEFDK_BLOCKS < 64
#    error "Invalid number of CEFDK-managed blocks"
#  endif

   /* Shorter name for convenience */
#  define CEFDK_BLOCKS		CONFIG_MTD_NAND_INTEL_CE_CEFDK_BLOCKS
#else
#  define CEFDK_BLOCKS		0
#endif

// TODO(surovegin): limit to one for now.
#define	MAX_BANKS		1

/* ECC metrics */
#define ECC_SECTOR_SIZE		512
#define ECC_BYTES_PER_SECTOR	14

struct nfc_regs {
	u32 reset;
	u32 __rsrv0[0x3];
	u32 transfer_spare;
	u32 __rsrv1[0x13];
	u32 rb_pin_en;
	u32 __rsrv2[0x1b];
	u32 ce_dont_care;
	u32 __rsrv3[0x3];
	u32 ecc_en;
	u32 __rsrv4[0x3];
	u32 global_irq_en;
	u32 __rsrv5[0x3];
	u32 we_2_re;
	u32 __rsrv6[0x3];
	u32 addr_2_data;
	u32 __rsrv7[0x3];
	u32 re_2_we;
	u32 __rsrv8[0x3];
	u32 acc_clks;
	u32 __rsrv9[0x7];
	u32 pages_per_block;
	u32 __rsrv10[0x7];
	u32 device_main_area_size;
	u32 __rsrv11[0x3];
	u32 device_spare_area_size;
	u32 __rsrv12[0xb];
	u32 ecc_correction;
	u32 __rsrv13[0xf];
	u32 rdwr_en_lo;
	u32 __rsrv14[0x3];
	u32 rdwr_en_hi;
	u32 __rsrv15[0x7];
	u32 cs_setup;
	u32 __rsrv16[0x3];
	u32 spare_area_skip_bytes;
	u32 __rsrv17[0x3];
	u32 spare_area_marker;
	u32 __rsrv18[0x13];
	u32 re_2_re;
	u32 __rsrv19[0x37];
	u32 revision;
	u32 __rsrv20[0xb];
	u32 onfi_timing_modes;
	u32 __rsrv21[0x13];
	u32 features;
	u32 __rsrv22[0x7];

	struct {
		u32 sts;
		u32 __rsrv23[0x3];
		u32 en;
		u32 __rsrv24[0xf];
	} irq[4];

	u32 __rsrv25[0x38];
	u32 ecc_error_address;
	u32 __rsrv26[0x3];
	u32 ecc_correction_info;
	u32 __rsrv27[0x2f];
	u32 dma_en;
};

/* Device reset register */
#define RST_BANK_MASK(bank)		(1 << (bank))

/* Interrupt status register */
#define IRQ_STS_ECC_DONE		(1 << 0)
#define IRQ_STS_ECC_FAILED		(1 << 1)
#define IRQ_STS_DMA_DONE		(1 << 2)
#define IRQ_STS_PROG_FAILED		(1 << 4)
#define IRQ_STS_ERASE_FAILED		(1 << 5)
#define IRQ_STS_LOADED			(1 << 6)
#define IRQ_STS_PROG_DONE		(1 << 7)
#define IRQ_STS_ERASE_DONE		(1 << 8)
#define IRQ_STS_RST_DONE		(1 << 13)

#define IRQ_STS_ALL			(IRQ_STS_ECC_DONE | \
					 IRQ_STS_ECC_FAILED | \
					 IRQ_STS_DMA_DONE | \
					 IRQ_STS_PROG_FAILED | \
					 IRQ_STS_ERASE_FAILED | \
					 IRQ_STS_LOADED | \
					 IRQ_STS_PROG_DONE | \
					 IRQ_STS_ERASE_DONE | \
					 IRQ_STS_RST_DONE)

/* ECC error address register */
#define ECC_ERR_ADDR_OFFSET(v)		((v) & 0xfff)
#define ECC_ERR_ADDR_SECTOR(v)		(((v) >> 12) & 0xf)

/* ECC error correction info register */
#define ECC_ERR_INFO_VALUE(v)		((v) & 0xff)
#define ECC_ERR_INFO_DEVICE(v)		(((v) >> 8) & 0xf)
#define ECC_ERR_INFO_UNREC		(1 << 14)
#define ECC_ERR_INFO_LAST		(1 << 15)

struct nfc_mem_regs {
	u32 cmd;
	u32 __rsrv0[0x3];
	u32 io;
	u32 __rsrv1[0xb];
	u32 data;
};

/* Command register */
#define CMD_COMMAND_CYCLE		0
#define CMD_ADDRESS_CYCLE		1
#define CMD_STATUS_CYCLE		2
#define CMD_BANK(cs)			((cs) << 24)
#define CMD_MODE_00			(0 << 26)
#define CMD_MODE_01			(1 << 26)
#define CMD_MODE_10			(2 << 26)
#define CMD_MODE_11			(3 << 26)

#define IO_BUF_SIZE			(NAND_MAX_PAGESIZE + NAND_MAX_OOBSIZE)

struct nfc_device {
	struct mtd_info mtd;
	struct nand_chip nand;
	struct pci_dev *pdev;

	struct nfc_regs __iomem *regs;
	struct nfc_mem_regs __iomem *mem_regs;

	spinlock_t lock;	/* protects 'irq_sts' and 'cs' */
	wait_queue_head_t wq;
	u32 irq_sts;		/* accumulated IRQ status since last
				 * clear_irq()
				 */

	int cs;			/* current selected bank */
	int page;		/* current I/O page */

	/* number of pages at the chip start managed by CEFDK */
	int cefdk_pages;
	int bb_scanning;	/* flag showing that we are scanning
				 * for bad blocks
				 */
	int status;		/* status of the last erase/write operation */

	/* I/O buffer */
	int buf_head;
	int buf_tail;
	u8 *buf;
	dma_addr_t buf_dma;
};

#define mtd_to_dev(p)		container_of(p, struct nfc_device, mtd)

/* "Emulation" I/O buffer helpers */
static inline void __reset_buf(struct nfc_device *dev)
{
	dev->buf_head = dev->buf_tail = 0;
}

static inline void __put_buf_byte(struct nfc_device *dev, u8 v)
{
	BUG_ON(dev->buf_tail >= IO_BUF_SIZE);
	dev->buf[dev->buf_tail++] = v;
}

static inline void nfc_writel(u32 val, volatile void __iomem *addr)
{
	DBG3(": 0x%03x <- 0x%08x\n", (((u32) addr) & 0x1fff) / 4, val);
	writel(val, addr);
}

static inline u32 nfc_readl(const volatile void __iomem *addr)
{
	return readl(addr);
}

static inline void nfc_writel_flush(u32 val, volatile void __iomem *addr)
{
	nfc_writel(val, addr);
	(void)nfc_readl(addr);
}

static irqreturn_t nfc_irq_handler(int irq, void *id)
{
	struct nfc_device *dev = id;
	struct nfc_regs __iomem *p = dev->regs;
	irqreturn_t res = IRQ_NONE;
	u32 sts;

	spin_lock(&dev->lock);
	if (dev->cs < 0)
		goto out;

	sts = nfc_readl(&p->irq[dev->cs].sts) & IRQ_STS_ALL;
	if (!sts)
		goto out;

	DBG2("%d: IRQ 0x%04x\n", dev->cs, sts);

	/* ACK */
	nfc_writel_flush(sts, &p->irq[dev->cs].sts);

	dev->irq_sts |= sts;
	wake_up(&dev->wq);
	res = IRQ_HANDLED;

     out:
	spin_unlock(&dev->lock);
	return res;
}

static u32 wait_irq(struct nfc_device *dev, u32 mask)
{
	long timeout = HZ;
	DEFINE_WAIT(wait);
	u32 res;

	spin_lock_irq(&dev->lock);
	while (!(dev->irq_sts & mask) && timeout) {
		prepare_to_wait(&dev->wq, &wait, TASK_UNINTERRUPTIBLE);
		spin_unlock_irq(&dev->lock);

		timeout = schedule_timeout(timeout);
		finish_wait(&dev->wq, &wait);

		spin_lock_irq(&dev->lock);
	}
	res = dev->irq_sts;
	spin_unlock_irq(&dev->lock);

	if (unlikely(!(res & mask)))
		printk(KERN_ERR DRV_NAME
		       "%d: timeout wating for IRQ 0x%04x\n", dev->cs, mask);
	return res;
}

/* Clear "accumulated" IRQs */
static inline void clear_irqs(struct nfc_device *dev)
{
	spin_lock_irq(&dev->lock);
	dev->irq_sts = 0;
	spin_unlock_irq(&dev->lock);
}

/* Status of the last erase and write command */
static int nfc_waitfunc(struct mtd_info *mtd, struct nand_chip *nand)
{
	struct nfc_device *dev = mtd_to_dev(mtd);
	int res = dev->status;
	dev->status = 0;
	DBG2("%d: nfc_waitfunc -> %d\n", dev->cs, res);
	return res;
}

static void nfc_select_chip(struct mtd_info *mtd, int chip)
{
	struct nfc_device *dev = mtd_to_dev(mtd);
	DBG2(": nfc_select_chip(%d)\n", chip);
	spin_lock_irq(&dev->lock);
	dev->cs = chip;
	spin_unlock_irq(&dev->lock);
}

static u8 nfc_read_byte(struct mtd_info *mtd)
{
	struct nfc_device *dev = mtd_to_dev(mtd);
	u8 res;

	if (likely(dev->buf_head < dev->buf_tail)) {
		res = dev->buf[dev->buf_head++];
	} else {
		printk(KERN_ERR DRV_NAME
		       "%d: nfc_read_byte - read beyond buffer end\n", dev->cs);
		res = 0xff;
	}

	DBG2("%d: nfc_read_byte -> 0x%02x\n", dev->cs, res);
	return res;
}

static u16 nfc_read_word(struct mtd_info *mtd)
{
	/* we don't support 16-bit I/O */
	BUG();
	return 0xffff;
}

static int nfc_verify_buf(struct mtd_info *mtd, const u8 *buf, int len)
{
	DBG2("%d: nfc_verify_buf(%d)\n", mtd_to_dev(mtd)->cs, len);
	return 0;
}

/* Send "indirect" command/data */
static inline void nfc_write_indirect(struct nfc_mem_regs __iomem *pm,
				      u32 addr, u32 val)
{
	nfc_writel(addr, &pm->cmd);
	nfc_writel(val, &pm->io);
}

/* "RESET" 0xff command */
static void reset_bank(struct nfc_device *dev)
{
	DBG("%d: reset_chip\n", dev->cs);

	clear_irqs(dev);
	nfc_writel(RST_BANK_MASK(dev->cs), &dev->regs->reset);
	wait_irq(dev, IRQ_STS_RST_DONE);
}

/* "READ ID" 0x90 command */
static void read_id(struct nfc_device *dev)
{
	struct nfc_mem_regs __iomem *pm = dev->mem_regs;
	u32 mode;
	int i;

	__reset_buf(dev);

	mode = CMD_MODE_11 | CMD_BANK(dev->cs);
	nfc_write_indirect(pm, mode | CMD_COMMAND_CYCLE, 0x90);
	nfc_write_indirect(pm, mode | CMD_ADDRESS_CYCLE, 0x00);

	for (i = 0; i < 5; ++i) {
		nfc_writel(mode | CMD_STATUS_CYCLE, &pm->cmd);
		__put_buf_byte(dev, nfc_readl(&pm->data));
	}

	DBG("%d: read_id -> %02x %02x %02x %02x %02x\n", dev->cs,
	    dev->buf[0], dev->buf[1], dev->buf[2], dev->buf[3], dev->buf[4]);
}

/* "READ STATUS" 0x70 command */
static void read_status(struct nfc_device *dev)
{
	struct nfc_mem_regs __iomem *pm = dev->mem_regs;
	u32 mode;

	mode = CMD_MODE_11 | CMD_BANK(dev->cs);
	nfc_write_indirect(pm, mode | CMD_COMMAND_CYCLE, 0x70);
	nfc_writel(mode | CMD_STATUS_CYCLE, &pm->cmd);

	__reset_buf(dev);
	__put_buf_byte(dev, nfc_readl(&pm->data));

	DBG2("%d: read_status -> %02x\n", dev->cs, dev->buf[0]);
}

/* Erase a block */
static void nfc_erase(struct mtd_info *mtd, int page)
{
	struct nfc_device *dev = mtd_to_dev(mtd);
	struct nfc_mem_regs __iomem *pm = dev->mem_regs;
	u32 sts;

	DBG("%d: erase(%d)\n", dev->cs, page);

	clear_irqs(dev);
	nfc_write_indirect(pm, CMD_MODE_10 | CMD_BANK(dev->cs) | page, 0x01);

	sts = wait_irq(dev, IRQ_STS_ERASE_FAILED | IRQ_STS_ERASE_DONE);
	dev->status = (sts & IRQ_STS_ERASE_FAILED) ? NAND_STATUS_FAIL : 0;
}

/* Perform full page (main + spare) DMA read into dev->buf */
static u32 do_dma_read(struct nfc_device *dev, u32 irq_mask)
{
	struct nfc_mem_regs __iomem *pm = dev->mem_regs;
	size_t size = dev->mtd.writesize + dev->mtd.oobsize;
	dma_addr_t dma = dev->buf_dma;
	u32 mode, res;

	pci_dma_sync_single_for_device(dev->pdev, dma, size,
				       PCI_DMA_FROMDEVICE);
	clear_irqs(dev);

	mode = CMD_MODE_10 | CMD_BANK(dev->cs);
	nfc_write_indirect(pm, mode | dev->page, 0x2000 | 1);
	nfc_write_indirect(pm, mode | ((u16)(dma >> 16) << 8), 0x2200);
	nfc_write_indirect(pm, mode | ((u16)dma << 8), 0x2300);
	nfc_write_indirect(pm, mode | 0x14000, 0x2400);

	res = wait_irq(dev, irq_mask);

	pci_dma_sync_single_for_cpu(dev->pdev, dma, size, PCI_DMA_FROMDEVICE);
	return res;
}

/* Read full page (main + spare) with ECC check */
static int nfc_read_page(struct mtd_info *mtd, struct nand_chip *chip, u8 *buf)
{
	struct nfc_device *dev = mtd_to_dev(mtd);
	struct nfc_regs __iomem *p = dev->regs;
	u8 *sbuf, *dbuf;
	int i, sts;

	DBG("%d: read_page(%d)\n", dev->cs, dev->page);

	nfc_writel_flush(1, &p->dma_en);
	sts = do_dma_read(dev, IRQ_STS_ECC_DONE | IRQ_STS_ECC_FAILED);

	/* Remove gaps used by "infix OOB" */
	for (i = 0, sbuf = dev->buf, dbuf = buf;
	     i < mtd->writesize / ECC_SECTOR_SIZE; ++i) {
		memcpy(dbuf, sbuf, ECC_SECTOR_SIZE);
		sbuf += ECC_SECTOR_SIZE + ECC_BYTES_PER_SECTOR;
		dbuf += ECC_SECTOR_SIZE;
	}
	memcpy(chip->oob_poi + chip->ecc.total, sbuf, mtd->oobavail);

	if (unlikely(sts & IRQ_STS_ECC_FAILED)) {
		u32 err_addr, info;
		int sectors = mtd->writesize / ECC_SECTOR_SIZE;
		int check_ecc_erased = 0, ecc_corrected = 0;

		do {
			err_addr = nfc_readl(&p->ecc_error_address);
			info = nfc_readl(&p->ecc_correction_info);

			DBG("%d: ECC failed @ %d-%d:%d (0x%04x) 0x%04x\n",
			    dev->cs, dev->page, ECC_ERR_ADDR_SECTOR(err_addr),
			    ECC_ERR_ADDR_OFFSET(err_addr), err_addr, info);

			if (!(info & ECC_ERR_INFO_UNREC)) {
				int sector = ECC_ERR_ADDR_SECTOR(err_addr);
				int off = ECC_ERR_ADDR_OFFSET(err_addr);
				if (likely(off < ECC_SECTOR_SIZE &&
					   sector < sectors)) {
					buf[sector * ECC_SECTOR_SIZE + off] ^=
					    ECC_ERR_INFO_VALUE(info);
				} else {
					DBG("%d: ECC failure inside ECC\n",
					    dev->cs);
				}
				++ecc_corrected;
			} else {
				/* ECC would fail for erased pages
				 * we defer reporting ECC error until
				 * we are sure that page is not empty.
				 */
				check_ecc_erased = 1;
			}
		} while (!(info & ECC_ERR_INFO_LAST));

		if (check_ecc_erased) {
			// TODO(surovegin): figure something more efficient
			u32 *pbuf = (u32 *) dev->buf;
			int n = (mtd->writesize + mtd->oobsize) / 4;

			for (; n; --n, ++pbuf)
				if (*pbuf != 0xffffffff) {
					/* OK, real unrecoverable ECC error */
					++mtd->ecc_stats.failed;
					break;
				}
		}

#ifdef CONFIG_MTD_NAND_INTEL_CE_IGNORE_1BIT_ERRORS
		if (ecc_corrected == 1)
			ecc_corrected = 0;
#endif
		mtd->ecc_stats.corrected += ecc_corrected;
	}
	nfc_writel_flush(0, &p->dma_en);

	return 0;
}

/* Read full page (main + spare) without ECC check */
static int nfc_read_page_raw(struct mtd_info *mtd, struct nand_chip *chip,
			     u8 *buf)
{
	struct nfc_device *dev = mtd_to_dev(mtd);
	struct nfc_regs __iomem *p = dev->regs;

	DBG("%d: read_page_raw(%d)\n", dev->cs, dev->page);

	nfc_writel_flush(0, &p->ecc_en);
	nfc_writel_flush(1, &p->dma_en);
	do_dma_read(dev, IRQ_STS_DMA_DONE);
	nfc_writel_flush(0, &p->dma_en);
	nfc_writel_flush(1, &p->ecc_en);

	memcpy(buf, dev->buf, mtd->writesize);
	memcpy(chip->oob_poi, dev->buf + mtd->writesize, mtd->oobsize);

	return 0;
}

/* Perform full page (main + spare) DMA write from dev->buf */
static void do_dma_write(struct nfc_device *dev)
{
	struct nfc_regs __iomem *p = dev->regs;
	struct nfc_mem_regs __iomem *pm = dev->mem_regs;
	size_t size = dev->mtd.writesize + dev->mtd.oobsize;
	dma_addr_t dma = dev->buf_dma;
	u32 mode, sts;

	pci_dma_sync_single_for_device(dev->pdev, dma, size, PCI_DMA_TODEVICE);

	clear_irqs(dev);
	nfc_writel_flush(1, &p->dma_en);

	mode = CMD_MODE_10 | CMD_BANK(dev->cs);
	nfc_write_indirect(pm, mode | dev->page, 0x2100 | 1);
	nfc_write_indirect(pm, mode | ((u16)(dma >> 16) << 8), 0x2200);
	nfc_write_indirect(pm, mode | ((u16)dma << 8), 0x2300);
	nfc_write_indirect(pm, mode | 0x14000, 0x2400);

	sts = wait_irq(dev, IRQ_STS_PROG_FAILED | IRQ_STS_DMA_DONE);
	dev->status = (sts & IRQ_STS_PROG_FAILED) ? NAND_STATUS_FAIL : 0;

	nfc_writel_flush(0, &p->dma_en);

	pci_dma_sync_single_for_cpu(dev->pdev, dma, size, PCI_DMA_TODEVICE);
}

static void nfc_write_page(struct mtd_info *mtd, struct nand_chip *chip,
			   const u8 *buf)
{
	struct nfc_device *dev = mtd_to_dev(mtd);
	u8 *dbuf = dev->buf;
	int i;

	DBG("%d: write_page(%d)\n", dev->cs, dev->page);

	/* Prepare "infix OOB" page layout leaving space for ECC between
	 * each sector. Actual contents of these gaps don't matter.
	 */
	for (i = 0; i < mtd->writesize / ECC_SECTOR_SIZE; ++i) {
		memcpy(dbuf, buf, ECC_SECTOR_SIZE);
		dbuf += ECC_SECTOR_SIZE + ECC_BYTES_PER_SECTOR;
		buf += ECC_SECTOR_SIZE;
	}
	/* The rest of OOB */
	memcpy(dbuf, chip->oob_poi + chip->ecc.total, mtd->oobavail);

	do_dma_write(dev);
}

static void nfc_write_page_raw(struct mtd_info *mtd, struct nand_chip *chip,
			       const u8 *buf)
{
	struct nfc_device *dev = mtd_to_dev(mtd);
	struct nfc_regs __iomem *p = dev->regs;

	DBG("%d: write_page_raw(%d)\n", dev->cs, dev->page);

	memcpy(dev->buf, buf, mtd->writesize);
	memcpy(dev->buf + mtd->writesize, chip->oob_poi, mtd->oobsize);

	nfc_writel(0, &p->ecc_en);
	do_dma_write(dev);
	nfc_writel_flush(1, &p->ecc_en);
}

static int nfc_read_oob(struct mtd_info *mtd, struct nand_chip *chip,
			int page, int sndcmd)
{
	struct nfc_device *dev = mtd_to_dev(mtd);
	struct nfc_mem_regs __iomem *pm = dev->mem_regs;
	u32 addr = CMD_BANK(dev->cs) | page;
	u32 *pbuf = (u32 *) chip->oob_poi;
	int i;

	DBG("%d: read_oob(%d, %d)\n", dev->cs, page, sndcmd);

	/* See comment in nfc_probe() for details */
	if (unlikely(dev->bb_scanning && page < dev->cefdk_pages)) {
		/* Lie about CEFDK pages' OOB area */
		memset(pbuf, 0xff, dev->mtd.oobsize);
		return 1;
	}

	clear_irqs(dev);

	nfc_write_indirect(pm, CMD_MODE_10 | addr, 0x41);
	nfc_write_indirect(pm, CMD_MODE_10 | addr, 0x2000 | 1);

	wait_irq(dev, IRQ_STS_LOADED);
	nfc_writel(CMD_MODE_01 | addr, &pm->cmd);

	for (i = 0; i < dev->mtd.oobsize / 4; ++i)
		*pbuf++ = nfc_readl(&pm->io);

	nfc_write_indirect(pm, CMD_MODE_10 | addr, 0x42);

	return 1;
}

static int nfc_write_oob(struct mtd_info *mtd, struct nand_chip *chip, int page)
{
	struct nfc_device *dev = mtd_to_dev(mtd);
	struct nfc_mem_regs __iomem *pm = dev->mem_regs;
	u32 sts, addr = CMD_BANK(dev->cs) | page;
	u32 *pbuf = (u32 *) chip->oob_poi;
	int i;

	DBG("%d: write_oob(%d)\n", dev->cs, page);

	clear_irqs(dev);

	nfc_write_indirect(pm, CMD_MODE_10 | addr, 0x41);
	nfc_writel(CMD_MODE_01 | addr, &pm->cmd);

	for (i = 0; i < mtd->oobsize / 4; ++i)
		nfc_writel(*pbuf++, &pm->io);

	sts = wait_irq(dev, IRQ_STS_PROG_FAILED | IRQ_STS_PROG_DONE);

	nfc_write_indirect(pm, CMD_MODE_10 | addr, 0x42);

	return (sts & IRQ_STS_PROG_FAILED) ? -EIO : 0;
}

static void nfc_cmdfunc(struct mtd_info *mtd, unsigned int cmd, int col,
			int page)
{
	struct nfc_device *dev = mtd_to_dev(mtd);

	DBG2("%d: nfc_cmdfunc(0x%x, %d, %d)\n", dev->cs, cmd, col, page);

	switch (cmd) {
	case NAND_CMD_RESET:
		reset_bank(dev);
		break;

	case NAND_CMD_READID:
		read_id(dev);
		break;

	case NAND_CMD_STATUS:
		read_status(dev);
		break;

	case NAND_CMD_READ0:
	case NAND_CMD_SEQIN:
		BUG_ON(col != 0);
		dev->page = page;
		break;

	case NAND_CMD_PAGEPROG:
		/* ignore */
		break;
	default:
		printk(KERN_ERR DRV_NAME ": unsupported command 0x%x\n", cmd);
		break;
	}
}

static void __devinit nfc_onfi_timings(struct nfc_device *dev, int mode)
{
	struct nfc_regs __iomem *p = dev->regs;
	static struct {
		u8 rdwr_en_lo;
		u8 rdwr_en_hi;
		u8 acc_clks;
		u8 addr_2_data;
		u8 re_2_re;
		u8 we_2_re;
		u8 re_2_we;
		u8 cs_setup;
	} timings[] __devinitdata = {
		/* These timings assume CLK_X = 4 * 50 MHz */
		{
			.rdwr_en_lo = 13,
			.rdwr_en_hi = 7,
			.acc_clks = 9,
			.addr_2_data = 40,
			.re_2_re = 40,
			.we_2_re = 24,
			.re_2_we = 40,
			.cs_setup = 12,
		},
		{
			.rdwr_en_lo = 6,
			.rdwr_en_hi = 4,
			.acc_clks = 7,
			.addr_2_data = 20,
			.re_2_re = 20,
			.we_2_re = 16,
			.re_2_we = 20,
			.cs_setup = 3,
		},
	};

	BUG_ON(mode < 0 || mode >= ARRAY_SIZE(timings));

	nfc_writel(timings[mode].rdwr_en_lo, &p->rdwr_en_lo);
	nfc_writel(timings[mode].rdwr_en_hi, &p->rdwr_en_hi);
	nfc_writel(timings[mode].acc_clks, &p->acc_clks);
	nfc_writel(timings[mode].addr_2_data, &p->addr_2_data);
	nfc_writel(timings[mode].re_2_re, &p->re_2_re);
	nfc_writel(timings[mode].we_2_re, &p->we_2_re);
	nfc_writel(timings[mode].re_2_we, &p->re_2_we);
	nfc_writel_flush(timings[mode].cs_setup, &p->cs_setup);
}

static inline int nfc_init_hw(struct nfc_device *dev)
{
	struct nfc_regs __iomem *p = dev->regs;
	int i, err;
	u32 r;

	r = nfc_readl(&p->onfi_timing_modes) & 0x3f;
	if (r) {
		int mode = fls(r) - 1;

		/* CE4100 only supports ONFI Mode 0 and 1 */
		if (mode > 1)
			mode = 1;

		nfc_onfi_timings(dev, mode);
	}
	else {
		printk(KERN_INFO " Defaulting to ONFI 1 timings\n");
		nfc_onfi_timings(dev, 1);
	}

	nfc_writel(0xf, &p->rb_pin_en);
	nfc_writel(1, &p->ce_dont_care);

	for (i = 0; i < MAX_BANKS; ++i) {
		nfc_writel_flush(IRQ_STS_ALL, &p->irq[i].sts);
		nfc_writel(IRQ_STS_ALL, &p->irq[i].en);
	}

	nfc_write_indirect(dev->mem_regs, CMD_MODE_10, 0x42);
	nfc_writel(1, &p->transfer_spare);
	nfc_writel(1, &p->ecc_en);
	nfc_writel(8, &p->ecc_correction);

	nfc_writel(0, &p->spare_area_skip_bytes);
	nfc_writel(0xffff, &p->spare_area_marker);

	err = request_irq(dev->pdev->irq, &nfc_irq_handler, IRQF_SHARED,
			  DRV_NAME, dev);
	if (unlikely(err)) {
		printk(KERN_ERR DRV_NAME ": failed to install IRQ handler\n");
		return err;
	}

	nfc_writel(1, &p->global_irq_en);
	for (i = 0; i < MAX_BANKS; ++i) {
		dev->cs = i;
		reset_bank(dev);
	}
	dev->cs = -1;

	return 0;
}

static inline void nfc_fini_hw(struct nfc_device *dev)
{
	nfc_writel_flush(0, &dev->regs->global_irq_en);
	free_irq(dev->pdev->irq, dev);
}

static void nfc_bug(struct mtd_info *mtd)
{
	BUG();
}

#define ECC_BYTES_PER_2K_PAGE	(ECC_BYTES_PER_SECTOR * (2048 / ECC_SECTOR_SIZE))
static struct nand_ecclayout oobinfo_2048 = {
	.eccbytes = ECC_BYTES_PER_2K_PAGE,
	.eccpos = { 0,   1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13,
		    14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27,
		    28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41,
		    42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55,
	},
	.oobfree = {{ ECC_BYTES_PER_2K_PAGE, 64 - ECC_BYTES_PER_2K_PAGE }}
};

static struct nand_bbt_descr bbt_main_descr_2048 = {
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
		| NAND_BBT_2BIT | NAND_BBT_VERSION,
	.offs =	ECC_BYTES_PER_2K_PAGE,
	.len = 4,
	.veroffs = ECC_BYTES_PER_2K_PAGE + 4,
	.maxblocks = 4,
	.pattern = (u8[]) { 'B', 'b', 't', '0' },
};

static struct nand_bbt_descr bbt_mirror_descr_2048 = {
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
		| NAND_BBT_2BIT | NAND_BBT_VERSION,
	.offs =	ECC_BYTES_PER_2K_PAGE,
	.len = 4,
	.veroffs = ECC_BYTES_PER_2K_PAGE + 4,
	.maxblocks = 4,
	.pattern = (u8[]) { '1', 't', 'b', 'B' },
};

#define ECC_BYTES_PER_4K_PAGE	(ECC_BYTES_PER_SECTOR * (4096 / ECC_SECTOR_SIZE))
static struct nand_ecclayout oobinfo_4096 = {
	.eccbytes = ECC_BYTES_PER_4K_PAGE,
	/* Note that eccpos array is too small for 4K,
	 * but we cannot increase it easily - it's part if user-space API.
	 * We do best we can - fill as much as possible, luckily we don't
	 * need it for any operation.
	 */
	.eccpos = { 0,   1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13,
		    14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27,
		    28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41,
		    42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55,
		    56, 57, 58, 59, 60, 61, 62, 63
	},
	.oobfree = {{ ECC_BYTES_PER_4K_PAGE, 128 - ECC_BYTES_PER_4K_PAGE }}
};

static struct nand_bbt_descr bbt_main_descr_4096 = {
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
		| NAND_BBT_2BIT | NAND_BBT_VERSION,
	.offs =	ECC_BYTES_PER_4K_PAGE,
	.len = 4,
	.veroffs = ECC_BYTES_PER_4K_PAGE + 4,
	.maxblocks = 4,
	.pattern = (u8[]) { 'B', 'b', 't', '0' },
};

static struct nand_bbt_descr bbt_mirror_descr_4096 = {
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
		| NAND_BBT_2BIT | NAND_BBT_VERSION,
	.offs =	ECC_BYTES_PER_4K_PAGE,
	.len = 4,
	.veroffs = ECC_BYTES_PER_4K_PAGE + 4,
	.maxblocks = 4,
	.pattern = (u8[]) { '1', 't', 'b', 'B' },
};

static int __devinit nfc_probe(struct pci_dev *pdev,
			       const struct pci_device_id *id)
{
	struct nfc_device *dev;
	int res;

	printk(KERN_INFO DRV_DESC ", version " DRV_VERSION "\n");
	DBG(": nfc_probe\n");

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (unlikely(!dev)) {
		printk(KERN_ERR DRV_NAME ": failed to allocate device data\n");
		return -ENOMEM;
	}
	dev->buf = kmalloc(IO_BUF_SIZE, GFP_KERNEL);
	if (unlikely(!dev->buf)) {
		printk(KERN_ERR DRV_NAME ": failed to allocate I/O buffer\n");
		res = -ENOMEM;
		goto out_free_dev;
	}
	dev->pdev = pdev;
	spin_lock_init(&dev->lock);
	init_waitqueue_head(&dev->wq);

	res = pci_enable_device(pdev);
	if (unlikely(res)) {
		printk(KERN_ERR DRV_NAME ": failed to enable PCI device\n");
		goto out_free_buf;
	}

	if (unlikely(!(pci_resource_flags(pdev, 0) & IORESOURCE_MEM) ||
		     !(pci_resource_flags(pdev, 1) & IORESOURCE_MEM))) {
		printk(KERN_ERR DRV_NAME
		       ": cannnot find valid base addresses\n");
		res = -ENODEV;
		goto out_disable_pdev;
	}

	res = pci_request_regions(pdev, DRV_NAME);
	if (unlikely(res)) {
		printk(KERN_ERR DRV_NAME ": failed to obtain PCI resources\n");
		goto out_disable_pdev;
	}

	res = pci_set_dma_mask(pdev, DMA_32BIT_MASK);
	if (unlikely(res)) {
		printk(KERN_ERR DRV_NAME ": no usable DMA configuration\n");
		goto out_release_regions;
	}

	dev->buf_dma = pci_map_single(pdev, dev->buf, IO_BUF_SIZE,
				      PCI_DMA_BIDIRECTIONAL);
	if (unlikely(pci_dma_mapping_error(dev->buf_dma))) {
		printk(KERN_ERR DRV_NAME ": failed to map I/O buffer for DMA\n");
		res = -ENOMEM;
		goto out_release_regions;
	}

	dev->regs = ioremap(pci_resource_start(pdev, 1), sizeof(*dev->regs));
	dev->mem_regs =
	    ioremap(pci_resource_start(pdev, 0), sizeof(*dev->mem_regs));
	if (unlikely(!dev->regs || !dev->mem_regs)) {
		printk(KERN_ERR DRV_NAME
		       ": failed to ioremap device registers\n");
		res = -ENOMEM;
		goto out_unmap;
	}
	pci_set_master(pdev);

	dev->mtd.priv = &dev->nand;
	dev->mtd.owner = THIS_MODULE;
	dev->mtd.name = DRV_NAME;

	dev->nand.options = NAND_USE_FLASH_BBT | NAND_SKIP_BBTSCAN |
			    NAND_NO_READRDY;
	dev->nand.waitfunc = nfc_waitfunc;
	dev->nand.select_chip = nfc_select_chip;
	dev->nand.cmdfunc = nfc_cmdfunc;
	dev->nand.read_word = nfc_read_word;
	dev->nand.read_byte = nfc_read_byte;
	dev->nand.read_buf = (void *)nfc_bug;
	dev->nand.write_buf = (void *)nfc_bug;
	dev->nand.verify_buf = nfc_verify_buf;
	dev->nand.chip_delay = 0;

	res = nfc_init_hw(dev);
	if (unlikely(res))
		goto out_unmap;

	if (nand_scan_ident(&dev->mtd, MAX_BANKS)) {
		res = -ENXIO;
		goto out_fini_hw;
	}

	if ((dev->mtd.writesize != 2048 && dev->mtd.writesize != 4096) ||
	    (dev->mtd.oobsize != 64 && dev->mtd.oobsize != 128)) {
		printk(KERN_ERR DRV_NAME
		       ": unsupported page size %d or OOB size %d\n",
		       dev->mtd.writesize, dev->mtd.oobsize);
		res = -ENXIO;
		goto out_fini_hw;
	}

	dev->nand.erase_cmd = nfc_erase;
	dev->nand.ecc.mode = NAND_ECC_HW_SYNDROME;
	if (dev->mtd.writesize == 2048) {
		dev->nand.ecc.bytes = ECC_BYTES_PER_2K_PAGE,
		dev->nand.ecc.layout = &oobinfo_2048;
		dev->nand.bbt_td = &bbt_main_descr_2048;
		dev->nand.bbt_md = &bbt_mirror_descr_2048;
	} else if (dev->mtd.writesize == 4096) {
		dev->nand.ecc.bytes = ECC_BYTES_PER_4K_PAGE,
		dev->nand.ecc.layout = &oobinfo_4096;
		dev->nand.bbt_td = &bbt_main_descr_4096;
		dev->nand.bbt_md = &bbt_mirror_descr_4096;

		if (dev->mtd.oobsize <
		    nfc_readl(&dev->regs->device_spare_area_size)) {
			printk(KERN_WARNING DRV_NAME ": forcing smaller OOB\n");
			nfc_writel(dev->mtd.oobsize,
				   &dev->regs->device_spare_area_size);
		}
	} else {
		BUG();
	}
	dev->nand.ecc.size = dev->mtd.writesize;
	dev->nand.ecc.hwctl = (void *)nfc_bug;
	dev->nand.ecc.calculate = (void *)nfc_bug;
	dev->nand.ecc.correct  = (void *)nfc_bug;
	dev->nand.ecc.read_page = nfc_read_page;
	dev->nand.ecc.read_page_raw = nfc_read_page_raw;
	dev->nand.ecc.read_oob = nfc_read_oob;
	dev->nand.ecc.write_page = nfc_write_page;
	dev->nand.ecc.write_page_raw = nfc_write_page_raw;
	dev->nand.ecc.write_oob = nfc_write_oob;

	dev->cefdk_pages = CEFDK_BLOCKS *
		(1 << (dev->nand.phys_erase_shift - dev->nand.page_shift));

	/* We do a scan later */
	if (dev->cefdk_pages > 0)
		dev->nand.options |= NAND_SKIP_BBTSCAN;

	if (nand_scan_tail(&dev->mtd)) {
		res = -ENXIO;
		goto out_fini_hw;
	}

	if (dev->cefdk_pages > 0) {
		/* CEFDK managed blocks have manufacturer bad block markers
		 * overwritten, we always assume that this blocks are good
		 * as there is no way to know otherwise.
		 * For this purpose, we do scan manually, and use a flag
		 * to make nfc_read_oob() to always return 0xff's for
		 * CEFDK blocks.
		 */
		dev->bb_scanning = 1;
		dev->nand.scan_bbt(&dev->mtd);
		dev->bb_scanning = 0;
	}

#ifdef CONFIG_MTD_CMDLINE_PARTS
{
	static const char *part_probes[] = { "cmdlinepart", NULL };
	struct mtd_partition *partitions;
	int num_partitions =
		parse_mtd_partitions(&dev->mtd, part_probes, &partitions, 0);

	if (partitions && num_partitions)
		res = add_mtd_partitions(&dev->mtd, partitions, num_partitions);
	else
		res = add_mtd_device(&dev->mtd);
}
#else
	res = add_mtd_device(&dev->mtd);
#endif
	if (res)
		goto out_release;

	pci_set_drvdata(pdev, dev);
	return 0;

      out_release:
	nand_release(&dev->mtd);
      out_fini_hw:
        nfc_fini_hw(dev);
      out_unmap:
	if (dev->regs)
		iounmap(dev->regs);
	if (dev->mem_regs)
		iounmap(dev->mem_regs);
	pci_unmap_single(pdev, dev->buf_dma, IO_BUF_SIZE,
			 PCI_DMA_BIDIRECTIONAL);
      out_release_regions:
	pci_release_regions(pdev);
      out_disable_pdev:
	pci_disable_device(pdev);
      out_free_buf:
	kfree(dev->buf);
      out_free_dev:
	kfree(dev);
	return res;
}

static void nfc_shutdown(struct pci_dev *pdev)
{
	DBG(": nfc_shutdown\n");
	pci_disable_device(pdev);
}

static void __devexit nfc_remove(struct pci_dev *pdev)
{
	struct nfc_device *dev = pci_get_drvdata(pdev);

	DBG(": nfc_remove\n");

	pci_release_regions(pdev);
	pci_disable_device(pdev);
	pci_set_drvdata(pdev, NULL);
	nfc_fini_hw(dev);
	nand_release(&dev->mtd);
	iounmap(dev->regs);
	iounmap(dev->mem_regs);
	pci_unmap_single(pdev, dev->buf_dma, IO_BUF_SIZE,
			 PCI_DMA_BIDIRECTIONAL);
	kfree(dev->buf);
	kfree(dev);
}

static const struct pci_device_id intel_ce_nand_pci_ids[] = {
	{ PCI_VDEVICE(INTEL, 0x0701) },
	{ },
};
MODULE_DEVICE_TABLE(pci, intel_ce_nand_pci_ids);

static struct pci_driver intel_ce_nand_driver = {
	.name = DRV_NAME,
	.probe = nfc_probe,
	.remove = __devexit_p(nfc_remove),
	.shutdown = nfc_shutdown,
	.id_table = intel_ce_nand_pci_ids,
};

static int __init intel_ce_nand_init(void)
{
	DBG(": init\n");
	return pci_register_driver(&intel_ce_nand_driver);
}

static void __exit intel_ce_nand_fini(void)
{
	DBG(": fini\n");
	pci_unregister_driver(&intel_ce_nand_driver);
}

module_init(intel_ce_nand_init);
module_exit(intel_ce_nand_fini);
