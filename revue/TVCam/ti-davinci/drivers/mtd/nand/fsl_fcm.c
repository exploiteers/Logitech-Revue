/* linux/drivers/mtd/nand/fsl_fcm.c
 *
 * Copyright (c) 2006 Freescale Semiconductor
 *
 * Freescale Enhanced Local Bus Controller NAND driver
 *
 * Author: Nick Spence <Nick.Spence@freescale.com>
 * Maintainer: Tony Li <Tony.Li@freescale.com>
 *
 * Changelog:
 *      2006-12 Tony Li <Tony.Li@freescale.com>
 *              Adopt to MPC8313ERDB board
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/fsl_devices.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/nand_ecc.h>
#include <linux/mtd/partitions.h>

#include <asm/io.h>
#include <linux/mtd/fsl_elbc.h>

#define MIN(x, y)		((x < y) ? x : y)

#define PARAM_VERIFY(x) if (!(x)) {			\
	pr_debug ("ERROR %s: " ""#x"" "\n", __func__);	\
	return -EINVAL;					\
}

#define PARAM_VERIFY_VOID(x) if (!(x)) {			\
	pr_debug ("ERROR %s: " ""#x"" "\n", __func__);	\
	return;					\
}

struct fsl_elbc_ctrl fcm_ctrl;

/* These map to the positions used by the FCM hardware ECC generator */

/* Small Page FLASH with FMR[ECCM] = 0 */
static struct nand_ecclayout fsl_fcm_oob_sp_eccm0 = {
	.eccbytes = 3,
	.eccpos = {6, 7, 8},
	.oobfree = {{0, 5}, {9, 7}}
};

/* Small Page FLASH with FMR[ECCM] = 1 */
static struct nand_ecclayout fsl_fcm_oob_sp_eccm1 = {
	.eccbytes = 3,
	.eccpos = {8, 9, 10},
	.oobfree = {{0, 5}, {6, 2}, {11, 5}}
};

/* Large Page FLASH with FMR[ECCM] = 0 */
static struct nand_ecclayout fsl_fcm_oob_lp_eccm0 = {
	.eccbytes = 12,
	.eccpos = {6, 7, 8, 22, 23, 24, 38, 39, 40, 54, 55, 56},
	.oobfree = {{1, 5}, {9, 13}, {25, 13}, {41, 13}, {57, 7}}
};

/* Large Page FLASH with FMR[ECCM] = 1 */
static struct nand_ecclayout fsl_fcm_oob_lp_eccm1 = {
	.eccbytes = 12,
	.eccpos = {8, 9, 10, 24, 25, 26, 40, 41, 42, 56, 57, 58},
	.oobfree = {{1, 7}, {11, 13}, {27, 13}, {43, 13}, {59, 5}}
};

/*=================================*/

/*
 * Set up the FCM hardware block and page address fields, and the fcm
 * structure addr field to point to the correct FCM buffer in memory
 */
static void set_addr(struct mtd_info *mtd, int column, int page_addr, int oob)
{
	struct nand_chip *chip;
	struct fsl_elbc_mtd *nmtd;
	struct fsl_elbc_ctrl *ctrl;
	volatile lbus83xx_t *lbc;
	int buf_num;

	PARAM_VERIFY_VOID(mtd);
	chip = mtd->priv;
	PARAM_VERIFY_VOID(chip);
	nmtd = chip->priv;
	PARAM_VERIFY_VOID(nmtd);
	ctrl = nmtd->ctrl;
	PARAM_VERIFY_VOID(ctrl);
	lbc = ctrl->regs;
	PARAM_VERIFY_VOID(lbc);

	ctrl->page = page_addr;

	out_be32(&lbc->fbar, page_addr >> (chip->phys_erase_shift - chip->page_shift));
	if (nmtd->pgs) {
		out_be32(&lbc->fpar, ((page_addr << FPAR_LP_PI_SHIFT) & FPAR_LP_PI) |
			(oob ? FPAR_LP_MS : 0) | column);
		buf_num = (page_addr & 1) << 2;
	} else {
		out_be32(&lbc->fpar, ((page_addr << FPAR_SP_PI_SHIFT) & FPAR_SP_PI) |
			(oob ? FPAR_SP_MS : 0) | column);
		buf_num = page_addr & 7;
	}
	ctrl->addr = (unsigned char *)(nmtd->vbase + (buf_num * 1024));

	/* for OOB data point to the second half of the buffer */
	if (oob) {
		ctrl->addr += (nmtd->pgs ? 2048 : 512);
	}
	pr_debug("set_addr: bank=%d, ctrl->addr=0x%p (0x%08x)\n",
		buf_num, ctrl->addr, nmtd->vbase);
}

/* fsl_fcm_cmd_ctrl
 *
 * Issue command and address cycles to the chip
*/

static void fsl_fcm_cmd_ctrl(struct mtd_info *mtd, int dat,
				    unsigned int ctrl)
{
}

/*
 * execute FCM command and wait for it to complete
 */
static int fsl_fcm_run_command(struct mtd_info *mtd)
{
	struct nand_chip *chip;
	struct fsl_elbc_mtd *nmtd;
	struct fsl_elbc_ctrl *ctrl;
	volatile lbus83xx_t *lbc;

	PARAM_VERIFY(mtd);
	chip = mtd->priv;
	PARAM_VERIFY(chip);
	nmtd = chip->priv;
	PARAM_VERIFY(nmtd);
	ctrl = nmtd->ctrl;
	PARAM_VERIFY(ctrl);
	lbc = ctrl->regs;
	PARAM_VERIFY(lbc);

	/* Setup the FMR[OP] to execute without write protection */
	out_be32(&lbc->fmr, nmtd->fmr | 3);
	if (ctrl->use_mdr)
		out_be32(&lbc->mdr, ctrl->mdr);

	pr_debug("fsl_fcm_run_command: fmr= %08X fir= %08X fcr= %08X\n",
		  in_be32(&lbc->fmr), in_be32(&lbc->fir), in_be32(&lbc->fcr));
	pr_debug("fsl_fcm_run_command: fbar=%08X fpar=%08X fbcr=%08X bank=%d\n",
		  in_be32(&lbc->fbar), in_be32(&lbc->fpar), in_be32(&lbc->fbcr),
			nmtd->bank);

	/* clear event registers */
	out_be32(&lbc->lteatr, 0);
	out_be32(&lbc->ltesr, (LTESR_FCT | LTESR_PAR | LTESR_CC));

	/* execute special operation */
	out_be32(&lbc->lsor, nmtd->bank);

	/* wait for FCM complete flag or timeout */
	might_sleep();
	ctrl->status = ctrl->irq_status = 0;
	wait_event_timeout(ctrl->irq_wait, ctrl->irq_status,
			   FCM_TIMEOUT_MSECS * HZ / 1000);
	ctrl->status = ctrl->irq_status;

	/* store mdr value in case it was needed */
	if (ctrl->use_mdr)
		ctrl->mdr = in_be32(&lbc->mdr);

	ctrl->use_mdr = 0;

	pr_debug("fsl_fcm_run_command: stat=%08X mdr= %08X fmr= %08X\n",
		  ctrl->status, ctrl->mdr, in_be32(&lbc->fmr));

	/* returns 0 on success otherwise non-zero) */
	return (ctrl->status == LTESR_CC ? 0 : EFAULT);
}

