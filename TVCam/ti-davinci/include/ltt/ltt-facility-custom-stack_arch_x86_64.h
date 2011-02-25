#ifndef _ASM_LTT_FACILITY_CUSTOM_STACK_H_
#define _ASM_LTT_FACILITY_CUSTOM_STACK_H_

#include <linux/types.h>
#include <ltt/ltt-tracer.h>
#include <linux/sched.h>
#include <linux/thread_info.h>
#include <linux/ptrace.h>
#include <asm/desc.h>
#include <asm/user32.h>

#ifndef CONFIG_LTT_PROCESS_MAX_FUNCTION_STACK
#define CONFIG_LTT_PROCESS_MAX_FUNCTION_STACK 0
#endif

#ifndef CONFIG_LTT_PROCESS_MAX_STACK_LEN
#define CONFIG_LTT_PROCESS_MAX_STACK_LEN 0
#endif

/* Event dump structures */
typedef struct lttng_sequence_stack_process_dump_32_eip lttng_sequence_stack_process_dump_32_eip;
struct lttng_sequence_stack_process_dump_32_eip {
	unsigned int len;
	const uint32_t *array;
};

static inline size_t lttng_get_alignment_sequence_stack_process_dump_32_eip(
		lttng_sequence_stack_process_dump_32_eip *obj)
{
	size_t align=0, localign;
	localign = sizeof(unsigned int);
	align = max(align, localign);

	localign = sizeof(const uint32_t);
	align = max(align, localign);

	return align;
}

static inline void lttng_write_custom_sequence_stack_process_dump_32_eip(
		char *buffer,
		size_t *to_base,
		size_t *to,
		const char **from,
		size_t *len,
		lttng_sequence_stack_process_dump_32_eip *obj)
{
	size_t align;

	/* Flush pending memcpy */
	if (*len != 0) {
		if (buffer != NULL)
			memcpy(buffer+*to_base+*to, *from, *len);
	}
	*to += *len;
	*len = 0;

	align = lttng_get_alignment_sequence_stack_process_dump_32_eip(obj);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	/* Contains variable sized fields : must explode the structure */

	/* Copy members */
	align = sizeof(unsigned int);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}
	
	*len += sizeof(unsigned int);

	if (buffer != NULL)
		memcpy(buffer+*to_base+*to, &obj->len, *len);
	else {
		/* Just calculate the size of the sequence */
		struct pt_regs *regs = (struct pt_regs*)obj->array;
		uint32_t addr;
		uint32_t *stack, *prev_stack, *beg_stack;
		obj->len = 1;
		
		/* For disabling multithread process */
		//if (!thread_group_leader(current))
		//	goto size_is_thread;

		/* Start at the top of the user stack */
		beg_stack = prev_stack = stack = (uint32_t*)regs->rsp;

		/* Keep on going until we reach the end of the process' stack limit or find
		 * a invalid rbp. */
		while (!get_user(addr, stack)) {
			/* Check if the page is write protected : guard page for thread */
			//if (!((unsigned long)stack&~PAGE_MASK)) {
				//pte_t *pte = lookup_address((unsigned long)stack);
				//if (!pte || (pte->pte_low & _PAGE_PROTNONE)) {
				//	break;
				//}
				//struct vm_area_struct *vma;
				//vma = find_vma(current->mm, (unsigned long)stack);
				//if (!vma ||
				//	vma->vm_start > (unsigned long)stack ||
				//	pgprot_val(vma->vm_page_prot) & _PAGE_PROTNONE) {
				//	printk("Encoutered guard page\n");
				//	break;
				//}
			//}
			/* Does this LOOK LIKE an address in the program */
			if (addr > current->mm->start_code && addr < current->mm->end_code) {
				obj->len++;
				prev_stack = stack;
			}
			/* Go on to the next address */
			stack++;
			if (stack > prev_stack + CONFIG_LTT_PROCESS_MAX_FUNCTION_STACK) break;
			if (stack > beg_stack + CONFIG_LTT_PROCESS_MAX_STACK_LEN) break;
		}
	}
