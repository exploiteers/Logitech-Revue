/*
GPL LICENSE SUMMARY

Copyright(c) 2005-2009 Intel Corporation. All rights reserved.

This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
General Public License for more details.
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
The full GNU General Public License is included in this distribution
in the file called LICENSE.GPL.

Contact Information:
Intel Corporation
2200 Mission College Blvd.
Santa Clara, CA 97052
*/

/* gbe_mac_access.h
 * Structures, enums, and macros for the MAC
 */
#ifndef _GBE_MAC_ACCESS_H_
#define _GBE_MAC_ACCESS_H_

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/pci_ids.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <asm/semaphore.h>
#include <asm/uaccess.h>
#include <linux/time.h>

extern	void __iomem * gbe_config_base_virt;
#define GBE_CONFIG_BASE_VIRT  	gbe_config_base_virt

#define CONFIG_RAM_BASE		0x60000 
#define GBE_CONFIG_OFFSET	0x0
#define GBE_CONFIG_RAM_BASE		((unsigned int)(CONFIG_RAM_BASE + GBE_CONFIG_OFFSET))
#define GBE_CONFIG_DATA_LENGTH	0x200 

#define GBE_CONFIG_FLASH_READ(base,offset,count,data)		gbe_config_read_words(base,offset,count,data)
#define GBE_CONFIG_FLASH_WRITE(base,offset,count,data)		gbe_config_write_words(base,offset,count,data)

#define GBE_PERROR(fmt, args...) printk("[ERROR] %s: " fmt, __FUNCTION__ , ## args)
  
extern int (* gbe_config_media_read)(void  __iomem * config_ram_base_t, int length);
extern int (* gbe_config_media_write)(void  __iomem * config_ram_base_t, int length);

/* = = = = = = = = = = = = = = = = = = = = == = = = = = = = = = = = = = = = = = = = = */
/*!  \brief Write GbE configuration data from RAM
 *
 *   Write configuration data from RAM 
 *
 *     @param[in] ram_base_t         the ram base address used to store the configuration data
 *     @param[in] offset_t           offset address in the configuration data. 
 *     @param[in] count              total numbers of word to be read  
 *     @param[out] buf_t             buffer of the  data to be written
 *     @return  logical true if successful
 *              logical false if failed for some reason.
 */

static int inline gbe_config_write_words(void  __iomem * ram_base_t, unsigned short offset_t, unsigned short count, unsigned short *buf_t)
{
	volatile unsigned short * dst;
	unsigned int * config_ram_base_t ;
	int length = GBE_CONFIG_DATA_LENGTH; 

	dst = (volatile unsigned short *)(ram_base_t + (unsigned int)offset_t);
	*dst = *buf_t;

	if(gbe_config_media_write){
        config_ram_base_t = gbe_config_base_virt;	
		gbe_config_media_write(config_ram_base_t, length);		
	}
	return 0;
}

/*  write a word  */
static int inline read_one_word(void  __iomem * ram_base_t, unsigned short offset, unsigned short *buf)
{
    int ret = 0;
    volatile unsigned int ramBase;
	volatile unsigned short * src;

	if(!buf){
		GBE_PERROR("Invalid buffer address!\n");
		return -EINVAL;
	}
	
	if( offset > 0x3f)
	{
		GBE_PERROR("invalid offset number: %x\n", offset);
		return -EINVAL;
	}

	// translate the address from eeporm address (in word) into the flash address (in byte);
	offset<<=1;

	ramBase = (volatile unsigned int)ram_base_t;
	src = (volatile unsigned short *)(ramBase + (unsigned int)offset);
	*buf = *src;
    
	return ret;
}
/* = = = = = = = = = = = = = = = = = = = = == = = = = = = = = = = = = = = = = = = = = */
/*!  \brief Read GbE configuration data from RAM
 *
 *   Read  configuration data from RAM 
 *
 *     @param[in] ram_base_t           the ram base address used to store the configuration data
 *     @param[in] offset_t           offset address in the configuration data. 
 *     @param[in] count              totol numbers of word to be read  
 *     @param[out] buf_t             buffer of returned data   
 *     @return  logical true if successful
 *              logical false if failed for some reason.
 */

static int inline gbe_config_read_words(void  __iomem * ram_base_t, unsigned short offset_t, unsigned short count, unsigned short *buf_t)
{
	int ret = 0 ;
	unsigned short i, offset, * buf;

	if( (count + offset_t - 1 ) > 0x3f){
		printk("Invalid words number, eeprom space limited to 0x3f word size\n");
	}

	offset = offset_t;
	buf = buf_t;	

	for(i=0;i<count;i++){
		read_one_word(ram_base_t, offset, buf);
		offset ++;
		buf ++;
	}
	
	return ret;
}
#endif //_GBE_MAC_ACCESS_H_
