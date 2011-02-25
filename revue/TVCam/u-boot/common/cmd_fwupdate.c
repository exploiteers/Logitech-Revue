/*
 * (C) Copyright 2009
 * AdvanceV Corp
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

#ifdef ADVANCEV_FW_UPDATE
/*
 * FW Update functions
 */
#include <common.h>
#include <command.h>
#include <dfu.h>				/* wolverine fw download */
#include <cramfs/cramfs_fs.h> 	/* cramfs */
#include <image.h> 				/* linux image */
#include <i2c_slave.h>
#include <nand.h>
#include <IWolverine.h>

/* macros */

/* image flag mask */
#define UBL_IMAGE		 	0x01
#define UBOOT_IMAGE		 	0x02
#define PARAMS_BLOCK_IMAGE	0x04
#define LINUX_IMAGE	 		0x08
#define CRAMFS_IMAGE 		0x10
#define LIVEFS_IMAGE	 	0x20

#if 0
/* image address range */
#define LINUX_IMAGE_START 	(LINUX_DDR_LOCATION)
#define	LINUX_IMAGE_END		0x80afffff
#define CRAMFS_IMAGE_START	(CRAMFS_DDR_LOCATION)
#define	CRAMFS_IMAGE_END	0x836fffff /* start + 32MB */
#endif
	
/* XU command */
#define XU_COMMAND_MIN_LEN 5

/* XU Status 0x0000 */
#define INDEX_OPERATION_STATUS		0x01
#define MODE_UBOOT_DFU_MODE			0x01
#define MODE_LINUX_MODE				0x02
#define MODE_DATA_DOWNLOAD_MODE		0x03
#define MODE_FAIL					0x80

/* DFU states */
typedef enum {
	DFU_IDLE = 0,
	DFU_MODE,
	DFU_DATA,
	DFU_DATA_DONE,
	DFU_MODE_DONE,
	DFU_MODE_RESTART
} dfu_state_t;

/* functions */
static int process_fw_update(dfu_state_t *dfu_state, unsigned int *image_flag) 
{
	int 			rc = 0;  /* default to success */
	dfu_state_t state = *dfu_state;
	unsigned char	index;
	unsigned char	mode;

	printf("%s(): Enter. dfu_state=%d, image_flag=0x%x\n", __FUNCTION__, state, *image_flag);
	
	/* check state */
	switch (state) {
		/*case DFU_MODE_DONE:*/
		case DFU_DATA_DONE:
			/*  one DATA type at a time only */
			if ( *image_flag & LINUX_IMAGE ) {
				/* check linux image crc */
				rc = check_linux_crc((unsigned char*)LINUX_DDR_LOCATION);
				if (0 == rc) {
					/* write image to NAND */
					rc = store_image_to_nand(LINUX_NAND_OFFSET, LINUX_NAND_SIZE, (unsigned char*)LINUX_DDR_LOCATION);
					if ( rc ) {
						/* may need to update status in IWolverine */
					}
				}
				*image_flag &=~LINUX_IMAGE;
			} 

			if ( *image_flag & CRAMFS_IMAGE ) {
				/* check cramfs image crc */
				rc = check_cramfs_crc((unsigned char*)CRAMFS_DDR_LOCATION);
				if (0 == rc) {
					/* write image to NAND */
					rc = store_image_to_nand(CRAMFS_NAND_OFFSET, CRAMFS_NAND_SIZE, (unsigned char*)CRAMFS_DDR_LOCATION);
					if ( rc ) {
						/* may need to update status in IWolverine */
					}
				}
				*image_flag &=~CRAMFS_IMAGE;
			}

			/* based on the image_flag to continue update */
			/* code: TBD */

			/* change state */
			state = DFU_MODE;
			/* update Status in IWolverine */
			index = INDEX_OPERATION_STATUS;
			if (0 == rc) {
				/* update success */
				mode  = MODE_UBOOT_DFU_MODE;
			}
			else {
				/* update fail */
				/* back to FW download start state for another update from host */
				mode  = MODE_FAIL;
			}
			rc = sendWolverineIntStatus(hWolverineInterface, index, mode);
		break;

		default:
		break;		
	} /* end of switch */

	*dfu_state = state;

	printf("%s(): Exit.  dfu_state=%d, image_flag=0x%x, rc=%d\n", __FUNCTION__, state, *image_flag, rc);

	return rc;
}

