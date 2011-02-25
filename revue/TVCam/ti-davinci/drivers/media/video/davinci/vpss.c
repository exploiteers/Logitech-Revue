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
 /* File vpss.c : File for common vpss utilities shared across VPBE and VPFE.
  *
  */

#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/arch/io.h>
#include <asm/arch/cpu.h>
#include <media/davinci/vpss.h>

/* lock to write into common register */
static spinlock_t vpss_lock;

/* irq no, pre_allocated , in_use */
/* DM6446 vpss irq to INTC IRQ map */
static struct vpss_irq_table_entry
 dm6446_vpss_irq_table[DM6446_MAX_VPSS_CPU_IRQ] = {
	{VPSS_VDINT0, IRQ_DM644X_VDINT0, 1, 1},
	{VPSS_VDINT1, IRQ_DM644X_VDINT1, 1, 1},
	{VPSS_VDINT2, IRQ_DM644X_VDINT2, 1, 1},
	{VPSS_HISTINT, IRQ_DM644X_HISTINT, 1, 1},
	{VPSS_H3AINT, IRQ_DM644X_H3AINT, 1, 1},
	{VPSS_PRVUINT, IRQ_DM644X_PRVUINT, 1, 1},
	{VPSS_RSZINT, IRQ_DM644X_RSZINT, 1, 1},
	{VPSS_VENCINT, IRQ_DM644X_VENCINT, 1, 1}
};

/* DM355 vpss irq to INTC IRQ map */
static struct vpss_irq_table_entry dm355_vpss_irq_table[DM3XX_MAX_VPSS_CPU_IRQ]
    = {
	{VPSS_VDINT0, IRQ_DM355_CCDC_VDINT0, 1, 1},
	{VPSS_VDINT1, IRQ_DM355_CCDC_VDINT1, 1, 1},
	{VPSS_H3AINT, IRQ_DM355_H3AINT, 1, 1},
	{VPSS_VENCINT, IRQ_DM355_VENCINT, 1, 1},
	/* IPIPE_INT_SDR is shared with VPSS_RSZ_INT_SDR. But one
	   of them is always allocated. First entry of dynamically
	   allocated INT is reserved for IPIPE SDR interrupts 
	 */
	{VPSS_INT_LAST, IRQ_DM355_IPIPE_SDR, 0, 0},
	{VPSS_INT_LAST, IRQ_DM355_IPIPE_HST, 0, 0},
	{VPSS_INT_LAST, IRQ_DM355_CCDC_VDINT2, 0, 0},
	{VPSS_INT_LAST, IRQ_DM355_IPIPEIFINT, 0, 0},
	{VPSS_INT_LAST, IRQ_DM355_OSDINT, 0, 0}
};

/* DM355 vpss irq to vpss irq - register value map */
static unsigned char dm355_vpssirq_regval_map[VPSS_INT_LAST + 1] = {
	0,			/* VPSS_VDINT0 */
	1,			/* VPSS_VDINT1 */
	2,			/* VPSS_VDINT2 */
	7,			/* VPSS_HISTINT */
	3,			/* VPSS_H3AINT */
	DM355_IRQ_VAL_INVALID,	/* VPSS_PRVUINT */
	DM355_IRQ_VAL_INVALID,	/* VPSS_RSZINT */
	4,			/* VPSS_VENCINT */
	6,			/* VPSS_IPIPEIFINT */
	8,			/* VPSS_IPIPE_INT_SDR */
	9,			/* VPSS_IPIPE_INT_RZA */
	10,			/* VPSS_IPIPE_INT_RZB */
	11,			/* VPSS_IPIPE_INT_BSC */
	12,			/* VPSS_IPIPE_INT_MMR */
	13,			/* VPSS_CFALDINT */
	14,			/* VPSS_HSSIINT */
	5,			/* VPSS_OSDINT */
	DM355_IRQ_VAL_INVALID,	/* VPSS_INT3 */
	DM355_IRQ_VAL_INVALID,	/* VPSS_IPIPE_INT_REG */
	DM355_IRQ_VAL_INVALID,	/* VPSS_IPIPE_INT_LAST_PIX */
	DM355_IRQ_VAL_INVALID,	/* VPSS_AEW_INT */
	DM355_IRQ_VAL_INVALID,	/* VPSS_AF_INT */
	DM355_IRQ_VAL_INVALID,	/* VPSS_RSZ_INT_REG */
	DM355_IRQ_VAL_INVALID,	/* VPSS_RSZ_INT_LAST_PIX */
	DM355_IRQ_VAL_INVALID,	/* VPSS_RSZ_INT_SDR */
	DM355_IRQ_VAL_INVALID,	/* VPSS_LDC_INT */
	DM355_IRQ_VAL_INVALID,	/* VPSS_FDIF_INT */
	DM355_IRQ_VAL_INVALID,	/* VPSS_RSZ_INT_EOF0 */
	DM355_IRQ_VAL_INVALID,	/* VPSS_RSZ_INT_EOF1 */
	DM355_IRQ_VAL_INVALID,	/* VPSS_H3A_INT_EOF */
	DM355_IRQ_VAL_INVALID,	/* VPSS_IPIPE_INT_EOF */
	DM355_IRQ_VAL_INVALID,	/* VPSS_LDC_INT_EOF */
	DM355_IRQ_VAL_INVALID,	/* VPSS_FD_INT_EOF */
	DM355_IRQ_VAL_INVALID,	/* VPSS_IPIPE_INT_DPC_INI */
	DM355_IRQ_VAL_INVALID,	/* VPSS_IPIPE_INT_DPC_RENEW0 */
	DM355_IRQ_VAL_INVALID,	/* VPSS_IPIPE_INT_DPC_RENEW1 */
	DM355_IRQ_VAL_INVALID,	/* VPSS_INT_LAST */
};

