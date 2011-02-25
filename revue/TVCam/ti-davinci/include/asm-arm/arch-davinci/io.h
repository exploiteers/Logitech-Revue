/*
 * DaVinci IO address definitions
 *
 * Copied from include/asm/arm/arch-omap/io.h
 *
 * 2007 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */
#ifndef __ASM_ARCH_IO_H
#define __ASM_ARCH_IO_H

#define IO_SPACE_LIMIT 0xffffffff

/*
 * Deep sleep mode IRAM
*/
#define IO_PHYS_IRAM   0x00000500
#define IO_OFFSET_IRAM 0xFEA00000
#define IO_SIZE_IRAM   0x00002000
#define IO_VIRT_IRAM   (IO_PHYS_IRAM + IO_OFFSET_IRAM)

/*
 * ----------------------------------------------------------------------------
 * I/O mapping
 * ----------------------------------------------------------------------------
 */
#define IO_PHYS		0x01c00000
#define IO_OFFSET	0xfa000000 /* Virtual IO = 0xfbc00000 */
#define IO_SIZE		0x00400000
#define IO_VIRT		(IO_PHYS + IO_OFFSET)
#define io_p2v(pa)	((pa) + IO_OFFSET)
#define io_v2p(va)	((va) - IO_OFFSET)
#define IO_ADDRESS(x)	io_p2v(x)

/*
 * DA8xx have their cp_intc interrupt controller mapped outside the IO_PHYS to
 * IO_PHYS + IO_SIZE - 1 range, so we're creating a fixed virtual mapping for
 * its registers.  The address is determined by the size of cp_intc register
 * region (which is 8 KB or 2 pages) and a 1-page hole to reduce the chance of
 * an invalid memory access.
 */
#define CP_INTC_SIZE		SZ_8K
#define CP_INTC_VIRT		(IO_VIRT - CP_INTC_SIZE - SZ_4K)

/*
 * We don't actually have real ISA nor PCI buses, but there is so many
 * drivers out there that might just work if we fake them...
 */
#define PCIO_BASE               0
#define __io(a)			((void __iomem *)(PCIO_BASE + (a)))
#define __mem_pci(a)		(a)
#define __mem_isa(a)		(a)

#ifndef __ASSEMBLER__

/*
 * Functions to access the DaVinci IO region
 *
 * NOTE: - Use davinci_read/write[bwl] for physical register addresses
 *	 - Use __raw_read/write[bwl]() for virtual register addresses
 *	 - Use IO_ADDRESS(phys_addr) to convert registers to virtual addresses
 *	 - DO NOT use hardcoded virtual addresses to allow changing the
 *	   IO address space again if needed
 */
#define davinci_readb(a)	(*(volatile unsigned char  *)IO_ADDRESS(a))
#define davinci_readw(a)	(*(volatile unsigned short *)IO_ADDRESS(a))
#define davinci_readl(a)	(*(volatile unsigned int   *)IO_ADDRESS(a))

#define davinci_writeb(v,a)	(*(volatile unsigned char  *)IO_ADDRESS(a) = (v))
#define davinci_writew(v,a)	(*(volatile unsigned short *)IO_ADDRESS(a) = (v))
#define davinci_writel(v,a)	(*(volatile unsigned int   *)IO_ADDRESS(a) = (v))

/* 16 bit uses LDRH/STRH, base +/- offset_8 */
typedef struct { volatile unsigned short offset[256]; } __regbase16;
#define __REGV16(vaddr)		((__regbase16 *)((vaddr)&~0xff)) \
					->offset[((vaddr)&0xff)>>1]
#define __REG16(paddr)          __REGV16(io_p2v(paddr))

/* 8/32 bit uses LDR/STR, base +/- offset_12 */
typedef struct { volatile unsigned char offset[4096]; } __regbase8;
#define __REGV8(vaddr)		((__regbase8  *)((vaddr)&~4095)) \
					->offset[((vaddr)&4095)>>0]
#define __REG8(paddr)		__REGV8(io_p2v(paddr))

typedef struct { volatile unsigned int offset[4096]; } __regbase32;
#define __REGV32(vaddr)		((__regbase32 *)((vaddr)&~4095)) \
					->offset[((vaddr)&4095)>>2]

#define __REG(paddr)		__REGV32(io_p2v(paddr))
#else

#define __REG(x)	(*((volatile unsigned long *)io_p2v(x)))

#endif /* __ASSEMBLER__ */
#endif /* __ASM_ARCH_IO_H */
