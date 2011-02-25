/*----------------------------------------------------------------------------+
|
|       This source code has been made available to you by IBM on an AS-IS
|       basis.  Anyone receiving this source is licensed under IBM
|       copyrights to use it in any way he or she deems fit, including
|       copying it, modifying it, compiling it, and redistributing it either
|       with or without modifications.  No license under IBM patents or
|       patent applications is to be implied by the copyright license.
|
|       Any user of this software should understand that IBM cannot provide
|       technical support for this software and will not be responsible for
|       any consequences resulting from the use of this software.
|
|       Any person who transfers this source code or any derivative work
|       must include the IBM copyright notice, this paragraph, and the
|       preceding two paragraphs in the transferred software.
|
|       COPYRIGHT   I B M   CORPORATION 1999
|       LICENSED MATERIAL  -  PROGRAM PROPERTY OF I B M
+----------------------------------------------------------------------------*/

#ifndef	__PPC4XX_H__
#define __PPC4XX_H__

/*
 * Configure which SDRAM/DDR/DDR2 controller is equipped
 */
#if defined(CONFIG_405GP) || defined(CONFIG_405CR) || defined(CONFIG_405EP) || \
	defined(CONFIG_AP1000) || defined(CONFIG_ML2)
#define CONFIG_SDRAM_PPC4xx_IBM_SDRAM	/* IBM SDRAM controller */
#endif

#if defined(CONFIG_440GP) || defined(CONFIG_440GX) || \
    defined(CONFIG_440EP) || defined(CONFIG_440GR)
#define CONFIG_SDRAM_PPC4xx_IBM_DDR	/* IBM DDR controller */
#endif

#if defined(CONFIG_440EPX) || defined(CONFIG_440GRX)
#define CONFIG_SDRAM_PPC4xx_DENALI_DDR2	/* Denali DDR(2) controller */
#endif

#if defined(CONFIG_405EX) || \
    defined(CONFIG_440SP) || defined(CONFIG_440SPE) || \
    defined(CONFIG_460EX) || defined(CONFIG_460GT) || \
    defined(CONFIG_460SX)
#define CONFIG_SDRAM_PPC4xx_IBM_DDR2	/* IBM DDR(2) controller */
#endif

#if defined(CONFIG_440)
/*
 * Enable long long (%ll ...) printf format on 440 PPC's since most of
 * them support 36bit physical addressing
 */
#define CFG_64BIT_VSPRINTF
#define CFG_64BIT_STRTOUL
#include <ppc440.h>
#else
#include <ppc405.h>
#endif

#include <asm/ppc4xx-sdram.h>
#include <asm/ppc4xx-ebc.h>
#if !defined(CONFIG_XILINX_440)
#include <asm/ppc4xx-uic.h>
#endif

/*
 * Macro for generating register field mnemonics
 */
#define	PPC_REG_BITS		32
#define	PPC_REG_VAL(bit, value)	((value) << ((PPC_REG_BITS - 1) - (bit)))

/*
 * Elide casts when assembling register mnemonics
 */
#ifndef __ASSEMBLY__
#define	static_cast(type, val)	(type)(val)
#else
#define	static_cast(type, val)	(val)
#endif

/*
 * Common stuff for 4xx (405 and 440)
 */

#define EXC_OFF_SYS_RESET	0x0100	/* System reset				*/
#define _START_OFFSET		(EXC_OFF_SYS_RESET + 0x2000)

#define RESET_VECTOR	0xfffffffc
#define CACHELINE_MASK	(CFG_CACHELINE_SIZE - 1) /* Address mask for cache
						     line aligned data. */

#define CPR0_DCR_BASE	0x0C
#define cprcfga		(CPR0_DCR_BASE+0x0)
#define cprcfgd		(CPR0_DCR_BASE+0x1)

#define SDR_DCR_BASE	0x0E
#define sdrcfga		(SDR_DCR_BASE+0x0)
#define sdrcfgd		(SDR_DCR_BASE+0x1)

#define SDRAM_DCR_BASE	0x10
#define memcfga		(SDRAM_DCR_BASE+0x0)
#define memcfgd		(SDRAM_DCR_BASE+0x1)

#define EBC_DCR_BASE	0x12
#define ebccfga		(EBC_DCR_BASE+0x0)
#define ebccfgd		(EBC_DCR_BASE+0x1)

/*
 * Macros for indirect DCR access
 */
#define mtcpr(reg, d)	do { mtdcr(cprcfga,reg);mtdcr(cprcfgd,d); } while (0)
#define mfcpr(reg, d)	do { mtdcr(cprcfga,reg);d = mfdcr(cprcfgd); } while (0)

#define mtebc(reg, d)	do { mtdcr(ebccfga,reg);mtdcr(ebccfgd,d); } while (0)
#define mfebc(reg, d)	do { mtdcr(ebccfga,reg);d = mfdcr(ebccfgd); } while (0)

#define mtsdram(reg, d)	do { mtdcr(memcfga,reg);mtdcr(memcfgd,d); } while (0)
#define mfsdram(reg, d)	do { mtdcr(memcfga,reg);d = mfdcr(memcfgd); } while (0)

#define mtsdr(reg, d)	do { mtdcr(sdrcfga,reg);mtdcr(sdrcfgd,d); } while (0)
#define mfsdr(reg, d)	do { mtdcr(sdrcfga,reg);d = mfdcr(sdrcfgd); } while (0)

#ifndef __ASSEMBLY__

typedef struct
{
	unsigned long freqDDR;
	unsigned long freqEBC;
	unsigned long freqOPB;
	unsigned long freqPCI;
	unsigned long freqPLB;
	unsigned long freqTmrClk;
	unsigned long freqUART;
	unsigned long freqProcessor;
	unsigned long freqVCOHz;
	unsigned long freqVCOMhz;	/* in MHz                          */
	unsigned long pciClkSync;	/* PCI clock is synchronous        */
	unsigned long pciIntArbEn;	/* Internal PCI arbiter is enabled */
	unsigned long pllExtBusDiv;
	unsigned long pllFbkDiv;
	unsigned long pllFwdDiv;
	unsigned long pllFwdDivA;
	unsigned long pllFwdDivB;
	unsigned long pllOpbDiv;
	unsigned long pllPciDiv;
	unsigned long pllPlbDiv;
} PPC4xx_SYS_INFO;

#endif	/* __ASSEMBLY__ */

#endif	/* __PPC4XX_H__ */
