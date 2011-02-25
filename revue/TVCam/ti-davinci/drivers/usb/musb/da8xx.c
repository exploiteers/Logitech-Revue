/*
 * Texas Instruments DA8xx "glue layer"
 *
 * Copyright (c) 2008, MontaVista Software, Inc. <source@mvista.com>
 *
 * Based on the DaVinci "glue layer" code.
 * Copyright (C) 2005-2006 by Texas Instruments
 *
 * This file is part of the Inventra Controller Driver for Linux.
 *
 * The Inventra Controller Driver for Linux is free software; you
 * can redistribute it and/or modify it under the terms of the GNU
 * General Public License version 2 as published by the Free Software
 * Foundation.
 *
 * The Inventra Controller Driver for Linux is distributed in
 * the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
 * License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with The Inventra Controller Driver for Linux ; if not,
 * write to the Free Software Foundation, Inc., 59 Temple Place,
 * Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <linux/init.h>
#include <linux/clk.h>
#include <linux/io.h>

#include <asm/arch/hardware.h>
#include <asm/arch/cp_intd.h>

#include "musb_core.h"
#include "cppi41_dma.h"

/*
 * DA8xx specific definitions
 */

/* USB 2.0 PHY Control */
#define BOOTCFG_CFGCHIP2_REG	(DA8XX_BOOT_CFG_BASE + 0x184)
#define CFGCHIP2_PHYCLKGD	(1 << 17)
#define CFGCHIP2_VBUSSENSE	(1 << 16)
#define CFGCHIP2_RESET		(1 << 15)
#define CFGCHIP2_OTGMODE	(3 << 13)
#define CFGCHIP2_USB1PHYCLKMUX	(1 << 12)
#define CFGCHIP2_USB2PHYCLKMUX	(1 << 11)
#define CFGCHIP2_PHYPWRDN	(1 << 10)
#define CFGCHIP2_OTGPWRDN	(1 << 9)
#define CFGCHIP2_DATPOL 	(1 << 8)
#define CFGCHIP2_USB1SUSPENDM	(1 << 7)
#define CFGCHIP2_PHY_PLLON	(1 << 6)	/* override PLL suspend */
#define CFGCHIP2_SESENDEN	(1 << 5)	/* Vsess_end comparator */
#define CFGCHIP2_VBDTCTEN	(1 << 4)	/* Vbus comparator */
#define CFGCHIP2_REFFREQ	(0xf << 0)

#define DA8XX_TX_ENDPTS_MASK	0x1f		/* EP0 + 4 Tx EPs */
#define DA8XX_RX_ENDPTS_MASK	0x1e		/* 4 Rx EPs */

#define DA8XX_TX_INTR_MASK	(DA8XX_TX_ENDPTS_MASK << USB_INTR_TX_SHIFT)
#define DA8XX_RX_INTR_MASK	(DA8XX_RX_ENDPTS_MASK << USB_INTR_RX_SHIFT)

/* INTD status register 0 bits */
#define USB_RX_SBUF		(1 << 1)	/* Rx MOP Desc. Starvation */
#define USB_RX_SDES		(1 << 0)	/* Rx SOP Desc. Starvation */

/* INTD status register 1 bits */
#define USB_CORE		(1 << 19)	/* USB Core Interrupt */
#define USB_RX_EP_SHIFT 	14		/* RX EP 1..4 Interrupt */
#define USB_RX_EP_MASK		(0x1f << INTD_USB_RX_EP_SHIFT)
#define USB_TX_EP_SHIFT 	9		/* TX EP 1..4 Interrupt */
#define USB_TX_EP_MASK		(0x1f << INTD_USB_TX_EP_SHIFT)
#define USB_INT_MASK		(0x1ff << 0)	/* USB Interrupt 0..8 */

/* INTD status register 2 bits */
#define USB_TX_CMP_INT_SHIFT	16
#define USB_TX_CMP_INT_MASK	(3 << USB_TX_CMP_INT_SHIFT)
#define USB_RX_CMP_INT_SHIFT	0
#define USB_RX_CMP_INT_MASK	(3 << USB_RX_CMP_INT_SHIFT)

#ifdef CONFIG_USB_TI_CPPI41_DMA

