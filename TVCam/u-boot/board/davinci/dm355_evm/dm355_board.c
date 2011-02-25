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
 *
 */
#include <common.h>
#include <i2c.h>
#include <asm/arch/hardware.h>

extern int timer_init(void);

/*******************************************
 Routine: board_init
 Description:  Board Initialization routine
*******************************************/
int board_init (void)
{
	DECLARE_GLOBAL_DATA_PTR;
	
	/* arch number of DaVinci DM355 */
	gd->bd->bi_arch_number = 1381;

	/* adress of boot parameters */
	gd->bd->bi_boot_params = LINUX_BOOT_PARAM_ADDR;
        
	/* Set the Bus Priority Register to appropriate value */
	REG(VBPR) = 0x20;
	
	timer_init();
	return 0;
}


/******************************
 Routine: misc_init_r
 Description:  Misc. init
******************************/
int misc_init_r (void)
{
	char temp[20], *env=0;
	int i = 0;
	int clk = 0;

	clk = ((REG(PLL2_PLLM) + 1) * 24) / ((REG(PLL2_PREDIV) & 0x1F) + 1); 

	printf ("ARM Clock :- %dMHz\n", ( ( ((REG(PLL1_PLLM) + 1)*24 )/(2*(7 + 1)*((REG(SYSTEM_MISC) & 0x2)?2:1 )))) );
	printf ("DDR Clock :- %dMHz\n", (clk/2));

	if ( !(env=getenv("videostd")) || !strcmp(env,"ntsc") || !strcmp(env, "pal") )
	{
		i2c_write (0x25, 0x04, 1, (u_int8_t *)&i, 1);
		i2c_read (0x25, 0x04, 1, (u_int8_t *)&i, 1);
		setenv ("videostd", ((i  & 0x10)?"ntsc":"pal"));
	}

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


