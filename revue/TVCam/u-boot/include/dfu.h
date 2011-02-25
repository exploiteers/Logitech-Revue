/*
 * (C) Copyright 2009 AdvanceV Corp
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 ********************************************************************
 * NOTE: This header file defines an interface to firmware update
 ********************************************************************
 */

#ifndef __DFU_H__
#define __DFU_H__

#include <command.h>
#define DFU_MAGIC_NUMBER 		0x3c7b8516

#define DFU_TRIGGER_MEMORY_ADDR 0x87fffff0 

#define LINUX_DDR_LOCATION 		0x80700000
#define CRAMFS_DDR_LOCATION 	0x82000000

#define LINUX_NAND_OFFSET		0x400000
#define CRAMFS_NAND_OFFSET	 	0x800000

#define LINUX_NAND_SIZE			0x400000
#define CRAMFS_NAND_SIZE		0xa00000

/*
 * DFU trigger structure
 */
typedef struct dfu_trigger {
	unsigned int	dfu_magic;
	unsigned int	dfu_mode;	/* DFU mode: 1 in DFU mode; 0: not in DFU mode	*/
	unsigned int	dfu_status;	/* DFU status (not used as default to 0 for now)*/
	unsigned int	dfu_crc;	/* DFU mode and data CRC Checksum	*/
} dfu_trigger_t;

int check_cramfs_crc(unsigned char *ddr_image_location);
int check_linux_crc(unsigned char *ddr_image_location);
int load_cramfs_image(unsigned int nand_offset, unsigned char *ddr_image_location);
int load_linux_image(unsigned int nand_offset, unsigned char *ddr_image_location);
int store_image_to_nand(unsigned int nand_offset, unsigned int image_size, unsigned char *ddr_image_location);


#endif	/* __DFU_H__ */
