/*
 * DaVinci serial device definitions
 *
 * Author: Kevin Hilman, MontaVista Software, Inc. <source@mvista.com>
 *
 * 2007 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */
#ifndef __ASM_ARCH_SERIAL_H
#define __ASM_ARCH_SERIAL_H

#include <asm/arch/io.h>

#define DAVINCI_UART0_BASE   (IO_PHYS + 0x20000)
#define DAVINCI_UART1_BASE   (IO_PHYS + 0x20400)
#define DAVINCI_UART2_BASE   (IO_PHYS + 0x20800)

#define DA8XX_UART0_BASE     (IO_PHYS + 0x42000)
#define DA8XX_UART1_BASE     (IO_PHYS + 0x10C000)
#define DA8XX_UART2_BASE     (IO_PHYS + 0x10D000)

struct device;
extern int davinci_serial_init(struct device *dev);

#endif /* __ASM_ARCH_SERIAL_H */
