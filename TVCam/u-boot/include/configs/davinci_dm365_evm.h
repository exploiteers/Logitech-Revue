/*
 * (C) Copyright 2008
 * Texas Instruments Inc.
 * Configuation settings for the TI Davinci DM355 EVM board.
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

/*=======*/
/* Board */
/*=======*/
#define CFG_DM365_EVM
#define CFG_USE_NAND
#define CFG_DAVINCI_STD_NAND_LAYOUT
#define CONFIG_ENV_OVERWRITE

/*===================*/
/* SoC Configuration */
/*===================*/
#define CONFIG_ARM926EJS			/* arm926ejs CPU core */
#define CONFIG_SYS_CLK_FREQ	297000000	/* Arm Clock frequency */
#define CFG_TIMERBASE		0x01C21400	/* use timer 0 */
#define CFG_HZ_CLOCK		24000000	/* Timer Input clock freq */	
#define CFG_HZ			1000
/*=============*/
/* Memory Info */
/*=============*/
#define CFG_MALLOC_LEN		(0x40000 + 128*1024)  /* malloc () len */
#define CFG_GBL_DATA_SIZE	128		/* reserved for initial data */
#define CFG_MEMTEST_START	0x82000000	/* memtest start address  */
#define CFG_MEMTEST_END		0x90000000	/* 16MB RAM test   	  */
#define CONFIG_NR_DRAM_BANKS	1		/* we have 1 bank of DRAM */
#define CONFIG_STACKSIZE	(256*1024)	/* regular stack	  */
#define PHYS_SDRAM_1		0x80000000	/* DDR Start 		  */
#define PHYS_SDRAM_1_SIZE	0x8000000	/* DDR size 128MB 	  */

/*====================*/
/* Serial Driver info */
/*====================*/
#define CFG_NS16550
#define CFG_NS16550_SERIAL
#define CFG_NS16550_REG_SIZE	4		/* NS16550 register size */
#define CFG_NS16550_COM1	0x01C20000	/* Base address of UART0  */
#define CFG_NS16550_CLK		24000000	/* Input clock to NS16550 */
#define CONFIG_CONS_INDEX	1		/* use UART0 for console  */
#define CONFIG_BAUDRATE		115200		/* Default baud rate      */
#define CFG_BAUDRATE_TABLE	{ 9600, 19200, 38400, 57600, 115200 }

/*===================*/
/* I2C Configuration */
/*===================*/
#define CONFIG_HARD_I2C
#define CONFIG_HARD_I2C_SLAVE
#define CONFIG_DRIVER_DAVINCI_I2C
#define CFG_I2C_SPEED		100000	/* 100Kbps */
/*#define CFG_I2C_SLAVE		10*/	/* Bogus, master-only in U-Boot */
/* 2009-12-22: Hard I2C slave address is 0x44 */
#define CFG_I2C_SLAVE		0x44	

#define CONFIG_SOFT_I2C
#ifdef CONFIG_SOFT_I2C
#ifndef DAVINCI_GPIO_BASE
#define DAVINCI_GPIO_BASE	(0x01C67000)
#endif
#define DIR45			0x60
#define OUT_DATA45		0x64
#define SET_DATA45		0x68
#define CLR_DATA45		0x6C
#define IN_DATA45		0x70
#define S_SCL			0x01000000 	/* GIO88 */
#define S_SDA			0x10000000	/* GIO92 */
#define I2C_INIT		(*(volatile unsigned int *)(DAVINCI_GPIO_BASE + DIR45) |=  S_SCL)
#define I2C_ACTIVE		(*(volatile unsigned int *)(DAVINCI_GPIO_BASE + DIR45) &=  ~S_SDA)
#define I2C_TRISTATE		(*(volatile unsigned int *)(DAVINCI_GPIO_BASE + DIR45) |=  S_SDA)
#define I2C_READ		((*(volatile unsigned int *)(DAVINCI_GPIO_BASE + IN_DATA45) & S_SDA) != 0)
#define I2C_SDA(bit)		if(bit) { \
					(*(volatile unsigned int *) (DAVINCI_GPIO_BASE + DIR45) |=  S_SDA);	\
				} else { \
					(*(volatile unsigned int *) (DAVINCI_GPIO_BASE + CLR_DATA45) = S_SDA);	\
					(*(volatile unsigned int *) (DAVINCI_GPIO_BASE + DIR45) &= ~S_SDA);	\
				}
#define I2C_SCL(bit)    	if(bit) { \
					(*(volatile unsigned int *) (DAVINCI_GPIO_BASE + DIR45) |=  S_SCL);	\
				} else { \
					(*(volatile unsigned int *) (DAVINCI_GPIO_BASE + CLR_DATA45) = S_SCL);	\
					(*(volatile unsigned int *) (DAVINCI_GPIO_BASE + DIR45) &= ~S_SCL);	\
				}
#define I2C_DELAY		udelay(5)
#endif

/*====================================================*/
#define CFG_I2C_EEPROM_ADDR_LEN		2
#define CFG_I2C_EEPROM_ADDR		0x50
#define CFG_EEPROM_PAGE_WRITE_BITS	6
#define CFG_EEPROM_PAGE_WRITE_DELAY_MS	20

