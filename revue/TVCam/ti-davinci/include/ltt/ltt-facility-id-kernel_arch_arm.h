#ifndef _LTT_FACILITY_ID_KERNEL_ARCH_H_
#define _LTT_FACILITY_ID_KERNEL_ARCH_H_

#ifdef CONFIG_LTT
#include <linux/ltt-facilities.h>

/****  facility handle  ****/

extern ltt_facility_t ltt_facility_kernel_arch_93C7DAA1;
extern ltt_facility_t ltt_facility_kernel_arch;


/****  event index  ****/

enum kernel_arch_event {
	event_kernel_arch_syscall_entry,
	event_kernel_arch_syscall_exit,
	facility_kernel_arch_num_events
};

#endif //CONFIG_LTT
#endif //_LTT_FACILITY_ID_KERNEL_ARCH_H_