/* cmdfunc send commands to the FCM */
static void fsl_fcm_cmdfunc(struct mtd_info *mtd, unsigned command,
			     int column, int page_addr)
{
	struct nand_chip *chip;
	struct fsl_elbc_mtd *nmtd;
	struct fsl_elbc_ctrl *ctrl;
	volatile lbus83xx_t *lbc;

	PARAM_VERIFY_VOID(mtd);
	chip = mtd->priv;
	PARAM_VERIFY_VOID(chip);
	nmtd = chip->priv;
	PARAM_VERIFY_VOID(nmtd);
	ctrl = nmtd->ctrl;
	PARAM_VERIFY_VOID(ctrl);
	lbc = ctrl->regs;
	PARAM_VERIFY_VOID(lbc);

	ctrl->use_mdr = 0;

	/* clear the read buffer */
	ctrl->read_bytes = 0;
	if (command != NAND_CMD_PAGEPROG) {
		ctrl->index = 0;
		ctrl->oobbuf = -1;
	}

	switch (command) {
		/* READ0 and READ1 read the entire buffer to use hardware ECC */
	case NAND_CMD_READ1:
		pr_debug("fsl_fcm_cmdfunc: NAND_CMD_READ1, page_addr:"
			  " 0x%x, column: 0x%x.\n", page_addr, column);
		ctrl->index = column + 256;
		goto read0;
	case NAND_CMD_READ0:
		pr_debug("fsl_fcm_cmdfunc: NAND_CMD_READ0, page_addr:"
			  " 0x%x, column: 0x%x.\n", page_addr, column);
		ctrl->index = column;
	      read0:
		if (nmtd->pgs) {
			out_be32(&lbc->fir, (FIR_OP_CW0 << FIR_OP0_SHIFT) |
				(FIR_OP_CA << FIR_OP1_SHIFT) |
				(FIR_OP_PA << FIR_OP2_SHIFT) |
				(FIR_OP_CW1 << FIR_OP3_SHIFT) |
				(FIR_OP_RBW << FIR_OP4_SHIFT));
		} else {
			out_be32(&lbc->fir, (FIR_OP_CW0 << FIR_OP0_SHIFT) |
				(FIR_OP_CA << FIR_OP1_SHIFT) |
				(FIR_OP_PA << FIR_OP2_SHIFT) |
				(FIR_OP_RBW << FIR_OP3_SHIFT));
		}
		out_be32(&lbc->fcr, (NAND_CMD_READ0 << FCR_CMD0_SHIFT) |
			(NAND_CMD_READSTART << FCR_CMD1_SHIFT));
		out_be32(&lbc->fbcr, 0);	/* read entire page to enable ECC */
		set_addr(mtd, 0, page_addr, 0);
		ctrl->read_bytes = mtd->writesize + mtd->oobsize;
		goto write_cmd2;
		/* READOOB read only the OOB becasue no ECC is performed */
	case NAND_CMD_READOOB:
		pr_debug("fsl_fcm_cmdfunc: NAND_CMD_READOOB, page_addr:"
			  " 0x%x, column: 0x%x.\n", page_addr, column);
		if (nmtd->pgs) {
			out_be32(&lbc->fir, (FIR_OP_CW0 << FIR_OP0_SHIFT) |
			    (FIR_OP_CA << FIR_OP1_SHIFT) |
			    (FIR_OP_PA << FIR_OP2_SHIFT) |
			    (FIR_OP_CW1 << FIR_OP3_SHIFT) |
			    (FIR_OP_RBW << FIR_OP4_SHIFT));
			out_be32(&lbc->fcr, (NAND_CMD_READ0 << FCR_CMD0_SHIFT) |
			    (NAND_CMD_READSTART << FCR_CMD1_SHIFT));
		} else {
			out_be32(&lbc->fir, (FIR_OP_CW0 << FIR_OP0_SHIFT) |
			    (FIR_OP_CA << FIR_OP1_SHIFT) |
			    (FIR_OP_PA << FIR_OP2_SHIFT) |
			    (FIR_OP_RBW << FIR_OP3_SHIFT));
			out_be32(&lbc->fcr, (NAND_CMD_READOOB << FCR_CMD0_SHIFT));
		}
		out_be32(&lbc->fbcr, mtd->oobsize - column);
		set_addr(mtd, column, page_addr, 1);
		ctrl->read_bytes = mtd->oobsize;
		ctrl->index = column;
		goto write_cmd2;
		/* READID must read all 5 possible bytes while CEB is active */
	case NAND_CMD_READID:
		pr_debug("fsl_fcm_cmdfunc: NAND_CMD_READID.\n");
		out_be32(&lbc->fir, (FIR_OP_CW0 << FIR_OP0_SHIFT) |
		    (FIR_OP_UA << FIR_OP1_SHIFT) |
		    (FIR_OP_RBW << FIR_OP2_SHIFT));
		 out_be32(&lbc->fcr, (NAND_CMD_READID << FCR_CMD0_SHIFT));
		out_be32(&lbc->fbcr, 5);
		ctrl->use_mdr = 1;
		ctrl->mdr = 0;
		goto write_cmd0;
		/* ERASE1 stores the block and page address */
	case NAND_CMD_ERASE1:
		pr_debug("fsl_fcm_cmdfunc: NAND_CMD_ERASE1, page_addr:"
			  " 0x%x.\n", page_addr);
		set_addr(mtd, 0, page_addr, 0);
		goto end;
		/* ERASE2 uses the block and page address from ERASE1 */
	case NAND_CMD_ERASE2:
		pr_debug("fsl_fcm_cmdfunc: NAND_CMD_ERASE2.\n");
		out_be32(&lbc->fir, (FIR_OP_CW0 << FIR_OP0_SHIFT) |
		    (FIR_OP_PA << FIR_OP1_SHIFT) |
		    (FIR_OP_CM1 << FIR_OP2_SHIFT));
		out_be32(&lbc->fcr, (NAND_CMD_ERASE1 << FCR_CMD0_SHIFT) |
		    (NAND_CMD_ERASE2 << FCR_CMD1_SHIFT));
		out_be32(&lbc->fbcr, 0);
		goto write_cmd1;
		/* SEQIN sets up the addr buffer and all registers except the length */
	case NAND_CMD_SEQIN:
		pr_debug("fsl_fcm_cmdfunc: NAND_CMD_SEQIN/PAGE_PROG, page_addr:"
			  " 0x%x, column: 0x%x.\n", page_addr, column);
		if (column == 0)
			out_be32(&lbc->fbcr, 0);
		else
			out_be32(&lbc->fbcr, 1);
		if (nmtd->pgs) {
			/* always use READ0 for large page devices */
			out_be32(&lbc->fir, (FIR_OP_CW0 << FIR_OP0_SHIFT) |
			    (FIR_OP_CA << FIR_OP1_SHIFT) |
			    (FIR_OP_PA << FIR_OP2_SHIFT) |
			    (FIR_OP_WB << FIR_OP3_SHIFT) |
			    (FIR_OP_CW1 << FIR_OP4_SHIFT));
			out_be32(&lbc->fcr, (NAND_CMD_SEQIN << FCR_CMD0_SHIFT) |
			    (NAND_CMD_PAGEPROG << FCR_CMD1_SHIFT));
			set_addr(mtd, column, page_addr, 0);
		} else {
			 out_be32(&lbc->fir, (FIR_OP_CW0 << FIR_OP0_SHIFT) |
			    (FIR_OP_CM2 << FIR_OP1_SHIFT) |
			    (FIR_OP_CA << FIR_OP2_SHIFT) |
			    (FIR_OP_PA << FIR_OP3_SHIFT) |
			    (FIR_OP_WB << FIR_OP4_SHIFT) |
			    (FIR_OP_CW1 << FIR_OP5_SHIFT));
			if (column >= mtd->writesize) {
				/* OOB area --> READOOB */
				column -= mtd->writesize;
				out_be32(&lbc->fcr, (NAND_CMD_READOOB << FCR_CMD0_SHIFT)
				    | (NAND_CMD_PAGEPROG << FCR_CMD1_SHIFT)
				    | (NAND_CMD_SEQIN << FCR_CMD2_SHIFT));
				set_addr(mtd, column, page_addr, 1);
			} else if (column < 256) {
				/* First 256 bytes --> READ0 */
				out_be32(&lbc->fcr, (NAND_CMD_READ0 << FCR_CMD0_SHIFT)
				    | (NAND_CMD_PAGEPROG << FCR_CMD1_SHIFT)
				    | (NAND_CMD_SEQIN << FCR_CMD2_SHIFT));
				set_addr(mtd, column, page_addr, 0);
			} else {
				/* Second 256 bytes --> READ1 */
				column -= 256;
				out_be32(&lbc->fcr, (NAND_CMD_READ1 << FCR_CMD0_SHIFT)
				    | (NAND_CMD_PAGEPROG << FCR_CMD1_SHIFT)
				    | (NAND_CMD_SEQIN << FCR_CMD2_SHIFT));
				set_addr(mtd, column, page_addr, 0);
			}
		}
		goto end;
		/* PAGEPROG reuses all of the setup from SEQIN and adds the length */
	case NAND_CMD_PAGEPROG:
		pr_debug("fsl_fcm_cmdfunc: NAND_CMD_PAGEPROG"
			  " writing %d bytes.\n", ctrl->index);
		/* if the write did not start at 0 or is not a full page */
		/* then set the exact length, otherwise use a full page  */
		/* write so the HW generates the ECC. */
		if (in_be32(&lbc->fbcr) ||
		    (ctrl->index != (mtd->writesize + mtd->oobsize)))
			out_be32(&lbc->fbcr, ctrl->index);
		goto write_cmd2;
		/* CMD_STATUS must read the status byte while CEB is active */
		/* Note - it does not wait for the ready line */
	case NAND_CMD_STATUS:
		pr_debug("fsl_fcm_cmdfunc: NAND_CMD_STATUS.\n");
		out_be32(&lbc->fir, (FIR_OP_CM0 << FIR_OP0_SHIFT) |
		    (FIR_OP_RBW << FIR_OP1_SHIFT));
		out_be32(&lbc->fcr, (NAND_CMD_STATUS << FCR_CMD0_SHIFT));
		out_be32(&lbc->fbcr, 1);
		goto write_cmd0;
		/* RESET without waiting for the ready line */
	case NAND_CMD_RESET:
		pr_debug("fsl_fcm_cmdfunc: NAND_CMD_RESET.\n");
		out_be32(&lbc->fir, (FIR_OP_CM0 << FIR_OP0_SHIFT));
		out_be32(&lbc->fcr, (NAND_CMD_RESET << FCR_CMD0_SHIFT));
		out_be32(&lbc->fbcr, 0);
		goto write_cmd0;
	default:
		printk("fsl_fcm_cmdfunc: error, unsupported command.\n");
		goto end;
	}

	/* Short cuts fall through to save code */
      write_cmd0:
	set_addr(mtd, 0, 0, 0);
      write_cmd1:
	ctrl->read_bytes = in_be32(&lbc->fbcr);
      write_cmd2:
	fsl_fcm_run_command(mtd);

#ifdef CONFIG_MTD_NAND_VERIFY_WRITE
	/* if we wrote a page then read back the oob to get the ECC */
	if ((command == NAND_CMD_PAGEPROG) &&
	    (chip->ecc.mode > NAND_ECC_SOFT) &&
	    (in_be32(&lbc->fbcr) == 0) && (ctrl->oobbuf != 0) && (ctrl->oobbuf != -1)) {
		int i;
		uint *oob_config;
		unsigned char *oob_buf;

		i = ctrl->page;
		oob_buf = (unsigned char *)ctrl->oobbuf;
		oob_config = chip->ecc.layout->eccpos;

		/* wait for the write to complete and check it passed */
		if (!(chip->waitfunc(mtd, chip) & 0x01)) {
			/* read back the OOB */
			fsl_fcm_cmdfunc(mtd, NAND_CMD_READOOB, 0, i);
			/* if it succeeded then copy the ECC bytes */
			if (ctrl->status == LTESR_CC) {
				for (i = 0; i < chip->ecc.layout->eccbytes; i++) {
					oob_buf[oob_config[i]] =
					    ctrl->addr[oob_config[i]];
				}
			}
		}
	}
#endif

      end:
	return;
}