static int process_xu_command(dfu_state_t *dfu_state, unsigned int *image_flag, unsigned long *ddr_base)
{
	int 			rc = 0;  /* default to success */
	unsigned long   u32val = 0;

	printf("%s(): Enter... \n", __FUNCTION__);

	/* get var handle and also lock the mutex */
	T_WOLVERINE_INTERFACE_VARS_HANDLE hVars = getWolverineIntVars(hWolverineInterface);

	/* 0xFFAA_0074 Go into DFU mode */
	/*if (_wolverineIntIsDirty(hWolverineInterface, 0x0074, 4) ) {*/	
	if ( wolverineIntIsDirty(hWolverineInterface, lxu_enter_dfu_mode) ) {
		u32val = hVars->lxu_enter_dfu_mode.dwDFUMode;
		wolverineIntClrDirty(hWolverineInterface, lxu_enter_dfu_mode );
		/*rc = wolverineIntReadBuffer((unsigned char*)&u32val, 4, 0xFFAA0074);
		_wolverineIntClrDirty(hWolverineInterface, 0x0074, 4);*/

		printf("%s(): 0xFFAA_0074 Go into DFU mode 0x%x=(%u)\n", __FUNCTION__, (unsigned int)u32val, (unsigned int)u32val);
		
		if ( 0 == u32val ) {
			/* dfu mode 0x00 as end of fw download */
			if (DFU_MODE == *dfu_state) {
				/* report success status */
				/* check dm365_status */
    			if (INDEX_OPERATION_STATUS == hVars->dm365_status.index &&
					MODE_FAIL ==  hVars->dm365_status.mode )
					rc = -1; /* exit fw-updae as fail */
			}
			else {
				/* report fw_update process fail status, exit fw_update as fail */
				rc = -2;
			}
#if 0
			*dfu_state = DFU_MODE_DONE;

			/* notify 2211 (2010-01-07: not required) */
			rc = sendWolverineIntStatus(hWolverineInterface, INDEX_OPERATION_STATUS, MODE_UBOOT_DFU_MODE);
#endif

			/* exit DFU mode. means exit fw_update operation */
			*dfu_state = DFU_IDLE;
		}
	}

	/* 0xFFAA_0078 Data Down Load mode */
	else if ( wolverineIntIsDirty(hWolverineInterface, lxu_data_download_mode) ) {
		u32val = hVars->lxu_data_download_mode.dwDataDownloadMode;
		wolverineIntClrDirty(hWolverineInterface, lxu_data_download_mode );
		/*rc = wolverineIntReadBuffer((unsigned char*)&u32val, 4, 0xFFAA0078);		
		_wolverineIntClrDirty(hWolverineInterface, 0x0078, 4);*/
		
		printf("%s(): 0xFFAA_0078 Data download mode 0x%x=(%u)\n", __FUNCTION__, (unsigned int) u32val, (unsigned int)u32val);

		if ( u32val ) {
			switch (u32val) {
				case 1:
					/* UBL */
				break;
				case 2:
					/* Uboot */
					/* use this case to re-load linux */
					rc = load_linux_image((unsigned int)LINUX_NAND_OFFSET, (unsigned char*)LINUX_DDR_LOCATION);
					rc = check_linux_crc((unsigned char*)LINUX_DDR_LOCATION);
				break;
				case 3:
					/* params block */

					/* use this case to re-load cramfs */
					rc = load_cramfs_image((unsigned int)CRAMFS_NAND_OFFSET, (unsigned char*)CRAMFS_DDR_LOCATION);
					rc = check_cramfs_crc((unsigned char*)CRAMFS_DDR_LOCATION);
				break;
				case 4:
					/* linux kernel */
					printf("%s():0xFFAA_0078 in data mode for Linux image\n", __FUNCTION__);
					*image_flag |= LINUX_IMAGE;
					*ddr_base = LINUX_DDR_LOCATION;
				break;
				case 5:
					/* cramfs */
					printf("%s():0xFFAA_0078 in data mode for cramfs image\n", __FUNCTION__);
					*image_flag |= CRAMFS_IMAGE;
					*ddr_base = CRAMFS_DDR_LOCATION; 
				break;
				case 6:
				/* livefs */
				break;
				default:
				/* reset of file TBD */
				break;
			} /* end of switch */
			/* enter data download mode */
			*dfu_state = DFU_DATA;
			/* notify 2211 */
			rc = sendWolverineIntStatus(hWolverineInterface, INDEX_OPERATION_STATUS, MODE_DATA_DOWNLOAD_MODE);
		}
		else {
			*dfu_state = DFU_DATA_DONE;
			rc = process_fw_update(dfu_state, image_flag);
		}
	}

	printf("%s(): Exit. rc=%d\n", __FUNCTION__, rc);

	return rc;
}