/* DM365 vpss irq to INTC IRQ map */
static struct vpss_irq_table_entry dm365_vpss_irq_table[DM3XX_MAX_VPSS_CPU_IRQ]
    = {
	{VPSS_VDINT0, IRQ_DM365_VPSSINT0, 1, 1},
	{VPSS_VDINT1, IRQ_DM365_VPSSINT1, 1, 1},
	{VPSS_H3AINT, IRQ_DM365_VPSSINT2, 1, 1},
	{VPSS_VENCINT, IRQ_DM365_VPSSINT3, 1, 1},
	/* IPIPE_INT_SDR is shared with VPSS_RSZ_INT_SDR. But one
	   of them is always allocated. First entry of dynamically
	   allocated INT is reserved for IPIPE SDR interrupts 
	 */
	{VPSS_INT_LAST, IRQ_DM365_VPSSINT4, 0, 0},
	/* Below irqs are allocated dynamically */
	{VPSS_INT_LAST, IRQ_DM365_VPSSINT5, 0, 0},
	{VPSS_INT_LAST, IRQ_DM365_VPSSINT6, 0, 0},
	{VPSS_INT_LAST, IRQ_DM365_VPSSINT7, 0, 0},
	{VPSS_INT_LAST, IRQ_DM365_VPSSINT8, 0, 0}
};

/* DM365 vpss irq to vpss irq - register value map */
static unsigned char dm365_vpssirq_regval_map[VPSS_INT_LAST + 1] = {
	0,			/* VPSS_VDINT0 */
	1,			/* VPSS_VDINT1 */
	2,			/* VPSS_VDINT2 */
	8,			/* VPSS_HISTINT */
	12,			/* VPSS_H3AINT */
	DM365_IRQ_VAL_INVALID,	/* VPSS_PRVUINT */
	DM365_IRQ_VAL_INVALID,	/* VPSS_RSZINT */
	21,			/* VPSS_VENCINT */
	9,			/* VPSS_IPIPEIFINT */
	6,			/* VPSS_IPIPE_INT_SDR */
	16,			/* VPSS_IPIPE_INT_RZA */
	17,			/* VPSS_IPIPE_INT_RZB */
	7,			/* VPSS_IPIPE_INT_BSC */
	DM365_IRQ_VAL_INVALID,	/* VPSS_IPIPE_INT_MMR */
	DM365_IRQ_VAL_INVALID,	/* VPSS_CFALDINT */
	DM365_IRQ_VAL_INVALID,	/* VPSS_HSSIINT */
	20,			/* VPSS_OSDINT */
	3,			/* VPSS_INT3 */
	4,			/* VPSS_IPIPE_INT_REG */
	5,			/* VPSS_IPIPE_INT_LAST_PIX */
	10,			/* VPSS_AEW_INT */
	11,			/* VPSS_AF_INT */
	13,			/* VPSS_RSZ_INT_REG */
	14,			/* VPSS_RSZ_INT_LAST_PIX */
	15,			/* VPSS_RSZ_INT_SDR */
	18,			/* VPSS_LDC_INT */
	19,			/* VPSS_FDIF_INT */
	22,			/* VPSS_RSZ_INT_EOF0 */
	23,			/* VPSS_RSZ_INT_EOF1 */
	24,			/* VPSS_H3A_INT_EOF */
	25,			/* VPSS_IPIPE_INT_EOF */
	26,			/* VPSS_LDC_INT_EOF */
	27,			/* VPSS_FD_INT_EOF */
	28,			/* VPSS_IPIPE_INT_DPC_INI */
	29,			/* VPSS_IPIPE_INT_DPC_RENEW0 */
	30,			/* VPSS_IPIPE_INT_DPC_RENEW1 */
	DM365_IRQ_VAL_INVALID	/* VPSS_INT_LAST */
};

