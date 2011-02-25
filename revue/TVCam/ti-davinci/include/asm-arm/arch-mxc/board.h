/*
 * Copyright 2004-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#ifndef __ASM_ARCH_MXC_BOARD_H__
#define __ASM_ARCH_MXC_BOARD_H__

#ifndef __ASM_ARCH_MXC_HARDWARE_H__
#error "Do not include directly."
#endif

/*
 * The modes of the UART ports
 */
#define MODE_DTE                0
#define MODE_DCE                1
/*
 * Is the UART configured to be a IR port
 */
#define IRDA                    0
#define NO_IRDA                 1

#ifdef CONFIG_MACH_MX27ADS
#include <asm/arch/board-mx27ads.h>
#endif

#endif				/* __ASM_ARCH_MXC_BOARD_H__ */