/*
 * CPPI 4.1 resources used for USB OTG controller module:
 *
 * USB   DMA  DMA  QMgr  Tx     Src
 *       Tx   Rx         QNum   Port
 * ---------------------------------
 * EP0   0    0    0     16,17  1
 * ---------------------------------
 * EP1   1    1    0     18,19  2
 * ---------------------------------
 * EP2   2    2    0     20,21  3
 * ---------------------------------
 * EP3   3    3    0     22,23  4
 * ---------------------------------
 */

static const u16 tx_comp_q[] = { 24, 25 };
static const u16 rx_comp_q[] = { 26, 27 };

const struct usb_cppi41_info usb_cppi41_info = {
	.dma_block	= 0,
	.ep_dma_ch	= { 0, 1, 2, 3 },
	.q_mgr		= 0,
	.num_tx_comp_q	= 2,
	.num_rx_comp_q	= 2,
	.tx_comp_q	= tx_comp_q,
	.rx_comp_q	= rx_comp_q,
	.rx_fdb_q	= { 0, 4, 8, 12 }
};

#endif /* CONFIG_USB_TI_CPPI41_DMA */

/*
 * REVISIT (PM): we should be able to keep the PHY in low power mode most
 * of the time (24 MHZ oscillator and PLL off, etc) by setting POWER.D0
 * and, when in host mode, autosuspending idle root ports... PHYPLLON
 * (overriding SUSPENDM?) then likely needs to stay off.
 */

static inline void phy_on(void)
{
	/*
	 * Start the on-chip PHY and its PLL.
	 * REVISIT:
	 * - KICK0/1 registers;
	 * - PHY data polarity;
	 * - OTG powerdown.
	 */
	davinci_writel(CFGCHIP2_SESENDEN | CFGCHIP2_VBDTCTEN |
		       CFGCHIP2_PHY_PLLON, BOOTCFG_CFGCHIP2_REG);

	while (!(davinci_readl(BOOTCFG_CFGCHIP2_REG) & CFGCHIP2_PHYCLKGD))
		cpu_relax();
}

static inline void phy_off(void)
{
	/*
	 * Power down the on-chip PHY.
	 * REVISIT:
	 * - KICK0/1 registers;
	 * - OTG powerdown.
	 */
	davinci_writel(CFGCHIP2_PHYPWRDN, BOOTCFG_CFGCHIP2_REG);
}

static int dma_off = 1;

/*
 * Because we don't set CTRL.UINT, it's "important" to:
 *	- not read/write INTRUSB/INTRUSBE (except during
 *	  initial setup, as a workaround);
 *	- use INTSET/INTCLR instead.
 */

/**
 * musb_platform_enable - enable interrupts
 */
void musb_platform_enable(struct musb *musb)
{
	void __iomem *reg_base = musb->ctrl_base;
	u32 mask;

	/* Workaround: setup IRQs through both register sets. */
	mask = ((musb->epmask & DA8XX_TX_ENDPTS_MASK) << USB_INTR_TX_SHIFT) |
	       ((musb->epmask & DA8XX_RX_ENDPTS_MASK) << USB_INTR_RX_SHIFT) |
	       (0x01ff << USB_INTR_USB_SHIFT);
	musb_writel(reg_base, USB_INTR_MASK_SET_REG, mask);

	if (is_dma_capable() && !dma_off)
		printk(KERN_WARNING "%s::%s: DMA not reactivated\n",
		       __FILE__, __func__);
	else
		dma_off = 0;

	/* Force the DRVVBUS IRQ so we can start polling for ID change. */
	if (is_otg_enabled(musb))
		musb_writel(reg_base, USB_INTR_SRC_SET_REG,
			    USB_INTR_DRVVBUS << USB_INTR_USB_SHIFT);
}

/**
 * musb_platform_disable - disable HDRC and flush interrupts
 */
void musb_platform_disable(struct musb *musb)
{
	void __iomem *reg_base = musb->ctrl_base;

	musb_writel(reg_base, USB_INTR_MASK_CLEAR_REG, USB_INTR_USB_MASK |
		    DA8XX_TX_INTR_MASK | DA8XX_RX_INTR_MASK);
	musb_writeb(musb->mregs, MUSB_DEVCTL, 0);
	musb_writel(reg_base, USB_END_OF_INTR_REG, 0);

	if (is_dma_capable() && !dma_off)
		WARN("DMA still active\n");
}

