/*
 * Low-level board setup code for TI DaVinci SoC based boards.
 *
 * Copyright (C) 2007 Sergey Kubushyn <ksi@koi8.net>
 *
 * Partially based on TI sources, original copyrights follow:
 */

/*
 * Board specific setup info
 *
 * (C) Copyright 2003
 * Texas Instruments, <www.ti.com>
 * Kshitij Gupta <Kshitij@ti.com>
 *
 * Modified for OMAP 1610 H2 board by Nishant Kamat, Jan 2004
 *
 * Modified for OMAP 5912 OSK board by Rishi Bhattacharya, Apr 2004
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * Modified for DV-EVM board by Rishi Bhattacharya, Apr 2005
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * Modified for DV-EVM board by Swaminathan S, Nov 2005
 * See file CREDITS for list of people who contributed to this
 * project.
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
 */

#include <config.h>

.globl	lowlevel_init
lowlevel_init:
	/* Go back if we are running from RAM or NAND. */
#ifdef CFG_PCI_BOOT
	mov	pc, lr
#endif /* CFG_PCI_BOOT */

	/*-------------------------------------------------------*
	 * Mask all IRQs by setting all bits in the EINT default *
	 *-------------------------------------------------------*/
	mov	r1, $0
	ldr	r0, =EINT_ENABLE0
	str	r1, [r0]
	ldr	r0, =EINT_ENABLE1
	str	r1, [r0]

	/*------------------------------------------------------*
	 * Put the GEM in reset					*
	 *------------------------------------------------------*/

	/* Put the GEM in reset */
	/* No GEM in DM355 */
#if !defined(CFG_DM355_EVM) && !defined(CFG_DM365_EVM)
	ldr	r8, PSC_GEM_FLAG_CLEAR
	ldr	r6, MDCTL_GEM
	ldr	r7, [r6]
	and	r7, r7, r8
	str	r7, [r6]

	/* Enable the Power Domain Transition Command */
	ldr	r6, PTCMD
	ldr	r7, [r6]
	orr	r7, r7, $0x02
	str	r7, [r6]

	/* Check for Transition Complete(PTSTAT) */
checkStatClkStopGem:
	ldr	r6, PTSTAT
	ldr	r7, [r6]
	ands	r7, r7, $0x02
	bne	checkStatClkStopGem

	/* Check for GEM Reset Completion */
checkGemStatClkStop:
	ldr	r6, MDSTAT_GEM
	ldr	r7, [r6]
	ands	r7, r7, $0x100
	bne	checkGemStatClkStop

	/* Do this for enabling a WDT initiated reset this is a workaround
	   for a chip bug.  Not required under normal situations */
	ldr	r6, P1394
	mov	r10, $0
	str	r10, [r6]
#endif
	/*------------------------------------------------------*
	 * Enable L1 & L2 Memories in Fast mode                 *
	 *------------------------------------------------------*/
	ldr	r6, DFT_ENABLE
	mov	r10, $0x01
	str	r10, [r6]

	ldr	r6, MMARG_BRF0
	ldr	r10, MMARG_BRF0_VAL
	str	r10, [r6]

	ldr	r6, DFT_ENABLE
	mov	r10, $0
	str	r10, [r6]

	/*------------------------------------------------------*
	 * DDR2 PLL Initialization				*
	 *------------------------------------------------------*/

	/* Select the Clock Mode Depending on the Value written in the Boot Table by the run script */
	mov	r10, $0
	ldr	r6, PLL2_CTL
	ldr	r7, PLL_CLKSRC_MASK
	ldr	r8, [r6]
	and	r8, r8, r7
	mov	r9, r10, lsl $8
	orr	r8, r8, r9
	str	r8, [r6]

	/* Select the PLLEN source */
	ldr	r7, PLL_ENSRC_MASK
	and	r8, r8, r7
	str	r8, [r6]

	/* Bypass the PLL */
	ldr	r7, PLL_BYPASS_MASK
	and	r8, r8, r7
	str	r8, [r6]

	/* Wait for few cycles to allow PLLEN Mux switch properly to bypass Clock */
	mov	r10, $0x20
