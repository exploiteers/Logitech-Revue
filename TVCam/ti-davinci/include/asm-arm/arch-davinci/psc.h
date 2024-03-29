/*
 *  DaVinci Power & Sleep Controller (PSC) defines
 *
 *  Copyright (C) 2006 Texas Instruments.
 *  Copyright (C) 2008 MontaVista Software, Inc. <source@mvista.com>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 *  THIS  SOFTWARE  IS PROVIDED   ``AS  IS'' AND   ANY  EXPRESS OR IMPLIED
 *  WARRANTIES,   INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 *  NO  EVENT  SHALL   THE AUTHOR  BE    LIABLE FOR ANY   DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED   TO, PROCUREMENT OF  SUBSTITUTE GOODS  OR SERVICES; LOSS OF
 *  USE, DATA,  OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  You should have received a copy of the  GNU General Public License along
 *  with this program; if not, write  to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
#ifndef __ASM_ARCH_PSC_H
#define __ASM_ARCH_PSC_H

/* PSC register offsets */
#define EPCPR		0x070
#define PTCMD		0x120
#define PTSTAT		0x128
#define PDSTAT		0x200
#define PDCTL1		0x304
#define MDSTAT(n)	(0x800 + (n) * 4)
#define MDCTL(n)	(0xA00 + (n) * 4)

/* Power and Sleep Controller (PSC) Domains */
#define DAVINCI_GPSC_ARMDOMAIN		0
#define DAVINCI_GPSC_DSPDOMAIN		1

/*
 * LPSC Assignments
 */
#define DAVINCI_LPSC_VPSSMSTR		0
#define DAVINCI_LPSC_VPSSSLV		1
#define DAVINCI_LPSC_TPCC		2
#define DAVINCI_LPSC_TPTC0		3
#define DAVINCI_LPSC_TPTC1		4
#define DAVINCI_LPSC_EMAC		5
#define DAVINCI_LPSC_EMAC_WRAPPER	6
#define DAVINCI_LPSC_MDIO		7	/* DM644x only */
#define DAVINCI_LPSC_MMC_SD1		7	/* DM355  only */
#define DAVINCI_LPSC_IEEE1394		8	/* DM644x only */
#define DAVINCI_LPSC_McBSP1		8	/* DM355  only */
#define DAVINCI_LPSC_USB		9
#define DAVINCI_LPSC_ATA		10	/* DM644x only */
#define DAVINCI_LPSC_PWM3		10	/* DM355  only */
#define DAVINCI_LPSC_VLYNQ		11
#define DAVINCI_LPSC_UHPI		12
#define DAVINCI_LPSC_DDR_EMIF		13
#define DAVINCI_LPSC_AEMIF		14
#define DAVINCI_LPSC_MMC_SD		15	/* DM644x only */
#define DAVINCI_LPSC_MMC_SD0		15	/* DM355  only */
#define DAVINCI_LPSC_MEMSTICK		16
#define DAVINCI_LPSC_McBSP		17	/* DM644x only */
#define DAVINCI_LPSC_McBSP0		17	/* DM355  only */
#define DAVINCI_LPSC_I2C		18
#define DAVINCI_LPSC_UART0		19
#define DAVINCI_LPSC_UART1		20
#define DAVINCI_LPSC_UART2		21
#define DAVINCI_LPSC_SPI		22
#define DAVINCI_LPSC_PWM0		23
#define DAVINCI_LPSC_PWM1		24
#define DAVINCI_LPSC_PWM2		25
#define DAVINCI_LPSC_GPIO		26
#define DAVINCI_LPSC_TIMER0		27
#define DAVINCI_LPSC_TIMER1		28
#define DAVINCI_LPSC_TIMER2		29
#define DAVINCI_LPSC_SYSTEM_SUBSYS	30
#define DAVINCI_LPSC_ARM		31
#define DAVINCI_LPSC_SCR2		32
#define DAVINCI_LPSC_SCR3		33
#define DAVINCI_LPSC_SCR4		34
#define DAVINCI_LPSC_CROSSBAR		35
#define DAVINCI_LPSC_CFG27		36
#define DAVINCI_LPSC_CFG3		37
#define DAVINCI_LPSC_CFG5		38
#define DAVINCI_LPSC_GEM		39
#define DAVINCI_LPSC_IMCOP		40

