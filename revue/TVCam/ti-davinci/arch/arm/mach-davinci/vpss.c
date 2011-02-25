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
 *  File for common vpss registers shared across VPBE and VPFE
 *  This file has APIs for set/get registers in VPSS and should be used by
 *  all video drivers.
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/arch/io.h>
#include <asm/arch/cpu.h>
#include <asm/arch/vpss.h>

static struct vpss_oper_cfg oper_cfg;

/* irq no, pre_allocated , in_use */
/* DM6446 vpss irq to INTC IRQ map */
static struct vpss_irq_table_entry
	dm6446_vpss_irq_table[DM6446_MAX_VPSS_CPU_IRQ] = {
	{VPSS_VDINT0,	IRQ_DM644X_VDINT0,	1},
	{VPSS_VDINT1,	IRQ_DM644X_VDINT1,	1},
	{VPSS_VDINT2,	IRQ_DM644X_VDINT2,	1},
	{VPSS_HISTINT,	IRQ_DM644X_HISTINT,	1},
	{VPSS_H3AINT,	IRQ_DM644X_H3AINT,	1},
	{VPSS_PRVUINT,	IRQ_DM644X_PRVUINT,	1},
	{VPSS_RSZINT,	IRQ_DM644X_RSZINT,	1},
	{VPSS_VENCINT,	IRQ_DM644X_VENCINT,	1}
};

/* DM355 vpss irq to vpss irq - register value map */
static unsigned char dm355_vpssirq_regval_map[VPSS_INT_LAST+1] = {
	/* VPSS_VDINT0 */
	0,
	/* VPSS_VDINT1 */
	1,
	/* VPSS_VDINT2 */
	2,
	/* VPSS_HISTINT */
	7,
	/* VPSS_H3AINT */
	3,
	/* VPSS_PRVUINT */
	DM355_IRQ_VAL_INVALID,
	/* VPSS_RSZINT */
	DM355_IRQ_VAL_INVALID,
	/* VPSS_VENCINT */
	4,
	/* VPSS_IPIPEIFINT */
	6,
	/* VPSS_IPIPE_INT_SDR */
	8,
	/* VPSS_IPIPE_INT_RZA */
	9,
	/* VPSS_IPIPE_INT_RZB */
	10,
	/* VPSS_IPIPE_INT_BSC */
	11,
	/* VPSS_IPIPE_INT_MMR */
	12,
	/* VPSS_CFALDINT */
	13,
	/* VPSS_HSSIINT */
	14,
	/* VPSS_OSDINT */
	5,
	/* VPSS_INT3 */
	DM355_IRQ_VAL_INVALID,
	/* VPSS_IPIPE_INT_REG */
	DM355_IRQ_VAL_INVALID,
	/* VPSS_IPIPE_INT_LAST_PIX */
	DM355_IRQ_VAL_INVALID,
	/* VPSS_AEW_INT */
	DM355_IRQ_VAL_INVALID,
	/* VPSS_AF_INT */
	DM355_IRQ_VAL_INVALID,
	/* VPSS_RSZ_INT_REG */
	DM355_IRQ_VAL_INVALID,
	/* VPSS_RSZ_INT_LAST_PIX */
	DM355_IRQ_VAL_INVALID,
	/* VPSS_RSZ_INT_SDR */
	DM355_IRQ_VAL_INVALID,
	/* VPSS_LDC_INT */
	DM355_IRQ_VAL_INVALID,
	/* VPSS_FDIF_INT */
	DM355_IRQ_VAL_INVALID,
	/* VPSS_RSZ_INT_EOF0 */
	DM355_IRQ_VAL_INVALID,
	/* VPSS_RSZ_INT_EOF1 */
	DM355_IRQ_VAL_INVALID,
	/* VPSS_H3A_INT_EOF */
	DM355_IRQ_VAL_INVALID,
	/* VPSS_IPIPE_INT_EOF */
	DM355_IRQ_VAL_INVALID,
	/* VPSS_LDC_INT_EOF */
	DM355_IRQ_VAL_INVALID,
	/* VPSS_FD_INT_EOF */
	DM355_IRQ_VAL_INVALID,
	/* VPSS_IPIPE_INT_DPC_INI */
	DM355_IRQ_VAL_INVALID,
	/* VPSS_IPIPE_INT_DPC_RENEW0 */
	DM355_IRQ_VAL_INVALID,
	/* VPSS_IPIPE_INT_DPC_RENEW1 */
	DM355_IRQ_VAL_INVALID,
	/* VPSS_INT_LAST */
	DM355_IRQ_VAL_INVALID,
};