//size_is_thread:
	*to += *len;
	*len = 0;

	align = sizeof(uint32_t);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	*len += sizeof(uint32_t);

	*len = obj->len * (*len);
	if (buffer != NULL) {
		/* Copy the data into the sequence */
		struct pt_regs *regs = (struct pt_regs*)obj->array;
		uint32_t addr;
		uint32_t *stack;
		uint32_t *dest = (uint32_t*)(buffer+*to_base+*to);
		
		*dest = regs->rip;
		dest++;

		/* For disabling multithread process */
		//if (!thread_group_leader(current))
		//	goto write_is_thread;

		/* Start at the top of the user stack */
		stack = (uint32_t*) regs->rsp;
		
		/* Keep on going until we reach the end of the process' stack limit or find
		 * a invalid rbp. */
		while (!get_user(addr, stack)) {
			/* Check if the page is write protected : guard page for thread */
			//if (!((unsigned long)stack&~PAGE_MASK)) {
				//pte_t *pte = lookup_address((unsigned long)stack);
				//if (!pte || (pte->pte_low & _PAGE_PROTNONE)) {
				//	break;
				//}
				//struct vm_area_struct *vma;
				//vma = find_vma(current->mm, (unsigned long)stack);
				//if (!vma ||
				//	vma->vm_start > (unsigned long)stack ||
				//	pgprot_val(vma->vm_page_prot) & _PAGE_PROTNONE) {
				//	printk("Encoutered guard page\n");
				//	break;
				//}
			//}
			/* If another thread corrupts the thread user space stack concurrently
			   Also used as stop condition when maximum of longs between pointers is reached.
			   */
			if (dest == (uint32_t*)(buffer+*to_base+*to) + obj->len) break;
			/* Does this LOOK LIKE an address in the program */
			if (addr > current->mm->start_code && addr < current->mm->end_code) {
				*dest = addr;
				dest++;
			}
			/* Go on to the next address */
			stack++;
		}
		/* Concurrently been changed : pad with zero */
		while (dest < (uint32_t*)(buffer+*to_base+*to) + obj->len) {
			*dest = 0;
			dest++;
		}
	}
//write_is_thread:
	*to += *len;
	*len = 0;


	/* Realign the *to_base on arch size, set *to to 0 */
	*to += ltt_align(*to, sizeof(void *));
	*to_base = *to_base+*to;
	*to = 0;
}

/* Event dump structures */
typedef struct lttng_sequence_stack_process_dump_64_eip lttng_sequence_stack_process_dump_64_eip;
struct lttng_sequence_stack_process_dump_64_eip {
	unsigned int len;
	const uint64_t *array;
};

static inline size_t lttng_get_alignment_sequence_stack_process_dump_64_eip(
		lttng_sequence_stack_process_dump_64_eip *obj)
{
	size_t align=0, localign;
	localign = sizeof(unsigned int);
	align = max(align, localign);

	localign = sizeof(const uint64_t);
	align = max(align, localign);

	return align;
}

static inline void lttng_write_custom_sequence_stack_process_dump_64_eip(
		char *buffer,
		size_t *to_base,
		size_t *to,
		const char **from,
		size_t *len,
		lttng_sequence_stack_process_dump_64_eip *obj)
{
	size_t align;

	/* Flush pending memcpy */
	if (*len != 0) {
		if (buffer != NULL)
			memcpy(buffer+*to_base+*to, *from, *len);
	}
	*to += *len;
	*len = 0;

	align = lttng_get_alignment_sequence_stack_process_dump_64_eip(obj);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	/* Contains variable sized fields : must explode the structure */

	/* Copy members */
	align = sizeof(unsigned int);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}
	
	*len += sizeof(unsigned int);

	if (buffer != NULL)
		memcpy(buffer+*to_base+*to, &obj->len, *len);
	else {
		/* Just calculate the size of the sequence */
		struct pt_regs *regs = (struct pt_regs*)obj->array;
		uint64_t addr;
		uint64_t *stack, *prev_stack, *beg_stack;
		obj->len = 1;
		
		/* For disabling multithread process */
		//if (!thread_group_leader(current))
		//	goto size_is_thread;

		/* Start at the top of the user stack */
		beg_stack = prev_stack = stack = (uint64_t*) regs->rsp;

		/* Keep on going until we reach the end of the process' stack limit or find
		 * a invalid rbp. */
		while (!get_user(addr, stack)) {
			/* Check if the page is write protected : guard page for thread */
			//if (!((unsigned long)stack&~PAGE_MASK)) {
				//pte_t *pte = lookup_address((unsigned long)stack);
				//if (!pte || (pte->pte_low & _PAGE_PROTNONE)) {
				//	break;
				//}
				//struct vm_area_struct *vma;
				//vma = find_vma(current->mm, (unsigned long)stack);
				//if (!vma ||
				//	vma->vm_start > (unsigned long)stack ||
				//	pgprot_val(vma->vm_page_prot) & _PAGE_PROTNONE) {
				//	printk("Encoutered guard page\n");
				//	break;
				//}
			//}
			/* Does this LOOK LIKE an address in the program */
			if (addr > current->mm->start_code && addr < current->mm->end_code) {
				obj->len++;
				prev_stack = stack;
			}
			/* Go on to the next address */
			stack++;
			if (stack > prev_stack + CONFIG_LTT_PROCESS_MAX_FUNCTION_STACK) break;
			if (stack > beg_stack + CONFIG_LTT_PROCESS_MAX_STACK_LEN) break;
		}
	}
