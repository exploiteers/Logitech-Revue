/*  linux/include/linux/clockchips.h
 *
 *  This file contains the structure definitions for clockchips.
 *
 *  If you are not a clockchip, or the time of day code, you should
 *  not be including this file!
 */
#ifndef _LINUX_CLOCKCHIPS_H
#define _LINUX_CLOCKCHIPS_H

#ifdef CONFIG_GENERIC_CLOCKEVENTS

#include <linux/clocksource.h>
#include <linux/interrupt.h>

struct clock_event_device;

/* Clock event mode commands */
enum clock_event_mode {
	CLOCK_EVT_PERIODIC,
	CLOCK_EVT_ONESHOT,
	CLOCK_EVT_SHUTDOWN,
};

/*
 * Clock event capability flags:
 *
 * CAP_TICK:	The event source should be used for the periodic tick
 * CAP_UPDATE:	The event source handler should call update_process_times()
 * CAP_PROFILE: The event source handler should call profile_tick()
 * CAP_NEXTEVT:	The event source can be reprogrammed in oneshot mode and is
 *		a per cpu event source.
 *
 * The capability flags are used to select the appropriate handler for an event
 * source. On an i386 UP system the PIT can serve all of the functionalities,
 * while on a SMP system the PIT is solely used for the periodic tick and the
 * local APIC timers are used for UPDATE / PROFILE / NEXTEVT. To avoid the run
 * time query of those flags, the clock events layer assigns the appropriate
 * event handler function, which contains only the selected calls, to the
 * event.
 */
#define CLOCK_CAP_TICK		0x000001
#define CLOCK_CAP_UPDATE	0x000002
#define CLOCK_CAP_PROFILE	0x000004
#ifdef CONFIG_HIGH_RES_TIMERS
# define CLOCK_CAP_NEXTEVT	0x000008
#else
# define CLOCK_CAP_NEXTEVT	0x000000
#endif

#define CLOCK_BASE_CAPS_MASK	(CLOCK_CAP_TICK | CLOCK_CAP_PROFILE | \
				 CLOCK_CAP_UPDATE)
#define CLOCK_CAPS_MASK		(CLOCK_BASE_CAPS_MASK | CLOCK_CAP_NEXTEVT)

/**
 * struct clock_event_device - clock event descriptor
 *
 * @name:		ptr to clock event name
 * @capabilities:	capabilities of the event chip
 * @max_delta_ns:	maximum delta value in ns
 * @min_delta_ns:	minimum delta value in ns
 * @mult:		nanosecond to cycles multiplier
 * @shift:		nanoseconds to cycles divisor (power of two)
 * @set_next_event:	set next event
 * @set_mode:		set mode function
 * @suspend:		suspend function (optional)
 * @resume:		resume function (optional)
 * @evthandler:		Assigned by the framework to be called by the low
 *			level handler of the event source
 */
struct clock_event_device {
	const char	*name;
	unsigned int	capabilities;
	unsigned long	max_delta_ns;
	unsigned long	min_delta_ns;
	unsigned long	mult;
	int		shift;
	void		(*set_next_event)(unsigned long evt,
					  struct clock_event_device *);
	void		(*set_mode)(enum clock_event_mode mode,
				    struct clock_event_device *);
	void		(*event_handler)(struct pt_regs *regs);
};

/*
 * Calculate a multiplication factor for scaled math, which is used to convert
 * nanoseconds based values to clock ticks:
 *
 * clock_ticks = (nanoseconds * factor) >> shift.
 *
 * div_sc is the rearranged equation to calculate a factor from a given clock
 * ticks / nanoseconds ratio:
 *
 * factor = (clock_ticks << shift) / nanoseconds
 */
static inline unsigned long div_sc(unsigned long ticks, unsigned long nsec,
				   int shift)
{
	uint64_t tmp = ((uint64_t)ticks) << shift;

	do_div(tmp, nsec);
	return (unsigned long) tmp;
}

/* Clock event layer functions */
extern int register_local_clockevent(struct clock_event_device *);
extern int register_global_clockevent(struct clock_event_device *);
extern unsigned long clockevent_delta2ns(unsigned long latch,
					 struct clock_event_device *evt);
extern void clockevents_init(void);

extern int clockevents_init_next_event(void);
extern int clockevents_set_next_event(ktime_t expires, int force);
extern int clockevents_next_event_available(void);
extern void clockevents_resume_events(void);

#else
# define clockevents_init()		do { } while(0)
# define clockevents_resume_events()	do { } while(0)
#endif

#endif