#define DAVINCI_DM646X_LPSC_RESERVED	0	/* Reserved */
#define DAVINCI_DM646X_LPSC_C64X_CPU	1	/* C64x + CPU */
#define DAVINCI_DM646X_LPSC_HDVICP0	2	/* HDVICP0 */
#define DAVINCI_DM646X_LPSC_HDVICP1	3	/* HDVICP1 */
#define DAVINCI_DM646X_LPSC_TPCC	4	/* TPCC LPSC */
#define DAVINCI_DM646X_LPSC_TPTC0	5	/* TPTC0 LPSC */
#define DAVINCI_DM646X_LPSC_TPTC1	6	/* TPTC1 LPSC */
#define DAVINCI_DM646X_LPSC_TPTC2	7	/* TPTC2 LPSC */
#define DAVINCI_DM646X_LPSC_TPTC3	8	/* TPTC3 LPSC */
#define DAVINCI_DM646X_LPSC_PCI 	13	/* PCI LPSC */
#define DAVINCI_DM646X_LPSC_EMAC	14	/* EMAC LPSC */
#define DAVINCI_DM646X_LPSC_VDCE	15	/* VDCE LPSC */
#define DAVINCI_DM646X_LPSC_VPSSMSTR	16	/* VPSS Master LPSC */
#define DAVINCI_DM646X_LPSC_VPSSSLV	17	/* VPSS Slave LPSC */
#define DAVINCI_DM646X_LPSC_TSIF0	18	/* TSIF0 LPSC */
#define DAVINCI_DM646X_LPSC_TSIF1	19	/* TSIF1 LPSC */
#define DAVINCI_DM646X_LPSC_DDR_EMIF	20	/* DDR_EMIF LPSC */
#define DAVINCI_DM646X_LPSC_AEMIF	21	/* AEMIF LPSC */
#define DAVINCI_DM646X_LPSC_McASP0	22	/* McASP0 LPSC */
#define DAVINCI_DM646X_LPSC_McASP1	23	/* McASP1 LPSC */
#define DAVINCI_DM646X_LPSC_CRGEN0	24	/* CRGEN0 LPSC */
#define DAVINCI_DM646X_LPSC_CRGEN1	25	/* CRGEN1 LPSC */
#define DAVINCI_DM646X_LPSC_UART0	26	/* UART0 LPSC */
#define DAVINCI_DM646X_LPSC_UART1	27	/* UART1 LPSC */
#define DAVINCI_DM646X_LPSC_UART2	28	/* UART2 LPSC */
#define DAVINCI_DM646X_LPSC_PWM0	29	/* PWM0 LPSC */
#define DAVINCI_DM646X_LPSC_PWM1	30	/* PWM1 LPSC */
#define DAVINCI_DM646X_LPSC_I2C 	31	/* I2C LPSC */
#define DAVINCI_DM646X_LPSC_SPI 	32	/* SPI LPSC */
#define DAVINCI_DM646X_LPSC_GPIO	33	/* GPIO LPSC */
#define DAVINCI_DM646X_LPSC_TIMER0	34	/* TIMER0 LPSC */
#define DAVINCI_DM646X_LPSC_TIMER1	35	/* TIMER1 LPSC */
#define DAVINCI_DM646X_LPSC_ARM_INTC	45	/* ARM INTC LPSC */

/* DA8xx LPSC0/1 modules */
#define DA8XX_LPSC0_TPCC		0
#define DA8XX_LPSC0_TPTC0		1
#define DA8XX_LPSC0_TPTC1		2
#define DA8XX_LPSC0_EMIF25		3
#define DA8XX_LPSC0_SPI0		4
#define DA8XX_LPSC0_MMC_SD		5
#define DA8XX_LPSC0_AINTC		6
#define DA8XX_LPSC0_ARM_RAM_ROM 	7
#define DA8XX_LPSC0_SECU_MGR		8
#define DA8XX_LPSC0_UART0		9
#define DA8XX_LPSC0_SCR0_SS		10
#define DA8XX_LPSC0_SCR1_SS		11
#define DA8XX_LPSC0_SCR2_SS		12
#define DA8XX_LPSC0_DMAX 		13
#define DA8XX_LPSC0_ARM 		14
#define DA8XX_LPSC0_GEM 		15

