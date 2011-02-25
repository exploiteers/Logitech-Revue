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
#include <version.h>

#if defined(CONFIG_OMAP1610)
#include <./configs/omap1510.h> 
#endif

_TEXT_BASE:
	.word	TEXT_BASE	/* sdram load addr from config.mk */

.global reset_cpu
reset_cpu:
	bl reset_processor


.globl lowlevel_init
lowlevel_init:
	/*mov	pc,	lr*/
	
	/*------------------------------------------------------*
	 * mask all IRQs by setting all bits in the EINT default *
	 *------------------------------------------------------*/
	mov	r1, #0x00000000
	ldr	r0, =EINT_ENABLE0
	str	r1, [r0]
	ldr	r0, =EINT_ENABLE1
	str	r1, [r0]

	/*------------------------------------------------------*
	 * Put the GEM in reset		    		  *
 	 *------------------------------------------------------*/

      /* Put the GEM in reset */
  	LDR R8, PSC_GEM_FLAG_CLEAR
	LDR R6, MDCTL_GEM
	LDR R7, [R6]
	AND R7, R7, R8
	STR R7, [R6]

	/* Enable the Power Domain Transition Command */
	LDR R6, PTCMD_0
	LDR R7, [R6]
	ORR R7, R7, #0x2
	STR R7, [R6]

	/* Check for Transition Complete(PTSTAT) */
checkStatClkStopGem:
	LDR R6, PTSTAT_0
	LDR R7, [R6]
    	AND R7, R7, #0x2
    	CMP R7, #0x0
    	BNE checkStatClkStopGem

	/* Check for GEM Reset Completion */
checkGemStatClkStop:
	LDR R6, MDSTAT_GEM
	LDR R7, [R6]
    	AND R7, R7, #0x100
    	CMP R7, #0x0
    	BNE checkGemStatClkStop

	/* Do this for enabling a WDT initiated reset this is a workaround
	   for a chip bug.  Not required under normal situations */
	LDR R6, P1394
	MOV R10, #0x0	
	STR R10, [R6]

	/*------------------------------------------------------*
	 * Enable L1 & L2 Memories in Fast mode                 *
	 *------------------------------------------------------*/
	LDR R6, DFT_ENABLE
	MOV R10, #0x1
	STR R10, [R6]

	LDR R6, MMARG_BRF0
	LDR R10, MMARG_BRF0_VAL
	STR R10, [R6]

	LDR R6, DFT_ENABLE
	MOV R10, #0x0
	STR R10, [R6]
	/*------------------------------------------------------*
	 * DDR2 PLL Intialization			    	*
 	 *------------------------------------------------------*/
	
      /* Select the Clock Mode Depending on the Value written in the Boot Table by the run script */
	MOV R10, #0x0
	LDR R6, PLL2_CTL   
	LDR R7, PLL_CLKSRC_MASK
	LDR R8, [R6]
	AND R8, R8, R7
	MOV R9, R10, LSL #0x8
	ORR R8, R8, R9
	STR R8, [R6]
	
	/* Select the PLLEN source */
	LDR R7, PLL_ENSRC_MASK
	AND R8, R8, R7
	STR R8, [R6]
	
	/* Bypass the PLL */
	LDR R7, PLL_BYPASS_MASK
	AND R8, R8, R7
	STR R8, [R6]
	
	/* Wait for few cycles to allow PLLEN Mux switches properly to bypass Clock */
	MOV R10, #0x20 