int dm355_enable_irq(enum vpss_irq irq, unsigned int cpu_irq, unsigned char en)
{
	u32 val = 0, temp, shift = 0, mask = 0xf;
	unsigned int offset = DM355_VPSS_INTSEL;
	if (cpu_irq == IRQ_DM355_VENCINT) {
		offset = DM355_VPSS_EVTSEL;
	} else {
		shift = (((u32) cpu_irq) << 2);
	}
	temp = davinci_readl((DM355_VPSSBL_REG_BASE + offset));
	val = dm355_vpssirq_regval_map[irq];
	if (val == DM355_IRQ_VAL_INVALID) {
		printk(KERN_ERR "Invalid vpss irq requested\n");
		return -1;
	}
	temp &= (~(mask << shift));
	if (en) {
		temp |= ((val & mask) << shift);
	}
	davinci_writel(temp, (DM355_VPSSBL_REG_BASE + offset));
	return 0;
}

static int dm365_enable_irq(enum vpss_irq irq,
			    unsigned int cpu_irq, unsigned char en)
{
	u32 val = 0, temp, shift = 0, mask = 0x1f;
	unsigned int offset = DM365_VPSS_INTSEL1;
	if (cpu_irq <= IRQ_DM365_VPSSINT3) {
		shift = (((u32) cpu_irq) << 3);
	} else if ((cpu_irq > IRQ_DM365_VPSSINT3)
		   && (cpu_irq <= IRQ_DM365_VPSSINT7)) {
		shift = (((u32) cpu_irq & 3) << 3);
		offset = DM365_VPSS_INTSEL2;
	} else {
		offset = DM365_VPSS_INTSEL3;
	}
	val = dm365_vpssirq_regval_map[irq];
	if (val == DM365_IRQ_VAL_INVALID) {
		printk(KERN_ERR "Invalid vpss irq requested\n");
		return -1;
	}
	temp = davinci_readl((DM365_ISP_REG_BASE + offset));
	temp &= ((~mask) << shift);
	if (en) {
		temp |= ((val & mask) << shift);
	}
	davinci_writel(temp, (DM365_ISP_REG_BASE + offset));
	return 0;
}

/**
 *  vpss_request_irq - allocate an interrupt line
 *  @irq: vpss interrupt line
 *  @handler: Function to be called when the IRQ occurs
 *  @irqflags: Interrupt type flags
 *  @devname: An ascii name for the claiming device
 *  @dev_id: A cookie passed back to the handler function
 *
 *  This is called by vpss drivers to request a pre-allocated
 *  or shared irq and attach a handler. This allocates irq if
 *  shared and fail if there are no irq line to use. returns
 *  zero if success, non-zero on failure 
 */
