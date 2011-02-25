/*
 * TI DaVinci DMA Support
 *
 * Copyright (C) 2006 Texas Instruments.
 * Copyright (c) 2007-2008, MontaVista Software, Inc. <source@mvista.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#ifndef __ASM_ARCH_EDMA_H
#define __ASM_ARCH_EDMA_H

#include <linux/types.h>

#include <asm/arch/hardware.h>

/* resource flags for IORESOURCE_DMA resources */
#define IORESOURCE_DMA_RX_CHAN		0x01
#define IORESOURCE_DMA_TX_CHAN		0x02
#define IORESOURCE_DMA_EVENT_Q		0x04

/*
 * EDMA3 Driver
 * EDMA3 Driver abstracts each ParamEntry as a Logical DMA channel
 * for the user. So for eg on DM644x, the user can request 128 DMA channels
 *
 * Actual Physical DMA channels (on DM644x) = 64 EDMA channels + 8 QDMA channels
 *
 * User can request for two kinds of Logical DMA channels
 * DMA MasterChannel -> ParamEntry which is associated with a DMA channel.
 *                      On DM644x, there are (64 + 8) MasterChanneles
 *                      MasterChannel can be triggered by an event or manually
 *
 * DMA SlaveChannel  -> ParamEntry which is not associated with DMA cahnnel but
 *                      which can be used to associate with MasterChannel.
 *                      On DM644x, there are (128-(64 + 8)) SlaveChannels
 *                      SlaveChannel can only be triggered by a MasterChannel
 */

#define EDMA_BASE		DAVINCI_DMA_3PCC_BASE

#define EDMA_REV		(EDMA_BASE + 0x0000)
#define EDMA_CCCFG		(EDMA_BASE + 0x0004)
#define EDMA_CLKGDIS		(EDMA_BASE + 0x00FC)
#define EDMA_DCHMAP(n)		(EDMA_BASE + 0x0100 + ((n) << 2))
#define EDMA_QCHMAP(n)		(EDMA_BASE + 0x0200 + ((n) << 2))
#define EDMA_DMAQNUM(n)		(EDMA_BASE + 0x0240 + ((n) << 2))
#define EDMA_QDMAQNUM		(EDMA_BASE + 0x0260)
#define EDMA_QUETCMAP		(EDMA_BASE + 0x0280)
#define EDMA_QUEPRI		(EDMA_BASE + 0x0284)
#define EDMA_EMR		(EDMA_BASE + 0x0300)
#define EDMA_EMRH		(EDMA_BASE + 0x0304)
#define EDMA_EMCR		(EDMA_BASE + 0x0308)
#define EDMA_EMCRH		(EDMA_BASE + 0x030C)
#define EDMA_QEMR		(EDMA_BASE + 0x0310)
#define EDMA_QEMCR		(EDMA_BASE + 0x0314)
#define EDMA_CCERR		(EDMA_BASE + 0x0318)
#define EDMA_CCERRCLR		(EDMA_BASE + 0x031C)
#define EDMA_EEVAL		(EDMA_BASE + 0x0320)
#define EDMA_DRAE(n)		(EDMA_BASE + 0x0340 + ((n) << 3))
#define EDMA_DRAEH(n)		(EDMA_BASE + 0x0344 + ((n) << 3))
#define EDMA_QRAE(n)		(EDMA_BASE + 0x0380 + ((n) << 2))
#define EDMA_QEE0(n)		(EDMA_BASE + 0x0400 + ((n) << 2))
#define EDMA_QEE1(n)		(EDMA_BASE + 0x0440 + ((n) << 2))
#define EDMA_QSTAT0		(EDMA_BASE + 0x0600)
#define EDMA_QSTAT1		(EDMA_BASE + 0x0604)
#define EDMA_QWMTHRA		(EDMA_BASE + 0x0620)
#define EDMA_QWMTHRB		(EDMA_BASE + 0x0624)
#define EDMA_CCSTAT		(EDMA_BASE + 0x0640)
#define EDMA_AETCTL		(EDMA_BASE + 0x0700)
#define EDMA_AETSTAT		(EDMA_BASE + 0x0704)
#define EDMA_AETCMD		(EDMA_BASE + 0x0708)
#define EDMA_MPFAR		(EDMA_BASE + 0x0800)
#define EDMA_MPFSR		(EDMA_BASE + 0x0804)
#define EDMA_MPFCR		(EDMA_BASE + 0x0808)
#define EDMA_MPPAG		(EDMA_BASE + 0x080C)
#define EDMA_MPPA(n)		(EDMA_BASE + 0x0810 + ((n) << 2))
#define EDMA_ER			(EDMA_BASE + 0x1000)
#define EDMA_ERH		(EDMA_BASE + 0x1004)
#define EDMA_ECR		(EDMA_BASE + 0x1008)
#define EDMA_ECRH		(EDMA_BASE + 0x100C)
#define EDMA_ESR		(EDMA_BASE + 0x1010)
#define EDMA_ESRH		(EDMA_BASE + 0x1014)
#define EDMA_CER		(EDMA_BASE + 0x1018)
#define EDMA_CERH		(EDMA_BASE + 0x101C)
#define EDMA_EER		(EDMA_BASE + 0x1020)
#define EDMA_EERH		(EDMA_BASE + 0x1024)
#define EDMA_EECR		(EDMA_BASE + 0x1028)
#define EDMA_EECRH		(EDMA_BASE + 0x102C)
#define EDMA_EESR		(EDMA_BASE + 0x1030)
#define EDMA_EESRH		(EDMA_BASE + 0x1034)
#define EDMA_SER		(EDMA_BASE + 0x1038)
#define EDMA_SERH		(EDMA_BASE + 0x103C)
#define EDMA_SECR		(EDMA_BASE + 0x1040)
#define EDMA_SECRH		(EDMA_BASE + 0x1044)
#define EDMA_IER		(EDMA_BASE + 0x1050)
#define EDMA_IERH		(EDMA_BASE + 0x1054)
#define EDMA_IECR		(EDMA_BASE + 0x1058)
#define EDMA_IECRH		(EDMA_BASE + 0x105C)
#define EDMA_IESR		(EDMA_BASE + 0x1060)
#define EDMA_IESRH		(EDMA_BASE + 0x1064)
#define EDMA_IPR		(EDMA_BASE + 0x1068)
#define EDMA_IPRH		(EDMA_BASE + 0x106C)
#define EDMA_ICR		(EDMA_BASE + 0x1070)
#define EDMA_ICRH		(EDMA_BASE + 0x1074)
#define EDMA_IEVAL		(EDMA_BASE + 0x1078)
#define EDMA_QER		(EDMA_BASE + 0x1080)
#define EDMA_QEER		(EDMA_BASE + 0x1084)
#define EDMA_QEECR		(EDMA_BASE + 0x1088)
#define EDMA_QEESR		(EDMA_BASE + 0x108C)
#define EDMA_QSER		(EDMA_BASE + 0x1090)
#define EDMA_QSECR		(EDMA_BASE + 0x1094)