WaitPPL2Loop:
	subs	r10, r10, $1
	bne	WaitPPL2Loop

	/* Reset the PLL */
	ldr	r7, PLL_RESET_MASK
	and	r8, r8, r7
	str	r8, [r6]

	/* Power up the PLL */
	ldr	r7, PLL_PWRUP_MASK
	and	r8, r8, r7
	str	r8, [r6]

	/* Enable the PLL from Disable Mode */
	ldr	r7, PLL_DISABLE_ENABLE_MASK
	and	r8, r8, r7
	str	r8, [r6]

	/* Program the PLL Multiplier */
	ldr	r6, PLL2_PLLM
#ifdef CFG_DM6467_EVM
	mov	r2, $0x15
#else
	mov	r2, $0x17	/* 162 MHz */
#endif
	str	r2, [r6]

#ifndef CFG_DM6467_EVM
	/* Program the PLL2 Divisor Value */
	ldr	r6, PLL2_DIV2
	mov	r3, $0x01
	str	r3, [r6]
#endif

	/* Program the PLL2 Divisor Value */
	ldr	r6, PLL2_DIV1
#ifdef CFG_DM6467_EVM
	mov	r4, $0x1
#else
	mov	r4, $0x0b	/* 54 MHz */
#endif
	str	r4, [r6]

#ifndef CFG_DM6467_EVM
	/* PLL2 DIV2 MMR */
	ldr	r8, PLL2_DIV_MASK
	ldr	r6, PLL2_DIV2
	ldr	r9, [r6]
	and	r8, r8, r9
	mov	r9, $0x01
	mov	r9, r9, lsl $15
	orr	r8, r8, r9
	str	r8, [r6]

	/* Program the GOSET bit to take new divider values */
	ldr	r6, PLL2_PLLCMD
	ldr	r7, [r6]
	orr	r7, r7, $0x01
	str	r7, [r6]

	/* Wait for Done */
	ldr	r6, PLL2_PLLSTAT
doneLoop_0:
	ldr	r7, [r6]
	ands	r7, r7, $0x01
	bne	doneLoop_0
#endif

	/* PLL2 DIV1 MMR */
	ldr	r8, PLL2_DIV_MASK
	ldr	r6, PLL2_DIV1
	ldr	r9, [r6]
	and	r8, r8, r9
	mov	r9, $0x01
	mov	r9, r9, lsl $15
	orr	r8, r8, r9
	str	r8, [r6]

	/* Program the GOSET bit to take new divider values */
	ldr	r6, PLL2_PLLCMD
	ldr	r7, [r6]
	orr	r7, r7, $0x01
	str	r7, [r6]

	/* Wait for Done */
	ldr	r6, PLL2_PLLSTAT
doneLoop:
	ldr	r7, [r6]
	ands	r7, r7, $0x01
	bne	doneLoop

	/* Wait for PLL to Reset Properly */
	mov	r10, $0x218
ResetPPL2Loop:
	subs	r10, r10, $1
	bne	ResetPPL2Loop

	/* Bring PLL out of Reset */
	ldr	r6, PLL2_CTL
	ldr	r8, [r6]
	orr	r8, r8, $0x08
	str	r8, [r6]

	/* Wait for PLL to Lock */
	ldr	r10, PLL_LOCK_COUNT
PLL2Lock:
	subs	r10, r10, $1
	bne	PLL2Lock

	/* Enable the PLL */
	ldr	r6, PLL2_CTL
	ldr	r8, [r6]
	orr	r8, r8, $0x01
	str	r8, [r6]

	/*------------------------------------------------------*
	 * Issue Soft Reset to DDR Module			*
	 *------------------------------------------------------*/

	/* Shut down the DDR2 LPSC Module */
	ldr	r8, PSC_FLAG_CLEAR
	ldr	r6, MDCTL_DDR2
	ldr	r7, [r6]
	and	r7, r7, r8
	orr	r7, r7, $0x03
	str	r7, [r6]

	/* Enable the Power Domain Transition Command */
	ldr	r6, PTCMD
	ldr	r7, [r6]
	orr	r7, r7, $0x01
	str	r7, [r6]

	/* Check for Transition Complete(PTSTAT) */
