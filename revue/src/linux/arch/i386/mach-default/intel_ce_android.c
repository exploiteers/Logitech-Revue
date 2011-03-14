/*
 * Low-level init/reboot code for systems based on Intel CE 3100/4100 SoCs.
 * Some parts are Android specific.
 *
 * Copyright (c) 2010 Google, Inc.
 * Author: Eugene Surovegin <es@google.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */
#include <linux/err.h>
#include <linux/flash_ts.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>
#include <linux/mtd/mtd.h>
#include <linux/reboot.h>
#include <linux/string.h>

#include <asm/setup.h>

#include "logitech_kmod_keys.h"

#define ONE_MB		(1024 * 1024)

static int bcb_fts_reboot_hook(struct notifier_block*, unsigned long, void*);

/* We need to reserve chunk at the top of system memory for Intel SDK.
 * Different boards may have different requirements.
 * There are also per-board flash layouts.
 */
static __initdata struct board_config {
	char *name;
	unsigned revision;
	unsigned long sdk_pool; /* how much memory is reserved, normal boot */
	unsigned long sdk_pool_recovery; 	       /* ... recovery boot */

	/* MTD layout(s) in cmdlinepart format */
	const char *mtdparts;

	/* optional MTD layout(s) in cmdlinepart format for recovery mode */
	const char *mtdparts_recovery;

	/* default root device in case nothing was specified */
	const char *default_root;

	/* do not delay mounting devices from this (internal) usb bus */
	const char *nodelay_bus_name;

	/* kernel module keys */
	struct rsa_key *kmod_key;

	/* optional reboot hook */
	int (*reboot_notifier)(struct notifier_block*, unsigned long, void*);

} boards[] = {
	{ .name = "tatung4",
	  .sdk_pool = 328 * ONE_MB,
	  .sdk_pool_recovery = 256 * ONE_MB,
	  .default_root = "/dev/mtdblock:boot",
	  .reboot_notifier = bcb_fts_reboot_hook,

	   /* 1G NAND, 4K pages, 512K block */
#define TATUNG4_MTDPARTS(__RO__)					\
		"intel_ce_nand:"					\
		"4m(mbr)ro,"			/* 8x redundancy */	\
		"4m(cefdk-s1)" __RO__ ","	/* 8x redundancy */	\
		"12m(cefdk-s2)" __RO__ ","	/* 8x redundancy */	\
		"4m(redboot)" __RO__ ","	/* 8x redundancy */	\
		"4m(cefdk-config)ro,"		/* 8x redundancy */	\
		/* 4M gap (blocks 56 - 63) */				\
		"8m@32m(splash),"		/* 4x redundancy */	\
		"2m(fts)ro,"						\
		"20m(recovery),"		/* 5 spare blocks */	\
		"5m(kernel)" __RO__ ","		/* 3 spare blocks */	\
		"64m(boot)" __RO__ ","					\
		"300m(system),"						\
		"431m(userdata),"					\
		"160m(cache),"						\
		"2m(bbt)ro"

	  /* we want to limit write access to important partitions
	   * in non-recovery mode
	   */
	  .mtdparts = TATUNG4_MTDPARTS("ro"),
	  .mtdparts_recovery = TATUNG4_MTDPARTS(""),
	},


	/* Samsung K9F8G08U0M (1G NAND, 4K pages, 256K block) */
#define KA_256K_MTDPARTS(__RO__, __RODEV__)				\
		"intel_ce_nand:"					\
		"2m(mbr)ro,"			/* 8x redundancy */	\
		"8m(cefdk)" __RO__ ","		/* 8x redundancy */	\
		"2m(redboot)" __RO__ ","	/* 8x redundancy */	\
		"2m(cefdk-config)ro,"		/* 8x redundancy */	\
		/* 2M gap (blocks 56 - 63) */				\
		"8m@16m(splash)" __RO__ ","	/* 4x redundancy */	\
		"1m(fts)ro,"			/* 3 spare blocks */	\
		"20m(recovery),"		/* 5 spare blocks */	\
		"5m(kernel)" __RO__ ","		/* 3 spare blocks */	\
		"64m(boot)" __RO__ ","					\
		"384m(system)" __RODEV__ ","			       	\
		"520m(data),"						\
		"5m(keystore)" __RODEV__ ","	/* not android */	\
	        "1m(bbt)ro"						\

	/* Micron MT29F8G08ABABA (1G NAND, 4K pages, 512K block) */
#define KA_512K_MTDPARTS(__RO__, __RODEV__)				\
		"intel_ce_nand:"					\
		"4m(mbr)ro,"			/* 8x redundancy */	\
		"16m(cefdk)" __RO__ ","		/* 8x redundancy */	\
		"4m(redboot)" __RO__ ","	/* 8x redundancy */	\
		"4m(cefdk-config)ro,"		/* 8x redundancy */	\
		/* 4M gap (blocks 56 - 63) */				\
		"8m@32m(splash)" __RO__ ","	/* 4x redundancy */	\
		"2m(fts)ro,"			/* 3 spare blocks */	\
		"20m(recovery),"		/* 5 spare blocks */	\
		"5m(kernel)" __RO__ ","		/* 3 spare blocks */	\
		"64m(boot)" __RO__ ","					\
		"384m(system)" __RODEV__ ","			       	\
		"502m(data),"						\
		"5m(keystore)" __RODEV__ ","	/* not android */	\
		"2m(bbt)ro"						\

#define KA_COMMON(__NAME__, __REVISION__, __KMOD_KEY__,                 \
		  __MTDPARTS__, __MTDPARTS_RECOVERY__)                  \
	{ .name = __NAME__,                                             \
	  .revision = __REVISION__,                                     \
	  .sdk_pool = 328 * ONE_MB,                                     \
	  .sdk_pool_recovery = 256 * ONE_MB,                            \
	  .default_root = "/dev/mtdblock:boot",                         \
	  .reboot_notifier = bcb_fts_reboot_hook,                       \
	  .nodelay_bus_name = "0000:01:0d.0",                           \
	  .kmod_key = __KMOD_KEY__,                                     \
	  .mtdparts = __MTDPARTS__,                                     \
	  .mtdparts_recovery = __MTDPARTS_RECOVERY__,                   \
	},

	/* PB2 */
	KA_COMMON("logitech_ka3", 0,
		  &ka3_rsa_key,
		  KA_256K_MTDPARTS("ro", ""),
		  KA_256K_MTDPARTS("", ""))

	KA_COMMON("logitech_ka3", 1,
		  &ka3_rsa_key,
		  KA_512K_MTDPARTS("ro", ""),
		  KA_512K_MTDPARTS("", ""))

	/* PB3 */
	KA_COMMON("logitech_ka4", 0,
		  &ka3_rsa_key,
		  KA_256K_MTDPARTS("ro", ""),
		  KA_256K_MTDPARTS("", ""))

	KA_COMMON("logitech_ka4", 1,
		  &ka3_rsa_key,
		  KA_512K_MTDPARTS("ro", ""),
		  KA_512K_MTDPARTS("", ""))

	/* PB4/MP */
	KA_COMMON("logitech_ka5", 0,
		  &ka5_rsa_key,
		  KA_256K_MTDPARTS("ro", "ro"),
		  KA_256K_MTDPARTS("", ""))

	KA_COMMON("logitech_ka5", 1,
		  &ka5_rsa_key,
		  KA_512K_MTDPARTS("ro", "ro"),
		  KA_512K_MTDPARTS("", ""))
};