//size_is_thread:
	*to += *len;
	*len = 0;

	align = sizeof(uint64_t);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	*len += sizeof(uint64_t);

	*len = obj->len * (*len);
	if (buffer != NULL) {
		/* Copy the data into the sequence */
		struct pt_regs *regs = (struct pt_regs*)obj->array;
		uint64_t addr;
		uint64_t *stack;
		uint64_t *dest = (uint64_t*)(buffer+*to_base+*to);
		
		*dest = regs->rip;
		dest++;

		/* For disabling multithread process */
		//if (!thread_group_leader(current))
		//	goto write_is_thread;

		/* Start at the top of the user stack */
		stack = (uint64_t*) regs->rsp;
		
		/* Keep on going until we reach the end of the process' stack limit or find
		 * a invalid rbp. */
		while (!get_user(addr, stack)) {
			/* Check if the page is write protected : guard page for thread */
			//if (!((unsigned long)stack&~PAGE_MASK)) {
				//pte_t *pte = lookup_address((unsigned long)stack);
				//if (!pte || (pte->pte_low & _PAGE_PROTNONE)) {
				//	break;
				//}
				//struct vm_area_struct *vma;
				//vma = find_vma(current->mm, (unsigned long)stack);
				//if (!vma ||
				//	vma->vm_start > (unsigned long)stack ||
				//	pgprot_val(vma->vm_page_prot) & _PAGE_PROTNONE) {
				//	printk("Encoutered guard page\n");
				//	break;
				//}
			//}
			/* If another thread corrupts the thread user space stack concurrently */
			if (dest == (uint64_t*)(buffer+*to_base+*to) + obj->len) break;
			/* Does this LOOK LIKE an address in the program */
			if (addr > current->mm->start_code && addr < current->mm->end_code) {
				*dest = addr;
				dest++;
			}
			/* Go on to the next address */
			stack++;
		}
		/* Concurrently been changed : pad with zero */
		while (dest < (uint64_t*)(buffer+*to_base+*to) + obj->len) {
			*dest = 0;
			dest++;
		}
	}
//write_is_thread:
	*to += *len;
	*len = 0;


	/* Realign the *to_base on arch size, set *to to 0 */
	*to += ltt_align(*to, sizeof(void *));
	*to_base = *to_base+*to;
	*to = 0;
}

/* Kernel specific trace stack dump routines, from arch/x86_64/kernel/traps.c */

/* Event kernel_dump structures 
 * ONLY USE IT ON THE CURRENT STACK.
 * It does not protect against other stack modifications and _will_
 * cause corruption upon race between threads.
 * Also make sure that the stack (stack pointer and up) will not be modified
 * between the "size calculation" and "buffer write" calls. */
typedef struct lttng_sequence_stack_kernel_dump_eip lttng_sequence_stack_kernel_dump_eip;
struct lttng_sequence_stack_kernel_dump_eip {
	unsigned int len;
	const unsigned long *array;
	unsigned long *stack;
};


/* From traps.c */

