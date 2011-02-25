/*
 * NAND driver for TI DaVinci based boards.
 *
 * Copyright (C) 2007 Sergey Kubushyn <ksi@koi8.net>
 *
 * Based on Linux DaVinci NAND driver by TI. Original copyright follows:
 */

/*
 *
 * linux/drivers/mtd/nand/nand_davinci.c
 *
 * NAND Flash Driver
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
 *
 *  Overview:
 *   This is a device driver for the NAND flash device found on the
 *   DaVinci board which utilizes the Samsung k9k2g08 part.
 *
 Modifications:
 ver. 1.0: Feb 2005, Vinod/Sudhakar
 -
 *
 */

#include <common.h>
#include <asm/io.h>

#ifdef CFG_USE_NAND
#if !defined(CONFIG_NAND_LEGACY)

#include <nand.h>
#include <asm/arch/nand_defs.h>
#include <asm/arch/emif_defs.h>

/* Definitions for 4-bit hardware ECC */
#define NAND_STATUS_RETRY		5
#define NAND_TIMEOUT			10240
#define NAND_ECC_BUSY			0xC
#define NAND_4BITECC_MASK		0x03FF03FF
#define EMIF_NANDFSR_ECC_STATE_MASK  	0x00000F00
#define ECC_STATE_NO_ERR		0x0
#define ECC_STATE_TOO_MANY_ERRS		0x1
#define ECC_STATE_ERR_CORR_COMP_P	0x2
#define ECC_STATE_ERR_CORR_COMP_N	0x3
#define ECC_MAX_CORRECTABLE_ERRORS	0x4

static u_char davinci_ecc_buf[NAND_MAX_OOBSIZE];

extern struct nand_chip nand_dev_desc[CFG_MAX_NAND_DEVICE];

static void nand_davinci_hwcontrol(struct mtd_info *mtd, int cmd, unsigned int ctrl)
{
	struct		nand_chip *this = mtd->priv;
	u_int32_t	IO_ADDR_W = (u_int32_t)this->IO_ADDR_W;

	IO_ADDR_W &= ~(MASK_ALE|MASK_CLE);

	if (ctrl & NAND_CTRL_CHANGE) {
		if ( ctrl & NAND_CLE )
			IO_ADDR_W |= MASK_CLE;
		if ( ctrl & NAND_ALE )
			IO_ADDR_W |= MASK_ALE;
		this->IO_ADDR_W = (void __iomem *) IO_ADDR_W;
	}

	if (cmd != NAND_CMD_NONE)
		writeb(cmd, this->IO_ADDR_W);
}

/* Set WP on deselect, write enable on select */
static void nand_davinci_select_chip(struct mtd_info *mtd, int chip)
{
#define GPIO_SET_DATA01	0x01c67018
#define GPIO_CLR_DATA01	0x01c6701c
#define GPIO_NAND_WP	(1 << 4)
#ifdef SONATA_BOARD_GPIOWP
	if (chip < 0) {
		REG(GPIO_CLR_DATA01) |= GPIO_NAND_WP;
	} else {
		REG(GPIO_SET_DATA01) |= GPIO_NAND_WP;
	}
#endif
}

#ifdef CFG_DM365_IPNC
static uint8_t scan_ff_pattern[] = { 0xff, 0xff };
/*
 * We should always have a flash-based bad block table.  However, if one isn't
 * found then all blocks will be scanned to look for factory-marked bad blocks.
 * We supply a null pattern so that no blocks will be detected as bad.
 */
static struct nand_bbt_descr nand_dm365_hw10_512_badblock_pattern = {
  .options = 0, //NAND_BBT_SCANEMPTY | NAND_BBT_SCANALLPAGES,
	.offs = 5,
	.len = 1,
	.pattern = scan_ff_pattern
};

#endif

#ifdef CFG_NAND_HW_ECC
#ifdef CFG_NAND_4BIT_ECC
/* flash bbt decriptors */
static uint8_t nand_davinci_bbt_pattern[] = { 'B', 'b', 't', '0' };
static uint8_t nand_davinci_mirror_pattern[] = { '1', 't', 'b', 'B' };

static struct nand_bbt_descr nand_davinci_bbt_main_descr = {
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
	| NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_PERCHIP,
	.offs = 2,
	.len = 4,
	.veroffs = 16,
	.maxblocks = 4,
	.pattern = nand_davinci_bbt_pattern
};

static struct nand_bbt_descr nand_davinci_bbt_mirror_descr = {
	.options = NAND_BBT_LASTBLOCK | NAND_BBT_CREATE | NAND_BBT_WRITE
	| NAND_BBT_2BIT | NAND_BBT_VERSION | NAND_BBT_PERCHIP,
	.offs = 2,
	.len = 4,
	.veroffs = 16,
	.maxblocks = 4,
	.pattern = nand_davinci_mirror_pattern
};