/* REVISIT: it's not clear whether DA8xx can support full OTG.  */

static int vbus_state = -1;

#ifdef CONFIG_USB_MUSB_HDRC_HCD
#define portstate(stmt) 	stmt
#else
#define portstate(stmt)
#endif

static void da8xx_source_power(struct musb *musb, int is_on, int immediate)
{
	if (is_on)
		is_on = 1;

	if (vbus_state == is_on)
		return;
	vbus_state = is_on;		/* 0/1 vs "-1 == unknown/init" */
}

static void da8xx_set_vbus(struct musb *musb, int is_on)
{
	WARN_ON(is_on && is_peripheral_active(musb));
	da8xx_source_power(musb, is_on, 0);
}

#define	POLL_SECONDS	2

static struct timer_list otg_workaround;

static void otg_timer(unsigned long _musb)
{
	struct musb		*musb = (void *)_musb;
	void __iomem		*mregs = musb->mregs;
	u8			devctl;
	unsigned long		flags;

	/* We poll because DaVinci's won't expose several OTG-critical
	* status change events (from the transceiver) otherwise.
	 */
	devctl = musb_readb(mregs, MUSB_DEVCTL);
	DBG(7, "Poll devctl %02x (%s)\n", devctl, otg_state_string(musb));

	spin_lock_irqsave(&musb->lock, flags);
	switch (musb->xceiv.state) {
	case OTG_STATE_A_WAIT_VFALL:
		/*
		 * Wait till VBUS falls below SessionEnd (~0.2 V); the 1.3
		 * RTL seems to mis-handle session "start" otherwise (or in
		 * our case "recover"), in routine "VBUS was valid by the time
		 * VBUSERR got reported during enumeration" cases.
		 */
		if (devctl & MUSB_DEVCTL_VBUS) {
			mod_timer(&otg_workaround, jiffies + POLL_SECONDS * HZ);
			break;
		}
		musb->xceiv.state = OTG_STATE_A_WAIT_VRISE;
		musb_writel(musb->ctrl_base, USB_INTR_SRC_SET_REG,
			    MUSB_INTR_VBUSERROR << USB_INTR_USB_SHIFT);
		break;
	case OTG_STATE_B_IDLE:
		if (!is_peripheral_enabled(musb))
			break;

		/*
		 * There's no ID-changed IRQ, so we have no good way to tell
		 * when to switch to the A-Default state machine (by setting
		 * the DEVCTL.SESSION flag).
		 *
		 * Workaround:  whenever we're in B_IDLE, try setting the
		 * session flag every few seconds.  If it works, ID was
		 * grounded and we're now in the A-Default state machine.
		 *
		 * NOTE: setting the session flag is _supposed_ to trigger
		 * SRP but clearly it doesn't.
		 */
		musb_writeb(mregs, MUSB_DEVCTL, devctl | MUSB_DEVCTL_SESSION);
		devctl = musb_readb(mregs, MUSB_DEVCTL);
		if (devctl & MUSB_DEVCTL_BDEVICE)
			mod_timer(&otg_workaround, jiffies + POLL_SECONDS * HZ);
		else
			musb->xceiv.state = OTG_STATE_A_IDLE;
		break;
	default:
		break;
	}
	spin_unlock_irqrestore(&musb->lock, flags);
}

