/*
 * NAND driver for TI DaVinci based boards.
 *
 * Copyright (C) 2007 Sergey Kubushyn <ksi@koi8.net>
 *
 * Based on Linux DaVinci NAND driver by TI. Original copyright follows:
 */

/*
 *
 * linux/drivers/mtd/nand/nand_dm355.c
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
           March 2008, Sandeep
 -
 *
 */

#include <common.h>

#ifdef CONFIG_CMD_NAND
#if !defined(CFG_NAND_LEGACY)

#include <asm/arch/hardware.h>
#include <nand.h>
#include <asm/arch/nand_defs.h>
#include <asm/arch/emif_defs.h>

#define CSL_EMIF_1_REGS     (0x68000000)
#define NAND4BITECCLOAD	    (CSL_EMIF_1_REGS + 0xBC)
#define NAND4BITECC1        (CSL_EMIF_1_REGS + 0xC0)
#define NAND4BITECC2        (CSL_EMIF_1_REGS + 0xC4) 
#define NAND4BITECC3        (CSL_EMIF_1_REGS + 0xC8)
#define NAND4BITECC4        (CSL_EMIF_1_REGS + 0xCC)
#define NANDERRADD1	    (CSL_EMIF_1_REGS + 0xD0)
#define NANDERRADD2         (CSL_EMIF_1_REGS + 0xD4)
#define NANDERRVAL1	    (CSL_EMIF_1_REGS + 0xD8)
#define NANDERRVAL2	    (CSL_EMIF_1_REGS + 0xDC)

/* Definitions for 4-bit hardware ECC */
#define NAND_4BITECC_MASK		0x03FF03FF
#define EMIF_NANDFSR_ECC_STATE_MASK  	0x00000F00
#define ECC_STATE_NO_ERR		0x0
#define ECC_STATE_TOO_MANY_ERRS		0x1
#define ECC_STATE_ERR_CORR_COMP_P	0x2
#define ECC_STATE_ERR_CORR_COMP_N	0x3
#define ECC_MAX_CORRECTABLE_ERRORS	0x4

extern struct nand_chip nand_dev_desc[CFG_MAX_NAND_DEVICE];

static void nand_dm350evm_hwcontrol(struct mtd_info *mtd, int cmd)
{
	struct		nand_chip *this = mtd->priv;
	u_int32_t	IO_ADDR_W = (u_int32_t)this->IO_ADDR_W;
	u_int32_t	IO_ADDR_R = (u_int32_t)this->IO_ADDR_R;

	IO_ADDR_W &= ~(MASK_ALE|MASK_CLE);

	switch (cmd) {
		case NAND_CTL_SETCLE:
			IO_ADDR_W |= MASK_CLE;
			break;
		case NAND_CTL_SETALE:
			IO_ADDR_W |= MASK_ALE;
			break;
	}

	this->IO_ADDR_W = (void *)IO_ADDR_W;
}

/*
 * Instead of placing the spare data at the end of the page, the 4-bit ECC
 * hardware generator requires that the page be subdivided into 4 subpages,
 * each with its own spare data area.  This structure defines the format of
 * each of these subpages.
 */
static struct page_layout_item nand_dm355_hw10_512_layout[] = {
	{.type = ITEM_TYPE_DATA,.length = 512},
	{.type = ITEM_TYPE_OOB,.length = 6,},
	{.type = ITEM_TYPE_ECC,.length = 10,},
	{.type = 0,.length = 0,},
};

static struct nand_oobinfo nand_dm355_hw10_512_oobinfo = {
	.useecc = MTD_NANDECC_AUTOPLACE,
	/*
	 * We actually have 40 bytes of ECC per page, but the nand_oobinfo
	 * structure definition limits us to a maximum of 32 bytes.  This
	 * doesn't matter, because out page_layout_item structure definition
	 * determines where our ECC actually goes in the flash page.
	 */
	.eccbytes = 32,
	.eccpos = {6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
		   22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
		   38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
		   54, 55,
		   },
	.oobfree = {{0, 6}, {16, 6}, {32, 6}, {48, 6}},
};



static int nand_dm355_hw10_512_block_markbad(struct mtd_info *mtd, loff_t ofs)
{
	struct nand_chip *this = mtd->priv;
	int block;

	/* Get block number */
	block = ((int)ofs) >> this->bbt_erase_shift;
	if (this->bbt)
		this->bbt[block >> 2] |= 0x01 << ((block & 0x03) << 1);

	/* Do we have a flash based bad block table ? */
	if (this->options & NAND_USE_FLASH_BBT)
		return nand_update_bbt(mtd, ofs);

	return 0;
}