/* DM355 vpss irq to INTC IRQ map */
static struct vpss_irq_table_entry
	dm355_vpss_irq_table[DM3XX_MAX_VPSS_CPU_IRQ] = {
	{VPSS_VDINT0,	IRQ_DM3XX_VPSSINT0,	1},
	{VPSS_VDINT1,	IRQ_DM3XX_VPSSINT1,	1},
	{VPSS_H3AINT,	IRQ_DM3XX_VPSSINT4,	1},
	{VPSS_VENCINT,	IRQ_DM3XX_VPSSINT8,	1},
	/* IPIPE_INT_SDR is shared with RSZ_INT_SDR. But one
	 * of them is always allocated. First entry of dynamically
	 * allocated INT is reserved for IPIPE SDR interrupts and
	 *  is assigned to IRQ_DM3XX_VPSSINT5
	 */
	{VPSS_INT_LAST,	IRQ_DM3XX_VPSSINT5,	0},
	/* Below irqs are allocated dynamically */
	{VPSS_INT_LAST,	IRQ_DM3XX_VPSSINT2,	0},
	{VPSS_INT_LAST,	IRQ_DM3XX_VPSSINT3,	0},
	{VPSS_INT_LAST,	IRQ_DM3XX_VPSSINT6,	0},
	{VPSS_INT_LAST,	IRQ_DM3XX_VPSSINT7,	0}
};

/* DM365 vpss irq to INTC IRQ map */
static struct vpss_irq_table_entry
	dm365_vpss_irq_table[DM3XX_MAX_VPSS_CPU_IRQ] = {
	{VPSS_VDINT0,	IRQ_DM3XX_VPSSINT0,	1},
	{VPSS_VDINT1,	IRQ_DM3XX_VPSSINT1,	1},
	{VPSS_VENCINT,	IRQ_DM3XX_VPSSINT8,	1},
	/* IPIPE_INT_SDR is shared with RSZ_INT_SDR. But one
	 * of them is always allocated. First entry of dynamically
	 * allocated INT is reserved for IPIPE SDR interrupts and
	 *  is assigned to IRQ_DM3XX_VPSSINT5
	 */
	{VPSS_INT_LAST,	IRQ_DM3XX_VPSSINT5,	0},
	/* Below irqs are allocated dynamically */
	{VPSS_INT_LAST,	IRQ_DM3XX_VPSSINT2,	0},
	{VPSS_INT_LAST,	IRQ_DM3XX_VPSSINT3,	0},
	{VPSS_INT_LAST,	IRQ_DM3XX_VPSSINT6,	0},
	{VPSS_INT_LAST,	IRQ_DM3XX_VPSSINT7,	0}
};

/* DM365 vpss irq to vpss irq - register value map */
static unsigned char dm365_vpssirq_regval_map[VPSS_INT_LAST+1] = {
	/* VPSS_VDINT0 */
	0,
	/* VPSS_VDINT1 */
	1,
	/* VPSS_VDINT2 */
	2,
	/* VPSS_HISTINT */
	8,
	/* VPSS_H3AINT */
	12,
	/* VPSS_PRVUINT */
	DM365_IRQ_VAL_INVALID,
	/* VPSS_RSZINT */
	DM365_IRQ_VAL_INVALID,
	/* VPSS_VENCINT */
	21,
	/* VPSS_IPIPEIFINT */
	9,
	/* VPSS_IPIPE_INT_SDR */
	6,
	/* VPSS_IPIPE_INT_RZA */
	16,
	/* VPSS_IPIPE_INT_RZB */
	17,
	/* VPSS_IPIPE_INT_BSC */
	7,
	/* VPSS_IPIPE_INT_MMR */
	DM365_IRQ_VAL_INVALID,
	/* VPSS_CFALDINT */
	DM365_IRQ_VAL_INVALID,
	/* VPSS_HSSIINT */
	DM365_IRQ_VAL_INVALID,
	/* VPSS_OSDINT */
	20,
	/* VPSS_INT3 */
	3,
	/* VPSS_IPIPE_INT_REG */
	4,
	/* VPSS_IPIPE_INT_LAST_PIX */
	5,
	/* VPSS_AEW_INT */
	10,
	/* VPSS_AF_INT */
	11,
	/* VPSS_RSZ_INT_REG */
	13,
	/* VPSS_RSZ_INT_LAST_PIX */
	14,
	/* VPSS_RSZ_INT_SDR */
	15,
	/* VPSS_LDC_INT */
	18,
	/* VPSS_FDIF_INT */
	19,
	/* VPSS_RSZ_INT_EOF0 */
	22,
	/* VPSS_RSZ_INT_EOF1 */
	23,
	/* VPSS_H3A_INT_EOF */
	24,
	/* VPSS_IPIPE_INT_EOF */
	25,
	/* VPSS_LDC_INT_EOF */
	26,
	/* VPSS_FD_INT_EOF */
	27,
	/* VPSS_IPIPE_INT_DPC_INI */
	28,
	/* VPSS_IPIPE_INT_DPC_RENEW0 */
	29,
	/* VPSS_IPIPE_INT_DPC_RENEW1 */
	30,
	/* VPSS_INT_LAST */
	DM365_IRQ_VAL_INVALID
};

