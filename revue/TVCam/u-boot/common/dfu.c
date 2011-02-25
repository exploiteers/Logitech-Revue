/*
 * (C) Copyright 2009
 * AdvanceV Corp.
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
#include <asm/byteorder.h>
#include <cramfs/cramfs_fs.h>
#include <image.h>
#include <malloc.h>
#include <nand.h>
#include <dfu.h>

#define GPIO_OUT_DATA0	0x14

/* check cramfs_crc.
 * assume: cramfs image is in memory
 * return: 0: success
 *         1: fail (either on magic or crc)
 */
int check_cramfs_crc(unsigned char *ddr_image_location)
{
	unsigned char *hdr = ddr_image_location;
	unsigned int stored_crc = 0;
	struct cramfs_super *super = (struct cramfs_super *) ddr_image_location;	

	if( CRAMFS_MAGIC != super->magic)
	{
		printf("%s(): bad magic number\n", __FUNCTION__);
		return -1;
	}

	stored_crc = super->fsid.crc;
	super->fsid.crc = 0;
	if(crc32_wd (0, (unsigned char *)hdr, super->size, CHUNKSZ_CRC32) != stored_crc)
	{
		printf("%s(): bad data checksum\n", __FUNCTION__);
		return -2;
	}
	/* restore crc */
	super->fsid.crc = stored_crc;
	printf("%s(): success\n", __FUNCTION__);
	return 0;
}

/* check linux_crc.
 * assume: linux image is in memory
 * return: 0: success
 *         1: fail (either on magic or crc)
 */
int check_linux_crc(unsigned char *ddr_image_location)
{
	unsigned char *hdr = (void *)ddr_image_location;
	unsigned int size = 0;

	if (!image_check_magic ((image_header_t*)hdr)) {
		printf("%s(): bad magic number\n", __FUNCTION__);
		return -1;
	}
	
	if (!image_check_hcrc ((image_header_t*)hdr)) {
		printf("%s(): bad header checksum\n", __FUNCTION__);
		return -2;
	}

	size = image_get_data_size((image_header_t*)hdr) + image_get_header_size();
	printf("%s(): uImage size = %d (0x%x)\n", __FUNCTION__, size, size );

	if (!image_check_dcrc ((image_header_t*)hdr)) {
		printf("%s(): bad data checksum\n", __FUNCTION__);
		return -3;
	}

	printf("%s(): success\n", __FUNCTION__);
	return 0;
}

/* load cramfs image from nand to ddr memory
 * return: 0: success
 *         1: fail (either on magic or crc)
 */
int load_cramfs_image(unsigned int nand_offset, unsigned char *ddr_image_location)
{
	unsigned char *hdr = ddr_image_location;
	unsigned int nand_block_size = 0x20000;
	unsigned int size = nand_block_size;
	nand_info_t *nand;
	nand = &nand_info[nand_curr_device];
	struct cramfs_super *super = (struct cramfs_super *) ddr_image_location;	

	if(nand_read_skip_bad(nand, nand_offset, &size, hdr) != 0) {
		printf("%s(): nand read header fail\n", __FUNCTION__);
		return -1;
	}
	if( CRAMFS_MAGIC != super->magic) {
		printf("%s(): bad magic number\n", __FUNCTION__);
		return -2;
	}

	/* rounding off to block size */
	size = ((super->size + sizeof(struct cramfs_super) + nand_block_size -1)/nand_block_size) * nand_block_size;
	
	/* load image */
	if(nand_read_skip_bad(nand, nand_offset, &size, hdr) != 0) {
		printf("%s(): nand read failed\n", __FUNCTION__);
		return -3;
	}

	printf("%s(): success\n", __FUNCTION__);
	return 0;
}


/* load linux uImage from nand to ddr memory
 * return: 0: success
 *         1: fail (either on magic or crc)
 */
int load_linux_image(unsigned int nand_offset, unsigned char *ddr_image_location)
{
	unsigned char *hdr = ddr_image_location;
	unsigned int nand_block_size = 0x20000;
	unsigned int size = nand_block_size;
	nand_info_t *nand;
	nand = &nand_info[nand_curr_device];

	/* load image header first */
	if(nand_read_skip_bad(nand, nand_offset, &size, hdr) != 0) {
		printf("%s(): nand read image header fail\n", __FUNCTION__);
		return -1;
	}
	if (!image_check_magic ((image_header_t*)hdr)) {
		printf("%s(): bad magic number\n", __FUNCTION__);
		return -2;
	}
	if (!image_check_hcrc ((image_header_t*)hdr)) {
		printf("%s(): bad header checksum\n", __FUNCTION__);
		return -3;
	}

	/* rounding off to block size */
	size = 	((image_get_data_size((image_header_t*)hdr) + image_get_header_size() + nand_block_size -1 ) / nand_block_size) * nand_block_size;	
	
	/* load image */
	if(nand_read_skip_bad(nand, nand_offset, &size, hdr) != 0) {
		printf("%s(): nand read failed\n", __FUNCTION__);
		return -4;
	}

	size = image_get_data_size((image_header_t*)hdr) + image_get_header_size();
	printf("%s(): success. uImage size = %d (0x%x)\n", __FUNCTION__, size, size );

	return 0;
}

/* store image from ddr memory to nand
 * return: 0: success
 *         1: fail (either on magic or crc)
 */
int store_image_to_nand(unsigned int nand_offset, unsigned int image_size, unsigned char *ddr_image_location)
{
	int rc;
	char buf[256];

	sprintf(buf, "nand erase %x %x", nand_offset, image_size);
	rc = run_command(buf, 0);
	if (rc < 0 ) {
		printf("%s(): nand erase fail\n", __FUNCTION__);
		return rc;
	}
	
	sprintf(buf, "nand write %x %x %x", (unsigned int)ddr_image_location, nand_offset, image_size);
	rc = run_command(buf, 0);
	if (rc < 0 ) {
		printf("%s(): nand write fail\n", __FUNCTION__);
		return rc;
	}
	
	printf("%s(): success. rc=%d\n", __FUNCTION__, rc);
	return 0;
}