static struct nand_ecclayout nand_davinci_4bit_layout = {
	.eccbytes = 10,
	.eccpos = {6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
		   22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
		   38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
		   54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
		   70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
		   86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
		   102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
		   118, 119, 120, 121, 122, 123, 124, 125, 126, 127
		},
	.oobfree = {
		{.offset = 0, .length = 6},
		{.offset = 16, .length = 6},
		{.offset = 32, .length = 6},
		{.offset = 48, .length = 6},
		{.offset = 64, .length = 6},
		{.offset = 80, .length = 6},
		{.offset = 96, .length = 6},
		{.offset = 112, .length = 6},
		}


};
#else
#ifdef CFG_NAND_LARGEPAGE
static struct nand_ecclayout davinci_nand_ecclayout = {
	.eccbytes = 12,
	.eccpos = {8, 9, 10, 24, 25, 26, 40, 41, 42, 56, 57, 58},
	.oobfree = {
		{.offset = 2, .length = 6},
		{.offset = 12, .length = 12},
		{.offset = 28, .length = 12},
		{.offset = 44, .length = 12},
		{.offset = 60, .length = 4}
	}
};
#elif defined(CFG_NAND_SMALLPAGE)
static struct nand_ecclayout davinci_nand_ecclayout = {
	.eccbytes = 3,
	.eccpos = {0, 1, 2},
	.oobfree = {
		{.offset = 6, .length = 2},
		{.offset = 8, .length = 8}
	}
};
#else
#error "Either CFG_NAND_LARGEPAGE or CFG_NAND_SMALLPAGE must be defined!"
#endif
#endif

#ifdef CFG_NAND_4BIT_ECC
/*
 * When using 4-bit ECC with a 2048-byte data + 64-byte spare page size, the
 * oob is scattered throughout the page in 4 16-byte chunks instead of being
 * grouped together at the end of the page.  This means that the factory
 * bad-block markers at offsets 2048 and 2049 will be overwritten when data
 * is written to the flash.  Thus, we cannot use the factory method to mark
 * or detect bad blocks and must rely on a flash-based bad block table instead.
 *
 */

static void nand_davinci_4bit_enable_hwecc(struct mtd_info *mtd, int mode)
{
	u32 val;
	emifregs emif_addr;
	emif_addr = (emifregs)DM3XX_NAND_BASE;

	switch (mode) {
	case NAND_ECC_WRITE:
	case NAND_ECC_READ:
		/*
		 * Start a new ECC calculation for reading or writing 512 bytes
		 *  of data.
		 */
		val = (emif_addr->NANDFCR & ~(3 << 4)) | (1 << 12);
		emif_addr->NANDFCR = val;
		break;
	case NAND_ECC_READSYN:
		val = emif_addr->NAND4BITECC1;
		break;
	default:
		break;
	}
}

static u32 nand_davinci_4bit_readecc(struct mtd_info *mtd, unsigned int ecc[4])
{
	emifregs emif_addr;

	emif_addr = (emifregs)DM3XX_NAND_BASE;

	ecc[0] = emif_addr->NAND4BITECC1 & NAND_4BITECC_MASK;
	ecc[1] = emif_addr->NAND4BITECC2 & NAND_4BITECC_MASK;
	ecc[2] = emif_addr->NAND4BITECC3 & NAND_4BITECC_MASK;
	ecc[3] = emif_addr->NAND4BITECC4 & NAND_4BITECC_MASK;

	return 0;
}

static int nand_davinci_4bit_calculate_ecc(struct mtd_info *mtd,
					   const uint8_t *dat,
					   uint8_t *ecc_code)
{
	unsigned int hw_4ecc[4] = { 0, 0, 0, 0 };
	unsigned int const1 = 0, const2 = 0;
	unsigned char count1 = 0;

	/*
	 * Since the NAND_HWECC_SYNDROME option is enabled, this routine is
	 * only called just after the data and oob have been written.  The
	 * ECC value calculated by the hardware ECC generator is available
	 * for us to read.
	 */
	nand_davinci_4bit_readecc(mtd, hw_4ecc);

	/*Convert 10 bit ecc value to 8 bit */
	for (count1 = 0; count1 < 2; count1++) {
		const2 = count1 * 5;
		const1 = count1 * 2;

		/* Take first 8 bits from val1 (count1=0) or val5 (count1=1) */
		ecc_code[const2] = hw_4ecc[const1] & 0xFF;

		/*
		 * Take 2 bits as LSB bits from val1 (count1=0) or val5
		 * (count1=1) and 6 bits from val2 (count1=0) or val5 (count1=1)
		 */
		ecc_code[const2 + 1] =
		    ((hw_4ecc[const1] >> 8) & 0x3) | ((hw_4ecc[const1] >> 14) &
						      0xFC);

		/*
		 * Take 4 bits from val2 (count1=0) or val5 (count1=1) and
		 * 4 bits from val3 (count1=0) or val6 (count1=1)
		 */
		ecc_code[const2 + 2] =
		    ((hw_4ecc[const1] >> 22) & 0xF) |
		    ((hw_4ecc[const1 + 1] << 4) & 0xF0);

		/*
		 * Take 6 bits from val3(count1=0) or val6 (count1=1) and
		 * 2 bits from val4 (count1=0) or  val7 (count1=1)
		 */
		ecc_code[const2 + 3] =
		    ((hw_4ecc[const1 + 1] >> 4) & 0x3F) |
		    ((hw_4ecc[const1 + 1] >> 10) & 0xC0);

		/* Take 8 bits from val4 (count1=0) or val7 (count1=1) */
		ecc_code[const2 + 4] = (hw_4ecc[const1 + 1] >> 18) & 0xFF;
	}
	return 0;
}