/* Shadow Registers */
#define EDMA_SHADOW_BASE	(EDMA_BASE + 0x2000)
#define EDMA_SHADOW(offset, n)	(EDMA_SHADOW_BASE + offset + (n << 9))

#define EDMA_SH_ER(n)		EDMA_SHADOW(0x00, n)
#define EDMA_SH_ERH(n)		EDMA_SHADOW(0x04, n)
#define EDMA_SH_ECR(n)		EDMA_SHADOW(0x08, n)
#define EDMA_SH_ECRH(n)		EDMA_SHADOW(0x0C, n)
#define EDMA_SH_ESR(n)		EDMA_SHADOW(0x10, n)
#define EDMA_SH_ESRH(n)		EDMA_SHADOW(0x14, n)
#define EDMA_SH_CER(n)		EDMA_SHADOW(0x18, n)
#define EDMA_SH_CERH(n)		EDMA_SHADOW(0x1C, n)
#define EDMA_SH_EER(n)		EDMA_SHADOW(0x20, n)
#define EDMA_SH_EERH(n)		EDMA_SHADOW(0x24, n)
#define EDMA_SH_EECR(n)		EDMA_SHADOW(0x28, n)
#define EDMA_SH_EECRH(n)	EDMA_SHADOW(0x2C, n)
#define EDMA_SH_EESR(n)		EDMA_SHADOW(0x30, n)
#define EDMA_SH_EESRH(n)	EDMA_SHADOW(0x34, n)
#define EDMA_SH_SER(n)		EDMA_SHADOW(0x38, n)
#define EDMA_SH_SERH(n)		EDMA_SHADOW(0x3C, n)
#define EDMA_SH_SECR(n)		EDMA_SHADOW(0x40, n)
#define EDMA_SH_SECRH(n)	EDMA_SHADOW(0x44, n)
#define EDMA_SH_IER(n)		EDMA_SHADOW(0x50, n)
#define EDMA_SH_IERH(n)		EDMA_SHADOW(0x54, n)
#define EDMA_SH_IECR(n)		EDMA_SHADOW(0x58, n)
#define EDMA_SH_IECRH(n)	EDMA_SHADOW(0x5C, n)
#define EDMA_SH_IESR(n)		EDMA_SHADOW(0x60, n)
#define EDMA_SH_IESRH(n)	EDMA_SHADOW(0x64, n)
#define EDMA_SH_IPR(n)		EDMA_SHADOW(0x68, n)
#define EDMA_SH_IPRH(n)		EDMA_SHADOW(0x6C, n)
#define EDMA_SH_ICR(n)		EDMA_SHADOW(0x70, n)
#define EDMA_SH_ICRH(n)		EDMA_SHADOW(0x74, n)
#define EDMA_SH_IEVAL(n)	EDMA_SHADOW(0x78, n)
#define EDMA_SH_QER(n)		EDMA_SHADOW(0x80, n)
#define EDMA_SH_QEER(n)		EDMA_SHADOW(0x84, n)
#define EDMA_SH_QEECR(n)	EDMA_SHADOW(0x88, n)
#define EDMA_SH_QEESR(n)	EDMA_SHADOW(0x8C, n)
#define EDMA_SH_QSER(n)		EDMA_SHADOW(0x90, n)
#define EDMA_SH_QSECR(n)	EDMA_SHADOW(0x94, n)

/* Paramentry Registers */
#define EDMA_PARAM_BASE			(EDMA_BASE + 0x4000)
#define EDMA_PARAM_ENTRY_SIZE		0x20
#define EDMA_PARAM_SIZE			EDMA_PARAM_ENTRY_SIZE * 512
#define EDMA_PARAM(offset, n)		(EDMA_PARAM_BASE + offset + (n << 5))

#define EDMA_PARAM_OPT(n)		EDMA_PARAM(0x00, n)
#define EDMA_PARAM_SRC(n)		EDMA_PARAM(0x04, n)
#define EDMA_PARAM_A_B_CNT(n)		EDMA_PARAM(0x08, n)
#define EDMA_PARAM_DST(n)		EDMA_PARAM(0x0C, n)
#define EDMA_PARAM_SRC_DST_BIDX(n)	EDMA_PARAM(0x10, n)
#define EDMA_PARAM_LINK_BCNTRLD(n)	EDMA_PARAM(0x14, n)
#define EDMA_PARAM_SRC_DST_CIDX(n)	EDMA_PARAM(0x18, n)
#define EDMA_PARAM_CCNT(n)		EDMA_PARAM(0x1C, n)

