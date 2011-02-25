/*
 * Copyright (C) 2008 Sekhar Nori, Texas Instruments, Inc
 *
 * Based on hardware.h for DaVinci. Original Copyrights follow.
 *
 * Sergey Kubushyn <ksi@koi8.net>
 * Copyright (C) 2007 Sergey Kubushyn <ksi@koi8.net>
 *
 * Based on:
 *
 * -------------------------------------------------------------------------
 *
 *  linux/include/asm-arm/arch-davinci/hardware.h
 *
 *  Copyright (C) 2006 Texas Instruments.
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
#ifndef __ASM_ARCH_HARDWARE_H
#define __ASM_ARCH_HARDWARE_H

#include <config.h>

#ifndef __ASSEMBLY__

#include <asm/sizes.h>

#define	REG(addr)	(*(volatile unsigned int *)(addr))
#define REG_P(addr)	((volatile unsigned int *)(addr))

typedef volatile unsigned int	dv_reg;
typedef volatile unsigned int *	dv_reg_p;

#endif

/*
 * Base register addresses
 */
#define DAVINCI_UART0_BASE			(0x01c42000)
#define DAVINCI_UART1_BASE			(0x01d0c000)
#define DAVINCI_UART2_BASE			(0x01d0d000)
#define DAVINCI_I2C0_BASE			(0x01c22000)
#define DAVINCI_I2C1_BASE			(0x01e28000)
#define DAVINCI_TIMER0_BASE			(0x01c20000)
#define DAVINCI_TIMER1_BASE			(0x01c21000)
#define DAVINCI_WDOG_BASE			(0x01c21000)
#define DAVINCI_PLL_CNTRL0_BASE		(0x01c11000)
#define DAVINCI_PSC0_BASE 			(0x01c10000)
#define DAVINCI_PSC1_BASE 			(0x01e27000)
#define DAVINCI_SPI0_BASE			(0x01c41000)
#define DAVINCI_SPI1_BASE			(0x01e12000)
#define DAVINCI_GPIO_BASE			(0x01e26000)
#define DAVINCI_EMAC_CNTRL_REGS_BASE			(0x01e23000)
#define DAVINCI_EMAC_WRAPPER_CNTRL_REGS_BASE	(0x01e22000)
#define DAVINCI_EMAC_WRAPPER_RAM_BASE			(0x01e20000)
#define DAVINCI_MDIO_CNTRL_REGS_BASE			(0x01e24000)
#define DAVINCI_ASYNC_EMIF_CNTRL_BASE		(0x68000000)
#define DAVINCI_ASYNC_EMIF_DATA_CE0_BASE	(0x40000000)
#define DAVINCI_ASYNC_EMIF_DATA_CE2_BASE	(0x60000000)
#define DAVINCI_ASYNC_EMIF_DATA_CE3_BASE	(0x62000000)
#define DAVINCI_ASYNC_EMIF_DATA_CE4_BASE	(0x64000000)
#define DAVINCI_ASYNC_EMIF_DATA_CE5_BASE	(0x66000000)
#define DAVINCI_DDR_EMIF_CTRL_BASE	(0xb0000000)
#define DAVINCI_DDR_EMIF_DATA_BASE	(0xc0000000)
#define DAVINCI_INTC_BASE			(0xfffee000)
#define DAVINCI_BOOTCFG_BASE		(0x01c14000)

/* Clock IDs */
#define DAVINCI_PLLM_CLKID			(0xFF + 0)
#define DAVINCI_PLLC_CLKID			(0xFF + 1)
#define DAVINCI_AUXCLK_CLKID		(0xFF + 2)
#define DAVINCI_MDIO_CLKID			4
#define DAVINCI_SPI0_CLKID			2
#define DAVINCI_UART2_CLKID         2
#define DAVINCI_ARM_CLKID         6

/* Power and Sleep Controller (PSC) Domains */
#define DAVINCI_GPSC_ARMDOMAIN		0
#define DAVINCI_GPSC_DSPDOMAIN		1

/* LPSCs in PSC0 */
#define DAVINCI_LPSC_TPCC		0
#define DAVINCI_LPSC_TPTC0		1
#define DAVINCI_LPSC_TPTC1		2
#define DAVINCI_LPSC_AEMIF		3
#define DAVINCI_LPSC_SPI0		4
#define DAVINCI_LPSC_MMC_SD		5
#define DAVINCI_LPSC_AINTC		6
#define DAVINCI_LPSC_ARM_RAM_ROM	7
#define DAVINCI_LPSC_SECCTL_KEYMGR	8
#define DAVINCI_LPSC_UART0		9
#define DAVINCI_LPSC_SCR0		10
#define DAVINCI_LPSC_SCR1		11
#define DAVINCI_LPSC_SCR2		12
#define DAVINCI_LPSC_DMAX		13
#define DAVINCI_LPSC_ARM		14
#define DAVINCI_LPSC_GEM		15

