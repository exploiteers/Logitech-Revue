/*
 * (C) Copyright 2008
 * Sandeep Paulraj <s-paulraj@ti.com>
 * Configuation settings for the TI DaVinci DM6467 EVM board.
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
 */

#ifndef __CONFIG_H
#define __CONFIG_H
#include <asm/sizes.h>

/* Chip Configurations */
/*=======================================================================*/
#define CFG_DM6467_EVM
#define CONFIG_ARM926EJS			/* arm926ejs CPU core */
#define CONFIG_SYS_CLK_FREQ	297000000	/* Arm Clock frequency */
#define CFG_TIMERBASE		0x01C21400	/* use timer 0 */
#define CFG_HZ_CLOCK		148500000	/* Timer Input clock freq */
#define CFG_HZ			1000
#define CFG_USE_NAND

/* #define CFG_PCI_BOOT */                      /* Build for PCI Booting  */
/*======================================================================*/

#ifdef CFG_PCI_BOOT
/* PCI Boot Info */
/*======================================================================*/

#define CONFIG_INITRD_TAG	(1)
#define CFG_ENV_IS_NOWHERE	(1)		/* PCI booting */
#define CFG_ENV_ADDR		(0x80000000)
#define CFG_ENV_SIZE		(64 * 2048)

#else
#ifdef CFG_USE_NAND
#define CFG_NAND_LARGEPAGE
#undef CFG_ENV_IS_IN_FLASH
#define CFG_NO_FLASH
#define CFG_ENV_IS_IN_NAND		/* U-Boot env in NAND Flash  */
#ifdef CFG_NAND_SMALLPAGE
#define CFG_ENV_SECT_SIZE	512	/* Env sector Size */
#define CFG_ENV_SIZE		SZ_16K
#else
#define CFG_ENV_SECT_SIZE	2048	/* Env sector Size */
#define CFG_ENV_SIZE		SZ_128K
#endif
#define CONFIG_SKIP_LOWLEVEL_INIT	/* U-Boot is loaded by a bootloader */
#define CONFIG_SKIP_RELOCATE_UBOOT	/* to a proper address, init done */
#define CFG_NAND_BASE		0x42000000
#define CFG_NAND_HW_ECC
#define CFG_MAX_NAND_DEVICE	1	/* Max number of NAND devices */
#define NAND_MAX_CHIPS		1
#define CFG_ENV_OFFSET		0x80000	/* Block 0--not used by bootcode */
#define DEF_BOOTM		""

#endif /* CFG_USE_NAND */
#endif /* CFG_PCI_BOOT */

/*
 * Memory Info 
 */
/*=======================================================================*/
#define CFG_MALLOC_LEN		(0x20000 + 128*1024)  /* malloc () len */
#define CFG_GBL_DATA_SIZE	128		/* reserved for initial data */
#define CFG_MEMTEST_START	0x82000000	/* memtest start address  */
#define CFG_MEMTEST_END		0x90000000	/* 16MB RAM test   	  */
#define CONFIG_NR_DRAM_BANKS	1		/* we have 1 bank of DRAM */
#define PHYS_SDRAM_1		0x80000000	/* DDR Start 		  */
#define PHYS_SDRAM_1_SIZE	0x10000000	/* DDR size 256MB 	  */
#define CONFIG_STACKSIZE	(256*1024)	/* regular stack	  */
/*==========================================================================*/

/*
 * Serial Driver info
 */
/*==========================================================================*/
#define CFG_NS16550			/* Include NS16550 as serial driver */
#define CFG_NS16550_SERIAL
#define CFG_NS16550_REG_SIZE 	4		/* NS16550 register size */
#define CFG_NS16550_COM1 	0X01C20000	/* Base address of UART0  */
#define CFG_NS16550_CLK 	24000000	/* Input clock to NS16550 */
#define CONFIG_CONS_INDEX	1		/* use UART0 for console  */
#define CONFIG_BAUDRATE		115200		/* Default baud rate      */
#define CFG_BAUDRATE_TABLE	{ 9600, 19200, 38400, 57600, 115200 }
/*========================================================================*/