WaitPPL2Loop:
    	SUB R10, R10, #0x1
    	CMP R10, #0x0
    	BNE WaitPPL2Loop	

     /* Reset the PLL */
    	LDR R7, PLL_RESET_MASK	
	AND R8, R8, R7
	STR R8, [R6]

     /* Power up the PLL */
    	LDR R7, PLL_PWRUP_MASK	
	AND R8, R8, R7
	STR R8, [R6]

     /* Enable the PLL from Disable Mode */
    	LDR R7, PLL_DISABLE_ENABLE_MASK	
	AND R8, R8, R7
	STR R8, [R6]

	/* Program the PLL Multiplier */
	LDR R6, PLL2_PLLM
	/*MOV R2, #0x13 Orig value */
	/*MOV R2, #0xB 165MHz */
	/*MOV R2, #0xD 189 MHz */
	MOV R2, #0x17 /* 162 MHz */
	STR R2, [R6]   /* R2 */

	/* Program the PLL2 Divisior Value */
	LDR R6, PLL2_DIV2
	MOV R3, #0x1 /* Orig */
	/*MOV R3, #0x0*/
	STR R3, [R6]    /* R3 */

	/* Program the PLL2 Divisior Value */
	LDR R6, PLL2_DIV1
	/*MOV R4, #0x9 Orig */
	/*MOV R4, #0x5 165MHz */
	/*MOV R4, #0x6 189 MHz */
	MOV R4, #0xB /* 54 MHz */
	STR R4, [R6]    /* R4 */

      /* PLL2 DIV1 MMR */
	LDR R8, PLL2_DIV_MASK
	LDR R6, PLL2_DIV2
	LDR R9, [R6]
	AND R8, R8, R9
	MOV R9, #0X1
	MOV R9, R9, LSL #15
	ORR R8, R8, R9
	STR R8, [R6]

	/* Program the GOSET bit to take new divier values */
	LDR R6, PLL2_PLLCMD
	LDR R7, [R6]
	ORR R7, R7, #0x1
	STR R7, [R6]

      /* Wait for Done */
    	LDR R6, PLL2_PLLSTAT
doneLoop_0:    
    	LDR R7, [R6]
    	AND R7, R7, #0x1
    	CMP R7, #0x0
    	BNE doneLoop_0

      /* PLL2 DIV2 MMR */
	LDR R8, PLL2_DIV_MASK
	LDR R6, PLL2_DIV1
	LDR R9, [R6]
	AND R8, R8, R9
	MOV R9, #0X1
	MOV R9, R9, LSL #15
	ORR R8, R8, R9
	STR R8, [R6]

	/* Program the GOSET bit to take new divier values */
	LDR R6, PLL2_PLLCMD
	LDR R7, [R6]
	ORR R7, R7, #0x1
	STR R7, [R6]

     /* Wait for Done */
    	LDR R6, PLL2_PLLSTAT
doneLoop:    
    	LDR R7, [R6]
    	AND R7, R7, #0x1
    	CMP R7, #0x0
    	BNE doneLoop
    				
	/* Wait for PLL to Reset Properly */
	MOV R10, #0x218  
ResetPPL2Loop:
    	SUB R10, R10, #0x1
    	CMP R10, #0x0
    	BNE ResetPPL2Loop
    
    	/* Bring PLL out of Reset */
    	LDR R6, PLL2_CTL    	
    	LDR R8, [R6]
    	ORR R8, R8, #0x08
    	STR R8, [R6]
     	
     /* Wait for PLL to Lock */
	LDR R10, PLL_LOCK_COUNT    
PLL2Lock:
    	SUB R10, R10, #0x1
    	CMP R10, #0x0
    	BNE PLL2Lock
    
     /* Enable the PLL */
    	LDR R6, PLL2_CTL    	
    	LDR R8, [R6]
    	ORR R8, R8, #0x01
    	STR R8, [R6]

	/*------------------------------------------------------*
	 * Issue Soft Reset to DDR Module		    		  *
 	 *------------------------------------------------------*/

      /* Shut down the DDR2 LPSC Module */
  	LDR R8, PSC_FLAG_CLEAR
	LDR R6, MDCTL_DDR2_0
	LDR R7, [R6]
	AND R7, R7, R8
	ORR R7, R7, #0x3
	STR R7, [R6]  

	/* Enable the Power Domain Transition Command */
	LDR R6, PTCMD_0
	LDR R7, [R6]
	ORR R7, R7, #0x1
	STR R7, [R6]    
	
	/* Check for Transition Complete(PTSTAT) */
checkStatClkStop:	
	LDR R6, PTSTAT_0
	LDR R7, [R6]
    	AND R7, R7, #0x1	
    	CMP R7, #0x0
    	BNE checkStatClkStop  
    
	/* Check for DDR2 Controller Enable Completion */
