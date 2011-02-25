/*
 * Copyright (C) 2008 Atmel Corporation
 *
 * Configuration settings for the Favr-32 EarthLCD LCD kit.
 *
 * See file CREDITS for list of people who contributed to this project.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA 02111-1307 USA
 */
#ifndef __CONFIG_H
#define __CONFIG_H

#include <asm/arch/memory-map.h>

#define CONFIG_AVR32			1
#define CONFIG_AT32AP			1
#define CONFIG_AT32AP7000		1
#define CONFIG_FAVR32_EZKIT		1

#define CONFIG_FAVR32_EZKIT_EXT_FLASH	1

/*
 * Timer clock frequency. We're using the CPU-internal COUNT register
 * for this, so this is equivalent to the CPU core clock frequency
 */
#define CFG_HZ				1000

/*
 * Set up the PLL to run at 140 MHz, the CPU to run at the PLL
 * frequency, the HSB and PBB at 1/2, and the PBA to run at 1/4 the
 * PLL frequency.
 * (CFG_OSC0_HZ * CFG_PLL0_MUL) / CFG_PLL0_DIV = PLL MHz
 */
#define CONFIG_PLL			1
#define CFG_POWER_MANAGER		1
#define CFG_OSC0_HZ			20000000
#define CFG_PLL0_DIV			1
#define CFG_PLL0_MUL			7
#define CFG_PLL0_SUPPRESS_CYCLES	16
/*
 * Set the CPU running at:
 * PLL / (2^CFG_CLKDIV_CPU) = CPU MHz
 */
#define CFG_CLKDIV_CPU			0
/*
 * Set the HSB running at:
 * PLL / (2^CFG_CLKDIV_HSB) = HSB MHz
 */
#define CFG_CLKDIV_HSB			1
/*
 * Set the PBA running at:
 * PLL / (2^CFG_CLKDIV_PBA) = PBA MHz
 */
#define CFG_CLKDIV_PBA			2
/*
 * Set the PBB running at:
 * PLL / (2^CFG_CLKDIV_PBB) = PBB MHz
 */
#define CFG_CLKDIV_PBB			1

/*
 * The PLLOPT register controls the PLL like this:
 *   icp = PLLOPT<2>
 *   ivco = PLLOPT<1:0>
 *
 * We want icp=1 (default) and ivco=0 (80-160 MHz) or ivco=2 (150-240MHz).
 */
#define CFG_PLL0_OPT			0x04

#undef CONFIG_USART0
#undef CONFIG_USART1
#undef CONFIG_USART2
#define CONFIG_USART3			1

/* User serviceable stuff */
#define CONFIG_DOS_PARTITION		1

#define CONFIG_CMDLINE_TAG		1
#define CONFIG_SETUP_MEMORY_TAGS	1
#define CONFIG_INITRD_TAG		1

#define CONFIG_STACKSIZE		(2048)

#define CONFIG_BAUDRATE			115200
#define CONFIG_BOOTARGS							\
	"root=/dev/mtdblock1 rootfstype=jffs fbmem=1800k"

#define CONFIG_BOOTCOMMAND						\
	"fsload; bootm $(fileaddr)"

/*
 * Only interrupt autoboot if <space> is pressed. Otherwise, garbage
 * data on the serial line may interrupt the boot sequence.
 */
#define CONFIG_BOOTDELAY		1
#define CONFIG_AUTOBOOT			1
#define CONFIG_AUTOBOOT_KEYED		1
#define CONFIG_AUTOBOOT_PROMPT		\
	"Press SPACE to abort autoboot in %d seconds\n", bootdelay
#define CONFIG_AUTOBOOT_DELAY_STR	"d"
#define CONFIG_AUTOBOOT_STOP_STR	" "

/*
 * After booting the board for the first time, new ethernet addresses
 * should be generated and assigned to the environment variables
 * "ethaddr" and "eth1addr". This is normally done during production.
 */
#define CONFIG_OVERWRITE_ETHADDR_ONCE	1
#define CONFIG_NET_MULTI		1

/*
 * BOOTP options
 */
#define CONFIG_BOOTP_SUBNETMASK
#define CONFIG_BOOTP_GATEWAY


/*
 * Command line configuration.
 */
#include <config_cmd_default.h>

#define CONFIG_CMD_ASKENV
#define CONFIG_CMD_DHCP
#define CONFIG_CMD_EXT2
#define CONFIG_CMD_FAT
#define CONFIG_CMD_JFFS2
#define CONFIG_CMD_MMC

#undef CONFIG_CMD_AUTOSCRIPT
#undef CONFIG_CMD_FPGA
#undef CONFIG_CMD_SETGETDCR
#undef CONFIG_CMD_XIMG

#define CONFIG_ATMEL_USART		1
#define CONFIG_MACB			1
#define CONFIG_PIO2			1
#define CFG_NR_PIOS			5
#define CFG_HSDRAMC			1
#define CONFIG_MMC			1
#define CONFIG_ATMEL_MCI		1

#define CFG_DCACHE_LINESZ		32
#define CFG_ICACHE_LINESZ		32

#define CONFIG_NR_DRAM_BANKS		1

/* External flash on Favr-32 */
#if 0
#define CFG_FLASH_CFI			1
#define CFG_FLASH_CFI_DRIVER		1
#endif

#define CFG_FLASH_BASE			0x00000000
#define CFG_FLASH_SIZE			0x800000
#define CFG_MAX_FLASH_BANKS		1
#define CFG_MAX_FLASH_SECT		135

#define CFG_MONITOR_BASE		CFG_FLASH_BASE

#define CFG_INTRAM_BASE			INTERNAL_SRAM_BASE
#define CFG_INTRAM_SIZE			INTERNAL_SRAM_SIZE
#define CFG_SDRAM_BASE			EBI_SDRAM_BASE

#define CFG_ENV_IS_IN_FLASH		1
#define CFG_ENV_SIZE			65536
#define CFG_ENV_ADDR			(CFG_FLASH_BASE + CFG_FLASH_SIZE - CFG_ENV_SIZE)

#define CFG_INIT_SP_ADDR		(CFG_INTRAM_BASE + CFG_INTRAM_SIZE)

#define CFG_MALLOC_LEN			(256*1024)
#define CFG_DMA_ALLOC_LEN		(16384)

/* Allow 4MB for the kernel run-time image */
#define CFG_LOAD_ADDR			(EBI_SDRAM_BASE + 0x00400000)
#define CFG_BOOTPARAMS_LEN		(16 * 1024)

/* Other configuration settings that shouldn't have to change all that often */
#define CFG_PROMPT			"U-Boot> "
#define CFG_CBSIZE			256
#define CFG_MAXARGS			16
#define CFG_PBSIZE			(CFG_CBSIZE + sizeof(CFG_PROMPT) + 16)
#define CFG_LONGHELP			1

#define CFG_MEMTEST_START		EBI_SDRAM_BASE
#define CFG_MEMTEST_END			(CFG_MEMTEST_START + 0x700000)
#define CFG_BAUDRATE_TABLE { 115200, 38400, 19200, 9600, 2400 }

#endif /* __CONFIG_H */