/*
 * FCM does not support 16 bit data busses
 */
static u16 fsl_fcm_read_word(struct mtd_info *mtd)
{
	struct nand_chip *chip;
	struct fsl_elbc_ctrl *ctrl;

	PARAM_VERIFY(mtd);
	chip = mtd->priv;
	PARAM_VERIFY(chip);
	ctrl = (struct fsl_elbc_ctrl *)chip->controller;
	PARAM_VERIFY(ctrl);

	dev_err(ctrl->device, "fsl_fcm_read_word: UNIMPLEMENTED.\n");
	return 0;
}

/*
 * Write buf to the FCM Controller Data Buffer
 */
static void fsl_fcm_write_buf(struct mtd_info *mtd, const u_char * buf,
			       int len)
{
	struct nand_chip *chip;
	struct fsl_elbc_mtd *nmtd;
	struct fsl_elbc_ctrl *ctrl;

	PARAM_VERIFY_VOID(mtd);
	PARAM_VERIFY_VOID(buf);
	chip = mtd->priv;
	PARAM_VERIFY_VOID(chip);
	nmtd = chip->priv;
	PARAM_VERIFY_VOID(nmtd);
	ctrl = nmtd->ctrl;
	PARAM_VERIFY_VOID(ctrl);

	pr_debug("fsl_fcm_write_buf: writing %d bytes starting with 0x%lx"
		  " at %d.\n", len, *((unsigned long *)buf), ctrl->index);

	/* If armed catch the address of the OOB buffer so that it can be */
	/* updated with the real signature after the program comletes */
	if (!ctrl->oobbuf)
		ctrl->oobbuf = (int)buf;

	/* copy the data into the FCM hardware buffer and update the index */
	memcpy(&(ctrl->addr[ctrl->index]), buf, len);
	ctrl->index += len;
	return;
}

