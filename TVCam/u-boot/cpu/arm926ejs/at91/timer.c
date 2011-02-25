/*
 * (C) Copyright 2007-2008
 * Stelian Pop <stelian.pop@leadtechdesign.com>
 * Lead Tech Design <www.leadtechdesign.com>
 *
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <asm/arch/hardware.h>
#include <asm/arch/at91_pit.h>
#include <asm/arch/at91_pmc.h>
#include <asm/arch/at91_rstc.h>
#include <asm/arch/io.h>

/*
 * We're using the AT91CAP9/SAM9 PITC in 32 bit mode, by
 * setting the 20 bit counter period to its maximum (0xfffff).
 */
#define TIMER_LOAD_VAL	0xfffff
#define READ_RESET_TIMER at91_sys_read(AT91_PIT_PIVR)
#define READ_TIMER at91_sys_read(AT91_PIT_PIIR)
#define TIMER_FREQ (AT91C_MASTER_CLOCK << 4)
#define TICKS_TO_USEC(ticks) ((ticks) / 6)

ulong get_timer_masked(void);
ulong resettime;

/* nothing really to do with interrupts, just starts up a counter. */
int timer_init(void)
{
	/*
	 * Enable PITC Clock
	 * The clock is already enabled for system controller in boot
	 */
	at91_sys_write(AT91_PMC_PCER, 1 << AT91_ID_SYS);

	/* Enable PITC */
	at91_sys_write(AT91_PIT_MR, TIMER_LOAD_VAL | AT91_PIT_PITEN);

	reset_timer_masked();

	return 0;
}

/*
 * timer without interrupts
 */

static inline ulong get_timer_raw(void)
{
	ulong now = READ_TIMER;

	if (now >= resettime)
		return now - resettime;
	else
		return 0xFFFFFFFFUL - (resettime - now) ;
}

void reset_timer_masked(void)
{
	resettime = READ_TIMER;
}

ulong get_timer_masked(void)
{
	return TICKS_TO_USEC(get_timer_raw());

}

void udelay_masked(unsigned long usec)
{
	ulong tmp;

	tmp = get_timer(0);
	while (get_timer(tmp) < usec)	/* our timer works in usecs */
		; /* NOP */
}

void reset_timer(void)
{
	reset_timer_masked();
}

ulong get_timer(ulong base)
{
	ulong now = get_timer_masked();

	if (now >= base)
		return now - base;
	else
		return TICKS_TO_USEC(0xFFFFFFFFUL) - (base - now) ;
}

void udelay(unsigned long usec)
{
	udelay_masked(usec);
}

/*
 * This function is derived from PowerPC code (read timebase as long long).
 * On ARM it just returns the timer value.
 */
unsigned long long get_ticks(void)
{
	return get_timer(0);
}

/*
 * This function is derived from PowerPC code (timebase clock frequency).
 * On ARM it returns the number of timer ticks per second.
 */
ulong get_tbclk(void)
{
	ulong tbclk;

	tbclk = CFG_HZ;
	return tbclk;
}

/*
 * Reset the cpu by setting up the watchdog timer and let him time out.
 */
void reset_cpu(ulong ignored)
{
	/* this is the way Linux does it */
	at91_sys_write(AT91_RSTC_CR, AT91_RSTC_KEY |
				     AT91_RSTC_PROCRST |
				     AT91_RSTC_PERRST);

	while (1);
	/* Never reached */
}
