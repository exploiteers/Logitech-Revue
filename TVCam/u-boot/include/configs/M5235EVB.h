/*
 * Configuation settings for the Freescale MCF5329 FireEngine board.
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

#ifndef _M5235EVB_H
#define _M5235EVB_H

/*
 * High Level Configuration Options
 * (easy to change)
 */
#define CONFIG_MCF523x		/* define processor family */
#define CONFIG_M5235		/* define processor type */

#define CONFIG_MCFUART
#define CFG_UART_PORT		(0)
#define CONFIG_BAUDRATE		115200
#define CFG_BAUDRATE_TABLE	{ 9600 , 19200 , 38400 , 57600, 115200 }

#undef CONFIG_WATCHDOG
#define CONFIG_WATCHDOG_TIMEOUT	5000	/* timeout in milliseconds, max timeout is 6.71sec */

/*
 * BOOTP options
 */
#define CONFIG_BOOTP_BOOTFILESIZE
#define CONFIG_BOOTP_BOOTPATH
#define CONFIG_BOOTP_GATEWAY
#define CONFIG_BOOTP_HOSTNAME

/* Command line configuration */
#include <config_cmd_default.h>

#define CONFIG_CMD_BOOTD
#define CONFIG_CMD_CACHE
#define CONFIG_CMD_DHCP
#define CONFIG_CMD_ELF
#define CONFIG_CMD_FLASH
#define CONFIG_CMD_I2C
#define CONFIG_CMD_MEMORY
#define CONFIG_CMD_MISC
#define CONFIG_CMD_MII
#define CONFIG_CMD_NET
#define CONFIG_CMD_PCI
#define CONFIG_CMD_PING
#define CONFIG_CMD_REGINFO

#undef CONFIG_CMD_LOADB
#undef CONFIG_CMD_LOADS

#define CONFIG_MCFFEC
#ifdef CONFIG_MCFFEC
#	define CONFIG_NET_MULTI		1
#	define CONFIG_MII		1
#	define CONFIG_MII_INIT		1
#	define CFG_DISCOVER_PHY
#	define CFG_RX_ETH_BUFFER	8
#	define CFG_FAULT_ECHO_LINK_DOWN

#	define CFG_FEC0_PINMUX		0
#	define CFG_FEC0_MIIBASE		CFG_FEC0_IOBASE
#	define MCFFEC_TOUT_LOOP		50000
/* If CFG_DISCOVER_PHY is not defined - hardcoded */
#	ifndef CFG_DISCOVER_PHY
#		define FECDUPLEX	FULL
#		define FECSPEED		_100BASET
#	else
#		ifndef CFG_FAULT_ECHO_LINK_DOWN
#			define CFG_FAULT_ECHO_LINK_DOWN
#		endif
#	endif			/* CFG_DISCOVER_PHY */
#endif

/* Timer */
#define CONFIG_MCFTMR
#undef CONFIG_MCFPIT

/* I2C */
#define CONFIG_FSL_I2C
#define CONFIG_HARD_I2C			/* I2C with hw support */
#undef CONFIG_SOFT_I2C			/* I2C bit-banged */
#define CFG_I2C_SPEED		80000
#define CFG_I2C_SLAVE		0x7F
#define CFG_I2C_OFFSET		0x00000300
#define CFG_IMMR		CFG_MBAR

/* this must be included AFTER the definition of CONFIG_COMMANDS (if any) */
#define CONFIG_BOOTDELAY	1	/* autoboot after 5 seconds */
#define CONFIG_BOOTFILE		"u-boot.bin"
#ifdef CONFIG_MCFFEC
#	define CONFIG_ETHADDR	00:e0:0c:bc:e5:60
#	define CONFIG_IPADDR	192.162.1.2
#	define CONFIG_NETMASK	255.255.255.0
#	define CONFIG_SERVERIP	192.162.1.1
#	define CONFIG_GATEWAYIP	192.162.1.1
#	define CONFIG_OVERWRITE_ETHADDR_ONCE
#endif				/* FEC_ENET */

#define CONFIG_HOSTNAME		M5235EVB
#define CONFIG_EXTRA_ENV_SETTINGS		\
	"netdev=eth0\0"				\
	"loadaddr=10000\0"			\
	"u-boot=u-boot.bin\0"			\
	"load=tftp ${loadaddr) ${u-boot}\0"	\
	"upd=run load; run prog\0"		\
	"prog=prot off ffe00000 ffe3ffff;"	\
	"era ffe00000 ffe3ffff;"		\
	"cp.b ${loadaddr} ffe00000 ${filesize};"\
	"save\0"				\
	""

#define CONFIG_PRAM		512	/* 512 KB */
#define CFG_PROMPT		"-> "
#define CFG_LONGHELP		/* undef to save memory */

#if defined(CONFIG_KGDB)
#	define CFG_CBSIZE		1024	/* Console I/O Buffer Size */
#else
#	define CFG_CBSIZE		256	/* Console I/O Buffer Size */
#endif