/*
 * read a byte from either the FCM hardware buffer if it has any data left
 * otherwise issue a command to read a single byte.
 */
static u_char fsl_fcm_read_byte(struct mtd_info *mtd)
{
	struct nand_chip *chip;
	struct fsl_elbc_mtd *nmtd;
	struct fsl_elbc_ctrl *ctrl;
	volatile lbus83xx_t *lbc;
	unsigned char byte;

	PARAM_VERIFY(mtd);
	chip = mtd->priv;
	PARAM_VERIFY(chip);
	nmtd = chip->priv;
	PARAM_VERIFY(nmtd);
	ctrl = nmtd->ctrl;
	PARAM_VERIFY(ctrl);
	lbc = ctrl->regs;
	PARAM_VERIFY(lbc);

	/* If there are still bytes in the FCM then use the next byte */
	if (ctrl->index < ctrl->read_bytes) {
		byte = ctrl->addr[(ctrl->index)++];
		pr_debug("fsl_fcm_read_byte: byte %u (%02X): %d of %d.\n",
			  byte, byte, ctrl->index - 1, ctrl->read_bytes);
	} else {
		/* otherwise issue a command to read 1 byte */
		out_be32(&lbc->fir, (FIR_OP_RSW << FIR_OP0_SHIFT));
		ctrl->use_mdr = 1;
		ctrl->read_bytes = 0;
		ctrl->index = 0;
		ctrl->read_bytes = 0;
		ctrl->index = 0;
		byte = fsl_fcm_run_command(mtd) ? ERR_BYTE : ctrl->mdr & 0xff;
		pr_debug("fsl_fcm_read_byte: byte %u (%02X) from bus.\n",
			  byte, byte);
	}

	return byte;
}

/*
 * Read from the FCM Controller Data Buffer
 */
static void fsl_fcm_read_buf(struct mtd_info *mtd, u_char * buf, int len)
{
	struct nand_chip *chip;
	struct fsl_elbc_mtd *nmtd;
	struct fsl_elbc_ctrl *ctrl;
	int i;
	int rest;
	unsigned long old_status;

	PARAM_VERIFY_VOID(mtd);
	chip = mtd->priv;
	PARAM_VERIFY_VOID(chip);
	nmtd = chip->priv;
	PARAM_VERIFY_VOID(nmtd);
	ctrl = nmtd->ctrl;
	PARAM_VERIFY_VOID(ctrl);

	pr_debug("fsl_fcm_read_buf: reading %d bytes.\n", len);

	/* see how much is still in the FCM buffer */
	i = min((unsigned int)len, (ctrl->read_bytes - ctrl->index));
	rest = len - i;
	len = i;

	/* copying bytes even if there was an error so that the oob works */
	memcpy(buf, &(ctrl->addr[(ctrl->index)]), len);
	ctrl->index += len;

	/* If more data is needed then issue another block read */
	if (rest) {
		pr_debug("fsl_fcm_read_buf: getting %d more bytes.\n",
			  rest);

		buf += len;

		/* keep last status in case it was an error */
		old_status = ctrl->status;

		/* read full next page to use HW ECC if enabled */
		fsl_fcm_cmdfunc(mtd, NAND_CMD_READ0, 0, ctrl->page + 1);

		/* preserve the worst status code */
		if (ctrl->status == LTESR_CC)
			ctrl->status = old_status;

		fsl_fcm_read_buf(mtd, buf, rest);
	}
	return;
}

/*
 * Verify buffer against the FCM Controller Data Buffer
 */