static int fw_update(void)
{
	int 			rc = 1;
	dfu_state_t		dfu_state = DFU_MODE;
	unsigned int	image_flag = 0;
	dfu_trigger_t 	*dfu = (dfu_trigger_t*) DFU_TRIGGER_MEMORY_ADDR ;
	uint32_t	 	image_ddr_base = 0;
	uint32_t		offset;
	uint32_t		data;
	int 			len;		
	T_I2C_WRITE		xucmd;
	int 			i;

	/* clear dfu_trigger */
	dfu->dfu_mode = 0;

	/* enter DFU_MODE state */
	dfu_state = DFU_MODE;

	/* do the slave reset here, so that we are ready to process the "get status" request that will be 
	   generated by the GPIO interrupt below (after CRC checks) */

	i2c_slave_reset(CFG_I2C_SLAVE);

	/* load uImage and cramfs from NAND to memory */
	rc = load_linux_image((unsigned int)LINUX_NAND_OFFSET, (unsigned char*)LINUX_DDR_LOCATION);
	rc = load_cramfs_image((unsigned int)CRAMFS_NAND_OFFSET, (unsigned char*)CRAMFS_DDR_LOCATION);
#if 0
	rc = check_linux_crc(LINUX_DDR_LOCATION);
	rc = check_cramfs_crc(CRAMFS_DDR_LOCATION);
#endif

	/* toggle GPIO to notify 2211 dm365 u-boot is ready to start fw updae */
	/* toggle GPIO API here */
	rc = sendWolverineIntStatus(hWolverineInterface, INDEX_OPERATION_STATUS, MODE_UBOOT_DFU_MODE);

	/* process */
	while ( DFU_IDLE != dfu_state ) {

		len = MAX_I2C_LENGTH;

		/* get XU command from 2211 */
		rc = i2c_slave_read( (uchar*) &xucmd, &len );
		if (rc || len < XU_COMMAND_MIN_LEN ) {
			printf("%s(): fail. rc=%d, len=%d; while reading xu cmd from i2c\n", __FUNCTION__, rc, len);
			if (rc == -2) {  // error: in slave-xmit mode when expecting to do a read
				printf("  Doing dummy write to get out of hang (i.e. unexpected slave-xmit mode)\n");
				/* do a dummy write, to get out of hang in slave-xmit mode */
				/*  (the master (2211) is "hung" in read state, and slave (365) is hung in slave-xmit mode */
				/* can reuse xucmd.data buffer,and len since botn xucmd.xxx and len contain junk now anyway */
				len = MAX_I2C_LENGTH - 5;
				/* fill with dummy data */
				for (i=0; i<len; i++) {
					xucmd.data[i] = 0xEE;
				}
				/* even though length is ~MAX, the stop bit from master will get it out of this write */
				rc = i2c_slave_write((unsigned char*)xucmd.data, &len);
			}
			/* reset i2c driver */
			/*i2c_slave_reset(0)*/;
			continue;
		}

		/* got XU command */
		/* convert offset */			
		offset = ntohl(xucmd.addr);
		len -= XU_COMMAND_MIN_LEN;

		if ( 0 == xucmd.readLength) {
			/* XU Set command */
			if ( offset >= 0xFFAA0000 ) {
				printf("XU_SET command: offset = 0x%x, readLength=%d, data_len=%d\n", (unsigned int)xucmd.addr, xucmd.readLength, len);

				/* XU command */
				rc = wolverineIntWriteBuffer(&xucmd, len);
				/* Show command and value */
			    memcpy(&data, xucmd.data, 4);
				printf("XU command = 0x%x, value=0x%x\n", offset, data);

				/* process XU command */
				rc = process_xu_command(&dfu_state, &image_flag, (unsigned long*)&image_ddr_base);
			} 
			else {
				if ( DFU_DATA == dfu_state ) {
					offset += image_ddr_base;
				}

#if 0
				/*debug crc checksum fail */
				if (*(unsigned char*)offset != xucmd.data[0]) {
					printf("%s(): xucmd.data[0]=0x%x fail. offset=0x%x, data_len=%d\n", __FUNCTION__, xucmd.data[0], offset, len );
				}
#endif 

#if 0
				/*debug crc checksum fail */
				if (0 !=memcmp((unsigned char*)offset, (unsigned char*)&xucmd.data, len)) {
					printf("%s(): memcmp fail. offset=0x%x, data_len=%d\n", __FUNCTION__, offset, len );
				}
#endif 
				/* copy data to memory as part of image download */
				memcpy((unsigned char*)offset, (unsigned char*)&xucmd.data, len);
				rc = 0;
			}
			if ( 0 != rc ) {
				printf("XU set command: fail to set! rc=%d.\n", rc);
			}
		}  /* end of XU Set command */
		else {
			/* XU Get command */
			if ( offset >= 0xFFAA0000 ) {
				/* get data from IWolverine interface or from system register to buffer */
				rc = wolverineIntReadBuffer(xucmd.data, xucmd.readLength, offset);
				printf("XU_Get_command done! 0x%x %x %x %x rc=%d.\n", xucmd.data[0], xucmd.data[1],xucmd.data[2],xucmd.data[3], rc);
			}
			else {
				/* get memory */
				memcpy((unsigned long*)&xucmd.data, (unsigned long*)offset, xucmd.readLength);
				rc = 0;
				printf("XU_Get from memory done!\n");
			}
			if ( 0 == rc ) {
				/* send get data back to host */
				len = xucmd.readLength;
				rc = i2c_slave_write((unsigned char*)xucmd.data, &len);
				if ( (rc) || (len != xucmd.readLength) ) {
				    printf("XU Get command: write to i2c fail: rc=%d, readLength=%d, len=%d\n", rc, xucmd.readLength, len);
					rc = -10;
				}
			}
			else
				rc = -10;

#if 0
			if ( (0 == rc) && (0xFFAA0000 == offset) && (DFU_MODE_DONE == dfu_state) ) {
				/* 2211 got DFU mode status, exit */
				dfu_state = DFU_IDLE;
			}
#endif
		}  /* end of XU Get command */
#if 0
		if (rc == -10) {
			/* reset i2c driver */
			i2c_slave_reset(0);
		}
#endif
	} /* end of while-loop */
	
	return rc;
}

