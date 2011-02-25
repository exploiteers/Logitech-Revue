/*
 * (C) Copyright 2007
 * Stefano Babic, DENX Gmbh, sbabic@denx.de
 *
 * (C) Copyright 2004
 * Robert Whaley, Applied Data Systems, Inc. rwhaley@applieddata.net
 *
 * (C) Copyright 2002
 * Kyle Harris, Nexus Technologies, Inc. kharris@nexus-tech.net
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * Configuation settings for the LUBBOCK board.
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

/*
 * High Level Configuration Options
 * (easy to change)
 */
#define CONFIG_PXA27X		1	/* This is an PXA27x CPU    */

#define LITTLEENDIAN		1	/* used by usb_ohci.c		*/

#define CONFIG_MMC		1
#define BOARD_LATE_INIT		1

#undef CONFIG_USE_IRQ			/* we don't need IRQ/FIQ stuff */

#define RTC

/*
 * Size of malloc() pool
 */
#define CFG_MALLOC_LEN	    (CFG_ENV_SIZE + 128*1024)
#define CFG_GBL_DATA_SIZE	128	/* size in bytes reserved for initial data */

/*
 * Hardware drivers
 */

/*
 * select serial console configuration
 */
#define CONFIG_SERIAL_MULTI
#define CONFIG_FFUART	       1       /* we use FFUART on Conxs */
#define CONFIG_BTUART	       1       /* we use BTUART on Conxs */
#define CONFIG_STUART	       1       /* we use STUART on Conxs */

/* allow to overwrite serial and ethaddr */
#define CONFIG_ENV_OVERWRITE

#define CONFIG_BAUDRATE	       38400

#define CONFIG_DOS_PARTITION   1

/*
 * Command line configuration.
 */
#include <config_cmd_default.h>

#define CONFIG_CMD_MMC
#define CONFIG_CMD_FAT
#define CONFIG_CMD_IMLS
#define CONFIG_CMD_PING
#define CONFIG_CMD_USB

/* this must be included AFTER the definition of CONFIG_COMMANDS (if any) */

#undef CONFIG_SHOW_BOOT_PROGRESS

#define CONFIG_BOOTDELAY	3
#define CONFIG_SERVERIP		192.168.1.99
#define CONFIG_BOOTCOMMAND	"run boot_flash"
#define CONFIG_BOOTARGS		"console=ttyS0,38400 ramdisk_size=12288"\
				" rw root=/dev/ram initrd=0xa0800000,5m"

#define CONFIG_EXTRA_ENV_SETTINGS					\
	"program_boot_mmc="						\
			"mw.b 0xa0010000 0xff 0x20000; "		\
			"if	 mmcinit && "				\
				"fatload mmc 0 0xa0010000 u-boot.bin; "	\
			"then "						\
				"protect off 0x0 0x1ffff; "		\
				"erase 0x0 0x1ffff; "			\
				"cp.b 0xa0010000 0x0 0x20000; "		\
			"fi\0"						\
	"program_uzImage_mmc="						\
			"mw.b 0xa0010000 0xff 0x180000; "		\
			"if	 mmcinit && "				\
				"fatload mmc 0 0xa0010000 uzImage; "	\
			"then "						\
				"protect off 0x40000 0x1bffff; "	\
				"erase 0x40000 0x1bffff; "		\
				"cp.b 0xa0010000 0x40000 0x180000; "	\
			"fi\0"						\
	"program_ramdisk_mmc="						\
			"mw.b 0xa0010000 0xff 0x500000; "		\
			"if	 mmcinit && "				\
				"fatload mmc 0 0xa0010000 ramdisk.gz; "	\
			"then "						\
				"protect off 0x1c0000 0x6bffff; "	\
				"erase 0x1c0000 0x6bffff; "		\
				"cp.b 0xa0010000 0x1c0000 0x500000; "	\
			"fi\0"						\
	"boot_mmc="							\
			"if	 mmcinit && "				\
				"fatload mmc 0 0xa0030000 uzImage && "	\
				"fatload mmc 0 0xa0800000 ramdisk.gz; "	\
			"then "						\
				"bootm 0xa0030000; "			\
			"fi\0"						\
	"boot_flash="							\
			"cp.b 0x1c0000 0xa0800000 0x500000; "		\
			"bootm 0x40000\0"				\

