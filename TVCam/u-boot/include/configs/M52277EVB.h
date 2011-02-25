/*
 * Configuation settings for the Freescale MCF52277 EVB board.
 *
 * Copyright (C) 2004-2007 Freescale Semiconductor, Inc.
 * TsiChung Liew (Tsi-Chung.Liew@freescale.com)
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

/*
 * board/config.h - configuration options, board specific
 */

#ifndef _M52277EVB_H
#define _M52277EVB_H

/*
 * High Level Configuration Options
 * (easy to change)
 */
#define CONFIG_MCF5227x		/* define processor family */
#define CONFIG_M52277		/* define processor type */
#define CONFIG_M52277EVB	/* M52277EVB board */

#define CONFIG_MCFUART
#define CFG_UART_PORT		(0)
#define CONFIG_BAUDRATE		115200
#define CFG_BAUDRATE_TABLE	{ 9600 , 19200 , 38400 , 57600, 115200 }

#undef CONFIG_WATCHDOG

#define CONFIG_TIMESTAMP	/* Print image info with timestamp */

/*
 * BOOTP options
 */
#define CONFIG_BOOTP_BOOTFILESIZE
#define CONFIG_BOOTP_BOOTPATH
#define CONFIG_BOOTP_GATEWAY
#define CONFIG_BOOTP_HOSTNAME

/* Command line configuration */
#include <config_cmd_default.h>

#define CONFIG_CMD_CACHE
#define CONFIG_CMD_DATE
#define CONFIG_CMD_ELF
#define CONFIG_CMD_FLASH
#define CONFIG_CMD_I2C
#define CONFIG_CMD_JFFS2
#define CONFIG_CMD_LOADB
#define CONFIG_CMD_LOADS
#define CONFIG_CMD_MEMORY
#define CONFIG_CMD_MISC
#undef CONFIG_CMD_NET
#define CONFIG_CMD_REGINFO
#undef CONFIG_CMD_USB
#undef CONFIG_CMD_BMP

#define CONFIG_HOSTNAME		M52277EVB
#define CONFIG_EXTRA_ENV_SETTINGS		\
	"inpclk=" MK_STR(CFG_INPUT_CLKSRC) "\0"	\
	"loadaddr=" MK_STR(CFG_LOAD_ADDR) "\0"	\
	"u-boot=u-boot.bin\0"			\
	"load=tftp ${loadaddr) ${u-boot}\0"	\
	"upd=run load; run prog\0"		\
	"prog=prot off 0 0x3ffff;"		\
	"era 0 3ffff;"				\
	"cp.b ${loadaddr} 0 ${filesize};"	\
	"save\0"				\
	""

#define CONFIG_BOOTDELAY	3	/* autoboot after 3 seconds */
/* LCD */
#ifdef CONFIG_CMD_BMP
#define CONFIG_LCD
#define CONFIG_SPLASH_SCREEN
#define CONFIG_LCD_LOGO
#define CONFIG_SHARP_LQ035Q7DH06
#endif

/* USB */
#ifdef CONFIG_CMD_USB
#define CONFIG_USB_EHCI
#define CONFIG_USB_STORAGE
#define CONFIG_DOS_PARTITION
#define CONFIG_MAC_PARTITION
#define CONFIG_ISO_PARTITION
#define CFG_USB_EHCI_REGS_BASE		0xFC0B0000
#define CFG_USB_EHCI_CPU_INIT
#endif

/* Realtime clock */
#define CONFIG_MCFRTC
#undef RTC_DEBUG
#define CFG_RTC_OSCILLATOR	(32 * CFG_HZ)

/* Timer */
#define CONFIG_MCFTMR
#undef CONFIG_MCFPIT

/* I2c */
#define CONFIG_FSL_I2C
#define CONFIG_HARD_I2C		/* I2C with hardware support */
#undef	CONFIG_SOFT_I2C		/* I2C bit-banged               */
#define CFG_I2C_SPEED		80000	/* I2C speed and slave address  */
#define CFG_I2C_SLAVE		0x7F
#define CFG_I2C_OFFSET		0x58000
#define CFG_IMMR		CFG_MBAR

/* Input, PCI, Flexbus, and VCO */
#define CONFIG_EXTRA_CLOCK

#define CFG_INPUT_CLKSRC	16000000

#define CONFIG_PRAM		512	/* 512 KB */

#define CFG_PROMPT		"-> "
#define CFG_LONGHELP		/* undef to save memory */

#if defined(CONFIG_CMD_KGDB)
#define CFG_CBSIZE		1024	/* Console I/O Buffer Size */
#else
#define CFG_CBSIZE		256	/* Console I/O Buffer Size */
#endif
#define CFG_PBSIZE		(CFG_CBSIZE+sizeof(CFG_PROMPT)+16)	/* Print Buffer Size */
#define CFG_MAXARGS		16	/* max number of command args */
#define CFG_BARGSIZE		CFG_CBSIZE	/* Boot Argument Buffer Size    */