checkStatClkStop:
	ldr	r6, PTSTAT
	ldr	r7, [r6]
	ands	r7, r7, $0x01
	bne	checkStatClkStop

	/* Check for DDR2 Controller Enable Completion */
checkDDRStatClkStop:
	ldr	r6, MDSTAT_DDR2
	ldr	r7, [r6]
	and	r7, r7, $0x1f
	cmp	r7, $0x03
	bne	checkDDRStatClkStop

	/*------------------------------------------------------*
	 * Program DDR2 MMRs for 162MHz Setting			*
	 *------------------------------------------------------*/

	/* Program PHY Control Register */
	ldr	r6, DDRCTL
	ldr	r7, DDRCTL_VAL
	str	r7, [r6]

	/* Program SDRAM Bank Config Register */
	ldr	r6, SDCFG
	ldr	r7, SDCFG_VAL
	str	r7, [r6]

	/* Program SDRAM TIM-0 Config Register */
	ldr	r6, SDTIM0
	ldr	r7, SDTIM0_VAL
	str	r7, [r6]

	/* Program SDRAM TIM-1 Config Register */
	ldr	r6, SDTIM1
	ldr	r7, SDTIM1_VAL
	str	r7, [r6]

	/* Program the SDRAM Bank Config Control Register */
	ldr	r10, MASK_VAL
	ldr	r8, SDCFG
	ldr	r9, SDCFG_VAL
	and	r9, r9, r10
	str	r9, [r8]

	/* Program SDRAM SDREF Config Register */
	ldr	r6, SDREF
	ldr	r7, SDREF_VAL
	str	r7, [r6]

	/*------------------------------------------------------*
	 * Issue Soft Reset to DDR Module			*
	 *------------------------------------------------------*/

	/* Issue a Dummy DDR2 read/write */
	ldr	r8, DDR2_START_ADDR
	ldr	r7, DUMMY_VAL
	str	r7, [r8]
	ldr	r7, [r8]

	/* Shut down the DDR2 LPSC Module */
	ldr	r8, PSC_FLAG_CLEAR
	ldr	r6, MDCTL_DDR2
	ldr	r7, [r6]
	and	r7, r7, r8
	orr	r7, r7, $0x01
	str	r7, [r6]

	/* Enable the Power Domain Transition Command */
	ldr	r6, PTCMD
	ldr	r7, [r6]
	orr	r7, r7, $0x01
	str	r7, [r6]

	/* Check for Transition Complete(PTSTAT) */
checkStatClkStop2:
	ldr	r6, PTSTAT
	ldr	r7, [r6]
	ands	r7, r7, $0x01
	bne	checkStatClkStop2

	/* Check for DDR2 Controller Enable Completion */
checkDDRStatClkStop2:
	ldr	r6, MDSTAT_DDR2
	ldr	r7, [r6]
	and	r7, r7, $0x1f
	cmp	r7, $0x01
	bne	checkDDRStatClkStop2

	/*------------------------------------------------------*
	 * Turn DDR2 Controller Clocks On			*
	 *------------------------------------------------------*/

	/* Enable the DDR2 LPSC Module */
	ldr	r6, MDCTL_DDR2
	ldr	r7, [r6]
	orr	r7, r7, $0x03
	str	r7, [r6]

	/* Enable the Power Domain Transition Command */
	ldr	r6, PTCMD
	ldr	r7, [r6]
	orr	r7, r7, $0x01
	str	r7, [r6]

	/* Check for Transition Complete(PTSTAT) */
checkStatClkEn2:
	ldr	r6, PTSTAT
	ldr	r7, [r6]
	ands	r7, r7, $0x01
	bne	checkStatClkEn2

	/* Check for DDR2 Controller Enable Completion */