/*
 *  dm355_enable_irq - Enable VPSS IRQ to ARM IRQ line on DM355
 *  @irq: vpss interrupt line
 *  @cpu_irq: arm irq line
 *  @en: enable/disable flag
 *
 *  This is called to enable or disable a vpss irq to to arm irq line
 */
static int dm355_enable_irq(enum vpss_irq irq,
		unsigned int cpu_irq,
		unsigned char en)
{
	u32 val, temp, shift = 0, mask = 0xf;
	unsigned int offset = DM355_VPSS_INTSEL;

	val = dm355_vpssirq_regval_map[irq];
	if (val == DM355_IRQ_VAL_INVALID) {
		printk(KERN_ERR "Invalid vpss irq requested\n");
		return -1;
	}

	if (cpu_irq == IRQ_DM3XX_VPSSINT8)
		offset = DM355_VPSS_EVTSEL;
	else
		shift = (((u32)cpu_irq) << 2);

	temp = davinci_readl((DM355_VPSSBL_REG_BASE + offset));

	temp &= (~(mask << shift));
	if (en)
		temp |= ((val & mask) << shift);
	davinci_writel(temp, (DM355_VPSSBL_REG_BASE + offset));
	return 0;
}

/*
 *  dm365_enable_irq - Enable VPSS IRQ to ARM IRQ line on DM365
 *  @irq: vpss interrupt line
 *  @cpu_irq: arm irq line
 *  @en: enable/disable flag
 *
 *  This is called to enable or disable a vpss irq to to arm irq line
 */
static int dm365_enable_irq(enum vpss_irq irq,
			unsigned int cpu_irq,
			unsigned char en)
{
	u32 val = 0, temp, shift = 0, mask = 0x1f;
	unsigned int offset = DM365_VPSS_INTSEL1;

	val = dm365_vpssirq_regval_map[irq];
	if (val == DM365_IRQ_VAL_INVALID) {
		printk(KERN_ERR "Invalid vpss irq requested\n");
		return -1;
	}

	if (cpu_irq <= IRQ_DM3XX_VPSSINT3)
		shift = (((u32)cpu_irq) << 3);
	else if (cpu_irq <= IRQ_DM3XX_VPSSINT7) {
		shift = (((u32)cpu_irq & 3) << 3);
		offset = DM365_VPSS_INTSEL2;
	} else
		offset = DM365_VPSS_INTSEL3;

	temp = davinci_readl((DM365_ISP_REG_BASE + offset));
	temp &= ((~mask) << shift);
	if (en)
		temp |= ((val & mask) << shift);
	davinci_writel(temp, (DM365_ISP_REG_BASE + offset));
	return 0;
}

