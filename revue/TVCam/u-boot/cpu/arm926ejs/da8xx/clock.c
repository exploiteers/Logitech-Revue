/*
 * Copyright (C) 2008 Sekhar Nori, Texas Instruments, Inc.  <nsekhar@ti.com>
 * 
 * DA8xx clock module
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
 */

#include <common.h>
#include <asm/arch/hardware.h>

dv_reg_p sysdiv[9] = {
    PLL0_DIV1, PLL0_DIV2, PLL0_DIV3, PLL0_DIV4, PLL0_DIV5, PLL0_DIV6, 
    PLL0_DIV7, PLL0_DIV8, PLL0_DIV9 };

int clk_get(unsigned int id)
{
    int pre_div = (REG(PLL0_PREDIV) & 0xff) + 1;
    int pllm = REG(PLL0_PLLM)  + 1;
    int post_div = (REG(PLL0_POSTDIV) & 0xff) + 1;
    int pll_out = CFG_OSCIN_FREQ;

    if(id == DAVINCI_AUXCLK_CLKID) 
        goto out;

    /* Lets keep this simple. Combining operations can result in 
     * unexpected approximations
     */
    pll_out /= pre_div;
    pll_out *= pllm;

    if(id == DAVINCI_PLLM_CLKID) 
        goto out;
    
    pll_out /= post_div;
	
    if(id == DAVINCI_PLLC_CLKID) 
        goto out;
    
   pll_out /= (REG(sysdiv[id - 1]) & 0xff) + 1;

out:
	return pll_out;	
}