static void nand_dm355_4bit_enable_hwecc(struct mtd_info *mtd, int mode)
{
	struct nand_chip *this = mtd->priv;
    emifregs    emif_addr = (emifregs)CSL_EMIF_1_REGS;
	u32 val;

	switch (mode) {
	case NAND_ECC_WRITE:
	case NAND_ECC_READ:
		/*
		 * Start a new ECC calculation for reading or writing 512 bytes
		 *  of data.
		 */
		val = (emif_addr->NANDFCR & ~(3 << 4));
		val |= (1 << 4) | (1 << 12);
        	emif_addr->NANDFCR = val;
		break;
	case NAND_ECC_WRITEOOB:
	case NAND_ECC_READOOB:
		/*
		 * Terminate ECC calculation by performing a dummy read of an
		 * ECC register.  Our hardware ECC generator supports including
		 * the OOB in the ECC calculation, but the NAND core code
		 * doesn't really support that.  We will only calculate the ECC
		 * on the data; errors in the non-ECC bytes in the OOB will not
		 * be detected or corrected.
		 */
        val = emif_addr->NANDF1ECC;
		break;
	case NAND_ECC_WRITESYN:
	case NAND_ECC_READSYN:
		/*
		 * Our ECC calculation has already been terminated, so no need
		 * to do anything here.
		 */
		break;
	default:
		break;
	}
}

static u32 nand_dm355_4bit_readecc(struct mtd_info *mtd, unsigned int ecc[4])
{
    emifregs    emif_addr = (emifregs)CSL_EMIF_1_REGS;

	ecc[0] = (*(dv_reg_p) NAND4BITECC1) & NAND_4BITECC_MASK;
	ecc[1] = (*(dv_reg_p) NAND4BITECC2) & NAND_4BITECC_MASK;
	ecc[2] = (*(dv_reg_p) NAND4BITECC3) & NAND_4BITECC_MASK;
	ecc[3] = (*(dv_reg_p) NAND4BITECC4) & NAND_4BITECC_MASK;

	return 0;
}

static int nand_dm355_4bit_calculate_ecc(struct mtd_info *mtd,
					   const u_char * dat,
					   u_char * ecc_code)
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
	nand_dm355_4bit_readecc(mtd, hw_4ecc);

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

