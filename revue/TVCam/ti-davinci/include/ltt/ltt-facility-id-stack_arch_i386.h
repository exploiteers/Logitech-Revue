#ifndef _LTT_FACILITY_ID_STACK_ARCH_H_
#define _LTT_FACILITY_ID_STACK_ARCH_H_

#ifdef CONFIG_LTT
#include <linux/ltt-facilities.h>

/****  facility handle  ****/

extern ltt_facility_t ltt_facility_stack_arch_C67264DD;
extern ltt_facility_t ltt_facility_stack_arch;


/****  event index  ****/

enum stack_arch_event {
	event_stack_arch_process_dump,
	event_stack_arch_kernel_dump,
	facility_stack_arch_num_events
};

#endif //CONFIG_LTT
#endif //_LTT_FACILITY_ID_STACK_ARCH_H_