/* Transfer Controller registers */
#define EDMATC_REV(base)	(base + 0x000)
#define EDMATC_TCCFG(base)	(base + 0x004)
#define EDMATC_CLKGDIS(base)	(base + 0x0FC)
#define EDMATC_TCSTAT(base)	(base + 0x100)
#define EDMATC_INTSTAT(base)	(base + 0x104)
#define EDMATC_INTEN(base)	(base + 0x108)
#define EDMATC_INTCLR(base)	(base + 0x10C)
#define EDMATC_INTCMD(base)	(base + 0x110)
#define EDMATC_ERRSTAT(base)	(base + 0x120)
#define EDMATC_ERREN(base)	(base + 0x124)
#define EDMATC_ERRCLR(base)	(base + 0x128)
#define EDMATC_ERRDET(base)	(base + 0x12C)
#define EDMATC_ERRCMD(base)	(base + 0x130)
#define EDMATC_RDRATE(base)	(base + 0x140)
#define EDMATC_POPT(base)	(base + 0x200)
#define EDMATC_PSRC(base)	(base + 0x204)
#define EDMATC_PCNT(base)	(base + 0x208)
#define EDMATC_PDST(base)	(base + 0x20C)
#define EDMATC_PBIDX(base)	(base + 0x210)
#define EDMATC_PMPPRXY(base)	(base + 0x214)
#define EDMATC_SAOPT(base)	(base + 0x240)
#define EDMATC_SASRC(base)	(base + 0x244)
#define EDMATC_SACNT(base)	(base + 0x248)
#define EDMATC_SADST(base)	(base + 0x24C)
#define EDMATC_SABIDX(base)	(base + 0x250)
#define EDMATC_SAMPPRXY(base)	(base + 0x254)
#define EDMATC_SACNTRLD(base)	(base + 0x258)
#define EDMATC_SASRCBREF(base)	(base + 0x25C)
#define EDMATC_SADSTBREF(base)	(base + 0x260)
#define EDMATC_DFCNTRLD(base)	(base + 0x280)
#define EDMATC_DFSRCBREF(base)	(base + 0x284)
#define EDMATC_DFDSTBREF(base)	(base + 0x288)

#define EDMATC_DFIFREG(base, n, x)	((base + 0x300) + (n << 6) + x)
#define EDMATC_DFOPT(base, n)		EDMATC_DFIFREG(base, n, 0x00)
#define EDMATC_DFSRC(base, n)		EDMATC_DFIFREG(base, n, 0x04)
#define EDMATC_DFCNT(base, n)		EDMATC_DFIFREG(base, n, 0x08)
#define EDMATC_DFDST(base, n)		EDMATC_DFIFREG(base, n, 0x0C)
#define EDMATC_DFBIDX(base, n)		EDMATC_DFIFREG(base, n, 0x10)
#define EDMATC_DFMPPRXY(base, n)	EDMATC_DFIFREG(base, n, 0x14)

/*
 * Paramentry descriptor
 */
struct paramentry_descriptor {
	unsigned int opt;
	unsigned int src;
	unsigned int a_b_cnt;
	unsigned int dst;
	unsigned int src_dst_bidx;
	unsigned int link_bcntrld;
	unsigned int src_dst_cidx;
	unsigned int ccnt;
};

#define EDMA_DCHMAP_SIZE		0x100

#define dma_write(val, addr)	davinci_writel((val), (addr))
#define dma_read(addr)		davinci_readl(addr)

#define SET_REG_VAL(mask, reg) do { \
	dma_write(dma_read(reg) | (mask), (reg)); \
} while (0)

#define CLEAR_REG_VAL(mask, reg) do { \
	dma_write(dma_read(reg) & ~(mask), (reg)); \
} while (0)

#define CLEAR_EVENT(mask, event, reg) do { \
	if (dma_read(event) & (mask)) \
		SET_REG_VAL((mask), (reg)); \
} while (0)

/* Maximum number of DMA channels possible */
#define EDMA_MAX_DMACH			64
/* Maximum number of QDMA channels possible */
#define EDMA_MAX_QDMACH 		8
/* Maximum number of TCCs possible */
#define EDMA_MAX_TCC			64
/* Maximum number of TCs possible */
#define EDMA_MAX_TC			8
/* Maximum number of PaRAM entries possible */
#define EDMA_MAX_PARAM_SET		512

/* Generic defines for all the platforms */
#define EDMA_NUM_QDMACH 		8
#define EDMA_EVENT_QUEUE_TC_MAPPING	1
#define EDMA_MASTER_SHADOW_REGION	0
#define EDMA_NUM_DMA_CHAN_DWRDS 	(EDMA_MAX_DMACH / 32)
#define EDMA_NUM_QDMA_CHAN_BYTES	1

#define SAM		(1 << 0)
#define DAM		(1 << 1)
#define SYNCDIM		(1 << 2)
#define STATIC		(1 << 3)
#define EDMA_FWID	(7 << 8)
#define TCCMODE 	(1 << 11)
#define TCC		(0x3f << 12)
#define WIMODE		(1 << 19)
#define TCINTEN 	(1 << 20)
#define ITCINTEN	(1 << 21)
#define TCCHEN		(1 << 22)
#define ITCCHEN 	(1 << 23)
#define SECURE		(1 << 30)
#define PRIV		(1 << 31)

#define TRWORD		(7 << 2)
#define PAENTRY		(0x1ff << 5)
/* if changing the QDMA_TRWORD do appropriate change in davinci_start_dma */
#define QDMA_DEF_TRIG_WORD		7

/* Defines for QDMA Channels */
#define EDMA_MAX_CHANNEL		7
#define EDMA_QDMA_CHANNEL_0		davinci_get_qdma_channel(0)
#define EDMA_QDMA_CHANNEL_1		davinci_get_qdma_channel(1)
#define EDMA_QDMA_CHANNEL_2		davinci_get_qdma_channel(2)
#define EDMA_QDMA_CHANNEL_3		davinci_get_qdma_channel(3)
#define EDMA_QDMA_CHANNEL_4		davinci_get_qdma_channel(4)
#define EDMA_QDMA_CHANNEL_5		davinci_get_qdma_channel(5)
#define EDMA_QDMA_CHANNEL_6		davinci_get_qdma_channel(6)
#define EDMA_QDMA_CHANNEL_7		davinci_get_qdma_channel(7)

#define dma_is_qdmach(lch)		((lch) >= EDMA_QDMA_CHANNEL_0 && \
					 (lch) <= EDMA_QDMA_CHANNEL_7)

#define CCRL_CCERR_TCCERR_SHIFT		0x10

/* DMAQNUM bits clear */
#define DMAQNUM_CLR_MASK(ch_num)	(7 << (((ch_num) % 8) * 4))
/* DMAQNUM bits set */
#define DMAQNUM_SET_MASK(ch_num, que_num) ((7 & (que_num)) << \
					   (((ch_num) % 8) * 4))
