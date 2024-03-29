/*
 * Copyright (C) 2008 Texas Instruments, Inc <www.ti.com>
 * 
 * Based on davinci_dvevm.h. Original Copyrights follow:
 *
 * Copyright (C) 2007 Sergey Kubushyn <ksi@koi8.net>
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
#define  CFG_USE_SPIFLASH

/*===================*/
/* SoC Configuration */
/*===================*/
#define CONFIG_ARM926EJS				/* arm926ejs CPU core */
#define CONFIG_DA8XX					/* TI DA8xx SoC */
#define CONFIG_SYS_CLK_FREQ	clk_get(DAVINCI_ARM_CLKID)	/* Arm Clock */
#define CFG_OSCIN_FREQ		24000000
#define CFG_TIMERBASE		DAVINCI_TIMER0_BASE	/* use timer 0 */
#define CFG_HZ_CLOCK		clk_get(DAVINCI_AUXCLK_CLKID) /* Timer Input clock freq */
#define CFG_HZ				1000
#undef CONFIG_SKIP_LOWLEVEL_INIT	/* U-Boot is _always_ loaded by a bootloader */
#define CONFIG_SKIP_RELOCATE_UBOOT	/* to a proper address, init done */

/*=============*/
/* Memory Info */
/*=============*/
#define CFG_MALLOC_LEN		(0x10000 + 128*1024)	/* malloc() len */
#define CFG_GBL_DATA_SIZE	128		/* reserved for initial data */
#define PHYS_SDRAM_1		DAVINCI_DDR_EMIF_DATA_BASE	/* DDR Start */
#define PHYS_SDRAM_1_SIZE	0x04000000	/* SDRAM size 64MB */
#define CFG_MEMTEST_START	PHYS_SDRAM_1	/* memtest start address */
#define CFG_MEMTEST_END		(PHYS_SDRAM_1 + 16*1024*1024)	/* 16MB RAM test */
#define CONFIG_NR_DRAM_BANKS	1		/* we have 1 bank of DRAM */
#define CONFIG_STACKSIZE	(256*1024)	/* regular stack */
#define SDRAM_4BANKS_10COLS				/* TODO: Update this! */

/*====================*/
/* Serial Driver info */
/*====================*/
#define CFG_NS16550
#define CFG_NS16550_SERIAL
#define CFG_NS16550_REG_SIZE	4		/* NS16550 register size */
#define CFG_NS16550_COM1	DAVINCI_UART2_BASE	/* Base address of UART2 */
#define CFG_NS16550_CLK clk_get(DAVINCI_UART2_CLKID) /*	Input clock to NS16550 */
#define CONFIG_CONS_INDEX	1			/* use UART0 for console */
#define CONFIG_BAUDRATE		115200		/* Default baud rate */
#define CFG_BAUDRATE_TABLE	{ 9600, 19200, 38400, 57600, 115200 }

/*===================*/
/* I2C Configuration */
/*===================*/
#define CONFIG_HARD_I2C
#define CONFIG_DRIVER_DAVINCI_I2C
#define CFG_I2C_SPEED           25000   /* 100Kbps won't work, silicon bug */
#define CFG_I2C_SLAVE           10      /* Bogus, master-only in U-Boot */

/*====================================================*/
/* I2C EEPROM definitions for catalyst 24W256 EEPROM chip */
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
#define CFG_ENV_SIZE		SZ_128K
#define CFG_NAND_1BIT_ECC
#define CFG_NAND_CS		3
#define CFG_NAND_BASE		DAVINCI_ASYNC_EMIF_DATA_CE3_BASE
#define CFG_CLE_MASK		0x10
#define CFG_ALE_MASK		0x8
#define CFG_NAND_HW_ECC
#define CFG_MAX_NAND_DEVICE	1	/* Max number of NAND devices */
#define NAND_MAX_CHIPS		1
#define CFG_ENV_OFFSET		0x0	/* Block 0--not used by bootcode */
#define DEF_BOOTM		""
#endif

#ifdef CFG_USE_NOR
#define CFG_ENV_IS_IN_FLASH
#undef CFG_NO_FLASH
#define CFG_FLASH_CFI_DRIVER
#define CFG_FLASH_CFI
#define CFG_MAX_FLASH_BANKS	1		/* max number of flash banks */
#define CFG_FLASH_SECT_SZ	0x10000		/* 64KB sect size AMD Flash */
#define CFG_ENV_OFFSET		(CFG_FLASH_SECT_SZ*3)
#define CFG_FLASH_BASE		DAVINCI_ASYNC_EMIF_DATA_CE2_BASE 
#define PHYS_FLASH_SIZE		0x2000000	/* Flash size 32MB 	 */
#define CFG_MAX_FLASH_SECT	(PHYS_FLASH_SIZE/CFG_FLASH_SECT_SZ)
#define CFG_ENV_SECT_SIZE	CFG_FLASH_SECT_SZ	/* Env sector Size */
#endif