static irqreturn_t da8xx_interrupt(int irq, void *hci, struct pt_regs *regs)
{
	struct musb  *musb = hci;
	void __iomem *reg_base = musb->ctrl_base;
	unsigned long flags;
	irqreturn_t ret = IRQ_NONE;
	int cppi_intr = 0;
	u32 status;

	spin_lock_irqsave(&musb->lock, flags);

	/*
	 * NOTE: DA8xx shadows the Mentor IRQs.  Don't manage them through
	 * the Mentor registers (except for setup), use the TI ones and EOI.
	 */

	/*
	 * CPPI 4.1 interrupts share the same IRQ but have their own status
	 * and EOI (?) registers.
	 */
	if (is_cppi41_enabled()) {
		u16 tx, rx;

		/*
		 * First, check for the Tx/Rx completion queue interrupts.
		 * They are level-triggered and will stay asserted until
		 * the queues are emptied...
		 */
		status = intd_intr_stat(2);
		tx = (status & USB_TX_CMP_INT_MASK) >> USB_TX_CMP_INT_SHIFT;
		rx = (status & USB_RX_CMP_INT_MASK) >> USB_RX_CMP_INT_SHIFT;
		if (tx || rx) {
			DBG(4, "CPPI 4.1 IRQ: Tx %x, Rx %x\n", tx, rx);
			cppi41_completion(musb, rx, tx);
			cppi_intr = 1;
		}

		/*
		 * Second, check for Rx descriptor/buffer starvation interrupts.
		 * They should not normally occur, and don't require any action
		 * except clearing since they are pulsed interrupts...
		 */
		status = intd_intr_stat(0) & (USB_RX_SDES | USB_RX_SBUF);
		if (status) {
			DBG(4, "CPPI 4.1 IRQ MOP/SOP %x\n", status);
			intd_clear_intr(0, status);
			cppi_intr = 1;
		}

		if (cppi_intr)
			intd_write_eoi(0);
	}

	/* Acknowledge and handle non-CPPI interrupts */
	status = musb_readl(reg_base, USB_INTR_SRC_MASKED_REG);
	musb_writel(reg_base, USB_INTR_SRC_CLEAR_REG, status);
	DBG(4, "USB IRQ %08x\n", status);

	musb->int_rx = (status & DA8XX_RX_INTR_MASK) >> USB_INTR_RX_SHIFT;
	musb->int_tx = (status & DA8XX_TX_INTR_MASK) >> USB_INTR_TX_SHIFT;
	musb->int_usb = (status & USB_INTR_USB_MASK) >> USB_INTR_USB_SHIFT;
	musb->int_regs = regs;

	/*
	 * DRVVBUS IRQs are the only proxy we have (a very poor one!) for
	 * DA8xx's missing ID change IRQ.  We need an ID change IRQ to
	 * switch appropriately between halves of the OTG state machine.
	 * Managing DEVCTL.SESSION per Mentor docs requires we know its
	 * value, but DEVCTL.BDEVICE is invalid without DEVCTL.SESSION set.
	 * Also, DRVVBUS pulses for SRP (but not at 5V) ...
	 */
	if (status & (USB_INTR_DRVVBUS << USB_INTR_USB_SHIFT)) {
		int drvvbus = musb_readl(reg_base, USB_STAT_REG);
		void __iomem *mregs = musb->mregs;
		u8 devctl = musb_readb(mregs, MUSB_DEVCTL);
		int err;

		err = is_host_enabled(musb) && (musb->int_usb &
						MUSB_INTR_VBUSERROR);
		if (err) {
			/*
			 * The Mentor core doesn't debounce VBUS as needed
			 * to cope with device connect current spikes. This
			 * means it's not uncommon for bus-powered devices
			 * to get VBUS errors during enumeration.
			 *
			 * This is a workaround, but newer RTL from Mentor
			 * seems to allow a better one: "re"-starting sessions
			 * without waiting for VBUS to stop registering in
			 * devctl.
			 */
			musb->int_usb &= ~MUSB_INTR_VBUSERROR;
			musb->xceiv.state = OTG_STATE_A_WAIT_VFALL;
			mod_timer(&otg_workaround, jiffies + POLL_SECONDS * HZ);
			WARN("VBUS error workaround (delay coming)\n");
		} else if (is_host_enabled(musb) && drvvbus) {
			musb->is_active = 1;
			MUSB_HST_MODE(musb);
			musb->xceiv.default_a = 1;
			musb->xceiv.state = OTG_STATE_A_WAIT_VRISE;
			portstate(musb->port1_status |= USB_PORT_STAT_POWER);
			del_timer(&otg_workaround);
		} else {
			musb->is_active = 0;
			MUSB_DEV_MODE(musb);
			musb->xceiv.default_a = 0;
			musb->xceiv.state = OTG_STATE_B_IDLE;
			portstate(musb->port1_status &= ~USB_PORT_STAT_POWER);
		}

		/* NOTE: this must complete poweron within 100 ms. */
		da8xx_source_power(musb, drvvbus, 0);
		DBG(2, "VBUS %s (%s)%s, devctl %02x\n",
				drvvbus ? "on" : "off",
				otg_state_string(musb),
				err ? " ERROR" : "",
				devctl);
		ret = IRQ_HANDLED;
	}

	if (musb->int_tx || musb->int_rx || musb->int_usb)
		ret |= musb_interrupt(musb);

	/* EOI needs to be written for the IRQ to be re-asserted. */
	if (ret == IRQ_HANDLED)
		musb_writel(reg_base, USB_END_OF_INTR_REG, 0);

	musb->int_regs = NULL;

	/* Poll for ID change */
	if (is_otg_enabled(musb) && musb->xceiv.state == OTG_STATE_B_IDLE)
		mod_timer(&otg_workaround, jiffies + POLL_SECONDS * HZ);

	spin_unlock_irqrestore(&musb->lock, flags);

	/*
	 * REVISIT: we sometimes get unhandled IRQs (e.g. EP0) --
	 * not clear why...
	 */
	ret |= IRQ_RETVAL(cppi_intr);
	if (ret != IRQ_HANDLED)
		DBG(5, "Unhandled IRQ %08x?\n", status);
	return ret;
}