/* QDMAQNUM bits clear */
#define QDMAQNUM_CLR_MASK(ch_num)	(7 << ((ch_num) * 4))
/* QDMAQNUM bits set */
#define QDMAQNUM_SET_MASK(ch_num, que_num) ((7 & (que_num)) << ((ch_num) * 4))


/* QUETCMAP bits clear */
#define QUETCMAP_CLR_MASK(que_num)	(7 << ((que_num) * 4))
/* QUETCMAP bits set */
#define QUETCMAP_SET_MASK(que_num, tc_num) ((7 & (tc_num)) << ((que_num) * 4))
/* QUEPRI bits clear */
#define QUEPRI_CLR_MASK(que_num)	(7 << ((que_num) * 4))
/* QUEPRI bits set */
#define QUEPRI_SET_MASK(que_num, que_pri) ((7 & (que_pri)) << ((que_num) * 4))
/* QUEWMTHR bits clear */
#define QUEWMTHR_CLR_MASK(que_num)	(0x1F << ((que_num) * 8))
/* QUEWMTHR bits set */
#define QUEWMTHR_SET_MASK(que_num, que_thr) ((0x1F & (que_thr)) << \
					     ((que_num) * 8))


/* DCHMAP-PaRAMEntry bitfield clear */
#define DMACH_PARAM_CLR_MASK		0x3FE0
/* DCHMAP-PaRAMEntry bitfield set */
#define DMACH_PARAM_SET_MASK(param_id)	(((0x3FE0 >> 5) & (param_id)) << 5)

/* QCHMAP-PaRAMEntry bitfield clear */
#define QDMACH_PARAM_CLR_MASK		0x3FE0
/* QCHMAP-PaRAMEntry bitfield set */
#define QDMACH_PARAM_SET_MASK(param_id) (((0x3FE0 >> 5) & (param_id)) << 5)
/* QCHMAP-TrigWord bitfield clear */
#define QDMACH_TRWORD_CLR_MASK		0x1C
/* QCHMAP-TrigWord bitfield set */
#define QDMACH_TRWORD_SET_MASK(param_id) (((0x1C >> 2) & (param_id)) << 2)

/* Defines needed for TC error checking */
#define EDMA_TC_ERRSTAT_BUSERR_SHIFT	0
#define EDMA_TC_ERRSTAT_TRERR_SHIFT	2
#define EDMA_TC_ERRSTAT_MMRAERR_SHIFT	3

/* Used for any TCC (Interrupt Channel) */
#define EDMA_TCC_ANY			1001
/* Used for LINK Channels */
#define DAVINCI_EDMA_PARAM_ANY		1002
/* Used for any DMA Channel */
#define EDMA_DMA_CHANNEL_ANY		1003
/* Used for any QDMA Channel */
#define EDMA_QDMA_CHANNEL_ANY		1004
/* Used for a Symmetric TCC */
#define EDMA_TCC_SYMM			1005
/* Used to find Contiguous PARAM sets from any start point */
#define EDMA_CONT_PARAMS_ANY		1006
/* Used to find Contiguous PARAM sets from a fixed point */
#define EDMA_CONT_PARAMS_FIXED_EXACT	1007
/*
 * Used to find Contiguous PARAM sets from a fixed start point
 * On failure find contiguous PARAM sets from the remaining available
 * PARAM sets
 */
#define EDMA_CONT_PARAMS_FIXED_NOT_EXACT	1008

#define edmach_has_event(lch)		(edma2event_map[(lch) >> 5] & \
					 (1 << ((lch) % 32)))

/* SoC specific EDMA3 hardware information, should be provided for a new SoC */

/* Generic defines for all the DaVinci platforms */
#define EDMA_DAVINCI_NUM_DMACH		64
#define EDMA_DAVINCI_NUM_TCC		64

/* DM644x specific EDMA3 information */
#define EDMA_DM644X_NUM_PARAMENTRY	128
#define EDMA_DM644X_NUM_EVQUE		2
#define EDMA_DM644X_NUM_TC		2
#define EDMA_DM644X_CHMAP_EXIST 	0
#define EDMA_DM644X_NUM_REGIONS 	4
#define DM644X_DMACH2EVENT_MAP0 	0x3DFF0FFC
#define DM644X_DMACH2EVENT_MAP1 	0x007F1FFF

/* DM644x specific EDMA3 Events Information */
enum dm644x_edma_ch {
	DM644X_DMACH_MCBSP_TX = 2,
	DM644X_DMACH_MCBSP_RX,
	DM644X_DMACH_VPSS_HIST,
	DM644X_DMACH_VPSS_H3A,
	DM644X_DMACH_VPSS_PRVU,
	DM644X_DMACH_VPSS_RSZ,
	DM644X_DMACH_IMCOP_IMXINT,
	DM644X_DMACH_IMCOP_VLCDINT,
	DM644X_DMACH_IMCO_PASQINT,
	DM644X_DMACH_IMCOP_DSQINT,
	DM644X_DMACH_SPI_SPIX = 16,
	DM644X_DMACH_SPI_SPIR,
	DM644X_DMACH_UART0_URXEVT0,
	DM644X_DMACH_UART0_UTXEVT0,
	DM644X_DMACH_UART1_URXEVT1,
	DM644X_DMACH_UART1_UTXEVT1,
	DM644X_DMACH_UART2_URXEVT2,
	DM644X_DMACH_UART2_UTXEVT2,
	DM644X_DMACH_MEMSTK_MSEVT,
	DM644X_DMACH_MMCRXEVT = 26,
	DM644X_DMACH_MMCTXEVT,
	DM644X_DMACH_I2C_ICREVT,
	DM644X_DMACH_I2C_ICXEVT,
	DM644X_DMACH_GPIO_GPINT0 = 32,
	DM644X_DMACH_GPIO_GPINT1,
	DM644X_DMACH_GPIO_GPINT2,
	DM644X_DMACH_GPIO_GPINT3,
	DM644X_DMACH_GPIO_GPINT4,
	DM644X_DMACH_GPIO_GPINT5,
	DM644X_DMACH_GPIO_GPINT6,
	DM644X_DMACH_GPIO_GPINT7,
	DM644X_DMACH_GPIO_GPBNKINT0,
	DM644X_DMACH_GPIO_GPBNKINT1,
	DM644X_DMACH_GPIO_GPBNKINT2,
	DM644X_DMACH_GPIO_GPBNKINT3,
	DM644X_DMACH_GPIO_GPBNKINT4,
	DM644X_DMACH_TIMER0_TINT0 = 48,
	DM644X_DMACH_TIMER1_TINT1,
	DM644X_DMACH_TIMER2_TINT2,
	DM644X_DMACH_TIMER3_TINT3,
	DM644X_DMACH_PWM0,
	DM644X_DMACH_PWM1,
	DM644X_DMACH_PWM2,
};

