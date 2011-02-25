#ifndef _LTT_FACILITY_ID_MEMORY_H_
#define _LTT_FACILITY_ID_MEMORY_H_

#ifdef CONFIG_LTT
#include <linux/ltt-facilities.h>

/****  facility handle  ****/

extern ltt_facility_t ltt_facility_memory_7F007B54;
extern ltt_facility_t ltt_facility_memory;


/****  event index  ****/

enum memory_event {
	event_memory_page_alloc,
	event_memory_page_free,
	event_memory_swap_in,
	event_memory_swap_out,
	event_memory_page_wait_start,
	event_memory_page_wait_end,
	event_memory_page_fault,
	facility_memory_num_events
};

#endif //CONFIG_LTT
#endif //_LTT_FACILITY_ID_MEMORY_H_
