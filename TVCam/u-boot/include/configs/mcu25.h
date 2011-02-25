/*
 *(C) Copyright 2005-2007 Netstal Maschinen AG
 *    Niklaus Giger (Niklaus.Giger@netstal.com)
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/************************************************************************
 * mcu25.h - configuration for MCU25 board (similar to hcu4.h)
 ***********************************************************************/

#ifndef __CONFIG_H
#define __CONFIG_H

/*-----------------------------------------------------------------------
 * High Level Configuration Options
 *----------------------------------------------------------------------*/
#define CONFIG_MCU25		1		/* Board is MCU25	*/
#define CONFIG_4xx		1		/* ... PPC4xx family	*/
#define CONFIG_405GP 1
#define CONFIG_4xx   1

#define CONFIG_SYS_CLK_FREQ	33333333	/* external freq to pll	*/

#define CONFIG_BOARD_EARLY_INIT_F 1		/* Call board_early_init_f */
#define CONFIG_MISC_INIT_R	1		/* Call misc_init_r	*/

/*-----------------------------------------------------------------------
 * Base addresses -- Note these are effective addresses where the
 * actual resources get mapped (not physical addresses)
*----------------------------------------------------------------------*/
#define CFG_MONITOR_LEN		(320 * 1024) /* Reserve 320 kB for Monitor */
#define CFG_MALLOC_LEN		(256 * 1024) /* Reserve 256 kB for malloc() */


#define CFG_SDRAM_BASE		0x00000000	/* _must_ be 0		*/
#define CFG_FLASH_BASE		0xfff80000	/* start of FLASH	*/
#define CFG_MONITOR_BASE	TEXT_BASE

/* ... with on-chip memory here (4KBytes) */
#define CFG_OCM_DATA_ADDR	0xF4000000
#define CFG_OCM_DATA_SIZE	0x00001000
/* Do not set up locked dcache as init ram. */
#undef CFG_INIT_DCACHE_CS

/* Use the On-Chip-Memory (OCM) as a temporary stack for the startup code. */
#define CFG_TEMP_STACK_OCM	1

#define CFG_INIT_RAM_ADDR	CFG_OCM_DATA_ADDR	/* OCM		*/
#define CFG_INIT_RAM_END	CFG_OCM_DATA_SIZE
#define CFG_GBL_DATA_SIZE	256		/* num bytes initial data */
#define CFG_GBL_DATA_OFFSET	(CFG_INIT_RAM_END - CFG_GBL_DATA_SIZE)
#define CFG_INIT_SP_OFFSET	CFG_POST_WORD_ADDR

/*-----------------------------------------------------------------------
 * Serial Port
 *----------------------------------------------------------------------*/
/*
 * If CFG_EXT_SERIAL_CLOCK, then the UART divisor is 1.
 * If CFG_405_UART_ERRATA_59, then UART divisor is 31.
 * Otherwise, UART divisor is determined by CPU Clock and CFG_BASE_BAUD value.
 * The Linux BASE_BAUD define should match this configuration.
 *    baseBaud = cpuClock/(uartDivisor*16)
 * If CFG_405_UART_ERRATA_59 and 200MHz CPU clock,
 * set Linux BASE_BAUD to 403200.
 */
#undef CFG_EXT_SERIAL_CLOCK	       /* external serial clock */
#define CONFIG_SERIAL_MULTI  1
/* needed to be able to define CONFIG_SERIAL_SOFTWARE_FIFO */
#undef	CFG_405_UART_ERRATA_59	       /* 405GP/CR Rev. D silicon */
#define CFG_BASE_BAUD	    691200

/* Size (bytes) of interrupt driven serial port buffer.
 * Set to 0 to use polling instead of interrupts.
 * Setting to 0 will also disable RTS/CTS handshaking.
 */
#undef CONFIG_SERIAL_SOFTWARE_FIFO

/* Set console baudrate to 9600 */
#define CONFIG_BAUDRATE		9600


#define CFG_BAUDRATE_TABLE						\
	{300, 600, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200}

/*-----------------------------------------------------------------------
 * Flash
 *----------------------------------------------------------------------*/

