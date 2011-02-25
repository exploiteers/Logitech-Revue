/*
 * (C) Copyright 2006-2008
 * Stefan Roese, DENX Software Engineering, sr@denx.de.
 *
 * (C) Copyright 2006
 * Jacqueline Pira-Ferriol, AMCC/IBM, jpira-ferriol@fr.ibm.com
 * Alain Saurel,            AMCC/IBM, alain.saurel@fr.ibm.com
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

/*
 * sequoia.h - configuration for Sequoia & Rainier boards
 */
#ifndef __CONFIG_H
#define __CONFIG_H

/*
 * High Level Configuration Options
 */
/* This config file is used for Sequoia (440EPx) and Rainier (440GRx)	*/
#ifndef CONFIG_RAINIER
#define CONFIG_440EPX		1	/* Specific PPC440EPx		*/
#define CONFIG_HOSTNAME		sequoia
#else
#define CONFIG_440GRX		1	/* Specific PPC440GRx		*/
#define CONFIG_HOSTNAME		rainier
#endif
#define CONFIG_440		1	/* ... PPC440 family		*/
#define CONFIG_4xx		1	/* ... PPC4xx family		*/

/*
 * Include common defines/options for all AMCC eval boards
 */
#include "amcc-common.h"

/* Detect Sequoia PLL input clock automatically via CPLD bit		*/
#define CONFIG_SYS_CLK_FREQ    ((in8(CFG_BCSR_BASE + 3) & 0x80) ? \
				33333333 : 33000000)

/*
 * Define this if you want support for video console with radeon 9200 pci card
 * Also set TEXT_BASE to 0xFFF80000 in board/amcc/sequoia/config.mk in this case
 */
#undef CONFIG_VIDEO

#ifdef CONFIG_VIDEO
/*
 * 44x dcache supported is working now on sequoia, but we don't enable
 * it yet since it needs further testing
 */
#define CONFIG_4xx_DCACHE		/* enable dcache		*/
#endif

#define CONFIG_BOARD_EARLY_INIT_F 1	/* Call board_early_init_f	*/
#define CONFIG_MISC_INIT_R	1	/* Call misc_init_r		*/

/*
 * Base addresses -- Note these are effective addresses where the actual
 * resources get mapped (not physical addresses).
 */
#define CFG_TLB_FOR_BOOT_FLASH	0x0003
#define CFG_BOOT_BASE_ADDR	0xf0000000
#define CFG_FLASH_BASE		0xfc000000	/* start of FLASH	*/
#define CFG_NAND_ADDR		0xd0000000	/* NAND Flash		*/
#define CFG_OCM_BASE		0xe0010000	/* ocm			*/
#define CFG_OCM_DATA_ADDR	CFG_OCM_BASE
#define CFG_PCI_BASE		0xe0000000	/* Internal PCI regs	*/
#define CFG_PCI_MEMBASE		0x80000000	/* mapped pci memory	*/
#define CFG_PCI_MEMBASE1	CFG_PCI_MEMBASE  + 0x10000000
#define CFG_PCI_MEMBASE2	CFG_PCI_MEMBASE1 + 0x10000000
#define CFG_PCI_MEMBASE3	CFG_PCI_MEMBASE2 + 0x10000000

/* Don't change either of these */
#define CFG_PERIPHERAL_BASE	0xef600000	/* internal peripherals	*/

#define CFG_USB2D0_BASE		0xe0000100
#define CFG_USB_DEVICE		0xe0000000
#define CFG_USB_HOST		0xe0000400
#define CFG_BCSR_BASE		0xc0000000

/*
 * Initial RAM & stack pointer
 */
/* 440EPx/440GRx have 16KB of internal SRAM, so no need for D-Cache	*/
#define CFG_INIT_RAM_ADDR	CFG_OCM_BASE	/* OCM			*/
#define CFG_INIT_RAM_END	(4 << 10)
#define CFG_GBL_DATA_SIZE	256	/* num bytes initial data	*/
#define CFG_GBL_DATA_OFFSET	(CFG_INIT_RAM_END - CFG_GBL_DATA_SIZE)
#define CFG_INIT_SP_OFFSET	CFG_POST_WORD_ADDR

/*
 * Serial Port
 */