static int check_dfu (void) 
{
	int rc = 1; /* not in dfu mode */
	uint32_t dfu_crc, crc;
	dfu_trigger_t *dfu = (dfu_trigger_t*) DFU_TRIGGER_MEMORY_ADDR;
	uchar *p = (uchar*) DFU_TRIGGER_MEMORY_ADDR;

	if ( DFU_MAGIC_NUMBER != dfu->dfu_magic ) {
#ifndef FAST_BOOT
		printf("DFU: bad magic number\n");
#endif
		return 1;
	}

	dfu_crc = dfu->dfu_crc;
	dfu->dfu_crc = 0;
	crc = crc32(0, p, sizeof(dfu_trigger_t));
	printf ("%s() crc=0x%x\n", __FUNCTION__, crc);

	if ( dfu_crc == crc) {
		/* valid checksum */
		if (dfu->dfu_mode) {
			rc = 0; /* in dfu mode */
		}
	}
	return rc;
}

/* u-boot commands */
int do_fw_update (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int ret = 1;

	if (argc != 1) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	ret = fw_update();
	if( 0 != ret) {
		printf("%s(): process failed. ret=%d\n", __FUNCTION__, ret);
	}
	else {
		printf("%s(): process success. ret=%d\n", __FUNCTION__, ret);
	}
	return ret;
}

/*U_BOOT_CMD(name,maxargs,repeatable,command,"usage","help")*/
U_BOOT_CMD(
	fw_update,    1,    1,     do_fw_update,
	"fw_update   - Wolverine firmware update via hard i2c (as slave)\n",
	NULL
);

/* return: 0 - in dfu mode
 *         != 0 - not in dfu mode
 */
int do_check_dfu (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int ret = 1; /* not in dfu mode */

	if (argc != 1) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return 1;
	}

	ret = check_dfu();
	if ( 0 != ret ) {
#ifndef FAST_BOOT
		printf("DFU trigger CRC checksum fail.\n");
#endif
	}
	else {
		printf("DFU trigger CRC checksum success. ret=%d\n", ret);
	}

	return ret;
}

U_BOOT_CMD(
	check_dfu,	1,	1,	do_check_dfu,
	"check_dfu  - check if it is in DFU mode\n",
	NULL
);
#endif /*ADVANCEV_FW_UPDATE*/