checkDDRStatClkStop:	
	LDR R6, MDSTAT_DDR2_0
	LDR R7, [R6]
    	AND R7, R7, #0x1F	
    	CMP R7, #0x3
    	BNE checkDDRStatClkStop  

	/*------------------------------------------------------*
	 * Program DDR2 MMRs for 162MHz Setting	    		  *
 	 *------------------------------------------------------*/
	
	/* Program PHY Control Register */
	LDR R6, DDRCTL
	LDR R7, DDRCTL_VAL
	STR R7, [R6]

	/* Program SDRAM Bank Config Register */
	LDR R6, SDCFG
	LDR R7, SDCFG_VAL
	STR R7, [R6]

	/* Program SDRAM TIM-0 Config Register */
	LDR R6, SDTIM0
	LDR R7, SDTIM0_VAL_162MHz
	STR R7, [R6]		

	/* Program SDRAM TIM-1 Config Register */
	LDR R6, SDTIM1
	LDR R7, SDTIM1_VAL_162MHz
	STR R7, [R6]		

	/* Program the SDRAM Bang Config Control Register */
	LDR R10, MASK_VAL
	LDR R8, SDCFG
	LDR R9, SDCFG_VAL
	AND R9, R9, R10
	STR R9, [R8]

	/* Program SDRAM TIM-1 Config Register */
	LDR R6, SDREF
	LDR R7, SDREF_VAL
	STR R7, [R6]		

        /*------------------------------------------------------*
         * Issue Soft Reset to DDR Module                                 *
         *------------------------------------------------------*/
                                                                                
        /* Issue a Dummy DDR2 read/write */
        LDR R8, DDR2_VAL
        LDR R7, DUMMY_VAL
        STR R7, [R8]
        LDR R7, [R8]
                                                                                
      /* Shut down the DDR2 LPSC Module */
        LDR R8, PSC_FLAG_CLEAR
        LDR R6, MDCTL_DDR2_0
        LDR R7, [R6]
        AND R7, R7, R8
        ORR R7, R7, #0x1
        STR R7, [R6]
                                                                                
        /* Enable the Power Domain Transition Command */
        LDR R6, PTCMD_0
        LDR R7, [R6]
        ORR R7, R7, #0x1
        STR R7, [R6]
                                                                                
        /* Check for Transition Complete(PTSTAT) */
checkStatClkStop2:
        LDR R6, PTSTAT_0
        LDR R7, [R6]
        AND R7, R7, #0x1
        CMP R7, #0x0
        BNE checkStatClkStop2
                                                                                
        /* Check for DDR2 Controller Enable Completion */
checkDDRStatClkStop2:
        LDR R6, MDSTAT_DDR2_0
        LDR R7, [R6]
        AND R7, R7, #0x1F
        CMP R7, #0x1
        BNE checkDDRStatClkStop2

        /*------------------------------------------------------*
         * Turn DDR2 Controller Clocks On                                 *
         *------------------------------------------------------*/
                                                                                
     /* Enable the DDR2 LPSC Module */
        LDR R6, MDCTL_DDR2_0
        LDR R7, [R6]
        ORR R7, R7, #0x3
        STR R7, [R6]
                                                                                
        /* Enable the Power Domain Transition Command */
        LDR R6, PTCMD_0
        LDR R7, [R6]
        ORR R7, R7, #0x1
        STR R7, [R6]
                                                                                
        /* Check for Transition Complete(PTSTAT) */
checkStatClkEn2:
        LDR R6, PTSTAT_0
        LDR R7, [R6]
        AND R7, R7, #0x1
        CMP R7, #0x0
        BNE checkStatClkEn2
                                                                                
        /* Check for DDR2 Controller Enable Completion */
checkDDRStatClkEn2:
        LDR R6, MDSTAT_DDR2_0
        LDR R7, [R6]
        AND R7, R7, #0x1F
        CMP R7, #0x3
        BNE checkDDRStatClkEn2
                                                                                
        /*  DDR Writes and Reads */
        LDR R6, CFGTEST
        MOV R3, #0x1
        STR R3, [R6]    /* R3 */

	/*------------------------------------------------------*
	 * System PLL Intialization			    		  *
 	 *------------------------------------------------------*/

      /* Select the Clock Mode Depending on the Value written in the Boot Table by the run script */
	MOV R2, #0x0
	LDR R6, PLL1_CTL   
	LDR R7, PLL_CLKSRC_MASK
	LDR R8, [R6]
	AND R8, R8, R7
	MOV R9, R2, LSL #0x8
	ORR R8, R8, R9
	STR R8, [R6]
	
	/* Select the PLLEN source */
	LDR R7, PLL_ENSRC_MASK
	AND R8, R8, R7
	STR R8, [R6]
	
	/* Bypass the PLL */
	LDR R7, PLL_BYPASS_MASK
	AND R8, R8, R7
	STR R8, [R6]
	
	/* Wait for few cycles to allow PLLEN Mux switches properly to bypass Clock */
	MOV R10, #0x20 