#define DA8XX_LPSC1_USB20		1
#define DA8XX_LPSC1_USB11		2
#define DA8XX_LPSC1_GPIO		3
#define DA8XX_LPSC1_UHPI		4
#define DA8XX_LPSC1_CPGMAC		5
#define DA8XX_LPSC1_EMIF3C		6
#define DA8XX_LPSC1_McASP0		7
#define DA8XX_LPSC1_McASP1		8
#define DA8XX_LPSC1_McASP2		9
#define DA8XX_LPSC1_SPI1 		10
#define DA8XX_LPSC1_I2C 		11
#define DA8XX_LPSC1_UART1		12
#define DA8XX_LPSC1_UART2		13
#define DA8XX_LPSC1_LCDC		16
#define DA8XX_LPSC1_PWM 		17
#define DA8XX_LPSC1_ECAP 		20
#define DA8XX_LPSC1_EQEP 		21
#define DA8XX_LPSC1_SCR_P0_SS		24
#define DA8XX_LPSC1_SCR_P1_SS		25
#define DA8XX_LPSC1_CR_P3_SS		26
#define DA8XX_LPSC1_L3_CBA_RAM		31

#define DAVINCI_DM365_LPSC_TPCC		0
#define DAVINCI_DM365_LPSC_TPTC0	1
#define DAVINCI_DM365_LPSC_TPTC1	2
#define DAVINCI_DM365_LPSC_TPTC2	3
#define DAVINCI_DM365_LPSC_TPTC3	4
#define DAVINCI_DM365_LPSC_TIMER3	5
#define DAVINCI_DM365_LPSC_SPI1		6
#define DAVINCI_DM365_LPSC_MMC_SD1	7
#define DAVINCI_DM365_LPSC_McBSP	8
#define DAVINCI_DM365_LPSC_USB		9
#define DAVINCI_DM365_LPSC_PWM3		10
#define DAVINCI_DM365_LPSC_SPI2		11
#define DAVINCI_DM365_LPSC_RTO		12
#define DAVINCI_DM365_LPSC_DDR_EMIF	13
#define DAVINCI_DM365_LPSC_AEMIF	14
#define DAVINCI_DM365_LPSC_MMC_SD	15
#define DAVINCI_DM365_LPSC_MMC_SD0	15
#define DAVINCI_DM365_LPSC_MEMSTICK	16
#define DAVINCI_DM365_LPSC_TIMER4	17
#define DAVINCI_DM365_LPSC_I2C		18
#define DAVINCI_DM365_LPSC_UART0	19
#define DAVINCI_DM365_LPSC_UART1	20
#define DAVINCI_DM365_LPSC_UHPI		21
#define DAVINCI_DM365_LPSC_SPI0		22
#define DAVINCI_DM365_LPSC_PWM0		23
#define DAVINCI_DM365_LPSC_PWM1		24
#define DAVINCI_DM365_LPSC_PWM2		25
#define DAVINCI_DM365_LPSC_GPIO		26
#define DAVINCI_DM365_LPSC_TIMER0	27
#define DAVINCI_DM365_LPSC_TIMER1	28
#define DAVINCI_DM365_LPSC_TIMER2	29
#define DAVINCI_DM365_LPSC_SYSTEM_SUBSYS 30
#define DAVINCI_DM365_LPSC_ARM		31
#define DAVINCI_DM365_LPSC_SCR0		33
#define DAVINCI_DM365_LPSC_SCR1		34
#define DAVINCI_DM365_LPSC_EMU		35
#define DAVINCI_DM365_LPSC_CHIPDFT	36
#define DAVINCI_DM365_LPSC_PBIST	37
#define DAVINCI_DM365_LPSC_SPI3		38
#define DAVINCI_DM365_LPSC_SPI4		39
#define DAVINCI_DM365_LPSC_CPGMAC	40
#define DAVINCI_DM365_LPSC_RTC		41
#define DAVINCI_DM365_LPSC_KEYSCAN	42
#define DAVINCI_DM365_LPSC_ADCIF	43
#define DAVINCI_DM365_LPSC_VOICE_CODEC	44
#define DAVINCI_DM365_LPSC_DAC_CLKRES	45
#define DAVINCI_DM365_LPSC_DAC_CLK	46
#define DAVINCI_DM365_LPSC_VPSSMSTR	47
#define DAVINCI_DM365_LPSC_IMCOP	50
#define DAVINCI_DM365_LPSC_KALEIDO	51

#endif /* __ASM_ARCH_PSC_H */