enum dm644x_qdma_ch {
	QDMACH0 = EDMA_DAVINCI_NUM_DMACH,
	QDMACH1,
	QDMACH2,
	QDMACH3,
	QDMACH4,
	QDMACH5,
	QDMACH6 = 71,
	QDMACH7
};
/* end DM644x specific */

/* DM646x specific EDMA3 information */
#define EDMA_DM646X_NUM_PARAMENTRY	512
#define EDMA_DM646X_NUM_EVQUE		4
#define EDMA_DM646X_NUM_TC		4
#define EDMA_DM646X_CHMAP_EXIST 	1
#define EDMA_DM646X_NUM_REGIONS 	8
#define DM646X_DMACH2EVENT_MAP0 	0x30FF1FF0
#define DM646X_DMACH2EVENT_MAP1 	0xFE3FFFFF

/* DM646x specific EDMA3 Events Information */
enum dm646x_edma_ch {
	DAVINCI_DM646X_DMA_MCASP0_AXEVTE0 = 4,
	DAVINCI_DM646X_DMA_MCASP0_AXEVTO0,
	DAVINCI_DM646X_DMA_MCASP0_AXEVT0,
	DAVINCI_DM646X_DMA_MCASP0_AREVTE0,
	DAVINCI_DM646X_DMA_MCASP0_AREVTO0,
	DAVINCI_DM646X_DMA_MCASP0_AREVT0,
	DAVINCI_DM646X_DMA_MCASP1_AXEVTE1,
	DAVINCI_DM646X_DMA_MCASP1_AXEVTO1,
	DAVINCI_DM646X_DMA_MCASP1_AXEVT1,
	DAVINCI_DM646X_DMA_IMCOP1_CP_ECDCMP1 = 43,
	DAVINCI_DM646X_DMA_IMCOP1_CP_MC1,
	DAVINCI_DM646X_DMA_IMCOP1_CP_BS1,
	DAVINCI_DM646X_DMA_IMCOP1_CP_CALC1,
	DAVINCI_DM646X_DMA_IMCOP1_CP_LPF1,
	DAVINCI_DM646X_DMA_IMCOP0_CP_ME0 = 57,
	DAVINCI_DM646X_DMA_IMCOP0_CP_IPE0,
	DAVINCI_DM646X_DMA_IMCOP0_CP_ECDCMP0,
	DAVINCI_DM646X_DMA_IMCOP0_CP_MC0,
	DAVINCI_DM646X_DMA_IMCOP0_CP_BS0,
	DAVINCI_DM646X_DMA_IMCOP0_CP_CALC0,
	DAVINCI_DM646X_DMA_IMCOP0_CP_LPF0,
};
/* end DM646X specific info */

/* DM355 specific info */
#define EDMA_DM355_NUM_PARAMENTRY	128
#define EDMA_DM355_NUM_EVQUE		2
#define EDMA_DM355_NUM_TC		2
#define EDMA_DM355_CHMAP_EXIST		0
#define EDMA_DM355_NUM_REGIONS		4
#define DM355_DMACH2EVENT_MAP0		0xCD03FFFC
#define DM355_DMACH2EVENT_MAP1		0xFF000000

/* DM355 specific EDMA3 Events Information */
enum dm355_edma_ch {
	DM355_DMA_TIMER3_TINT6 = 0,
	DM355_DMA_TIMER3_TINT7,
	DM355_DMA_MCBSP0_TX,
	DM355_DMA_MCBSP0_RX,
	DM355_DMA_MCBSP1_TX = 8,
	DM355_DMA_TIMER2_TINT4 = 8,
	DM355_DMA_MCBSP1_RX,
	DM355_DMA_TIMER2_TINT5 = 9,
	DM355_DMA_SPI2_SPI2XEVT,
	DM355_DMA_SPI2_SPI2REVT,
	DM355_DMA_IMCOP_IMXINT,
	DM355_DMA_IMCOP_SEQINT,
	DM355_DMA_SPI1_SPI1XEVT,
	DM355_DMA_SPI1_SPI1REVT,
	DM355_DMA_SPI0_SPIX0,
	DM355_DMA_SPI0_SPIR0,
	DM355_DMA_RTOINT = 24,
	DM355_DMA_GPIO_GPINT9,
	DM355_DMA_MMC0RXEVT,
	DM355_DMA_MEMSTK_MSEVT = 26,
	DM355_DMA_MMC0TXEVT,
	DM355_DMA_MMC1RXEVT = 30,
	DM355_DMA_MMC1TXEVT,
	DM355_DMA_GPIO_GPBNKINT5 = 45,
	DM355_DMA_GPIO_GPBNKINT6,
	DM355_DMA_GPIO_GPINT8,
	DM355_DMA_TIMER0_TINT1 = 49,
	DM355_DMA_TIMER1_TINT2,
	DM355_DMA_TIMER1_TINT3,
	DM355_DMA_PWM3 = 55,
	DM355_DMA_IMCOP_VLCDINT,
	DM355_DMA_IMCOP_BIMINT,
	DM355_DMA_IMCOP_DCTINT,
	DM355_DMA_IMCOP_QIQINT,
	DM355_DMA_IMCOP_BPSINT,
	DM355_DMA_IMCOP_VLCDERRINT,
	DM355_DMA_IMCOP_RCNTINT,
	DM355_DMA_IMCOP_COPCINT,
};
/* end DM355 specific info */

