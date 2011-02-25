/*
 * DaVinci SPI-EEPROM client driver header file
 *
 * Author: Steve Chen <schen@mvista.com>
 *
 * 2008 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#ifndef MTD_SPI_FLASH_H
#define MTD_SPI_FLASH_H

#include <linux/cache.h>
/*
 *  op-codes
 */
#define MTD_SPI_FLASH_WREN	0x06	/* write enable */
#define MTD_SPI_FLASH_WRDIS	0x04	/* write disable */
#define MTD_SPI_FLASH_WRSTAT	0x01	/* write status register */
#define MTD_SPI_FLASH_WRITE	0x02	/* write/page program */

#define MTD_SPI_FLASH_RDSTAT	0x05	/* read status register */
#define MTD_SPI_FLASH_RD	0x03	/* read data */
#define MTD_SPI_FLASH_FRD	0x0B	/* fast read */
#define MTD_SPI_FLASH_FRDDO	0x3B	/* fast read dual output */

#define MTD_SPI_FLASH_SECERA	0x20	/* sector erase */
#define MTD_SPI_FLASH_BKERA	0xD8	/* block erase */
#define MTD_SPI_FLASH_CHPERA	0xC7	/* chip erase */

#define MTD_SPI_FLASH_PWRDN	0xB8	/* power down */
#define MTD_SPI_FLASH_RELPWRDN	0xAB	/* release power down/ device id */

#define MTD_SPI_FLASH_MFRID	0x90	/* manufacture ID */
#define MTD_SPI_FLASH_JEDECID	0x9F	/* JEDEC ID */

/*
 *  Manufacture ID
 */
#define SPI_FLASH_MFR_WINBOND	0xEF

/*
 *  Device ID
 */
#define SPI_FLASH_W25X16	0x14
#define SPI_FLASH_W25X32	0x15
#define SPI_FLASH_W25X64	0x16

/*
 * Status register definitions
 */
#define SPI_FLASH_STAT_BUSY	0x1	/* read/write in progress */
#define SPI_FLASH_STAT_WEL	0x2	/* Write Enable Latch */

#define SPI_FLASH_BUFFER_SIZE 256
#define SPI_FLASH_CMD_SIZE 	4

#define MTD_SPI_FLASH_NAME "spi_flash"

struct mtd_partition;

struct mtd_spi_flash_info {
	char *name;
	unsigned int page_size;
	unsigned int page_mask;
	unsigned int sector_erase_size;
	unsigned int block_erase_size;
	unsigned long chip_sel;
	unsigned int commit_delay;
	struct spi_device *spi;
	struct davinci_spi *spi_data;
	void *controller_data;

	struct mtd_info mtd;

	struct mtd_partition *parts;
	unsigned int nr_parts;

	struct mutex lock;
	char tx_buffer[SPI_FLASH_BUFFER_SIZE + SPI_FLASH_CMD_SIZE];
	char rx_buffer[SPI_FLASH_BUFFER_SIZE + 1];
};

#endif	/* MTD_SPI_FLASH_H */
