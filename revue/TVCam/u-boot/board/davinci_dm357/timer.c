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
 Modifications:
 ver. 1.0: Oct 2005, Swaminathan S
 *
 */

#include "soc.h"
#include "timer.h"

/* Use Timer 3&4 (Timer 2) */
#define TIMER_BASE_ADDR	CSL_TMR_1_REGS

davinci_timer_reg    *davinci_timer = (davinci_timer_reg *) TIMER_BASE_ADDR;

/* Timer Initialize */
void inittimer(void)
{
        /* disable Timer 1 & 2 timers */
        davinci_timer->tcr = 0;

        /* Set timers to unchained dual 32 bit timers, Unreset timer34 */
        davinci_timer->tgcr = 0x0;
        davinci_timer->tgcr = 0x6;

        /* Program the timer12 counter register - set the prd12 for right count */
        davinci_timer->tim34 = 0;

        /* The timer is programmed to expire after 0xFFFFFFFF ticks */
        davinci_timer->prd34 = 0xFFFFFFFF;

        /* Enable timer34 */
        davinci_timer->tcr = (0x80 << 16); /* Timer34 continously enabled, Timer12 disabled */
}

/************************************************************
********************** Reset Processor **********************
************************************************************/
#define WDT_BASE_ADDR          CSL_WDT_1_REGS


void reset_processor(void)
{
        davinci_timer_reg *davinci_wdt = (davinci_timer_reg *) WDT_BASE_ADDR;
        davinci_wdt->tgcr   = 0x00000008;
        davinci_wdt->tgcr  |= 0x00000003;
        davinci_wdt->tim12  = 0x00000000;
        davinci_wdt->tim34  = 0x00000000;
        davinci_wdt->prd12  = 0x00000000;
        davinci_wdt->prd34  = 0x00000000;
        davinci_wdt->tcr   |= 0x00000040;
        davinci_wdt->wdtcr |= 0x00004000;
        davinci_wdt->wdtcr  = 0xA5C64000;
        davinci_wdt->wdtcr  = 0xDA7E4000;
}