static int fsl_fcm_verify_buf(struct mtd_info *mtd, const u_char * buf,
			       int len)
{
	struct nand_chip *chip;
	struct fsl_elbc_mtd *nmtd;
	struct fsl_elbc_ctrl *ctrl;
	int i;
	int rest;

	PARAM_VERIFY(mtd);
	chip = mtd->priv;
	PARAM_VERIFY(chip);
	nmtd = chip->priv;
	PARAM_VERIFY(nmtd);
	ctrl = nmtd->ctrl;
	PARAM_VERIFY(ctrl);

	pr_debug("fsl_fcm_verify_buf: checking %d bytes starting with 0x%02lx.\n",
		  len, *((unsigned long *)buf));

	/* If last read failed then return error bytes */
	if (ctrl->status != LTESR_CC) {
		return EFAULT;
	}

	/* see how much is still in the FCM buffer */
	i = min((unsigned int)len, (ctrl->read_bytes - ctrl->index));
	rest = len - i;
	len = i;

	if (memcmp(buf, &(ctrl->addr[(ctrl->index)]), len)) {
		return EFAULT;
	}

	ctrl->index += len;
	if (rest) {
		pr_debug("fsl_fcm_verify_buf: getting %d more bytes.\n",
			  rest);
		buf += len;

		/* read full next page to use HW ECC if enabled */
		fsl_fcm_cmdfunc(mtd, NAND_CMD_READ0, 0, ctrl->page + 1);

		return fsl_fcm_verify_buf(mtd, buf, rest);
	}
	return 0;
}

/* this function is called after Program and Erase Operations to
 * check for success or failure */
static int fsl_fcm_wait(struct mtd_info *mtd, struct nand_chip *this)
{
	struct nand_chip *chip;
	struct fsl_elbc_mtd *nmtd;
	struct fsl_elbc_ctrl *ctrl;
	lbus83xx_t *lbc;

	PARAM_VERIFY(mtd);
	chip = mtd->priv;
	PARAM_VERIFY(chip);
	nmtd = chip->priv;
	PARAM_VERIFY(nmtd);
	ctrl = nmtd->ctrl;
	PARAM_VERIFY(ctrl);
	lbc = ctrl->regs;
	PARAM_VERIFY(lbc);

  	if (ctrl->status != LTESR_CC) {
		return (0x1);	/* Status Read error */
	}

	/* Use READ_STATUS command, but wait for the device to be ready */
	ctrl->use_mdr = 0;
	ctrl->oobbuf = -1;
	out_be32(&lbc->fir, (FIR_OP_CW0 << FIR_OP0_SHIFT) |
	    (FIR_OP_RBW << FIR_OP1_SHIFT));
	out_be32(&lbc->fcr, (NAND_CMD_STATUS << FCR_CMD0_SHIFT));
	set_addr(mtd, 0, 0, 0);
	out_be32(&lbc->fbcr, 1);
	ctrl->index = 0;
	ctrl->read_bytes = lbc->fbcr;
	fsl_fcm_run_command(mtd);
	if (ctrl->status != LTESR_CC) {
		return (0x1);	/* Status Read error */
	}
	return chip->read_byte(mtd);
}

/* ECC handling functions */

/*
 * fsl_fcm_enable_hwecc - start ECC generation
 */
static void fsl_fcm_enable_hwecc(struct mtd_info *mtd, int mode)
{
	return;
}

/*
 * fsl_fcm_calculate_ecc - Calculate the ECC bytes
 * This is done by hardware during the write process, so we use this
 * to arm the oob buf capture on the next write_buf() call. The ECC bytes
 * only need to be captured if CONFIG_MTD_NAND_VERIFY_WRITE is defined which
 * reads back the pages and checks they match the data and oob buffers.
 */
static int fsl_fcm_calculate_ecc(struct mtd_info *mtd, const u_char * dat,
				  u_char * ecc_code)
{
#ifdef CONFIG_MTD_NAND_VERIFY_WRITE
	struct nand_chip *chip;
	struct fsl_elbc_mtd *nmtd;
	struct fsl_elbc_ctrl *ctrl;

	PARAM_VERIFY(mtd);
	chip = mtd->priv;
	PARAM_VERIFY(chip);
	nmtd = chip->priv;
	PARAM_VERIFY(nmtd);
	ctrl = nmtd->ctrl;
	PARAM_VERIFY(ctrl);
	/* arm capture of oob buf ptr on next write_buf */
	ctrl->oobbuf = 0;
#endif
	return 0;
}

/*
 * fsl_fcm_correct_data - Detect and correct bit error(s)
 * The detection and correction is done automatically by the hardware,
 * if the complete page was read. If the status code is okay then there
 * was no error, otherwise we return an error code indicating an uncorrectable
 * error.
 */
static int fsl_fcm_correct_data(struct mtd_info *mtd, u_char * dat,
				 u_char * read_ecc, u_char * calc_ecc)
{
	struct nand_chip *chip;
	struct fsl_elbc_mtd *nmtd;
	struct fsl_elbc_ctrl *ctrl;

	PARAM_VERIFY(mtd);
	chip = mtd->priv;
	PARAM_VERIFY(chip);
	nmtd = chip->priv;
	PARAM_VERIFY(nmtd);
	ctrl = nmtd->ctrl;
	PARAM_VERIFY(ctrl);
	/* No errors */
	if (ctrl->status == LTESR_CC)
		return 0;

	return -1;		/* uncorrectable error */
}

/*************************************************************************/
/*                     Chip setup and control functions                  */
/*************************************************************************/

/*
 * Dummy scan_bbt to complete setup of the FMR based on NAND size
 */