WaitLoop:
    	SUB R10, R10, #0x1
    	CMP R10, #0x0
    	BNE WaitLoop	

     /* Reset the PLL */
    	LDR R7, PLL_RESET_MASK	
	AND R8, R8, R7
	STR R8, [R6]

      /* Disable the PLL */
    	ORR R8, R8, #0x10    
    	STR R8, [R6]

      /* Power up the PLL */
    	LDR R7, PLL_PWRUP_MASK	
	AND R8, R8, R7
	STR R8, [R6]

      /* Enable the PLL from Disable Mode */
    	LDR R7, PLL_DISABLE_ENABLE_MASK	
	AND R8, R8, R7
	STR R8, [R6]

	/* Program the PLL Multiplier */
	LDR R6, PLL1_PLLM
	/*MOV R3, #0x10  As per Amit, PLL should be in normal mode i.e X by 16 */
	/*MOV R3, #0x11  As per Ebby 486 MHz */
	/*MOV R3, #0x14  For 567MHz */
	/*MOV R3, #0x15  For 594MHz */
	MOV R3, #0x13 /* For 540MHz in DM357 */
	STR R3, [R6]      

	/* Wait for PLL to Reset Properly */
	MOV R10, #0xFF 

ResetLoop:
    	SUB R10, R10, #0x1
    	CMP R10, #0x0
    	BNE ResetLoop
    
      /* Bring PLL out of Reset */
    	LDR R6, PLL1_CTL    	
    	ORR R8, R8, #0x08
    	STR R8, [R6]
     	
      /* Wait for PLL to Lock */
	LDR R10, PLL_LOCK_COUNT    

PLL1Lock:
    	SUB R10, R10, #0x1
    	CMP R10, #0x0
    	BNE PLL1Lock
    
      /* Enable the PLL */
    	ORR R8, R8, #0x01
    	STR R8, [R6]

	nop
	nop	
	nop
	nop

	/*------------------------------------------------------*
	 * AEMIF configuration for NOR Flash (double check)     *
	 *------------------------------------------------------*/
	LDR R0, _PINMUX0
	LDR R1, _DEV_SETTING
	STR R1, [R0]

 	LDR R0, WAITCFG   
	LDR R1, WAITCFG_VAL 
	LDR R2, [R0]
	ORR R2, R2, R1
	STR R2, [R0]    

	LDR R0, ACFG3
	LDR R1, ACFG3_VAL
	LDR R2, [R0]
	AND R1, R2, R1
	STR R1, [R0]

	LDR R0, ACFG4
	LDR R1, ACFG4_VAL
	LDR R2, [R0]
	AND R1, R2, R1
	STR R1, [R0]

	LDR R0, ACFG5
	LDR R1, ACFG5_VAL
	LDR R2, [R0]
	AND R1, R2, R1
	STR R1, [R0]     

	/*--------------------------------------*
	 * VTP manual Calibration               *
	 *--------------------------------------*/
	LDR R0, VTPIOCR
	LDR R1, VTP_MMR0
	STR R1, [R0]

	LDR R0, VTPIOCR
	LDR R1, VTP_MMR1
	STR R1, [R0]
	
     /* Wait for 33 VTP CLK cycles.  VRP operates at 27 MHz */
	LDR R10, VTP_LOCK_COUNT    
VTPLock:
    	SUB R10, R10, #0x1
    	CMP R10, #0x0
    	BNE VTPLock

	LDR R6, DFT_ENABLE
	MOV R10, #0x1
	STR R10, [R6]

	LDR R6, DDRVTPR
	LDR R7, [R6]
	AND R7, R7, #0x1F
	AND R8, R7, #0x3E0
	ORR R8, R7, R8
	LDR R7, VTP_RECAL
	ORR R8, R7, R8
	LDR R7, VTP_EN
	ORR R8, R7, R8
	STR R8, [R0]


     /* Wait for 33 VTP CLK cycles.  VRP operates at 27 MHz */
	LDR R10, VTP_LOCK_COUNT    
VTP1Lock:
    	SUB R10, R10, #0x1
    	CMP R10, #0x0
    	BNE VTP1Lock

	LDR R1, [R0]
	LDR R2, VTP_MASK
	AND R2, R1, R2
	STR R2, [R0]

	LDR R6, DFT_ENABLE
	MOV R10, #0x0
	STR R10, [R6]


	/* Start MPU Timer 1 */
