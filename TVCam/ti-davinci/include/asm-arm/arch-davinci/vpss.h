/*
 * Copyright (C) 2008-2009 Texas Instruments Inc
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
 *
 *  Description: File for common vpss registers shared across VPBE and VPFE
 *  This file has APIs for set/get registers in VPSS and expected to
 *  be used by all video drivers.
 */
#ifndef _VPSS_COMMON_H
#define _VPSS_COMMON_H
#ifdef __KERNEL__
#include <linux/interrupt.h>

/**************** DM355 registers ********************************/
#define DM355_VPSS_REG_BASE		0x01C70000
#define DM355_VPSSBL_REG_BASE		0x01C70800

/* DM355 VPSS Clock control registers */
#define DM355_VPSSCLK_CLKCTRL		0x04
#define DM355_VPSS_INTSEL		0x10
#define DM355_VPSS_EVTSEL		0x14
#define DM355_VPSS_INTSTAT		0x0C
/* DM355 VPSS buffer logic registers */
#define DM355_VPSSBL_MEMCTRL		0x18
#define DM355_VPSSBL_PCR		0x4

/**************** DM365 registers ********************************/
#define DM365_ISP_REG_BASE		0x01c70000
#define DM365_PCCR			0x04
#define DM365_BCR			0x08
#define DM365_VPSS_INTSTAT		0x0C
#define DM365_VPSS_INTSEL1		0x10
#define DM365_VPSS_INTSEL2		0x14
#define DM365_VPSS_INTSEL3		0x18
#define DM365_VPSS_EVTSEL		0x1C

#define DM365_VPSS_REG_BASE		0x01c70200
#define DM365_VPBE_CLK_CTRL		0x00

#define DM6446_MAX_VPSS_CPU_IRQ 	8
#define DM3XX_MAX_VPSS_CPU_IRQ 		9
#define DM3XX_DYNAMIC_IRQ_START 	4

#define DM355_IRQ_VAL_INVALID		0xf
#define DM365_IRQ_VAL_INVALID		0x1f

/* vpss modules use the enum below to request interrupt */
enum vpss_irq {
	VPSS_VDINT0 = 0,
	VPSS_VDINT1,
	VPSS_VDINT2,
	VPSS_HISTINT,
	VPSS_H3AINT,
	/* Only in 6446 */
	VPSS_PRVUINT,
	/* Only for 6446 */
	VPSS_RSZINT,
	VPSS_VENCINT,
	VPSS_IPIPEIFINT,
	/* DMA */
	VPSS_IPIPE_INT_SDR,
	VPSS_IPIPE_INT_RZA,
	VPSS_IPIPE_INT_RZB,
	VPSS_IPIPE_INT_BSC,
	VPSS_IPIPE_INT_MMR,
	VPSS_CFALDINT,
	VPSS_HSSIINT,
	VPSS_OSDINT,
	VPSS_INT3,
	VPSS_IPIPE_INT_REG,
	VPSS_IPIPE_INT_LAST_PIX,
	VPSS_AEW_INT,
	VPSS_AF_INT,
	VPSS_RSZ_INT_REG,
	VPSS_RSZ_INT_LAST_PIX,
	/* DMA */
	VPSS_RSZ_INT_SDR,
	VPSS_LDC_INT,
	VPSS_FDIF_INT,
	VPSS_RSZ_INT_EOF0,
	VPSS_RSZ_INT_EOF1,
	VPSS_H3A_INT_EOF,
	VPSS_IPIPE_INT_EOF,
	VPSS_LDC_INT_EOF,
	VPSS_FD_INT_EOF,
	VPSS_IPIPE_INT_DPC_INI,
	VPSS_IPIPE_INT_DPC_RENEW0,
	VPSS_IPIPE_INT_DPC_RENEW1,
	VPSS_INT_LAST
};

struct vpss_irq_table_entry {
	/* irq number */
	enum vpss_irq irq_no;
	/* irq requested to the cpu */
	unsigned int cpu_irq;
	/* For dynamic irq only. 1 - in use */
	unsigned char in_use;
};

