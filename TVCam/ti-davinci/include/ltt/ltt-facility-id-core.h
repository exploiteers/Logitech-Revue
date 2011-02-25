#ifndef _LTT_FACILITY_ID_CORE_H_
#define _LTT_FACILITY_ID_CORE_H_

#ifdef CONFIG_LTT
#include <linux/ltt-facilities.h>

/****  facility handle  ****/

extern ltt_facility_t ltt_facility_core_1A8DE486;
extern ltt_facility_t ltt_facility_core;


/****  event index  ****/

enum core_event {
	event_core_facility_load,
	event_core_facility_unload,
	event_core_time_heartbeat,
	event_core_state_dump_facility_load,
	facility_core_num_events
};

#endif //CONFIG_LTT
#endif //_LTT_FACILITY_ID_CORE_H_