/* This is what we will use if nothing has been passed by bootloader
 * or name wasn't recognized.
 */
#define DEFAULT_BOARD	"tatung4"
#define DEFAULT_REVISION 0
static __initdata char board_name[32] = DEFAULT_BOARD;
static __initdata unsigned board_revision;

static __initdata char console[32];
static __initdata char boot_mode[32];
static __initdata char root[32];

/* Optional reboot notfier we use to write BCB */
static struct notifier_block reboot_notifier;

static void __init add_boot_param(const char *param, const char *val)
{
	if (boot_command_line[0])
		strlcat(boot_command_line, " ", COMMAND_LINE_SIZE);
	strlcat(boot_command_line, param, COMMAND_LINE_SIZE);
	if (val) {
		strlcat(boot_command_line, "=", COMMAND_LINE_SIZE);
		strlcat(boot_command_line, val, COMMAND_LINE_SIZE);
	}
}

static int __init do_param(char *param, char *val)
{
	/* Skip all mem= and memmap= parameters */
	if (!strcmp(param, "mem") || !strcmp(param, "memmap"))
		return 0;

	/* Capture some params we will need later */
	if (!strcmp(param, "androidboot.hardware") && val) {
		strlcpy(board_name, val, sizeof(board_name));
		return 0;
	}

	if (!strcmp(param, "androidboot.mode") && val) {
		strlcpy(boot_mode, val, sizeof(boot_mode));
		return 0;
	}

	if (!strcmp(param, "androidboot.revision") && val) {
		board_revision = simple_strtoul(val, NULL, 0);
	}

	if (!strcmp(param, "console") && val)
		strlcpy(console, val, sizeof(console));

	if (!strcmp(param, "root") && val)
		strlcpy(root, val, sizeof(root));

	/* Re-add this parameter back */
	add_boot_param(param, val);

	return 0;
}

static __init struct board_config *get_board_config(const char *name,
						    int revision)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(boards); ++i)
		if (!strcmp(name, boards[i].name) &&
		    revision == boards[i].revision)
			return boards + i;
	return NULL;
}

