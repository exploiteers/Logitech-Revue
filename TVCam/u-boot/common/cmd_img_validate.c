/*
 * (C) Copyright 2000-2006
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <watchdog.h>
#include <command.h>
#include <image.h>
#include <malloc.h>
#include <zlib.h>
#include <bzlib.h>
#include <environment.h>
#include <lmb.h>
#include <asm/byteorder.h>
#include <nand.h>
#include <cramfs/cramfs_fs.h>

int validate_cramfs(void)
{
	unsigned int ddr_image_location = 0x82000000;
	unsigned char *hdr = (void *)ddr_image_location;
	unsigned int nand_block_size = 0x20000;
	unsigned int nand_offset = 0x800000;
	unsigned int size = nand_block_size;
	unsigned int stored_crc = 0;
	nand_info_t *nand;
	nand = &nand_info[nand_curr_device];
	struct cramfs_super *super = (struct cramfs_super *) ddr_image_location;	
	
	if(nand_read_skip_bad(nand, nand_offset, &size, hdr) != 0)
	{
		printf("CRAMFS: nand read failed\n");
		return 1;
	}

	if(0x28cd3d45 != super->magic)
	{
		printf("CRAMFS: bad magic number\n");
		return 1;
	}

	size = ((super->size + sizeof(struct cramfs_super) + nand_block_size -1)/nand_block_size) * nand_block_size;
	
	if(nand_read_skip_bad(nand, nand_offset, &size, hdr) != 0)
	{
		printf("CRAMFS: Nand read failed\n");
		return 1;
	}
		
	stored_crc = super->fsid.crc;
	super->fsid.crc = 0;
	if(crc32_wd (0, (unsigned char *)hdr, super->size, CHUNKSZ_CRC32) != stored_crc)
	{
		printf("CRAMFS: Bad data Checksum\n");
		return 1;
	}
	/* restore crc */
	super->fsid.crc = stored_crc;
	
	return 0;
}

int validate_linux(void)
{
	/*unsigned int ddr_image_location = 0x82000000;*/
	unsigned int ddr_image_location = 0x80700000;
	unsigned char *hdr = (void *)ddr_image_location;
	unsigned int nand_block_size = 0x20000;
	unsigned int nand_offset = 0x400000;
	unsigned int size = nand_block_size;
	nand_info_t *nand;
	nand = &nand_info[nand_curr_device];

	if(nand_read_skip_bad(nand, nand_offset, &size, hdr) != 0)
	{
		printf("LINUX: Nand Read Failed\n");
		return 1;
	}
	if (!image_check_magic (hdr)) {
		printf("LINUX: Bad Magic Number\n");
		return 1;
	}
	
	if (!image_check_hcrc (hdr)) {
		puts ("LINUX: Bad Header Checksum\n");
		return 1;
	}

	/* rounding off to block size */
	size = 	((image_get_data_size(hdr) + image_get_header_size() + nand_block_size -1 ) / nand_block_size) * nand_block_size;	
	
	if(nand_read_skip_bad(nand, nand_offset, &size, hdr) != 0)
	{
		printf("LINUX: Nand Read Failed\n");
		return 1;
	}
	
	if (!image_check_dcrc (hdr)) {
		printf("LINUX: Bad Data Checksum\n");
		return 1;
	}
	return 0;
}


int validate_images(void)
{
	int ret=0;
	ret = validate_linux();
	ret += validate_cramfs();
	return ret;

}

int do_img_validate (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{

	int ret = 1;

	if (argc != 1)
       	{
		return 1;
	}

	printf("validating Linux and cramfs images on nand\n");
	if(ret = validate_images())
	{
		printf("Linux and cramfs image validation failed\n");
	}
	else
	{
		printf("validation of Linux and cramfs images is successful\n");
	}
	return ret;
	
}

U_BOOT_CMD(
	img_validate,	1,	1,	do_img_validate,
	"img_validate  - validates the linux and cramfs images stored in nand\n",
	NULL
);

