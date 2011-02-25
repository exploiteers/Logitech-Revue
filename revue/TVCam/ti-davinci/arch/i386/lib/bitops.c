
#include <linux/mm.h>
#include <linux/highmem.h>
#include <linux/blkdev.h>
#include <linux/module.h>
#include <linux/bitops.h>
#include <asm/uaccess.h>
#include <asm/mmx.h>

/**
 * find_next_bit - find the first set bit in a memory region
 * @addr: The address to base the search on
 * @offset: The bitnumber to start searching at
 * @size: The maximum size to search
 */
int find_next_bit(const unsigned long *addr, int size, int offset)
{
	const unsigned long *p = addr + (offset >> 5);
	int set = 0, bit = offset & 31, res;

	if (bit) {
		/*
		 * Look for nonzero in the first 32 bits:
		 */
		__asm__("bsfl %1,%0\n\t"
			"jne 1f\n\t"
			"movl $32, %0\n"
			"1:"
			: "=r" (set)
			: "r" (*p >> bit));
		if (set < (32 - bit))
			return set + offset;
		set = 32 - bit;
		p++;
	}
	/*
	 * No set bit yet, search remaining full words for a bit
	 */
	res = find_first_bit (p, size - 32 * (p - addr));
	return (offset + set + res);
}
EXPORT_SYMBOL(find_next_bit);

/**
 * find_next_zero_bit - find the first zero bit in a memory region
 * @addr: The address to base the search on
 * @offset: The bitnumber to start searching at
 * @size: The maximum size to search
 */
int find_next_zero_bit(const unsigned long *addr, int size, int offset)
{
	unsigned long * p = ((unsigned long *) addr) + (offset >> 5);
	int set = 0, bit = offset & 31, res;

	if (bit) {
		/*
		 * Look for zero in the first 32 bits.
		 */
		__asm__("bsfl %1,%0\n\t"
			"jne 1f\n\t"
			"movl $32, %0\n"
			"1:"
			: "=r" (set)
			: "r" (~(*p >> bit)));
		if (set < (32 - bit))
			return set + offset;
		set = 32 - bit;
		p++;
	}
	/*
	 * No zero yet, search remaining full bytes for a zero
	 */
	res = find_first_zero_bit (p, size - 32 * (p - (unsigned long *) addr));
	return (offset + set + res);
}
EXPORT_SYMBOL(find_next_zero_bit);


/*
 * rw spinlock fallbacks
 */
#ifdef CONFIG_SMP
asm(
".section .sched.text, \"ax\"\n"
".align	4\n"
".globl	__write_lock_failed\n"
"__write_lock_failed:\n\t"
	"lock; addl	$" RW_LOCK_BIAS_STR ",(%eax)\n"
"1:	rep; nop\n\t"
	"cmpl	$" RW_LOCK_BIAS_STR ",(%eax)\n\t"
	"jne	1b\n\t"
	"lock; subl	$" RW_LOCK_BIAS_STR ",(%eax)\n\t"
	"jnz	__write_lock_failed\n\t"
	"ret\n"
".previous\n"
);

asm(
".section .sched.text, \"ax\"\n"
".align	4\n"
".globl	__read_lock_failed\n"
"__read_lock_failed:\n\t"
	"lock; incl	(%eax)\n"
"1:	rep; nop\n\t"
	"cmpl	$1,(%eax)\n\t"
	"js	1b\n\t"
	"lock; decl	(%eax)\n\t"
	"js	__read_lock_failed\n\t"
	"ret\n"
".previous\n"
);
#endif
