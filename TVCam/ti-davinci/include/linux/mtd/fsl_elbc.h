/*
 * (C) Copyright 2004-2006 Freescale Semiconductor, Inc.
 *
 * Freescale Enhanced Local Bus Controller Internal Memory Map
 *
 * History :
 * 20061010 : Extracted fomr immap_83xx.h
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 */
#ifndef __FSL_ELBC__
#define __FSL_ELBC__

#ifdef __KERNEL__

#define PFX "fsl-elbc: "

#define FCM_SIZE (8 * 1024)

#define MAX_BANKS (8)

#define ERR_BYTE 0xFF		/* Value returned for read bytes when read failed */

#define FCM_TIMEOUT_MSECS 100	/* Maximum number of mSecs to wait for FCM */

/*
 * Local Bus Controller Registers
 */
typedef struct lbus_bank{
	u32 br;             /**< Base Register  */
#define BR0 0x5000
#define BR1 0x5008
#define BR2 0x5010
#define BR3 0x5018
#define BR4 0x5020
#define BR5 0x5028
#define BR6 0x5030
#define BR7 0x5038

#define BR_BA           0xFFFF8000
#define BR_BA_SHIFT             15
#define BR_PS           0x00001800
#define BR_PS_SHIFT             11
#define BR_PS_8         0x00000800  /* Port Size 8 bit */
#define BR_PS_16        0x00001000  /* Port Size 16 bit */
#define BR_PS_32        0x00001800  /* Port Size 32 bit */
#define BR_DECC         0x00000600
#define BR_DECC_SHIFT            9
#define BR_DECC_OFF     0x00000000  /* HW ECC checking and generation off */
#define BR_DECC_CHK     0x00000200  /* HW ECC checking on, generation off */
#define BR_DECC_CHK_GEN 0x00000400  /* HW ECC checking and generation on */
#define BR_WP           0x00000100
#define BR_WP_SHIFT              8
#define BR_MSEL         0x000000E0
#define BR_MSEL_SHIFT            5
#define BR_MS_GPCM      0x00000000  /* GPCM */
#define BR_MS_FCM       0x00000020  /* FCM */
#define BR_MS_SDRAM     0x00000060  /* SDRAM */
#define BR_MS_UPMA      0x00000080  /* UPMA */
#define BR_MS_UPMB      0x000000A0  /* UPMB */
#define BR_MS_UPMC      0x000000C0  /* UPMC */
#define BR_V            0x00000001
#define BR_V_SHIFT               0
#define BR_RES          ~(BR_BA|BR_PS|BR_DECC|BR_WP|BR_MSEL|BR_V)

	u32 or;             /**< Base Register  */
#define OR0 0x5004
#define OR1 0x500C
#define OR2 0x5014
#define OR3 0x501C
#define OR4 0x5024
#define OR5 0x502C
#define OR6 0x5034
#define OR7 0x503C

#define OR_GPCM_AM              0xFFFF8000
#define OR_GPCM_AM_SHIFT                15
#define OR_GPCM_BCTLD           0x00001000
#define OR_GPCM_BCTLD_SHIFT             12
#define OR_GPCM_CSNT            0x00000800
#define OR_GPCM_CSNT_SHIFT              11
#define OR_GPCM_ACS             0x00000600
#define OR_GPCM_ACS_SHIFT                9
#define OR_GPCM_ACS_0b10        0x00000400
#define OR_GPCM_ACS_0b11        0x00000600
#define OR_GPCM_XACS            0x00000100
#define OR_GPCM_XACS_SHIFT               8
#define OR_GPCM_SCY             0x000000F0
#define OR_GPCM_SCY_SHIFT                4
#define OR_GPCM_SCY_1           0x00000010
#define OR_GPCM_SCY_2           0x00000020
#define OR_GPCM_SCY_3           0x00000030
#define OR_GPCM_SCY_4           0x00000040
#define OR_GPCM_SCY_5           0x00000050
#define OR_GPCM_SCY_6           0x00000060
#define OR_GPCM_SCY_7           0x00000070
#define OR_GPCM_SCY_8           0x00000080
#define OR_GPCM_SCY_9           0x00000090
#define OR_GPCM_SCY_10          0x000000a0
#define OR_GPCM_SCY_11          0x000000b0
#define OR_GPCM_SCY_12          0x000000c0
#define OR_GPCM_SCY_13          0x000000d0
#define OR_GPCM_SCY_14          0x000000e0
#define OR_GPCM_SCY_15          0x000000f0
#define OR_GPCM_SETA            0x00000008
#define OR_GPCM_SETA_SHIFT               3
#define OR_GPCM_TRLX            0x00000004
#define OR_GPCM_TRLX_SHIFT               2
#define OR_GPCM_EHTR            0x00000002
#define OR_GPCM_EHTR_SHIFT               1
#define OR_GPCM_EAD             0x00000001
#define OR_GPCM_EAD_SHIFT                0

#define OR_UPM_AM    0xFFFF8000
#define OR_UPM_AM_SHIFT      15
#define OR_UPM_XAM   0x00006000
#define OR_UPM_XAM_SHIFT     13
#define OR_UPM_BCTLD 0x00001000
#define OR_UPM_BCTLD_SHIFT   12
#define OR_UPM_BI    0x00000100
#define OR_UPM_BI_SHIFT       8
#define OR_UPM_TRLX  0x00000004
#define OR_UPM_TRLX_SHIFT     2
#define OR_UPM_EHTR  0x00000002
#define OR_UPM_EHTR_SHIFT     1
#define OR_UPM_EAD   0x00000001
#define OR_UPM_EAD_SHIFT      0

#define OR_SDRAM_AM    0xFFFF8000
#define OR_SDRAM_AM_SHIFT      15
#define OR_SDRAM_XAM   0x00006000
#define OR_SDRAM_XAM_SHIFT     13
#define OR_SDRAM_COLS  0x00001C00
#define OR_SDRAM_COLS_SHIFT    10
#define OR_SDRAM_ROWS  0x000001C0
#define OR_SDRAM_ROWS_SHIFT     6
#define OR_SDRAM_PMSEL 0x00000020
#define OR_SDRAM_PMSEL_SHIFT    5
#define OR_SDRAM_EAD   0x00000001
#define OR_SDRAM_EAD_SHIFT      0

#define OR_FCM_AM               0xFFFF8000
#define OR_FCM_AM_SHIFT                 15
#define OR_FCM_BCTLD            0x00001000
#define OR_FCM_BCTLD_SHIFT              12
#define OR_FCM_PGS              0x00000400
#define OR_FCM_PGS_SHIFT                10
#define OR_FCM_CSCT             0x00000200
#define OR_FCM_CSCT_SHIFT                9
#define OR_FCM_CST              0x00000100
#define OR_FCM_CST_SHIFT                 8
#define OR_FCM_CHT              0x00000080
#define OR_FCM_CHT_SHIFT                 7
#define OR_FCM_SCY              0x00000070
#define OR_FCM_SCY_SHIFT                 4
#define OR_FCM_SCY_1            0x00000010
#define OR_FCM_SCY_2            0x00000020
#define OR_FCM_SCY_3            0x00000030
#define OR_FCM_SCY_4            0x00000040
#define OR_FCM_SCY_5            0x00000050
#define OR_FCM_SCY_6            0x00000060
#define OR_FCM_SCY_7            0x00000070
#define OR_FCM_RST              0x00000008
#define OR_FCM_RST_SHIFT                 3
#define OR_FCM_TRLX             0x00000004
#define OR_FCM_TRLX_SHIFT                2
#define OR_FCM_EHTR             0x00000002
#define OR_FCM_EHTR_SHIFT                1
} lbus_bank_t;