/* PCCR for cloclk control */
/* Used for enable/diable VPSS Clock */
enum vpss_clock_sel {
	/* DM355/DM365 */
	VPSS_CCDC_CLOCK,
	VPSS_IPIPE_CLOCK,
	VPSS_H3A_CLOCK,
	VPSS_CFALD_CLOCK,
	/*
	 * When using VPSS_VENC_CLOCK_SEL in vpss_enable_clock() api
	 * following applies:-
	 * en = 0 selects ENC_CLK
	 * en = 1 selects ENC_CLK/2
	 */
	VPSS_VENC_CLOCK_SEL,
	VPSS_VPBE_CLOCK,
	/* DM365 only clocks */
	VPSS_IPIPEIF_CLOCK,
	VPSS_RSZ_CLOCK,
	VPSS_BL_CLOCK,
	/*
	 * When using VPSS_PCLK_INTERNAL in vpss_enable_clock() api
	 * following applies:-
	 * en = 0 disable internal PCLK
	 * en = 1 enables internal PCLK
	 */
	VPSS_PCLK_INTERNAL,
	/*
	 * When using VPSS_PSYNC_CLOCK_SEL in vpss_enable_clock() api
	 * following applies:-
	 * en = 0 enables MMR clock
	 * en = 1 enables VPSS clock
	 */
	VPSS_PSYNC_CLOCK_SEL,
	VPSS_LDC_CLOCK_SEL,
	VPSS_OSD_CLOCK_SEL,
	VPSS_FDIF_CLOCK,
	VPSS_LDC_CLOCK
};

enum vpss_dfc_mem_sel {
	VPSS_DFC_SEL_IPIPE,
	VPSS_DFC_SEL_CCDC
};

enum vpss_rsz_cfald_mem_sel {
	VPSS_IPIPE_MEM,
	VPSS_CFALD_MEM
};

enum vpss_bl1_src_sel {
	VPSS_BL1_SRC_SEL_IPIPE,
	VPSS_BL1_SRC_SEL_ISIF
};

enum vpss_bl2_src_sel {
	VPSS_BL2_SRC_SEL_LDC,
	VPSS_BL2_SRC_SEL_IPIPE
};

struct vpss_oper_cfg {
	/* irq table start */
	struct vpss_irq_table_entry *irq_table;
	/* Maximum entries in the table */
	unsigned char max_entries;
	/* Start of dynamic entries in the irq table */
	unsigned char dynamic_irq_start;
	/* enable irq to interrupt controller */
	int (*enable_irq)(enum vpss_irq irq,
		unsigned int cpu_irq, unsigned char en);
	/* enable clock */
	int (*enable_clock)(enum vpss_clock_sel clock_sel, unsigned int en);
	/* spin lock */
	spinlock_t vpss_lock;
};

/* 1 - enable the clock, 0 - disable the clock */
int vpss_enable_clock(enum vpss_clock_sel clock_sel, unsigned int en);
void vpss_dfc_memory_sel(enum vpss_dfc_mem_sel dfc_mem_sel);
void vpss_ipipe_enable_any_address(int en);
void vpss_rsz_cfald_mem_select(enum vpss_rsz_cfald_mem_sel mem_sel);
/* vpss drivers use this api's to allocate and/or  request irq */
int vpss_request_irq(enum vpss_irq,
		     irqreturn_t(*handler) (int, void *, struct pt_regs *),
		     unsigned long, const char *, void *);
/* vpss drivers use this api's to free and/or de-allocate irq */
void vpss_free_irq(enum vpss_irq, void *);
/* vpss buffer logic1 source select. IPIPE or ISIF. Only for DM365 */
void vpss_bl1_src_select(enum vpss_bl1_src_sel sel);
/* vpss buffer logic2 source select. IPIPE or LDC. Only for DM365 */
void vpss_bl2_src_select(enum vpss_bl2_src_sel sel);

/* enable or disable irq to arm */
void vpss_enable_irq(enum vpss_irq, int en);
/*
 * clear vpss interrupt, return status of interrupt line, 0 - interrupted
 * and cleared, -1 - no interrupt
 */
int vpss_clear_interrupt(enum vpss_irq);
#endif
#endif