static int fsl_fcm_chip_init_tail(struct mtd_info *mtd)
{
	struct nand_chip *chip;
	struct fsl_elbc_mtd *nmtd;
	struct fsl_elbc_ctrl *ctrl;
	volatile lbus83xx_t *lbc;
	unsigned int i;
	unsigned int al;

	PARAM_VERIFY(mtd);
	chip = mtd->priv;
	PARAM_VERIFY(chip);
	nmtd = chip->priv;
	PARAM_VERIFY(nmtd);
	ctrl = nmtd->ctrl;
	PARAM_VERIFY(ctrl);
	lbc = ctrl->regs;
	PARAM_VERIFY(lbc);

	/* calculate FMR Address Length field */
	al = 0;
	for (i = chip->pagemask >> 16; i; i >>= 8) {
		al++;
	}

	/* add to ECCM mode set in fsl_fcm_init */
	nmtd->fmr |= 12 << FMR_CWTO_SHIFT |	/* Timeout > 12 mSecs */
	    al << FMR_AL_SHIFT;

	pr_debug("fsl_fcm_init: nand->options  =   %08X\n", chip->options);
	pr_debug("fsl_fcm_init: nand->numchips = %10d\n", chip->numchips);
	pr_debug("fsl_fcm_init: nand->chipsize = %10ld\n", chip->chipsize);
	pr_debug("fsl_fcm_init: nand->pagemask = %10X\n", chip->pagemask);
	pr_debug("fsl_fcm_init: nand->chip_delay = %8d\n",
		  chip->chip_delay);
	pr_debug("fsl_fcm_init: nand->badblockpos = %7d\n",
		  chip->badblockpos);
	pr_debug("fsl_fcm_init: nand->chip_shift = %8d\n",
		  chip->chip_shift);
	pr_debug("fsl_fcm_init: nand->page_shift = %8d\n",
		  chip->page_shift);
	pr_debug("fsl_fcm_init: nand->phys_erase_shift = %2d\n",
		  chip->phys_erase_shift);
	pr_debug("fsl_fcm_init: nand->ecclayout= %10p\n", chip->ecclayout);
	pr_debug("fsl_fcm_init: nand->eccmode  = %10d\n", chip->ecc.mode);
	pr_debug("fsl_fcm_init: nand->eccsteps = %10d\n", chip->ecc.steps);
	pr_debug("fsl_fcm_init: nand->eccsize  = %10d\n", chip->ecc.size);
	pr_debug("fsl_fcm_init: nand->eccbytes = %10d\n", chip->ecc.bytes);
	pr_debug("fsl_fcm_init: nand->ecctotal = %10d\n", chip->ecc.total);
	pr_debug("fsl_fcm_init: nand->ecclayout= %10p\n",
		  chip->ecc.layout);
	pr_debug("fsl_fcm_init: mtd->flags     =   %08X\n", mtd->flags);
	pr_debug("fsl_fcm_init: mtd->size      = %10d\n", mtd->size);
	pr_debug("fsl_fcm_init: mtd->erasesize = %10d\n", mtd->erasesize);
	pr_debug("fsl_fcm_init: mtd->writesize = %10d\n", mtd->writesize);
	pr_debug("fsl_fcm_init: mtd->oobsize   = %10d\n", mtd->oobsize);

	/* adjust Option Register and ECC to match Flash page size */
	if (mtd->writesize == 512) {
		u32 tmp;
		tmp = in_be32(&lbc->bank[nmtd->bank].or);
		tmp &= ~(OR_FCM_PGS);
		out_be32(&lbc->bank[nmtd->bank].or, tmp);
	} else if (mtd->writesize == 2048) {
		lbc->bank[nmtd->bank].or |= OR_FCM_PGS;
		/* adjust ecc setup if needed */
		if ((in_be32(&lbc->bank[nmtd->bank].br) & BR_DECC) == BR_DECC_CHK_GEN) {
			chip->ecc.size = 2048;
			chip->ecc.steps = 1;
			chip->ecc.layout = (nmtd->fmr & FMR_ECCM) ?
			    &fsl_fcm_oob_lp_eccm1 : &fsl_fcm_oob_lp_eccm0;
			mtd->ecclayout = chip->ecc.layout;
		}
	} else {
		printk("fsl_fcm_init: page size %d is not supported\n",
		       mtd->writesize);
		return -1;
	}
	nmtd->pgs = (in_be32(&lbc->bank[nmtd->bank].or) >> OR_FCM_PGS_SHIFT) & 1;

	/* fix up the oobavail size in case the layout was changed */
	chip->ecc.layout->oobavail = 0;
	for (i = 0; chip->ecc.layout->oobfree[i].length; i++)
		chip->ecc.layout->oobavail +=
		    chip->ecc.layout->oobfree[i].length;

	/* return to the default bbt_scan_routine */
	chip->scan_bbt = nand_default_bbt;

	/* restore complete options including the real SKIP_BBTSCAN setting */
	chip->options = nmtd->options;

	/* Check, if we should skip the bad block table scan */
	if (chip->options & NAND_SKIP_BBTSCAN)
		return 0;

	return chip->scan_bbt(mtd);
}

/* fsl_fcm_chip_init
 *
 * init a single instance of an chip
*/