/* Use common CFI driver */
#define CFG_FLASH_CFI
#define CONFIG_FLASH_CFI_DRIVER
/* board provides its own flash_init code */
#define CONFIG_FLASH_CFI_LEGACY		1
#define CFG_FLASH_CFI_WIDTH		FLASH_CFI_8BIT
#define CFG_FLASH_LEGACY_512Kx8 1

/* print 'E' for empty sector on flinfo */
#define CFG_FLASH_EMPTY_INFO

#define CFG_MAX_FLASH_BANKS	1	/* max number of memory banks */
#define CFG_MAX_FLASH_SECT	8	/* max number of sectors on one chip */

/*-----------------------------------------------------------------------
 * Environment
 *----------------------------------------------------------------------*/

#undef	CFG_ENV_IS_IN_NVRAM
#define CFG_ENV_IS_IN_FLASH
#undef  CFG_ENV_IS_NOWHERE

#ifdef  CFG_ENV_IS_IN_EEPROM
/* Put the environment after the SDRAM configuration */
#define PROM_SIZE	2048
#define CFG_ENV_OFFSET	 512
#define CFG_ENV_SIZE	(PROM_SIZE-CFG_ENV_OFFSET)
#endif

#ifdef CFG_ENV_IS_IN_FLASH
/* Put the environment in Flash */
#define CFG_ENV_SECT_SIZE	0x10000 /* size of one complete sector */
#define CFG_ENV_ADDR		((-CFG_MONITOR_LEN)-CFG_ENV_SECT_SIZE)
#define	CFG_ENV_SIZE		8*1024	/* 8 KB Environment Sector */

/* Address and size of Redundant Environment Sector	*/
#define CFG_ENV_ADDR_REDUND	(CFG_ENV_ADDR-CFG_ENV_SECT_SIZE)
#define CFG_ENV_SIZE_REDUND	(CFG_ENV_SIZE)
#endif

/*-----------------------------------------------------------------------
 * I2C stuff for a ATMEL AT24C16 (2kB holding ENV, we are using the
 * the first internal I2C controller of the PPC440EPx
 *----------------------------------------------------------------------*/
#define CFG_SPD_BUS_NUM		0

#define CONFIG_HARD_I2C		1	/* I2C with hardware support */
#undef	CONFIG_SOFT_I2C			/* I2C bit-banged		*/
#define CFG_I2C_SPEED		400000	/* I2C speed and slave address	*/
#define CFG_I2C_SLAVE		0x7F

/* This is the 7bit address of the device, not including P. */
#define CFG_I2C_EEPROM_ADDR 0x50
#define CFG_I2C_EEPROM_ADDR_LEN 1

/* The EEPROM can do 16byte ( 1 << 4 ) page writes. */
#define CFG_I2C_EEPROM_ADDR_OVERFLOW	0x07
#define CFG_EEPROM_PAGE_WRITE_BITS 4
#define CFG_EEPROM_PAGE_WRITE_DELAY_MS 10
#define CFG_EEPROM_PAGE_WRITE_ENABLE
#undef CFG_I2C_MULTI_EEPROMS


#define CONFIG_PREBOOT	"echo;"						\
	"echo Type \"run flash_nfs\" to mount root filesystem over NFS;" \
	"echo"

#undef	CONFIG_BOOTARGS

/* Setup some board specific values for the default environment variables */
#define CONFIG_HOSTNAME		mcu25
#define CONFIG_IPADDR		172.25.1.99
#define CONFIG_ETHADDR      00:60:13:00:00:00   /* Netstal Machines AG MAC */
#define CONFIG_OVERWRITE_ETHADDR_ONCE
#define CONFIG_SERVERIP		172.25.1.3

#define CFG_TFTP_LOADADDR 0x01000000 /* @16 MB */