typedef struct lbus83xx {
	lbus_bank_t bank[8];
	u8 res0[0x28];
	u32 mar;                /**< UPM Address Register */
	u8 res1[0x4];
	u32 mamr;               /**< UPMA Mode Register */
	u32 mbmr;               /**< UPMB Mode Register */
	u32 mcmr;               /**< UPMC Mode Register */
	u8 res2[0x8];
	u32 mrtpr;              /**< Memory Refresh Timer Prescaler Register */
	u32 mdr;                /**< UPM Data Register */
	u8 res3[0x4];
	u32 lsor;               /**< Special Operation Initiation Register */
	u32 lsdmr;              /**< SDRAM Mode Register */
	u8 res4[0x8];
	u32 lurt;               /**< UPM Refresh Timer */
	u32 lsrt;               /**< SDRAM Refresh Timer */
	u8 res5[0x8];
	u32 ltesr;              /**< Transfer Error Status Register */
#define LTESR_BM   0x80000000
#define LTESR_FCT  0x40000000
#define LTESR_PAR  0x20000000
#define LTESR_WP   0x04000000
#define LTESR_ATMW 0x00800000
#define LTESR_ATMR 0x00400000
#define LTESR_CS   0x00080000
#define LTESR_CC   0x00000001
	u32 ltedr;              /**< Transfer Error Disable Register */
	u32 lteir;              /**< Transfer Error Interrupt Register */
	u32 lteatr;             /**< Transfer Error Attributes Register */
	u32 ltear;              /**< Transfer Error Address Register */
	u8 res6[0xC];
	u32 lbcr;               /**< Configuration Register */
#define LBCR_LDIS  0x80000000
#define LBCR_LDIS_SHIFT    31
#define LBCR_BCTLC 0x00C00000
#define LBCR_BCTLC_SHIFT   22
#define LBCR_AHD   0x00200000
#define LBCR_LPBSE 0x00020000
#define LBCR_LPBSE_SHIFT   17
#define LBCR_EPAR  0x00010000
#define LBCR_EPAR_SHIFT    16
#define LBCR_BMT   0x0000FF00
#define LBCR_BMT_SHIFT      8
#define LBCR_INIT  0x00040000
	u32 lcrr;               /**< Clock Ratio Register */
#define LCRR_DBYP    0x80000000
#define LCRR_DBYP_SHIFT      31
#define LCRR_BUFCMDC 0x30000000
#define LCRR_BUFCMDC_SHIFT   28
#define LCRR_ECL     0x03000000
#define LCRR_ECL_SHIFT       24
#define LCRR_EADC    0x00030000
#define LCRR_EADC_SHIFT      16
#define LCRR_CLKDIV  0x0000000F
#define LCRR_CLKDIV_SHIFT     0
	u8 res7[0x8];
	u32 fmr;               /**< Flash Mode Register */
#define FMR_CWTO     0x0000F000
#define FMR_CWTO_SHIFT       12
#define FMR_BOOT     0x00000800
#define FMR_ECCM     0x00000100
#define FMR_AL       0x00000030
#define FMR_AL_SHIFT          4
#define FMR_OP       0x00000003
#define FMR_OP_SHIFT          0
	u32 fir;               /**< Flash Instruction Register */
#define FIR_OP0      0xF0000000
#define FIR_OP0_SHIFT        28
#define FIR_OP1      0x0F000000
#define FIR_OP1_SHIFT        24
#define FIR_OP2      0x00F00000
#define FIR_OP2_SHIFT        20
#define FIR_OP3      0x000F0000
#define FIR_OP3_SHIFT        16
#define FIR_OP4      0x0000F000
#define FIR_OP4_SHIFT        12
#define FIR_OP5      0x00000F00
#define FIR_OP5_SHIFT         8
#define FIR_OP6      0x000000F0
#define FIR_OP6_SHIFT         4
#define FIR_OP7      0x0000000F
#define FIR_OP7_SHIFT         0
#define FIR_OP_NOP   0x0	/* No operation and end of sequence */
#define FIR_OP_CA    0x1        /* Issue current column address */
#define FIR_OP_PA    0x2        /* Issue current block+page address */
#define FIR_OP_UA    0x3        /* Issue user defined address */
#define FIR_OP_CM0   0x4        /* Issue command from FCR[CMD0] */
#define FIR_OP_CM1   0x5        /* Issue command from FCR[CMD1] */
#define FIR_OP_CM2   0x6        /* Issue command from FCR[CMD2] */
#define FIR_OP_CM3   0x7        /* Issue command from FCR[CMD3] */
#define FIR_OP_WB    0x8        /* Write FBCR bytes from FCM buffer */
#define FIR_OP_WS    0x9        /* Write 1 or 2 bytes from MDR[AS] */
#define FIR_OP_RB    0xA        /* Read FBCR bytes to FCM buffer */
#define FIR_OP_RS    0xB        /* Read 1 or 2 bytes to MDR[AS] */
#define FIR_OP_CW0   0xC        /* Wait then issue FCR[CMD0] */
#define FIR_OP_CW1   0xD        /* Wait then issue FCR[CMD1] */
#define FIR_OP_RBW   0xE        /* Wait then read FBCR bytes */
#define FIR_OP_RSW   0xE        /* Wait then read 1 or 2 bytes */
	u32 fcr;               /**< Flash Command Register */
#define FCR_CMD0     0xFF000000
#define FCR_CMD0_SHIFT       24
#define FCR_CMD1     0x00FF0000
#define FCR_CMD1_SHIFT       16
#define FCR_CMD2     0x0000FF00
#define FCR_CMD2_SHIFT        8
#define FCR_CMD3     0x000000FF
#define FCR_CMD3_SHIFT        0
	u32 fbar;              /**< Flash Block Address Register */
#define FBAR_BLK     0x00FFFFFF
	u32 fpar;              /**< Flash Page Address Register */
#define FPAR_SP_PI   0x00007C00
#define FPAR_SP_PI_SHIFT     10
#define FPAR_SP_MS   0x00000200
#define FPAR_SP_CI   0x000001FF
#define FPAR_SP_CI_SHIFT      0
#define FPAR_LP_PI   0x0003F000
#define FPAR_LP_PI_SHIFT     12
#define FPAR_LP_MS   0x00000800
#define FPAR_LP_CI   0x000007FF
#define FPAR_LP_CI_SHIFT      0
	u32 fbcr;              /**< Flash Byte Count Register */
#define FBCR_BC      0x00000FFF
	u8 res11[0x8];
	u8 res8[0xF00];
} lbus83xx_t;

