/*
 * highmem.h: virtual kernel memory mappings for high memory
 *
 * PowerPC version, stolen from the i386 version.
 *
 * Used in CONFIG_HIGHMEM systems for memory pages which
 * are not addressable by direct kernel virtual addresses.
 *
 * Copyright (C) 1999 Gerhard Wichert, Siemens AG
 *		      Gerhard.Wichert@pdb.siemens.de
 *
 *
 * Redesigned the x86 32-bit VM architecture to deal with
 * up to 16 Terrabyte physical memory. With current x86 CPUs
 * we now support up to 64 Gigabytes physical RAM.
 *
 * Copyright (C) 1999 Ingo Molnar <mingo@redhat.com>
 */

#ifndef _ASM_HIGHMEM_H
#define _ASM_HIGHMEM_H

#ifdef __KERNEL__

#include <linux/init.h>
#include <linux/interrupt.h>
#include <asm/kmap_types.h>
#include <asm/tlbflush.h>
#include <asm/page.h>

/* undef for production */
#define HIGHMEM_DEBUG 1

extern pte_t *kmap_pte;
extern pgprot_t kmap_prot;
extern pte_t *pkmap_page_table;

/*
 * Right now we initialize only a single pte table. It can be extended
 * easily, subsequent pte tables have to be allocated in one physical
 * chunk of RAM.
 */
#define PKMAP_BASE 	CONFIG_HIGHMEM_START
#define LAST_PKMAP 	(1 << PTE_SHIFT)
#define LAST_PKMAP_MASK (LAST_PKMAP-1)
#define PKMAP_NR(virt)  ((virt-PKMAP_BASE) >> PAGE_SHIFT)
#define PKMAP_ADDR(nr)  (PKMAP_BASE + ((nr) << PAGE_SHIFT))

#define KMAP_FIX_BEGIN	(PKMAP_BASE + 0x00400000UL)

extern void *kmap_high(struct page *page);
extern void kunmap_high(struct page *page);

static inline void *kmap(struct page *page)
{
	if (!PageHighMem(page))
		return page_address(page);
	might_sleep();
	return kmap_high(page);
}

static inline void kunmap(struct page *page)
{
	BUG_ON(in_interrupt());
	if (!PageHighMem(page))
		return;
	kunmap_high(page);
}

static inline void kunmap_virt(void *ptr)
{
	struct page *page;

	if ((unsigned long)ptr < PKMAP_ADDR(0))
		return;
	page = pte_page(pkmap_page_table[PKMAP_NR((unsigned long)ptr)]);
	kunmap(page);
}

static inline struct page *kmap_to_page(void *ptr)
{
	struct page *page;

	if ((unsigned long)ptr < PKMAP_ADDR(0))
		return virt_to_page(ptr);
	page = pte_page(pkmap_page_table[PKMAP_NR((unsigned long)ptr)]);
	return page;
}

/*
 * The use of kmap_atomic/kunmap_atomic is discouraged - kmap/kunmap
 * gives a more generic (and caching) interface. But kmap_atomic can
 * be used in IRQ contexts, so in some (very limited) cases we need
 * it.
 */
static inline void *__kmap_atomic(struct page *page, enum km_type type)
{
	unsigned int idx;
	unsigned long vaddr;

	/* even !CONFIG_PREEMPT needs this, for in_atomic in do_page_fault */
	preempt_disable();
	if (!PageHighMem(page))
		return page_address(page);

	idx = type + KM_TYPE_NR*smp_processor_id();
	vaddr = KMAP_FIX_BEGIN + idx * PAGE_SIZE;
#ifdef HIGHMEM_DEBUG
	BUG_ON(!pte_none(*(kmap_pte+idx)));
#endif
	set_pte_at(&init_mm, vaddr, kmap_pte+idx, mk_pte(page, kmap_prot));
	flush_tlb_page(NULL, vaddr);

	return (void*) vaddr;
}

static inline void __kunmap_atomic(void *kvaddr, enum km_type type)
{
#ifdef HIGHMEM_DEBUG
	unsigned long vaddr = (unsigned long) kvaddr & PAGE_MASK;
	unsigned int idx = type + KM_TYPE_NR*smp_processor_id();

	if (vaddr < KMAP_FIX_BEGIN) { // FIXME
		preempt_enable();
		return;
	}

	BUG_ON(vaddr != KMAP_FIX_BEGIN + idx * PAGE_SIZE);

	/*
	 * force other mappings to Oops if they'll try to access
	 * this pte without first remap it
	 */
	pte_clear(&init_mm, vaddr, kmap_pte+idx);
	flush_tlb_page(NULL, vaddr);
#endif
	preempt_enable();
}

static inline struct page *__kmap_atomic_to_page(void *ptr)
{
	unsigned long idx, vaddr = (unsigned long) ptr;

	if (vaddr < KMAP_FIX_BEGIN)
		return virt_to_page(ptr);

	idx = (vaddr - KMAP_FIX_BEGIN) >> PAGE_SHIFT;
	return pte_page(kmap_pte[idx]);
}

#ifdef CONFIG_PREEMPT_RT

#define kmap_atomic(page, type)		kmap(page)
#define kunmap_atomic(kvaddr, type)	kunmap_virt(kvaddr)
#define kmap_atomic_to_page(kvaddr)	kmap_to_page(kvaddr)

#else

#define kmap_atomic(page, type)		__kmap_atomic(page, type)
#define kunmap_atomic(kvaddr, type)	__kunmap_atomic(kvaddr, type)
#define kmap_atomic_to_page(kvaddr)	__kmap_atomic_to_page(kvaddr)

#endif

#define flush_cache_kmaps()	flush_cache_all()

#endif /* __KERNEL__ */

#endif /* _ASM_HIGHMEM_H */