/*	MOV R10, #0x1AFFFFFF

WaitRam:
    	SUB R10, R10, #0x1
    	CMP R10, #0x0
    	BNE WaitRam
*/

	/* back to arch calling code */
	mov	pc,	lr

	/* the literal pools origin */
	.ltorg

REG_TC_EMIFS_CONFIG:		/* 32 bits */
	.word 0xfffecc0c
REG_TC_EMIFS_CS0_CONFIG:	/* 32 bits */
	.word 0xfffecc10
REG_TC_EMIFS_CS1_CONFIG:	/* 32 bits */
	.word 0xfffecc14
REG_TC_EMIFS_CS2_CONFIG:	/* 32 bits */
	.word 0xfffecc18
REG_TC_EMIFS_CS3_CONFIG:	/* 32 bits */
	.word 0xfffecc1c

_PINMUX0:		.word 0x01C40000		/* Device Configuration Registers */
_PINMUX1:		.word 0x01C40004		/* Device Configuration Registers */

_DEV_SETTING:		.word 0x00000C1F

AEMIF_BASE_ADDR:	.word 0x01E00000
WAITCFG:         	.word 0x01E00004
ACFG2:           	.word 0x01E00010
ACFG3:           	.word 0x01E00014
ACFG4:           	.word 0x01E00018
ACFG5:           	.word 0x01E0001C

WAITCFG_VAL:     	.word 0x0
ACFG2_VAL:       	.word 0x3FFFFFFD
ACFG3_VAL:       	.word 0x3FFFFFFD
ACFG4_VAL:       	.word 0x3FFFFFFD
ACFG5_VAL:       	.word 0x3FFFFFFD

MDCTL_DDR2:		.word 0x01C41A34
PTCMD:		.word 0x01C41120
PTSTAT:		.word 0x01C41128
MDSTAT_DDR2:	.word 0x01C41834

MDCTL_TPCC:		.word	0x01C41A08
MDSTAT_TPCC:	.word 0x01C41808

MDCTL_TPTC0:	.word	0x01C41A0C
MDSTAT_TPTC0:	.word 0x01C4180C

MDCTL_TPTC1:	.word	0x01C41A10
MDSTAT_TPTC1:	.word 0x01C41810

DDR2DEBUG:		.word 0x8FFFF000

/* EINT0 register */
EINT_ENABLE0:
	.word 0x01c48018

/* EINT1 register */
EINT_ENABLE1:
	.word 0x01c4801C

CLEAR_FLAG:				.word 0xFFFFFFFF
EDMA_PARAM0_D_S_BIDX_VAL: 	.word 0x00010001
PSC_FLAG_CLEAR:			.word 0xFFFFFFE0
PSC_GEM_FLAG_CLEAR:			.word 0xFFFFFEFF
MDCTL_TPCC_SYNC:			.word 0x01C41A08
MDSTAT_TPCC_SYNC:			.word 0x01C41808

MDCTL_TPTC0_SYNC:			.word 0x01C41A0C
MDSTAT_TPTC0_SYNC:		.word 0x01C4180C

MDCTL_TPTC1_SYNC:			.word 0x01C41A10
MDSTAT_TPTC1_SYNC:		.word 0x01C41810

PTCMD_SYNC:				.word 0x01C41120
PTSTAT_SYNC:			.word 0x01C41128
DATA_MAX:				.word 0x0000FFFF
SPIN_ADDR:				.word 0x00003FFC   /* ARM PC value(B $) for the DSP Test cases */
SPIN_OPCODE:			.word 0xEAFFFFFE

/* Interrupt Clear Register */
FIQ0_CLEAR:				.word 0x01C48000
FIQ1_CLEAR:				.word 0x01C48004
IRQ0_CLEAR:				.word 0x01C48008
IRQ1_CLEAR:				.word 0x01C4800C

/* DDR2 MMR & CONFIGURATION VALUES for 75 MHZ */
DDRCTL:				.word	0x200000E4
SDREF:				.word	0x2000000C
SDCFG:				.word	0x20000008
SDTIM0:				.word	0x20000010
SDTIM1:				.word	0x20000014
SDSTAT:				.word 0x20000004
VTPIOCR:				.word 0x200000F0  /* VTP IO Control register */
DDRVTPR:				.word 0x01C42030  /* DDR VPTR MMR */
DFT_ENABLE:				.word 0x01C4004C
VTP_MMR0:				.word 0x201F
VTP_MMR1:				.word 0xA01F
PCH_MASK:				.word 0x3E0
VTP_LOCK_COUNT:				.word 0x5b0
VTP_MASK:				.word 0xFFFFDFFF
VTP_RECAL:				.word 0x40000
VTP_EN:					.word 0x02000