checkDDRStatClkEn2:
	ldr	r6, MDSTAT_DDR2
	ldr	r7, [r6]
	and	r7, r7, $0x1f
	cmp	r7, $0x03
	bne	checkDDRStatClkEn2

	/*  DDR Writes and Reads */
	ldr	r6, CFGTEST
	mov	r3, $0x01
	str	r3, [r6]

	/*------------------------------------------------------*
	 * System PLL Initialization				*
	 *------------------------------------------------------*/

	/* Select the Clock Mode Depending on the Value written in the Boot Table by the run script */
	mov	r2, $0
	ldr	r6, PLL1_CTL
	ldr	r7, PLL_CLKSRC_MASK
	ldr	r8, [r6]
	and	r8, r8, r7
	mov	r9, r2, lsl $8
	orr	r8, r8, r9
	str	r8, [r6]

	/* Select the PLLEN source */
	ldr	r7, PLL_ENSRC_MASK
	and	r8, r8, r7
	str	r8, [r6]

	/* Bypass the PLL */
	ldr	r7, PLL_BYPASS_MASK
	and	r8, r8, r7
	str	r8, [r6]

	/* Wait for few cycles to allow PLLEN Mux switch properly to bypass Clock */
	mov	r10, $0x20

WaitLoop:
	subs	r10, r10, $1
	bne	WaitLoop

	/* Reset the PLL */
	ldr	r7, PLL_RESET_MASK
	and	r8, r8, r7
	str	r8, [r6]

	/* Disable the PLL */
	orr	r8, r8, $0x10
	str	r8, [r6]

	/* Power up the PLL */
	ldr	r7, PLL_PWRUP_MASK
	and	r8, r8, r7
	str	r8, [r6]

	/* Enable the PLL from Disable Mode */
	ldr	r7, PLL_DISABLE_ENABLE_MASK
	and	r8, r8, r7
	str	r8, [r6]

	/* Program the PLL Multiplier */
	ldr	r6, PLL1_PLLM
	mov	r3, $0x15	/* For 594MHz */
	str	r3, [r6]

	/* Wait for PLL to Reset Properly */
	mov	r10, $0xff

ResetLoop:
	subs	r10, r10, $1
	bne	ResetLoop

	/* Bring PLL out of Reset */
	ldr	r6, PLL1_CTL
	orr	r8, r8, $0x08
	str	r8, [r6]

	/* Wait for PLL to Lock */
	ldr	r10, PLL_LOCK_COUNT

PLL1Lock:
	subs	r10, r10, $1
	bne	PLL1Lock

	/* Enable the PLL */
	orr	r8, r8, $0x01
	str	r8, [r6]

	nop
	nop
	nop
	nop

	/*------------------------------------------------------*
	 * AEMIF configuration for NOR Flash (double check)     *
	 *------------------------------------------------------*/
	ldr	r0, _PINMUX0
	ldr	r1, _DEV_SETTING
	str	r1, [r0]

	ldr	r0, WAITCFG
	ldr	r1, WAITCFG_VAL
	ldr	r2, [r0]
	orr	r2, r2, r1
	str	r2, [r0]

#ifdef CFG_DM6467_EVM
	ldr	r0, ACFG2
	ldr	r1, ACFG2_VAL
	ldr	r2, [r0]
	and	r1, r2, r1
	str	r1, [r0]
#endif
	ldr	r0, ACFG3
	ldr	r1, ACFG3_VAL
	ldr	r2, [r0]
	and	r1, r2, r1
	str	r1, [r0]

	ldr	r0, ACFG4
	ldr	r1, ACFG4_VAL
	ldr	r2, [r0]
	and	r1, r2, r1
	str	r1, [r0]

	ldr	r0, ACFG5
	ldr	r1, ACFG5_VAL
	ldr	r2, [r0]
	and	r1, r2, r1
	str	r1, [r0]

	/*--------------------------------------*
	 * VTP manual Calibration               *
	 *--------------------------------------*/
	ldr	r0, VTPIOCR
	ldr	r1, VTP_MMR0
	str	r1, [r0]

	ldr	r0, VTPIOCR
	ldr	r1, VTP_MMR1
	str	r1, [r0]

	/* Wait for 33 VTP CLK cycles.  VRP operates at 27 MHz */
	ldr	r10, VTP_LOCK_COUNT
VTPLock:
	subs	r10, r10, $1
	bne	VTPLock

	ldr	r6, DFT_ENABLE
	mov	r10, $0x01
	str	r10, [r6]

	ldr	r6, DDRVTPR
	ldr	r7, [r6]
	and	r7, r7, $0x1f
	and	r8, r7, $0x3e0
	orr	r8, r7, r8
	ldr	r7, VTP_RECAL
	orr	r8, r7, r8
	ldr	r7, VTP_EN
	orr	r8, r7, r8
	str	r8, [r0]


	/* Wait for 33 VTP CLK cycles.  VRP operates at 27 MHz */
	ldr	r10, VTP_LOCK_COUNT