/* DM365 specific info */
#define EDMA_DM365_NUM_PARAMENTRY	256
#define EDMA_DM365_NUM_EVQUE		4
#define EDMA_DM365_NUM_TC		    4
#define EDMA_DM365_CHMAP_EXIST		0
#define EDMA_DM365_NUM_REGIONS		4
//#define DM365_DMACH2EVENT_MAP0		0x0C00300Cu
//#define DM365_DMACH2EVENT_MAP1		0x80000F00u
#define DM365_DMACH2EVENT_MAP0		0x0C00000Cu // Relieved DM365_DMA_IMCOP_IMX0INT etc
#define DM365_DMACH2EVENT_MAP1		0x80000000u //Relieved EMAC (40,41,42,43)

/* DM365 specific EDMA3 Events Information */
enum dm365_edma_ch {
	DM365_DMA_TIMER3_TINT6,
	DM365_DMA_TIMER3_TINT7,
	DM365_DMA_MCBSP_TX = 2,
	DM365_DMA_VCIF_TX = 2,
	DM365_DMA_MCBSP_RX = 3,
	DM365_DMA_VCIF_RX = 3,
	DM365_DMA_VPSS_EVT1,
	DM365_DMA_VPSS_EVT2,
	DM365_DMA_VPSS_EVT3,
	DM365_DMA_VPSS_EVT4,
	DM365_DMA_TIMER2_TINT4,
	DM365_DMA_TIMER2_TINT5,
	DM365_DMA_SPI2XEVT,
	DM365_DMA_SPI2REVT,
	DM365_DMA_IMCOP_IMX0INT = 12,
	DM365_DMA_KALEIDO_ARMINT = 12,
	DM365_DMA_IMCOP_SEQINT,
	DM365_DMA_SPI1XEVT,
	DM365_DMA_SPI1REVT,
	DM365_DMA_SPI0XEVT,
	DM365_DMA_SPI0REVT,
	DM365_DMA_URXEVT0 = 18,
	DM365_DMA_SPI3XEVT = 18,
	DM365_DMA_UTXEVT0 = 19,
	DM365_DMA_SPI3REVT = 19,
	DM365_DMA_URXEVT1,
	DM365_DMA_UTXEVT1,
	DM365_DMA_TIMER4_TINT8,
	DM365_DMA_TIMER4_TINT9,
	DM365_DMA_RTOINT,
	DM365_DMA_GPIONT9,
	DM365_DMA_MMC0RXEVT = 26,
	DM365_DMA_MEMSTK_MSEVT = 26,
	DM365_DMA_MMC0TXEVT,
	DM365_DMA_I2C_ICREVT,
	DM365_DMA_I2C_ICXEVT,
	DM365_DMA_MMC1RXEVT,
	DM365_DMA_MMC1TXEVT,
	DM365_DMA_GPIOINT0,
	DM365_DMA_GPIOINT1,
	DM365_DMA_GPIOINT2,
	DM365_DMA_GPIOINT3,
	DM365_DMA_GPIOINT4,
	DM365_DMA_GPIOINT5,
	DM365_DMA_GPIOINT6,
	DM365_DMA_GPIOINT7,
	DM365_DMA_GPIOINT10 = 40,
	DM365_DMA_EMAC_RXTHREESH = 40,
	DM365_DMA_GPIOINT11 = 41,
	DM365_DMA_EMAC_RXPULSE = 41,
	DM365_DMA_GPIOINT12 = 42,
	DM365_DMA_EMAC_TXPULSE = 42,
	DM365_DMA_GPIOINT13 = 43,
	DM365_DMA_EMAC_MISCPULSE = 43,
	DM365_DMA_GPIOINT14 = 44,
	DM365_DMA_SPI4XEVT = 44,
	DM365_DMA_GPIOINT15 = 45,
	DM365_DMA_SPI4REVT = 45,
	DM365_DMA_ADC_ADINT,
	DM365_DMA_GPIOINT8,
	DM365_DMA_TIMER0_TINT0,
	DM365_DMA_TIMER0_TINT1,
	DM365_DMA_TIMER1_TINT2,
	DM365_DMA_TIMER1_TINT3,
	DM365_DMA_PWM0,
	DM365_DMA_PWM1 = 53,
	DM365_DMA_IMCOP_IMX1INT = 53,
	DM365_DMA_PWM2 = 54,
	DM365_DMA_IMCOP_NSFINT = 54,
	DM365_DMA_PWM3 = 55,
	DM365_DMA_KALEIDO6_CP_UNDEF = 55,
	DM365_DMA_IMCOP_VLCDINT = 56,
	DM365_DMA_KALEIDO5_CP_ECDCMP = 56,
	DM365_DMA_IMCOP_BIMINT = 57,
	DM365_DMA_KALEIDO8_CP_ME = 57,
	DM365_DMA_IMCOP_DCTINT = 58,
	DM365_DMA_KALEIDO1_CP_CALC = 58,
	DM365_DMA_IMCOP_QIQINT = 59,
	DM365_DMA_KALEIDO7_CP_IPE = 59,
	DM365_DMA_IMCOP_BPSINT = 60,
	DM365_DMA_KALEIDO2_CP_BS = 60,
	DM365_DMA_IMCOP_VLCDERRINT = 61,
	DM365_DMA_KALEIDO0_CP_LPF = 61,
	DM365_DMA_IMCOP_RCNTINT = 62,
	DM365_DMA_KALEIDO3_CP_MC = 62,
	DM365_DMA_IMCOP_COPCINT = 63,
	DM365_DMA_KALEIDO4_CP_ECDEND = 63,
};
/* end DM365 specific info */

/* DA8xx specific EDMA3 information */
#define EDMA_DA8XX_NUM_DMACH		32
#define EDMA_DA8XX_NUM_TCC		32
#define EDMA_DA8XX_NUM_PARAMENTRY	128
#define EDMA_DA8XX_NUM_EVQUE		2
#define EDMA_DA8XX_NUM_TC		2
#define EDMA_DA8XX_CHMAP_EXIST		0
#define EDMA_DA8XX_NUM_REGIONS		4
#define DA8XX_DMACH2EVENT_MAP0		0x000FC03Fu
#define DA8XX_DMACH2EVENT_MAP1		0x00000000u
#define DA8XX_EDMA_ARM_OWN		0x30FFCCFFu

