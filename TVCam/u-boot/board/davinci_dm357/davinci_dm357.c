/*
 *
 * Copyright (C) 2004 Texas Instruments.
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
 * Modifications:
 * ver. 1.0: Oct 2005, Swaminathan S
 * August 2007 - Modification for VICP init  Nageswari S 
 *
 */

#include <common.h>
#include <i2c.h>
#include "timer.h"

#if 0
void flash__init (void);
void ether__init (void);
#endif

#define PLL1_PLLM	    *(volatile unsigned int *)0x01c40910
#define PLL2_PLLM	    *(volatile unsigned int *)0x01c40D10
#define PLL2_DIV2	    *(volatile unsigned char *)0x01c40D1C

/* PSC Domains */
#define LPSC_VPSSMSTR       0       // VPSS Master LPSC
#define LPSC_VPSSSLV        1       // VPSS Slave LPSC
#define LPSC_TPCC           2       // TPCC LPSC
#define LPSC_TPTC0          3       // TPTC0 LPSC
#define LPSC_TPTC1          4       // TPTC1 LPSC
#define LPSC_EMAC           5       // EMAC LPSC
#define LPSC_EMAC_WRAPPER   6       // EMAC WRAPPER LPSC
#define LPSC_MDIO           7       // MDIO LPSC
#define LPSC_IEEE1394       8       // IEEE1394 LPSC
#define LPSC_USB            9       // USB LPSC
#define LPSC_ATA            10      // ATA LPSC
#define LPSC_VLYNQ          11      // VLYNQ LPSC
#define LPSC_UHPI           12      // UHPI LPSC
#define LPSC_DDR_EMIF       13      // DDR_EMIF LPSC
#define LPSC_AEMIF          14      // AEMIF LPSC
#define LPSC_MMC_SD         15      // MMC_SD LPSC
#define LPSC_MEMSTICK       16      // MEMSTICK LPSC
#define LPSC_McBSP          17      // McBSP LPSC
#define LPSC_I2C            18      // I2C LPSC
#define LPSC_UART0          19      // UART0 LPSC
#define LPSC_UART1          20      // UART1 LPSC
#define LPSC_UART2          21      // UART2 LPSC
#define LPSC_SPI            22      // SPI LPSC
#define LPSC_PWM0           23      // PWM0 LPSC
#define LPSC_PWM1           24      // PWM1 LPSC
#define LPSC_PWM2           25      // PWM2 LPSC
#define LPSC_GPIO           26      // GPIO LPSC
#define LPSC_TIMER0         27      // TIMER0 LPSC
#define LPSC_TIMER1         28      // TIMER1 LPSC
#define LPSC_TIMER2         29      // TIMER2 LPSC
#define LPSC_SYSTEM_SUBSYS  30      // SYSTEM SUBSYSTEM LPSC
#define LPSC_ARM            31      // ARM LPSC
#define LPSC_SCR2           32      // SCR2 LPSC
#define LPSC_SCR3           33      // SCR3 LPSC
#define LPSC_SCR4           34      // SCR4 LPSC
#define LPSC_CROSSBAR       35      // CROSSBAR LPSC
#define LPSC_CFG27          36      // CFG27 LPSC
#define LPSC_CFG3           37      // CFG3 LPSC
#define LPSC_CFG5           38      // CFG5 LPSC
#define LPSC_GEM            39      // GEM LPSC
#define LPSC_IMCOP          40      // IMCOP LPSC

#define CHP_SHRTSW          *( volatile unsigned int* )( 0x01C40038 )  
#define GBLCTL              *( volatile unsigned int* )( 0x01C41010 )
#define EPCPR               *( volatile unsigned int* )( 0x01C41070 )
#define EPCCR               *( volatile unsigned int* )( 0x01C41078 )
#define PDSTAT              *( volatile unsigned int* )( 0x01C41200 )
#define PDSTAT1             *( volatile unsigned int* )( 0x01C41204 )
#define PDCTL               *( volatile unsigned int* )( 0x01C41300 )
#define PDCTL1              *( volatile unsigned int* )( 0x01C41304 )
#define VBPR                *( volatile unsigned int* )( 0x20000020 )