int vpss_request_irq(enum vpss_irq irq,
		     irqreturn_t(*handler) (int, void *, struct pt_regs *),
		     unsigned long irqflags, const char *devname, void *dev_id)
{
	struct vpss_irq_table_entry *start, *temp;
	unsigned char max_entries, dynamic_irq_start;
	int i, j, found = 0;
	unsigned long flags;
    // KC: added to bypass installing of LSP ISR
    if(irq==VPSS_H3AINT
    || irq==VPSS_VDINT0
    ) {
      return 0;
    }

	if (cpu_is_davinci_dm355()) {
		start = temp = dm355_vpss_irq_table;
		max_entries = DM3XX_MAX_VPSS_CPU_IRQ;
		dynamic_irq_start = DM3XX_DYNAMIC_IRQ_START;

	} else if (cpu_is_davinci_dm365()) {
		start = temp = dm365_vpss_irq_table;
		max_entries = DM3XX_MAX_VPSS_CPU_IRQ;
		dynamic_irq_start = DM3XX_DYNAMIC_IRQ_START;
	} else {
		/* DM6446 */
		start = temp = dm6446_vpss_irq_table;
		max_entries = DM6446_MAX_VPSS_CPU_IRQ;
		dynamic_irq_start = DM6446_MAX_VPSS_CPU_IRQ;
	}

	/* First check for pre-allocated interrupts */
	for (i = 0; i < dynamic_irq_start; i++) {
		/* first check if this irq is pre-allocated */
		if (irq == temp->irq_no) {
			/* a pre-allocated and match found */
			found = 1;
			break;
		}
		temp++;
	}

	/* This is a pre-allocated irq. Just request the IRQ to CPU */
	if (found)
		return (request_irq
			(temp->cpu_irq, handler, irqflags, devname, dev_id));

	/* We have a dynamic allocated irq */
	spin_lock_irqsave(&vpss_lock, flags);
	temp = start + dynamic_irq_start;
	for (i = dynamic_irq_start; i < max_entries; i++) {
		if (i == dynamic_irq_start) {
			/* First entry is reserved for IPIPE/RSZ for DM355 & DM365 */
			if ((irq == VPSS_IPIPE_INT_SDR) ||
			    (irq == VPSS_RSZ_INT_SDR)) {
				if (temp->in_use) {
					printk(KERN_ERR
					       "IPIPE/RSZ IRQ is in use\n");
					spin_unlock_irqrestore(&vpss_lock,
							       flags);
					return -1;
				}
				found = 1;
				break;
			}
		} else {
			if (!temp->in_use) {
				found = 1;
				break;
			}
		}
		temp++;
	}

	if (i == max_entries) {
		printk(KERN_WARNING "No VPSS IRQ resource available\n");
		spin_unlock_irqrestore(&vpss_lock, flags);
		return -1;
	}

	/* check if this is duplicate */
	temp = start + dynamic_irq_start + 1;
	for (j = (dynamic_irq_start + 1); j < max_entries; j++) {
		if (irq == temp->irq_no) {
			printk(KERN_ERR
			       "Duplicate VPSS IRQ request received\n");
			spin_unlock_irqrestore(&vpss_lock, flags);
			return -1;
		}
		temp++;
	}

	/* temp points to available entry */
	if (cpu_is_davinci_dm355()) {
		if (dm355_enable_irq(irq, temp->cpu_irq, 1) < 0) {
			spin_unlock_irqrestore(&vpss_lock, flags);
			return -1;
		}
	} else if (cpu_is_davinci_dm365()) {
		if (dm365_enable_irq(irq, temp->cpu_irq, 1) < 0) {
			spin_unlock_irqrestore(&vpss_lock, flags);
			return -1;
		}
	} else {
		printk(KERN_WARNING
		       "No dynamic VPSS irq allocation on DM6446\n");
		spin_unlock_irqrestore(&vpss_lock, flags);
		return -1;
	}
	temp->in_use = 1;
	temp->irq_no = irq;
	spin_unlock_irqrestore(&vpss_lock, flags);
	return (request_irq(temp->cpu_irq, handler, irqflags, devname, dev_id));
}

/**
 *  vpss_free_irq - free an interrupt line
 *  @irq: vpss interrupt line
 *  @dev_id: A cookie passed back to the handler function
 *
 *  This is called by vpss drivers to free a pre-allocated
 *  or shared irq. returns
 *  zero if success, non-zero on failure 
 */
