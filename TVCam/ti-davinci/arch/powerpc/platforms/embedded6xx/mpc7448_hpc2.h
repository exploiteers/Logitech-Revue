/*
 * mpc7448_hpc2.h
 *
 * Definitions for Freescale MPC7448_HPC2 platform
 *
 * Author: Jacob Pan
 *         jacob.pan@freescale.com
 * Maintainer: Roy Zang <roy.zang@freescale.com>
 *
 * 2006 (c) Freescale Semiconductor, Inc.  This file is licensed under
 * the terms of the GNU General Public License version 2.  This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#ifndef __PPC_PLATFORMS_MPC7448_HPC2_H
#define __PPC_PLATFORMS_MPC7448_HPC2_H

#include <asm/ppcboot.h>

/* 'TICK' FPGA definitions */
#define TICK_TSCR		0x08
	#define TICK_TSCR_POE		(1<<6)
	#define TICK_TSCR_RSTE		(1<<5)

#define TICK_TRCR		0x0c
	#define TICK_TRCR_BRDRST	(1<<1)

#define TICK_TPWR		0x10
	#define TICK_TPWR_PWROFF	(1<<0)

#endif				/* __PPC_PLATFORMS_MPC7448_HPC2_H */
