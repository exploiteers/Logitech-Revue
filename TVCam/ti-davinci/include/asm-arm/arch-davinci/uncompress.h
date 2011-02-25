/*
 * Serial port stubs for kernel decompress status messages
 *
 *  Author:     Anant Gole
 * (C) Copyright (C) 2006, Texas Instruments, Inc
 * Copyright (C) 2008 MontaVista Software, Inc. <source@mvista.com>
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */

#include <linux/types.h>
#include <linux/serial_reg.h>
#include <asm/arch/serial.h>

/* FIXME: this is actually board specific! */
#ifdef	CONFIG_ARCH_DA8XX
#define CONSOLE_UART_BASE	DA8XX_UART2_BASE
#else
#define CONSOLE_UART_BASE	DAVINCI_UART0_BASE
#endif

/* PORT_16C550A, in polled non-FIFO mode */

static void putc(char c)
{
	volatile u32 *uart = (volatile u32 *)CONSOLE_UART_BASE;

	while (!(uart[UART_LSR] & UART_LSR_THRE))
		barrier();
	uart[UART_TX] = c;
}

static inline void flush(void)
{
	volatile u32 *uart = (volatile u32 *)CONSOLE_UART_BASE;

	while (!(uart[UART_LSR] & UART_LSR_THRE))
		barrier();
}

#define arch_decomp_setup()
#define arch_decomp_wdog()
