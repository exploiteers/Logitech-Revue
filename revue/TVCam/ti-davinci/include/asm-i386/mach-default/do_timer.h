/* defines for inline arch setup functions */
#include <linux/clockchips.h>

#include <asm/i8259.h>
#include <asm/i8253.h>

/**
 * do_timer_interrupt_hook - hook into timer tick
 * @regs:	standard registers from interrupt
 *
 * Call the pit clock event handler. see asm/i8253.h
 **/
static inline void do_timer_interrupt_hook(struct pt_regs *regs)
{
	pit_interrupt_hook(regs);
}

/* you can safely undefine this if you don't have the Neptune chipset */

#define BUGGY_NEPTUN_TIMER

/**
 * do_timer_overflow - process a detected timer overflow condition
 * @count:	hardware timer interrupt count on overflow
 *
 * Description:
 *	This call is invoked when the jiffies count has not incremented but
 *	the hardware timer interrupt has.  It means that a timer tick interrupt
 *	came along while the previous one was pending, thus a tick was missed
 **/
static inline int do_timer_overflow(int count)
{
	int i;

	spin_lock(&i8259A_lock);
	/*
	 * This is tricky when I/O APICs are used;
	 * see do_timer_interrupt().
	 */
	i = inb(0x20);
	spin_unlock(&i8259A_lock);
	
	/* assumption about timer being IRQ0 */
	if (i & 0x01) {
		/*
		 * We cannot detect lost timer interrupts ... 
		 * well, that's why we call them lost, don't we? :)
		 * [hmm, on the Pentium and Alpha we can ... sort of]
		 */
		count -= LATCH;
	} else {
#ifdef BUGGY_NEPTUN_TIMER
		/*
		 * for the Neptun bug we know that the 'latch'
		 * command doesn't latch the high and low value
		 * of the counter atomically. Thus we have to 
		 * substract 256 from the counter 
		 * ... funny, isnt it? :)
		 */
		
		count -= 256;
#else
		printk("do_slow_gettimeoffset(): hardware timer problem?\n");
#endif
	}
	return count;
}