#define PSC_ADDR            0x01C41000
#define PTCMD               (PSC_ADDR+0x120)
#define PTSTAT              (PSC_ADDR+0x128)
#define UARTPWREMU_MGMT     0x01c20030
#define VDD3P3V_PWDN        0x01c40048
#define PINMUX0		    0x01C40000
#define PINMUX4		    0x01C40004


void davinci_psc_all_enable(void);

void raise()
{
        printf ("*** something is wrong... please reset ***\n");
}

void abort()
{
        printf ("*** something is wrong... please reset ***\n");
}

/*******************************************
 Routine: delay
 Description:  Delay function
*******************************************/
static inline void delay (unsigned long loops)
{
	__asm__ volatile ("1:\n"
		"subs %0, %1, #1\n"
		"bne 1b":"=r" (loops):"0" (loops));
}

/*******************************************
 Routine: board_init
 Description:  Board Initialization routine
*******************************************/
int board_init (void)
{
	DECLARE_GLOBAL_DATA_PTR;

        /* arch number of DaVinci DVDP-Board */
        gd->bd->bi_arch_number = 901;

      	/* adress of boot parameters */
       	gd->bd->bi_boot_params = LINUX_BOOT_PARAM_ADDR;

	/* Power on required peripherals */
    	davinci_psc_all_enable();
#if 0
      	/**
	 * This speeds up your boot a quite a bit.  However to make it
 	 * work, you need make sure your kernel startup flush bug is fixed.
	 */
	icache_enable ();
#endif
      	inittimer ();

      	return 0;
}


/**************************************
 Routine: board_setup_psc_on
 Description:  Enable a PSC domain
**************************************/
void board_setup_psc_on( unsigned int domain, unsigned int id )
{
	volatile unsigned int* mdstat 
		= ( unsigned int* )( 0x01C41800 + 4 * id );
	volatile unsigned int* mdctl  
		= ( unsigned int* )( 0x01C41A00 + 4 * id );

	// Set PowerDomain to turn on
	*mdctl |= 0x00000003;

	if ( ( PDSTAT & 0x00000001 ) == 0 )
	{
		PDCTL1 |= 0x1;
		*(volatile unsigned int*)PTCMD = ( 1 << domain );
		while((((EPCPR >> domain) & 1) == 0));

		PDCTL1 |= 0x100 ;
		while(!(((*(volatile unsigned int*)PTSTAT >> domain) & 1) == 0));
	}
	else
	{
		*(volatile unsigned int*)PTCMD = ( 1<<domain );
		while(!(((*(volatile unsigned int*)PTSTAT >> domain) & 1) == 0));
	}

	while(!((*mdstat & 0x0000001F) == 0x3));
}