#define CFG_EXT_SERIAL_CLOCK	11059200	/* ext. 11.059MHz clk	*/
/* define this if you want console on UART1 */
#undef CONFIG_UART1_CONSOLE

/*
 * Environment
 */
#if !defined(CONFIG_NAND_U_BOOT) && !defined(CONFIG_NAND_SPL)
#define CFG_ENV_IS_IN_FLASH	1	/* use FLASH for environ vars	*/
#else
#define CFG_ENV_IS_IN_NAND	1	/* use NAND for environ vars	*/
#define CFG_ENV_IS_EMBEDDED	1	/* use embedded environment	*/
#endif

/*
 * FLASH related
 */
#define CFG_FLASH_CFI			/* The flash is CFI compatible	*/
#define CONFIG_FLASH_CFI_DRIVER		/* Use common CFI driver	*/

#define CFG_FLASH_BANKS_LIST	{ CFG_FLASH_BASE }

#define CFG_MAX_FLASH_BANKS	1	/* max number of memory banks	      */
#define CFG_MAX_FLASH_SECT	512	/* max number of sectors on one chip  */

#define CFG_FLASH_ERASE_TOUT	120000	/* Timeout for Flash Erase (in ms)    */
#define CFG_FLASH_WRITE_TOUT	500	/* Timeout for Flash Write (in ms)    */

#define CFG_FLASH_USE_BUFFER_WRITE 1	/* use buffered writes (20x faster)   */
#define CFG_FLASH_PROTECTION	1	/* use hardware flash protection      */

#define CFG_FLASH_EMPTY_INFO	      /* print 'E' for empty sector on flinfo */
#define CFG_FLASH_QUIET_TEST	1	/* don't warn upon unknown flash      */

#ifdef CFG_ENV_IS_IN_FLASH
#define CFG_ENV_SECT_SIZE	0x20000	/* size of one complete sector	      */
#define CFG_ENV_ADDR		((-CFG_MONITOR_LEN)-CFG_ENV_SECT_SIZE)
#define	CFG_ENV_SIZE		0x2000	/* Total Size of Environment Sector   */

/* Address and size of Redundant Environment Sector	*/
#define CFG_ENV_ADDR_REDUND	(CFG_ENV_ADDR-CFG_ENV_SECT_SIZE)
#define CFG_ENV_SIZE_REDUND	(CFG_ENV_SIZE)
#endif

/*
 * IPL (Initial Program Loader, integrated inside CPU)
 * Will load first 4k from NAND (SPL) into cache and execute it from there.
 *
 * SPL (Secondary Program Loader)
 * Will load special U-Boot version (NUB) from NAND and execute it. This SPL
 * has to fit into 4kByte. It sets up the CPU and configures the SDRAM
 * controller and the NAND controller so that the special U-Boot image can be
 * loaded from NAND to SDRAM.
 *
 * NUB (NAND U-Boot)
 * This NAND U-Boot (NUB) is a special U-Boot version which can be started
 * from RAM. Therefore it mustn't (re-)configure the SDRAM controller.
 *
 * On 440EPx the SPL is copied to SDRAM before the NAND controller is
 * set up. While still running from cache, I experienced problems accessing
 * the NAND controller.	sr - 2006-08-25
 */
#define CFG_NAND_BOOT_SPL_SRC	0xfffff000	/* SPL location		      */
#define CFG_NAND_BOOT_SPL_SIZE	(4 << 10)	/* SPL size		      */
#define CFG_NAND_BOOT_SPL_DST	(CFG_OCM_BASE + (12 << 10)) /* Copy SPL here  */
#define CFG_NAND_U_BOOT_DST	0x01000000	/* Load NUB to this addr      */
#define CFG_NAND_U_BOOT_START	CFG_NAND_U_BOOT_DST	/* Start NUB from     */
							/*   this addr	      */
#define CFG_NAND_BOOT_SPL_DELTA	(CFG_NAND_BOOT_SPL_SRC - CFG_NAND_BOOT_SPL_DST)

/*
 * Define the partitioning of the NAND chip (only RAM U-Boot is needed here)
 */
#define CFG_NAND_U_BOOT_OFFS	(16 << 10)	/* Offset to RAM U-Boot image */
#define CFG_NAND_U_BOOT_SIZE	(384 << 10)	/* Size of RAM U-Boot image   */

/*
 * Now the NAND chip has to be defined (no autodetection used!)
 */