#define CFG_LOAD_ADDR		(CFG_SDRAM_BASE + 0x10000)

#define CFG_HZ			1000

#define CFG_MBAR		0xFC000000

/*
 * Low Level Configuration Settings
 * (address mappings, register initial values, etc.)
 * You should know what you are doing if you make changes here.
 */

/*-----------------------------------------------------------------------
 * Definitions for initial stack pointer and data area (in DPRAM)
 */
#define CFG_INIT_RAM_ADDR	0x80000000
#define CFG_INIT_RAM_END	0x8000	/* End of used area in internal SRAM */
#define CFG_INIT_RAM_CTRL	0x21
#define CFG_GBL_DATA_SIZE	128	/* size in bytes reserved for initial data */
#define CFG_GBL_DATA_OFFSET	((CFG_INIT_RAM_END - CFG_GBL_DATA_SIZE) - 16)
#define CFG_INIT_SP_OFFSET	CFG_GBL_DATA_OFFSET

/*-----------------------------------------------------------------------
 * Start addresses for the final memory configuration
 * (Set up by the startup code)
 * Please note that CFG_SDRAM_BASE _must_ start at 0
 */
#define CFG_SDRAM_BASE		0x40000000
#define CFG_SDRAM_SIZE		64	/* SDRAM size in MB */
#define CFG_SDRAM_CFG1		0x43711630
#define CFG_SDRAM_CFG2		0x56670000
#define CFG_SDRAM_CTRL		0xE1092000
#define CFG_SDRAM_EMOD		0x81810000
#define CFG_SDRAM_MODE		0x00CD0000

#define CFG_MEMTEST_START	CFG_SDRAM_BASE + 0x400
#define CFG_MEMTEST_END		((CFG_SDRAM_SIZE - 3) << 20)

#define CFG_MONITOR_BASE	(CFG_FLASH_BASE + 0x400)
#define CFG_BOOTPARAMS_LEN	64*1024
#define CFG_MONITOR_LEN		(256 << 10)	/* Reserve 256 kB for Monitor */
#define CFG_MALLOC_LEN		(128 << 10)	/* Reserve 128 kB for malloc() */

/* Initial Memory map for Linux */
#define CFG_BOOTMAPSZ		(CFG_SDRAM_BASE + (CFG_SDRAM_SIZE << 20))

/* Configuration for environment
 * Environment is embedded in u-boot in the second sector of the flash
 */
#define CFG_ENV_IS_IN_FLASH	1
#define CONFIG_ENV_OVERWRITE	1
#undef CFG_ENV_IS_EMBEDDED

/*-----------------------------------------------------------------------
 * FLASH organization
 */
#define CFG_FLASH_BASE		CFG_CS0_BASE
#define CFG_FLASH0_BASE		CFG_CS0_BASE
#define CFG_ENV_ADDR		(CFG_FLASH_BASE + 0x8000)
#define CFG_ENV_SECT_SIZE	0x8000

#define CFG_FLASH_CFI
#ifdef CFG_FLASH_CFI

#	define CONFIG_FLASH_CFI_DRIVER	1
#	define CFG_FLASH_SIZE		0x1000000	/* Max size that the board might have */
#	define CFG_FLASH_CFI_WIDTH	FLASH_CFI_16BIT
#	define CFG_MAX_FLASH_BANKS	1	/* max number of memory banks */
#	define CFG_MAX_FLASH_SECT	137	/* max number of sectors on one chip */
#	define CFG_FLASH_PROTECTION	/* "Real" (hardware) sectors protection */
#	define CFG_FLASH_CHECKSUM
#endif

/*
 * This is setting for JFFS2 support in u-boot.
 * NOTE: Enable CONFIG_CMD_JFFS2 for JFFS2 support.
 */
#ifdef CONFIG_CMD_JFFS2
#	define CONFIG_JFFS2_DEV		"nor0"
#	define CONFIG_JFFS2_PART_SIZE	(0x01000000 - 0x40000)
#	define CONFIG_JFFS2_PART_OFFSET	(CFG_FLASH0_BASE + 0x40000)
#endif

/*-----------------------------------------------------------------------
 * Cache Configuration
 */
#define CFG_CACHELINE_SIZE		16

/*-----------------------------------------------------------------------
 * Memory bank definitions
 */
/*
 * CS0 - NOR Flash
 * CS1 - Available
 * CS2 - Available
 * CS3 - Available
 * CS4 - Available
 * CS5 - Available
 */

#define CFG_CS0_BASE		0x00000000
#define CFG_CS0_MASK		0x00FF0001
#define CFG_CS0_CTRL		0x00001FA0

#endif				/* _M52277EVB_H */