/**************************************
 Routine: davinci_psc_all_enable
 Description:  Enable all PSC domains
**************************************/
void davinci_psc_all_enable(void)
{
        unsigned int alwaysOnPdNum = 0, dspPdNum = 1;
	unsigned int state;

	/**
	 * This function turns on all clocks in the ALWAYSON and DSP Power
	 * Domains. Note this function assumes that the Power Domains are
	 * already on.
	 */

	/**
	 * For special workaround:  Clear MDCTL[i].EMURSTIE to 0x0 for
         * all of the following Modules. VPSSSLV, EMAC, EMACCTRL, MDIO,
         * USB, ATA, VLYNQ, HPI, DDREMIF, AEMIF, MMCSD, MEMSTICK, ASP,
	 * GPIO, IMCOP.
         */
        *(volatile unsigned int*) (PSC_ADDR+0xA00+4*5) 
		= *(unsigned int*) (PSC_ADDR+0xA00+4*5) | 0x003;
        *(volatile unsigned int*) (PSC_ADDR+0xA00+4*6) 
		= *(unsigned int*) (PSC_ADDR+0xA00+4*6) | 0x003;
        *(volatile unsigned int*) (PSC_ADDR+0xA00+4*7) 
		= *(unsigned int*) (PSC_ADDR+0xA00+4*7) | 0x003;
        *(volatile unsigned int*) (PSC_ADDR+0xA00+4*13) 
		= *(unsigned int*) (PSC_ADDR+0xA00+4*13) | 0x003;
        *(volatile unsigned int*) (PSC_ADDR+0xA00+4*14) 
		= *(unsigned int*) (PSC_ADDR+0xA00+4*14) | 0x003;
        *(volatile unsigned int*) (PSC_ADDR+0xA00+4*19) 
		= *(unsigned int*) (PSC_ADDR+0xA00+4*19) | 0x003;
        *(volatile unsigned int*) (PSC_ADDR+0xA00+4*18) 
		= *(unsigned int*) (PSC_ADDR+0xA00+4*18) | 0x003;
        *(volatile unsigned int*) (PSC_ADDR+0xA00+4*28) 
		= *(unsigned int*) (PSC_ADDR+0xA00+4*28) | 0x003;

	/**
	 * Set PTCMD.GO0 to 0x1 to initiate the state transtion for Modules
	 * in the ALWAYSON Power Domain
	 */
	*(volatile unsigned int*) PTCMD = (1<<alwaysOnPdNum);

	/* Wait for PTSTAT.GOSTAT0 to clear to 0x0 */
	while(! (((*(volatile unsigned int*) PTSTAT >> alwaysOnPdNum) 
		& 0x00000001) == 0));

	/**
	 * DO GEM AND IMCOP INITIALIZATION, ONLY IF DSP POWER DOMAIN IS OFF
         * NOTE: this is a precise and refined sequence 
	 * - use extreme care if modifying! 
	 */
        if ((PDSTAT1 & 0x1F) == 0)
        {
                /**
		 * set PSC FORCE mode; may not be necessary
		 */
                GBLCTL = GBLCTL | 0x01;

                /* set DSP power domain next state to ON */
                PDCTL1 = PDCTL1 | 0x01;

                /* ensure external power indicator is cleared */
                PDCTL1 = PDCTL1 & 0xFFFFFEFF;

                // 1st step start
                /* SwRstDisable DSP module */
                *(volatile unsigned int*) (PSC_ADDR+0xA00+4*LPSC_GEM)
                        = (*(volatile unsigned int*)
                                (PSC_ADDR+0xA00+4*LPSC_GEM) & 0xFFFFFFF8);

                /* start power state transitions for DSP power domain */
                *(volatile unsigned int*) PTCMD = (1<<dspPdNum);

                /* wait for external power control pending to assert */
                do{
                        if (((EPCPR >> dspPdNum) & 0x00000001) == 1){
                                break;
                        }
                }while (1);

                /* close rail shorting switch */
                CHP_SHRTSW = 0x1;
		
                /* clear external power control pending register bit */
                EPCCR = (1 << dspPdNum);

                /* set external power good indicator */
                PDCTL1 = PDCTL1 | 0x0100;

                /* wait for DSP domain transitions to complete */
                do {
                        state = *(volatile unsigned int*) PTSTAT;
                        if (((state >> dspPdNum) & 0x00000001) == 0) {
                                break;
                        }
                }while(1);

                // 2nd step start
                /* enable DSP module */
                *(volatile unsigned int*) (PSC_ADDR+0xA00+4*LPSC_GEM) =
                        (*(volatile unsigned int*)(PSC_ADDR+0xA00+4*LPSC_GEM)
			& 0xFFFFFFE0) | 0x3;

                /* hold DSP in reset on next power ON */
                *(volatile unsigned int*) (PSC_ADDR+0xA00+4*LPSC_GEM) =
                        *(volatile unsigned int*)
                                (PSC_ADDR+0xA00+4*LPSC_GEM) & 0xFFFFFEFF;

                /* set IMCOP to enable state */
                *(volatile unsigned int*) (PSC_ADDR+0xA00+4*LPSC_IMCOP) =
                        (*(volatile unsigned int*)(PSC_ADDR+0xA00+4*LPSC_IMCOP)
			& 0xFFFFFFE0) | 0x3;

                /* start power state transitions for DSP power domain */
                *(volatile unsigned int*) PTCMD = (1<<dspPdNum);

                /* wait for DSP domain transitions to complete */
                do {
                        state = *(volatile unsigned int*) PTSTAT;
                        if (((state >> dspPdNum) & 0x00000001) == 0) {
                                break;
                        }
                }while(1);

                /* turn off PSC FORCE mode */
                GBLCTL = GBLCTL & 0xFFFFFFFE;
        } /* END GEM AND IMCOP INITIALIZATION */

        /**
	 * Bringup UART out of reset here since NS16650 code
	 * that we are using from uBoot will not do it
         */
        *(volatile unsigned int*) UARTPWREMU_MGMT = 0x0000E003;
        
	/* Enable GIO3.3V cells used for EMAC */
        *(volatile unsigned int*) VDD3P3V_PWDN = 0;

	/* Enable UART0 MUX lines */
        *(volatile unsigned int *)PINMUX4 |= 1;

	/* Enable EMAC and AEMIF pins */
#if (CONFIG_COMMANDS & CFG_CMD_NAND)
        *(volatile unsigned int*) PINMUX0 = 0x80000000;
#else
        *(volatile unsigned int*) PINMUX0 = 0x80000C1F;
#endif

	/* Enable I2C pin Mux */
        *(volatile unsigned int *)PINMUX4 |= (1 << 7);

	/* Set the Bus Priority Register to appropriate value */
	VBPR = 0x20;
}