/* U-Boot Configurations */
/*========================================================================*/

#undef 	CONFIG_USE_IRQ				/* we don't need IRQ/FIQ */
#define CONFIG_MISC_INIT_R
#define CONFIG_BOOTDELAY	4		/* Boot delay before OS boot*/
#define CONFIG_BOOTFILE		"uImage-dm6467"	/* file to load */
#define CFG_LONGHELP				/* undef to save memory     */
#define CFG_PROMPT	"DM6467 EVM # "		/* Monitor Command Prompt   */
#define CFG_CBSIZE	1024			/* Console I/O Buffer Size  */
#define CFG_PBSIZE	(CFG_CBSIZE+sizeof(CFG_PROMPT)+16) /* Print buffer  */
#define CFG_MAXARGS	16		/* max number of command args   */
#define CFG_BARGSIZE	CFG_CBSIZE	/* Boot Argument Buffer Size    */
#undef	CFG_CLKS_IN_HZ			/* Clock info are in HZ */
#define CFG_LOAD_ADDR	0x80700000	/* default load address of Linux */

/*
 *  I2C Configuration 
 */
#define CONFIG_HARD_I2C
#define CFG_I2C_SPEED 100000
#define CFG_I2C_SLAVE 10
#define CONFIG_DRIVER_DAVINCI_I2C

/* Linux Information */

#define LINUX_BOOT_PARAM_ADDR	0x80000100	/* Set the Boot location at
						 * the end of DDR
						 */
#define CONFIG_CMDLINE_TAG	  1	/* enable passing of ATAGs  */
#define CONFIG_SETUP_MEMORY_TAGS  1

/*=========================================================================*/

/*
 * Network & Ethernet Configuration
 */
/*=========================================================================*/
#define CONFIG_DRIVER_TI_EMAC

#define CONFIG_MII
#define CONFIG_BOOTP_DEFAULT
#define CONFIG_BOOTP_DNS
#define CONFIG_BOOTP_DNS2
#define CONFIG_BOOTP_SEND_HOSTNAME
#define CONFIG_NET_RETRY_COUNT	10

/* Required for EMAC address */
#define CFG_I2C_EEPROM_ADDR_LEN		2
#define CFG_I2C_EEPROM_ADDR		0x50
#define CFG_EEPROM_PAGE_WRITE_BITS	6
#define CFG_EEPROM_PAGE_WRITE_DELAY_MS	20

/*=========================================================================*/

#include <config_cmd_default.h>
#define CONFIG_CMD_ASKENV
#define CONFIG_CMD_DHCP
#define CONFIG_CMD_DIAG
#define CONFIG_CMD_I2C
#define CONFIG_CMD_MII
#define CONFIG_CMD_PING
#define CONFIG_CMD_SAVES
#define CONFIG_CMD_EEPROM
#undef CONFIG_CMD_BDI
#undef CONFIG_CMD_FPGA
#undef CONFIG_CMD_SETGETDCR
#ifdef CFG_USE_NAND
#define CFG_NO_FLASH
#undef CONFIG_CMD_FLASH
#undef CONFIG_CMD_IMLS
#define CONFIG_CMD_NAND
#elif defined(CFG_PCI_BOOT)
#else
#error "Either CFG_USE_NAND or CFG_PCI_BOOT MUST be defined !!!"
#endif

/* KGDB support */
/*=========================================================================*/
#ifdef CONFIG_CMD_KGDB
#define CONFIG_KGDB_BAUDRATE	115200	/* speed to run kgdb serial port */
#define CONFIG_KGDB_SER_INDEX	1	/* which serial port to use */
#endif
#endif /* __CONFIG_H */

