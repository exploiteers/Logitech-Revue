#ifndef _LTT_FACILITY_ID_PROCESS_H_
#define _LTT_FACILITY_ID_PROCESS_H_

#ifdef CONFIG_LTT
#include <linux/ltt-facilities.h>

/****  facility handle  ****/

extern ltt_facility_t ltt_facility_process_2905B6EB;
extern ltt_facility_t ltt_facility_process;


/****  event index  ****/

enum process_event {
	event_process_fork,
	event_process_kernel_thread,
	event_process_exit,
	event_process_wait,
	event_process_free,
	event_process_kill,
	event_process_signal,
	event_process_wakeup,
	event_process_schedchange,
	facility_process_num_events
};

#endif //CONFIG_LTT
#endif //_LTT_FACILITY_ID_PROCESS_H_
