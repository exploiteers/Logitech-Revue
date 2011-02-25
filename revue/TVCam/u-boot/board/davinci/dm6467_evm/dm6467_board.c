/*
 *
 * Copyright (C) 2008 Texas Instruments.
 *
 * ----------------------------------------------------------------------------
 *
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
 Modifications:
 ver. 1.0: Mar 2007, Suresh Rajashekara (Based on the file davinci.c by
 *                                       Swaminathan S)
 *	 : Nov 2007, Kaustubh Sarwate (Updated with PCI based changes)
 *       : August 2008, Sandeep Paulraj<s-paulraj@ti.com> 
 *                      ( for adding support to U-Boot 1.3.4 )
 */

#include <common.h>
#include <i2c.h>
#include <asm/arch/hardware.h>

void davinci_hd_psc_enable(void);
void enable_tcm_cp15(void);

extern void	timer_init(void);
extern int	eth_hw_init(void);

/*
 *Routine: delay
 *Description:  Delay function
 */
static inline void delay (unsigned long loops)
{
    __asm__ volatile ("1:\n"
		      "subs %0, %1, #1\n"
		      "bne 1b":"=r" (loops):"0" (loops));
}

/*
 *Routine: board_init
 *Description:  Board Initialization routine
 */
int board_init (void)
{
	DECLARE_GLOBAL_DATA_PTR;

	/* Arch Number */
	gd->bd->bi_arch_number = 1380;

	/* Adress of boot parameters */
	gd->bd->bi_boot_params = LINUX_BOOT_PARAM_ADDR;

	/* Power on required peripherals */
	davinci_hd_psc_enable();

	/* Timer initialization */
	timer_init();

	return 0;
}

/*
 * Routine: davinci_hd_psc_enable
 * Description:  Enable PSC domains
 */
void davinci_hd_psc_enable ( void )
{
	unsigned int alwaysonpdnum = 0;

	/* Note this function assumes that the Power Domains are 
	 * alread on
	 */
	REG(PSC_ADDR+0xA00+4*14) |= 0x03; /* EMAC */
	REG(PSC_ADDR+0xA00+4*15) |= 0x03; /* VDCE */
	REG(PSC_ADDR+0xA00+4*16) |= 0x03; /* Video Port */
	REG(PSC_ADDR+0xA00+4*17) |= 0x03; /* Video Port */
	REG(PSC_ADDR+0xA00+4*20) |= 0x03; /* DDR2 */
	REG(PSC_ADDR+0xA00+4*21) |= 0x03; /* EMIFA */ 
	REG(PSC_ADDR+0xA00+4*26) |= 0x03; /* UART0 */
	REG(PSC_ADDR+0xA00+4*27) |= 0x03; /* UART1 */
	REG(PSC_ADDR+0xA00+4*28) |= 0x03; /* UART2 */
	REG(PSC_ADDR+0xA00+4*31) |= 0x03; /* UART3 */
	REG(PSC_ADDR+0xA00+4*34) |= 0x03; /* TIMER0 */
	REG(PSC_ADDR+0xA00+4*35) |= 0x03; /* TIMER1 */

	/* Set PTCMD.GO to 0x1 to initiate the state transtion for Modules in
	 * the ALWAYSON Power Domain
	 */
	REG(PSC_PTCMD) = (1<<alwaysonpdnum);

	/* Wait for PTSTAT.GOSTAT0 to clear to 0x0 */
	while(! (((REG(PSC_PTSTAT) >> alwaysonpdnum) & 0x00000001) == 0));

	/* Enable GIO3.3V cells used for EMAC (???) */
	REG(VDD3P3V_PWDN) = 0x80000c0;

	/* Select UART function on UART0 */
	REG(PINMUX0) &= ~(0x0000003f << 18);
	REG(PINMUX1) &= ~(0x00000003);

#ifndef CFG_PCI_BOOT
	/* Enable AEMIF pins */
	REG(PINMUX0) &= ~(0x00000007);
#endif	/* CFG_PCI_BOOT */
    
	/* Enable USB */
	REG(PINMUX0) &= ~(0x80000000);
	
	/* Set the Bus Priority Register to appropriate value */
	REG(VBPR) = 0x20;
}

void enable_tcm_cp15(void)
{
	/* Set to SUPERVISOR  MODE */
	asm (" mrs R0, cpsr\n"
		" bic r0, r0, #0x1F\n"
		" orr r0, r0, #0x13\n"
		" msr cpsr, r0");

	/* Read ITCM */
	asm (" mrc p15, 0, R3, c9, c1, 1\n"
		" nop\n"
		" nop");

	/* Enable ITCM */
	asm(" mov R0, #0x1\n"
		" mcr p15, 0, R0, c9, c1, 1\n"
		" nop\n"
		" nop");

	/* Read Back the ITCM value to check the ITCM Enable function */
	asm(" mrc p15, 0, R4, c9, c1, 1\n"
		" nop\n"
		" nop");

	/* Read DTCM */
	asm(" mrc p15, 0, R3, c9, c1, 0\n"
		" nop\n"
		" nop");

	/* Create DTCM enable mask */
	asm(" ldr R0, =0x10\n"
		" mov R0, R0, lsl #12\n"
		" nop\n"
		" orr R0, R0, #0x1\n"
		" orr R0, R0, R3");

	/* Enable DTCM */
	asm(" mcr p15, 0, R0, c9, c1, 0\n"
		" nop\n"
		" nop");

	/* Read Back the DTCM value to check the DTCM Enable function */
	asm(" mrc p15, 0, R5, c9, c1, 0\n"
		" nop\n"
		" nop");

}

/*
 *Routine: misc_init_r
 *Description:  Misc. init
 */
int misc_init_r (void)
{
	int clk = 0;
	int i = 0;
	u_int8_t tmp[20], buf[10];

	clk = ((REG(PLL2_PLLM) + 1) * 27) / (REG_CHAR(PLL2_DIV1) + 1);
	printf ("ARM Clock :- %dMHz\n", ((((REG(PLL1_PLLM) + 1) * 27 ) / 2)) );
	printf ("DDR Clock :- %dMHz\n", (clk/2));

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

	if (!eth_hw_init())
		printf("ethernet init failed!\n");

	/* enable the ITCM and DTCM */
	enable_tcm_cp15();

#ifdef CFG_PCI_BOOT
	/* Use the following command to run a boot script at run time */
	setenv ("bootcmd", "autoscr 0x82080000");
#endif	/* CFG_PCI_BOOT */

	return (0);
}

/*
 *Routine: dram_init
 *Description:  Memory Info
 */
int dram_init (void)
{
	DECLARE_GLOBAL_DATA_PTR;

	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
	gd->bd->bi_dram[0].size = PHYS_SDRAM_1_SIZE;

	return 0;
}

#ifdef CFG_PCI_BOOT
char * env_name_spec = "nowhere";
int saveenv (void)
{
	return 0;
}
#endif	/* CFG_PCI_BOOT */