CFGTEST:				.word	0x80010000

/* original values					
DDRCTL_VAL:				.word	0x50006405
SDCFG_VAL:				.word	0x00008832
MASK_VAL:                    	.word 0x00000FFF
SDTIM0_VAL_135MHz:		.word	0x30923A91
SDTIM1_VAL_135MHz:		.word	0x0019c722
SDREF_VAL:				.word 0x000005c3
*/

/* 162MHz as per GEL file for DVEVM with Micron DDR2 SDRAM */
DDRCTL_VAL:                     .word   0x50006405
SDCFG_VAL:                      .word   0x00178632           /* CL=3 for MT47H64M16BT-5E */
MASK_VAL:                       .word   0xFFFF7FFF
SDTIM0_VAL_162MHz:              .word   0x28923211
SDTIM1_VAL_162MHz:              .word   0x0016c722
SDREF_VAL:                      .word   0x000004F0

/* GEM Power Up & LPSC Control Register */
CHP_SHRTSW:				.word 0x01C40038

PD1_CTL:				.word	0x01C41304
EPCPR:				.word	0x01C41070
EPCCR:				.word	0x01C41078
MDCTL_GEM:				.word	0x01C41A9C
MDSTAT_GEM:				.word 0x01C4189C
MDCTL_IMCOP:			.word	0x01C41AA0
MDSTAT_IMCOP:			.word 0x01C418A0

PTCMD_0:				.word 0x01C41120
PTSTAT_0:				.word 0x01C41128
P1394:					.word 0x01C41a20

PLL_CLKSRC_MASK: 			.word 0xFFFFFEFF  /* Mask the Clock Mode bit and it is programmble through the run script */
PLL_ENSRC_MASK:  			.word 0xFFFFFFDF  /* Select the PLLEN source */
PLL_BYPASS_MASK: 			.word 0xFFFFFFFE  /* Put the PLL in BYPASS, eventhough the device */
PLL_RESET_MASK:  			.word 0xFFFFFFF7  /* Put the PLL in Reset Mode */
PLL_PWRUP_MASK:  			.word 0xFFFFFFFD  /* PLL Power up Mask Bit  */
PLL_DISABLE_ENABLE_MASK:	.word 0xFFFFFFEF  /* Enable the PLL from Disable */
PLL_LOCK_COUNT:          	.word 0x2000

/* PLL1-SYSTEM PLL MMRs */
PLL1_CTL:				.word 0x01C40900 
PLL1_PLLM:				.word 0x01C40910

/* PLL2-SYSTEM PLL MMRs */
PLL2_CTL:				.word 0x01C40D00 
PLL2_PLLM:				.word 0x01C40D10
PLL2_DIV2:				.word 0x01C40D1C
PLL2_DIV1:				.word 0x01C40D18
PLL2_PLLCMD:			.word 0x01C40D38
PLL2_PLLSTAT:			.word 0x01C40D3C	
PLL2_BPDIV:				.word 0x01C40D2C
PLL2_DIV_MASK:			.word 0xFFFF7FFF


MDCTL_DDR2_0:			.word 0x01C41A34
MDSTAT_DDR2_0:			.word 0x01C41834
DLLPWRUPMASK:			.word 0xFFFFFFEF
DDR2_ADDR:			.word 0x80000000

DFT_BASEADDR:			.word 0x01C42000
MMARG_BRF0:			.word 0x01C42010     /* BRF margin mode 0     (Read / write)*/
MMARG_G10:			.word 0x01C42018     /*GL margin mode 0      (Read / write)*/
MMARG_BRF0_VAL:			.word 0x00444400
DDR2_VAL:			.word 0x80000000
DUMMY_VAL:			.word 0xA55AA55A

/* command values */
.equ CMD_SDRAM_NOP,		0x00000000
.equ CMD_SDRAM_PRECHARGE,	0x00000001
.equ CMD_SDRAM_AUTOREFRESH,	0x00000002
.equ CMD_SDRAM_CKE_SET_HIGH,	0x00000007