/*
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
		     irqreturn_t (*handler)(int, void *, struct pt_regs *),
		     unsigned long irqflags,
		     const char *devname,
		     void *dev_id)
{
	struct vpss_irq_table_entry *temp;
	unsigned char max_entries, dynamic_irq_start;
	int i, found = 0, found_index = -1;
	unsigned long flags;

	temp = oper_cfg.irq_table;
	max_entries = oper_cfg.max_entries;
	dynamic_irq_start = oper_cfg.dynamic_irq_start;

	/* First check for pre-allocated interrupts */
	for (i = 0; i < dynamic_irq_start; i++) {
		/* first check if this irq is pre-allocated */
		if (irq == temp->irq_no) {
			/* a pre-allocated and match found */
			return request_irq(temp->cpu_irq, handler, irqflags,
					  devname, dev_id);
		}
		temp++;
	}

	/* We have a dynamic allocated irq */
	spin_lock_irqsave(&oper_cfg.vpss_lock, flags);
	temp = oper_cfg.irq_table + dynamic_irq_start;

	if ((irq == VPSS_IPIPE_INT_SDR) || (irq == VPSS_RSZ_INT_SDR)) {
		if (temp->in_use) {
			printk(KERN_ERR "IPIPE/RSZ IRQ is in use\n");
			spin_unlock_irqrestore(&oper_cfg.vpss_lock, flags);
			return -1;
		}
		found = 1;
		found_index = dynamic_irq_start;
	} else {
		/*
		 * skip IPIPE SDR IRQ which is always reserved for IPIPE
		 * or RSZ
		 */
		dynamic_irq_start++;
		temp++;
		for (i = dynamic_irq_start; i < max_entries; i++) {
			if (irq == temp->irq_no) {
				printk(KERN_ERR "Duplicate VPSS IRQ"
					" request received\n");
				spin_unlock_irqrestore(&oper_cfg.vpss_lock,
						       flags);
				return -1;
			}
			if (!found && !temp->in_use) {
				found = 1;
				found_index = i;
			}
			temp++;
		}

		if (!found) {
			printk(KERN_WARNING "No VPSS IRQ resource available\n");
			spin_unlock_irqrestore(&oper_cfg.vpss_lock, flags);
			return -1;
		}
	}

	temp = oper_cfg.irq_table + found_index;
	/* temp points to available entry */
	if (oper_cfg.enable_irq) {
		if (oper_cfg.enable_irq(irq, temp->cpu_irq, 1) < 0) {
			spin_unlock_irqrestore(&oper_cfg.vpss_lock, flags);
			return -1;
		}
	} else {
		printk(KERN_WARNING "No dynamic VPSS irq allocation"
				"available\n");
		spin_unlock_irqrestore(&oper_cfg.vpss_lock, flags);
		return -1;
	}

	temp->in_use = 1;
	temp->irq_no = irq;
	spin_unlock_irqrestore(&oper_cfg.vpss_lock, flags);
	return request_irq(temp->cpu_irq, handler, irqflags, devname, dev_id);
}
EXPORT_SYMBOL(vpss_request_irq);

/*
 *  vpss_free_irq - free an interrupt line
 *  @irq: vpss interrupt line
 *  @dev_id: A cookie passed back to the handler function
 *
 *  This is called by vpss drivers to free a pre-allocated
 *  or shared irq. returns zero if success, non-zero on failure
 */
void vpss_free_irq(enum vpss_irq irq, void *dev_id)
{
	struct vpss_irq_table_entry *temp;
	unsigned char max_entries, dynamic_irq_start;
	int i, found = 0;
	unsigned long flags;

	temp = oper_cfg.irq_table;
	max_entries = oper_cfg.max_entries;
	dynamic_irq_start = oper_cfg.dynamic_irq_start;

	/* First check for pre-allocated interrupts */
	for (i = 0; i < dynamic_irq_start; i++) {
		/* first check if this irq is pre-allocated */
		if (irq == temp->irq_no) {
			/* a pre-allocated and match found */
			free_irq(temp->cpu_irq, dev_id);
			return;
		}
		temp++;
	}

	/* We have a dynamic allocated irq */
	temp =  oper_cfg.irq_table + dynamic_irq_start;
	for (i = dynamic_irq_start; i < max_entries; i++) {
		if (irq == temp->irq_no) {
			found = 1;
			break;
		}
		temp++;
	}
	if (found) {
		spin_lock_irqsave(&oper_cfg.vpss_lock, flags);
		temp->in_use = 0;
		temp->irq_no = VPSS_INT_LAST;
		if (oper_cfg.enable_irq)
			oper_cfg.enable_irq(irq, temp->cpu_irq, 0);
		else {
			printk(KERN_WARNING "No dynamic VPSS irq "
					"de-allocation on DM6446\n");
			spin_unlock_irqrestore(&oper_cfg.vpss_lock, flags);
			return;
		}
		spin_unlock_irqrestore(&oper_cfg.vpss_lock, flags);
		free_irq(temp->cpu_irq, dev_id);
	}
}
EXPORT_SYMBOL(vpss_free_irq);

