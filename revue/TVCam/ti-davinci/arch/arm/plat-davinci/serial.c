/*
 * TI DaVinci serial driver
 *
 * Copyright (C) 2006 Texas Instruments.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/serial_reg.h>
#include <linux/serial_8250.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/delay.h>

int davinci_serial_init(struct device *dev)
{
	struct clk *uart_clk;
	char uart_name[6];
	struct plat_serial8250_port *pspp;
	int i;

	if (dev && dev->bus_id && !strcmp(dev->bus_id, "serial8250.0")) {
		for (i = 0, pspp = dev->platform_data; pspp->flags;
				i++, pspp++) {
			sprintf(uart_name, "UART%i", i);
			uart_clk = clk_get(dev, uart_name);
			if (IS_ERR(uart_clk))
				goto err_out;

			clk_enable(uart_clk);
		}
	}
	return 0;

err_out:
	dev_err(dev, "failed to get %s clock\n", uart_name);
	return -1;
}

#define UART_DAVINCI_PWREMU 0x0c

static inline void davinci_serial_outp(struct plat_serial8250_port *p,
				       int offset, int value)
{
	offset <<= p->regshift;
	__raw_writew(value, p->membase + offset);
}

void davinci_serial_reset(struct plat_serial8250_port *p)
{
	/* reset both transmitter and receiver: bits 14,13 = UTRST, URRST */
	unsigned int pwremu = 0;

	davinci_serial_outp(p, UART_IER, 0);  /* disable all interrupts */

	davinci_serial_outp(p, UART_DAVINCI_PWREMU, pwremu);
	mdelay(10);

	pwremu |= 0x3 << 13;
	pwremu |= 0x1;
	davinci_serial_outp(p, UART_DAVINCI_PWREMU, pwremu);
}