/* for LPSCs in PSC1, 32 + actual id is being used for differentiation */
#define DAVINCI_LPSC_USB11	    (32 + 1)
#define DAVINCI_LPSC_USB20      (32 + 2)
#define DAVINCI_LPSC_GPIO		(32 + 3)
#define DAVINCI_LPSC_UHPI		(32 + 4)
#define DAVINCI_LPSC_EMAC       (32 + 5)
#define DAVINCI_LPSC_DDR_EMIF	(32 + 6)
#define DAVINCI_LPSC_McASP0     (32 + 7)
#define DAVINCI_LPSC_McASP1     (32 + 8)
#define DAVINCI_LPSC_McASP2     (32 + 9)
#define DAVINCI_LPSC_SPI1   	(32 + 10)
#define DAVINCI_LPSC_I2C1   	(32 + 11)
#define DAVINCI_LPSC_UART1   	(32 + 12)
#define DAVINCI_LPSC_UART2   	(32 + 13)
#define DAVINCI_LPSC_LCDC   	(32 + 16)
#define DAVINCI_LPSC_ePWM   	(32 + 17)
#define DAVINCI_LPSC_eCAP   	(32 + 20)
#define DAVINCI_LPSC_eQEP   	(32 + 21)
#define DAVINCI_LPSC_SCR_P0   	(32 + 22)
#define DAVINCI_LPSC_SCR_P1   	(32 + 23)
#define DAVINCI_LPSC_CR_P3   	(32 + 26)
#define DAVINCI_LPSC_L3_CBA_RAM   (32 + 31)

/* Some PSC defines */

#define PSC0_MDCTL			(DAVINCI_PSC0_BASE + 0xa00)
#define PSC0_MDSTAT			(DAVINCI_PSC0_BASE + 0x800)
#define PSC0_PTCMD			(DAVINCI_PSC0_BASE + 0x120)
#define PSC0_PTSTAT			(DAVINCI_PSC0_BASE + 0x128)

#define PSC1_MDCTL			(DAVINCI_PSC1_BASE + 0xa00)
#define PSC1_MDSTAT			(DAVINCI_PSC1_BASE + 0x800)
#define PSC1_PTCMD			(DAVINCI_PSC1_BASE + 0x120)
#define PSC1_PTSTAT			(DAVINCI_PSC1_BASE + 0x128)

/* Some PLL defines */
#define PLL0_PLLCTL			(DAVINCI_PLL_CNTRL0_BASE + 0x100)
#define PLL0_PLLM			(DAVINCI_PLL_CNTRL0_BASE + 0x110)
#define PLL0_PREDIV			(DAVINCI_PLL_CNTRL0_BASE + 0x114)
#define PLL0_POSTDIV		(DAVINCI_PLL_CNTRL0_BASE + 0x128)
#define PLL0_DIV1			(DAVINCI_PLL_CNTRL0_BASE + 0x118)
#define PLL0_DIV2			(DAVINCI_PLL_CNTRL0_BASE + 0x11c)
#define PLL0_DIV3			(DAVINCI_PLL_CNTRL0_BASE + 0x120)
#define PLL0_DIV4			(DAVINCI_PLL_CNTRL0_BASE + 0x160)
#define PLL0_DIV5			(DAVINCI_PLL_CNTRL0_BASE + 0x164)
#define PLL0_DIV6			(DAVINCI_PLL_CNTRL0_BASE + 0x168)
#define PLL0_DIV7			(DAVINCI_PLL_CNTRL0_BASE + 0x16c)
#define PLL0_DIV8			(DAVINCI_PLL_CNTRL0_BASE + 0x170)
#define PLL0_DIV9			(DAVINCI_PLL_CNTRL0_BASE + 0x114)

/* Boot config */
#define KICK0               (DAVINCI_BOOTCFG_BASE + 0x38)
#define KICK1               (DAVINCI_BOOTCFG_BASE + 0x3c)
#define PINMUX0             (DAVINCI_BOOTCFG_BASE + 0x120)
#define PINMUX1             (DAVINCI_BOOTCFG_BASE + 0x124)
#define PINMUX2             (DAVINCI_BOOTCFG_BASE + 0x128)
#define PINMUX3             (DAVINCI_BOOTCFG_BASE + 0x12c)
#define PINMUX4             (DAVINCI_BOOTCFG_BASE + 0x130)
#define PINMUX5             (DAVINCI_BOOTCFG_BASE + 0x134)
#define PINMUX6             (DAVINCI_BOOTCFG_BASE + 0x138)
#define PINMUX7             (DAVINCI_BOOTCFG_BASE + 0x13c)
#define PINMUX8             (DAVINCI_BOOTCFG_BASE + 0x140)
#define PINMUX9             (DAVINCI_BOOTCFG_BASE + 0x144)
#define PINMUX10            (DAVINCI_BOOTCFG_BASE + 0x148)
#define PINMUX11            (DAVINCI_BOOTCFG_BASE + 0x14c)
#define PINMUX12            (DAVINCI_BOOTCFG_BASE + 0x150)
#define PINMUX13            (DAVINCI_BOOTCFG_BASE + 0x154)
#define PINMUX14            (DAVINCI_BOOTCFG_BASE + 0x158)
#define PINMUX15            (DAVINCI_BOOTCFG_BASE + 0x15C)
#define PINMUX16            (DAVINCI_BOOTCFG_BASE + 0x160)
#define PINMUX17            (DAVINCI_BOOTCFG_BASE + 0x164)
#define PINMUX18            (DAVINCI_BOOTCFG_BASE + 0x168)
#define PINMUX19            (DAVINCI_BOOTCFG_BASE + 0x16c)
#define SUSPSRC             (DAVINCI_BOOTCFG_BASE + 0x170)
#define CFGCHIP0            (DAVINCI_BOOTCFG_BASE + 0x17c)

/* Interrupt controller */
#define INTC_GLB_EN			(DAVINCI_INTC_BASE + 0x10)
#define INTC_HINT_EN		(DAVINCI_INTC_BASE + 0x1500)
#define INTC_EN_CLR0		(DAVINCI_INTC_BASE + 0x380)

#endif /* __ASM_ARCH_HARDWARE_H */