void vpss_free_irq(enum vpss_irq irq, void *dev_id)
{
	struct vpss_irq_table_entry *start, *temp;
	unsigned char max_entries, dynamic_irq_start;
	int i, found = 0;
	unsigned long flags;

	if (cpu_is_davinci_dm355()) {
		start = temp = dm355_vpss_irq_table;
		max_entries = DM3XX_MAX_VPSS_CPU_IRQ;
		dynamic_irq_start = DM3XX_DYNAMIC_IRQ_START;

	} else if (cpu_is_davinci_dm365()) {
		start = temp = dm365_vpss_irq_table;
		max_entries = DM3XX_MAX_VPSS_CPU_IRQ;
		dynamic_irq_start = DM3XX_DYNAMIC_IRQ_START;
	} else {
		/* DM6446 */
		start = temp = dm6446_vpss_irq_table;
		max_entries = DM6446_MAX_VPSS_CPU_IRQ;
		dynamic_irq_start = DM6446_MAX_VPSS_CPU_IRQ;
	}

	/* First check for pre-allocated interrupts */
	for (i = 0; i < dynamic_irq_start; i++) {
		/* first check if this irq is pre-allocated */
		if (irq == temp->irq_no) {
			/* a pre-allocated and match found */
			found = 1;
			break;
		}
		temp++;
	}

	/* This is a pre-allocated irq. Just free the IRQ to CPU */
	if (found)
		free_irq(temp->cpu_irq, dev_id);

	/* Dynamically allocated */
	/* We have a dynamic allocated irq */
	temp = start + dynamic_irq_start;
	for (i = dynamic_irq_start; i < max_entries; i++) {
		if (irq == temp->irq_no) {
			found = 1;
			break;
		}
		temp++;
	}
	if (found) {
		spin_lock_irqsave(&vpss_lock, flags);
		temp->in_use = 0;
		temp->irq_no = VPSS_INT_LAST;
		if (cpu_is_davinci_dm355())
			dm355_enable_irq(irq, temp->cpu_irq, 0);
		else if (cpu_is_davinci_dm365())
			dm365_enable_irq(irq, temp->cpu_irq, 0);
		else {
			printk(KERN_WARNING
			       "No dynamic VPSS irq de-allocation on DM6446\n");
			spin_unlock_irqrestore(&vpss_lock, flags);
			return;
		}
		spin_unlock_irqrestore(&vpss_lock, flags);
	}
	free_irq(temp->cpu_irq, dev_id);
}

static int dm355_enable_clock(enum vpss_clock_sel clock_sel, unsigned int en)
{
	unsigned long flags;
	u32 utemp, mask = 0x1, shift = 0;

	spin_lock_irqsave(&vpss_lock, flags);
	utemp = davinci_readl((DM355_VPSS_REG_BASE + DM355_VPSSCLK_CLKCTRL));
	switch (clock_sel) {
	case VPSS_VPBE_CLOCK:
		/* nothing since lsb */
		break;
	case VPSS_VENC_CLOCK_SEL:
		shift = 2;
		break;
	case VPSS_CFALD_CLOCK:
		shift = 3;
		break;
	case VPSS_H3A_CLOCK:
		shift = 4;
		break;
	case VPSS_IPIPE_CLOCK:
		shift = 5;
		break;
	case VPSS_CCDC_CLOCK:
		shift = 6;
		break;
	default:
		printk("dm355_enable_clock: Invalid selector: %d\n", clock_sel);
		return -1;
	}

	if (!en) {
		mask = ~mask;
		utemp &= (mask << shift);
	} else
		utemp |= (mask << shift);

	davinci_writel(utemp, (DM355_VPSS_REG_BASE + DM355_VPSSCLK_CLKCTRL));
	spin_unlock_irqrestore(&vpss_lock, flags);
	return 0;
}

static int dm365_enable_clock(enum vpss_clock_sel clock_sel, unsigned int en)
{
	unsigned long flags;
	u32 utemp, mask = 0x1, shift = 0, offset = DM365_PCCR;
	volatile unsigned long base_reg = DM365_ISP_REG_BASE;

	switch (clock_sel) {
	case VPSS_BL_CLOCK:
		/* nothing since lsb */
		break;
	case VPSS_CCDC_CLOCK:
		shift = 1;
		break;
	case VPSS_H3A_CLOCK:
		shift = 2;
		break;
	case VPSS_RSZ_CLOCK:
		shift = 3;
		break;
	case VPSS_IPIPE_CLOCK:
		shift = 4;
		break;
	case VPSS_IPIPEIF_CLOCK:
		shift = 5;
		break;
	case VPSS_PCLK_INTERNAL:
		shift = 6;
		break;
	case VPSS_PSYNC_CLOCK_SEL:
		shift = 7;
		break;
	case VPSS_VPBE_CLOCK:
		/* nothing since lsb */
		base_reg = DM365_VPSS_REG_BASE;
		offset = DM365_VPBE_CLK_CTRL;
		break;
	case VPSS_VENC_CLOCK_SEL:
		shift = 2;
		base_reg = DM365_VPSS_REG_BASE;
		offset = DM365_VPBE_CLK_CTRL;
		break;
	case VPSS_LDC_CLOCK:
		shift = 3;
		base_reg = DM365_VPSS_REG_BASE;
		offset = DM365_VPBE_CLK_CTRL;
		break;
	case VPSS_FDIF_CLOCK:
		shift = 4;
		base_reg = DM365_VPSS_REG_BASE;
		offset = DM365_VPBE_CLK_CTRL;
		break;
	case VPSS_OSD_CLOCK_SEL:
		shift = 6;
		base_reg = DM365_VPSS_REG_BASE;
		offset = DM365_VPBE_CLK_CTRL;
		break;
	case VPSS_LDC_CLOCK_SEL:
		shift = 7;
		base_reg = DM365_VPSS_REG_BASE;
		offset = DM365_VPBE_CLK_CTRL;
		break;
	default:
		printk(KERN_ERR "dm355_enable_clock: Invalid selector: %d\n",
		       clock_sel);
		return -1;
	}

	spin_lock_irqsave(&vpss_lock, flags);
	utemp = davinci_readl((base_reg + offset));
	if (!en) {
		mask = ~mask;
		utemp &= (mask << shift);
	} else
		utemp |= (mask << shift);

	davinci_writel(utemp, (base_reg + base_reg));
	spin_unlock_irqrestore(&vpss_lock, flags);
	return 0;
}

