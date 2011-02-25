/*
 * cir.h - Header file for the driver for Consumer Infrared (CIR)
	   (on Davinci-HD EVM)
 *
 * Copyright (C) 2007  Texas Instruments, India
 * Author: Suresh Rajashekara <suresh.r@ti.com>
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#ifndef __CIR_H__
#define __CIR_H__

#ifdef __KERNEL__
#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/wait.h>
#include <linux/completion.h>
#include <linux/interrupt.h>
#include <linux/err.h>

#include <asm/delay.h>
#include <asm/io.h>
#include <asm/irq.h>

#include <asm/arch/memory.h>
#include <asm/arch/edma.h>
#include <asm/arch/irqs.h>
// #include <asm/arch/mcasp.h>
#include <linux/clk.h>
#include <asm/atomic.h>
#include <asm/uaccess.h>

#define CIR_IOC_BASE     'C'
#define CIRIOC_GET_SYS_CLOCK_FREQ_PRESCALER  _IOR(CIR_IOC_BASE, 1, int)
#define CIRIOC_SET_SYS_CLOCK_FREQ_PRESCALER  _IOW(CIR_IOC_BASE, 2, int)
#define CIRIOC_GET_ENCODING                  _IOR(CIR_IOC_BASE, 3, int)

#define CIR_ENCODING_RC5   1

#define CIR_MAJOR_NUMBER 253	/* This macro is currently not used. Define it
				   to any value if you are using static minor
				   numbers and use the same in cir.c */
#define CIR_IRQ          IRQ_UARTINT2
#define CIR_DEV_COUNT    1
#endif /* __KERNEL__ */

#define CIR_IOC_MAGIC 'k'

#ifdef __KERNEL__
/* Only 3 Ioctls now. Update if adding new ioctl */
#define CIR_IOC_MAXNR 2
#endif  /* __KERNEL__ */

/* Ioctl to flush the buffers driver uses to store the keys received */
#define CIR_FLUSH _IO(CIR_IOC_MAGIC, 0)

/* Change the duration, within which, if another key is received is neglected */
#define CIR_SET_REPEAT_DELAY _IOW(CIR_IOC_MAGIC, 1, int)

/* Read the duration, within which, if another key is received is neglected */
#define CIR_GET_REPEAT_DELAY _IOR(CIR_IOC_MAGIC, 2, int)
#ifdef __KERNEL__
#define CIR_BASE  DM644X_UART2_BASE /* UART 2 */

/* CIR Register Mapping */
#define RHR          (0x00)
#define THR          (0x00)
#define IER          (0x04)
#define IIR          (0x08)
#define FCR          (0x08)
#define LCR          (0x0C)
#define MCR          (0x10)
#define LSR          (0x14)
#define MSR          (0x18)
#define TCR          (0x18)
#define TLR          (0x1C)
#define SPR          (0x1C)
#define MDR1         (0x20)
#define MDR2         (0x24)
#define SFLSR        (0x28)
#define TXFLL        (0x28)
#define RESUME       (0x2C)
#define TXFLH        (0x2C)
#define SFREGL       (0x30)
#define RXFLL        (0x30)
#define SFREGH       (0x34)
#define RXFLH        (0x34)
#define BLR          (0x38)
#define ACREG        (0x3C)
#define SCR          (0x40)
#define SSR          (0x44)
#define EBLR         (0x48)
#define MVR          (0x50)
#define SYSC         (0x54)
#define SYSS         (0x58)
#define WER          (0x5C)
#define CFPS         (0x60)

/* ------------------------------------------------------------------------ *
 *  Control Registers for: UART Registers minus Base Address (  )           *
 *  These Registers are available with "LCR[bit 7] = 1"                     *
 * ------------------------------------------------------------------------ */
#define USE_LCR_80              ( 0x100 )
#define DLL                     ( USE_LCR_80 + 0x0 )
#define DLH                     ( USE_LCR_80 + 0x4 )
#define UASR                    ( USE_LCR_80 + 0x38 )

/* ------------------------------------------------------------------------  *
 *  Control Registers for: UART Registers minus Base Address (  )            *
 *  These Registers are available with "LCR = 0xBF"                          *
 *  ------------------------------------------------------------------------ */
#define USE_LCR_BF              ( 0x200 )
#define EFR                     ( USE_LCR_BF + 0x8 )
#define XON1                    ( USE_LCR_BF + 0x10 )
#define XON2                    ( USE_LCR_BF + 0x14 )
#define XOFF1                   ( USE_LCR_BF + 0x18 )
#define XOFF2                   ( USE_LCR_BF + 0x1c )

#define BIT(n)       (1<<n)
#endif /* __KERNEL__ */

#endif /* _CIR_H__ */