#define CONFIG_SETUP_MEMORY_TAGS 1
#define CONFIG_CMDLINE_TAG	 1	/* enable passing of ATAGs	*/
/* #define CONFIG_INITRD_TAG	 1 */

#if defined(CONFIG_CMD_KGDB)
#define CONFIG_KGDB_BAUDRATE	230400		/* speed to run kgdb serial port */
#define CONFIG_KGDB_SER_INDEX	2		/* which serial port to use */
#endif

/*
 * Miscellaneous configurable options
 */
#define CFG_HUSH_PARSER		1
#define CFG_PROMPT_HUSH_PS2	"> "

#define CFG_LONGHELP				/* undef to save memory		*/
#ifdef CFG_HUSH_PARSER
#define CFG_PROMPT		"$ "		/* Monitor Command Prompt */
#else
#define CFG_PROMPT		"=> "		/* Monitor Command Prompt */
#endif
#define CFG_CBSIZE		256		/* Console I/O Buffer Size	*/
#define CFG_PBSIZE (CFG_CBSIZE+sizeof(CFG_PROMPT)+16) /* Print Buffer Size */
#define CFG_MAXARGS		16		/* max number of command args	*/
#define CFG_BARGSIZE		CFG_CBSIZE	/* Boot Argument Buffer Size	*/
#define CFG_DEVICE_NULLDEV	1

#define CFG_MEMTEST_START	0xa0400000	/* memtest works on	*/
#define CFG_MEMTEST_END		0xa0800000	/* 4 ... 8 MB in DRAM	*/

#undef	CFG_CLKS_IN_HZ		/* everything, incl board info, in Hz */

#define CFG_LOAD_ADDR		0xa1000000	/* default load address */

#define CFG_HZ			3686400		/* incrementer freq: 3.6864 MHz */
#define CFG_CPUSPEED		0x207		/* need to look more closely, I think this is Turbo = 2x, L=91Mhz */

						/* valid baudrates */
#define CFG_BAUDRATE_TABLE	{ 9600, 19200, 38400, 57600, 115200 }

#define CFG_MMC_BASE		0xF0000000

/*
 * Stack sizes
 *
 * The stack sizes are set up in start.S using the settings below
 */
#define CONFIG_STACKSIZE	(128*1024)	/* regular stack */
#ifdef CONFIG_USE_IRQ
#define CONFIG_STACKSIZE_IRQ	(4*1024)	/* IRQ stack */
#define CONFIG_STACKSIZE_FIQ	(4*1024)	/* FIQ stack */
#endif

/*
 * Physical Memory Map
 */
#define CONFIG_NR_DRAM_BANKS	4	   /* we have 2 banks of DRAM */
#define PHYS_SDRAM_1		0xa0000000 /* SDRAM Bank #1 */
#define PHYS_SDRAM_1_SIZE	0x04000000 /* 64 MB */
#define PHYS_SDRAM_2		0xa4000000 /* SDRAM Bank #2 */
#define PHYS_SDRAM_2_SIZE	0x00000000 /* 0 MB */
#define PHYS_SDRAM_3		0xa8000000 /* SDRAM Bank #3 */
#define PHYS_SDRAM_3_SIZE	0x00000000 /* 0 MB */
#define PHYS_SDRAM_4		0xac000000 /* SDRAM Bank #4 */
#define PHYS_SDRAM_4_SIZE	0x00000000 /* 0 MB */

#define PHYS_FLASH_1		0x00000000 /* Flash Bank #1 */

#define CFG_DRAM_BASE		0xa0000000
#define CFG_DRAM_SIZE		0x04000000

#define CFG_FLASH_BASE		PHYS_FLASH_1

/*
 * GPIO settings
 */