static int nand_davinci_4bit_compare_ecc(struct mtd_info *mtd,
					 uint8_t *read_ecc, /* read from NAND */
					 uint8_t *page_data)
{
	struct nand_chip *this = mtd->priv;
	unsigned short ecc_10bit[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	int i;
	unsigned int hw_4ecc[4] = { 0, 0, 0, 0 }, iserror = 0;
	unsigned short *pspare = NULL, *pspare1 = NULL;
	unsigned int numErrors, errorAddress, errorValue;
	emifregs emif_addr;
	u32 val;

	emif_addr = (emifregs)DM3XX_NAND_BASE;

	/*
	 * Check for an ECC where all bytes are 0xFF.  If this is the case, we
	 * will assume we are looking at an erased page and we should ignore the
	 * ECC.
	 */
	for (i = 0; i < 10; i++) {
		if (read_ecc[i] != 0xFF)
			break;
	}
	if (i == 10)
		return 0;

	/* Convert 8 bit in to 10 bit */
	pspare = (unsigned short *)&read_ecc[2];
	pspare1 = (unsigned short *)&read_ecc[0];
	/* Take 10 bits from 0th and 1st bytes */
	ecc_10bit[0] = (*pspare1) & 0x3FF;	/* 10 */
	/* Take 6 bits from 1st byte and 4 bits from 2nd byte */
	ecc_10bit[1] = (((*pspare1) >> 10) & 0x3F)
	    | (((pspare[0]) << 6) & 0x3C0);	/* 6 + 4 */
	/* Take 4 bits form 2nd bytes and 6 bits from 3rd bytes */
	ecc_10bit[2] = ((pspare[0]) >> 4) & 0x3FF;	/* 10 */
	/*Take 2 bits from 3rd byte and 8 bits from 4th byte */
	ecc_10bit[3] = (((pspare[0]) >> 14) & 0x3)
	    | ((((pspare[1])) << 2) & 0x3FC);	/* 2 + 8 */
	/* Take 8 bits from 5th byte and 2 bits from 6th byte */
	ecc_10bit[4] = ((pspare[1]) >> 8)
	    | ((((pspare[2])) << 8) & 0x300);	/* 8 + 2 */
	/* Take 6 bits from 6th byte and 4 bits from 7th byte */
	ecc_10bit[5] = (pspare[2] >> 2) & 0x3FF;	/* 10 */
	/* Take 4 bits from 7th byte and 6 bits from 8th byte */
	ecc_10bit[6] = (((pspare[2]) >> 12) & 0xF)
	    | ((((pspare[3])) << 4) & 0x3F0);	/* 4 + 6 */
	/*Take 2 bits from 8th byte and 8 bits from 9th byte */
	ecc_10bit[7] = ((pspare[3]) >> 6) & 0x3FF;	/* 10 */

	/*
	 * Write the parity values in the NAND Flash 4-bit ECC Load register.
	 * Write each parity value one at a time starting from 4bit_ecc_val8
	 * to 4bit_ecc_val1.
	 */
	for (i = 7; i >= 0; i--)
		emif_addr->NAND4BITECCLOAD = ecc_10bit[i];

	/*
	 * Perform a dummy read to the EMIF Revision Code and Status register.
	 * This is required to ensure time for syndrome calculation after
	 * writing the ECC values in previous step.
	 */

	val = emif_addr->NANDFSR;

	/*
	 * Read the syndrome from the NAND Flash 4-Bit ECC 1-4 registers.
	 * A syndrome value of 0 means no bit errors. If the syndrome is
	 * non-zero then go further otherwise return.
	 */
	nand_davinci_4bit_readecc(mtd, hw_4ecc);

	if (hw_4ecc[0] == ECC_STATE_NO_ERR && hw_4ecc[1] == ECC_STATE_NO_ERR &&
	    hw_4ecc[2] == ECC_STATE_NO_ERR && hw_4ecc[3] == ECC_STATE_NO_ERR)
		return 0;

	/*
	 * Clear any previous address calculation by doing a dummy read of an
	 * error address register.
	 */
	val = emif_addr->NANDERRADD1;

	/*
	 * Set the addr_calc_st bit(bit no 13) in the NAND Flash Control
	 * register to 1.
	 */
	emif_addr->NANDFCR |= 1 << 13;

	i = 20;
	do {
		val = emif_addr->NANDFSR;
		val &= EMIF_NANDFSR_ECC_STATE_MASK;
              val = val >> 8;
              i--;
	} while((i>0) && (val<4));


	/*
	 * Wait for the corr_state field (bits 8 to 11)in the
	 * NAND Flash Status register to be equal to 0x0, 0x1, 0x2, or 0x3.
	 */
	i = NAND_TIMEOUT;
	do {
        	val = emif_addr->NANDFSR;
		val &= 0xc00;
		i--;
    	} while ((i > 0) && val);

	iserror = emif_addr->NANDFSR;
	iserror &= EMIF_NANDFSR_ECC_STATE_MASK;
	iserror = iserror >> 8;


	/*
	 * ECC_STATE_TOO_MANY_ERRS (0x1) means errors cannot be
	 * corrected (five or more errors).  The number of errors
	 * calculated (err_num field) differs from the number of errors
	 * searched.  ECC_STATE_ERR_CORR_COMP_P (0x2) means error
	 * correction complete (errors on bit 8 or 9).
	 * ECC_STATE_ERR_CORR_COMP_N (0x3) means error correction
	 * complete (error exists).
	 */

	for ( i = 0;  i < 1 ; i++)
	{
	udelay (this->chip_delay);
	udelay (this->chip_delay);
	udelay (this->chip_delay);
	udelay (this->chip_delay);
	}

	if (iserror == ECC_STATE_NO_ERR) {
		/* dummy read for correct ECC operation, per TI Errata 1.2.4 */
		val = emif_addr->NANDERRVAL1;
		return 0;
	}
	else if (iserror == ECC_STATE_TOO_MANY_ERRS) {
		/* dummy read for correct ECC operation, per TI Errata 1.2.4 */
		val = emif_addr->NANDERRVAL1;
		return -1;
	}

	numErrors = ((emif_addr->NANDFSR >> 16) & 0x3) + 1;

	/* Read the error address, error value and correct */
	for (i = 0; i < numErrors; i++) {
		if (i > 1) {
			errorAddress =
			    ((emif_addr->NANDERRADD2 >>
			      (16 * (i & 1))) & 0x3FF);
			errorAddress = ((512 + 7) - errorAddress);
			errorValue =
			    ((emif_addr->NANDERRVAL2 >>
			      (16 * (i & 1))) & 0xFF);
		} else {
			errorAddress =
			    ((emif_addr->NANDERRADD1 >>
			      (16 * (i & 1))) & 0x3FF);
			errorAddress = ((512 + 7) - errorAddress);
			errorValue =
			    ((emif_addr->NANDERRVAL1 >>
			      (16 * (i & 1))) & 0xFF);
		}
		/* xor the corrupt data with error value */
		if (errorAddress < 512)
			page_data[errorAddress] ^= errorValue;
	}

	return numErrors;
}

static int nand_davinci_4bit_correct_data(struct mtd_info *mtd, uint8_t *dat,
					  uint8_t *read_ecc, uint8_t *calc_ecc)
{
	int r;

	/*
	 * dat points to 512 bytes of data.  read_ecc points to the start of the
	 * ECC code exactly...  The calc_ecc pointer is not needed since
	 * our caclulated ECC is already latched in the hardware ECC generator.
	 */
	r = nand_davinci_4bit_compare_ecc(mtd, read_ecc, dat);

	return r;
}

static int
davinci_read_page_syndrome(struct mtd_info *mtd, struct nand_chip *nand,
				   uint8_t *buf, int page)
{
	int i, eccsize = nand->ecc.size;
	int eccbytes = nand->ecc.bytes;
	int eccsteps = nand->ecc.steps;
	uint8_t *p = buf;
	uint8_t *oob = nand->oob_poi;

	for (i = 0; eccsteps; eccsteps--, i += eccbytes, p += eccsize) {
		int stat;

		nand->ecc.hwctl(mtd, NAND_ECC_READ);
		nand->read_buf(mtd, p, eccsize);

		nand->ecc.hwctl(mtd, NAND_ECC_READSYN);

		if (nand->ecc.prepad) {
			nand->read_buf(mtd, oob, nand->ecc.prepad);
			oob += nand->ecc.prepad;
		}

		nand->read_buf(mtd, oob, eccbytes);
		stat = nand->ecc.correct(mtd, p, oob, NULL);

		if (stat == -1)
			mtd->ecc_stats.failed++;
		else
			mtd->ecc_stats.corrected += stat;

		oob += eccbytes;

		if (nand->ecc.postpad) {
			nand->read_buf(mtd, oob, nand->ecc.postpad);
			oob += nand->ecc.postpad;
		}
	}

	/* Calculate remaining oob bytes */
	i = mtd->oobsize - (oob - nand->oob_poi);
	if (i)
		nand->read_buf(mtd, oob, i);

	return 0;
}

static void davinci_write_page_syndrome(struct mtd_info *mtd,
				    struct nand_chip *nand, const uint8_t *buf)
{
	int i, eccsize = nand->ecc.size;
	int eccbytes = nand->ecc.bytes;
	int eccsteps = nand->ecc.steps;
	const uint8_t *p = buf;
	uint8_t *oob = nand->oob_poi;

	for (i = 0; eccsteps; eccsteps--, i += eccbytes, p += eccsize) {

		nand->ecc.hwctl(mtd, NAND_ECC_WRITE);
		nand->write_buf(mtd, p, eccsize);

		/* Calculate ECC without prepad */
		nand->ecc.calculate(mtd, p, oob + nand->ecc.prepad);

		if (nand->ecc.prepad) {
			nand->write_buf(mtd, oob, nand->ecc.prepad);
			oob += nand->ecc.prepad;
		}
		nand->write_buf(mtd, oob, eccbytes);
		oob += eccbytes;

		if (nand->ecc.postpad) {
			nand->write_buf(mtd, oob, nand->ecc.postpad);
			oob += nand->ecc.postpad;
		}
	}

	/* Calculate remaining oob bytes */
	i = mtd->oobsize - (oob - nand->oob_poi);
	if (i)
		nand->write_buf(mtd, oob, i);
}

static int
davinci_std_read_page_syndrome(struct mtd_info *mtd, struct nand_chip *chip,
				   uint8_t *buf, int page)
{
	struct nand_chip *this = mtd->priv;
	int i, eccsize = chip->ecc.size;
	int eccbytes = chip->ecc.bytes;
	int eccsteps = chip->ecc.steps;
	uint8_t *p = buf;
	uint8_t *oob = chip->oob_poi;

	chip->cmdfunc (mtd, NAND_CMD_READOOB, 0x0, page & this->pagemask);

	chip->read_buf(mtd, oob, mtd->oobsize);

	chip->cmdfunc (mtd, NAND_CMD_READ0, 0x0, page & this->pagemask);


	for (i = 0; eccsteps; eccsteps--, i += eccbytes, p += eccsize) {
		int stat;

		chip->ecc.hwctl(mtd, NAND_ECC_READ);
		chip->read_buf(mtd, p, eccsize);

		chip->ecc.hwctl(mtd, NAND_ECC_READSYN);

		if (chip->ecc.prepad)
			oob += chip->ecc.prepad;

		stat = chip->ecc.correct(mtd, p, oob, NULL);

		if (stat == -1)
			mtd->ecc_stats.failed++;
		else
			mtd->ecc_stats.corrected += stat;

		oob += eccbytes;

		if (chip->ecc.postpad) {
			oob += chip->ecc.postpad;
		}
	}

	/* Calculate remaining oob bytes */
	i = mtd->oobsize - (oob - chip->oob_poi);
	if (i)
		chip->read_buf(mtd, oob, i);

	return 0;
}

static void davinci_std_write_page_syndrome(struct mtd_info *mtd,
				    struct nand_chip *chip, const uint8_t *buf)
{
	struct nand_chip *this = mtd->priv;
	int i, eccsize = chip->ecc.size;
	int eccbytes = chip->ecc.bytes;
	int eccsteps = chip->ecc.steps;
	int chunk = chip->ecc.bytes + chip->ecc.prepad + chip->ecc.postpad;
	int offset = 0;
	const uint8_t *p = buf;
	uint8_t *oob = chip->oob_poi;

	for (i = 0; eccsteps; eccsteps--, i += eccbytes, p += eccsize) {

		chip->ecc.hwctl(mtd, NAND_ECC_WRITE);
		chip->write_buf(mtd, p, eccsize);

		/* Calculate ECC without prepad */
		chip->ecc.calculate(mtd, p, oob + chip->ecc.prepad);

		if (chip->ecc.prepad) {
			offset = ((chip->ecc.steps - eccsteps) * chunk);
			memcpy(&davinci_ecc_buf[offset], oob, chip->ecc.prepad);
			oob += chip->ecc.prepad;
		}


		offset = (((chip->ecc.steps - eccsteps) * chunk) + chip->ecc.prepad);
		memcpy(&davinci_ecc_buf[offset], oob, eccbytes);
		oob += eccbytes;

		if (chip->ecc.postpad) {
			offset = (((chip->ecc.steps - eccsteps) * chunk) +
				  (chip->ecc.prepad + eccbytes));
			memcpy(&davinci_ecc_buf[offset], oob, chip->ecc.postpad);
			oob += chip->ecc.postpad;
		}
	}

	/* Write the sparebytes into the page once
	 * all eccsteps have been covered
	 */
	for (i = 0; i < mtd->oobsize; i++)
		writeb(davinci_ecc_buf[i], this->IO_ADDR_W);

	/* Calculate remaining oob bytes */
	i = mtd->oobsize - (oob - chip->oob_poi);
	if (i)
		chip->write_buf(mtd, oob, i);
}

static int davinci_std_write_oob_syndrome(struct mtd_info *mtd,
				   struct nand_chip *chip, int page)
{
	int pos, status = 0;
	const uint8_t *bufpoi = chip->oob_poi;

	pos = mtd->writesize;

	chip->cmdfunc(mtd, NAND_CMD_SEQIN, pos, page);

	chip->write_buf(mtd, bufpoi, mtd->oobsize);

	chip->cmdfunc(mtd, NAND_CMD_PAGEPROG, -1, -1);
	status = chip->waitfunc(mtd, chip);

	return status & NAND_STATUS_FAIL ? -1 : 0;
}

static int davinci_std_read_oob_syndrome(struct mtd_info *mtd, struct nand_chip *chip,
				  int page, int sndcmd)
{
	struct nand_chip *this = mtd->priv;
	uint8_t *buf = chip->oob_poi;
	uint8_t *bufpoi = buf;

	chip->cmdfunc (mtd, NAND_CMD_READOOB, 0x0, page & this->pagemask);

	chip->read_buf(mtd, bufpoi, mtd->oobsize);

	return 1;
}

#endif

static void nand_davinci_enable_hwecc(struct mtd_info *mtd, int mode)
{
	emifregs	emif_addr;
	int		dummy;

	emif_addr = (emifregs)DAVINCI_ASYNC_EMIF_CNTRL_BASE;

	dummy = emif_addr->NANDF1ECC;
	dummy = emif_addr->NANDF2ECC;
	dummy = emif_addr->NANDF3ECC;
	dummy = emif_addr->NANDF4ECC;

	emif_addr->NANDFCR |= (1 << 8);
}

static u_int32_t nand_davinci_readecc(struct mtd_info *mtd, u_int32_t region)
{
	u_int32_t	ecc = 0;
	emifregs	emif_base_addr;

	emif_base_addr = (emifregs)DAVINCI_ASYNC_EMIF_CNTRL_BASE;

	if (region == 1)
		ecc = emif_base_addr->NANDF1ECC;
	else if (region == 2)
		ecc = emif_base_addr->NANDF2ECC;
	else if (region == 3)
		ecc = emif_base_addr->NANDF3ECC;
	else if (region == 4)
		ecc = emif_base_addr->NANDF4ECC;

	return(ecc);
}

static int nand_davinci_calculate_ecc(struct mtd_info *mtd, const u_char *dat, u_char *ecc_code)
{
	u_int32_t		tmp;
	int			region, n;
	struct nand_chip	*this = mtd->priv;

	n = (this->ecc.size/512);

	region = 1;
	while (n--) {
		tmp = nand_davinci_readecc(mtd, region);
		*ecc_code++ = tmp;
		*ecc_code++ = tmp >> 16;
		*ecc_code++ = ((tmp >> 8) & 0x0f) | ((tmp >> 20) & 0xf0);
		region++;
	}
	return(0);
}

static void nand_davinci_gen_true_ecc(u_int8_t *ecc_buf)
{
	u_int32_t	tmp = ecc_buf[0] | (ecc_buf[1] << 16) | ((ecc_buf[2] & 0xf0) << 20) | ((ecc_buf[2] & 0x0f) << 8);

	ecc_buf[0] = ~(P64o(tmp) | P64e(tmp) | P32o(tmp) | P32e(tmp) | P16o(tmp) | P16e(tmp) | P8o(tmp) | P8e(tmp));
	ecc_buf[1] = ~(P1024o(tmp) | P1024e(tmp) | P512o(tmp) | P512e(tmp) | P256o(tmp) | P256e(tmp) | P128o(tmp) | P128e(tmp));
	ecc_buf[2] = ~( P4o(tmp) | P4e(tmp) | P2o(tmp) | P2e(tmp) | P1o(tmp) | P1e(tmp) | P2048o(tmp) | P2048e(tmp));
}

static int nand_davinci_compare_ecc(u_int8_t *ecc_nand, u_int8_t *ecc_calc, u_int8_t *page_data)
{
	u_int32_t	i;
	u_int8_t	tmp0_bit[8], tmp1_bit[8], tmp2_bit[8];
	u_int8_t	comp0_bit[8], comp1_bit[8], comp2_bit[8];
	u_int8_t	ecc_bit[24];
	u_int8_t	ecc_sum = 0;
	u_int8_t	find_bit = 0;
	u_int32_t	find_byte = 0;
	int		is_ecc_ff;

	is_ecc_ff = ((*ecc_nand == 0xff) && (*(ecc_nand + 1) == 0xff) && (*(ecc_nand + 2) == 0xff));

	nand_davinci_gen_true_ecc(ecc_nand);
	nand_davinci_gen_true_ecc(ecc_calc);

	for (i = 0; i <= 2; i++) {
		*(ecc_nand + i) = ~(*(ecc_nand + i));
		*(ecc_calc + i) = ~(*(ecc_calc + i));
	}

	for (i = 0; i < 8; i++) {
		tmp0_bit[i] = *ecc_nand % 2;
		*ecc_nand = *ecc_nand / 2;
	}

	for (i = 0; i < 8; i++) {
		tmp1_bit[i] = *(ecc_nand + 1) % 2;
		*(ecc_nand + 1) = *(ecc_nand + 1) / 2;
	}

	for (i = 0; i < 8; i++) {
		tmp2_bit[i] = *(ecc_nand + 2) % 2;
		*(ecc_nand + 2) = *(ecc_nand + 2) / 2;
	}

	for (i = 0; i < 8; i++) {
		comp0_bit[i] = *ecc_calc % 2;
		*ecc_calc = *ecc_calc / 2;
	}

	for (i = 0; i < 8; i++) {
		comp1_bit[i] = *(ecc_calc + 1) % 2;
		*(ecc_calc + 1) = *(ecc_calc + 1) / 2;
	}

	for (i = 0; i < 8; i++) {
		comp2_bit[i] = *(ecc_calc + 2) % 2;
		*(ecc_calc + 2) = *(ecc_calc + 2) / 2;
	}

	for (i = 0; i< 6; i++)
		ecc_bit[i] = tmp2_bit[i + 2] ^ comp2_bit[i + 2];

	for (i = 0; i < 8; i++)
		ecc_bit[i + 6] = tmp0_bit[i] ^ comp0_bit[i];

	for (i = 0; i < 8; i++)
		ecc_bit[i + 14] = tmp1_bit[i] ^ comp1_bit[i];

	ecc_bit[22] = tmp2_bit[0] ^ comp2_bit[0];
	ecc_bit[23] = tmp2_bit[1] ^ comp2_bit[1];

	for (i = 0; i < 24; i++)
		ecc_sum += ecc_bit[i];

	switch (ecc_sum) {
		case 0:
			/* Not reached because this function is not called if
			   ECC values are equal */
			return 0;
		case 1:
			/* Uncorrectable error */
			MTDDEBUG (MTD_DEBUG_LEVEL0,
			          "ECC UNCORRECTED_ERROR 1\n");
			return(-1);
		case 12:
			/* Correctable error */
			find_byte = (ecc_bit[23] << 8) +
				(ecc_bit[21] << 7) +
				(ecc_bit[19] << 6) +
				(ecc_bit[17] << 5) +
				(ecc_bit[15] << 4) +
				(ecc_bit[13] << 3) +
				(ecc_bit[11] << 2) +
				(ecc_bit[9]  << 1) +
				ecc_bit[7];

			find_bit = (ecc_bit[5] << 2) + (ecc_bit[3] << 1) + ecc_bit[1];

			MTDDEBUG (MTD_DEBUG_LEVEL0, "Correcting single bit ECC "
			          "error at offset: %d, bit: %d\n",
			          find_byte, find_bit);

			page_data[find_byte] ^= (1 << find_bit);

			return(0);
		default:
			if (is_ecc_ff) {
				if (ecc_calc[0] == 0 && ecc_calc[1] == 0 && ecc_calc[2] == 0)
					return(0);
			}
			MTDDEBUG (MTD_DEBUG_LEVEL0,
			          "UNCORRECTED_ERROR default\n");
			return(-1);
	}
}

static int nand_davinci_correct_data(struct mtd_info *mtd, u_char *dat, u_char *read_ecc, u_char *calc_ecc)
{
	struct nand_chip	*this;
	int			block_count = 0, i, rc;

	this = mtd->priv;
	block_count = (this->ecc.size/512);
	for (i = 0; i < block_count; i++) {
		if (memcmp(read_ecc, calc_ecc, 3) != 0) {
			rc = nand_davinci_compare_ecc(read_ecc, calc_ecc, dat);
			if (rc < 0) {
				return(rc);
			}
		}
		read_ecc += 3;
		calc_ecc += 3;
		dat += 512;
	}
	return(0);
}
#endif

static int nand_davinci_dev_ready(struct mtd_info *mtd)
{
	emifregs	emif_addr;

#ifdef CFG_DM6467_EVM
	emif_addr = (emifregs)DAVINCI_DM646X_ASYNC_EMIF_CNTRL_BASE;
#elif defined(CFG_DM365_EVM) || defined(CFG_DM355_EVM) || defined(CFG_DM365_IPNC) || defined(CFG_DM368_IPNC)
	emif_addr = (emifregs)DM3XX_NAND_BASE;
#else
	emif_addr = (emifregs)DAVINCI_ASYNC_EMIF_CNTRL_BASE;
#endif
	return(emif_addr->NANDFSR & 0x1);
}

static int nand_davinci_waitfunc(struct mtd_info *mtd, struct nand_chip *this)
{
	volatile unsigned int status;

	while(!nand_davinci_dev_ready(mtd)) {;}

	do {
		*NAND_CE0CLE = NAND_STATUS;
		status = *NAND_CE0DATA;
	} while(!(status & 0x40));

	return(*NAND_CE0DATA);
}

static void nand_flash_init(void)
{
	u_int32_t	acfg1 = 0x3ffffffc;
	u_int32_t	acfg2 = 0x3ffffffc;
	u_int32_t	acfg3 = 0x3ffffffc;
	u_int32_t	acfg4 = 0x3ffffffc;
	emifregs	emif_regs;

	/*------------------------------------------------------------------*
	 *  NAND FLASH CHIP TIMEOUT @ 459 MHz                               *
	 *                                                                  *
	 *  AEMIF.CLK freq   = PLL1/6 = 459/6 = 76.5 MHz                    *
	 *  AEMIF.CLK period = 1/76.5 MHz = 13.1 ns                         *
	 *                                                                  *
	 *------------------------------------------------------------------*/
	 acfg1 = 0
		| (0 << 31 )	/* selectStrobe */
		| (0 << 30 )	/* extWait */
		| (1 << 26 )	/* writeSetup	10 ns */
		| (3 << 20 )	/* writeStrobe	40 ns */
		| (1 << 17 )	/* writeHold	10 ns */
		| (1 << 13 )	/* readSetup	10 ns */
		| (5 << 7 )	/* readStrobe	60 ns */
		| (1 << 4 )	/* readHold	10 ns */
		| (3 << 2 )	/* turnAround	?? ns */
		| (0 << 0 )	/* asyncSize	8-bit bus */
		;

	emif_regs = (emifregs)DAVINCI_ASYNC_EMIF_CNTRL_BASE;

	emif_regs->AWCCR |= 0x10000000;
	emif_regs->AB1CR = acfg1;	/* 0x08244128 */;
	emif_regs->AB2CR = acfg2;
	emif_regs->AB3CR = acfg3;
	emif_regs->AB4CR = acfg4;
	emif_regs->NANDFCR = 0x00000101;
}

int board_nand_init(struct nand_chip *nand)
{
#if !defined(CFG_DM355_EVM) && !defined(CFG_DM365_EVM) && !defined(CFG_DM365_IPNC) && !defined(CFG_DM368_IPNC)
	nand->IO_ADDR_R   = (void  __iomem *)NAND_CE0DATA;
	nand->IO_ADDR_W   = (void  __iomem *)NAND_CE0DATA;
#endif
	nand->chip_delay  = 0;
#if !defined(CFG_DM355_EVM) && !defined(CFG_DM365_EVM) && !defined(CFG_DM365_IPNC) && !defined(CFG_DM368_IPNC)
	nand->select_chip = nand_davinci_select_chip;
#endif
#ifdef CFG_NAND_USE_FLASH_BBT
	nand->options	  = NAND_USE_FLASH_BBT;
#endif
#ifdef CFG_NAND_HW_ECC
#ifdef CFG_NAND_4BIT_ECC
	nand->ecc.mode = NAND_ECC_HW_SYNDROME;
	nand->ecc.size = 512;
	nand->ecc.bytes = 10;
	nand->ecc.prepad = 6;
#ifdef CFG_DM365_IPNC
	nand->badblock_pattern = &nand_dm365_hw10_512_badblock_pattern;
#endif
	nand->bbt_td = &nand_davinci_bbt_main_descr;
	nand->bbt_md = &nand_davinci_bbt_mirror_descr;
	nand->ecc.layout = &nand_davinci_4bit_layout;
	nand->ecc.calculate = nand_davinci_4bit_calculate_ecc;
	nand->ecc.correct = nand_davinci_4bit_correct_data;
	nand->ecc.hwctl = nand_davinci_4bit_enable_hwecc;
#ifdef CFG_DAVINCI_STD_NAND_LAYOUT
	nand->options |= NAND_USE_FLASH_BBT;
	nand->ecc.read_page = davinci_std_read_page_syndrome;
	nand->ecc.write_page = davinci_std_write_page_syndrome;
	nand->ecc.read_oob = davinci_std_read_oob_syndrome;
	nand->ecc.write_oob = davinci_std_write_oob_syndrome;
#elif defined(CFG_DM365_IPNC)  /*ANR */
	nand->ecc.read_page = davinci_read_page_syndrome;
	nand->ecc.write_page = davinci_write_page_syndrome;
	nand->ecc.read_oob = davinci_std_read_oob_syndrome;
	nand->ecc.write_oob = davinci_std_write_oob_syndrome;
    nand->options |= NAND_USE_FLASH_BBT;
#else
	nand->ecc.read_page = davinci_read_page_syndrome;
	nand->ecc.write_page = davinci_write_page_syndrome;
	nand->options |= NAND_USE_FLASH_BBT | NAND_USE_DATA_ADJACENT_OOB;
#endif
#else
	nand->ecc.mode = NAND_ECC_HW;
#ifdef CFG_NAND_LARGEPAGE
	nand->ecc.size = 2048;
	nand->ecc.bytes = 12;
#elif defined(CFG_NAND_SMALLPAGE)
	nand->ecc.size = 512;
	nand->ecc.bytes = 3;
#else
#error "Either CFG_NAND_LARGEPAGE or CFG_NAND_SMALLPAGE must be defined!"
#endif
	nand->ecc.layout  = &davinci_nand_ecclayout;
	nand->ecc.calculate = nand_davinci_calculate_ecc;
	nand->ecc.correct  = nand_davinci_correct_data;
	nand->ecc.hwctl  = nand_davinci_enable_hwecc;
#endif
#else
	nand->ecc.mode = NAND_ECC_SOFT;
#endif

	/* Set address of hardware control function */
	nand->cmd_ctrl = nand_davinci_hwcontrol;
#if !defined(CFG_DM355_EVM) && !defined(CFG_DM365_EVM) && !defined(CFG_DM365_IPNC) && !defined(CFG_DM368_IPNC)
	nand->dev_ready = nand_davinci_dev_ready;
	nand->waitfunc = nand_davinci_waitfunc;
#endif

#if !defined(CFG_DM355_EVM) && !defined(CFG_DM365_EVM) && !defined(CFG_DM6467_EVM) && !defined(CFG_DM365_IPNC) && !defined(CFG_DM368_IPNC)
	nand_flash_init();
#endif

	return(0);
}

#else
#error "U-Boot legacy NAND support not available for DaVinci chips"
#endif
#endif	/* CFG_USE_NAND */