int vpss_enable_clock(enum vpss_clock_sel clock_sel, unsigned int en)
{
	if (cpu_is_davinci_dm644x()) {
		printk(KERN_ERR
		       "dm355_enable_clock: not supported for DM644x\n");
		return -1;
	}
	if (cpu_is_davinci_dm355())
		return (dm355_enable_clock(clock_sel, en));
	else
		return (dm365_enable_clock(clock_sel, en));
}

EXPORT_SYMBOL(vpss_enable_clock);

void vpss_dfc_memory_sel(enum vpss_dfc_mem_sel dfc_mem_sel)
{
	unsigned long flags;
	u32 utemp, mask = 1;

	spin_lock_irqsave(&vpss_lock, flags);
	utemp = davinci_readl((DM355_VPSSBL_REG_BASE + DM355_VPSSBL_MEMCTRL));
	if (dfc_mem_sel == VPSS_DFC_SEL_IPIPE)
		utemp &= (~mask);
	else
		utemp |= mask;
	davinci_writel(utemp, (DM355_VPSSBL_REG_BASE + DM355_VPSSBL_MEMCTRL));
	spin_unlock_irqrestore(&vpss_lock, flags);
}

EXPORT_SYMBOL(vpss_dfc_memory_sel);

void vpss_ipipe_enable_any_address(int en)
{
	unsigned long flags;
	u32 utemp, mask = 0x4;

	spin_lock_irqsave(&vpss_lock, flags);
	utemp = davinci_readl((DM355_VPSSBL_REG_BASE + DM355_VPSSBL_MEMCTRL));
	if (en)
		utemp |= mask;
	else
		utemp &= (~mask);
	davinci_writel(utemp, (DM355_VPSSBL_REG_BASE + DM355_VPSSBL_MEMCTRL));
	spin_unlock_irqrestore(&vpss_lock, flags);
}

EXPORT_SYMBOL(vpss_ipipe_enable_any_address);

void vpss_rsz_cfald_mem_select(enum vpss_rsz_cfald_mem_sel mem_sel)
{
	unsigned long flags;
	u32 utemp, mask = 2;

	spin_lock_irqsave(&vpss_lock, flags);
	utemp = davinci_readl((DM355_VPSSBL_REG_BASE + DM355_VPSSBL_MEMCTRL));
	if (mem_sel == VPSS_IPIPE_MEM)
		utemp &= (~mask);
	else
		utemp |= mask;
	davinci_writel(utemp, (DM355_VPSSBL_REG_BASE + DM355_VPSSBL_MEMCTRL));
	spin_unlock_irqrestore(&vpss_lock, flags);
}

EXPORT_SYMBOL(vpss_rsz_cfald_mem_select);

static int vpss_init(void)
{
	spin_lock_init(&vpss_lock);
	if (cpu_is_davinci_dm355()) {
		davinci_writel(0x7b3c0000,
			       DM355_VPSSBL_REG_BASE + DM355_VPSS_EVTSEL);
		/* enable pre-allocated interrupts */
		dm355_enable_irq(VPSS_VDINT0, IRQ_DM355_CCDC_VDINT0, 1);
		dm355_enable_irq(VPSS_VDINT1, IRQ_DM355_CCDC_VDINT1, 1);
		dm355_enable_irq(VPSS_H3AINT, IRQ_DM355_H3AINT, 1);
		dm355_enable_irq(VPSS_VENCINT, IRQ_DM355_VENCINT, 1);
	}
	return 0;
}

static void vpss_exit(void)
{
	return;
}

module_exit(vpss_exit);
subsys_initcall(vpss_init);
MODULE_LICENSE("GPL");