VTP1Lock:
	subs	r10, r10, $1
	bne	VTP1Lock

	ldr	r1, [r0]
	ldr	r2, VTP_MASK
	and	r2, r1, r2
	str	r2, [r0]

	ldr	r6, DFT_ENABLE
	mov	r10, $0
	str	r10, [r6]

	/*
	 * Call board-specific lowlevel init.
	 * That MUST be present and THAT returns
	 * back to arch calling code with "mov pc, lr."
	 */
	b	dv_board_init

.ltorg

_PINMUX0:
	.word	0x01c40000		/* Device Configuration Registers */
_PINMUX1:
	.word	0x01c40004		/* Device Configuration Registers */

_DEV_SETTING:
#ifdef CFG_DM6467_EVM
	.word	0x00000000		/* AEMIF Enable (PCIEN=HPIEN=ATAEN=0) */
#else
	.word	0x00000c1f
#endif

WAITCFG:
#ifdef CFG_DM355_EVM
	.word	0x01e10004
#elif defined(CFG_DM6467_EVM)
	.word	0x20008004
#elif defined(CFG_DM365_EVM)
	.word	0x01d10004
#else
	.word	0x01e00004
#endif
WAITCFG_VAL:
	.word	0
ACFG2:
#ifdef CFG_DM365_EVM
	.word	0x01d10010
#else	
	.word	0x20008010
#endif
ACFG2_VAL:
	.word	0x1
ACFG3:
#ifdef CFG_DM355_EVM
	.word	0x01e10014
#elif defined(CFG_DM365_EVM)
	.word	0x01d10014
#elif defined(CFG_DM6467_EVM)
	.word	0x20008014
#else
	.word	0x01e00014
#endif
ACFG3_VAL:
#if defined(CFG_DM6467_EVM) || defined(CFG_DM365_EVM)
	.word	0x1
#else
	.word	0x3ffffffd
#endif
ACFG4:
#ifdef CFG_DM355_EVM
	.word	0x01e10018
#elif defined(CFG_DM6467_EVM)
	.word	0x20008018
#else
	.word	0x01e00018
#endif
ACFG4_VAL:
#ifdef CFG_DM6467_EVM
	.word	0x1
#else
	.word	0x3ffffffd
#endif
ACFG5:
#ifdef CFG_DM355_EVM
	.word	0x01e1001c
#elif defined(CFG_DM6467_EVM)
	.word	0x2000801c
#else
	.word	0x01e0001c
#endif
ACFG5_VAL:
#ifdef CFG_DM6467_EVM
	.word	0x1
#else
	.word	0x3ffffffd
#endif
MDCTL_DDR2:
#ifdef CFG_DM6467_EVM
	.word	0x01c41a50
#else
	.word	0x01c41a34
#endif
MDSTAT_DDR2:
#ifdef CFG_DM6467_EVM
	.word	0x01c41850
#else
	.word	0x01c41834
#endif
PTCMD:
	.word	0x01c41120
PTSTAT:
	.word	0x01c41128

EINT_ENABLE0:
	.word	0x01c48018
EINT_ENABLE1:
	.word	0x01c4801c

PSC_FLAG_CLEAR:
	.word	0xffffffe0
PSC_GEM_FLAG_CLEAR:
	.word	0xfffffeff

/* DDR2 MMR & CONFIGURATION VALUES, 162 MHZ clock */
DDRCTL:
	.word	0x2000004
DDRCTL_VAL:
#ifdef CFG_DM6467_EVM
	.word	0x50006404
#elif defined(CFG_DM365_EVM)
	.word	0x000080c4
#else
	.word	0x50006405
#endif
SDREF:
	.word	0x2000000c
SDREF_VAL:
#ifdef CFG_DM355_EVM
	.word	0x000004f0
#elif defined(CFG_DM6467_EVM)
	.word	0x00000927
#elif defined(CFG_DM365_EVM)
	.word	0x00000768
#else
	.word	0x000005c3