/******************************
 Routine: misc_init_r
 Description:  Misc. init
******************************/
int misc_init_r (void)
{
	char temp[20], *env = 0;
	char rtcdata[10] = { 2, 1, 0, 0, 0, 0, 0, 0, 0, 0};
	char emac_read_addr [10] = { 0x7f, 0 }, i= 0;
        int clk = 0;

        clk = ((PLL2_PLLM + 1) * 27) / (PLL2_DIV2 + 1);

        printf ("ARM Clock :- %dMHz\n", ((((PLL1_PLLM + 1) * 27 ) / 2)));
        printf ("DDR Clock :- %dMHz\n", (clk/2));

	i2c_write (0x50, 0x00, 1, emac_read_addr, 2);
	i2c_read (0x50, 0x00, 1, emac_read_addr, 6);
        temp[0] = (emac_read_addr[0] & 0xF0) >> 4;
        temp[1] = (emac_read_addr[0] & 0x0F);
        temp[2] = ':';
        temp[3] = (emac_read_addr[1] & 0xF0) >> 4;
        temp[4] = (emac_read_addr[1] & 0x0F);
        temp[5] = ':';
        temp[6] = (emac_read_addr[2] & 0xF0) >> 4;
        temp[7] = (emac_read_addr[2] & 0x0F);
        temp[8] = ':';
        temp[9] = (emac_read_addr[3] & 0xF0) >> 4;
        temp[10]= (emac_read_addr[3] & 0x0F);
        temp[11]= ':';
        temp[12]= (emac_read_addr[4] & 0xF0) >> 4;
        temp[13]= (emac_read_addr[4] & 0x0F);
        temp[14]= ':';
        temp[15]= (emac_read_addr[5] & 0xF0) >> 4;
        temp[16]= (emac_read_addr[5] & 0x0F);
                                                                               
        for (i = 0; i < 17; i++)
        {
		if (temp[(int)i] == ':')
			continue;
		else if (temp[(int)i] >= 0 && temp[(int)i] <= 9)
			temp[(int)i] = temp[(int)i] + 48;
		else
			temp[(int)i] = temp[(int)i] + 87;
        }
                   
	temp [17] = 0;                                                            
	if ((emac_read_addr [0] != 0xFF) ||
	    (emac_read_addr [1] != 0xFF) ||	
	    (emac_read_addr [2] != 0xFF) ||	
	    (emac_read_addr [3] != 0xFF) ||	
	    (emac_read_addr [4] != 0xFF) ||	
	    (emac_read_addr [5] != 0xFF))
	{ 
		setenv ("ethaddr", temp);
	}

	i2c_read (0x39, 0x00, 1, &i, 1);
	
    	if ( !(env=getenv("videostd")) || !strcmp(env,"ntsc") || !strcmp(env, "pal") )
	    setenv ("videostd", ((i  & 0x80)?"pal":"ntsc"));
	
	i2c_write (0x23, 0x00, 1, rtcdata, 2);
	i2c_read (0x23, 0x00, 1, rtcdata, 1);

	if (rtcdata[0] == 10)
		printf("MSP430 Firmware supports AM/PM Feature\n");
	else
		printf("MSP430 Firmware does not support AM/PM Feature\n");

        return (0);
}

/******************************
 Routine: dram_init
 Description:  Memory Info
******************************/
int dram_init (void)
{
        DECLARE_GLOBAL_DATA_PTR;
	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
	gd->bd->bi_dram[0].size = PHYS_SDRAM_1_SIZE;
        return 0;
}