/*
 *  vpss_free_irq - free an interrupt line
 *  @irq: vpss interrupt line
 *  @en: enable/disable flag, 1 - enable, 0 - disable
 *
 *  This is called by vpss drivers to enable/disable irq to arm
 */
void vpss_enable_irq(enum vpss_irq irq, int en)
{
	int i;

	for (i = 0; i < oper_cfg.max_entries; i++) {
		if (irq == oper_cfg.irq_table[i].irq_no &&
		    oper_cfg.irq_table[i].in_use) {
			if (en)
				enable_irq(oper_cfg.irq_table[i].cpu_irq);
			else
				disable_irq(oper_cfg.irq_table[i].cpu_irq);
			break;
		}
	}
}
EXPORT_SYMBOL(vpss_enable_irq);

/*
 *  vpss_clear_interrupt - clear irq bit in INTSTAT
 *  @irq: vpss interrupt line
 *
 *  This is called by vpss drivers to clear irq bit in INTSTAT
 *  Called from isr to clear interrupt. This returns 0 - if
 *  interrupted and cleared, -1 if no interrupt
 */
int vpss_clear_interrupt(enum vpss_irq irq)
{
	u32 vpss_irq_val, inval_irq, utemp;
	int ret = -1;
	unsigned long intstat_reg;
	if (cpu_is_davinci_dm355()) {
		intstat_reg = DM355_VPSSBL_REG_BASE + DM355_VPSS_INTSTAT;
		inval_irq = DM355_IRQ_VAL_INVALID;
		vpss_irq_val = dm355_vpssirq_regval_map[irq];
	} else if (cpu_is_davinci_dm365()) {
		intstat_reg = DM365_ISP_REG_BASE + DM365_VPSS_INTSTAT;
		inval_irq = DM365_IRQ_VAL_INVALID;
		vpss_irq_val = dm365_vpssirq_regval_map[irq];
	} else
		/* we don't do anything for other platforms */
		return ret;

	if (vpss_irq_val == inval_irq)
		return ret;
	vpss_irq_val =  1 << vpss_irq_val;
	utemp = davinci_readl(intstat_reg);

	if (utemp & vpss_irq_val)
		ret = 0;
	else
		return ret;
	/* clear the interrupt */
	utemp &= vpss_irq_val;
	davinci_writel(utemp, intstat_reg);
	return ret;
}
EXPORT_SYMBOL(vpss_clear_interrupt);

/*
 *  dm355_enable_clock - Enable VPSS Clock
 *  @clock_sel: CLock to be enabled/disabled
 *  @en: enable/disable flag
 *
 *  This is called to enable or disable a vpss clock
 */
static int dm355_enable_clock(enum vpss_clock_sel clock_sel, unsigned int en)
{
	unsigned long flags;
	u32 utemp, mask = 0x1, shift = 0;

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
		printk(KERN_ERR "dm355_enable_clock:"
				" Invalid selector: %d\n", clock_sel);
		return -1;
	}

	spin_lock_irqsave(&oper_cfg.vpss_lock, flags);
	utemp = davinci_readl((DM355_VPSS_REG_BASE + DM355_VPSSCLK_CLKCTRL));
	if (!en)
		utemp &= ~(mask << shift);
	else
		utemp |= (mask << shift);

	davinci_writel(utemp, (DM355_VPSS_REG_BASE + DM355_VPSSCLK_CLKCTRL));
	spin_unlock_irqrestore(&oper_cfg.vpss_lock, flags);
	return 0;
}

/*
 *  dm365_enable_clock - Enable VPSS Clock
 *  @clock_sel: CLock to be enabled/disabled
 *  @en: enable/disable flag
 *
 *  This is called to enable or disable a vpss clock
 */
static int dm365_enable_clock(enum vpss_clock_sel clock_sel, unsigned int en)
{
	unsigned long flags;
	u32 utemp, mask = 0x1, shift = 0, offset = DM365_PCCR;
	unsigned long base_reg = DM365_ISP_REG_BASE;

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
		printk(KERN_ERR "dm365_enable_clock:"
				" Invalid selector: %d\n", clock_sel);
		return -1;
	}

	spin_lock_irqsave(&oper_cfg.vpss_lock, flags);
	utemp = davinci_readl((base_reg + offset));
	if (!en)
		utemp &= ~(mask << shift);
	else
		utemp |= (mask << shift);

	davinci_writel(utemp, (base_reg + offset));
	spin_unlock_irqrestore(&oper_cfg.vpss_lock, flags);
	return 0;
}

