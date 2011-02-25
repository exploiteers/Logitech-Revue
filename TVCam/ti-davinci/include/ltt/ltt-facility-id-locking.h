#ifndef _LTT_FACILITY_ID_LOCKING_H_
#define _LTT_FACILITY_ID_LOCKING_H_

#ifdef CONFIG_LTT
#include <linux/ltt-facilities.h>

/****  facility handle  ****/

extern ltt_facility_t ltt_facility_locking_89053D28;
extern ltt_facility_t ltt_facility_locking;


/****  event index  ****/

enum locking_event {
	event_locking_hardirqs_on,
	event_locking_hardirqs_off,
	event_locking_softirqs_on,
	event_locking_softirqs_off,
	event_locking_lock_acquire,
	event_locking_lock_release,
	facility_locking_num_events
};

#endif //CONFIG_LTT
#endif //_LTT_FACILITY_ID_LOCKING_H_