/* DA8xx specific EDMA3 Events Information */
enum da8xx_edma_ch {
	DA8XX_DMACH_MCASP0_RX,
	DA8XX_DMACH_MCASP0_TX,
	DA8XX_DMACH_MCASP1_RX,
	DA8XX_DMACH_MCASP1_TX,
	DA8XX_DMACH_MCASP2_RX,
	DA8XX_DMACH_MCASP2_TX,
	DA8XX_DMACH_GPIO_BNK0INT,
	DA8XX_DMACH_GPIO_BNK1INT,
	DA8XX_DMACH_UART0_RX,
	DA8XX_DMACH_UART0_TX,
	DA8XX_DMACH_TMR64P0_EVTOUT12,
	DA8XX_DMACH_TMR64P0_EVTOUT34,
	DA8XX_DMACH_UART1_RX,
	DA8XX_DMACH_UART1_TX,
	DA8XX_DMACH_SPI0_RX,
	DA8XX_DMACH_SPI0_TX,
	DA8XX_DMACH_MMCSD_RX,
	DA8XX_DMACH_MMCSD_TX,
	DA8XX_DMACH_SPI1_RX,
	DA8XX_DMACH_SPI1_TX,
	DA8XX_DMACH_DMAX_EVTOUT6,
	DA8XX_DMACH_DMAX_EVTOUT7,
	DA8XX_DMACH_GPIO_BNK2INT,
	DA8XX_DMACH_GPIO_BNK3INT,
	DA8XX_DMACH_I2C0_RX,
	DA8XX_DMACH_I2C0_TX,
	DA8XX_DMACH_I2C1_RX,
	DA8XX_DMACH_I2C1_TX,
	DA8XX_DMACH_GPIO_BNK4INT,
	DA8XX_DMACH_GPIO_BNK5INT,
	DA8XX_DMACH_UART2_RX,
	DA8XX_DMACH_UART2_TX
};
/* End DA8xx specific info */

/* ch_status paramater of callback function possible values */
enum edma_status {
	DMA_COMPLETE = 1,
	DMA_EVT_MISS_ERROR,
	QDMA_EVT_MISS_ERROR,
	DMA_CC_ERROR,
	DMA_TC1_ERROR,
	DMA_TC2_ERROR,
	DMA_TC3_ERROR,
};

enum address_mode {
	INCR = 0,
	FIFO = 1
};

enum fifo_width {
	W8BIT = 0,
	W16BIT = 1,
	W32BIT = 2,
	W64BIT = 3,
	W128BIT = 4,
	W256BIT = 5
};

enum dma_event_q {
	EVENTQ_0 = 0,
	EVENTQ_1 = 1,
	EVENTQ_2 = 2,
	EVENTQ_3 = 3,
};

enum sync_dimension {
	ASYNC = 0,
	ABSYNC = 1
};

enum resource_type {
	RES_DMA_CHANNEL = 0,
	RES_QDMA_CHANNEL = 1,
	RES_TCC = 2,
	RES_PARAM_SET = 3
};

#ifdef __KERNEL__
/*
 * davinci_get_qdma_channel: Convert QDMA channel to logical channel
 * Arguments:
 *      ch      - input QDMA channel.
 *
 * Return: logical channel associated with QDMA channel or logical channel
 *      associated with QDMA channel 0 for out of range channel input.
 */
int davinci_get_qdma_channel(int ch);

/*
 * davinci_request_dma - request for the Davinci DMA channel
 *
 * dev_id - DMA channel number
 *
 * EX: DAVINCI_DMA_MCBSP_TX - For requesting a DMA MasterChannel with MCBSP_TX
 *     event association
 *
 *     EDMA_DMA_CHANNEL_ANY - For requesting a DMA MasterChannel which does
 *                            not have event association
 *
 *     DAVINCI_EDMA_PARAM_ANY - for requesting a DMA SlaveChannel
 *
 * dev_name   - name of the DMA channel in human readable format
 * callback   - channel callback function (valid only if you are requesting
 *              for a DMA MasterChannel)
 * data       - private data for the channel to be requested
 * lch        - contains the device ID allocated
 * tcc        - specifies the channel number on which the interrupt is
 *              generated
 *              Valid for QDMA and PaRAM channels
 * eventq_no  - Event Queue number with which the channel will be associated
 *              (valid only if you are requesting for a DMA MasterChannel)
 *              Values : EVENTQ_0/EVENTQ_1 for event queue 0/1.
 *
 * Return: zero on success,
 *         -EINVAL - if the requested channel is not supported on the ARM
 *		     side events
 */
int davinci_request_dma(int dev_id,
			const char *dev_name,
			void (*callback) (int lch, unsigned short ch_status,
					  void *data), void *data, int *lch,
			int *tcc, enum dma_event_q);

/*
 * davinci_set_dma_src_params - DMA source parameters setup
 *
 * lch         - channel for which the source parameters to be configured
 * src_port    - Source port address
 * mode        - indicates whether the address mode is FIFO or not
 * width       - valid only if addressMode is FIFO, indicates the width of FIFO
 *             0 - 8 bit
 *             1 - 16 bit
 *             2 - 32 bit
 *             3 - 64 bit
 *             4 - 128 bit
 *             5 - 256 bit
 */
int davinci_set_dma_src_params(int lch, u32 src_port,
			       enum address_mode mode, enum fifo_width width);

/*
 * davinci_set_dma_dest_params - DMA destination parameters setup
 *
 * lch         - channel or param device for destination parameters are to be
 *               configured
 * dest_port   - Destination port address
 * mode        - indicates whether the address mode is FIFO or not
 * width       - valid only if addressMode is FIFO,indicates the width of FIFO
 *             0 - 8 bit
 *             1 - 16 bit
 *             2 - 32 bit
 *             3 - 64 bit
 *             4 - 128 bit
 *             5 - 256 bit
 */
int davinci_set_dma_dest_params(int lch, u32 dest_port,
				enum address_mode mode, enum fifo_width width);