#define CFG_GPSR0_VAL		0x00018000
#define CFG_GPSR1_VAL		0x00000000
#define CFG_GPSR2_VAL		0x400dc000
#define CFG_GPSR3_VAL		0x00000000
#define CFG_GPCR0_VAL		0x00000000
#define CFG_GPCR1_VAL		0x00000000
#define CFG_GPCR2_VAL		0x00000000
#define CFG_GPCR3_VAL		0x00000000
#define CFG_GPDR0_VAL		0x00018000
#define CFG_GPDR1_VAL		0x00028801
#define CFG_GPDR2_VAL		0x520dc000
#define CFG_GPDR3_VAL		0x0001E000
#define CFG_GAFR0_L_VAL		0x801c0000
#define CFG_GAFR0_U_VAL		0x00000013
#define CFG_GAFR1_L_VAL		0x6990100A
#define CFG_GAFR1_U_VAL		0x00000008
#define CFG_GAFR2_L_VAL		0xA0000000
#define CFG_GAFR2_U_VAL		0x010900F2
#define CFG_GAFR3_L_VAL		0x54000003
#define CFG_GAFR3_U_VAL		0x00002401
#define CFG_GRER0_VAL		0x00000000
#define CFG_GRER1_VAL		0x00000000
#define CFG_GRER2_VAL		0x00000000
#define CFG_GRER3_VAL		0x00000000
#define CFG_GFER0_VAL		0x00000000
#define CFG_GFER1_VAL		0x00000000
#define CFG_GFER2_VAL		0x00000000
#define CFG_GFER3_VAL		0x00000020


#define CFG_PSSR_VAL		0x20	/* CHECK */

/*
 * Clock settings
 */
#define CFG_CKEN		0x01FFFFFF	/* CHECK */
#define CFG_CCCR		0x02000290 /*   520Mhz */

/*
 * Memory settings
 */

#define CFG_MSC0_VAL		0x4df84df0
#define CFG_MSC1_VAL		0x7ff87ff4
#define CFG_MSC2_VAL		0xa26936d4
#define CFG_MDCNFG_VAL		0x880009C9
#define CFG_MDREFR_VAL		0x20ca201e
#define CFG_MDMRS_VAL		0x00220022

#define CFG_FLYCNFG_VAL		0x00000000
#define CFG_SXCNFG_VAL		0x40044004

/*
 * PCMCIA and CF Interfaces
 */
#define CFG_MECR_VAL		0x00000001
#define CFG_MCMEM0_VAL		0x00004204
#define CFG_MCMEM1_VAL		0x00010204
#define CFG_MCATT0_VAL		0x00010504
#define CFG_MCATT1_VAL		0x00010504
#define CFG_MCIO0_VAL		0x00008407
#define CFG_MCIO1_VAL		0x0000c108

#define CONFIG_DRIVER_DM9000		1
#define CONFIG_DM9000_BASE	0x08000000
#define DM9000_IO			CONFIG_DM9000_BASE
#define DM9000_DATA			(CONFIG_DM9000_BASE+0x8004)

#define CONFIG_USB_OHCI_NEW	1
#define CFG_USB_OHCI_BOARD_INIT	1
#define CFG_USB_OHCI_MAX_ROOT_PORTS	3
#define CFG_USB_OHCI_REGS_BASE	0x4C000000
#define CFG_USB_OHCI_SLOT_NAME	"trizepsiv"
#define CONFIG_USB_STORAGE	1
#define CFG_USB_OHCI_CPU_INIT	1

/*
 * FLASH and environment organization
 */

#define CFG_FLASH_CFI
#define CONFIG_FLASH_CFI_DRIVER	1

#define CFG_MONITOR_BASE	0
#define CFG_MONITOR_LEN		0x40000

#define CFG_MAX_FLASH_BANKS	1	/* max number of memory banks		*/
#define CFG_MAX_FLASH_SECT	4 + 255  /* max number of sectors on one chip   */

/* timeout values are in ticks */
#define CFG_FLASH_ERASE_TOUT	(25*CFG_HZ) /* Timeout for Flash Erase */
#define CFG_FLASH_WRITE_TOUT	(25*CFG_HZ) /* Timeout for Flash Write */

/* write flash less slowly */
#define CFG_FLASH_USE_BUFFER_WRITE 1

/* Flash environment locations */
#define CFG_ENV_IS_IN_FLASH	1
#define CFG_ENV_ADDR		(PHYS_FLASH_1 + CFG_MONITOR_LEN) /* Addr of Environment Sector	*/
#define CFG_ENV_SIZE		0x40000	/* Total Size of Environment		*/
#define CFG_ENV_SECT_SIZE	0x40000	/* Total Size of Environment Sector	*/

/* Address and size of Redundant Environment Sector	*/
#define CFG_ENV_ADDR_REDUND	(CFG_ENV_ADDR+CFG_ENV_SECT_SIZE)
#define CFG_ENV_SIZE_REDUND	(CFG_ENV_SIZE)

#endif	/* __CONFIG_H */
