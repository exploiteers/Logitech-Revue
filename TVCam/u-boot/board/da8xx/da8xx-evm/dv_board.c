/*
 * Copyright (C) 2008 Sekhar Nori, Texas Instruments, Inc.  <nsekhar@ti.com>
 * 
 * Modified for DA8xx EVM. 
 *
 * Copyright (C) 2007 Sergey Kubushyn <ksi@koi8.net>
 *
 * Parts are shamelessly stolen from various TI sources, original copyright
 * follows:
 * -----------------------------------------------------------------
 *
 * Copyright (C) 2004 Texas Instruments.
 *
 * ----------------------------------------------------------------------------
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * ----------------------------------------------------------------------------
 */

#include <common.h>
#include <i2c.h>
#include <asm/arch/hardware.h>
#include <asm/arch/emac_defs.h>

#define MACH_TYPE_DA8XX_EVM		1781

DECLARE_GLOBAL_DATA_PTR;

extern void	timer_init(void);
extern int	eth_hw_init(void);

/* Works on Always On power domain only (no PD argument) */
void lpsc_on(unsigned int id)
{
	dv_reg_p	mdstat, mdctl, ptstat, ptcmd;

	if (id >= 64)
		return;	

    if(id < 32) {
	mdstat = REG_P(PSC0_MDSTAT + (id * 4));
        mdctl = REG_P(PSC0_MDCTL + (id * 4));
        ptstat = REG_P(PSC0_PTSTAT);
        ptcmd = REG_P(PSC0_PTCMD);
    } else {
        id -= 32;
	    mdstat = REG_P(PSC1_MDSTAT + (id * 4));
        mdctl = REG_P(PSC1_MDCTL + (id * 4));
        ptstat = REG_P(PSC1_PTSTAT);
        ptcmd = REG_P(PSC1_PTCMD);
    }
    
	while (*ptstat & 0x01) {;}

	if ((*mdstat & 0x1f) == 0x03)
		return;			/* Already on and enabled */

	*mdctl |= 0x03;

	/* Special treatment for some modules as for sprue14 p.7.4.2 */
    /* TBD: Confirm if such cases exist for Primus */
	if (0)
	   	*mdctl |= 0x200;

	*ptcmd = 0x01;

	while (*ptstat & 0x01) {;}
	while ((*mdstat & 0x1f) != 0x03) {;}	/* Probably an overkill... */
}