#define CFG_NAND_PAGE_SIZE	512		/* NAND chip page size	      */
#define CFG_NAND_BLOCK_SIZE	(16 << 10)	/* NAND chip block size	      */
#define CFG_NAND_PAGE_COUNT	32		/* NAND chip page count	      */
#define CFG_NAND_BAD_BLOCK_POS	5	      /* Location of bad block marker */
#undef CFG_NAND_4_ADDR_CYCLE		      /* No fourth addr used (<=32MB) */

#define CFG_NAND_ECCSIZE	256
#define CFG_NAND_ECCBYTES	3
#define CFG_NAND_ECCSTEPS	(CFG_NAND_PAGE_SIZE / CFG_NAND_ECCSIZE)
#define CFG_NAND_OOBSIZE	16
#define CFG_NAND_ECCTOTAL	(CFG_NAND_ECCBYTES * CFG_NAND_ECCSTEPS)
#define CFG_NAND_ECCPOS		{0, 1, 2, 3, 6, 7}

#ifdef CFG_ENV_IS_IN_NAND
/*
 * For NAND booting the environment is embedded in the U-Boot image. Please take
 * look at the file board/amcc/sequoia/u-boot-nand.lds for details.
 */
#define CFG_ENV_SIZE		CFG_NAND_BLOCK_SIZE
#define CFG_ENV_OFFSET		(CFG_NAND_U_BOOT_OFFS + CFG_ENV_SIZE)
#define CFG_ENV_OFFSET_REDUND	(CFG_ENV_OFFSET + CFG_ENV_SIZE)
#endif

/*
 * DDR SDRAM
 */
#define CFG_MBYTES_SDRAM        (256)	/* 256MB			*/
#if !defined(CONFIG_NAND_U_BOOT) && !defined(CONFIG_NAND_SPL)
#define CONFIG_DDR_DATA_EYE		/* use DDR2 optimization	*/
#endif
#define CFG_MEM_TOP_HIDE	(4 << 10) /* don't use last 4kbytes	*/
					/* 440EPx errata CHIP 11	*/

/*
 * I2C
 */
#define CFG_I2C_SPEED		400000	/* I2C speed and slave address	*/

#define CFG_I2C_MULTI_EEPROMS
#define CFG_I2C_EEPROM_ADDR	(0xa8>>1)
#define CFG_I2C_EEPROM_ADDR_LEN 1
#define CFG_EEPROM_PAGE_WRITE_ENABLE
#define CFG_EEPROM_PAGE_WRITE_BITS 3
#define CFG_EEPROM_PAGE_WRITE_DELAY_MS 10

/* I2C SYSMON (LM75, AD7414 is almost compatible)			*/
#define CONFIG_DTT_LM75		1	/* ON Semi's LM75		*/
#define CONFIG_DTT_AD7414	1	/* use AD7414			*/
#define CONFIG_DTT_SENSORS	{0}	/* Sensor addresses		*/
#define CFG_DTT_MAX_TEMP	70
#define CFG_DTT_LOW_TEMP	-30
#define CFG_DTT_HYSTERESIS	3

/*
 * Default environment variables
 */
#define	CONFIG_EXTRA_ENV_SETTINGS					\
	CONFIG_AMCC_DEF_ENV						\
	CONFIG_AMCC_DEF_ENV_POWERPC					\
	CONFIG_AMCC_DEF_ENV_PPC_OLD					\
	CONFIG_AMCC_DEF_ENV_NOR_UPD					\
	CONFIG_AMCC_DEF_ENV_NAND_UPD					\
	"kernel_addr=FC000000\0"					\
	"ramdisk_addr=FC180000\0"					\
	""

#define CONFIG_M88E1111_PHY	1
#define	CONFIG_IBM_EMAC4_V4	1
#define CONFIG_PHY_ADDR		0	/* PHY address, See schematics	*/

#define CONFIG_PHY_RESET        1	/* reset phy upon startup	*/
#define CONFIG_PHY_GIGE		1	/* Include GbE speed/duplex detection */

#define CONFIG_HAS_ETH0
#define CONFIG_HAS_ETH1		1	/* add support for "eth1addr"	*/
#define CONFIG_PHY1_ADDR	1