#ifdef CFG_USE_SPIFLASH
#undef CFG_ENV_IS_IN_FLASH
#undef CFG_ENV_IS_IN_NAND
#define CFG_ENV_IS_IN_SPI_FLASH
#define CFG_ENV_SIZE		SZ_16K
#define CFG_ENV_OFFSET		SZ_128K
#define CFG_NO_FLASH
#define CONFIG_SPI
#define CONFIG_SPI_FLASH
#define CONFIG_SPI_FLASH_WINBOND
#define CONFIG_DAVINCI_SPI
#define CFG_SPI_BASE DAVINCI_SPI0_BASE
#define CFG_SPI_CLK clk_get(DAVINCI_SPI0_CLKID)
#define CONFIG_SF_DEFAULT_SPEED 50000000
#define CFG_ENV_SPI_MAX_HZ CONFIG_SF_DEFAULT_SPEED
#endif


/*==============================*/
/* U-Boot general configuration */
/*==============================*/
#undef 	CONFIG_USE_IRQ			/* No IRQ/FIQ in U-Boot */
#define CONFIG_MISC_INIT_R
#undef CONFIG_BOOTDELAY
#define CONFIG_BOOTFILE		"uImage"	/* Boot file name */
#define CFG_PROMPT		"U-Boot > "	/* Monitor Command Prompt */
#define CFG_CBSIZE		1024		/* Console I/O Buffer Size  */
#define CFG_PBSIZE		(CFG_CBSIZE+sizeof(CFG_PROMPT)+16)	/* Print buffer sz */
#define CFG_MAXARGS		16		/* max number of command args */
#define CFG_BARGSIZE		CFG_CBSIZE	/* Boot Argument Buffer Size */
#define CFG_LOAD_ADDR		(CFG_MEMTEST_START + 0x700000)	/* default Linux kernel load address */
#define CONFIG_VERSION_VARIABLE
#define CONFIG_AUTO_COMPLETE		/* Won't work with hush so far, may be later */
#define CFG_HUSH_PARSER
#define CFG_PROMPT_HUSH_PS2	"> "
#define CONFIG_CMDLINE_EDITING
#define CFG_LONGHELP
#define CONFIG_CRC32_VERIFY
#define CONFIG_MX_CYCLIC

/*===================*/
/* Linux Information */
/*===================*/
#define LINUX_BOOT_PARAM_ADDR	(CFG_MEMTEST_START + 0x100)
#define CONFIG_CMDLINE_TAG
#define CONFIG_SETUP_MEMORY_TAGS
#define CONFIG_BOOTARGS		"mem=32M console=ttyS2,115200n8 root=/dev/mtdblock/2 rw noinitrd ip=dhcp"
#define CONFIG_BOOTCOMMAND	""

/*=================*/
/* U-Boot commands */
/*=================*/
#include <config_cmd_default.h>
#define CONFIG_CMD_ENV
#define CONFIG_CMD_ASKENV
#define CONFIG_CMD_DHCP
#define CONFIG_CMD_DIAG
#define CONFIG_CMD_MII
#define CONFIG_CMD_PING
#define CONFIG_CMD_SAVES
#define CONFIG_CMD_MEMORY
#undef CONFIG_CMD_BDI
#undef CONFIG_CMD_FPGA
#undef CONFIG_CMD_SETGETDCR
#define CONFIG_CMD_EEPROM

#ifdef CFG_USE_NAND
#undef CONFIG_CMD_FLASH
#undef CONFIG_CMD_IMLS
#define CONFIG_CMD_NAND
#endif

#ifdef CFG_USE_SPIFLASH
#undef CONFIG_CMD_IMLS
#undef CONFIG_CMD_FLASH
#define CONFIG_CMD_SF
#endif

#if !defined(CFG_USE_NAND) && !defined(CFG_USE_NOR) && !defined(CFG_USE_SPIFLASH)
#define CFG_ENV_IS_NOWHERE
#define CFG_NO_FLASH
#define CFG_ENV_SIZE		SZ_16K
#undef CONFIG_CMD_IMLS
#undef CONFIG_CMD_FLASH
#undef CONFIG_CMD_ENV
#endif

#endif /* __CONFIG_H */