static int fsl_fcm_chip_init(struct fsl_elbc_mtd *nmtd)
{
	struct fsl_elbc_ctrl *ctrl;
	volatile lbus83xx_t *lbc;
	struct nand_chip *chip;

	PARAM_VERIFY(nmtd);
	ctrl = nmtd->ctrl;
	PARAM_VERIFY(ctrl);
	lbc = ctrl->regs;
	PARAM_VERIFY(lbc);
	chip = &nmtd->chip;
	PARAM_VERIFY(chip);
	pr_debug("eLBC Set Information for bank %d\n", nmtd->bank);
	pr_debug("  name          = %s\n",
		  nmtd->pl_chip.name ? nmtd->pl_chip.name : "(NULL)");
	pr_debug("  nr_chips      = %d\n", nmtd->pl_chip.nr_chips);
	pr_debug("  partitions    = %s\n",
		  nmtd->pl_chip.partitions_str ?
		  nmtd->pl_chip.partitions_str : "(NULL)");
	dev_dbg(nmtd->device, "eLBC Set Information for bank %d\n", nmtd->bank);
	dev_dbg(nmtd->device, "  name          = %s\n",
		nmtd->name ? nmtd->name : "(NULL)");
	dev_dbg(nmtd->device, "  nr_chips      = %d\n", nmtd->pl_chip.nr_chips);
	dev_dbg(nmtd->device, "  partitions    = %s\n",
		nmtd->pl_chip.partitions_str ?
		nmtd->pl_chip.partitions_str : "(NULL)");

	/* Fill in fsl_elbc_mtd structure */
	nmtd->name = (char *)nmtd->pl_chip.name;
	nmtd->mtd.name = nmtd->name;
	nmtd->mtd.priv = chip;
	nmtd->mtd.owner = THIS_MODULE;
	nmtd->pgs = (in_be32(&lbc->bank[nmtd->bank].or) >> OR_FCM_PGS_SHIFT) & 1;
	nmtd->fmr = 0;		/* rest filled in later */

	/* fill in nand_chip structure */
	/* set physical base address from the Base Register */
	chip->IO_ADDR_W = (void __iomem *)(nmtd->pbase);
	chip->IO_ADDR_R = chip->IO_ADDR_W;

	/* set up function call table */
	chip->read_byte = fsl_fcm_read_byte;
	chip->read_word = fsl_fcm_read_word;
	chip->write_buf = fsl_fcm_write_buf;
	chip->read_buf = fsl_fcm_read_buf;
	chip->verify_buf = fsl_fcm_verify_buf;
	chip->cmd_ctrl  = fsl_fcm_cmd_ctrl;
	chip->cmdfunc = fsl_fcm_cmdfunc;
	chip->waitfunc = fsl_fcm_wait;
	chip->scan_bbt = fsl_fcm_chip_init_tail;

	/* set up nand options */
	chip->options = NAND_NO_READRDY;
	chip->chip_delay = 1;

	chip->controller = &ctrl->controller;
	chip->priv = nmtd;

	/* If CS Base Register selects full hardware ECC then use it */
	if ((in_be32(&lbc->bank[nmtd->bank].br) & BR_DECC) == BR_DECC_CHK_GEN) {
		chip->ecc.mode = NAND_ECC_HW;
		chip->ecc.calculate = fsl_fcm_calculate_ecc;
		chip->ecc.correct = fsl_fcm_correct_data;
		chip->ecc.hwctl = fsl_fcm_enable_hwecc;
		/* put in small page settings and adjust later if needed */
		chip->ecc.layout = (nmtd->fmr & FMR_ECCM) ?
		    &fsl_fcm_oob_sp_eccm1 : &fsl_fcm_oob_sp_eccm0;
		chip->ecc.size = 512;
		chip->ecc.bytes = 3;
	} else {
		/* otherwise fall back to default software ECC */
		chip->ecc.mode = NAND_ECC_SOFT;
	}

	/* force BBT scan to get to custom scan_bbt for final settings */
	nmtd->options = chip->options;
	chip->options &= ~(NAND_SKIP_BBTSCAN);

	return 0;
}

static int fsl_fcm_chip_remove(struct platform_device *pdev)
{
	struct fsl_elbc_mtd *nmtd = platform_get_drvdata(pdev);
	struct fsl_elbc_ctrl *ctrl;

	PARAM_VERIFY(nmtd);
	ctrl = nmtd->ctrl;
	PARAM_VERIFY(ctrl);

	nand_release(&nmtd->mtd);

	if (nmtd->vbase != 0) {
		iounmap((void __iomem *)nmtd->vbase);
		nmtd->vbase = 0;
	}

	platform_set_drvdata(pdev, NULL);

	ctrl->nmtd[nmtd->bank] = NULL;
	atomic_dec(&ctrl->childs_active);

	kfree(nmtd);

	return 0;
}

static int fsl_fcm_chip_probe(struct platform_device *pdev)
{
	struct platform_fsl_nand_chip *pnc;
	struct fsl_elbc_ctrl *ctrl = &fcm_ctrl;
	volatile lbus83xx_t *lbc = ctrl->regs;
	struct fsl_elbc_mtd *nmtd;
	struct resource *res;
	int err = 0;
	int size;
	int bank;

	dev_dbg(&pdev->dev, "fsl_fcm_chip_probe(%p)\n", pdev);
	PARAM_VERIFY(pdev);
	PARAM_VERIFY(lbc);

	pnc = pdev->dev.platform_data;
	/* check that the platform data structure was supplied */
	if (pnc == NULL) {
		dev_err(&pdev->dev, "Device needs a platform data structure\n");
		err = -ENOENT;
		goto exit_error;
	}

	/* check that the device has a name */
	if (pnc->name == NULL) {
		dev_err(&pdev->dev, "Device requires a name\n");
		err = -ENOENT;
		goto exit_error;
	}
	/* get, allocate and map the memory resource */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		dev_err(&pdev->dev, "failed to get memory region resource\n");
		err = -ENOENT;
		goto exit_error;
	}
	/* find which chip select it is connected to */
	for (bank = 0; bank < MAX_BANKS; bank++)
		if ((in_be32(&lbc->bank[bank].br) & BR_V) &&
		    ((in_be32(&lbc->bank[bank].br) & BR_MSEL) == BR_MS_FCM) &&
		    ((in_be32(&lbc->bank[bank].br) & in_be32(&lbc->bank[bank].or) & BR_BA) ==
		     res->start))
			break;

	if (bank >= MAX_BANKS) {
		dev_err(&pdev->dev, "address did not match any chip selects\n");
		err = -ENOENT;
		goto exit_error;
	}

	nmtd = kmalloc(sizeof(*nmtd), GFP_KERNEL);
	if (!nmtd) {
		dev_err(ctrl->device, "no memory for nand chip structure\n");
		err = -ENOMEM;
		goto exit_error;
	}
	memset(nmtd, 0, sizeof(*nmtd));

	platform_set_drvdata(pdev, nmtd);

	atomic_inc(&ctrl->childs_active);
	if (pnc)
		memcpy(&(nmtd->pl_chip), pnc, sizeof(*pnc));
	ctrl->nmtd[bank] = nmtd;
	nmtd->bank = bank;
	nmtd->ctrl = ctrl;
	nmtd->device = &pdev->dev;

	size = (res->end - res->start) + 1;
	nmtd->pbase = res->start;
	nmtd->vbase = (unsigned int)ioremap(nmtd->pbase, FCM_SIZE);
	if (nmtd->vbase == 0) {
		dev_err(ctrl->device, "failed to ioremap() memory region\n");
		err = -EIO;
		goto free_error;
	}

	err = fsl_fcm_chip_init(nmtd);
	if (err != 0)
		goto unmap_error;

	err = nand_scan(&nmtd->mtd,
			nmtd->pl_chip.nr_chips ? nmtd->pl_chip.nr_chips : 1);
	if (err != 0)
		goto unmap_error;

	err = add_mtd_device(&nmtd->mtd);

	if (err == 0)
		return 0;

unmap_error:
	iounmap((volatile void __iomem *)nmtd->vbase);
