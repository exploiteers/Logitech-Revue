#ifndef _ASM_LTT_FACILITY_CUSTOM_STACK_H_
#define _ASM_LTT_FACILITY_CUSTOM_STACK_H_

#include <linux/types.h>
#include <ltt/ltt-tracer.h>
#include <linux/sched.h>

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
		uint32_t addr, next_ebp;
		uint32_t *iter_ebp;
		obj->len = 1;
		
		/* For disabling multithread process */
		//if (!thread_group_leader(current))
		//	goto size_is_thread;

		/* Start at the top of the user stack */
		iter_ebp = (uint32_t*) regs->ebp;

		/* Keep on going until we reach the end of the process' stack limit or find
		 * a invalid ebp. */
		while (!get_user(next_ebp, iter_ebp)) {
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
			//if (addr > current->mm->start_code && addr < current->mm->end_code) {
			//	obj->len++;
			//}
			/* Go on to the next address */
			//stack++;
			/* The next ebp should be upper in the stack */
			if (next_ebp <= (uint32_t)(iter_ebp+1)) break;

			if (get_user(addr, iter_ebp+1)) break;
			//obj->len++;	//test : log ebp too
			obj->len++;

			iter_ebp = (uint32_t*)next_ebp;
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
		uint32_t addr, next_ebp;
		uint32_t *iter_ebp;
		uint32_t *dest = (uint32_t*)(buffer+*to_base+*to);
		
		*dest = regs->eip;
		dest++;

		/* For disabling multithread process */
		//if (!thread_group_leader(current))
		//	goto write_is_thread;

		/* Start at the top of the user stack */
		iter_ebp = (uint32_t*) regs->ebp;
		
		/* Keep on going until we reach the end of the process' stack limit or find
		 * a invalid ebp. */
		while (!get_user(next_ebp, iter_ebp)) {
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
			if (dest == (uint32_t*)(buffer+*to_base+*to) + obj->len) break;
			/* Does this LOOK LIKE an address in the program */
			//if (addr > current->mm->start_code && addr < current->mm->end_code) {
			//	*dest = addr;
			//	dest++;
			//}
			/* Go on to the next address */
			//stack++;
			/* The next ebp should be upper in the stack. If the next ebp is not
			 * valid, we do not log the associated EIP as it's not a valid return
			 * stack. */
			if (next_ebp <= (uint32_t)(iter_ebp+1)) break;

			if (get_user(addr, iter_ebp+1)) break;

			//*dest = next_ebp;	//test : log ebp too
			//dest++;
			*dest = addr;
			dest++;

			iter_ebp = (uint32_t*)next_ebp;
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

static inline void lttng_write_custom_sequence_stack_process_dump_64_eip(
		char *buffer,
		size_t *to_base,
		size_t *to,
		const char **from,
		size_t *len,
		lttng_sequence_stack_process_dump_64_eip *obj)
{
	/* 32 bits arch. */
	BUG();
}


/* Kernel specific trace stack dump routines, from arch/i386/kernel/traps.c */

/* Event kernel_dump structures 
 * ONLY USE IT ON THE CURRENT STACK.
 * It does not protect against other stack modifications and _will_
 * cause corruption upon race between threads. */
typedef struct lttng_sequence_stack_kernel_dump_eip lttng_sequence_stack_kernel_dump_eip;
struct lttng_sequence_stack_kernel_dump_eip {
	unsigned int len;
	const unsigned long *array;
	unsigned long *stack;
	unsigned long ebp;
};


static inline int dump_valid_stack_ptr(struct thread_info *tinfo, void *p)
{
	return	p > (void *)tinfo &&
		p < (void *)tinfo + THREAD_SIZE - 3;
}

static inline unsigned long dump_context_stack(struct thread_info *tinfo,
		unsigned long *stack, unsigned long ebp,
		char *buffer,
		size_t *to_base,
		size_t *to,
		const char **from,
		size_t *len,
		lttng_sequence_stack_kernel_dump_eip *obj)
{
	unsigned long addr;

#ifdef	CONFIG_FRAME_POINTER
	while (dump_valid_stack_ptr(tinfo, (void *)ebp)) {
		if (buffer != NULL) {
			unsigned long *dest = (unsigned long*)(buffer+*to_base+*to);
			addr = *(unsigned long *)(ebp + 4);
			*dest = addr;
			*to += sizeof(unsigned long);
		} else {
			obj->len++;
		}
		ebp = *(unsigned long *)ebp;
	}
#else
	while (dump_valid_stack_ptr(tinfo, stack)) {
		addr = *stack++;
		if (__kernel_text_address(addr)) {
			if (buffer != NULL) {
				unsigned long *dest = (unsigned long*)(buffer+*to_base+*to);
				*dest = addr;
				*to += sizeof(unsigned long);
			} else {
				obj->len++;
			}
		}
	}
#endif
	return ebp;
}

static void dump_trace(struct pt_regs *regs, unsigned long * stack,
		unsigned long ebp,
		char *buffer,
		size_t *to_base,
		size_t *to,
		const char **from,
		size_t *len,
		lttng_sequence_stack_kernel_dump_eip *obj)
{
	//unsigned long ebp;

	//ebp = regs->ebp;

	while (1) {
		struct thread_info *context;
		context = (struct thread_info *)
			((unsigned long)stack & (~(THREAD_SIZE - 1)));
		ebp = dump_context_stack(context, stack, ebp,
				buffer, to_base, to, from, len, obj);
		stack = (unsigned long*)context->previous_esp;
		if (!stack)
			break;
	}
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
		struct pt_regs *regs = (struct pt_regs*)obj->array;
		obj->len = 0;
		dump_trace(regs, obj->stack, obj->ebp,
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
		struct pt_regs *regs = (struct pt_regs*)obj->array;
		size_t local_to = *to;

		dump_trace(regs, obj->stack, obj->ebp,
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
	if (user_mode_vm(regs)) {
		lttng_sequence_stack_process_dump_32_eip sequence;
		sequence.len = 0; /* pass regs to the custom write */
		sequence.array = (const uint32_t*)regs;
		trace_stack_process_dump_32(&sequence);
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
		sequence.stack = (void*)&regs;
		/* Grab ebp right from our regs */
		asm ("movl %%ebp, %0" : "=r" (sequence.ebp) : );
		trace_stack_kernel_dump(&sequence);
	}
}
#endif //CONFIG_LTT_KERNEL_STACK

#endif //_ASM_LTT_FACILITY_CUSTOM_STACK_H_
