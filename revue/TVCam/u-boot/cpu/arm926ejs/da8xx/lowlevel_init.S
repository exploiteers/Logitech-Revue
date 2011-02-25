/*
 * Low-level board setup code for TI DA8xx SoC based boards.
 *
 * Copyright (C) 2008 Texas Instruments, Inc <www.ti.com>
 * Sekhar Nori <nsekhar@ti.com>
 * 
 * Based on TI DaVinci low level init code. Original copyrights follow.
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
#include <asm/arch/hardware.h>

.globl	lowlevel_init
lowlevel_init:

	nop
	nop
	nop
	nop

	/*
	 * Call board-specific lowlevel init.
 	 * That MUST be present and THAT returns
	 * back to arch calling code with "mov pc, lr."
	 */
	b	dv_board_init

#ifndef CONFIG_USE_IRQ
	/*-------------------------------------------------------*
	 * Mask all IRQs by clearing the global enable and setting
     * the enable clear for all the 90 interrupts.
	 *-------------------------------------------------------*/
	mov	r1, $0
	ldr	r0, INTC_GLB_EN_ADDR
	str	r1, [r0]

	ldr r0, INTC_HINT_EN_ADDR
	str	r1, [r0]
	add	r0, r0, $4
	str	r1, [r0]
	add	r0, r0, $4
	str	r1, [r0]

    mvn r1, r1  
	ldr	r0, INTC_EN_CLR0_ADDR
	str	r1, [r0]
	add	r0, r0, $4
	str	r1, [r0]
	add	r0, r0, $4
	str	r1, [r0]
#endif

	/*------------------------------------------------------*
	 * PLL0 Initialization - works only in non-secure devices
	 *------------------------------------------------------*/

     /* TODO: Write the kick values and the PLL master lock bits */

	/* Select OSCIN in clockmode bit in PLLCTL register. This is the only 
     * clock mode supported on DA8xx
     */
	ldr	r6, PLL0_PLLCTL_ADDR
	ldr	r7, PLL_CLKSRC_MASK
	ldr	r8, [r6]
	and	r8, r8, r7
	str	r8, [r6]

	/* Clear the PLLENSRC bit in PLLCTL */
	ldr	r7, PLL_ENSRC_MASK
	and	r8, r8, r7
	str	r8, [r6]

	/* Bypass the PLL */
	ldr	r7, PLL_BYPASS_MASK
	and	r8, r8, r7
	str	r8, [r6]

	/* Wait for few cycles to allow PLLEN Mux switch properly to bypass Clock */
	mov	r10, $0x20
WaitPLL0Loop:
	subs	r10, r10, $1
	bne	WaitPLL0Loop

	/* Reset the PLL */
	ldr	r7, PLL_RESET_MASK
	and	r8, r8, r7
	str	r8, [r6]
   
    /* disable PLL output */
    mov r7, $0x10
    orr r8, r8, r7
    str r8, [r6]

	/* Power up the PLL */
	ldr	r7, PLL_PWRUP_MASK
	and	r8, r8, r7
	str	r8, [r6]

	/* Enable the PLL from Disable Mode */
	ldr	r7, PLL_DISABLE_ENABLE_MASK
	and	r8, r8, r7
	str	r8, [r6]

    /* Wait for the PLL stabilization time - 150us assumed */
    mov r10, $0xE10
PLLStableLoop:
	subs	r10, r10, $1
	bne PLLStableLoop

	/* Program the PLL Multiplier */
	ldr	r6, PLL0_PLLM_ADDR
	mov	r2, $0x18	/* 24 * 25 = 600MHz*/
	str	r2, [r6]
    
	/* Program the POSTDIV Value */
	ldr	r6, PLL0_POSTDIV_ADDR
	mov	r3, $0x01
	mov	r3, r3, lsl $15
    orr r3, r3, $0x01
	str	r3, [r6]

    /* Program the SYSCLKx dividedrs */

    /* Following defaults are good:
     * SYSCLK1 = /1 => 300MHz GEM 
     * SYSCLK2 = /2 => 150MHz EDMA, MMC/SD, UART1/2, SPI1 
     * SYSCLK3 = /3 => 100MHz Possible EMIF2.5
     * SYSCLK4 = /4 => 75MHz  INTC, PSC, EMAC, USB 1.1, I2C1
     * SYSCLK5 = /3 => 100MHz Possible EMIF3.0 (use DIV4p5)
     * SYSCLK6 = /1 => 300MHz ARM
     * SYSCLK7 = /6 => 50MHz  RMII Ref Clk
     * SYSCLK8 = /6 => 50MHz  Dont use
     * SYSCLK9 = /6 => 50MHz  Dont use
     * AUXCLK FIXED => 24Mhz  Timer 0/1, 12C0, McASP AuxClk
     */
     
	/* Wait for PLL to Reset Properly - 128 OSCIN cycles*/
	mov	r10, $128