free_error:
	kfree(nmtd);
exit_error:
	return err;
}

/**************************************************************************/
/*                  Controller setup and control functions                */
/**************************************************************************/

static int fsl_fcm_ctrl_init(struct fsl_elbc_ctrl *ctrl,
			      struct platform_device *pdev)
{
	lbus83xx_t *lbc;
	u32 tmp;

	PARAM_VERIFY(ctrl);
	lbc = (lbus83xx_t *) ctrl->regs;
	PARAM_VERIFY(lbc);
	/* Enable only FCM detection of timeouts, ECC errors and completion */
	out_be32(&lbc->ltedr, ~(LTESR_FCT | LTESR_PAR | LTESR_CC));

	/* clear event registers */
	out_be32(&lbc->lteatr, 0);
	tmp = in_be32(&lbc->ltesr);
	out_be32(&lbc->ltesr, tmp | (LTESR_FCT | LTESR_PAR | LTESR_CC));

	/* Enable interrupts for any detected events */
	out_be32(&lbc->lteir, 0xffffffff);
	ctrl->read_bytes = 0;
	ctrl->index = 0;
	ctrl->addr = (unsigned char *)(NULL);
	ctrl->oobbuf = -1;

	return 0;
}

static int fsl_fcm_ctrl_remove(struct platform_device *pdev)
{
	struct fsl_elbc_ctrl *ctrl;

	PARAM_VERIFY(pdev);
	ctrl = platform_get_drvdata(pdev);
	PARAM_VERIFY(ctrl);

	if (atomic_read(&ctrl->childs_active))
		return -EBUSY;

	free_irq(ctrl->irq, pdev);
	iounmap(ctrl->regs);

	platform_set_drvdata(pdev, NULL);
	memset(ctrl, 0, sizeof(*ctrl));

	return 0;
}

/* interrupt handler code */

static irqreturn_t fsl_fcm_ctrl_irq(int irqno, void *param, struct pt_regs *ptrg)
{
	struct fsl_elbc_ctrl *ctrl =
	    platform_get_drvdata((struct platform_device *)param);
	lbus83xx_t *lbc = (lbus83xx_t *) ctrl->regs;
	u32 tmp;

	ctrl->irq_status = in_be32(&lbc->ltesr) & (LTESR_FCT | LTESR_PAR | LTESR_CC);
	if (ctrl->irq_status)
		wake_up(&ctrl->irq_wait);

	/* clear event registers */
	out_be32(&lbc->lteatr, 0);
	tmp = in_be32(&lbc->ltesr);
	out_be32(&lbc->ltesr, tmp | ctrl->irq_status);
	return IRQ_HANDLED;
}

/* fsl_fcm_ctrl_probe
 *
 * called by device layer when it finds a device matching
 * one our driver can handled. This code allocates all of
 * the resources needed for the controller only.  The
 * resources for the NAND banks themselves are allocated
 * in the chip probe function.
*/

static int fsl_fcm_ctrl_probe(struct platform_device *pdev)
{
	struct fsl_elbc_ctrl *ctrl;
	struct resource *res;
	int err = 0;
	int size;
	int ret;

	dev_dbg(&pdev->dev, "fsl_fcm_ctrl_probe(%p)\n", pdev);
	ctrl = &fcm_ctrl;

	memset(ctrl, 0, sizeof(*ctrl));

	spin_lock_init(&ctrl->controller.lock);
	init_waitqueue_head(&ctrl->controller.wq);
	init_waitqueue_head(&ctrl->irq_wait);

	/* get, allocate and map the memory resource */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		dev_err(&pdev->dev, "failed to get memory region resouce\n");
		err = -ENOENT;
		goto exit_error;
	}

	size = (res->end - res->start) + 1;
	ctrl->regs = ioremap(res->start, size);
	if (ctrl->regs == 0) {
		dev_err(&pdev->dev, "failed to ioremap() region\n");
		err = -EIO;
		goto exit_error;
	}

	/* get and allocate the irq resource */
	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (res == NULL) {
		dev_err(&pdev->dev, "failed to get irq resource\n");
		err = -ENOENT;
		goto unmap;
	}

	ret = request_irq(res->start, fsl_fcm_ctrl_irq, IRQF_DISABLED, pdev->name, pdev);
	if (ret != 0) {
		dev_err(&pdev->dev, "failed to install irq (%d)\n", ret);
		err = -EIO;
		goto unmap;
	}

	ctrl->irq = res->start;
	ctrl->device = &pdev->dev;
	dev_dbg(&pdev->dev, "mapped registers at %p\n", ctrl->regs);

	/* initialise the hardware */

	err = fsl_fcm_ctrl_init(ctrl, pdev);
	if (err)
		goto free_irq;
	platform_set_drvdata(pdev, ctrl);
	return 0;

free_irq:
	free_irq(ctrl->irq, pdev);
unmap:
	iounmap(ctrl->regs);
exit_error:
	return err;
}

/*************************************************************************/
/*                        device driver registration                     */
/*************************************************************************/

static struct platform_driver fsl_fcm_ctrl_driver = {
	.probe = fsl_fcm_ctrl_probe,
	.remove = fsl_fcm_ctrl_remove,
	.driver = {
		   .name = "fsl-fcm",
		   .owner = THIS_MODULE,
		   },
};

static struct platform_driver fsl_fcm_chip_driver = {
	.probe = fsl_fcm_chip_probe,
	.remove = fsl_fcm_chip_remove,
	.driver = {
		   .name = "fsl-nand",
		   .owner = THIS_MODULE,
		   },
};

static int __init fsl_fcm_init(void)
{
	int ret;

	printk(KERN_INFO "Freescale eLBC FCM NAND Driver (C) 2006 Freescale\n");

	ret = platform_driver_register(&fsl_fcm_ctrl_driver);
	if (!ret)
		ret = platform_driver_register(&fsl_fcm_chip_driver);

	return ret;
}

static void __exit fsl_fcm_exit(void)
{
	platform_driver_unregister(&fsl_fcm_chip_driver);
	platform_driver_unregister(&fsl_fcm_ctrl_driver);
}

module_init(fsl_fcm_init);
module_exit(fsl_fcm_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Freescale");
MODULE_DESCRIPTION("Freescale Enhanced Local Bus Controller MTD NAND driver");