static unsigned long *trace_in_exception_stack(unsigned cpu, unsigned long stack,
					unsigned *usedp, const char **idp)
{
	static char ids[][8] = {
		[DEBUG_STACK - 1] = "#DB",
		[NMI_STACK - 1] = "NMI",
		[DOUBLEFAULT_STACK - 1] = "#DF",
		[STACKFAULT_STACK - 1] = "#SS",
		[MCE_STACK - 1] = "#MC",
#if DEBUG_STKSZ > EXCEPTION_STKSZ
		[N_EXCEPTION_STACKS ... N_EXCEPTION_STACKS + DEBUG_STKSZ / EXCEPTION_STKSZ - 2] = "#DB[?]"
#endif
	};
	unsigned k;

	for (k = 0; k < N_EXCEPTION_STACKS; k++) {
		unsigned long end;

		switch (k + 1) {
#if DEBUG_STKSZ > EXCEPTION_STKSZ
		case DEBUG_STACK:
			end = cpu_pda(cpu)->debugstack + DEBUG_STKSZ;
			break;
#endif
		default:
			end = per_cpu(init_tss, cpu).ist[k];
			break;
		}
		if (stack >= end)
			continue;
		if (stack >= end - EXCEPTION_STKSZ) {
			if (*usedp & (1U << k))
				break;
			*usedp |= 1U << k;
			*idp = ids[k];
			return (unsigned long *)end;
		}
#if DEBUG_STKSZ > EXCEPTION_STKSZ
		if (k == DEBUG_STACK - 1 && stack >= end - DEBUG_STKSZ) {
			unsigned j = N_EXCEPTION_STACKS - 1;

			do {
				++j;
				end -= EXCEPTION_STKSZ;
				ids[j][4] = '1' + (j - N_EXCEPTION_STACKS);
			} while (stack < end - EXCEPTION_STKSZ);
			if (*usedp & (1U << j))
				break;
			*usedp |= 1U << j;
			*idp = ids[j];
			return (unsigned long *)end;
		}
#endif
	}
	return NULL;
}


static void dump_trace(unsigned long * stack,
		char *buffer,
		size_t *to_base,
		size_t *to,
		const char **from,
		size_t *len,
		lttng_sequence_stack_kernel_dump_eip *obj)
{
	const unsigned cpu = safe_smp_processor_id();
	unsigned long *irqstack_end = (unsigned long *)cpu_pda(cpu)->irqstackptr;
	unsigned used = 0;

#define HANDLE_STACK(cond) \
	do while (cond) { \
		unsigned long addr = *stack++; \
		if (__kernel_text_address(addr)) { \
			/* \
			 * If the address is either in the text segment of the \
			 * kernel, or in the region which contains vmalloc'ed \
			 * memory, it *may* be the address of a calling \
			 * routine; if so, print it so that someone tracing \
			 * down the cause of the crash will be able to figure \
			 * out the call path that was taken. \
			 */ \
			if (buffer != NULL) { \
				unsigned long *dest = (unsigned long*)(buffer+*to_base+*to); \
				*dest = addr; \
				*to += sizeof(unsigned long); \
			} else { \
				obj->len++; \
			} \
		} \
	} while (0)

	const char *id;
	unsigned long *estack_end;
	estack_end = trace_in_exception_stack(cpu, (unsigned long)stack,
					&used, &id);
	while (1) {
		if (estack_end) {
			HANDLE_STACK (stack < estack_end);
			stack = (unsigned long *) estack_end[-2];
			continue;
		}
		if (irqstack_end) {
			unsigned long *irqstack;
			irqstack = irqstack_end -
				(IRQSTACKSIZE - 64) / sizeof(*irqstack);

			if (stack >= irqstack && stack < irqstack_end) {
				HANDLE_STACK (stack < irqstack_end);
				stack = (unsigned long *) (irqstack_end[-1]);
				irqstack_end = NULL;
				continue;
			}
		}
		break;
	}

	HANDLE_STACK (((long) stack & (THREAD_SIZE-1)) != 0);
#undef HANDLE_STACK
}