/*
 *  vpss_enable_clock - Enable VPSS Clock
 *  @clock_sel: CLock to be enabled/disabled
 *  @en: enable/disable flag
 *
 *  This is called to enable or disable a vpss clock
 */
int vpss_enable_clock(enum vpss_clock_sel clock_sel, unsigned int en)
{
	if (NULL == oper_cfg.enable_clock) {
		printk(KERN_ERR "vpss_enable_clock: not"
				" supported for DM644x\n");
		return -1;
	}
	return oper_cfg.enable_clock(clock_sel, en);
}
EXPORT_SYMBOL(vpss_enable_clock);

/*
 *  vpss_memory_control - common function for updating memory
 *  control register
 *  @en: enable/disable
 *  @mask: bit mask
 *
 *  This is called to update memory control register
 */
static void vpss_mem_control(int en, u32 mask)
{
	u32 flags, utemp;

	spin_lock_irqsave(&oper_cfg.vpss_lock, flags);
	utemp = davinci_readl((DM355_VPSSBL_REG_BASE + DM355_VPSSBL_MEMCTRL));
	if (en)
		utemp |= mask;
	else
		utemp &= (~mask);

	davinci_writel(utemp, (DM355_VPSSBL_REG_BASE + DM355_VPSSBL_MEMCTRL));
	spin_unlock_irqrestore(&oper_cfg.vpss_lock, flags);
}

/*
 *  vpss_dfc_memory_sel - select dfc memory for use by IPIPE/CCDC
 *  @dfc_mem_sel: memory selector
 *
 *  This is called to allocate dfc memory to IPIPE or CCDC. Only
 *  applicable on DM355
 */
void vpss_dfc_memory_sel(enum vpss_dfc_mem_sel dfc_mem_sel)
{
	if (!cpu_is_davinci_dm355())
		return;

	if (dfc_mem_sel == VPSS_DFC_SEL_IPIPE)
		vpss_mem_control(0, 0x1);
	else
		vpss_mem_control(1, 0x1);
}
EXPORT_SYMBOL(vpss_dfc_memory_sel);

/*
 *  vpss_ipipe_enable_any_address - Allow IPIPE to use non-aligned buffer
 *  address.
 *  @en: enable/disable non-aligned buffer address use.
 *
 *  This is called to allow IPIPE to use non-aligned buffer address.
 *  Applicable only to DM355.
 */
void vpss_ipipe_enable_any_address(int en)
{
	if (!cpu_is_davinci_dm355())
		return;

	if (en)
		vpss_mem_control(1, 0x4);
	else
		vpss_mem_control(0, 0x4);
}
EXPORT_SYMBOL(vpss_ipipe_enable_any_address);

/*
 *  vpss_rsz_cfald_mem_select - This function will select the module
 *  that gets access to internal memory.
 *  @sel: selector
 *
 *  This function will select the module that gets access to internal memory.
 *  Choice is either IPIPE or CFALD. Applicable only on DM355
 */
void vpss_rsz_cfald_mem_select(enum vpss_rsz_cfald_mem_sel mem_sel)
{

	if (!cpu_is_davinci_dm355())
		return;

	if (mem_sel == VPSS_IPIPE_MEM)
		vpss_mem_control(0, 0x2);
	else
		vpss_mem_control(1, 0x2);

}
EXPORT_SYMBOL(vpss_rsz_cfald_mem_select);

/*
 *  vpss_bcr_control - common function for updating bcr
 *  register
 *  @en: enable/disable
 *  @mask: bit mask
 *
 *  This is called to update bcr register
 */
static void vpss_bcr_control(int en, u32 mask)
{
	u32 flags, utemp;

	spin_lock_irqsave(&oper_cfg.vpss_lock, flags);
	utemp = davinci_readl((DM365_ISP_REG_BASE + DM365_BCR));
	if (en)
		utemp |= mask;
	else
		utemp &= (~mask);

	davinci_writel(utemp, (DM365_ISP_REG_BASE + DM365_BCR));
	spin_unlock_irqrestore(&oper_cfg.vpss_lock, flags);
}

