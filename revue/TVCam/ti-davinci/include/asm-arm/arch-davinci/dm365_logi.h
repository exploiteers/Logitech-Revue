/* *
 * Copyright (C) 2009 Logitech Inc
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef DM365_LOGI_H
#define DM365_LOGI_H

#ifndef _LINUX_IOCTL_H
#include <linux/ioctl.h>
#endif

#ifdef __KERNEL__
#define DRIVERNAME  		"DM365LOGI"
#define DM365_PHYS_ADDR		0x01C70000			// VPSS subsystem physical start
#endif

#define CLASS_NAME			"dm365_logi"		// device file - /dev/dm365_logi
#define DM365_PHYS_SIZE		0x00010000			// VPSS subsystem physical size - 64K

#define LOGI_BYTE			0
#define LOGI_WORD			1
#define LOGI_LONG			2

#define LOGI_MAX_TABLES		16

// DM365 ioctl register access
struct LOGI_IO_REGISTER{
	int				reg;	// offset referenced from start
	int				size;	// see LOGI_xxxx defines above
	bool			rd;		// operation - read = true, write = false
	unsigned long	value;	// value read or value to write
};

// LSC ioctl table access
struct LOGI_TABLE{
	int		reg;   		// specifies address upper - lower is + 4 bytes
	int		size;		// length of data
	void	*_data;		// pointer to data
	int		tindex;		// returned to client
};

// ioctls
#define	LOGI_MAGIC_NO		'l'
#define LOGI_IO_REG		_IOWR(LOGI_MAGIC_NO, 0, struct LOGI_IO_REGISTER *)	// rw access to registers
#define LOGI_SET_TABLE	_IOWR(LOGI_MAGIC_NO, 1, struct LOGI_TABLE *)		// set table data
#define LOGI_DEL_TABLE	_IOWR(LOGI_MAGIC_NO, 2, struct LOGI_TABLE *)		// delete table data
#define LOGI_MAX_NR		3

#endif	// DM365_LOGI_H

