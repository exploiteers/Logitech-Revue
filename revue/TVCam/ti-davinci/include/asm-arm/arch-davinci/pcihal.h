/*
 *
 * TI DaVinciHD PCI Module file
 *
 * Copyright (C) 2007 Texas Instruments
 *
 * ----------------------------------------------------------------------------
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
 * ----------------------------------------------------------------------------
 *
 */
#ifndef __PCIHAL_H__
#define __PCIHAL_H__

/** ============================================================================
 *  @macro  PCIMODULE_PCI_IRQ
 *
 *  @desc   IRQ number of PCI interrupt on DAVINCI-HD
 *  ============================================================================
 */
#define PCIMODULE_PCI_IRQ		15

/** ============================================================================
 *  @const   PCI_REG_INTSTATUS
 *
 *  @desc    PCI int status register address used for generating INT to HOST.
 *  ============================================================================
 */
#define PCI_REG_INTSTATUS		0x01C1A010

/** ============================================================================
 *  @const   PCI_REG_HINTSET
 *
 *  @desc    PCI host int set register address used for generating INT to HOST.
 *  ============================================================================
 */
#define PCI_REG_HINTSET			0x01C1A020

/** ============================================================================
 *  @const   PCI_REG_HINTSET
 *
 *  @desc    PCI host int ckaer register address used for generating INT to HOST
 *  ============================================================================
 */
#define PCI_REG_HINTCLR			0x01C1A024

/** ============================================================================
 *  @const   PCI_REG_INTCLEAR
 *
 *  @desc    PCI int clear register address used for clearing INT from HOST.
 *  ============================================================================
 */
#define PCI_REG_INTCLEAR		0x01C1A014

/** ============================================================================
 *  @const   PCI_SOFTINT0_MASK
 *
 *  @desc    Mask for generating soft int0 (DSP->GPP)
 *  ============================================================================
 */
#define PCI_SOFTINT0_MASK		0x01000000

/** ============================================================================
 *  @const   PCI_SOFTINT1_MASK
 *
 *  @desc    Mask for generating soft int1 (GPP->DSP)
 *  ============================================================================
 */
#define PCI_SOFTINT1_MASK		0x02000000

/** ============================================================================
 *  @const   PCI_SOFTINT2_MASK
 *
 *  @desc    Mask for generating soft int2 (GPP->DSP)
 *  ============================================================================
 */
#define PCI_SOFTINT2_MASK		0x04000000

/** ============================================================================
 *  @const   PCI_SOFTINT3_MASK
 *
 *  @desc    Mask for generating soft int3 (GPP->DSP)
 *  ============================================================================
 */
#define PCI_SOFTINT3_MASK		0x08000000

/** ============================================================================
 *  @const   PCI_REG_BAR0TRL
 *
 *  @desc    Address of register containing physical mapping of BAR0
 *  ============================================================================
 */
#define PCI_REG_BAR0TRL			0x01C1A1C0

/** ============================================================================
 *  @const  REG32
 *
 *  @desc   Macro for register access.
 *  ============================================================================
 */
#define REG32(addr)			*((u32 *)(IO_ADDRESS(addr)))


#endif /* ifndef    (__PCIHAL_H__) */