/*==================================*/
/* Network & Ethernet Configuration */
/*==================================*/
#define CONFIG_DRIVER_TI_EMAC
#define CONFIG_MII
#define CONFIG_BOOTP_DEFAULT
#define CONFIG_BOOTP_DNS
#define CONFIG_BOOTP_DNS2
#define CONFIG_BOOTP_SEND_HOSTNAME
#define CONFIG_NET_RETRY_COUNT	10

/*=====================*/
/* Flash & Environment */
/*=====================*/
#ifdef CFG_USE_NAND
#undef CFG_ENV_IS_IN_FLASH
#define CFG_NO_FLASH
#define CFG_ENV_IS_IN_NAND		/* U-Boot env in NAND Flash  */
#define CFG_ENV_SECT_SIZE	0x40000
#define CFG_ENV_SIZE		0x800 //wing SZ_256K
#define CONFIG_SKIP_LOWLEVEL_INIT	/* U-Boot is loaded by a bootloader */
#define CONFIG_SKIP_RELOCATE_UBOOT	/* to a proper address, init done */
#define CFG_NAND_BASE		0x02000000
#define CFG_NAND_4BIT_ECC
//#define CFG_NAND_LARGEPAGE
#define CFG_NAND_HW_ECC
#define CFG_MAX_NAND_DEVICE	2	/* Max number of NAND devices */
#define CFG_ENV_OFFSET		0x3C0000	/* environment starts here  */
#define CFG_NAND_BASE_LIST      {CFG_NAND_BASE, CFG_NAND_BASE+0x4000}
#endif

/*==============================*/
 /* U-Boot general configuration */
/*==============================*/
#undef	CONFIG_USE_IRQ			/* No IRQ/FIQ in U-Boot */
#define CONFIG_MISC_INIT_R
#define CONFIG_BOOTDELAY        0
#define CONFIG_ZERO_BOOTDELAY_CHECK
#define CONFIG_BOOTFILE		"uImage"	/* Boot file name */
#define CFG_PROMPT		"DM365 EVM :>"	/* Monitor Command Prompt */
#define CFG_CBSIZE		1024		/* Console I/O Buffer Size  */
#define CFG_PBSIZE		(CFG_CBSIZE+sizeof(CFG_PROMPT) + 16)	/* Print buffer sz */
#define CFG_MAXARGS		16		/* max number of command args */
#define CFG_BARGSIZE		CFG_CBSIZE	/* Boot Argument Buffer Size */
#define CFG_LOAD_ADDR		0x80700000	/* default Linux kernel load address */
#define CONFIG_VERSION_VARIABLE
#define CONFIG_CMDLINE_EDITING

/*===================*/
/* Linux Information */
/*===================*/
#define LINUX_BOOT_PARAM_ADDR	0x80000100
#define CONFIG_CMDLINE_TAG
#define CONFIG_SETUP_MEMORY_TAGS
//#define CONFIG_BOOTARGS		"mem=116M console=ttyS0,115200n8 root=/dev/ram0 rw initrd=0x82000000,4M ip=dhcp"
//#define CONFIG_BOOTARGS		"console=ttyS0,115200n8 ip=none root=/dev/mtdblock3 ro rootfstype=cramfs mem=76M video=davincifb:vid0=OFF:vid1=OFF:osd0=720x576,4050K dm365_imp.oper_mode=0 davinci_capture.device_type=4"
#define CONFIG_BOOTARGS		"mem=60M console=ttyS0,115200n8 ip=off root=/dev/mtdblock3 ro rootfstype=cramfs video=davincifb:vid0=720x576x16,2500K:vid1=720x576x16,2500K:osd0=720x576x16,2025K init=/init quiet"
//#define CONFIG_BOOTCOMMAND	"setenv setboot setenv bootargs \\$(bootargs) video=dm36x:output=\\$(videostd);run setboot; bootm 0x2050000"
#define CONFIG_BOOTCOMMAND	"nboot 0x80700000 0 0x400000;bootm"

/*=================*/
/* U-Boot commands */
/*=================*/
#include <config_cmd_default.h>
#define CONFIG_CMD_ASKENV
#define CONFIG_CMD_DHCP
#define CONFIG_CMD_DIAG
#define CONFIG_CMD_I2C
#define CONFIG_CMD_MII
#define CONFIG_CMD_PING
#define CONFIG_CMD_SAVES
#undef CONFIG_CMD_EEPROM
#undef CONFIG_CMD_FLASH
#undef CONFIG_CMD_BDI
#undef CONFIG_CMD_FPGA
#undef CONFIG_CMD_SETGETDCR
#undef CONFIG_CMD_FLASH
#undef CONFIG_CMD_IMLS
#define CONFIG_CMD_NAND
#define CONFIG_CMD_JFFS2
#ifdef ADVANCEV_FW_UPDATE
#define CONFIG_CMD_FW_UPDATE
#endif

/* KGDB support (if any) */
/*=======================*/
#ifdef CONFIG_CMD_KGDB
#define CONFIG_KGDB_BAUDRATE	115200	/* speed to run kgdb serial port */
#define CONFIG_KGDB_SER_INDEX	1	/* which serial port to use */
#endif
#endif /* __CONFIG_H */