/*
 * davinci_set_dma_src_index - DMA source index setup
 *
 * lch     - channel or param device for configuration of source index
 * srcbidx - source B-register index
 * srccidx - source C-register index
 */
int davinci_set_dma_src_index(int lch, u16 srcbidx, u16 srccidx);

/*
 * davinci_set_dma_dest_index - DMA destination index setup
 *
 * lch      - channel or param device for configuration of destination index
 * destbidx - dest B-register index
 * destcidx - dest C-register index
 */
int davinci_set_dma_dest_index(int lch, u16 destbidx, u16 destcidx);

/*
 * davinci_set_dma_transfer_params -  DMA transfer parameters setup
 *
 * lch  - channel or param device for configuration of aCount, bCount and
 *        cCount regs.
 * aCnt - aCnt register value to be configured
 * bCnt - bCnt register value to be configured
 * cCnt - cCnt register value to be configured
 */
int davinci_set_dma_transfer_params(int lch, u16 acnt, u16 bcnt, u16 ccnt,
				    u16 bcntrld,
				    enum sync_dimension sync_mode);

/*
 * davinci_set_dma_params -
 * ARGUMENTS:
 *      lch - logical channel number
 */
int davinci_set_dma_params(int lch, struct paramentry_descriptor *d);

/*
 * davinci_get_dma_params -
 * ARGUMENTS:
 *      lch - logical channel number
 */
int davinci_get_dma_params(int lch, struct paramentry_descriptor *d);

/*
 * davinci_start_dma -  Starts the DMA on the channel passed
 *
 * lch - logical channel number
 *
 * Note:    This API can be used only on DMA MasterChannel
 *
 * Return: zero on success
 *        -EINVAL on failure, i.e if requested for the slave channels
 */
int davinci_start_dma(int lch);

/*
 * davinci_stop_dma -  Stops the DMA on the channel passed
 *
 * lch - logical channel number
 *
 * Note:    This API can be used on MasterChannel and SlaveChannel
 */
int davinci_stop_dma(int lch);

/*
 * davinci_dma_link_lch - Link two Logical channels
 *
 * lch_head  - logical channel number in which the link field is linked to
 *             the param pointed to by lch_queue
 *             Can be a MasterChannel or SlaveChannel
 * lch_queue - logical channel number or the param entry number, which is to be
 *             linked to the lch_head
 *             Must be a SlaveChannel
 *
 *                     |---------------|
 *                     v               |
 *      Ex:    ch1--> ch2-->ch3-->ch4--|
 *
 *             ch1 must be a MasterChannel
 *
 *             ch2, ch3, ch4 must be SlaveChannels
 *
 * Note:       After channel linking the user should not update any PaRAM entry
 *             of MasterChannel ( In the above example ch1 )
 */
int davinci_dma_link_lch(int lch_head, int lch_queue);

/*
 * davinci_dma_unlink_lch - unlink the two logical channels passed through by
 *                          setting the link field of head to 0xffff.
 *
 * lch_head  - logical channel number from which the link field is to be
 *             removed
 * lch_queue - logical channel number or the PaRAM entry number which is to be
 *             unlinked from lch_head
 */
int davinci_dma_unlink_lch(int lch_head, int lch_queue);

/*
 * DMA channel chain - chains the two logical channels passed through by
 * ARGUMENTS:
 * lch_head - logical channel number from which the link field is to be removed
 * lch_queue - logical channel number or the PaRAM entry number which is to be
 *             unlinked from lch_head
 */
int davinci_dma_chain_lch(int lch_head, int lch_queue);

/*
 * DMA channel unchain - unchain the two logical channels passed through by
 * ARGUMENTS:
 * lch_head - logical channel number, from which the link field is to be removed
 * lch_queue - logical channel number or the param entry number, which is to be
 *             unlinked from lch_head
 */
int davinci_dma_unchain_lch(int lch_head, int lch_queue);

/*
 * Free DMA channel - Free the DMA channel number passed
 *
 * ARGUMENTS:
 * lch - DMA channel number to set free
 */
int davinci_free_dma(int lch);

/*
 * davinci_clean_channel - clean PaRAM entry and bring back EDMA to initial
 *			   state if media has been removed before EDMA has
 *			   finished.  It is useful for removable media.
 * Arguments:
 * lch		- logical channel number
 */
void davinci_clean_channel(int lch);

/*
 * davinci_dma_getposition - returns the current transfer points for the DMA
 *			     source and destination
 * Arguments:
 * lch		- logical channel number
 * src		- source port position
 * dst		- destination port position
 */
void davinci_dma_getposition(int lch, dma_addr_t *src, dma_addr_t *dst);

/*
 * davinci_dma_clear_event - clear an outstanding event on the DMA channel
 * Arguments:
 *     lch - logical channel number
 */
void davinci_dma_clear_event(int lch);

/*
 * The API will return the PARAM associated with a logical channel
 * Return value on success will be the value of the PARAM
 * Arguments:
 *     lch - logical channel number
 */
int davinci_get_param(int lch);

/*
 * The API will return the TCC associated with a logical channel
 * Return value on success will be the value of the TCC
 * Arguments:
 *     lch - logical channel number
 */
int davinci_get_tcc(int lch);

/*
 * The API will return the starting point of a set of
 * contiguous PARAM sets that have been requested
 * Return value on failure will be -1
 * Arguments:
 *	res_id	- can only be EDMA_CONT_PARAMS_ANY or
 *		  EDMA_CONT_PARAMS_FIXED_EXACT or
 *		  EDMA_CONT_PARAMS_FIXED_NOT_EXACT
 *	num	- number of contiguous PARAM sets
 *	param	- the start value of PARAM that should be passed if res_id
 *		  is EDMA_CONT_PARAMS_FIXED_EXACT or
 *		  EDMA_CONT_PARAMS_FIXED_NOT_EXACT
 */
int davinci_request_params(unsigned int res_id, unsigned int num,
			   unsigned int param);

/*
 * The API will free a set of contiguous PARAM sets
 * Arguments:
 *	start	- start location of PARAM sets
 *	num	- num of contiguous PARAM sets
 */
void davinci_free_params(unsigned int start, unsigned int num);

#endif /* __KERNEL__ */

#endif