#define	CONFIG_EXTRA_ENV_SETTINGS				\
	"netdev=eth0\0"							\
	"loadaddr=0x01000000\0"						\
	"nfsargs=setenv bootargs root=/dev/nfs rw "			\
		"nfsroot=${serverip}:${rootpath}\0"			\
	"ramargs=setenv bootargs root=/dev/ram rw\0"			\
	"addip=setenv bootargs ${bootargs} "				\
		"ip=${ipaddr}:${serverip}:${gatewayip}:${netmask}"	\
		":${hostname}:${netdev}:off panic=1\0"			\
	"addtty=setenv bootargs ${bootargs} console=ttyS0,${baudrate}\0"\
	"nfs=tftp 200000 ${bootfile};run nfsargs addip addtty;"		\
	        "bootm\0"						\
	"rootpath=/home/diagnose/eldk/ppc_4xx\0"			\
	"bootfile=/tftpboot/mcu25/uImage\0"				\
	"load=tftp 100000 mcu25/u-boot.bin\0"				\
	"update=protect off FFFB0000 FFFFFFFF;era FFFB0000 FFFFFFFF;"	\
		"cp.b 100000 FFFB0000 50000\0"			        \
	"upd=run load;run update\0"					\
	"vx_rom=mcu25/mcu25_vx_rom\0"					\
	"vx=tftp ${loadaddr} ${vx_rom};run vxargs; bootvx\0"		\
	"vxargs=setenv bootargs emac(0,0)c:${vx_rom} e=${ipaddr}"	\
	" h=${serverip} u=dpu pw=netstal8752 tn=hcu5 f=0x3008\0"	\
	""
#define CONFIG_BOOTCOMMAND	"run vx"

#define CONFIG_BOOTDELAY	5	/* autoboot after 5 seconds	*/

#define CONFIG_LOADS_ECHO	1	/* echo on for serial download	*/
#define CFG_LOADS_BAUD_CHANGE	1	/* allow baudrate change	*/

#define CONFIG_MII		1	/* MII PHY management		*/
#define CONFIG_PHY_ADDR	1	/* PHY address			*/

#define CONFIG_PHY_RESET        1	/* reset phy upon startup */

#define CONFIG_HAS_ETH0
#define CFG_RX_ETH_BUFFER	16 /* Number of ethernet rx buffers & descr */

/*
 * BOOTP options
 */
#define CONFIG_BOOTP_BOOTFILESIZE
#define CONFIG_BOOTP_BOOTPATH
#define CONFIG_BOOTP_GATEWAY
#define CONFIG_BOOTP_HOSTNAME

/*
 * Command line configuration.
 */
#include <config_cmd_default.h>

#define CONFIG_CMD_ASKENV
#define CONFIG_CMD_CACHE
#define CONFIG_CMD_DHCP
#define CONFIG_CMD_DIAG
#define CONFIG_CMD_EEPROM
#define CONFIG_CMD_ELF
#define CONFIG_CMD_FLASH
#define CONFIG_CMD_I2C
#define CONFIG_CMD_IMMAP
#define CONFIG_CMD_IRQ
#define CONFIG_CMD_MII
#define CONFIG_CMD_NET
#define CONFIG_CMD_PING
#define CONFIG_CMD_REGINFO
#define CONFIG_CMD_SDRAM

/* SPD EEPROM (sdram speed config) disabled */
#define CONFIG_SPD_EEPROM          1
#define SPD_EEPROM_ADDRESS      0x50

/* POST support */
#define CONFIG_POST		(CFG_POST_MEMORY   | \
				 CFG_POST_CPU	   | \
				 CFG_POST_UART	   | \
				 CFG_POST_I2C	   | \
				 CFG_POST_CACHE	   | \
				 CFG_POST_ETHER	   | \
				 CFG_POST_SPR)

#define CFG_POST_UART_TABLE	{UART0_BASE}
#define CFG_POST_WORD_ADDR	(CFG_GBL_DATA_OFFSET - 0x4)
#undef  CONFIG_LOGBUFFER
#define CFG_POST_CACHE_ADDR	0x00800000 /* free virtual address	*/
#define CFG_CONSOLE_IS_IN_ENV /* Otherwise it catches logbuffer as output */

/*-----------------------------------------------------------------------
 * Miscellaneous configurable options
 *----------------------------------------------------------------------*/
#define CFG_LONGHELP			/* undef to save memory		*/
#define CFG_PROMPT	"=> "		/* Monitor Command Prompt	*/
#if defined(CONFIG_CMD_KGDB)
	#define CFG_CBSIZE	1024		/* Console I/O Buffer Size */
