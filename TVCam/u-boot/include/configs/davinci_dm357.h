/*
 * (C) Copyright 2008
 * Configuation settings for the TI DM357 EVM board.
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

/* Chip Configurations */
/*============================================================================*/
#define CFG_DAVINCI
#define CFG_DM357
#define CONFIG_ARM926EJS	/* This is an arm926ejs CPU core  	  */
#define CONFIG_SYS_CLK_FREQ	229500000	/* Arm Clock frequency    */
#define CFG_TIMERBASE		0x01C21400	/* use timer 0 		  */
#define CFG_HZ			27000000	/* Timer Input clock freq */	
/*============================================================================*/

/* Flash Boot info */
/*============================================================================*/
//#define CFG_ENV_IS_IN_FLASH 	1		/* U-Boot env in NOR Flash   */

#ifndef CFG_ENV_IS_IN_FLASH
#define CONFIG_INITRD_TAG  	1
#define CFG_ENV_IS_IN_NAND 	1               /* U-Boot env in NAND Flash  */
#define CFG_ENV_SECT_SIZE	512		/* Env sector Size */
#define CFG_ENV_SIZE		(16 * 1024)
#else
#define CONFIG_INITRD_TAG  	1
#define CFG_ENV_SECT_SIZE	CFG_FLASH_SECT_SZ	/* Env sector Size */
#define CFG_ENV_SIZE		CFG_FLASH_SECT_SZ
#define CFG_ENV_ADDR		(CFG_FLASH_BASE + 0x20000)
#endif


/*
 * NOR Flash Info 
 */
/*============================================================================*/
#define CONFIG_CS0_BOOT				/* Boot from Flash 	     */
#define CFG_MAX_FLASH_BANKS	1		/* max number of flash banks */
#define CFG_FLASH_SECT_SZ	0x20000		/* 128KB sect size Intel Flash */

#ifdef CONFIG_CS0_BOOT
#define PHYS_FLASH_1		0x02000000	/* CS0 Base address 	 */
#endif
#ifdef CONFIG_CS3_BOOT
#define PHYS_FLASH_1		0x00000000	/* Need to update CHECK  */
#endif
#define CFG_FLASH_BASE		PHYS_FLASH_1 	/* Flash Base for U-Boot */
#define CONFIG_ENV_OVERWRITE			/* allow env overwrie 	 */
#define PHYS_FLASH_SIZE		0x1000000	/* Flash size 16MB 	 */
#define CFG_MAX_FLASH_SECT	256		/* max sectors on flash  */
						/* Intel 28F128P30T has  */
						/* 131 sectors, 256      */
						/* is used for backwards */
						/* compatibility with    */
						/* AMD AMLV256U on early */
						/* boards.               */
#if(0)
#define CFG_MAX_FLASH_SECT	(PHYS_FLASH_SIZE/CFG_FLASH_SECT_SZ)
#endif
#define CFG_FLASH_ERASE_TOUT	(20*CFG_HZ)	/* Timeout for Flash Erase */
#define CFG_FLASH_WRITE_TOUT	(20*CFG_HZ)	/* Timeout for Flash Write */
/*============================================================================*/

/*
 * Memory Info 
 */
/*============================================================================*/
#define CFG_MALLOC_LEN		(0x20000 + 128*1024)  /* malloc () len */
#define CFG_GBL_DATA_SIZE	128		/* reserved for initial data */
#define CFG_MEMTEST_START	0x82000000	/* memtest start address  */
#define CFG_MEMTEST_END		0x90000000	/* 16MB RAM test   	  */
#define CONFIG_NR_DRAM_BANKS	1		/* we have 1 bank of DRAM */
#define PHYS_SDRAM_1		0x80000000	/* DDR Start 0x81000000		  */
#define PHYS_SDRAM_1_SIZE	0x10000000	/* DDR size 256MB 	  */
#define CONFIG_STACKSIZE	(256*1024)	/* regular stack	  */
/*============================================================================*/

/*
 * Serial Driver info
 */
/*============================================================================*/
#define CFG_NS16550			/* Include NS16550 as serial driver */
#define CFG_NS16550_SERIAL
#define CFG_NS16550_REG_SIZE 	4		/* NS16550 register size */
#define CFG_NS16550_COM1 	0X01C20000	/* Base address of UART0  */
#define CFG_NS16550_CLK 	27000000	/* Input clock to NS16550 */
#define CONFIG_CONS_INDEX	1		/* use UART0 for console  */
#define CONFIG_BAUDRATE		115200		/* Default baud rate      */
#define CFG_BAUDRATE_TABLE	{ 9600, 19200, 38400, 57600, 115200 }
/*============================================================================*/

/* U-Boot Configurations */
/*============================================================================*/
/*
 * If we are developing, we might want to start armboot from ram
 * so we MUST NOT initialize critical regs like mem-timing ...
 */
/*#undef CONFIG_INIT_CRITICAL             undef for developing */

#undef 	CONFIG_USE_IRQ				/* we don't need IRQ/FIQ */
#define CONFIG_MISC_INIT_R
#define CONFIG_BOOTDELAY	  3     	/* Boot delay before OS boot*/
#define CONFIG_BOOTFILE		"uImage"	/* file to load */
#define CFG_LONGHELP				/* undef to save memory     */
#define CFG_PROMPT	"DM357 EVM # "	/* Monitor Command Prompt   */
#define CFG_CBSIZE	1024			/* Console I/O Buffer Size  */
#define CFG_PBSIZE	(CFG_CBSIZE+sizeof(CFG_PROMPT)+16) /* Print buffer sz */
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