static inline size_t lttng_get_alignment_sequence_stack_kernel_dump_eip(
		lttng_sequence_stack_kernel_dump_eip *obj)
{
	size_t align=0, localign;
	localign = sizeof(unsigned int);
	align = max(align, localign);

	localign = sizeof(const unsigned long);
	align = max(align, localign);

	return align;
}

static void lttng_write_custom_sequence_stack_kernel_dump_eip(
		char *buffer,
		size_t *to_base,
		size_t *to,
		const char **from,
		size_t *len,
		lttng_sequence_stack_kernel_dump_eip *obj)
{
	size_t align;

	/* Flush pending memcpy */
	if (*len != 0) {
		if (buffer != NULL)
			memcpy(buffer+*to_base+*to, *from, *len);
	}
	*to += *len;
	*len = 0;

	align = lttng_get_alignment_sequence_stack_kernel_dump_eip(obj);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	/* Contains variable sized fields : must explode the structure */

	/* Copy members */
	align = sizeof(unsigned int);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}
	
	*len += sizeof(unsigned int);

	if (buffer != NULL)
		memcpy(buffer+*to_base+*to, &obj->len, *len);
	else {
		/* Just calculate the size of the sequence */
		obj->len = 0;
		/* We use obj, on the caller stack, as a stack pointer */
		dump_trace(obj->stack,
				buffer, to_base, to, from, len, obj);
	}
//size_is_thread:
	*to += *len;
	*len = 0;

	align = sizeof(unsigned long);

	if (*len == 0) {
		*to += ltt_align(*to, align); /* align output */
	} else {
		*len += ltt_align(*to+*len, align); /* alignment, ok to do a memcpy of it */
	}

	*len += sizeof(unsigned long);

	*len = obj->len * (*len);
	if (buffer != NULL) {
		/* Copy the data into the sequence */
		size_t local_to = *to;

		dump_trace(obj->stack,
				buffer, to_base, &local_to, from, len, obj);
		WARN_ON(*to_base+*to+*len != *to_base+local_to);
	}
//write_is_thread:
	*to += *len;
	*len = 0;


	/* Realign the *to_base on arch size, set *to to 0 */
	*to += ltt_align(*to, sizeof(void *));
	*to_base = *to_base+*to;
	*to = 0;
}

#include <ltt/ltt-facility-stack.h>

static inline void trace_process_stack_dump(struct pt_regs *regs)
#ifndef CONFIG_LTT_PROCESS_STACK
{ }
#else
{
	/* FIXME :
	 * Dumping the process stack on x86_64 (no useful frame pointers for
	 * us) has two limitations :
	 * - From the kernel, we cannot know what is code from libraries or
	 *   not (we can only dump function pointers which functions resides
	 *   in the executable).
	 * - For multithreaded applications, the stack of the other threads
	 *   will also be dumped and can change. As they do not necessarily
	 *   have a guard page, we cannot assume that, because they do not
	 *   have a fixed size. The best we can do is to pad missing pointers
	 *   with zeros and make sure that we never go beyond our reserved
	 *   space. */
	if (user_mode_vm(regs)) {
		if (test_thread_flag(TIF_IA32)) {
			lttng_sequence_stack_process_dump_32_eip sequence;
			sequence.len = 0; /* pass regs to the custom write */
			sequence.array = (const uint32_t*)regs;
			trace_stack_process_dump_32(&sequence);
		} else {
			lttng_sequence_stack_process_dump_64_eip sequence;
			sequence.len = 0; /* pass regs to the custom write */
			sequence.array = (const uint64_t*)regs;
			trace_stack_process_dump_64(&sequence);
		}
	}
}
#endif //CONFIG_LTT_PROCESS_STACK

static inline void trace_kernel_stack_dump(struct pt_regs *regs)
#ifndef CONFIG_LTT_KERNEL_STACK
{ }
#else
{
	if (!user_mode_vm(regs)) {
		lttng_sequence_stack_kernel_dump_eip sequence;
		sequence.len = 0;	/* pass regs to the custom write */
		sequence.array = (const unsigned long*)regs;
		sequence.stack = (void*)regs->rsp;
		trace_stack_kernel_dump(&sequence);
	}
}
#endif //CONFIG_LTT_KERNEL_STACK

#endif //_ASM_LTT_FACILITY_CUSTOM_STACK_H_