char* __init memory_setup(void)
{
	static __initdata char tmp[COMMAND_LINE_SIZE];
	struct board_config *cfg;
	unsigned long reserve, memory_total, memory_linux;
	const char *mtdparts;
	int recovery_boot;

	/* Scan boot command line removing all memory layout
	 * parameters on the way.
	 */
	strlcpy(tmp, boot_command_line, sizeof(tmp));
	boot_command_line[0] = '\0';
	parse_args("memory_setup", tmp, NULL, 0, do_param);

	/* Try to determine if we are booting in recovery mode */
	if (!boot_mode[0]) {
		/* Old bootloader, which didn't pass "androidboot.mode".
		 * In this case we use root device as a hint - recovery
		 * image uses initrd instead of /dev/sda.
		 */
		if (!strcmp(root, "/dev/ram0"))
			strlcpy(boot_mode, "recovery", sizeof(boot_mode));
	}
	recovery_boot = !strcmp(boot_mode, "recovery");
	if (recovery_boot)
		printk(KERN_INFO "Recovery boot detected\n");

	/* Board-specific config */
	cfg = get_board_config(board_name, board_revision);
	if (!cfg) {
		printk(KERN_WARNING "Unknown board name: '%s', using '"
		       DEFAULT_BOARD "'\n", board_name);
		cfg = get_board_config(DEFAULT_BOARD, DEFAULT_REVISION);
		strlcpy(board_name, DEFAULT_BOARD, sizeof(board_name));
		BUG_ON(!cfg);
	}
	reserve = recovery_boot ? cfg->sdk_pool_recovery : cfg->sdk_pool;

	/* Our only memory config option is e801,
	 * firmware doesn't seem to bother with e820 map.
	 * Note that first 1M is reserved for 8051 in CE4100.
	 */
	memory_total = ALT_MEM_K * 1024;  /* this doesn't include the first 1MB */

	/* Ancient bootloaders pass garbage here, do a basic sanity check */
	if (memory_total < 4 * ONE_MB) {
		printk(KERN_WARNING
			"Incorrect e801 memory size 0x%08lx, using 767 MB\n",
			memory_total);
		memory_total = 767 * ONE_MB;
	}

	if (reserve >= memory_total) {
		printk(KERN_ERR "Not enough memory to reserve %lu MB for SDK\n",
		       reserve / ONE_MB);
		memory_linux = memory_total;
		reserve = 0;
	} else {
		memory_linux = memory_total - reserve;
	}

	add_memory_region(HIGH_MEMORY, memory_linux, E820_RAM);
	if (reserve)
		add_memory_region(HIGH_MEMORY + memory_linux, reserve,
				  E820_RESERVED);

	/* Add root device if it wasn't specified explicitly */
	if (!root[0])
		add_boot_param("root", cfg->default_root);

	/* Flash layout (in cmdlinepart format) */
	mtdparts = cfg->mtdparts;
	if (recovery_boot && cfg->mtdparts_recovery)
		mtdparts = cfg->mtdparts_recovery;
	if (mtdparts)
		add_boot_param("mtdparts", mtdparts);

	/* Add some Android-specific parameters */
	add_boot_param("androidboot.hardware", board_name);
	if (boot_mode[0])
		add_boot_param("androidboot.mode", boot_mode);
	if (console[0]) {
		/* keep only console device name, e.g. ttyS0 */
		char *p = strchr(console, ',');
		if (p)
			*p = '\0';
		add_boot_param("androidboot.console", console);
	}

	/* Add usb bus name for internal usb disk */
	if (cfg->nodelay_bus_name) {
		add_boot_param("usb-storage.nodelay_bus_name", cfg->nodelay_bus_name);
	}

	/* Add kernel module keys */
	if (cfg->kmod_key) {
		rsa_install_key(cfg->kmod_key);
	}

	/* Register optional reboot hook */
	if (cfg->reboot_notifier) {
		reboot_notifier.notifier_call = cfg->reboot_notifier;
		register_reboot_notifier(&reboot_notifier);
	}

	return "BIOS-e801";
}

/* This is to prevent "Unknown boot option" messages */
static int __init dummy_param(char *arg) { return 0; }
__setup_param("androidboot.bootloader", dummy_1, dummy_param, 1);
__setup_param("androidboot.console", dummy_2, dummy_param, 1);
__setup_param("androidboot.hardware", dummy_3, dummy_param, 1);
__setup_param("androidboot.mode", dummy_4, dummy_param, 1);
__setup_param("androidboot.revision", dummy_5, dummy_param, 1);
__setup_param("androidboot.serialno", dummy_6, dummy_param, 1);

static int bcb_fts_reboot_hook(struct notifier_block *notifier,
			       unsigned long code, void *cmd)
{
	if (code == SYS_RESTART && cmd && !strcmp(cmd, "recovery")) {
		if (flash_ts_set("bootloader.command", "boot-recovery") ||
		    flash_ts_set("bootloader.status", "") ||
		    flash_ts_set("bootloader.recovery", ""))
		{
			printk(KERN_ERR "Failed to set bootloader command\n");
		}
	}

	return NOTIFY_DONE;
}