/* USB */
#ifdef CONFIG_440EPX
#define CONFIG_USB_OHCI_NEW
#define CONFIG_USB_STORAGE
#define CFG_OHCI_BE_CONTROLLER

#undef CFG_USB_OHCI_BOARD_INIT
#define CFG_USB_OHCI_CPU_INIT	1
#define CFG_USB_OHCI_REGS_BASE	CFG_USB_HOST
#define CFG_USB_OHCI_SLOT_NAME	"ppc440"
#define CFG_USB_OHCI_MAX_ROOT_PORTS 15

/* Comment this out to enable USB 1.1 device */
#define USB_2_0_DEVICE

#endif /* CONFIG_440EPX */

/* Partitions */
#define CONFIG_MAC_PARTITION
#define CONFIG_DOS_PARTITION
#define CONFIG_ISO_PARTITION

/*
 * Commands additional to the ones defined in amcc-common.h
 */
#define CONFIG_CMD_DTT
#define CONFIG_CMD_FAT
#define CONFIG_CMD_NAND
#define CONFIG_CMD_PCI
#define CONFIG_CMD_SDRAM

#ifdef CONFIG_440EPX
#define CONFIG_CMD_USB
#endif

#ifndef CONFIG_RAINIER
#define CFG_POST_FPU_ON		CFG_POST_FPU
#else
#define CFG_POST_FPU_ON		0
#endif

/* POST support */
#define CONFIG_POST		(CFG_POST_CACHE	   | \
				 CFG_POST_CPU	   | \
				 CFG_POST_ETHER	   | \
				 CFG_POST_FPU_ON   | \
				 CFG_POST_I2C	   | \
				 CFG_POST_MEMORY   | \
				 CFG_POST_SPR	   | \
				 CFG_POST_UART)

#define CFG_POST_WORD_ADDR	(CFG_GBL_DATA_OFFSET - 0x4)
#define CONFIG_LOGBUFFER
#define CFG_POST_CACHE_ADDR	0x7fff0000	/* free virtual address     */

#define CFG_CONSOLE_IS_IN_ENV	/* Otherwise it catches logbuffer as output */

#define CONFIG_SUPPORT_VFAT

/*
 * PCI stuff
 */
/* General PCI */
#define CONFIG_PCI			/* include pci support		*/
#define CONFIG_PCI_PNP			/* do pci plug-and-play		*/
#define CFG_PCI_CACHE_LINE_SIZE	0	/* to avoid problems with PNP	*/
#define CONFIG_PCI_SCAN_SHOW		/* show pci devices on startup	*/
#define CFG_PCI_TARGBASE	0x80000000	/* PCIaddr mapped to	*/
						/*   CFG_PCI_MEMBASE	*/
/* Board-specific PCI */
#define CFG_PCI_TARGET_INIT
#define CFG_PCI_MASTER_INIT

#define CFG_PCI_SUBSYS_VENDORID 0x10e8	/* AMCC				*/
#define CFG_PCI_SUBSYS_ID       0xcafe	/* Whatever			*/

/*
 * External Bus Controller (EBC) Setup
 */

/*
 * On Sequoia CS0 and CS3 are switched when configuring for NAND booting
 */
#if !defined(CONFIG_NAND_U_BOOT) && !defined(CONFIG_NAND_SPL)
#define CFG_NAND_CS		3	/* NAND chip connected to CSx	*/
/* Memory Bank 0 (NOR-FLASH) initialization				*/
#define CFG_EBC_PB0AP		0x03017200
#define CFG_EBC_PB0CR		(CFG_FLASH_BASE | 0xda000)

/* Memory Bank 3 (NAND-FLASH) initialization				*/
#define CFG_EBC_PB3AP		0x018003c0
#define CFG_EBC_PB3CR		(CFG_NAND_ADDR | 0x1c000)
#else
#define CFG_NAND_CS		0	/* NAND chip connected to CSx	*/
/* Memory Bank 3 (NOR-FLASH) initialization				*/
#define CFG_EBC_PB3AP		0x03017200
#define CFG_EBC_PB3CR		(CFG_FLASH_BASE | 0xda000)

/* Memory Bank 0 (NAND-FLASH) initialization				*/
#define CFG_EBC_PB0AP		0x018003c0
#define CFG_EBC_PB0CR		(CFG_NAND_ADDR | 0x1c000)
#endif

