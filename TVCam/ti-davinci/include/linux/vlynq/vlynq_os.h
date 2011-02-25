/*******************************************************************************
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
 *
 ******************************************************************************/

#ifndef __VLYNQ_OS_H__
#define __VLYNQ_OS_H__

#include <asm/arch/hardware.h>


/*
 * To keep ISR
 * [1] dipatch simple and short, and,
 * [2] to align signature of the ISR with that stipulated by the OS.
 *
 * We define the following.
 */
struct vlynq_dev_isr_param_grp_t {
	void *arg1;

};

/* The number of params in the ISR signature. */
#define VLYNQ_DEV_ISR_PARM_NUM 1

/*
 * We can have multiple VLYNQ IPs in our system.
 * Define 'LOW_VLYNQ_CONTROL_BASE' with the VLYNQ
 * IP having lowest base address.
 * Define 'HIGH_VLYNQ_CONTROL_BASE' with the VLYNQ
 * IP having highest base address.
 * In case of only one VLYNQ IP, define only the
 * 'LOW_VLYNQ_CONTROL_BASE'.
 * These macros are defined in SoC specific file
 * "asm/arch/hardware.h"
 */
#ifdef LOW_VLYNQ_CONTROL_BASE
#define LOW_VLYNQ_ROOT 1
#else
#define LOW_VLYNQ_ROOT 0
#endif

#ifdef HIGH_VLYNQ_CONTROL_BASE
#define HIGH_VLYNQ_ROOT 1
#else
#define HIGH_VLYNQ_ROOT 0
#endif

#define MAX_ROOT_VLYNQ (LOW_VLYNQ_ROOT + HIGH_VLYNQ_ROOT)

/* Board dependent. Should be moved to a board specific file, but later. */
#define MAX_VLYNQ_COUNT 4

#endif				/* #ifndef __VLYNQ_OS_H__ */