/* macro to read the 32 bit timer Timer 2 */
#define READ_TIMER (0xFFFFFFFF - (*(volatile ulong *)(CFG_TIMERBASE + 0x14)))

/* Linux Information */
/* 0x81000100 */ 
#define LINUX_BOOT_PARAM_ADDR	(0x80000100 + SDRAM_OFFSET)	/* Set the Boot location at the
						 * end of DDR
						 */
#define CONFIG_CMDLINE_TAG	  1	/* enable passing of ATAGs  */
#define CONFIG_SETUP_MEMORY_TAGS  1
#define CONFIG_BOOTARGS		"mem=120M console=ttyS0,115200n8 root=/dev/ram0 rw init=/bin/ash initrd=0x82000000,4M"

#define CONFIG_BOOTCOMMAND	"dhcp;tftpboot 0x82000000 initrd.image;setenv addip setenv bootargs \$(bootargs) ip=\$(ipaddr):\$(serverip):\$(gatewayip):\$(netmask):\$(hostname)::off eth=\$(ethaddr) video=dm64xxfb:output=\$(videostd);run addip;bootm 0x80700000"

/*============================================================================*/

/*
 * Network & Ethernet Configuration
 */
/*============================================================================*/
#define CONFIG_DRIVER_TI_EMAC

#define CONFIG_BOOTP_MASK	(CONFIG_BOOTP_DEFAULT | CONFIG_BOOTP_DNS | CONFIG_BOOTP_DNS2 | CONFIG_BOOTP_SEND_HOSTNAME)
#define CONFIG_NET_RETRY_COUNT  10
/*============================================================================*/

/*============================================================================*/

/* NAND Flash stuff */
/*============================================================================*/      
#ifdef CFG_ENV_IS_IN_NAND
#define CONFIG_COMMANDS		(CFG_CMD_DFL | CFG_CMD_ENV | CFG_CMD_NAND | CFG_CMD_LOADB | CFG_CMD_LOADS | CFG_CMD_MEMORY | CFG_CMD_ASKENV | CFG_CMD_RUN | CFG_CMD_AUTOSCRIPT | CFG_CMD_BDI | CFG_CMD_CONSOLE | CFG_CMD_IMI | CFG_CMD_BOOTD | CFG_CMD_MISC | CFG_CMD_PING | CFG_CMD_DHCP | CFG_CMD_NET | CFG_CMD_NFS)
#define CONFIG_SKIP_LOWLEVEL_INIT       /* needed for booting from NAND as UBL
					 * bootloads u-boot.  The low level init
					 * is configured by the UBL.
					 */
#define CFG_NAND_ADDR           0x02000000
#define CFG_NAND_BASE           0x02000000

#define CFG_MAX_NAND_DEVICE     1	/* Max number of NAND devices */
#define SECTORSIZE              512

#define ADDR_COLUMN             1
#define ADDR_PAGE               2
#define ADDR_COLUMN_PAGE        3

#define NAND_ChipID_UNKNOWN     0x00
#define NAND_MAX_FLOORS         1
#define NAND_MAX_CHIPS          1
//#define CFG_ENV_OFFSET	        0x40000 /* environment starts here  */
#define CFG_ENV_OFFSET		0x3c000

#define WRITE_NAND_COMMAND(d, adr) do {*(volatile u8 *)0x02000010 = (u8)d;} while(0)
#define WRITE_NAND_ADDRESS(d, adr) do {*(volatile u8 *)0x0200000A = (u8)d;} while(0)
#define WRITE_NAND(d, adr) do {*(volatile u8 *)0x02000000 = (u8)d;} while(0)
#define READ_NAND(adr) (*(volatile u8 *)0x02000000)
#define NAND_WAIT_READY(nand) while (!((*(volatile u32 *)0x01E00064) & 1))

#define NAND_NO_RB          1

#define NAND_CTL_CLRALE(nandptr) do {} while(0)
#define NAND_CTL_SETALE(nandptr) do {} while(0)
#define NAND_CTL_CLRCLE(nandptr) do {} while(0)
#define NAND_CTL_SETCLE(nandptr) do {} while(0)
#define NAND_DISABLE_CE(nand) do {*(volatile u32 *)0x01E00060 &= ~0x01;} while(0)
#define NAND_ENABLE_CE(nand) do {*(volatile u32 *)0x01E00060 |= 0x01;} while(0)
#else
#define CONFIG_COMMANDS		(CONFIG_CMD_DFL | CFG_CMD_PING | CFG_CMD_DHCP)
#endif

/* this must be included AFTER the definition of CONFIG_COMMANDS (if any) */
#include <cmd_confdefs.h>

/* KGDB support */
/*============================================================================*/
#if (CONFIG_COMMANDS & CFG_CMD_KGDB)
#define CONFIG_KGDB_BAUDRATE	115200	/* speed to run kgdb serial port */
#define CONFIG_KGDB_SER_INDEX	1	/* which serial port to use */
#endif
#endif /* __CONFIG_H */

