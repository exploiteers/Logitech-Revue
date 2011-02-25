/*
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
 */
#ifndef __PCIMODULE_H__
#define __PCIMODULE_H__

#include <linux/types.h>

#define PCIMODULE_NAME			("dm6467pci")

/*
 * IOCTLs
 */

#define PCI_IOC_BASE			'P'
#define PCIMODULE_CMD_GENNOTIFY		(_IOW(PCI_IOC_BASE , 1, unsigned int))
#define PCIMODULE_CMD_GETBARLOCN	(_IOWR(PCI_IOC_BASE , 2		\
					, struct pcimodule_bar_mapping))

/** ============================================================================
 *  @name   pcimodule_intr_num
 *
 *  @desc   This object contains enumerations for the interrupt to be raised
 *
 *  @field  PCIMODULE_SOFT_INT0
 *              Enumerated value to generate software interrupt 0 to host
 *  @field  PCIMODULE_SOFT_INT1
 *              Enumerated value to generate software interrupt 1 to host
 *  @field  PCIMODULE_SOFT_INT2
 *              Enumerated value to generate software interrupt 2 to host
 *  @field  PCIMODULE_SOFT_INT3
 *              Enumerated value to generate software interrupt 3 to host
 *
 *  @see    None
 *  ============================================================================
 */
enum pcimodule_intr_num {
	PCIMODULE_SOFT_INT0 = 0,
	PCIMODULE_SOFT_INT1,
	PCIMODULE_SOFT_INT2,
	PCIMODULE_SOFT_INT3
};

/** ============================================================================
 *  @name   pcimodule_bar_num
 *
 *  @desc   This object contains enumerations for the BAR numbers
 *
 *  @field  PCIMODULE_BAR_0
 *              Enumerated value to indicate BAR-0
 *  @field  PCIMODULE_BAR_1
 *              Enumerated value to indicate BAR-1
 *  @field  PCIMODULE_BAR_2
 *              Enumerated value to indicate BAR-2
 *  @field  PCIMODULE_BAR_3
 *              Enumerated value to indicate BAR-3
 *  @field  PCIMODULE_BAR_4
 *              Enumerated value to indicate BAR-4
 *  @field  PCIMODULE_BAR_5
 *              Enumerated value to indicate BAR-5
 *
 *  @see    None
 *  ============================================================================
 */
enum pcimodule_bar_num {
	PCIMODULE_BAR_0 = 0,
	PCIMODULE_BAR_1,
	PCIMODULE_BAR_2,
	PCIMODULE_BAR_3,
	PCIMODULE_BAR_4,
	PCIMODULE_BAR_5
};

/** ============================================================================
 *  @name   pcimodule_bar_mapping
 *
 *  @desc   This object will be used to query bar mapping information
 *
 *  @field  enum pcimodule_bar_num
 *              Enumerated value to indicate bar number to be queried
 *  @field  unsigned int mapping
 *              Address where the BAR is mapped
 *
 *  @see    None
 *  ============================================================================
 */
struct pcimodule_bar_mapping {
	enum pcimodule_bar_num bar_num;
	unsigned int mapping;
};

#endif /* ifndef    (__PCIMODULE_H__) */