/* Memory Bank 2 (CPLD) initialization					*/
#define CFG_EBC_PB2AP		0x24814580
#define CFG_EBC_PB2CR		(CFG_BCSR_BASE | 0x38000)

#define CFG_BCSR5_PCI66EN	0x80

/*
 * NAND FLASH
 */
#define CFG_MAX_NAND_DEVICE	1
#define NAND_MAX_CHIPS		1
#define CFG_NAND_BASE		(CFG_NAND_ADDR + CFG_NAND_CS)
#define CFG_NAND_SELECT_DEVICE  1	/* nand driver supports mutipl. chips */

/*
 * PPC440 GPIO Configuration
 */
/* test-only: take GPIO init from pcs440ep ???? in config file */
#define CFG_4xx_GPIO_TABLE { /*	  Out		  GPIO	Alternate1	Alternate2	Alternate3 */ \
{											\
/* GPIO Core 0 */									\
{GPIO0_BASE, GPIO_BI , GPIO_ALT1, GPIO_OUT_0}, /* GPIO0	EBC_ADDR(7)	DMA_REQ(2)	*/	\
{GPIO0_BASE, GPIO_BI , GPIO_ALT1, GPIO_OUT_0}, /* GPIO1	EBC_ADDR(6)	DMA_ACK(2)	*/	\
{GPIO0_BASE, GPIO_IN , GPIO_SEL , GPIO_OUT_0}, /* GPIO2	EBC_ADDR(5)	DMA_EOT/TC(2)	*/	\
{GPIO0_BASE, GPIO_IN , GPIO_SEL , GPIO_OUT_0}, /* GPIO3	EBC_ADDR(4)	DMA_REQ(3)	*/	\
{GPIO0_BASE, GPIO_IN , GPIO_SEL , GPIO_OUT_0}, /* GPIO4	EBC_ADDR(3)	DMA_ACK(3)	*/	\
{GPIO0_BASE, GPIO_IN , GPIO_SEL , GPIO_OUT_0}, /* GPIO5	EBC_ADDR(2)	DMA_EOT/TC(3)	*/	\
{GPIO0_BASE, GPIO_IN , GPIO_SEL , GPIO_OUT_0}, /* GPIO6	EBC_CS_N(1)			*/	\
{GPIO0_BASE, GPIO_OUT, GPIO_ALT1, GPIO_OUT_0}, /* GPIO7	EBC_CS_N(2)			*/	\
{GPIO0_BASE, GPIO_OUT, GPIO_ALT1, GPIO_OUT_0}, /* GPIO8	EBC_CS_N(3)			*/	\
{GPIO0_BASE, GPIO_OUT, GPIO_ALT1, GPIO_OUT_0}, /* GPIO9	EBC_CS_N(4)			*/	\
{GPIO0_BASE, GPIO_OUT, GPIO_ALT1, GPIO_OUT_0}, /* GPIO10 EBC_CS_N(5)			*/	\
{GPIO0_BASE, GPIO_IN , GPIO_SEL , GPIO_OUT_0}, /* GPIO11 EBC_BUS_ERR			*/	\
{GPIO0_BASE, GPIO_IN , GPIO_SEL , GPIO_OUT_0}, /* GPIO12				*/	\
{GPIO0_BASE, GPIO_IN , GPIO_SEL , GPIO_OUT_0}, /* GPIO13				*/	\
{GPIO0_BASE, GPIO_IN , GPIO_SEL , GPIO_OUT_0}, /* GPIO14				*/	\
{GPIO0_BASE, GPIO_IN , GPIO_SEL , GPIO_OUT_0}, /* GPIO15				*/	\
{GPIO0_BASE, GPIO_OUT, GPIO_ALT1, GPIO_OUT_1}, /* GPIO16 GMCTxD(4)			*/	\
{GPIO0_BASE, GPIO_OUT, GPIO_ALT1, GPIO_OUT_1}, /* GPIO17 GMCTxD(5)			*/	\
{GPIO0_BASE, GPIO_OUT, GPIO_ALT1, GPIO_OUT_1}, /* GPIO18 GMCTxD(6)			*/	\
{GPIO0_BASE, GPIO_OUT, GPIO_ALT1, GPIO_OUT_1}, /* GPIO19 GMCTxD(7)			*/	\
{GPIO0_BASE, GPIO_IN , GPIO_SEL , GPIO_OUT_0}, /* GPIO20 RejectPkt0			*/	\
{GPIO0_BASE, GPIO_IN , GPIO_SEL , GPIO_OUT_0}, /* GPIO21 RejectPkt1			*/	\
{GPIO0_BASE, GPIO_IN , GPIO_SEL , GPIO_OUT_0}, /* GPIO22				*/	\
{GPIO0_BASE, GPIO_IN , GPIO_SEL , GPIO_OUT_0}, /* GPIO23 SCPD0				*/	\
{GPIO0_BASE, GPIO_OUT, GPIO_ALT1, GPIO_OUT_1}, /* GPIO24 GMCTxD(2)			*/	\
{GPIO0_BASE, GPIO_OUT, GPIO_ALT1, GPIO_OUT_1}, /* GPIO25 GMCTxD(3)			*/	\
{GPIO0_BASE, GPIO_IN , GPIO_SEL , GPIO_OUT_0}, /* GPIO26				*/	\
{GPIO0_BASE, GPIO_IN , GPIO_ALT2, GPIO_OUT_0}, /* GPIO27 EXT_EBC_REQ	USB2D_RXERROR	*/	\
{GPIO0_BASE, GPIO_OUT, GPIO_ALT2, GPIO_OUT_1}, /* GPIO28		USB2D_TXVALID	*/	\
{GPIO0_BASE, GPIO_OUT, GPIO_ALT2, GPIO_OUT_1}, /* GPIO29 EBC_EXT_HDLA	USB2D_PAD_SUSPNDM */	\
{GPIO0_BASE, GPIO_OUT, GPIO_ALT2, GPIO_OUT_1}, /* GPIO30 EBC_EXT_ACK	USB2D_XCVRSELECT*/	\
{GPIO0_BASE, GPIO_OUT, GPIO_ALT2, GPIO_OUT_1}, /* GPIO31 EBC_EXR_BUSREQ	USB2D_TERMSELECT*/	\
},											\
{											\
/* GPIO Core 1 */									\
{GPIO1_BASE, GPIO_OUT, GPIO_ALT1, GPIO_OUT_1}, /* GPIO32 USB2D_OPMODE0	EBC_DATA(2)	*/	\
{GPIO1_BASE, GPIO_OUT, GPIO_ALT1, GPIO_OUT_1}, /* GPIO33 USB2D_OPMODE1	EBC_DATA(3)	*/	\
{GPIO1_BASE, GPIO_IN , GPIO_ALT2, GPIO_OUT_0}, /* GPIO34 UART0_8PIN_DCD_N UART1_DSR_CTS_N UART2_SOUT*/ \
{GPIO1_BASE, GPIO_OUT, GPIO_ALT2, GPIO_OUT_1}, /* GPIO35 UART0_8PIN_DSR_N UART1_RTS_DTR_N UART2_SIN*/ \
{GPIO1_BASE, GPIO_IN , GPIO_ALT1, GPIO_OUT_0}, /* GPIO36 UART0_CTS_N	EBC_DATA(0)	UART3_SIN*/ \
{GPIO1_BASE, GPIO_OUT, GPIO_ALT1, GPIO_OUT_1}, /* GPIO37 UART0_RTS_N	EBC_DATA(1)	UART3_SOUT*/ \
{GPIO1_BASE, GPIO_OUT, GPIO_ALT2, GPIO_OUT_1}, /* GPIO38 UART0_8PIN_DTR_N UART1_SOUT	*/	\
{GPIO1_BASE, GPIO_IN , GPIO_ALT2, GPIO_OUT_0}, /* GPIO39 UART0_8PIN_RI_N UART1_SIN	*/	\
{GPIO1_BASE, GPIO_IN , GPIO_ALT1, GPIO_OUT_0}, /* GPIO40 UIC_IRQ(0)			*/	\
{GPIO1_BASE, GPIO_IN , GPIO_ALT1, GPIO_OUT_0}, /* GPIO41 UIC_IRQ(1)			*/	\
{GPIO1_BASE, GPIO_IN , GPIO_ALT1, GPIO_OUT_0}, /* GPIO42 UIC_IRQ(2)			*/	\
{GPIO1_BASE, GPIO_IN , GPIO_ALT1, GPIO_OUT_0}, /* GPIO43 UIC_IRQ(3)			*/	\
{GPIO1_BASE, GPIO_IN , GPIO_ALT1, GPIO_OUT_0}, /* GPIO44 UIC_IRQ(4)	DMA_ACK(1)	*/	\
{GPIO1_BASE, GPIO_IN , GPIO_ALT1, GPIO_OUT_0}, /* GPIO45 UIC_IRQ(6)	DMA_EOT/TC(1)	*/	\
{GPIO1_BASE, GPIO_IN , GPIO_SEL , GPIO_OUT_0}, /* GPIO46 UIC_IRQ(7)	DMA_REQ(0)	*/	\
{GPIO1_BASE, GPIO_IN , GPIO_SEL , GPIO_OUT_0}, /* GPIO47 UIC_IRQ(8)	DMA_ACK(0)	*/	\
{GPIO1_BASE, GPIO_IN , GPIO_SEL , GPIO_OUT_0}, /* GPIO48 UIC_IRQ(9)	DMA_EOT/TC(0)	*/	\
{GPIO1_BASE, GPIO_IN , GPIO_SEL , GPIO_OUT_0}, /* GPIO49  Unselect via TraceSelect Bit	*/	\
{GPIO1_BASE, GPIO_IN , GPIO_SEL , GPIO_OUT_0}, /* GPIO50  Unselect via TraceSelect Bit	*/	\
{GPIO1_BASE, GPIO_IN , GPIO_SEL , GPIO_OUT_0}, /* GPIO51  Unselect via TraceSelect Bit	*/	\
{GPIO1_BASE, GPIO_IN , GPIO_SEL , GPIO_OUT_0}, /* GPIO52  Unselect via TraceSelect Bit	*/	\
{GPIO1_BASE, GPIO_IN , GPIO_SEL , GPIO_OUT_0}, /* GPIO53  Unselect via TraceSelect Bit	*/	\
{GPIO1_BASE, GPIO_IN , GPIO_SEL , GPIO_OUT_0}, /* GPIO54  Unselect via TraceSelect Bit	*/	\
{GPIO1_BASE, GPIO_IN , GPIO_SEL , GPIO_OUT_0}, /* GPIO55  Unselect via TraceSelect Bit	*/	\
{GPIO1_BASE, GPIO_IN , GPIO_SEL , GPIO_OUT_0}, /* GPIO56  Unselect via TraceSelect Bit	*/	\
{GPIO1_BASE, GPIO_IN , GPIO_SEL , GPIO_OUT_0}, /* GPIO57  Unselect via TraceSelect Bit	*/	\
{GPIO1_BASE, GPIO_IN , GPIO_SEL , GPIO_OUT_0}, /* GPIO58  Unselect via TraceSelect Bit	*/	\
{GPIO1_BASE, GPIO_IN , GPIO_SEL , GPIO_OUT_0}, /* GPIO59  Unselect via TraceSelect Bit	*/	\
{GPIO1_BASE, GPIO_IN , GPIO_SEL , GPIO_OUT_0}, /* GPIO60  Unselect via TraceSelect Bit	*/	\
{GPIO1_BASE, GPIO_IN , GPIO_SEL , GPIO_OUT_0}, /* GPIO61  Unselect via TraceSelect Bit	*/	\
{GPIO1_BASE, GPIO_IN , GPIO_SEL , GPIO_OUT_0}, /* GPIO62  Unselect via TraceSelect Bit	*/	\
{GPIO1_BASE, GPIO_IN , GPIO_SEL , GPIO_OUT_0}, /* GPIO63  Unselect via TraceSelect Bit	*/	\
}											\
}

#ifdef CONFIG_VIDEO
#define CONFIG_BIOSEMU			/* x86 bios emulator for vga bios */
#define CONFIG_ATI_RADEON_FB		/* use radeon framebuffer driver */
#define VIDEO_IO_OFFSET			0xe8000000
#define CFG_ISA_IO_BASE_ADDRESS		VIDEO_IO_OFFSET
#define CONFIG_VIDEO_SW_CURSOR
#define CONFIG_VIDEO_LOGO
#define CONFIG_CFB_CONSOLE
#define CONFIG_SPLASH_SCREEN
#define CONFIG_VGA_AS_SINGLE_DEVICE
#define CONFIG_CMD_BMP
#endif

#endif /* __CONFIG_H */
