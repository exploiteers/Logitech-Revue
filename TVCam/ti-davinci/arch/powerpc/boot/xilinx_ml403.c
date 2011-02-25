/*
 * Xilinx ML403 bootwrapper
 *
 * Author: MontaVista Software, Inc.
 *	source@mvista.com
 *
 * 2008 (c) MontaVista, Software, Inc.  This file is licensed under
 * the terms of the GNU General Public License version 2.  This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */
#include <stdarg.h>
#include <stddef.h>
#include "types.h"
#include "gunzip_util.h"
#include "elf.h"
#include "string.h"
#include "stdio.h"
#include "page.h"
#include "ops.h"

extern char _start[];
extern char _end[];
extern char _dtb_start[];
extern char _dtb_end[];
extern char _vmlinux_start[], _vmlinux_end[];

#define KB	1024U
#define MB	(KB*KB)
#define MHz	(1000U*1000U)

#define HEAP_SIZE	(4*MB)
BSS_STACK(64 * KB);

asm("  .globl _zimage_start\n\
	_zimage_start:\n\
		mfccr0  0\n\
		oris    0,0,0x50000000@h\n\
		mtccr0  0\n\
		b _zimage_start_lib\n\
");

void platform_init(unsigned long r3, unsigned long r4, unsigned long r5)
{
	int dt_size = _dtb_end - _dtb_start;
	void *vmlinuz_addr = _vmlinux_start;
	unsigned long vmlinuz_size = _vmlinux_end - _vmlinux_start;
	char *heap_start, *dtb;
	struct elf_info ei;
	struct gunzip_state gzstate;
	char elfheader[256];
	static const unsigned long line_size = 32;
	static const unsigned long congruence_classes = 256;
	unsigned long addr;
	unsigned long dccr;

	if (dt_size <= 0)	/* No fdt */
		exit();

	/*
	 * Invalidate the data cache if the data cache is turned off.
	 * - The 405 core does not invalidate the data cache on power-up
	 *   or reset but does turn off the data cache. We cannot assume
	 *   that the cache contents are valid.
	 * - If the data cache is turned on this must have been done by
	 *   a bootloader and we assume that the cache contents are
	 *   valid.
	 */
      __asm__("mfdccr %0":"=r"(dccr));
	if (dccr == 0) {
		for (addr = 0;
		     addr < (congruence_classes * line_size);
		     addr += line_size) {
		      __asm__("dccci 0,%0": :"b"(addr));
		}
	}

	/*
	 * Start heap after end of the kernel (after decompressed to
	 * address 0) or the end of the zImage, whichever is higher.
	 * That's so things allocated by simple_alloc won't overwrite
	 * any part of the zImage and the kernel won't overwrite the dtb
	 * when decompressed & relocated.
	 */
	gunzip_start(&gzstate, vmlinuz_addr, vmlinuz_size);
	gunzip_exactly(&gzstate, elfheader, sizeof(elfheader));

	if (!parse_elf32(elfheader, &ei))
		exit();

	heap_start = (char *)(ei.memsize + ei.elfoffset);	/* end of kernel */
	heap_start = max(heap_start, (char *)_end);	/* end of zImage */

	if ((unsigned)
	    simple_alloc_init(heap_start, HEAP_SIZE, HEAP_SIZE / 8, 16)
	    > (32 * MB))
		exit();

	/* Relocate dtb to safe area past end of zImage & kernel */
	dtb = malloc(dt_size);
	if (!dtb)
		exit();
	memmove(dtb, _dtb_start, dt_size);
	if (ft_init(dtb, dt_size, 16))
		exit();

	if (serial_console_init() < 0)	/* serial_console_init uses
					 * serial_get_stdout_devp()
					 * to find "ns16550" compatible device */
		exit();
}
