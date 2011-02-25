#ifndef _LTT_FACILITY_ID_KERNEL_H_
#define _LTT_FACILITY_ID_KERNEL_H_

#ifdef CONFIG_LTT
#include <linux/ltt-facilities.h>

/****  facility handle  ****/

extern ltt_facility_t ltt_facility_kernel_25D21CBD;
extern ltt_facility_t ltt_facility_kernel;


/****  event index  ****/

enum kernel_event {
	event_kernel_trap_entry,
	event_kernel_trap_exit,
	event_kernel_soft_irq_entry,
	event_kernel_soft_irq_exit,
	event_kernel_tasklet_entry,
	event_kernel_tasklet_exit,
	event_kernel_irq_entry,
	event_kernel_irq_exit,
	event_kernel_printk,
	event_kernel_vprintk,
	event_kernel_module_free,
	event_kernel_module_load,
	facility_kernel_num_events
};

#endif //CONFIG_LTT
#endif //_LTT_FACILITY_ID_KERNEL_H_