struct platform_fsl_nand_chip {
	const char	*name;
	int		nr_chips;
	const char 	*partitions_str;
};

/* mtd information per set */

struct fsl_elbc_mtd {
	struct mtd_info mtd;
	struct nand_chip chip;
	struct platform_fsl_nand_chip pl_chip;
	struct fsl_elbc_ctrl *ctrl;

	struct device *device;
	char *name;		/* Name of set (optional)    */
	int *nr_map;		/* Physical chip num (option) */
	unsigned int options;
	int bank;		/* Chip select bank number           */
	unsigned int pbase;	/* Chip select base physical address */
	unsigned int vbase;	/* Chip select base virtual address  */
	int pgs;		/* NAND page size (0=512, 1=2048)    */
	unsigned int fmr;	/* FCM Flash Mode Register value     */
};

/* overview of the fsl elbc controller */

struct fsl_elbc_ctrl {
	struct nand_hw_control controller;
	struct fsl_elbc_mtd *nmtd[MAX_BANKS];

	/* device info */
	atomic_t childs_active;
	struct device *device;
	lbus83xx_t *regs;
	int irq;
	wait_queue_head_t irq_wait;
	unsigned int irq_status;	/* status read from LTESR by irq handler */
	u_char *addr;		/* Address of assigned FCM buffer        */
	unsigned int page;	/* Last page written to / read from      */
	unsigned int read_bytes;	/* Number of bytes read during command   */
	unsigned int index;	/* Pointer to next byte to 'read'        */
	unsigned int status;	/* status read from LTESR after last op  */
	int oobbuf;		/* Pointer to OOB block                  */
	unsigned int mdr;	/* UPM/FCM Data Register value           */
	unsigned int use_mdr;	/* Non zero if the MDR is to be set      */
};

/* Setting this option prevents the command line from being parsed
 * for MTD partitions. */
#define FSL_ELBC_NO_CMDLINE_PARTITIONS	0x10000000

#endif /* __KERNEL__ */
#endif /* __FSL_ELBC__ */
