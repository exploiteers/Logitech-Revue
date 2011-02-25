#ifndef _LTT_FACILITY_ID_STACK_H_
#define _LTT_FACILITY_ID_STACK_H_

#ifdef CONFIG_LTT
#include <linux/ltt-facilities.h>

/****  facility handle  ****/

extern ltt_facility_t ltt_facility_stack_C90868B5;
extern ltt_facility_t ltt_facility_stack;


/****  event index  ****/

enum stack_event {
	event_stack_process_dump_32,
	event_stack_process_dump_64,
	event_stack_kernel_dump,
	facility_stack_num_events
};

#endif //CONFIG_LTT
#endif //_LTT_FACILITY_ID_STACK_H_