void musb_platform_set_mode(struct musb *musb, u8 musb_mode)
{
	WARN("FIXME: %s not implemented\n", __func__);
}

int __init musb_platform_init(struct musb *musb)
{
	void __iomem *reg_base = musb->ctrl_base;
	u32 rev;

	musb->mregs += USB_MENTOR_CORE_OFFSET;

	musb->clock = clk_get(NULL, "USB20CLK");
	if (IS_ERR(musb->clock))
		return PTR_ERR(musb->clock);

	if (clk_enable(musb->clock) < 0)
		return -ENODEV;

	/* Returns zero if e.g. not clocked */
	rev = musb_readl(reg_base, USB_REVISION_REG);
	if (!rev)
		return -ENODEV;

	if (is_host_enabled(musb))
		setup_timer(&otg_workaround, otg_timer, (unsigned long) musb);

	musb->board_set_vbus = da8xx_set_vbus;
	da8xx_source_power(musb, 0, 1);

	/* Reset the controller */
	musb_writel(reg_base, USB_CTRL_REG, USB_SOFT_RESET_MASK);

	/* Start the on-chip PHY and its PLL. */
	phy_on();

	msleep(5);

	/* NOTE: IRQs are in mixed mode, not bypass to pure MUSB */
	pr_debug("DA8xx OTG revision %08x, PHY %03x, control %02x\n",
		 rev, davinci_readl(BOOTCFG_CFGCHIP2_REG),
		 musb_readb(reg_base, USB_CTRL_REG));

	musb->isr = da8xx_interrupt;
	return 0;
}

int musb_platform_exit(struct musb *musb)
{
	if (is_host_enabled(musb))
		del_timer_sync(&otg_workaround);

	da8xx_source_power(musb, 0 /* off */, 1);

	/* Delay to avoid problems with module reload... */
	if (is_host_enabled(musb) && musb->xceiv.default_a) {
		int maxdelay = 30;
		u8 devctl, warn = 0;

		/*
		 * If there's no peripheral connected, this can take a
		 * long time to fall...
		 */
		do {
			devctl = musb_readb(musb->mregs, MUSB_DEVCTL);
			if (!(devctl & MUSB_DEVCTL_VBUS))
				break;
			if ((devctl & MUSB_DEVCTL_VBUS) != warn) {
				warn = devctl & MUSB_DEVCTL_VBUS;
				DBG(1, "VBUS %d\n",
					warn >> MUSB_DEVCTL_VBUS_SHIFT);
			}
			msleep(1000);
			maxdelay--;
		} while (maxdelay > 0);

		/* In OTG mode, another host might be connected... */
		if (devctl & MUSB_DEVCTL_VBUS)
			DBG(1, "VBUS off timeout (devctl %02x)\n", devctl);
	}

	phy_off();
	return 0;
}
