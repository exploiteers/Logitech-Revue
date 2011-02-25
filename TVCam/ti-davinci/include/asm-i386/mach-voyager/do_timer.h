/* defines for inline arch setup functions */
#include <linux/clockchips.h>

#include <asm/voyager.h>
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
	voyager_timer_interrupt(regs);
}

static inline int do_timer_overflow(int count)
{
	/* can't read the ISR, just assume 1 tick
	   overflow */
	if(count > LATCH || count < 0) {
		printk(KERN_ERR "VOYAGER PROBLEM: count is %d, latch is %d\n", count, LATCH);
		count = LATCH;
	}
	count -= LATCH;

	return count;
}