ResetPPL2Loop:
	subs	r10, r10, $1
	bne	ResetPPL2Loop

	/* Bring PLL out of Reset */
	ldr	r6, PLL0_PLLCTL_ADDR
	ldr	r8, [r6]
	orr	r8, r8, $0x08
	str	r8, [r6]

	/* Wait for PLL to Lock */
	ldr	r10, PLL_LOCK_COUNT
PLL0Lock:
	subs	r10, r10, $1
	bne	PLL0Lock

	/* Enable the PLL */
	ldr	r6, PLL0_PLLCTL_ADDR
	ldr	r8, [r6]
	orr	r8, r8, $0x01
	str	r8, [r6]

	/*------------------------------------------------------*
	 * Setup the pinmux for DDR2			                *
	 *------------------------------------------------------*/	 

	ldr	r0, PINMUX1_ADDR
	ldr	r1, PINMUX1_VAL
	str	r1, [r0]

	ldr	r0, PINMUX2_ADDR
	ldr	r1, PINMUX2_VAL
	str	r1, [r0]

	ldr	r0, PINMUX5_ADDR
	ldr	r1, PINMUX5_VAL
	str	r1, [r0]
	
	ldr	r0, PINMUX6_ADDR
	ldr	r1, PINMUX6_VAL
	str	r1, [r0]

	ldr r8, PINMUX7_FLAG_CLEAR
	ldr r7, PINMUX7_VAL
	ldr r0, PINMUX7_ADDR
	ldr r1, [r0]
	and r1, r1, r8
	orr r1, r1, r7
	str r1, [r0]


	/*------------------------------------------------------*
	 * Get the EMIF3 out of reset			                *
	 *------------------------------------------------------*/	 

	/* Shut down the DDR2 LPSC Module */
	ldr	r8, PSC_FLAG_CLEAR
	ldr	r6, MDCTL_EMIF3
	ldr	r7, [r6]
	and	r7, r7, r8
	orr	r7, r7, $0x03
	str	r7, [r6]

	/* Enable the Power Domain Transition Command */
	ldr	r6, PSC1_PTCMD_ADDR
	ldr	r7, [r6]
	orr	r7, r7, $0x01
	str	r7, [r6]

	/* Check for Transition Complete(PTSTAT) */
checkStatClkStop0:
	ldr	r6, PSC1_PTSTAT_ADDR
	ldr	r7, [r6]
	ands	r7, r7, $0x01
	bne	checkStatClkStop0

	/* Check for DDR2 Controller Enable Completion */
checkDDRStatClkStop0:
	ldr	r6, MDSTAT_EMIF3
	ldr	r7, [r6]
	and	r7, r7, $0x1f
	cmp	r7, $0x03
	bne	checkDDRStatClkStop0

	/*------------------------------------------------------*
	 * Put the EMIF3 in reset			                *
	 *------------------------------------------------------*/

	/* Shut down the DDR2 LPSC Module */
	ldr	r8, PSC_FLAG_CLEAR
	ldr	r6, MDCTL_EMIF3
	ldr	r7, [r6]
	and	r7, r7, r8
	orr	r7, r7, $0x01
	str	r7, [r6]

	/* Enable the Power Domain Transition Command */
	ldr	r6, PSC1_PTCMD_ADDR
	ldr	r7, [r6]
	orr	r7, r7, $0x01
	str	r7, [r6]

	/* Check for Transition Complete(PTSTAT) */
checkStatClkStop1:
	ldr	r6, PSC1_PTSTAT_ADDR
	ldr	r7, [r6]
	ands	r7, r7, $0x01
	bne	checkStatClkStop1

	/* Check for DDR2 Controller Enable Completion */
checkDDRStatClkStop1:
	ldr	r6, MDSTAT_EMIF3
	ldr	r7, [r6]
	and	r7, r7, $0x1f
	cmp	r7, $0x01
	bne	checkDDRStatClkStop1

    nop
    nop
    
	/*------------------------------------------------------*
	 * Get the EMIF3 out of reset			                *
	 *------------------------------------------------------*/

	/* Shut down the DDR2 LPSC Module */
	ldr	r8, PSC_FLAG_CLEAR
	ldr	r6, MDCTL_EMIF3
	ldr	r7, [r6]
	and	r7, r7, r8
	orr	r7, r7, $0x03
	str	r7, [r6]

	/* Enable the Power Domain Transition Command */
	ldr	r6, PSC1_PTCMD_ADDR
	ldr	r7, [r6]
	orr	r7, r7, $0x01
	str	r7, [r6]

	/* Check for Transition Complete(PTSTAT) */
checkStatClkStop2:
	ldr	r6, PSC1_PTSTAT_ADDR
	ldr	r7, [r6]
	ands	r7, r7, $0x01
	bne	checkStatClkStop2

	/* Check for DDR2 Controller Enable Completion */