/*
 *  vpss_bl1_src_select -  select the source that gets BL1 to write to SDRAM.
 *
 *  @sel: selector
 *
 *  This function will select the source that gets BL1 to write to SDRAM.
 *  Choice is either ISIF or IPIPE. Applicable only on DM365
 */
void vpss_bl1_src_select(enum vpss_bl1_src_sel sel)
{
	if (!cpu_is_davinci_dm365())
		return;

	if (sel == VPSS_BL1_SRC_SEL_IPIPE)
		vpss_bcr_control(0, 0x2);
	else
		vpss_bcr_control(1, 0x2);
}
EXPORT_SYMBOL(vpss_bl1_src_select);

/*
 *  vpss_bl2_src_select - This function will select the source that gets
 *  BL2 to write to SDRAM.
 *  @sel: selector
 *
 *  This function will select the source that gets BL2 to write to SDRAM.
 *  Choice is either IPIPE or LDC
 */
void vpss_bl2_src_select(enum vpss_bl2_src_sel sel)
{
	if (!cpu_is_davinci_dm365())
		return;

	if (sel == VPSS_BL2_SRC_SEL_LDC)
		vpss_bcr_control(0, 0x1);
	else
		vpss_bcr_control(1, 0x1);
}
EXPORT_SYMBOL(vpss_bl2_src_select);

static int vpss_init(void)
{
	spin_lock_init(&oper_cfg.vpss_lock);

	if (cpu_is_davinci_dm355()) {
		davinci_writel(0xFFFFFFFF,
			DM355_VPSSBL_REG_BASE + DM355_VPSS_INTSEL);
		davinci_writel(0x7b3c000F,
			DM355_VPSSBL_REG_BASE + DM355_VPSS_EVTSEL);
		dm355_enable_irq(VPSS_IPIPE_INT_SDR, IRQ_DM3XX_VPSSINT5, 1);
		oper_cfg.enable_irq = dm355_enable_irq;
		oper_cfg.enable_clock = dm355_enable_clock;
		oper_cfg.irq_table = dm355_vpss_irq_table;
		oper_cfg.max_entries = DM3XX_MAX_VPSS_CPU_IRQ;
		oper_cfg.dynamic_irq_start = DM3XX_DYNAMIC_IRQ_START;
		dm355_enable_irq(VPSS_VDINT0, IRQ_DM3XX_VPSSINT0, 1);
		dm355_enable_irq(VPSS_VDINT1, IRQ_DM3XX_VPSSINT1, 1);
		dm355_enable_irq(VPSS_H3AINT, IRQ_DM3XX_VPSSINT4, 1);
		dm355_enable_irq(VPSS_VENCINT, IRQ_DM3XX_VPSSINT8, 1);
	} else if (cpu_is_davinci_dm365()) {
		davinci_writel(0x1F1F1F1F,
			DM365_ISP_REG_BASE + DM365_VPSS_INTSEL1);
		davinci_writel(0x1F1F1F1F,
			DM365_ISP_REG_BASE + DM365_VPSS_INTSEL2);
		davinci_writel(0x1F1F1F1F,
			DM365_ISP_REG_BASE + DM365_VPSS_INTSEL3);
		oper_cfg.enable_irq = dm365_enable_irq;
		oper_cfg.enable_clock = dm365_enable_clock;
		/* clear all vpss interrupts */
		davinci_writel(0xFFFFFFFF,
			DM365_ISP_REG_BASE + DM365_VPSS_INTSTAT);
		dm365_enable_irq(VPSS_RSZ_INT_SDR, IRQ_DM3XX_VPSSINT5, 1);
		oper_cfg.irq_table = dm365_vpss_irq_table;
		oper_cfg.max_entries = DM3XX_MAX_VPSS_CPU_IRQ;
		oper_cfg.dynamic_irq_start = DM3XX_DYNAMIC_IRQ_START;
		dm365_enable_irq(VPSS_VDINT0, IRQ_DM3XX_VPSSINT0, 1);
		dm365_enable_irq(VPSS_VDINT1, IRQ_DM3XX_VPSSINT1, 1);
		dm365_enable_irq(VPSS_VENCINT, IRQ_DM3XX_VPSSINT8, 1);
	} else {
		/* DM6446 */
		oper_cfg.irq_table = dm6446_vpss_irq_table;
		oper_cfg.max_entries = DM6446_MAX_VPSS_CPU_IRQ;
		oper_cfg.dynamic_irq_start = DM6446_MAX_VPSS_CPU_IRQ;
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
