/*
 * Access to the registers of the MSP430 controller on the TI DaVinci boards
 *
 * Author: Aleksey Makarov, MontaVista Software, Inc. <source@mvista.com>
 *
 * 2008 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
*/

#ifndef __ASM_ARCH_TI_MSP430_H
#define __ASM_ARCH_TI_MSP430_H

int ti_msp430_read(u16 reg, u8 * value);
int ti_msp430_write(u16 reg, u8 value);

#endif /* __ASM_ARCH_TI_MSP430_H */
