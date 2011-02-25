/*
 * A collection of structures, addresses, and values associated with
 * the Embedded Planet ep88xc board.
 * Copied from the FADS stuff.
 *
 * Author: MontaVista Software, Inc.
 *         source@mvista.com
 *
 * 2005 (c) MontaVista Software, Inc.  This file is licensed under the
 * terms of the GNU General Public License version 2.  This program is licensed
 * "as is" without any warranty of any kind, whether express or implied.
 */

#ifdef __KERNEL__
#ifndef __ASM_EP88XC_H__
#define __ASM_EP88XC_H__

#include <asm/ppcboot.h>
#include <sysdev/fsl_soc.h>

#define IMAP_ADDR		(get_immrbase())
#define IMAP_SIZE		((uint)(64 * 1024))

extern void cpm_reset(void);
extern void __init init_ioports(void);

#endif /* __ASM_EP88XC_H__ */
#endif /* __KERNEL__ */