#endif
SDCFG:
	.word	0x20000008
SDCFG_VAL:
#ifdef CFG_DM6467_EVM
	.word	0x00008832
#elif defined(CFG_DM365_EVM)
	.word	0x08534832
#endif
#ifndef CFG_DM6467_EVM
#ifndef CFG_DM355_EVM
#ifndef CFG_DM365_EVM
#ifdef	DDR_4BANKS
	.word	0x00178622
#elif defined DDR_8BANKS
	.word	0x00178632
#else
#error "Unknown DDR configuration!!!"
#endif
#endif
#endif
#endif
SDTIM0:
	.word	0x20000010
SDTIM0_VAL:
#ifdef CFG_DM6467_EVM
	.word	0x4D243232	/* value for 297 MHZ */
#elif defined(CFG_DM365_EVM)
	.word	0x3C934B51	/* value for 243 MHZ */
#else
	.word	0x28923211	/* value for 162 MHZ */
#endif
SDTIM1:
	.word	0x20000014
SDTIM1_VAL:
#ifdef CFG_DM6467_EVM
	.word	0x0B29C742	/* value for 297 MHZ */
#elif defined(CFG_DM365_EVM)
	.word	0x4221C722	/* value for 243 MHZ */
#else
	.word	0x0016c722	/* value for 162 MHZ */
#endif
VTPIOCR:
	.word	0x200000f0	/* VTP IO Control register */
DDRVTPR:
	.word	0x01c42030	/* DDR VPTR MMR */
VTP_MMR0:
	.word	0x201f
VTP_MMR1:
	.word	0xa01f
DFT_ENABLE:
	.word	0x01c4004c
VTP_LOCK_COUNT:
	.word	0x5b0
VTP_MASK:
	.word	0xffffdfff
VTP_RECAL:
	.word	0x40000
VTP_EN:
	.word	0x02000
CFGTEST:
	.word	0x80010000
MASK_VAL:
#ifdef CFG_DM355_EVM
	.word	0xffff7fff
#elif defined(CFG_DM6467_EVM)
	.word	0xffff7fff
#else
	.word	0x00000fff
#endif

/* GEM Power Up & LPSC Control Register */
MDCTL_GEM:
#ifdef CFG_DM6467_EVM
	.word	0x01C41a04
#else
	.word	0x01c41a9c
#endif
MDSTAT_GEM:
#ifdef CFG_DM6467_EVM
	.word	0x01C41804
#else
	.word	0x01c4189c
#endif

/* For WDT reset chip bug */
P1394:
	.word	0x01c41a20

PLL_CLKSRC_MASK:
	.word	0xfffffeff	/* Mask the Clock Mode bit */
PLL_ENSRC_MASK:
	.word	0xffffffdf	/* Select the PLLEN source */
PLL_BYPASS_MASK:
	.word	0xfffffffe	/* Put the PLL in BYPASS */
PLL_RESET_MASK:
	.word	0xfffffff7	/* Put the PLL in Reset Mode */
PLL_PWRUP_MASK:
	.word	0xfffffffd	/* PLL Power up Mask Bit  */
PLL_DISABLE_ENABLE_MASK:
	.word	0xffffffef	/* Enable the PLL from Disable */
PLL_LOCK_COUNT:
	.word	0x2000

/* PLL1-SYSTEM PLL MMRs */
PLL1_CTL:
	.word	0x01c40900
PLL1_PLLM:
	.word	0x01c40910

/* PLL2-SYSTEM PLL MMRs */
PLL2_CTL:
	.word	0x01c40d00
PLL2_PLLM:
	.word	0x01c40d10
PLL2_DIV1:
	.word	0x01c40d18
PLL2_DIV2:
	.word	0x01c40d1c
PLL2_PLLCMD:
	.word	0x01c40d38
PLL2_PLLSTAT:
	.word	0x01c40d3c
PLL2_DIV_MASK:
	.word	0xffff7fff

MMARG_BRF0:
	.word	0x01c42010	/* BRF margin mode 0 (R/W)*/
MMARG_BRF0_VAL:
	.word	0x00444400

DDR2_START_ADDR:
	.word	0x80000000
DUMMY_VAL:
	.word	0xa55aa55a
