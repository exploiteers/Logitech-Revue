#ifndef __ASM_I8253_H__
#define __ASM_I8253_H__

#include <linux/clockchips.h>

extern raw_spinlock_t i8253_lock;

#ifdef CONFIG_HPET_TIMER
extern struct clock_event_device *global_clock_event;
#else
extern struct clock_event_device pit_clockevent;
# define global_clock_event (&pit_clockevent)
#endif

/**
 * pit_interrupt_hook - hook into timer tick
 * @regs:	standard registers from interrupt
 *
 * Call the global clock event handler.
 **/
static inline void pit_interrupt_hook(struct pt_regs *regs)
{
	global_clock_event->event_handler(regs);
}

#endif	/* __ASM_I8253_H__ */