static int nand_dm355_4bit_compare_ecc(struct mtd_info *mtd, u8 * read_ecc,	/* read from NAND */
					 u8 * page_data)
{
	struct nand_chip *this = mtd->priv;
	struct nand_dm355_info *info = this->priv;
	unsigned short ecc_10bit[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	int i;
	unsigned int hw_4ecc[4] = { 0, 0, 0, 0 }, iserror = 0;
	unsigned short *pspare = NULL, *pspare1 = NULL;
	unsigned int numErrors, errorAddress, errorValue;
    emifregs    emif_addr = (emifregs)CSL_EMIF_1_REGS;
	u32 val;

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
    {
       *(dv_reg_p)NAND4BITECCLOAD = ecc_10bit[i];  
    }

	/*
	 * Perform a dummy read to the EMIF Revision Code and Status register.
	 * This is required to ensure time for syndrome calculation after
	 * writing the ECC values in previous step.
	 */
	val = emif_addr->ERCSR;

	/*
	 * Read the syndrome from the NAND Flash 4-Bit ECC 1-4 registers.
	 * A syndrome value of 0 means no bit errors. If the syndrome is
	 * non-zero then go further otherwise return.
	 */
	nand_dm355_4bit_readecc(mtd, hw_4ecc);

	if (hw_4ecc[0] == ECC_STATE_NO_ERR && hw_4ecc[1] == ECC_STATE_NO_ERR &&
	    hw_4ecc[2] == ECC_STATE_NO_ERR && hw_4ecc[3] == ECC_STATE_NO_ERR)
		return 0;

	/*
	 * Clear any previous address calculation by doing a dummy read of an
	 * error address register.
	 */
	val = *(dv_reg_p)NANDERRADD1;

	/*
	 * Set the addr_calc_st bit(bit no 13) in the NAND Flash Control
	 * register to 1.
	 */
    
    emif_addr->NANDFCR |= (1 << 13);

	/*
	 * Wait for the corr_state field (bits 8 to 11)in the
	 * NAND Flash Status register to be equal to 0x0, 0x1, 0x2, or 0x3.
	 */
    do {
        iserror = emif_addr->NANDFSR & 0xC00;
    } while (iserror);       

	iserror = emif_addr->NANDFSR;
	iserror &= EMIF_NANDFSR_ECC_STATE_MASK;
	iserror = iserror >> 8;


	if (iserror == ECC_STATE_NO_ERR)
		return 0;
	else if (iserror == ECC_STATE_TOO_MANY_ERRS)
    {
        printf("too many erros to be corrected!\n");
		return -1;
    }

#if 1
	numErrors = ((emif_addr->NANDFSR >> 16) & 0x3) + 1;

	/* Read the error address, error value and correct */
	for (i = 0; i < numErrors; i++) {
		if (i > 1) {
			errorAddress =
			    ((*(dv_reg_p)(NANDERRADD2) >>
			      (16 * (i & 1))) & 0x3FF);
			errorAddress = ((512 + 7) - errorAddress);
			errorValue =
			    ((*(dv_reg_p)(NANDERRVAL2) >>
			      (16 * (i & 1))) & 0xFF);
		} else {
			errorAddress =
			    ((*(dv_reg_p)(NANDERRADD1) >>
			      (16 * (i & 1))) & 0x3FF);
			errorAddress = ((512 + 7) - errorAddress);
			errorValue =
			    ((*(dv_reg_p)(NANDERRVAL1) >>
			      (16 * (i & 1))) & 0xFF);
		}
		/* xor the corrupt data with error value */
		if (errorAddress < 512)
			page_data[errorAddress] ^= errorValue;
	}
#else
	numErrors = ((emif_addr->NANDFSR >> 16) & 0x3);
        // bit 9:0
        errorAddress = 519 - (*(dv_reg_p)NANDERRADD1 & (0x3FF));
        errorValue   = (*(dv_reg_p)NANDERRVAL1) & (0x3FF);
        page_data[errorAddress] ^= (char)errorValue;

        if(numErrors == 0)
            return numErrors;
        else {
            // bit 25:16
            errorAddress = 519 - ( (*(dv_reg_p)NANDERRADD1 & (0x3FF0000))>>16 );
            errorValue   = (*(dv_reg_p)NANDERRVAL1) & (0x3FF);
            page_data[errorAddress] ^= (char)errorValue;

            if(numErrors == 1)
                return numErrors;
            else {
                // bit 9:0
                errorAddress = 519 - (*(dv_reg_p)NANDERRADD2 & (0x3FF));
                errorValue = (*(dv_reg_p)NANDERRVAL2) & (0x3FF);
                page_data[errorAddress] ^= (char)errorValue;

                if (numErrors == 2)
                    return numErrors;
                else {
                    // bit 25:16
                    errorAddress = 519 - ( (*(dv_reg_p)NANDERRADD2 & (0x3FF0000))>>16 );
                    errorValue = (*(dv_reg_p)NANDERRVAL2) & (0x3FF);
                    page_data[errorAddress] ^= (char)errorValue;
                }
            }
        }
#endif

	return numErrors;
}

static int nand_dm355_4bit_correct_data(struct mtd_info *mtd, u_char * dat,
					  u_char * read_ecc, u_char * calc_ecc)
{
	int r = 0;

	/*
	 * dat points to 512 bytes of data.  read_ecc points to the start of the
	 * oob area for this subpage, so the ecc values start at offset 6.
	 * The calc_ecc pointer is not needed since our caclulated ECC is
	 * already latched in the hardware ECC generator.
	 */
#if 1
	r = nand_dm355_4bit_compare_ecc(mtd, read_ecc + 6, dat);
#endif

	return r;
}
int board_nand_init(struct nand_chip *nand)
{
	nand->chip_delay  = 0;
	nand->eccmode     = NAND_ECC_HW10_512;
	nand->options     = NAND_HWECC_SYNDROME;
    	nand->autooob = &nand_dm355_hw10_512_oobinfo;
	nand->layout = nand_dm355_hw10_512_layout;
	nand->calculate_ecc = nand_dm355_4bit_calculate_ecc;
	nand->correct_data = nand_dm355_4bit_correct_data;
	nand->enable_hwecc = nand_dm355_4bit_enable_hwecc;
	nand->block_markbad = nand_dm355_hw10_512_block_markbad;
	    
	/* Set address of hardware control function */
	nand->hwcontrol = nand_dm350evm_hwcontrol;

	
	return 0;
}

#else
#error "U-Boot legacy NAND support not available for DaVinci chips"
#endif
#endif	/* CFG_USE_NAND */