checkDDRStatClkStop2:
	ldr	r6, MDSTAT_EMIF3
	ldr	r7, [r6]
	and	r7, r7, $0x1f
	cmp	r7, $0x03
	bne	checkDDRStatClkStop2

    nop
    nop

    /*-----------------------------------------------------*
     * Wait before programing the SDRAM timimg values      *
     *-----------------------------------------------------*/

     ldr r10, EMIF3_TIMING_WAIT_VAL
emif3TimingWait:
    sub r10, r10, $0x1
    cmp r10, $0x0
    bne emif3TimingWait

	/*------------------------------------------------------*
	 * Program EMIF3 MMRs for 133MHz SDRAM Setting			*
	 *------------------------------------------------------*/

	/* Program SDRAM Bank Config Register */
	ldr	r6, SDCFG
	ldr	r7, SDCFG_VAL
	str	r7, [r6]

	/* Program SDRAM TIM-0 Config Register */
	ldr	r6, SDTIM0
	ldr	r7, SDTIM0_VAL_162MHz
	str	r7, [r6]

	/* Program SDRAM TIM-1 Config Register */
	ldr	r6, SDTIM1
	ldr	r7, SDTIM1_VAL_162MHz
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

	/* Issue a Dummy DDR2 read/write */
	ldr	r8, DDR2_START_ADDR
	ldr	r7, DUMMY_VAL
	str	r7, [r8]
	ldr	r7, [r8]

	/*  DDR Writes and Reads */
	ldr	r6, CFGTEST
	mov	r3, $0x01
	str	r3, [r6]

	nop
	nop
	nop
	nop

	/*
	 * Call board-specific lowlevel init.
 	 * That MUST be present and THAT returns
	 * back to arch calling code with "mov pc, lr."
	 */
	b	dv_board_init

.ltorg

PINMUX1_ADDR:
	.word PINMUX1
PINMUX2_ADDR:
	.word PINMUX2
PINMUX5_ADDR:
	.word PINMUX5
PINMUX6_ADDR:
	.word PINMUX6
PINMUX7_ADDR:
	.word PINMUX7
PINMUX1_VAL:
	.word 0x11111111
PINMUX2_VAL:
	.word 0x01111111
PINMUX5_VAL:
	.word 0x11111110
PINMUX6_VAL:
	.word 0x11111111
PINMUX7_FLAG_CLEAR:
	.word 0xFFFFF000
PINMUX7_VAL:
	.word 0x111
	

MDCTL_EMIF3:
	.word	PSC1_MDCTL + 4 * 6
MDSTAT_EMIF3:
	.word   PSC1_MDSTAT + 4 * 6

PSC1_PTCMD_ADDR:
	.word	PSC1_PTCMD
PSC1_PTSTAT_ADDR:
	.word	PSC1_PTSTAT

INTC_GLB_EN_ADDR:
    .word	INTC_GLB_EN  
INTC_EN_CLR0_ADDR:
	.word	INTC_EN_CLR0
INTC_HINT_EN_ADDR:
	.word	INTC_HINT_EN		

PSC_FLAG_CLEAR:
	.word	0xffffffe0
PSC_GEM_FLAG_CLEAR:
	.word	0xfffffeff

/* DDR2 MMR & CONFIGURATION VALUES, 162 MHZ clock */
SDREF:
	.word	DAVINCI_DDR_EMIF_CTRL_BASE + 0xc
SDREF_VAL:
	.word	0x000005c3
SDCFG:
	.word	DAVINCI_DDR_EMIF_CTRL_BASE + 0x8
SDCFG_VAL:
#ifdef	SDRAM_4BANKS_10COLS
	.word	0x00178622
#elif defined SDRAM_8BANKS_10COLS
	.word	0x00178632
#else
#error "Unknown SDRAM configuration!!!"
#endif
SDTIM0:
	.word	DAVINCI_DDR_EMIF_CTRL_BASE + 0x10
SDTIM0_VAL_162MHz:
	.word	0x28923211
SDTIM1:
	.word	DAVINCI_DDR_EMIF_CTRL_BASE + 0x14
SDTIM1_VAL_162MHz:
	.word	0x0016c722
CFGTEST:
	.word	DAVINCI_DDR_EMIF_DATA_BASE + 0x10000
MASK_VAL:
	.word	0x00000fff
EMIF3_TIMING_WAIT_VAL:
	.word 990000

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
	.word	2000

/* PLL0 MMRs */
PLL0_PLLCTL_ADDR:
	.word	PLL0_PLLCTL
PLL0_PLLM_ADDR:
	.word	PLL0_PLLM
PLL0_POSTDIV_ADDR:
	.word	PLL0_POSTDIV

DDR2_START_ADDR:
	.word	0xc0000000
DUMMY_VAL:
	.word	0xa55aa55a