int board_init(void)
{

	dv_reg_p intc;		

	/*-------------------------------------------------------*
	 * Mask all IRQs by clearing the global enable and setting
     * the enable clear for all the 90 interrupts. This code is
	 * also included in low level init. Including it here in case
	 * low level init is skipped. Not removing it from low level
	 * init in case some of the low level init code generates 
	 * interrupts... Not expected... but you never know...
	 *-------------------------------------------------------*/
		
#ifndef CONFIG_USE_IRQ
	intc = REG_P(INTC_GLB_EN);
	intc[0] = 0;	

	intc = REG_P(INTC_HINT_EN);
	intc[0] = 0;
	intc[1] = 0;
	intc[2] = 0;			

	intc = REG_P(INTC_EN_CLR0);
	intc[0] = 0xFFFFFFFF;
	intc[1] = 0xFFFFFFFF;
	intc[2] = 0xFFFFFFFF;
#endif

	/* arch number of the board */
	gd->bd->bi_arch_number = MACH_TYPE_DA8XX_EVM;

	/* address of boot parameters */
	gd->bd->bi_boot_params = LINUX_BOOT_PARAM_ADDR;

	/* setup the SUSPSRC for ARM to control emulation suspend */
	REG(SUSPSRC) &= ~( (1 << 27) 	/* Timer0 */
			| (1 << 21) 	/* SPI0 */
			| (1 << 20) 	/* UART2 */ 
			| (1 << 5) 	/* EMAC */
			| (1 << 16) 	/* I2C0 */
			);	

	/* Power on required peripherals 
	 * ARM does not have acess by default to PSC0 and PSC1
	 * assuming here that the DSP bootloader has set the IOPU
	 * such that PSC access is available to ARM
	 */
	lpsc_on(DAVINCI_LPSC_AEMIF);    /* NAND, NOR */
	lpsc_on(DAVINCI_LPSC_SPI0);     /* Serial Flash */
	lpsc_on(DAVINCI_LPSC_EMAC);     /* image download */
	lpsc_on(DAVINCI_LPSC_UART2);    /* console */
	lpsc_on(DAVINCI_LPSC_GPIO);

	/* Pin Muxing support */
	
	/* write the kick registers to unlock the PINMUX registers */
	REG(KICK0) = 0x83e70b13;  /* Kick0 unlock */
	REG(KICK1) = 0x95a4f1e0;  /* Kick1 unlock */
	
#ifdef CONFIG_SPI_FLASH
	/* SPI0 */
	REG(PINMUX7) &= 0x00000FFF;
	REG(PINMUX7) |= 0x11111000;
#endif

#ifdef CONFIG_DRIVER_TI_EMAC
	/* RMII clock is sourced externally */
	REG(PINMUX9) &= 0xFF0FFFFF; 
	REG(PINMUX10) &= 0x0000000F; 
	REG(PINMUX10) |= 0x22222220; 
	REG(PINMUX11) &= 0xFFFFFF00; 
	REG(PINMUX11) |= 0x00000022; 
#endif

	/* Async EMIF */
#if defined(CFG_USE_NAND) || defined(CFG_USE_NOR)
	REG(PINMUX13) &= 0x00FFFFFF;
	REG(PINMUX13) |= 0x11000000;
	REG(PINMUX14) =  0x11111111;
	REG(PINMUX15) =  0x11111111;
	REG(PINMUX16) =  0x11111111;
	REG(PINMUX17) =  0x11111111;
	REG(PINMUX18) =  0x11111111;
	REG(PINMUX19) &= 0xFFFFFFF0;
	REG(PINMUX19) |= 0x1;
#endif

	/* UART Muxing and enabling */
	REG(PINMUX8) &= 0x0FFFFFFF; 
	REG(PINMUX8) |= 0x20000000;
    
	REG(PINMUX9) &= 0xFFFFFFF0; 
	REG(PINMUX9) |= 0x00000002;

	REG(DAVINCI_UART2_BASE + 0x30) = 0xE001;

	/* I2C muxing */
	REG(PINMUX8) &= 0xFFF00FFF;
	REG(PINMUX8) |= 0x00022000;

	/* write the kick registers to lock the PINMUX registers */
	REG(KICK0) = 0x0;  /* Kick0 lock */
	REG(KICK1) = 0x0;  /* Kick1 lock */
     
	return(0);
}

int misc_init_r (void)
{
	u_int8_t	tmp[20], buf[10];
	int i;

	printf ("ARM Clock : %d Hz\n", clk_get(DAVINCI_ARM_CLKID));

	/* Set Ethernet MAC address from EEPROM */
	if (i2c_read(CFG_I2C_EEPROM_ADDR, 0x7f00, CFG_I2C_EEPROM_ADDR_LEN, buf, 6)) {
		printf("\nEEPROM @ 0x%02x read FAILED!!!\n", CFG_I2C_EEPROM_ADDR);
	} else {
		tmp[0] = 0xff;
		for (i = 0; i < 6; i++)
			tmp[0] &= buf[i];

		if ((tmp[0] != 0xff) && (getenv("ethaddr") == NULL)) {
			sprintf((char *)&tmp[0], "%02x:%02x:%02x:%02x:%02x:%02x",
				buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
			setenv("ethaddr", (char *)&tmp[0]);
		}
	}

	tmp[0] = 0x01;
	tmp[1] = 0x23;
	if(i2c_write(0x5f, 0, 0, tmp, 2)) {
		printf("Ethernet switch start failed!\n");
	}

	if (!eth_hw_init()) {
		printf("Error: Ethernet init failed!\n");
	}

	return(0);
}

int dram_init(void)
{
	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
	gd->bd->bi_dram[0].size = PHYS_SDRAM_1_SIZE;

	return(0);
}