#define CFG_PBSIZE		(CFG_CBSIZE+sizeof(CFG_PROMPT)+16)	/* Print Buffer Size */
#define CFG_MAXARGS		16	/* max number of command args */
#define CFG_BARGSIZE		CFG_CBSIZE	/* Boot Argument Buffer Size    */
#define CFG_LOAD_ADDR		(CFG_SDRAM_BASE+0x20000)

#define CFG_HZ			1000
#define CFG_CLK			75000000
#define CFG_CPU_CLK		CFG_CLK * 2

#define CFG_MBAR		0x40000000

/*
 * Low Level Configuration Settings
 * (address mappings, register initial values, etc.)
 * You should know what you are doing if you make changes here.
 */
/*-----------------------------------------------------------------------
 * Definitions for initial stack pointer and data area (in DPRAM)
 */
#define CFG_INIT_RAM_ADDR	0x20000000
#define CFG_INIT_RAM_END	0x10000	/* End of used area in internal SRAM */
#define CFG_INIT_RAM_CTRL	0x21
#define CFG_GBL_DATA_SIZE	128	/* size in bytes reserved for initial data */
#define CFG_GBL_DATA_OFFSET	(CFG_INIT_RAM_END - CFG_GBL_DATA_SIZE - 0x10)
#define CFG_INIT_SP_OFFSET	CFG_GBL_DATA_OFFSET

/*-----------------------------------------------------------------------
 * Start addresses for the final memory configuration
 * (Set up by the startup code)
 * Please note that CFG_SDRAM_BASE _must_ start at 0
 */
#define CFG_SDRAM_BASE		0x00000000
#define CFG_SDRAM_SIZE		16	/* SDRAM size in MB */

#define CFG_MEMTEST_START	CFG_SDRAM_BASE + 0x400
#define CFG_MEMTEST_END		((CFG_SDRAM_SIZE - 3) << 20)

#define CFG_MONITOR_BASE	(CFG_FLASH_BASE + 0x400)
#define CFG_MONITOR_LEN		(256 << 10)	/* Reserve 256 kB for Monitor */

#define CFG_BOOTPARAMS_LEN	64*1024
#define CFG_MALLOC_LEN		(128 << 10)	/* Reserve 128 kB for malloc() */

/*
 * For booting Linux, the board info and command line data
 * have to be in the first 8 MB of memory, since this is
 * the maximum mapped by the Linux kernel during initialization ??
 */
/* Initial Memory map for Linux */
#define CFG_BOOTMAPSZ		(CFG_SDRAM_BASE + (CFG_SDRAM_SIZE << 20))

/*-----------------------------------------------------------------------
 * FLASH organization
 */
#define CFG_FLASH_CFI
#ifdef CFG_FLASH_CFI
#	define CONFIG_FLASH_CFI_DRIVER	1
#	define CFG_FLASH_SIZE		0x800000	/* Max size that the board might have */
#ifdef NORFLASH_PS32BIT
#	define CFG_FLASH_CFI_WIDTH	FLASH_CFI_32BIT
#else
#	define CFG_FLASH_CFI_WIDTH	FLASH_CFI_16BIT
#endif
#	define CFG_MAX_FLASH_BANKS	1	/* max number of memory banks */
#	define CFG_MAX_FLASH_SECT	137	/* max number of sectors on one chip */
#	define CFG_FLASH_PROTECTION	/* "Real" (hardware) sectors protection */
#endif

#define CFG_FLASH_BASE		(CFG_CS0_BASE << 16)

/* Configuration for environment
 * Environment is embedded in u-boot in the second sector of the flash
 */
#define CFG_ENV_IS_IN_FLASH	1
#define CFG_ENV_IS_EMBEDDED	1
#ifdef NORFLASH_PS32BIT
#	define CFG_ENV_OFFSET		(0x8000)
#	define CFG_ENV_SIZE		0x4000
#	define CFG_ENV_SECT_SIZE	0x4000
#else
#	define CFG_ENV_OFFSET		(0x4000)
#	define CFG_ENV_SIZE		0x2000
#	define CFG_ENV_SECT_SIZE	0x2000
#endif

/*-----------------------------------------------------------------------
 * Cache Configuration
 */
#define CFG_CACHELINE_SIZE	16

/*-----------------------------------------------------------------------
 * Chipselect bank definitions
 */
/*
 * CS0 - NOR Flash 1, 2, 4, or 8MB
 * CS1 - Available
 * CS2 - Available
 * CS3 - Available
 * CS4 - Available
 * CS5 - Available
 * CS6 - Available
 * CS7 - Available
 */
#ifdef NORFLASH_PS32BIT
#	define CFG_CS0_BASE	0xFFC0
#	define CFG_CS0_MASK	0x003f0001
#	define CFG_CS0_CTRL	0x1D00
#else
#	define CFG_CS0_BASE	0xFFE0
#	define CFG_CS0_MASK	0x001f0001
#	define CFG_CS0_CTRL	0x1D80
#endif

#endif				/* _M5329EVB_H */
