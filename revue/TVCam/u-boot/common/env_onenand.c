/*
 * (C) Copyright 2005-2007 Samsung Electronics
 * Kyungmin Park <kyungmin.park@samsung.com>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>

#if defined(CFG_ENV_IS_IN_ONENAND)	/* Environment is in OneNAND */

#include <command.h>
#include <environment.h>
#include <linux/stddef.h>
#include <malloc.h>

#include <linux/mtd/compat.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/onenand.h>

extern struct mtd_info onenand_mtd;
extern struct onenand_chip onenand_chip;

/* References to names in env_common.c */
extern uchar default_environment[];

#define ONENAND_ENV_SIZE(mtd)	(mtd.writesize - ENV_HEADER_SIZE)

char *env_name_spec = "OneNAND";

#ifdef ENV_IS_EMBEDDED
extern uchar environment[];
env_t *env_ptr = (env_t *) (&environment[0]);
#else /* ! ENV_IS_EMBEDDED */
static unsigned char onenand_env[MAX_ONENAND_PAGESIZE];
env_t *env_ptr = (env_t *) onenand_env;
#endif /* ENV_IS_EMBEDDED */

DECLARE_GLOBAL_DATA_PTR;

uchar env_get_char_spec(int index)
{
	return (*((uchar *) (gd->env_addr + index)));
}

void env_relocate_spec(void)
{
	unsigned long env_addr;
	int use_default = 0;
	size_t retlen;

	env_addr = CFG_ENV_ADDR;

	/* Check OneNAND exist */
	if (onenand_mtd.writesize)
		/* Ignore read fail */
		onenand_read(&onenand_mtd, env_addr, onenand_mtd.writesize,
			     &retlen, (u_char *) env_ptr);
	else
		onenand_mtd.writesize = MAX_ONENAND_PAGESIZE;

	if (crc32(0, env_ptr->data, ONENAND_ENV_SIZE(onenand_mtd)) !=
	    env_ptr->crc)
		use_default = 1;

	if (use_default) {
		memcpy(env_ptr->data, default_environment,
		       ONENAND_ENV_SIZE(onenand_mtd));
		env_ptr->crc =
		    crc32(0, env_ptr->data, ONENAND_ENV_SIZE(onenand_mtd));
	}

	gd->env_addr = (ulong) & env_ptr->data;
	gd->env_valid = 1;
}

int saveenv(void)
{
	unsigned long env_addr = CFG_ENV_ADDR;
	struct erase_info instr = {
		.callback	= NULL,
	};
	size_t retlen;

	instr.len = CFG_ENV_SIZE;
	instr.addr = env_addr;
	if (onenand_erase(&onenand_mtd, &instr)) {
		printf("OneNAND: erase failed at 0x%08lx\n", env_addr);
		return 1;
	}

	/* update crc */
	env_ptr->crc =
	    crc32(0, env_ptr->data, ONENAND_ENV_SIZE(onenand_mtd));

	if (onenand_write(&onenand_mtd, env_addr, onenand_mtd.writesize, &retlen,
	     (u_char *) env_ptr)) {
		printf("OneNAND: write failed at 0x%08x\n", instr.addr);
		return 2;
	}

	return 0;
}

int env_init(void)
{
	/* use default */
	gd->env_addr = (ulong) & default_environment[0];
	gd->env_valid = 1;

	return 0;
}

#endif /* CFG_ENV_IS_IN_ONENAND */
