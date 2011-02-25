#ifndef _LTT_FACILITY_ID_TIMER_H_
#define _LTT_FACILITY_ID_TIMER_H_

#ifdef CONFIG_LTT
#include <linux/ltt-facilities.h>

/****  facility handle  ****/

extern ltt_facility_t ltt_facility_timer_DBFF2E10;
extern ltt_facility_t ltt_facility_timer;


/****  event index  ****/

enum timer_event {
	event_timer_expired,
	event_timer_softirq,
	event_timer_set_itimer,
	event_timer_set_timer,
	event_timer_update_time,
	facility_timer_num_events
};

#endif //CONFIG_LTT
#endif //_LTT_FACILITY_ID_TIMER_H_