#else
	#define CFG_CBSIZE	256		/* Console I/O Buffer Size */
#endif
#define CFG_PBSIZE (CFG_CBSIZE+sizeof(CFG_PROMPT)+16) /* Print Buffer Size */
#define CFG_MAXARGS	16		/* max number of command args	*/
#define CFG_BARGSIZE	CFG_CBSIZE	/* Boot Argument Buffer Size	*/

#define CFG_MEMTEST_START	0x0400000	/* memtest works on	*/
#define CFG_MEMTEST_END		0x0C00000	/* 4 ... 12 MB in DRAM	*/


#define CFG_LOAD_ADDR		0x100000	/* default load address */
#define CFG_EXTBDINFO		1	/* To use extended board_into (bd_t) */

#define CFG_HZ		1000		/* decrementer freq: 1 ms ticks */

#define CONFIG_CMDLINE_EDITING	1	/* add command line history	*/
#define CONFIG_LOOPW            1       /* enable loopw command         */
#define CONFIG_VERSION_VARIABLE 1	/* include version env variable */

/*-----------------------------------------------------------------------
 * External Bus Controller (EBC) Setup
 */

#define CFG_EBC_CFG            0x98400000

/* Memory Bank 0 (Flash Bank 0) initialization	*/
#define CFG_EBC_PB0AP		0x02005400
#define CFG_EBC_PB0CR		0xFFF18000  /* BAS=0xFFF,BS=1MB,BU=R/W,BW=8bit*/

#define CFG_EBC_PB1AP		0x03041200
#define CFG_EBC_PB1CR		0x7009A000  /* BAS=,BS=MB,BU=R/W,BW=bit	*/

#define CFG_EBC_PB2AP		0x01845200u  /* BAS=,BS=MB,BU=R/W,BW=bit */
#define CFG_EBC_PB2CR		0x7A09A000u

#define CFG_EBC_PB3AP		0x01845200u  /* BAS=,BS=MB,BU=R/W,BW=bit */
#define CFG_EBC_PB3CR		0x7B09A000u

#define CFG_EBC_PB4AP		0x01845200u  /* BAS=,BS=MB,BU=R/W,BW=bit */
#define CFG_EBC_PB4CR		0x7C09A000u

#define CFG_EBC_PB5AP		0x00800200u
#define CFG_EBC_PB5CR		0x7D81A000u

#define CFG_EBC_PB6AP		0x01040200u
#define CFG_EBC_PB6CR		0x7D91A000u

#define CFG_GPIO0_OR		0x087FFFFF  /* GPIO value */
#define CFG_GPIO0_TCR		0x7FFF8000  /* GPIO value */
#define CFG_GPIO0_ODR		0xFFFF0000  /* GPIO value */
/*
 * For booting Linux, the board info and command line data
 * have to be in the first 8 MB of memory, since this is
 * the maximum mapped by the Linux kernel during initialization.
 */
#define CFG_BOOTMAPSZ		(8 << 20) /* Initial Memory map for Linux */

/* Init Memory Controller:
 *
 * BR0/1 and OR0/1 (FLASH)
 */

#define FLASH_BASE0_PRELIM	CFG_FLASH_BASE	/* FLASH bank #0	*/
#define FLASH_BASE1_PRELIM	0		/* FLASH bank #1	*/


/* Configuration Port location */
#define CONFIG_PORT_ADDR	0xF0000500

#define CFG_HUSH_PARSER                 /* use "hush" command parser    */
#ifdef  CFG_HUSH_PARSER
#define CFG_PROMPT_HUSH_PS2	"> "
#endif

#if defined(CONFIG_CMD_KGDB)
#define CONFIG_KGDB_BAUDRATE	230400	/* speed to run kgdb serial port */
#define CONFIG_KGDB_SER_INDEX	2	    /* which serial port to use */
#endif

/* pass open firmware flat tree */
#define CONFIG_OF_LIBFDT	1
#define CONFIG_OF_BOARD_SETUP	1

#endif	/* __CONFIG_H */
